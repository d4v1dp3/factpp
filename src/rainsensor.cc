#if BOOST_VERSION < 104600
#include <assert.h>
#endif

#include <boost/array.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

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

#include "HeadersRainSensor.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::property_tree;
namespace dummy = ba::placeholders;

using namespace std;
using namespace RainSensor;

// ------------------------------------------------------------------------

class ConnectionRain : public Connection
{
    uint16_t fInterval;

    bool fDebugRx;

    string fSite;
    string fRdfData;
    string fAuthentication;

    boost::array<char, 4096> fArray;

    Time fLastReport;

    int fStatus;

protected:
    struct data_t
    {
        float rain;
        int64_t stat;
    } __attribute__((__packed__));

    virtual void UpdateRain(const Time &, const data_t &)
    {
    }

    void ProcessAnswer()
    {
        if (fDebugRx)
        {
            Out() << "------------------------------------------------------" << endl;
            Out() << fRdfData << endl;
            Out() << "------------------------------------------------------" << endl;
        }

        const size_t p1 = fRdfData.find("\r\n\r\n");
        if (p1==string::npos)
        {
            Warn("HTTP header not found.");
            fStatus = RainSensor::State::kConnected;
            PostClose(false);
            return;
        }

        fRdfData.erase(0, p1+4);

        data_t data;
        Time time;
        try
        {
            //{"rain": 0.0, "time": "2018-04-07T15:28:01.044273", "statistics": 30}

            std::stringstream ss;
            ss << fRdfData;

            pt::ptree tree;
            pt::read_json(ss, tree);

            data.rain = tree.get_child("rain").get_value<float>();
            data.stat = tree.get_child("statistics").get_value<int64_t>();
            time = Time(tree.get_child("time").get_value<string>());
        }
        catch (std::exception const& e)
        {
            Warn("Parsing of JSON failed: "+string(e.what()));
            fStatus = RainSensor::State::kConnected;
            PostClose(false);
            return;
        }

        fRdfData = "";

        UpdateRain(time, data);

        ostringstream msg;
        msg << Time::iso << time << " - Rain=" << data.rain << " [N=" << data.stat << "]";
        Message(msg);

        fStatus = RainSensor::State::kValid;

        fLastReport = Time();
        PostClose(false);
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
            {
                if (!fRdfData.empty())
                    ProcessAnswer();
                return;
            }

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

            fRdfData = "";
            return;
        }

        fRdfData += string(fArray.data(), bytes_received);

        // Does the message contain a header?
        const size_t p1 = fRdfData.find("\r\n\r\n");
        if (p1!=string::npos)
        {
            // Does the answer also contain the body?
            const size_t p2 = fRdfData.find("\r\n\r\n", p1+4);
            if (p2!=string::npos)
                ProcessAnswer();
        }

        // Go on reading until the web-server closes the connection
        StartReadReport();
    }

    boost::asio::streambuf fBuffer;

    void StartReadReport()
    {
        async_read_some(ba::buffer(fArray),
                        boost::bind(&ConnectionRain::HandleRead, this,
                                    dummy::error, dummy::bytes_transferred));
    }


    ba::deadline_timer fKeepAlive;

    void PostRequest()
    {
        const string auth = fAuthentication.empty() ? "" :
            "Authorization: Basic "+fAuthentication+"\r\n";

        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "Accept: */*\r\n"
            "Content-Type: application/octet-stream\r\n"
            +auth+
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
        fKeepAlive.async_wait(boost::bind(&ConnectionRain::HandleRequest,
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
    ConnectionRain(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fLastReport(Time::none), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetDebugRx(bool b)
    {
        fDebugRx = b;
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
        if (!fLastReport.IsValid() || Time()>fLastReport+boost::posix_time::seconds(fInterval*3))
            return RainSensor::State::kDisconnected;

        return fStatus;
    }

    void SetAuthentication(const string &user, const string &password)
    {
        if (user.empty() && password.empty())
        {
            fAuthentication = "";
            return;
        }

        const string auth = user+":"+password;

        // convert binary values to base64 characters
        // retrieve 6 bit integers from a sequence of 8 bit bytes
        // compose all the above operations in to a new iterator

        using namespace boost::archive::iterators;
        using it = base64_from_binary<transform_width<string::const_iterator, 6, 8>>;

        fAuthentication = string(it(begin(auth)), it(end(auth)));
    }
};

const uint16_t ConnectionRain::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimRain : public ConnectionRain
{
private:

    DimDescribedService fDimRain;

    virtual void UpdateRain(const Time &tm, const data_t &data)
    {
        fDimRain.setData(data);
        fDimRain.Update(tm);
    }

public:
    ConnectionDimRain(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionRain(ioservice, imp),
        fDimRain("RAIN_SENSOR/DATA", "F:1;X:1",
                     "|rain[float]:Rain"
                     "|count:Number of sensor requests")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineRain : public StateMachineAsio<T>
{
private:
    S fRain;

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

        fRain.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDebugRx(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDebugRx", 1))
            return T::kSM_FatalError;

        fRain.SetDebugRx(evt.GetBool());

        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fRain.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fRain.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fRain.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fRain.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fRain.GetState();
    }

public:
    StateMachineRain(ostream &out=cout) :
        StateMachineAsio<T>(out, "RAIN_SENSOR"), fRain(*this, *this)
    {
        // State names
        T::AddStateName(RainSensor::State::kDisconnected, "NoConnection",
                        "No connection to web-server could be established recently");

        T::AddStateName(RainSensor::State::kConnected, "Connected",
                        "Connection established, but no valid data received");

        T::AddStateName(RainSensor::State::kValid, "Valid",
                        "Connection established, received data valid");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineRain::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("SET_DEBUG_RX", "B")
            (bind(&StateMachineRain::SetDebugRx, this, placeholders::_1))
            ("enable debugging for received data"
             "|debug-rx[bool]:disable or enable verbosity for received data (yes/no)");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineRain::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineRain::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        const string user = conf.Has("user") ? conf.Get<string>("user") : "";
        const string pass = conf.Has("password") ? conf.Get<string>("password") : "";

        fRain.SetInterval(conf.Get<uint16_t>("interval"));
        fRain.SetDebugTx(conf.Get<bool>("debug-tx"));
        fRain.SetDebugRx(conf.Get<bool>("debug-rx"));
        fRain.SetSite(conf.Get<string>("url"));
        fRain.SetEndpoint(conf.Get<string>("addr"));
        fRain.SetAuthentication(user, pass);
        fRain.StartConnect();

        return -1;
    }
};



// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineRain<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("MAGIC rain sensor connection");
    control.add_options()
        ("no-dim,d",  po_switch(), "Disable dim services")
        ("addr,a",  var<string>("www.magic.iac.es:80"),  "Network address of Cosy")
        ("url,u",  var<string>("/site/weather/rain_current.json"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(60), "Interval between two updates on the server in seconds")
        ("debug-tx", po_bool(), "Enable debugging of ethernet transmission.")
        ("debug-rx", po_bool(), "Enable debugging of ethernet receptions.")
        ("user", var<string>(), "User name for authentication.")
        ("password", var<string>(), "Password for authentication.")
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
        "The rainsensor is an interface to the MAGIC rainsensor data.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: rainsensor [-c type] [OPTIONS]\n"
        "  or:  rainsensor [OPTIONS]\n";
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
                return RunShell<LocalStream, StateMachine, ConnectionRain>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimRain>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionRain>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionRain>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimRain>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimRain>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
