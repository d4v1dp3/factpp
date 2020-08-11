#include <boost/array.hpp>

#include <string>

#include <QtXml/QDomDocument>

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

#include "HeadersPower.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionInterlock : public Connection
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

    Power::Status fStatus;

    virtual void Update(const Power::Status &)
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
        fRdfData.insert(0, "<?xml version=\"1.0\"?>\n");

        QDomDocument doc;
        if (!doc.setContent(QString(fRdfData.c_str()), false))
        {
            Warn("Parsing of html failed.");
            PostClose(false);
            return;
        }

        if (fDebugRx)
            Out() << "Parsed:\n-------\n" << doc.toString().toStdString() << endl;

        const QDomNodeList imageElems = doc.elementsByTagName("span");

        for (unsigned int i=0; i<imageElems.length(); i++)
        {
            const QDomElement e = imageElems.item(i).toElement();

            const QDomNamedNodeMap att = e.attributes();

            if (fStatus.Set(att))
                fIsValid = true;
        }

        if (fIsVerbose)
            fStatus.Print(Out());

        Update(fStatus);

        fRdfData = "";

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
                        boost::bind(&ConnectionInterlock::HandleRead, this,
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
    static const uint16_t kMaxAddr;

public:
    ConnectionInterlock(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsValid(false), fIsVerbose(true), fDebugRx(false), fLastReport(Time::none), fKeepAlive(ioservice)
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
        fKeepAlive.async_wait(boost::bind(&ConnectionInterlock::HandleRequest,
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
            return Power::State::kDisconnected;

        // No data received yet
        if (!fIsValid)
            return Power::State::kConnected;

        /*
         bool fWaterFlowOk;
         bool fWaterLevelOk;
         bool fPwrBiasOn;
         bool fPwr24VOn;
         bool fPwrPumpOn;
         bool fPwrDriveOn;
         bool fDriveMainSwitchOn;
         bool fDriveFeedbackOn;
        */

        if (!fStatus.fWaterLevelOk || (fStatus.fPwrPumpOn && !fStatus.fWaterFlowOk))
            return Power::State::kCoolingFailure;

        const int rc =
            (fStatus.fPwrBiasOn       ? Power::State::kBiasOn   : 0) |
            (fStatus.fPwrPumpOn       ? Power::State::kCameraOn : 0) |
            (fStatus.fDriveFeedbackOn ? Power::State::kDriveOn  : 0);

        return rc==0 ? Power::State::kSystemOff : rc;
    }
};

const uint16_t ConnectionInterlock::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionInterlock
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionInterlock(ioservice, imp),
        fDim("PWR_CONTROL/DATA", "C:1;C:1;C:1;C:1;C:1;C:1;C:1;C:1",
             "|water_lvl[bool]:Water level ok"
             "|water_flow[bool]:Water flowing"
             "|pwr_24V[bool]:24V power enabled"
             "|pwr_pump[bool]:Pump power enabled"
             "|pwr_bias[bool]:Bias power enabled"
             "|pwr_drive[bool]:Drive power enabled (command value)"
             "|main_drive[bool]:Drive manual main switch on"
             "|feedback_drive[bool]:Drive power on (feedback value)")
    {
    }

    void Update(const Power::Status &status)
    {
        fDim.setQuality(status.GetVal());
        fDim.Update(status);
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

    int Post(const EventImp &evt)
    {
        fPower.Post(evt.GetText());
        return T::GetCurrentState();
    }

    int SetCameraPower(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetCameraPower", 1))
            return T::kSM_FatalError;

        fLastCommand = Time();
        fPower.Post(evt.GetBool() ? "cam_on=Camera+ON" : "cam_off=Camera+OFF");
        return T::GetCurrentState();
    }

    int ToggleDrive()
    {
        fLastCommand = Time();
        fPower.Post("dt=Drive+ON%2FOFF");
        return T::GetCurrentState();

    }

    int Execute()
    {
        const int rc = fPower.GetState();

        if (rc==Power::State::kCoolingFailure && T::GetCurrentState()!=Power::State::kCoolingFailure)
            T::Error("Power control unit reported cooling failure.");

        return fPower.GetState();
    }


public:
    StateMachinePowerControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "PWR_CONTROL"), fPower(*this, *this)
    {
        // State names
        T::AddStateName(Power::State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(Power::State::kConnected, "Connected",
                     "Connection established, but status still not known");

        T::AddStateName(Power::State::kSystemOff, "PowerOff",
                     "Camera, Bias and Drive power off");

        T::AddStateName(Power::State::kBiasOn, "BiasOn",
                     "Camera and Drive power off, Bias on");

        T::AddStateName(Power::State::kDriveOn, "DriveOn",
                     "Camera and Bias power off, Drive on");

        T::AddStateName(Power::State::kCameraOn, "CameraOn",
                     "Drive and Bias power off, Camera on");

        T::AddStateName(Power::State::kBiasOff, "BiasOff",
                     "Camera and Drive power on, Bias off");

        T::AddStateName(Power::State::kDriveOff, "DriveOff",
                     "Camera and Bias power on, Drive off");

        T::AddStateName(Power::State::kCameraOff, "CameraOff",
                     "Drive and Bias power on, Camera off");

        T::AddStateName(Power::State::kSystemOn, "SystemOn",
                     "Camera, Bias and drive power on");

        T::AddStateName(Power::State::kCoolingFailure, "CoolingFailure",
                     "The cooling unit has failed, the interlock has switched off");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachinePowerControl::SetVerbosity, this, placeholders::_1))
            ("Set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for interpreted data (yes/no)");

        T::AddEvent("SET_DEBUG_RX", "B:1")
            (bind(&StateMachinePowerControl::SetDebugRx, this, placeholders::_1))
            ("Set debux-rx state"
             "|debug[bool]:dump received text and parsed text to console (yes/no)");

        T::AddEvent("CAMERA_POWER", "B:1")
            (bind(&StateMachinePowerControl::SetCameraPower, this, placeholders::_1))
            ("Switch camera power"
             "|power[bool]:Switch camera power 'on' or 'off'");

        T::AddEvent("TOGGLE_DRIVE")
            (bind(&StateMachinePowerControl::ToggleDrive, this))
            ("Toggle drive power");

        T::AddEvent("POST", "C")
            (bind(&StateMachinePowerControl::Post, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
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
    po::options_description control("Interlock control");
    control.add_options()
        ("no-dim,d",   po_switch(),    "Disable dim services")
        ("addr,a",     var<string>(""),  "Network address of the lid controling Arduino including port")
        ("url,u",      var<string>(""),  "File name and path to load")
        ("quiet,q",    po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(5), "Interval between two updates on the server in seconds")
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
        "The pwrctrl is an interface to the interlock hardware.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: pwrctrl [-c type] [OPTIONS]\n"
        "  or:  pwrctrl [OPTIONS]\n";
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
            return RunShell<LocalStream, StateMachine, ConnectionInterlock>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionInterlock>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionInterlock>(conf);
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
