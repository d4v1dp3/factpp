#include <boost/algorithm/string.hpp>

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

#include "HeadersSQM.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionSQM : public Connection
{
protected:
    virtual void Update(const SQM::Data &)
    {
    }

private:
    bool     fIsVerbose;
    bool     fFirstMessage;
    bool     fValid;
    uint16_t fTimeout;

    boost::asio::streambuf fBuffer;

    boost::asio::deadline_timer fTrigger;

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
            PostClose(false);//err!=ba::error::basic_errors::operation_aborted);
            return;
        }

        istream is(&fBuffer);

        string buffer;
        if (!getline(is, buffer, '\n'))
        {
            Fatal("Received message does not contain \\n... closing connection.");
            PostClose(false);
            return;
        }

        buffer = buffer.substr(0, buffer.size()-1);

        if (fIsVerbose)
        {
            Out() << Time().GetAsStr("%H:%M:%S.%f") << "[" << buffer.size() << "]: " << buffer << "|" << endl;
            // Out() << Time().GetAsStr("%H:%M:%S.%f") << "[ " << vec.size() << "]: ";
            // for (auto it=vec.begin(); it!=vec.end(); it++)
            //     Out() << *it << "|";
            // Out() << endl;
        }

        vector<string> vec;
        boost::split(vec, buffer, boost::is_any_of(","));

        try
        {
            if (vec.size()!=6)
                throw runtime_error("Unknown number of fields in received data");

            if (vec[0]!="r")
                throw runtime_error("Not a proper answer");

            SQM::Data data;

            data.mag    = stof(vec[1]);
            data.freq   = stol(vec[2]);
            data.counts = stol(vec[3]);
            data.period = stof(vec[4]);
            data.temp   = stof(vec[5]);

            Update(data);

            fValid = true;
        }
        catch (const exception &e)
        {
            if (fFirstMessage)
                Warn("Parsing first message failed ["+string(e.what())+"]");
            else
            {
                Error("Parsing received message failed ["+string(e.what())+"]");
                Error("Received: "+buffer);
                PostClose(false);
                return;
            }
        }

        // Send next request in fTimeout milliseconds calculated from
        // the last request onwards.
        fTrigger.expires_at(fTrigger.expires_at()+boost::posix_time::milliseconds(fTimeout));
        fTrigger.async_wait(boost::bind(&ConnectionSQM::HandleRequestTrigger,
                                        this, dummy::error));

        fFirstMessage = false;
    }

    void HandleReadTimeout(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "ReadTimeout of " << URL() << " failed: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            fValid = false;
            PostClose(true);
            return;
        }

        // This is called if the deadline has been shifted
        if (error==ba::error::basic_errors::operation_aborted)
            return;

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fInTimeout.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        ostringstream str;
        str << "No valid answer received from " << URL() << " within " << ceil(fTimeout*1.5f) << "ms";
        Error(str);

        PostClose(false);

        fInTimeout.expires_from_now(boost::posix_time::milliseconds(1000));
        fInTimeout.async_wait(boost::bind(&ConnectionSQM::HandleReadTimeout,
                                          this, dummy::error));
    }

    void HandleRequestTrigger(const bs::error_code &error)
    {

        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "RequestTrigger failed of " << URL() << " failed: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            //PostClose(true);
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fTrigger.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        StartReadReport();
    }

    void StartReadReport()
    {
        PostMessage(string("rx\n"), 3);

        // Do not schedule two reads
        if (!fFirstMessage)
        {
            async_read_until(*this, fBuffer, '\n',
                             boost::bind(&ConnectionSQM::HandleRead, this,
                                         dummy::error, dummy::bytes_transferred));
        }

        fInTimeout.expires_from_now(boost::posix_time::milliseconds(int(fTimeout*1.5f)));
        fInTimeout.async_wait(boost::bind(&ConnectionSQM::HandleReadTimeout,
                                          this, dummy::error));
    }

private:
    // This is called when a connection was established
    void ConnectionEstablished()
    {
        fValid = false;
        fFirstMessage = true;

        // Empty a possible buffer first before we start reading
        // otherwise reading and writing might not be consecutive
        async_read_until(*this, fBuffer, '\n',
                         boost::bind(&ConnectionSQM::HandleRead, this,
                                     dummy::error, dummy::bytes_transferred));

        // If there was no immediate answer, send a request
        fTrigger.expires_at(Time()+boost::posix_time::milliseconds(1000));
        fTrigger.async_wait(boost::bind(&ConnectionSQM::HandleRequestTrigger,
                                        this, dummy::error));
    }

public:
    static const uint16_t kMaxAddr;

public:
    ConnectionSQM(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fTimeout(0), fTrigger(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
        Connection::SetVerbose(b);
    }

    void SetTimeout(uint16_t t)
    {
        fTimeout = t;
    }

    int GetState() const
    {
        if (!IsConnected())
            return  SQM::State::kDisconnected;

        return fValid ? SQM::State::kValid : SQM::State::kConnected;
    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionSQM
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionSQM(ioservice, imp),
        fDim("SQM_CONTROL/DATA", "F:1;I:1;I:1;F:1;F:1",
             "Data received from sky quality meter"
             "|Mag[mag/arcsec^2]:Magnitude (0 means upper brightness limit)"
             "|Freq[Hz]:Frequency of sensor"
             "|Counts:Period of sensor (counts occur at 14.7456MHz/32)"
             "|Period[s]:Period of sensor"
             "|Temp[deg C]:Sensor temperature in deg C")
    {
    }

    void Update(const SQM::Data &data)
    {
        fDim.Update(data);
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineSQMControl : public StateMachineAsio<T>
{
private:
    S fSQM;

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
        fSQM.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fSQM.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fSQM.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fSQM.PostClose(true);

        return T::GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fSQM.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int Send(const string &cmd)
    {
        const string tx = cmd+"\r\n";
        fSQM.PostMessage(tx, tx.size());
        return T::GetCurrentState();
    }

    int SendCommand(const EventImp &evt)
    {
        return Send(evt.GetString());
    }

    int Execute()
    {
        return fSQM.GetState();
    }


public:
    StateMachineSQMControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "SQM_CONTROL"), fSQM(*this, *this)
    {
        // State names
        T::AddStateName(SQM::State::kDisconnected, "Disconnected",
                        "No connection to Sky Quality Meter");

        T::AddStateName(SQM::State::kConnected, "Connected",
                        "Connection established, but no valid message received");

        T::AddStateName(SQM::State::kValid, "Valid",
                        "Valid message received");

        // Commands
        //T::AddEvent("SEND_COMMAND", "C")
        //    (bind(&StateMachineSQMControl::SendCommand, this, placeholders::_1))
        //    ("Send command to SQM");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineSQMControl::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        //T::AddEvent("ENABLE")
        //    (bind(&StateMachineSQMControl::Send, this, "veto_60"))
        //    ("Enable trigger signal once a second vetoed at every exact minute");

        //T::AddEvent("DISABLE")
        //    (bind(&StateMachineSQMControl::Send, this, "veto_on"))
        //    ("Diable trigger output");

        // Conenction commands
        T::AddEvent("DISCONNECT")
            (bind(&StateMachineSQMControl::Disconnect, this))
            ("disconnect from ethernet");

         T::AddEvent("RECONNECT", "O")
            (bind(&StateMachineSQMControl::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to SQM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

    }

    int EvalOptions(Configuration &conf)
    {
        fSQM.SetVerbose(!conf.Get<bool>("quiet"));
        fSQM.SetTimeout(conf.Get<uint16_t>("request-interval"));
        fSQM.SetDebugTx(conf.Get<bool>("debug-tx"));
        fSQM.SetEndpoint(conf.Get<string>("addr"));
        fSQM.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineSQMControl<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("SQM control");
    control.add_options()
        ("no-dim,d",         po_switch(),         "Disable dim services")
        ("addr,a",           var<string>("10.0.100.208:10001"), "Network address of the lid controling Arduino including port")
        ("quiet,q",          po_bool(true),       "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("debug-tx",         po_bool(),           "Enable debugging of ethernet transmission.")
        ("request-interval", var<uint16_t>(5000), "How often to request a report [milliseconds].")
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
        "The sqmctrl is an interface to the Sky Quality Meter.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: sqmctrl [-c type] [OPTIONS]\n"
        "  or:  sqmctrl [OPTIONS]\n";
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
            return RunShell<LocalStream, StateMachine, ConnectionSQM>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionSQM>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionSQM>(conf);
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
