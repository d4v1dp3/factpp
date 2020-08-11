#include "FACT.h"
#include "Dim.h"
#include "Event.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Console.h"

#include "tools.h"

#include "HeadersGPS.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionGPS : public Connection
{
protected:
    virtual void Update(const GPS::NEMA &)
    {
    }

private:
    bool fIsVerbose;

    Time fLastReport;

    int fState;

    float ConvLngLat(const string &l) const
    {
        const double   lf = stof(l);
        const uint32_t li = stoi(l);

        const double min = fmod(lf, 100);
        const double deg = li/100;

        return deg + min/60;
    }

    float ConvTm(const string &t) const
    {
        const double   tf = stof(t);
        const uint32_t ti = stoi(t);

        const double h = ti/10000;
        const double m = (ti/100)%100;
        const double s = fmod(tf, 100);

        return h/24 + m/1440 + s/86400;
    }

    bool ParseAnswer(const string &buffer)
    {
        if (buffer=="Invalid command, type help")
        {
            Error("Command was ignored by GPS.");
            return false;
        }

        // answer to get_status or veto_[on|off|60]
        if (buffer=="veto_60" || buffer=="veto 60 now on")
        {
            if (fState!=GPS::State::kLocked)
                fState = GPS::State::kEnabled;
            PostMessage(string("get_nema\r\n"), 10);
            return true;
        }
        if (buffer=="veto_on" || buffer=="veto now on")
        {
            fState = GPS::State::kDisabled;
            PostMessage(string("get_nema\r\n"), 10);
            return true;
        }
        /*
        if (buffer=="veto_off" || buffer=="veto now off")
        {
            fState = GPS::State::kVetoOff;
            PostMessage(string("get_nema\r\n"), 10);
            return true;
        }*/

        // answer to get_nema
        if (buffer[0]=='$')
        {
            /*
             1    = UTC of Position
             2    = Latitude
             3    = N or S
             4    = Longitude
             5    = E or W
             6    = GPS quality indicator (0=invalid; 1=GPS fix;
                    2=Diff. GPS fix)
             7    = Number of satellites in use [not those in view]
             8    = Horizontal dilution of position
             9    = Antenna altitude above/below mean sea level (geoid)
             10   = Meters  (Antenna height unit)
             11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid
                    and mean sea level.  -=geoid is below WGS-84 ellipsoid)
             12   = Meters  (Units of geoidal separation)
             13   = Age in seconds since last update from diff.
                    reference station
             14   = Diff. reference station ID#
             */

            const vector<string> cs = Tools::Split(buffer, "$*");
            if (cs.size()!=3)
                throw runtime_error("syntax error");

            // check checksum
            uint8_t c = cs[1][0];
            for (size_t i=1; i<cs[1].size(); i++)
                c ^= cs[1][i];

            stringstream ss;
            ss << std::hex << cs[2];

            unsigned int x;
            ss >> x;

            if (x!=c)
                throw runtime_error("checksum error");

            // interpret contents
            const vector<string> dat = Tools::Split(cs[1], ",");
            if (dat.size()!=15)
                throw runtime_error("size mismatch");
            if (dat[0]!="GPGGA")
                throw runtime_error("type mismatch");
            if (dat[5]!="W" && dat[5]!="E")
                throw runtime_error("longitude type unknown");
            if (dat[10]!="M")
                throw runtime_error("height unit unknown");
            if (dat[12]!="M")
                throw runtime_error("hdop unit unknown");
            if (!dat[13].empty())
                throw runtime_error("unexpected data at position 13");
            if (dat[14]!="0000")
                throw runtime_error("unexpected data at position 14");

            GPS::NEMA nema;
            nema.time   = ConvTm(dat[1]);
            nema.lat    = dat[3]=="N" ? ConvLngLat(dat[2]) : -ConvLngLat(dat[3]);
            nema.lng    = dat[5]=="W" ? ConvLngLat(dat[4]) : -ConvLngLat(dat[4]);
            nema.qos    = stoi(dat[6]);
            nema.count  = stoi(dat[7]);
            nema.hdop   = stof(dat[8]);
            nema.height = stof(dat[9]);
            nema.geosep = stof(dat[11]);

            if (fabs(nema.time-fmod(Time().Mjd(), 1))*24*3600>5)
            {
                Error("Time mismatch: GPS time deviates from PC time by more than 5s");
                return false;
            }

            if (fState>=GPS::State::kEnabled)
                fState = nema.qos==1 ? GPS::State::kLocked : GPS::State::kEnabled;

            Update(nema);

            return true;
        }

        return false;
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host.");

            // 107: Transport endpoint is not connected (bs::error_code(107, bs::system_category))
            // 125: Operation canceled
            if (err && err!=ba::error::eof &&                     // Connection closed by remote host
                err!=ba::error::basic_errors::not_connected &&    // Connection closed by remote host
                err!=ba::error::basic_errors::operation_aborted)  // Connection closed by us
            {
                ostringstream str;
                str << "Reading from " << URL() << ": " << err.message() << " (" << err << ")";// << endl;
                Error(str);
            }
            PostClose(err!=ba::error::basic_errors::operation_aborted);
            return;
        }

        istream is(&fBuffer);

        string buffer;
        if (!getline(is, buffer, '\n'))
        {
            Error("Received message does not contain \\n... closing connection.");
            PostClose(false);
            return;
        }
        buffer = buffer.substr(0, buffer.size()-1);

        if (fIsVerbose)
            Out() << buffer << endl;

        try
        {
            if (!ParseAnswer(buffer))
            {
                Error("Received: "+buffer);
                PostClose(false);
                return;
            }
        }
        catch (const exception &e)
        {
            Error("Parsing NEMA message failed ["+string(e.what())+"]");
            Error("Received: "+buffer);
            PostClose(false);
            return;
        }

        fLastReport = Time();
        StartReadReport();
    }

    boost::asio::streambuf fBuffer;

    void StartReadReport()
    {
        async_read_until(*this, fBuffer, '\n',
                         boost::bind(&ConnectionGPS::HandleRead, this,
                                     dummy::error, dummy::bytes_transferred));
    }

    boost::asio::deadline_timer fKeepAlive;

    void HandleRequest(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Write timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            PostClose(true);
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fKeepAlive.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        PostMessage(string("get_status\r\n"), 12);
        Request();
    }


private:
    // This is called when a connection was established
    void ConnectionEstablished()
    {
        fState = GPS::State::kConnected;

        StartReadReport();
        Request(true);
    }

public:
    static const uint16_t kMaxAddr;

public:
    ConnectionGPS(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fLastReport(Time::none), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
        Connection::SetVerbose(b);
    }

    void Request(bool immediate=false)
    {
        double mjd = Time().Mjd();

        if (!immediate)
            mjd = (ceil(mjd*24*60+0.01)+0.5)/(24*60);

        fKeepAlive.expires_at(Time(mjd));
        fKeepAlive.async_wait(boost::bind(&ConnectionGPS::HandleRequest,
                                          this, dummy::error));
    }

    int GetState() const
    {
        if (!IsConnected())
            return GPS::State::kDisconnected;

        if (fState!=GPS::State::kConnected && fLastReport+boost::posix_time::seconds(105) < Time())
            return StateMachineImp::kSM_Error;

        return fState;
    }
};

const uint16_t ConnectionGPS::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionGPS
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionGPS(ioservice, imp),
        fDim("GPS_CONTROL/NEMA", "F:1;F:1;F:1;F:1;F:1;F:1;S:1;S:1",
             "NEMA message from the GPS module"
             "|time[utc]:Time of day as fraction of day (UTC)"
             "|lat[deg]:Latitude"
             "|long[deg]:Longitude"
             "|hdop:Horizontal delution of precision"
             "|height[m]:Antenna altitude above mean sea level (geoid)"
             "|geosep[m]:Geoidal sep.(Diff. between WGS-84 earth ellipsoid and mean sea lvl)"
             "|count:Number of satellites in use (not those in view)"
             "|quality:GPS quality indicator (see Venus manual)")
    {
    }
    void Update(const GPS::NEMA &nema)
    {
        fDim.Update(nema);
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineGPSControl : public StateMachineAsio<T>
{
private:
    S fGPS;
    Time fLastCommand;

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int Disconnect()
    {
        // Close all connections
        fGPS.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fGPS.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fGPS.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fGPS.PostClose(true);

        return T::GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fGPS.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int Send(const string &cmd)
    {
        const string tx = cmd+"\r\n";
        fGPS.PostMessage(tx, tx.size());
        return T::GetCurrentState();
    }

    int SendCommand(const EventImp &evt)
    {
        return Send(evt.GetString());
    }

    int Execute()
    {
        return fGPS.GetState();
    }


public:
    StateMachineGPSControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "GPS_CONTROL"), fGPS(*this, *this)
    {
        // State names
        T::AddStateName(GPS::State::kDisconnected, "Disconnected",
                        "No connection to web-server could be established recently");

        T::AddStateName(GPS::State::kConnected, "Connected",
                        "Connection established, but status still not known");

        T::AddStateName(GPS::State::kDisabled, "Disabled",
                        "Veto is on, no trigger will be emitted");

        T::AddStateName(GPS::State::kEnabled, "Enabled",
                        "System enabled, waiting for satellites");

        T::AddStateName(GPS::State::kLocked, "Locked",
                        "One trigger per second will be send, but the one at the exact minute is vetoed");

        // Commands
        T::AddEvent("SEND_COMMAND", "C")
            (bind(&StateMachineGPSControl::SendCommand, this, placeholders::_1))
            ("Send command to GPS");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineGPSControl::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("ENABLE")
            (bind(&StateMachineGPSControl::Send, this, "veto_60"))
            ("Enable trigger signal once a second vetoed at every exact minute");

        T::AddEvent("DISABLE")
            (bind(&StateMachineGPSControl::Send, this, "veto_on"))
            ("Diable trigger output");

        // Conenction commands
        T::AddEvent("DISCONNECT")
            (bind(&StateMachineGPSControl::Disconnect, this))
            ("disconnect from ethernet");

         T::AddEvent("RECONNECT", "O")
            (bind(&StateMachineGPSControl::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to GPS, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

    }

    int EvalOptions(Configuration &conf)
    {
        fGPS.SetVerbose(!conf.Get<bool>("quiet"));
        fGPS.SetDebugTx(conf.Get<bool>("debug-tx"));
        fGPS.SetEndpoint(conf.Get<string>("addr"));
        fGPS.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineGPSControl<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("GPS control");
    control.add_options()
        ("no-dim,d", po_switch(), "Disable dim services")
        ("addr,a",   var<string>("gps:23"), "Network address of the lid controling Arduino including port")
        ("quiet,q",  po_bool(true), "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("debug-tx", po_bool(), "Enable debugging of ethernet transmission.")
        ;

    conf.AddOptions(control);
}

/*
 Extract usage clause(s) [if any] for SYNOPSIS.
 Translators: "Usage" and "or" here are patterns (regular expressions) which
 are used to match the usage synopsis in program output.  An example from cp
 (GNU coreutils) which contains both strings:
  Usage: cp [OPTION]... [-T] SOURCE DEST
    or:  cp [OPTION]... SOURCE... DIRECTORY
    or:  cp [OPTION]... -t DIRECTORY SOURCE...
 */
void PrintUsage()
{
    cout <<
        "The gpsctrl is an interface to the GPS hardware.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: gpsctrl [-c type] [OPTIONS]\n"
        "  or:  gpsctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
//    Main::PrintHelp<StateMachineFTM<StateMachine, ConnectionFTM>>();

    /* Additional help text which is printed after the configuration
     options goes here */

    /*
     cout << "bla bla bla" << endl << endl;
     cout << endl;
     cout << "Environment:" << endl;
     cout << "environment" << endl;
     cout << endl;
     cout << "Examples:" << endl;
     cout << "test exam" << endl;
     cout << endl;
     cout << "Files:" << endl;
     cout << "files" << endl;
     cout << endl;
     */
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    // No console access at all
    if (!conf.Has("console"))
    {
        if (conf.Get<bool>("no-dim"))
            return RunShell<LocalStream, StateMachine, ConnectionGPS>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionGPS>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionGPS>(conf);
    }
    else
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachineDim, ConnectionDimWeather>(conf);
        else
            return RunShell<LocalConsole, StateMachineDim, ConnectionDimWeather>(conf);
    }

    return 0;
}
