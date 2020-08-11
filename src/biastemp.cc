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

#include "HeadersBiasTemp.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::property_tree;
namespace dummy = ba::placeholders;

using namespace std;
using namespace BiasTemp;

// ------------------------------------------------------------------------

class ConnectionBiasTemp : public Connection
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

protected:

    Time fLastReport;
    Time fLastReception;

    boost::asio::streambuf fBuffer;
    string fData;

    virtual void UpdateBiasTemp(const Data &)
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

            Data data;
            data.time = tree.get_child("timestamp").get_value<uint64_t>();

            const auto &branch = tree.get_child("temperatures_deg_C");

            // FIXME: Move to config file?
            static const string id[10] =
            {
                "10 e5 54 2d 03 08 00 cc",  // between master and BIAS_0
                "10 ca 24 2e 03 08 00 bb",  // between BIAS_0 and BIAS_1
                "10 9f 51 2d 03 08 00 49",  // between BIAS_1 and BIAS_2
                "10 b7 6d 2d 03 08 00 fb",  // between BIAS_3 and BIAS_4
                "10 a5 f0 2d 03 08 00 95",  // between BIAS_4 and BIAS_5
                "10 16 75 2d 03 08 00 d2",  // between BIAS_5 and BIAS_6
                "10 50 77 2d 03 08 00 96",  // between BIAS_7 and "SPARE"
                "10 9f 02 2e 03 08 00 1a",  // between "SPARE" and BIAS_9
                "10 1d ce 2d 03 08 00 15",  // right of BIAS_9
                "10 cc f3 2d 03 08 00 8e",  // far to the right, where no heat should be generated
            };

            data.avg = 0;
            data.rms = 0;

            for (int i=0; i<10; i++)
            {
                data.temp[i] = branch.get_child(id[i]).get_value<float>();
                data.avg += data.temp[i];
                data.rms += data.temp[i]*data.temp[i];
            }

            data.avg /= 10;
            data.rms /= 10;

            data.rms = sqrt(data.rms - data.avg*data.avg);

            ostringstream out;
            out << Tools::Form("T=%09d:", data.time);
            for (int i=0; i<10; i++)
                out << Tools::Form("%5.1f", data.temp[i]);

            out << Tools::Form(" |%5.1f +-%4.1f", data.avg, data.rms);

            Info(out);

            UpdateBiasTemp(data);

            fLastReport = Time();
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
            Out() << buffer << endl;

        fData += buffer;
        fData += '\n';

        StartReadLine();
    }

    void StartReadLine()
    {
        async_read_until(*this, fBuffer, '\n',
                             boost::bind(&ConnectionBiasTemp::HandleRead, this,
                                         dummy::error, dummy::bytes_transferred));
    }

    ba::deadline_timer fKeepAlive;

    void PostRequest()
    {
        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "\r\n";

        PostMessage(cmd);
    }

    void Request()
    {
        PostRequest();

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval/2));
        fKeepAlive.async_wait(boost::bind(&ConnectionBiasTemp::HandleRequest,
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
    ConnectionBiasTemp(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
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

class ConnectionDimBiasTemp : public ConnectionBiasTemp
{
private:

    DimDescribedService fDimBiasTemp;

    virtual void UpdateBiasTemp(const Data &data)
    {
        fDimBiasTemp.setData(&data, sizeof(Data));
        fDimBiasTemp.Update();
    }

public:
    ConnectionDimBiasTemp(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionBiasTemp(ioservice, imp),
        fDimBiasTemp("BIAS_TEMP/DATA", "X:1;F:10;D:1;D:1",
                     "|time[s]:Seconds since device start"
                     "|T[degC]:Temperature"
                     "|Tavg[degC]:Average temperature"
                     "|Trms[degC]:RMS of temperatures")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineBiasTemp : public StateMachineAsio<T>
{
private:
    S fBiasTemp;

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

        fBiasTemp.SetVerbose(evt.GetBool());

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
        return fBiasTemp.GetState();
    }

public:
    StateMachineBiasTemp(ostream &out=cout) :
        StateMachineAsio<T>(out, "BIAS_TEMP"), fBiasTemp(*this, *this)
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
            (bind(&StateMachineBiasTemp::SetVerbosity, this, placeholders::_1))
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
        fBiasTemp.SetVerbose(!conf.Get<bool>("quiet"));
        fBiasTemp.SetInterval(conf.Get<uint16_t>("interval"));
        fBiasTemp.SetDebugTx(conf.Get<bool>("debug-tx"));
        fBiasTemp.SetSite(conf.Get<string>("url"));
        fBiasTemp.SetEndpoint(conf.Get<string>("addr"));
        fBiasTemp.StartConnect();

        return -1;
    }
};



// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineBiasTemp<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Bias Crate temperature readout");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("10.0.100.101:80"),  "Network address of the hardware")
        ("url,u",  var<string>("/index.html"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(15), "Interval between two updates on the server in seconds")
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
        "The biastemp is an interface to the temperature sensors in the bias crate.\n"
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
                return RunShell<LocalStream, StateMachine, ConnectionBiasTemp>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimBiasTemp>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionBiasTemp>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionBiasTemp>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimBiasTemp>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimBiasTemp>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
