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

#include "HeadersPFmini.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionPFmini : public Connection
{
protected:
    virtual void Update(const PFmini::Data &)
    {
    }

private:
    bool fIsVerbose;
    uint16_t fInterval;

    bool fReceived;

    int fState;

    vector<int16_t> fBuffer;

    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int /*type*/)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || (err && err!=ba::error::eof))
        {
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
            PostClose(false);
            return;
        }

        const uint16_t chk0 = Tools::Fletcher16(fBuffer.data(), 2);
        const uint16_t chk1 = uint16_t(fBuffer[2]);

        if (chk0!=chk1)
        {
            ostringstream out;
            out << "Checksum error (";
            out << hex << setfill('0');
            out << setw(4) << fBuffer[0] << ":";
            out << setw(4) << fBuffer[1] << "|";
            out << setw(4) << fBuffer[2] << "!=";
            out << setw(4) << chk1 << ")";

            Error(out);

            PostClose(false);

            return;
        }

        PFmini::Data data;
        data.hum  = 110*fBuffer[0]/1024.;
        data.temp = 110*fBuffer[1]/1024.-20;

        Update(data);

        ostringstream msg;
        msg << fixed << setprecision(1) << "H=" << data.hum << "% T=" << data.temp << "\u00b0C";
        Message(msg);

        fState = PFmini::State::kReceiving;
        fReceived = true;
    }

    boost::asio::deadline_timer fKeepAlive;

    void HandleRequest(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fKeepAlive.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        // Re-open connection
        PostClose(true);
    }

    void HandleReadTimeout(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Read timeout of " << URL() << " timed out: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        if (!fReceived)
            PostClose(false);
    }

    void Request()
    {
        fReceived = false;

        string cmd =  "GET / HTTP/1.1\r\n\r\n";
        PostMessage(cmd);

        fBuffer.resize(6);
        AsyncRead(ba::buffer(fBuffer));
        AsyncWait(fInTimeout, 3000, &Connection::HandleReadTimeout);

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval));
        fKeepAlive.async_wait(boost::bind(&ConnectionPFmini::HandleRequest,
                                          this, dummy::error));
    }

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        // Keep state kReceiving
        if (fState<PFmini::State::kConnected)
            fState = PFmini::State::kConnected;

        Request();
    }

public:
    static const uint16_t kMaxAddr;

public:
    ConnectionPFmini(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fKeepAlive(ioservice)
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

    int GetState() const
    {
        if (!is_open())
            return PFmini::State::kDisconnected;

        return fState;
    }
};

const uint16_t ConnectionPFmini::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionPFmini
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionPFmini(ioservice, imp),
        fDim("PFMINI_CONTROL/DATA", "F:1;F:1",
             "Humidity and temperature as read out from the PFmini arduino"
             "|Humidity[%]:Measures humidity"
             "|Temperature[deg]:Measured temperature")
    {
    }

    void Update(const PFmini::Data &data)
    {
        fDim.Update(data);
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachinePFminiControl : public StateMachineAsio<T>
{
private:
    S fPFmini;
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
        fPFmini.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fPFmini.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fPFmini.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fPFmini.PostClose(true);

        return T::GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fPFmini.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int Execute()
    {
        return fPFmini.GetState();
    }


public:
    StateMachinePFminiControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "PFMINI_CONTROL"), fPFmini(*this, *this)
    {
        // State names
        T::AddStateName(PFmini::State::kDisconnected, "Disconnected",
                        "No connection to web-server could be established recently");

        T::AddStateName(PFmini::State::kConnected, "Connected",
                        "Connection established, but status still not known");

        T::AddStateName(PFmini::State::kReceiving, "Receiving",
                        "Connection established, receiving reports");

        // Commands
        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachinePFminiControl::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        // Conenction commands
        T::AddEvent("DISCONNECT")
            (bind(&StateMachinePFminiControl::Disconnect, this))
            ("disconnect from ethernet");

         T::AddEvent("RECONNECT", "O")
            (bind(&StateMachinePFminiControl::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to PFmini, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

    }

    int EvalOptions(Configuration &conf)
    {
        fPFmini.SetVerbose(!conf.Get<bool>("quiet"));
        fPFmini.SetDebugTx(conf.Get<bool>("debug-tx"));
        fPFmini.SetEndpoint(conf.Get<string>("addr"));
        fPFmini.SetInterval(conf.Get<uint16_t>("interval"));
        fPFmini.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachinePFminiControl<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("PFmini control");
    control.add_options()
        ("no-dim,d", po_switch(), "Disable dim services")
        ("addr,a",   var<string>("10.0.130.140:80"), "Network address of the lid controling Arduino including port")
        ("quiet,q",  po_bool(true), "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("debug-tx", po_bool(), "Enable debugging of ethernet transmission.")
        ("interval", var<uint16_t>(15), "Interval in seconds at which a report is requested.")
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
        "The pfminictrl is an interface to the PFmini arduino.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: pfminictrl [-c type] [OPTIONS]\n"
        "  or:  pfminictrl [OPTIONS]\n";
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
            return RunShell<LocalStream, StateMachine, ConnectionPFmini>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionPFmini>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionPFmini>(conf);
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
