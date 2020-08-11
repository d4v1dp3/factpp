#include <boost/bind.hpp>
#if BOOST_VERSION < 104400
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#undef BOOST_HAS_RVALUE_REFS
#endif
#endif
#include <boost/thread.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "Connection.h"
#include "Configuration.h"
#include "Timers.h"
#include "Console.h"

#include "tools.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;

using ba::deadline_timer;
using ba::ip::tcp;

using namespace std;


// ------------------------------------------------------------------------

#include "LocalControl.h"

// ------------------------------------------------------------------------

class ConnectionFAD : public Connection
{
    MessageImp &fMsg;

    int state;

    char fReadBuffer[1000];

public:
    void ConnectionEstablished()
    {
        StartAsyncRead();
    }

    void HandleReadTimeout(const bs::error_code &error)
    {
        return;
        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            return;
        }

        // 125: Operation canceled

        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;

            str << "HandleReadTimeout: " << error.message() << " (" << error << ")";// << endl;
            if (error==ba::error::misc_errors::eof)
                Warn(str); // Connection: EOF (closed by remote host)
            else
                Error(str);
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fInTimeout.expires_at() > deadline_timer::traits_type::now())
            return;

        Error("fInTimeout has expired...");

       PostClose();
    }

    void HandleReceivedData(const bs::error_code& error, size_t bytes_received, int)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || error)
        {
            // 107: Transport endpoint is not connected
            // 125: Operation canceled
            if (error && error!=ba::error::basic_errors::not_connected)
            {
                ostringstream str;
                str << "Reading from " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
                Error(str);
            }
            PostClose(error!=ba::error::basic_errors::operation_aborted);
            return;
        }

        string txt;

        if (bytes_received==2)
        {
            txt = string(fReadBuffer, bytes_received);
            //std::vector<char> buf(128);
            //bytes_transferred = sock.receive(boost::asio::buffer(d3));

            fMsg() << "Received b=" << bytes_received << ": " << (int)fReadBuffer[0] << " " << (int)txt[0] << " '" << txt << "' " << " " << error.message() << "  (" << error << ")" << endl;

            if (fReadBuffer[0]=='T')
            {
                // AsyncRead + Deadline
                // Do all manipulation to the buffer BEFORE this call!
                AsyncRead(ba::buffer(fReadBuffer+2, 21)/*,
                          &Connection::HandleReceivedData*/);
                AsyncWait(fInTimeout, 5000, &Connection::HandleReadTimeout);
            }
            else
            {
                // AsyncRead + Deadline
                // Do all manipulation to the buffer BEFORE this call!
                AsyncRead(ba::buffer(fReadBuffer+2, 35)/*,
                          &Connection::HandleReceivedData*/);
                AsyncWait(fInTimeout, 5000, &Connection::HandleReadTimeout);
            }
        }
        else
        {
            txt = string(fReadBuffer, bytes_received+2);
            const int s = atoi(fReadBuffer+35);
            if (s==9)
                Info("Requested time received: "+txt);
            else
                state = s;

            Out() << "Received b=" << bytes_received << ": " << (int)fReadBuffer[0] << " " << (int)txt[0] << " '" << txt << "' " << " " << error.message() << "  (" << error << ")" << endl;
            memset(fReadBuffer, 0, 100);

            // Do all manipulation to the buffer BEFORE this call!
            AsyncRead(ba::buffer(fReadBuffer, 2)/*,
                      &Connection::HandleReceivedData*/);


        }
    }

    int GetState() const { return state; }

    void StartAsyncRead()
    {
        // Start also a dealine_time for a proper timeout
        // Therefore we must know how often we expect messages
        // FIXME: Add deadline_counter

        memset(fReadBuffer, 0, 100);

        // AsyncRead + Deadline
        AsyncRead(ba::buffer(fReadBuffer, 2)/*,
                  &Connection::HandleReceivedData*/);
        AsyncWait(fInTimeout, 5000, &Connection::HandleReadTimeout);
    }

    /*
     ConnectionFAD(ba::io_service& io_service, const string &addr, int port) :
     Connection(io_service, addr, port), state(0) { }
     ConnectionFAD(ba::io_service& io_service, const string &addr, const string &port) :
     Connection(io_service, addr, port), state(0) { }
     */

    ConnectionFAD(ba::io_service& ioservice, MessageImp &imp) :
    Connection(ioservice, imp()), fMsg(imp), state(0)
    {
    }
};

template <class T>
class StateMachineFAD : public T, public ba::io_service
{
public:
    enum states_t
    {
        kSM_Disconnected = 1,
        kSM_Connecting,
        kSM_Connected,
        kSM_Running,
        kSM_SomeRunning,
        kSM_Starting,
        kSM_Stopping,
        kSM_Reconnect,
        kSM_SetUrl,
    };

    ConnectionFAD c1;
    ConnectionFAD c2;
    ConnectionFAD c3;
    ConnectionFAD c4;
    ConnectionFAD c5;
    ConnectionFAD c6;
    ConnectionFAD c7;
    ConnectionFAD c8;
    ConnectionFAD c9;

    /*
    int Write(const Time &time, const char *txt, int qos)
    {
        return T::Write(time, txt, qos);
    }
    */
    Timers fTimers;

    StateMachineFAD(const string &name="", ostream &out=cout) :
        T(out, name),
        c1(*this, *this), c2(*this, *this), c3(*this, *this), c4(*this, *this),
        c5(*this, *this), c6(*this, *this), c7(*this, *this), c8(*this, *this),
        c9(*this, *this), fTimers(out)
    {
//        c1.SetEndpoint();
        c2.SetEndpoint("localhost", 4001);
        c3.SetEndpoint("ftmboard1.ethz.ch", 5000);
        c4.SetEndpoint("localhost", 4003);
        c5.SetEndpoint("localhost", 4004);
        c6.SetEndpoint("localhost", 4005);
        c7.SetEndpoint("localhost", 4006);
        c8.SetEndpoint("localhost", 4007);
        c9.SetEndpoint("localhost", 4008);

        c1.SetLogStream(this);
        c2.SetLogStream(this);
        c3.SetLogStream(this);
        c4.SetLogStream(this);
        c5.SetLogStream(this);
        c6.SetLogStream(this);
        c7.SetLogStream(this);
        c8.SetLogStream(this);
        c9.SetLogStream(this);

        c1.StartConnect(); // This sets the connection to "open"
        c2.StartConnect(); // This sets the connection to "open"
        c3.StartConnect(); // This sets the connection to "open"
        //c4.StartConnect(); // This sets the connection to "open"
        //c5.StartConnect(); // This sets the connection to "open"
        //c6.StartConnect(); // This sets the connection to "open"
        //c7.StartConnect(); // This sets the connection to "open"
        //c8.StartConnect(); // This sets the connection to "open"
        //c9.StartConnect(); // This sets the connection to "open"

        AddStateName(kSM_Disconnected,  "Disconnected");
        AddStateName(kSM_Connecting,    "Connecting"); // Some connected
        AddStateName(kSM_Connected,     "Connected");
        AddStateName(kSM_Running,       "Running");
        AddStateName(kSM_SomeRunning,   "SomeRunning");
        AddStateName(kSM_Starting,      "Starting");
        AddStateName(kSM_Stopping,      "Stopping");

        AddEvent(kSM_Running,   "START", kSM_Connected).
            AssignFunction(boost::bind(&StateMachineFAD::Start, this, _1, 5));
        AddEvent(kSM_Connected, "STOP",  kSM_Running);

        AddEvent("TIME", kSM_Running);
        AddEvent("LED",  kSM_Connected);

        T::AddEvent("TESTI",    "I");
        T::AddEvent("TESTI2",   "I:2");
        T::AddEvent("TESTIF",   "I:2;F:2");
        T::AddEvent("TESTIC",   "I:2;C");

        T::AddEvent("CMD", "C").
            AssignFunction(boost::bind(&StateMachineFAD::Command, this, _1));

        AddEvent(kSM_Reconnect, "RECONNECT");

        AddEvent(kSM_SetUrl, "SETURL", "C");
    }

    int Command(const EventImp &evt)
    {
        string cmd = evt.GetText();

        size_t p0 = cmd.find_first_of(' ');
        if (p0==string::npos)
            p0 = cmd.length();

    T::Out() << "\nCommand: '" << cmd.substr(0, p0) << "'" << cmd.substr(p0)<< "'" << endl;
    /*
    const Converter c(T::Out(), "B:5;I:2;F;W;O;C", "yes no false 0 1 31 42 11.12 \"test hallo\" ");

     T::Out() << c.GetRc() << endl;
     T::Out() << c.N() << endl;
     T::Out() << c.Get<bool>(0) << endl;
     T::Out() << c.Get<bool>(1) << endl;
     T::Out() << c.Get<bool>(2) << endl;
     T::Out() << c.Get<bool>(3) << endl;
     T::Out() << c.Get<bool>(4) << endl;
     T::Out() << c.Get<int>(5) << endl;
     T::Out() << c.Get<int>(6) << endl;
     T::Out() << c.Get<float>(7) << endl;
     T::Out() << c.Get<int>(7) << endl;
     T::Out() << c.Get<string>(8) << endl;
     T::Out() << c.Get<string>(9) << endl;
     T::Out() << c.Get<string>(10) << endl;
     */
     return T::GetCurrentState();
    }
    int Start(const EventImp &evt, int i)
    {
        switch (evt.GetTargetState())
        {
        case kSM_Running:    // We are coming from kRunning
        case kSM_Starting:   // We are coming from kConnected
            T::Out() << "Received Start(" << i << ")" << endl;
            c1.PostMessage("START", 10);
            c2.PostMessage("START", 10);
            // We could introduce a "waiting for execution" state
            return T::GetCurrentState();
        }
        return T::kSM_FatalError;
    }

    void Close()
    {
        c1.PostClose();
        c2.PostClose();
        c3.PostClose();
        c4.PostClose();
        c5.PostClose();
        c6.PostClose();
        c7.PostClose();
        c8.PostClose();
        c9.PostClose();
    }


    int Execute()
    {
        // Dispatch at most one handler from the queue. In contrary
        // to run_run(), it doesn't wait until a handler is available
        // which can be dispatched, so poll_one() might return with 0
        // handlers dispatched. The handlers are always dispatched
        // synchronously.

        fTimers.SetT();
        const int n = poll_one();
        fTimers.Proc(n==0 && T::IsQueueEmpty());

//        return c3.IsConnected() ? kSM_Connected : kSM_Disconnected;


        // None is connected
        if (!c1.IsConnected() && !c2.IsConnected())
            return kSM_Disconnected;

        // Some are connected
        if (c1.IsConnected()!=c2.IsConnected())
            return kSM_Connecting;

        if (c1.GetState()==0 && c2.GetState()==0 && T::GetCurrentState()!=kSM_Starting)
            return kSM_Connected;

        if (c1.GetState()==1 && c2.GetState()==1 && T::GetCurrentState()!=kSM_Stopping)
            return kSM_Running;

        return kSM_SomeRunning;//GetCurrentState();
    }

    int Transition(const Event &evt)
    {
        ConnectionFAD *con1 = &c1;
        ConnectionFAD *con2 = &c2;

        switch (evt.GetTargetState())
        {
        case kSM_SetUrl:
            T::Out() << evt.GetText() << endl;
            c1.SetEndpoint(evt.GetText());
            return T::GetCurrentState();
        case kSM_Reconnect:
            // Close all connections
            c1.PostClose(false);
            c2.PostClose(false);
            c3.PostClose(false);

            // Now wait until all connection have been closed and
            // all pending handlers have been processed
            poll();

            // Now we can reopen the connection
            c1.PostClose(true);
            c2.PostClose(true);
            c3.PostClose(true);


            //c4.PostClose(true);
            //c5.PostClose(true);
            //c6.PostClose(true);
            //c7.PostClose(true);
            //c8.PostClose(true);
            //c9.PostClose(true);
            return T::GetCurrentState();
        case kSM_Running: // We are coming from kRunning
        case kSM_Starting:   // We are coming from kConnected
            T::Out() << "Received START" << endl;
            con1->PostMessage("START", 10);
            con2->PostMessage("START", 10);
            // We could introduce a "waiting for execution" state
            return T::GetCurrentState();
            return kSM_Starting; //GetCurrentState();

        case kSM_Connected:   // We are coming from kConnected
        case kSM_Stopping: // We are coming from kRunning
            T::Out() << "Received STOP" << endl;
            con1->PostMessage("STOP", 10);
            con2->PostMessage("STOP", 10);
            // We could introduce a "waiting for execution" state
            return T::GetCurrentState();
            return kSM_Stopping;//GetCurrentState();
        }

        return T::kSM_FatalError; //evt.GetTargetState();
    }
    int Configure(const Event &evt)
    {
        if (evt.GetName()=="TIME")
        {
            c1.PostMessage("TIME", 10);
            c2.PostMessage("TIME", 10);
        }

        vector<char> v(2);
        v[0] = 0xc0;
        v[1] = 0x00;

        if (evt.GetName()=="LED")
            c3.PostMessage(v);

        return T::GetCurrentState();
    }
};

// ------------------------------------------------------------------------

template<class S>
int RunDim(Configuration &conf)
{
    /*
     initscr();		      // Start curses mode
     cbreak();		      // Line buffering disabled, Pass on
     intrflush(stdscr, FALSE);
     start_color();            // Initialize ncurses colors
     use_default_colors();     // Assign terminal default colors to -1
     for (int i=1; i<8; i++)
        init_pair(i, i, -1);  // -1: def background
        scrollok(stdscr, true);
        */

    WindowLog wout;

    //log.SetWindow(stdscr);
    if (conf.Has("log"))
        if (!wout.OpenLogFile(conf.Get<string>("log")))
            wout << kRed << "ERROR - Couldn't open log-file " << conf.Get<string>("log") << ": " << strerror(errno) << endl;

    // Start io_service.Run to use the StateMachineImp::Run() loop
    // Start io_service.run to only use the commandHandler command detaching
    StateMachineFAD<S> io_service("DATA_LOGGER", wout);
    io_service.Run();

    return 0;
}

template<class T, class S>
int RunShell(Configuration &conf)
{
    static T shell(conf.GetName().c_str(), conf.Get<int>("console")!=1);

    WindowLog &win  = shell.GetStreamIn();
    WindowLog &wout = shell.GetStreamOut();

    if (conf.Has("log"))
        if (!wout.OpenLogFile(conf.Get<string>("log")))
            win << kRed << "ERROR - Couldn't open log-file " << conf.Get<string>("log") << ": " << strerror(errno) << endl;

    StateMachineFAD<S> io_service("DATA_LOGGER", wout);
    shell.SetReceiver(io_service);

    boost::thread t(boost::bind(&StateMachineFAD<S>::Run, &io_service));

    //io_service.SetReady();

    shell.Run();                 // Run the shell
    io_service.Stop();           // Signal Loop-thread to stop
    // io_service.Close();       // Obsolete, done by the destructor
    // wout << "join: " << t.timed_join(boost::posix_time::milliseconds(0)) << endl;

    // Wait until the StateMachine has finished its thread
    // before returning and destroying the dim objects which might
    // still be in use.
    t.join();

    return 0;
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
    cout << "\n"
        "The console connects to all available Dim Servers and allows to "
        "easily access all of their commands.\n"
        "\n"
        "Usage: test3 [-c type] [OPTIONS]\n"
        "  or:  test3 [OPTIONS]\n"
        "\n"
        "Options:\n"
        "The following describes the available commandline options. "
        "For further details on how command line option are parsed "
        "and in which order which configuration sources are accessed "
        "please refer to the class reference of the Configuration class.";
    cout << endl;

}

void PrintHelp()
{
    cout << "\n"
        "The default is that the program is started without user interaction. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen."
        << endl;

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

/*
 The first line of the --version information is assumed to be in one
 of the following formats:

   <version>
   <program> <version>
   {GNU,Free} <program> <version>
   <program> ({GNU,Free} <package>) <version>
   <program> - {GNU,Free} <package> <version>

 and separated from any copyright/author details by a blank line.

 Handle multi-line bug reporting sections of the form:

   Report <program> bugs to <addr>
   GNU <package> home page: <url>
   ...
*/
void PrintVersion(const char *name)
{
    cout <<
        name << " - "PACKAGE_STRING"\n"
        "\n"
        "Written by Thomas Bretz et al.\n"
        "\n"
        "Report bugs to <"PACKAGE_BUGREPORT">\n"
        "Home page: "PACKAGE_URL"\n"
        "\n"
        "Copyright (C) 2011 by the FACT Collaboration.\n"
        "This is free software; see the source for copying conditions.\n"
        << endl;
}


void SetupConfiguration(Configuration &conf)
{
    const string n = conf.GetName()+".log";

    po::options_description config("Program options");
    config.add_options()
        ("dns",       var<string>("localhost"),  "Dim nameserver host name (Overwites DIM_DNS_NODE environment variable)")
        ("log,l",     var<string>(n), "Write log-file")
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("console,c", var<int>(),     "Use console (0=shell, 1=simple buffered, X=simple unbuffered)")
        ;

    conf.AddEnv("dns", "DIM_DNS_NODE");

    conf.AddOptions(config);
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    po::variables_map vm;
    try
    {
        vm = conf.Parse(argc, argv);
    }
    catch (std::exception &e)
    {
#if BOOST_VERSION > 104000
        po::multiple_occurrences *MO = dynamic_cast<po::multiple_occurrences*>(&e);
        if (MO)
            cout << "Error: " << e.what() << " of '" << MO->get_option_name() << "' option." << endl;
        else
#endif
            cout << "Error: " << e.what() << endl;
        cout << endl;

        return -1;
    }

    if (conf.HasPrint())
        return -1;

    if (conf.HasVersion())
    {
        PrintVersion(argv[0]);
        return -1;
    }

    if (conf.HasHelp())
    {
        PrintHelp();
        return -1;
    }

    // To allow overwriting of DIM_DNS_NODE set 0 to 1
    setenv("DIM_DNS_NODE", conf.Get<string>("dns").c_str(), 1);

    try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
            if (conf.Get<bool>("no-dim"))
                return RunDim<StateMachine>(conf);
            else
                return RunDim<StateMachineDim>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine>(conf);
            else
                return RunShell<LocalConsole, StateMachine>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim>(conf);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

/*
class FADctrlDim : public StateMachineFAD<StateMachineDim>
{
public:
FADctrlDim(const std::string &name="DATA_LOGGER", std::ostream &out=std::cout)
: StateMachineFAD<StateMachineDim>(out, name) { }
};

 class FADctrlLocalShell : public StateMachineFAD<StateMachine>
{
public:
    ostream &win;

    FADctrlLocalShell(std::ostream &out, std::ostream &out2)
        : StateMachineFAD<StateMachine>(out), win(out2) { }

    FADctrlLocalShell(std::ostream &out=std::cout)
        : StateMachineFAD<StateMachine>(out), win(out) { }

};
*/
