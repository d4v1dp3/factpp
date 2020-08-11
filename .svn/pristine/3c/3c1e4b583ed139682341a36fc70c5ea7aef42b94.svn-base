#include <functional>

#include "Dim.h"
#include "Event.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"

#include "tools.h"

#include "HeadersAgilent.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace Agilent;

// ------------------------------------------------------------------------

class ConnectionAgilent : public Connection
{
public:
    static string fMode;

private:
    bool fIsVerbose;
    bool fDebugRx;

    uint16_t fInterval;

    boost::asio::deadline_timer fTimeout;
    boost::asio::deadline_timer fTimeoutPowerCycle;
    boost::asio::streambuf fBuffer;

    Data fData;

    Time fLastReceived;
    Time fLastCommand;

protected:

    virtual void UpdateDim(const Data &)
    {
    }

    void RequestStatus()
    {
        if (IsConnected())
            PostMessage(string("*IDN?\nvolt?\nmeas:volt?\nmeas:curr?\ncurr?\n"));

        fTimeout.expires_from_now(boost::posix_time::seconds(fInterval));
        fTimeout.async_wait(boost::bind(&ConnectionAgilent::HandleStatusTimer,
                                        this, dummy::error));
    }


    void HandleStatusTimer(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Status request timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
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
        if (fTimeout.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        RequestStatus();
    }

    void HandlePowerCycle(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Power cycle timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
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
        if (fTimeout.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        SetPower(true);
    }

private:
    void StartRead(int line=0)
    {
        ba::async_read_until(*this, fBuffer, "\n",
                             boost::bind(&ConnectionAgilent::HandleReceivedData, this,
                                         dummy::error, dummy::bytes_transferred, line+1));
    }

    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int line)
    {

        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host (FTM).");

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


        if (fDebugRx)
        {
            Out() << kBold << "Received (" << bytes_received << ", " << fBuffer.size() << " bytes):" << endl;
            Out() << "-----\n" << string(ba::buffer_cast<const char*>(fBuffer.data()), bytes_received) << "-----\n";
        }

        istream is(&fBuffer);

        string str;
        getline(is, str, '\n');

        try
        {
            switch (line)
            {
            case 1:  Out() << "ID: " << str << endl; break;
            case 2:  fData.fVoltageSet      = stof(str); break;
            case 3:  fData.fVoltageMeasured = stof(str); break;
            case 4:  fData.fCurrentMeasured = stof(str); break;
            case 5:  fData.fCurrentLimit    = stof(str); break;
            default:
                return;
            }
        }
        catch (const exception &e)
        {
            Error("String conversion failed for '"+str+" ("+e.what()+")");

            // We need to synchronize the stream again
            PostClose(true);
            return;
        }

        if (line==5)
        {
            if (fIsVerbose)
            {
                Out() << "Voltage: " << fData.fVoltageMeasured << "V/" << fData.fVoltageSet   << "V\n";
                Out() << "Current: " << fData.fCurrentMeasured << "A/" << fData.fCurrentLimit << "A\n" << endl;
            }

            UpdateDim(fData);

            fLastReceived = Time();

            line = 0;

        }

        StartRead(line);
    }


    // This is called when a connection was established
    void ConnectionEstablished()
    {
        fBuffer.prepare(1000);

        StartRead();
        RequestStatus();
    }

public:

    ConnectionAgilent(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fDebugRx(false), fTimeout(ioservice), fTimeoutPowerCycle(ioservice)
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
    }

    void SetInterval(uint16_t i)
    {
        fInterval = i;
    }

    bool SetPower(bool on)
    {
        if (!IsConnected())
            return false;

        if (fLastCommand+boost::posix_time::seconds(59)>Time())
        {
            Error("Last power command within the last 59 seconds... ignored.");
            return false;
        }

        PostMessage("outp "+string(on?"on":"off")+"\n*IDN?\nvolt?\nmeas:volt?\nmeas:curr?\ncurr?\n");
        fLastCommand = Time();

        // Stop any pending power cycling
        fTimeoutPowerCycle.cancel();

        return true;
    }

    void PowerCycle(uint16_t seconds)
    {
        if (!SetPower(false))
            return;

        fTimeoutPowerCycle.expires_from_now(boost::posix_time::seconds(seconds));
        fTimeoutPowerCycle.async_wait(boost::bind(&ConnectionAgilent::HandlePowerCycle,
                                                  this, dummy::error));
    }

    int GetState()
    {
        if (!IsConnected())
            return State::kDisconnected;

        if (fLastReceived+boost::posix_time::seconds(fInterval*2)<Time())
            return State::kDisconnected;

        if (fData.fCurrentMeasured<0)
            return State::kConnected;

        if (fData.fVoltageMeasured<0.1)
            return State::kVoltageOff;

        if (fData.fVoltageMeasured<fData.fVoltageSet-0.1)
            return State::kVoltageLow;

        if (fData.fVoltageMeasured>fData.fVoltageSet+0.1)
            return State::kVoltageHigh;

        return State::kVoltageOn;
    }
};

string ConnectionAgilent::fMode;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimAgilent : public ConnectionAgilent
{
private:

    DimDescribedService fDim;

    void UpdateDim(const Data &data)
    {
        fDim.Update(data);
    }

public:
    ConnectionDimAgilent(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionAgilent(ioservice, imp),
        fDim("AGILENT_CONTROL_"+fMode+"/DATA", "F:1;F:1;F:1;F:1",
             "|U_nom[V]: Nominal output voltage"
             "|U_mes[V]: Measured output voltage"
             "|I_max[A]: Current limit"
             "|I_mes[A]: Measured current")
    {
        // nothing happens here.
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineAgilent : public StateMachineAsio<T>
{
private:
    S fAgilent;

    int Disconnect()
    {
        // Close all connections
        fAgilent.PostClose(false);

        /*
         // Now wait until all connection have been closed and
         // all pending handlers have been processed
         poll();
         */

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fAgilent.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fAgilent.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fAgilent.PostClose(true);

        return T::GetCurrentState();
    }

    int Execute()
    {
        return fAgilent.GetState();
    }

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

        fAgilent.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDebugRx(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDebugRx", 1))
            return T::kSM_FatalError;

        fAgilent.SetDebugRx(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetPower(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetPower", 1))
            return T::kSM_FatalError;

        fAgilent.SetPower(evt.GetBool());

        return T::GetCurrentState();
    }

    int PowerCycle(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "PowerCyle", 2))
            return T::kSM_FatalError;

        if (evt.GetShort()<60)
        {
            T::Warn("Power cycle delays of less than 60s not allowed.");
            return T::GetCurrentState();
        }

        fAgilent.PowerCycle(evt.GetShort());

        return T::GetCurrentState();
    }


public:
    StateMachineAgilent(ostream &out=cout) :
        StateMachineAsio<T>(out, "AGILENT_CONTROL_"+S::fMode), fAgilent(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                        "Agilent not connected via ethernet.");
        T::AddStateName(State::kConnected, "Connected",
                        "Ethernet connection to Agilent established, but not data received yet.");

        T::AddStateName(State::kVoltageOff, "VoltageOff",
                        "The measured output voltage is lower than 0.1V");
        T::AddStateName(State::kVoltageLow, "VoltageLow",
                        "The measured output voltage is higher than 0.1V, but lower than the command voltage");
        T::AddStateName(State::kVoltageOn, "VoltageOn",
                        "The measured output voltage is higher than 0.1V and comparable to the command voltage");
        T::AddStateName(State::kVoltageHigh, "VoltageHigh",
                        "The measured output voltage is higher than the command voltage!");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineAgilent::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no)");

        T::AddEvent("SET_DEBUG_RX", "B:1")
            (bind(&StateMachineAgilent::SetVerbosity, this, placeholders::_1))
            ("set debug state"
             "|debug[bool]:disable or enable verbosity for received raw data (yes/no)");

        T::AddEvent("SET_POWER", "B:1")
            (bind(&StateMachineAgilent::SetPower, this, placeholders::_1))
            ("Enable or disable power output"
             "|output[bool]:set power output to 'on' or 'off'");

        T::AddEvent("POWER_CYCLE", "S:1")
            (bind(&StateMachineAgilent::PowerCycle, this, placeholders::_1))
            ("Power cycle the power output"
             "|delay[short]:Defines the delay between switching off and on.");


        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected)
            (bind(&StateMachineAgilent::Disconnect, this))
            ("disconnect from ethernet");

        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected)
            (bind(&StateMachineAgilent::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to Agilent, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

        fAgilent.StartConnect();
    }

    void SetEndpoint(const string &url)
    {
        fAgilent.SetEndpoint(url);
    }

    int EvalOptions(Configuration &conf)
    {
        fAgilent.SetVerbose(!conf.Get<bool>("quiet"));
        fAgilent.SetDebugRx(conf.Get<bool>("debug-rx"));
        fAgilent.SetInterval(conf.Get<uint16_t>("interval"));

        SetEndpoint(conf.Get<string>("addr.", S::fMode));

        const std::vector<std::string> opts = conf.GetWildcardOptions("addr.*");
        for (auto it=opts.begin(); it!=opts.end(); it++)
            conf.Get<string>(*it);

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineAgilent<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Control options");
    control.add_options()
        ("no-dim",    po_bool(),         "Disable dim services")
        ("mode,m",    var<string>()->required(), "Mode (e.g. 24V, 50V, 80V)")
        ("addr.*",    var<string>(),     "Network address of Agilent specified by mode")
        ("debug-rx",  po_bool(false),    "Enable raw debug output wehen receiving data")
        ("interval",  var<uint16_t>(15), "Interval in seconds in which the Agilent status is requested")
        ("quiet,q",   po_bool(true),     "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ;

    po::positional_options_description p;
    p.add("mode", 1); // The first positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
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
        "The agilentctrl controls the FACT Agilent power supplies.\n\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: agilentctrl [-c type] [OPTIONS] mode\n"
        "  or:  agilentctrl [OPTIONS] mode\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineAgilent<StateMachine, ConnectionAgilent>>();
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    ConnectionAgilent::fMode = conf.Get<string>("mode");

    //try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
            if (conf.Get<bool>("no-dim"))
                return RunShell<LocalStream, StateMachine, ConnectionAgilent>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimAgilent>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionAgilent>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionAgilent>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimAgilent>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimAgilent>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
