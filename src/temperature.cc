#if BOOST_VERSION < 104600
#include <assert.h>
#endif

#include <boost/array.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <string>

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

#include "HeadersTemperature.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::property_tree;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionPowerSwitch : public Connection
{
protected:
    bool fIsValid;

private:
    uint16_t fInterval;

    bool fIsVerbose;
    bool fDebugRx;

    string fSite;
    string fRdfData;

    boost::array<char, 4096> fArray;

    string fNextCommand;

    Time fLastReport;

    int fStatus;

    virtual void Update(const vector<float> &)
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
            PostClose(false);
            return;
        }

        fRdfData.erase(0, p1+4);

        vector<float> temp(3);
        try
        {
            std::stringstream ss;
            ss << fRdfData;

            pt::ptree tree;
            pt::read_json(ss, tree);

            const pt::ptree sub2 = tree.get_child("sensor_values.").begin()->second;
            const pt::ptree sub3 = sub2.get_child("values").begin()->second.begin()->second;

            temp[0] = sub3.get_child("v").get_value<float>();

            auto sub = sub3.get_child("st.").begin();

            temp[1] = sub++->second.get_value<float>();
            temp[2] = sub->second.get_value<float>();
        }
        catch (std::exception const& e)
        {
            Warn("Parsing of JSON failed: "+string(e.what()));

            fStatus = Temperature::State::kConnected;

            PostClose(false);
            return;
        }

        fRdfData = "";

        Update(temp);

        ostringstream msg;
        msg << "T="    << temp[0] << "\u00b0C"
            << " Tmin=" << temp[1] << "\u00b0C"
            << " Tmax=" << temp[2] << "\u00b0C";
        Message(msg);

        fStatus = Temperature::State::kValid;

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
                        boost::bind(&ConnectionPowerSwitch::HandleRead, this,
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
    ConnectionPowerSwitch(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsValid(false), fIsVerbose(true), fDebugRx(false), fLastReport(Time::none),
        fStatus(Temperature::State::kDisconnected), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetDebugRx(bool b)
    {
        fDebugRx = b;
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

    void Post(const string &post)
    {
        fNextCommand = post;
    }

    void Request()
    {
        string cmd = "GET " + fSite;

        if (!fNextCommand.empty())
            cmd += "?" + fNextCommand;

        cmd += " HTTP/1.1\r\n";
        cmd += "\r\n";

        PostMessage(cmd);

        fNextCommand = "";

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval));
        fKeepAlive.async_wait(boost::bind(&ConnectionPowerSwitch::HandleRequest,
                                          this, dummy::error));
    }

    int GetInterval() const
    {
        return fInterval;
    }

    int GetState() const
    {
        // Timeout
        if (!fLastReport.IsValid() || Time()>fLastReport+boost::posix_time::seconds(fInterval*3))
            return Temperature::State::kDisconnected;

        return fStatus;
    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimPowerSwitch : public ConnectionPowerSwitch
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimPowerSwitch(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionPowerSwitch(ioservice, imp),
        fDim("TEMPERATURE/DATA", "F:1;F:1;F:1",
             "Temperature readout from power switch"
             "|T[degC]:Current temperature"
             "|Tmin[degC]:24h minimum"
             "|Tmax[degC]:24h maximum")
    {
    }

    void Update(const vector<float> &temp)
    {
        fDim.Update(temp);
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachinePowerControl : public StateMachineAsio<T>
{
private:
    S fPower;
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

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fPower.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDebugRx(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDebugRx", 1))
            return T::kSM_FatalError;

        fPower.SetDebugRx(evt.GetBool());

        return T::GetCurrentState();
    }

    int Execute()
    {
        return fPower.GetState();
    }


public:
    StateMachinePowerControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "TEMPERATURE"), fPower(*this, *this)
    {
        // State names
        T::AddStateName(Temperature::State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(Temperature::State::kConnected, "Connected",
                     "Connection established, but no valid data received");

        T::AddStateName(Temperature::State::kValid, "Valid",
                     "Connection established, received data valid");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachinePowerControl::SetVerbosity, this, placeholders::_1))
            ("Set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for interpreted data (yes/no)");

        T::AddEvent("SET_DEBUG_RX", "B:1")
            (bind(&StateMachinePowerControl::SetDebugRx, this, placeholders::_1))
            ("Set debux-rx state"
             "|debug[bool]:dump received text and parsed text to console (yes/no)");

    }

    int EvalOptions(Configuration &conf)
    {
        fPower.SetVerbose(!conf.Get<bool>("quiet"));
        fPower.SetInterval(conf.Get<uint16_t>("interval"));
        fPower.SetDebugTx(conf.Get<bool>("debug-tx"));
        fPower.SetDebugRx(conf.Get<bool>("debug-rx"));
        fPower.SetSite(conf.Get<string>("url"));
        fPower.SetEndpoint(conf.Get<string>("addr"));
        fPower.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachinePowerControl<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Lid control");
    control.add_options()
        ("no-dim,d",   po_switch(),    "Disable dim services")
        ("addr,a",     var<string>("10.0.100.234:80"),  "Network address of the lid controling Arduino including port")
        ("url,u",      var<string>("/statusjsn.js?components=18179&_=1365876572736"),  "File name and path to load")
        ("quiet,q",    po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(60), "Interval between two updates on the server in seconds")
        ("debug-tx",   po_bool(), "Enable debugging of ethernet transmission.")
        ("debug-rx",   po_bool(), "Enable debugging for received data.")
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
        "The temperature is an interface to readout the temperature from the power switch.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: temperature [-c type] [OPTIONS]\n"
        "  or:  temperature [OPTIONS]\n";
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
            return RunShell<LocalStream, StateMachine, ConnectionPowerSwitch>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimPowerSwitch>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionPowerSwitch>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionPowerSwitch>(conf);
    }
    else
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachineDim, ConnectionDimPowerSwitch>(conf);
        else
            return RunShell<LocalConsole, StateMachineDim, ConnectionDimPowerSwitch>(conf);
    }

    return 0;
}
