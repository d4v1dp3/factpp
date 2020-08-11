#include <boost/array.hpp>

#include <string>    // std::string
#include <algorithm> // std::transform
#include <cctype>    // std::tolower

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

#include "HeadersMagicWeather.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace MagicWeather;

// ------------------------------------------------------------------------

class ConnectionWeather : public Connection
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

    virtual void UpdateWeather(const Time &, const DimWeather &)
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

        DimWeather data;

        int hh=0, mm=0, ss=0, y=0, m=0, d=0;

        bool keepalive = false;

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
                if (line.substr(0, 2)=="ST")
                    data.fStatus = stoi(line.substr(2));

                if (line.substr(0, 2)=="TE")
                    data.fTemp = stof(line.substr(2));

                if (line.substr(0, 2)=="DP")
                    data.fDew = stof(line.substr(2));

                if (line.substr(0, 3)=="HUM")
                    data.fHum = stof(line.substr(3));

                if (line.substr(0, 2)=="WS")
                    data.fWind = stof(line.substr(2));

                if (line.substr(0, 3)=="MWD")
                    data.fDir = stof(line.substr(3));

                if (line.substr(0, 2)=="WP")
                    data.fGusts = stof(line.substr(2));

                if (line.substr(0, 5)=="PRESS")
                    data.fPress = stof(line.substr(5));

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
        }

        if (!keepalive)
            PostClose(false);

        try
        {
            const Time tm = Time(2000+y, m, d, hh, mm, ss);
            if (tm==fLastReport)
                return;

            ostringstream msg;
            msg << tm.GetAsStr("%H:%M:%S") << "[" << data.fStatus << "]:"
                << " T="    << data.fTemp  << "\u00b0C"
                << " H="    << data.fHum   << "%"
                << " P="    << data.fPress << "hPa"
                << " Td="   << data.fDew   << "\u00b0C"
                << " V="    << data.fWind  << "km/h"
                << " Vmax=" << data.fGusts << "km/h"
                << " dir="  << data.fDir   << "\u00b0";
            Message(msg);

            UpdateWeather(tm, data);

            fLastReport = tm;
        }
        catch (const exception &e)
        {
            Warn("Corrupted time received.");
        }

    }

    void StartReadReport()
    {
        async_read_some(ba::buffer(fArray),
                        boost::bind(&ConnectionWeather::HandleRead, this,
                                    dummy::error, dummy::bytes_transferred));
    }

    ba::deadline_timer fKeepAlive;

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
        fKeepAlive.async_wait(boost::bind(&ConnectionWeather::HandleRequest,
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
    ConnectionWeather(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
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

const uint16_t ConnectionWeather::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionWeather
{
private:

    DimDescribedService fDimWeather;

    virtual void UpdateWeather(const Time &t, const DimWeather &data)
    {
        fDimWeather.setData(&data, sizeof(DimWeather));
        fDimWeather.Update(t);
    }

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionWeather(ioservice, imp),
        fDimWeather("MAGIC_WEATHER/DATA", "S:1;F:1;F:1;F:1;F:1;F:1;F:1;F:1",
                     "|stat:Status"
                     "|T[deg C]:Temperature"
                     "|T_dew[deg C]:Dew point"
                     "|H[%]:Humidity"
                     "|P[hPa]:Air pressure"
                     "|v[km/h]:Wind speed"
                     "|v_max[km/h]:Wind gusts"
                     "|d[deg]:Wind direction (N-E)")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineWeather : public StateMachineAsio<T>
{
private:
    S fWeather;

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

        fWeather.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fWeather.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fWeather.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fWeather.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fWeather.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fWeather.GetState();
    }

public:
    StateMachineWeather(ostream &out=cout) :
        StateMachineAsio<T>(out, "MAGIC_WEATHER"), fWeather(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(State::kConnected, "Invalid",
                     "Connection to webserver can be established, but received data is not recent or invalid");

        T::AddStateName(State::kReceiving, "Valid",
                     "Connection to webserver can be established, recent data received");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineWeather::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineWeather::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineWeather::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        fWeather.SetVerbose(!conf.Get<bool>("quiet"));
        fWeather.SetInterval(conf.Get<uint16_t>("interval"));
        fWeather.SetDebugTx(conf.Get<bool>("debug-tx"));
        fWeather.SetSite(conf.Get<string>("url"));
        fWeather.SetEndpoint(conf.Get<string>("addr"));
        fWeather.StartConnect();

        return -1;
    }
};



// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineWeather<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("MAGIC weather control options");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("www.magic.iac.es:80"),  "Network address of Cosy")
        ("url,u",  var<string>("/site/weather/weather_data.txt"),  "File name and path to load")
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
        "The magicweather is an interface to the MAGIC weather data.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: magicweather [-c type] [OPTIONS]\n"
        "  or:  magicweather [OPTIONS]\n";
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
                return RunShell<LocalStream, StateMachine, ConnectionWeather>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionWeather>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionWeather>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimWeather>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimWeather>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
