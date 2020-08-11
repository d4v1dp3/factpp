#include <boost/array.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
#include "Console.h"

#include "tools.h"

#include "HeadersGTC.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::property_tree;
namespace dummy = ba::placeholders;

using namespace std;
using namespace GTC;

// ------------------------------------------------------------------------

class ConnectionGTC : public Connection
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

protected:

    Time fLastReport;
    Time fLastReception;

    boost::asio::streambuf fBuffer;
    string fData;

    virtual void UpdateGTC(const Time &t, const float &)
    {
    }

    void ProcessAnswer(string s)
    {
        try
        {
            std::stringstream ss;
            ss << s;

            pt::ptree tree;
            pt::read_json(ss, tree);

            // {"pm25":{"Value":"13.9000","Date":"2019-10-03 10:31:40"}}

            const auto &pm25 = tree.get_child("pm25");

            const float value = pm25.get_child("Value").get_value<float>();

            Time date;
            date.SetFromStr(pm25.get_child("Date").get_value<string>());

            if (date!=fLastReport)
            {
                Info(date.GetAsStr()+": "+Tools::Form("%.2f", value)+" ug/m^3");
                UpdateGTC(date, value);
                fLastReport = date;
            }
        }
        catch (std::exception const& e)
        {
            Error(string("Parsing JSON failed: ")+e.what());
        }
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            // The last report (payload) is simply termined by
            // the connection being closed by the server.
            // I do not understand why bytes_received is 0 then.
            //
            // Extract whatever is left in the buffer
            istream is(&fBuffer);
            string buffer;
            getline(is, buffer);
            fData += buffer;

            if (fIsVerbose)
                Out() << "EOF|" << buffer << endl;

            if (err==ba::error::eof)
            {
                // Does the message contain a header?
                const size_t p1 = fData.find("\r\n\r\n");
                if (p1!=string::npos)
                    ProcessAnswer(fData.substr(p1));
                else
                    Warn("Received message lacks a header!");
                fData = "";

                PostClose(false);

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
            return;
        }

        fLastReception = Time();

        istream is(&fBuffer);

        string buffer;
        if (!getline(is, buffer, '\n'))
        {
            Fatal("Received message does not contain \\n... closing connection.");
            PostClose(false);
            return;
        }

        if (fIsVerbose)
            Out() << bytes_received << "|" << buffer << endl;

        fData += buffer;
        fData += '\n';

        StartReadLine();
    }

    void StartReadLine()
    {
        // The last report (payload) is simply termined by the connection being closed by the server.
        async_read_until(*this, fBuffer, '\n',
                         boost::bind(&ConnectionGTC::HandleRead, this,
                                     dummy::error, dummy::bytes_transferred));
    }

    ba::deadline_timer fKeepAlive;

    bool fRequestPayload;

    void PostRequest()
    {
        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "User-Agent: FACT gtcdust\r\n"
            "Accept: */*\r\n"
            "Accept-Encoding: identity\r\n"
            "Host: "+URL()+"\r\n"
            "Connection: close\r\n"
            "Pragma: no-cache\r\n"
            "Cache-Control: no-cache\r\n"
            "Expires: 0\r\n"
            "Cache-Control: max-age=0\r\n"
            "\r\n";

        PostMessage(cmd);
    }

    void Request()
    {
        PostRequest();

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval/2));
        fKeepAlive.async_wait(boost::bind(&ConnectionGTC::HandleRequest,
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
        StartReadLine();
    }

public:
    ConnectionGTC(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
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

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimGTC : public ConnectionGTC
{
private:

    DimDescribedService fDimGTC;

    virtual void UpdateGTC(const Time &t, const float &data)
    {
        fDimGTC.setData(&data, sizeof(float));
        fDimGTC.Update(t);
    }

public:
    ConnectionDimGTC(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionGTC(ioservice, imp),
        fDimGTC("GTC_DUST/DATA", "F:1",
                "|dust[ug/m3]:Seconds since device start")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineGTC : public StateMachineAsio<T>
{
private:
    S fGTC;

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

        fGTC.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fBiasTemp.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fBiasTemp.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fBiasTemp.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fBiasTemp.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fGTC.GetState();
    }

public:
    StateMachineGTC(ostream &out=cout) :
        StateMachineAsio<T>(out, "GTC_DUST"), fGTC(*this, *this)
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
            (bind(&StateMachineGTC::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineBiasTemp::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineBiasTemp::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        fGTC.SetVerbose(!conf.Get<bool>("quiet"));
        fGTC.SetInterval(conf.Get<uint16_t>("interval"));
        fGTC.SetDebugTx(conf.Get<bool>("debug-tx"));
        fGTC.SetSite(conf.Get<string>("url"));
        fGTC.SetEndpoint(conf.Get<string>("addr"));
        fGTC.StartConnect();

        return -1;
    }
};



// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineGTC<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Options");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("atmosportal.gtc.iac.es:80"),  "Network address of the hardware")
        ("url,u",  var<string>("/queries/pm25"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(120), "Interval between two data updates in second (request time is half, timeout is double)")
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
        "The gtdust is an interface to the GTC dust measurement.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: biastemp [-c type] [OPTIONS]\n"
        "  or:  biastemp [OPTIONS]\n";
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
                return RunShell<LocalStream, StateMachine, ConnectionGTC>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimGTC>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionGTC>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionGTC>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimGTC>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimGTC>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
