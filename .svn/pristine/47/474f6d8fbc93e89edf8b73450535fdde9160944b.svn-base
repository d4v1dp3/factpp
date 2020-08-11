#include <boost/array.hpp>

#include <string>    // std::string
#include <algorithm> // std::transform
#include <cctype>    // std::tolower

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

#include "HeadersLid.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

class ConnectionLid : public Connection
{
protected:

    struct Lid
    {
        int id;

        float position;
        float current;
        string status;

        Lid(int i) : id(i) { }

        bool Set(const QDomNamedNodeMap &map)
        {
            if (!map.contains("id") || !map.contains("value"))
                return false;

            QString item  = map.namedItem("id").nodeValue();
            QString value = map.namedItem("value").nodeValue();

            const char c = '0'+id;

            if (item==(QString("cur")+c))
            {
                current = value.toFloat();
                return true;
            }

            if (item==(QString("pos")+c))
            {
                position = value.toFloat();
                return true;
            }

            if (item==(QString("lid")+c))
            {
                status = value.toStdString();
                return true;
            }

            return false;
        }

        void Print(ostream &out)
        {
            out << "Lid" << id << " @ " << position << " / " << current << "A [" << status << "]" << endl;
        }

    };

private:
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;
    string fRdfData;

    boost::array<char, 4096> fArray;

    string fNextCommand;

    Time fLastReport;

    Lid fLid1;
    Lid fLid2;

    virtual void Update(const Lid &, const Lid &)
    {
    }


    void ProcessAnswer()
    {
        if (fIsVerbose)
        {
            Out() << "------------------------------------------------------" << endl;
            Out() << fRdfData << endl;
            Out() << "------------------------------------------------------" << endl;
        }

        fRdfData.insert(0, "<?xml version=\"1.0\"?>\n");

        QDomDocument doc;
        if (!doc.setContent(QString(fRdfData.c_str()), false))
        {
            Warn("Parsing of html failed.");
            return;
        }

        if (fIsVerbose)
        {
            Out() << "Parsed:\n-------\n" << doc.toString().toStdString() << endl;
            Out() << "------------------------------------------------------" << endl;
        }

        const QDomNodeList imageElems = doc.elementsByTagName("span"); // "input"

        /*
        // elementById
        for (unsigned int i=0; i<imageElems.length(); i++)
        {
            QDomElement e = imageElems.item(i).toElement();
            Out() << "<" << e.tagName().toStdString() << " ";

            QDomNamedNodeMap att = e.attributes();

            for (int j=0; j<att.size(); j++)
            {
                Out() << att.item(j).nodeName().toStdString() << "=";
                Out() << att.item(j).nodeValue().toStdString() << " ";
            }
            Out() << "> " << e.text().toStdString() << endl;
        }*/

        for (unsigned int i=0; i<imageElems.length(); i++)
        {
            const QDomElement e = imageElems.item(i).toElement();

            const QDomNamedNodeMap att = e.attributes();

            fLid1.Set(att);
            fLid2.Set(att);
        }

        if (fIsVerbose)
        {
            fLid1.Print(Out());
            fLid2.Print(Out());
            Out() << "------------------------------------------------------" << endl;
        }

        Update(fLid1, fLid2);

        fRdfData = "";

        if ((fLid1.status!="Open" && fLid1.status!="Closed" && fLid1.status!="Power Problem" && fLid1.status!="Unknown" && fLid1.status!="Overcurrent") ||
            (fLid2.status!="Open" && fLid2.status!="Closed" && fLid2.status!="Power Problem" && fLid2.status!="Unknown" && fLid1.status!="Overcurrent"))
            Warn("Lid reported status unknown by lidctrl ("+fLid1.status+"/"+fLid2.status+")");

        fLastReport = Time();
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
            {
                //Warn("Connection closed by remote host.");
                ProcessAnswer();
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

            fRdfData = "";
            return;
        }

        fRdfData += string(fArray.data(), bytes_received);

        //cout << "." << flush;

        // Does the message contain a header?
        const size_t p1 = fRdfData.find("\r\n\r\n");
        if (p1!=string::npos)
        {
            // Does the answer also contain the body?
            const size_t p2 = fRdfData.find("\r\n\r\n", p1+4);
            if (p2!=string::npos)
            {
                ProcessAnswer();
            }
        }

        // Go on reading until the web-server closes the connection
        StartReadReport();
    }

    boost::asio::streambuf fBuffer;

    void StartReadReport()
    {
        async_read_some(ba::buffer(fArray),
                        boost::bind(&ConnectionLid::HandleRead, this,
                                    dummy::error, dummy::bytes_transferred));
    }

    boost::asio::deadline_timer fKeepAlive;

    void PostRequest(string cmd, const string &args="")
    {
        cmd += " "+fSite+" HTTP/1.1\r\n"
            //"Connection: Keep-Alive\r\n"
            ;

        ostringstream msg;
        msg << args.length();

        cmd += "Content-Length: ";
        cmd += msg.str();
        cmd +="\r\n";

        if (args.length()>0)
            cmd += "\r\n"+args + "\r\n";

        cmd += "\r\n";

        //cout << "Post: " << cmd << endl;
        PostMessage(cmd);
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

    void ConnectionFailed()
    {
        StartConnect();
    }

public:
    static const uint16_t kMaxAddr;

public:
    ConnectionLid(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fLastReport(Time::none),
        fLid1(1), fLid2(2), fKeepAlive(ioservice)
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

    void Post(const string &post)
    {
        fNextCommand = post;

        fLid1.status = "";
        fLid2.status = "";
        //PostRequest("POST", post);
    }

    void Request()
    {
        PostRequest("POST", fNextCommand);
        fNextCommand = "";

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval));
        fKeepAlive.async_wait(boost::bind(&ConnectionLid::HandleRequest,
                                          this, dummy::error));
    }

    int GetInterval() const
    {
        return fInterval;
    }

    int GetState() const
    {
        using namespace Lid;

        // Timeout
        if (fLastReport.IsValid() && fLastReport+boost::posix_time::seconds(fInterval*2)<Time())
            return State::kDisconnected;

        // Unidentified state detected
        if ((!fLid1.status.empty() && fLid1.status!="Open" && fLid1.status!="Closed" && fLid1.status!="Power Problem" && fLid1.status!="Unknown" && fLid1.status!="Overcurrent") ||
            (!fLid2.status.empty() && fLid2.status!="Open" && fLid2.status!="Closed" && fLid2.status!="Power Problem" && fLid2.status!="Unknown" && fLid2.status!="Overcurrent"))
            return State::kUnidentified;

        // This is an assumption, but the best we have...
        if (fLid1.status=="Closed" && fLid2.status=="Power Problem")
            return State::kClosed;
        if (fLid2.status=="Closed" && fLid1.status=="Power Problem")
            return State::kClosed;
        if (fLid1.status=="Open" && fLid2.status=="Power Problem")
            return State::kOpen;
        if (fLid2.status=="Open" && fLid1.status=="Power Problem")
            return State::kOpen;

        // Inconsistency
        if (fLid1.status!=fLid2.status)
            return State::kInconsistent;

        // Unknown
        if (fLid1.status=="Unknown")
            return State::kUnknown;

        // Power Problem
        if (fLid1.status=="Power Problem")
            return State::kPowerProblem;

        // Overcurrent
        if (fLid1.status=="Overcurrent")
            return State::kOvercurrent;

        // Closed
        if (fLid1.status=="Closed")
            return State::kClosed;

        // Open
        if (fLid1.status=="Open")
            return State::kOpen;

        return State::kConnected;
    }
};

const uint16_t ConnectionLid::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimLid : public ConnectionLid
{
private:
    DimDescribedService fDim;

public:
    ConnectionDimLid(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionLid(ioservice, imp),
        fDim("LID_CONTROL/DATA", "S:2;F:2;F:2",
             "|status[bool]:Lid1/2 open or closed"
             "|I[A]:Lid1/2 current"
             "|P[dac]:Lid1/2 hall sensor position in averaged dac counts")
    {
    }

    void Update(const Lid &l1, const Lid &l2)
    {
        struct DimData
        {
            int16_t status[2];
            float   current[2];
            float   position[2];

            DimData() { status[0] = status[1] = -1; }

        } __attribute__((__packed__));

        DimData data;

        if (l1.status=="Unknown")
            data.status[0] = 3;
        if (l1.status=="Power Problem")
            data.status[0] = 2;
        if (l1.status=="Open")
            data.status[0] = 1;
        if (l1.status=="Closed")
            data.status[0] = 0;

        if (l2.status=="Unknown")
            data.status[1] = 3;
        if (l2.status=="Power Problem")
            data.status[1] = 2;
        if (l2.status=="Open")
            data.status[1] = 1;
        if (l2.status=="Closed")
            data.status[1] = 0;

        data.current[0]  = l1.current;
        data.current[1]  = l2.current;

        data.position[0] = l1.position;
        data.position[1] = l2.position;

        fDim.setQuality(GetState());
        fDim.Update(data);
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineLidControl : public StateMachineAsio<T>
{
private:
    S fLid;
    Time fLastCommand;
    Time fSunRise;

    uint16_t fTimeToMove;

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

        fLid.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int Post(const EventImp &evt)
    {
        fLid.Post(evt.GetText());
        return T::GetCurrentState();
    }

    int Open()
    {
        fLastCommand = Time();
        fLid.Post("Button5=");
        return Lid::State::kMoving;
    }
    int Close()
    {
        fLastCommand = Time();
        fLid.Post("Button6=");
        return Lid::State::kMoving;

    }
    /*
    int MoveMotor(const EventImp &evt, int mid)
    {
        if (!CheckEventSize(evt.GetSize(), "MoveMotor", 2))
            return T::kSM_FatalError;

        if (evt.GetUShort()>0xfff)
        {
            ostringstream msg;
            msg << "Position " << evt.GetUShort() << " for motor " << mid+1 << " out of range [0,1023].";
            T::Error(msg);
            return T::GetCurrentState();
        }

        fLid.MoveMotor(mid, evt.GetUShort());

        return T::GetCurrentState();
    }*/

    int Unlock()
    {
        return fLid.GetState();
    }

    int Execute()
    {
        const int rc = fLid.GetState();
        const int state = T::GetCurrentState();

        if (state==Lid::State::kMoving &&
            (rc==Lid::State::kConnected || rc==Lid::State::kDisconnected) &&
            fLastCommand+boost::posix_time::seconds(fTimeToMove+fLid.GetInterval()) > Time())
        {
            return Lid::State::kMoving;
        }

        const Time now;
        if (now>fSunRise)
        {
            if (state!=Lid::State::kClosed && state!=Lid::State::kLocked && state>Lid::State::kDisconnected)
            {
                T::Error("Lidctrl not in 'Closed' at end of nautical twilight!");
                Close();
            }

            fSunRise = now.GetNextSunRise(-6);

            ostringstream msg;
            msg << "During next sun-rise nautical twilight will end at " << fSunRise;
            T::Info(msg);

            return Lid::State::kLocked;
        }

        return rc==Lid::State::kConnected ? state : rc;
    }


public:
    StateMachineLidControl(ostream &out=cout) :
        StateMachineAsio<T>(out, "LID_CONTROL"), fLid(*this, *this),
        fSunRise(Time().GetNextSunRise(-6))
    {
        // State names
        T::AddStateName(Lid::State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(Lid::State::kConnected, "Connected",
                     "Connection established, but status still not known");

        T::AddStateName(Lid::State::kUnidentified, "Unidentified",
                     "At least one lid reported a state which could not be identified by lidctrl");

        T::AddStateName(Lid::State::kInconsistent, "Inconsistent",
                     "Both lids show different states");

        T::AddStateName(Lid::State::kUnknown, "Unknown",
                     "Arduino reports at least one lids in an unknown status");

        T::AddStateName(Lid::State::kPowerProblem, "PowerProblem",
                     "Arduino reports both lids to have a power problem (might also be that both are at the end switches)");

        T::AddStateName(Lid::State::kOvercurrent, "Overcurrent",
                     "Arduino reports both lids to have a overcurrent (might also be that both are at the end switches)");

        T::AddStateName(Lid::State::kClosed, "Closed",
                     "Both lids are closed");

        T::AddStateName(Lid::State::kOpen, "Open",
                     "Both lids are open");

        T::AddStateName(Lid::State::kMoving, "Moving",
                     "Lids are supposed to move, waiting for next status");

        T::AddStateName(Lid::State::kLocked, "Locked",
                        "Locked, no commands accepted except UNLOCK.");


        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineLidControl::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("OPEN", Lid::State::kUnidentified, Lid::State::kInconsistent, Lid::State::kUnknown, Lid::State::kPowerProblem, Lid::State::kClosed)
            (bind(&StateMachineLidControl::Open, this))
            ("Open the lids");

        T::AddEvent("CLOSE")(Lid::State::kUnidentified)(Lid::State::kInconsistent)(Lid::State::kUnknown)(Lid::State::kOvercurrent)(Lid::State::kPowerProblem)(Lid::State::kOpen)
            (bind(&StateMachineLidControl::Close, this))
            ("Close the lids");

        T::AddEvent("POST", "C")(Lid::State::kUnidentified)(Lid::State::kInconsistent)(Lid::State::kUnknown)(Lid::State::kOvercurrent)(Lid::State::kPowerProblem)(Lid::State::kOpen)(Lid::State::kClosed)(Lid::State::kMoving)
            (bind(&StateMachineLidControl::Post, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("UNLOCK", Lid::State::kLocked)
            (bind(&StateMachineLidControl::Unlock, this))
            ("Unlock if in locked state.");
    }

    int EvalOptions(Configuration &conf)
    {
        fLid.SetVerbose(!conf.Get<bool>("quiet"));
        fLid.SetInterval(conf.Get<uint16_t>("interval"));
        fLid.SetDebugTx(conf.Get<bool>("debug-tx"));
        fLid.SetSite(conf.Get<string>("url"));
        fLid.SetEndpoint(conf.Get<string>("addr"));
        fLid.StartConnect();

        fTimeToMove = conf.Get<uint16_t>("time-to-move");

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineLidControl<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Lid control");
    control.add_options()
        ("no-dim,d",   po_switch(),    "Disable dim services")
        ("addr,a",     var<string>(""),  "Network address of the lid controling Arduino including port")
        ("url,u",      var<string>(""),  "File name and path to load")
        ("quiet,q",    po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(5), "Interval between two updates on the server in seconds")
        ("time-to-move", var<uint16_t>(20), "Expected minimum time the lid taks to open/close")
        ("debug-tx",   po_bool(), "Enable debugging of ethernet transmission.")
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
        "The lidctrl is an interface to the LID control hardware.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: lidctrl [-c type] [OPTIONS]\n"
        "  or:  lidctrl [OPTIONS]\n";
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
            return RunShell<LocalStream, StateMachine, ConnectionLid>(conf);
        else
            return RunShell<LocalStream, StateMachineDim, ConnectionDimLid>(conf);
    }
    // Cosole access w/ and w/o Dim
    if (conf.Get<bool>("no-dim"))
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachine, ConnectionLid>(conf);
        else
            return RunShell<LocalConsole, StateMachine, ConnectionLid>(conf);
    }
    else
    {
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell, StateMachineDim, ConnectionDimLid>(conf);
        else
            return RunShell<LocalConsole, StateMachineDim, ConnectionDimLid>(conf);
    }

    return 0;
}
