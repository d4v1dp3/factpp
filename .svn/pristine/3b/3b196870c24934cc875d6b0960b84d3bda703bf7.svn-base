#include <boost/array.hpp>

#include "FACT.h"
#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Timers.h"
#include "Console.h"

#include "tools.h"

#include "HeadersMagicLidar.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace MagicLidar;

// ------------------------------------------------------------------------

class ConnectionLidar : public Connection
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

    virtual void UpdateLidar(const Time &, const DimLidar &)
    {
    }

protected:

    boost::array<char, 4096> fArray;

    Time fLastReport;
    Time fLastReception;

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

        fLastReception = Time();

        const string str(fArray.data(), bytes_received);
        memset(fArray.data(), 0, fArray.size());

        if (fIsVerbose)
            Out() << str << endl;

        bool isheader = true;

        DimLidar data;

        int hh=0, mm=0, ss=0, y=0, m=0, d=0;

        bool keepalive = false;
        bool failed = false;

        stringstream is(str);
        string line;
        while (getline(is, line))
        {
            if (line.size()==1 && line[0]==13)
            {
                isheader = false;
                continue;
            }

            if (isheader)
            {
                const size_t p = line.find_first_of(": ");
                if (p==string::npos)
                    continue;

                std::transform(line.begin(), line.end(), line.begin(), (int(&)(int))std::tolower);

                const string key = line.substr(0, p);
                const string val = line.substr(p+2);

                if (key=="connection" && val=="keep-alive")
                    keepalive = true;
            }
            else
            {
                try
                {
                    if (line.substr(0, 2)=="ZD")
                        data.fZd = stoi(line.substr(2));
                    if (line.substr(0, 2)=="AZ")
                        data.fAz = stof(line.substr(2));

                    //if (line.substr(0, 3)=="PBL")
                    //    data.fPBL = stof(line.substr(3));
                    //if (line.substr(0, 3)=="CHE")
                    //    data.fCHE = stof(line.substr(3));
                    //if (line.substr(0, 3)=="COT")
                    //    data.fCOT = stof(line.substr(3));

                    if (line.substr(0, 2)=="T3")
                        data.fT3 = stof(line.substr(2));
                    if (line.substr(0, 2)=="T6")
                        data.fT6 = stof(line.substr(2));
                    if (line.substr(0, 2)=="T9")
                        data.fT9 = stof(line.substr(2));
                    if (line.substr(0, 3)=="T12")
                        data.fT12 = stof(line.substr(3));

                    if (line.substr(0, 3)=="CLB")
                        data.fCloudBaseHeight = stof(line.substr(3));

                    if (line.substr(0, 4)=="HOUR")
                        hh = stoi(line.substr(4));
                    if (line.substr(0, 6)=="MINUTS")
                        mm = stoi(line.substr(6));
                    if (line.substr(0, 7)=="SECONDS")
                        ss = stoi(line.substr(7));

                    if (line.substr(0, 4)=="YEAR")
                        y = stoi(line.substr(4));
                    if (line.substr(0, 5)=="MONTH")
                        m = stoi(line.substr(5));
                    if (line.substr(0, 3)=="DAY")
                        d = stoi(line.substr(3));
                }
                catch (const exception &e)
                {
                    Warn("Conversion of received data failed");
                    failed = true;
                    break;
                }
            }
        }

        if (!keepalive)
            PostClose(false);

        if (failed)
            return;

        try
        {
            const Time tm = Time(y>999 ? y : 2000+y, m, d, hh, mm, ss);
            if (tm==fLastReport)
                return;

            fLastReport = tm;

            if (data.fT3==0 && data.fT6==0 && data.fT9==0 && data.fT12==0)
                return;

            ostringstream msg;
            msg << tm.GetAsStr("%H:%M:%S") << ":"
                //<< " PBL=" << data.fPBL
                //<< " CHE=" << data.fCHE
                //<< " COT=" << data.fCOT
                << " T3-12=" << data.fT3
                << "/" << data.fT6
                << "/" << data.fT9
                << "/" << data.fT12
                << " H="  << data.fCloudBaseHeight/1000 << "km"
                << " Zd="  << data.fZd  << "\u00b0"
                << " Az="  << data.fAz  << "\u00b0";
            Message(msg);

            UpdateLidar(tm, data);
        }
        catch (const exception &e)
        {
            Warn("Corrupted time received.");
        }

    }

    void StartReadReport()
    {
        async_read_some(ba::buffer(fArray),
                        boost::bind(&ConnectionLidar::HandleRead, this,
                                    dummy::error, dummy::bytes_transferred));
    }

    boost::asio::deadline_timer fKeepAlive;

    void PostRequest()
    {
        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "Accept: */*\r\n"
            "Content-Type: application/octet-stream\r\n"
            "User-Agent: FACT\r\n"
            "Host: www.fact-project.org\r\n"
            "Pragma: no-cache\r\n"
            "Cache-Control: no-cache\r\n"
            "Expires: 0\r\n"
            "Connection: Keep-Alive\r\n"
            "Cache-Control: max-age=0\r\n"
            "\r\n";

        PostMessage(cmd);
    }

    void Request()
    {
        PostRequest();

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval/2));
        fKeepAlive.async_wait(boost::bind(&ConnectionLidar::HandleRequest,
                                          this, dummy::error));
    }

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

        Request();
    }


private:
    // This is called when a connection was established
    void ConnectionEstablished()
    {
        Request();
        StartReadReport();
    }

public:

    static const uint16_t kMaxAddr;

public:
    ConnectionLidar(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fLastReport(Time::none), fLastReception(Time::none), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
        Connection::SetVerbose(b);
    }

    void SetInterval(uint16_t i)
    {
        fInterval = i;
    }

    void SetSite(const string &site)
    {
        fSite = site;
    }

    int GetState() const
    {
        if (fLastReport.IsValid() && fLastReport+boost::posix_time::seconds(fInterval*2)>Time())
            return 3;

        if (fLastReception.IsValid() && fLastReception+boost::posix_time::seconds(fInterval*2)>Time())
            return 2;

        return 1;

    }
};

const uint16_t ConnectionLidar::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimLidar : public ConnectionLidar
{
private:

    DimDescribedService fDimLidar;

    virtual void UpdateLidar(const Time &t, const DimLidar &data)
    {
        fDimLidar.setData(&data, sizeof(DimLidar));
        fDimLidar.Update(t);
    }

public:
    ConnectionDimLidar(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionLidar(ioservice, imp),
        fDimLidar("MAGIC_LIDAR/DATA", "F:1;F:1;F:1;F:1;F:1;F:1;F:1",
                  "|Zd[deg]:Pointing direction zenith distance"
                  "|Az[deg]:Pointing direction azimuth"
                  "|T3[1]:Transmission below 3km normalized to 1"
                  "|T6[1]:Transmission below 6km normalized to 1"
                  "|T9[1]:Transmission below 9km normalized to 1"
                  "|T12[1]:Transmission below 12km normalized to 1"
                  "|CLB[m]:Cloud base height")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineLidar : public StateMachineAsio<T>
{
private:
    S fLidar;

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fLidar.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fLidar.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fLidar.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        poll();

        if (evt.GetBool())
            fLidar.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fLidar.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fLidar.GetState();
    }


public:
    StateMachineLidar(ostream &out=cout) :
        StateMachineAsio<T>(out, "MAGIC_LIDAR"), fLidar(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(State::kConnected, "Invalid",
                     "Connection to webserver can be established, but received data is not recent or invalid");

        T::AddStateName(State::kReceiving, "Valid",
                     "Connection to webserver can be established, receint data received");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineLidar::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineLidar::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineLidar::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        fLidar.SetVerbose(!conf.Get<bool>("quiet"));
        fLidar.SetInterval(conf.Get<uint16_t>("interval"));
        fLidar.SetDebugTx(conf.Get<bool>("debug-tx"));
        fLidar.SetSite(conf.Get<string>("url"));
        fLidar.SetEndpoint(conf.Get<string>("addr"));
        fLidar.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineLidar<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("MAGIC lidar control options");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("www.magic.iac.es:80"),  "Network address of Cosy")
        ("url,u",  var<string>("/site/weather/lidar_data.txt"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(30), "Interval between two updates on the server in seconds")
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
        "The magiclidar is an interface to the MAGIC lidar data.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: magiclidar [-c type] [OPTIONS]\n"
        "  or:  magiclidar [OPTIONS]\n";
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

    //try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
            if (conf.Get<bool>("no-dim"))
                return RunShell<LocalStream, StateMachine, ConnectionLidar>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimLidar>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionLidar>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionLidar>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimLidar>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimLidar>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
