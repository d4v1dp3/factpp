#include "StateMachineDimControl.h"

//#include <sys/stat.h>

#include "RemoteControl.h"

using namespace std;

// ------------------------------------------------------------------------

#include "Main.h"

namespace fs = boost::filesystem;

template<class T>
int RunShell(Configuration &conf)
{
#if BOOST_VERSION < 104600
    const string fname = fs::path(conf.GetName()).filename();
#else
    const string fname = fs::path(conf.GetName()).filename().string();
#endif

    StateMachineDimControl::fIsServer = fname=="dimserver";
    return Main::execute<T, StateMachineDimControl>(conf);
}

void SetupConfiguration(Configuration &conf)
{
#if BOOST_VERSION < 104600
    const string fname = fs::path(conf.GetName()).filename();
#else
    const string fname = fs::path(conf.GetName()).filename().string();
#endif

    po::options_description control("Options ("+fname+")");
    control.add_options()
        ("force-console", po_switch(),     "Forces console mode in server-mode.")
        ("debug",         po_bool(false),  "Print the labels for debugging purpose")
        ("user,u",        var<string>(""), "A user name - just for logging purposes (default is ${USER})")
        ("JavaScript.*",  var<string>(),   "Additional arguments which are provided to JavaScripts started either locally or remotely in the dimserver via the START command")
        ;

    if (fname!="dimserver")
    {
        control.add_options()
            ("batch",   var<string>(), "Start a batch script with the given name at the given label (script.dim[:N]) on the dimctrl-server")
            ("start",   var<string>(), "Start a java script with the given name on the dimctrl-server")
            ("stop",    po_switch(),   "Stop a currently running script on the dimctrl-server")
            ("interrupt", var<string>()->implicit_value(""), "Send an interrupt request (IRQ) to a running JavaScript.")
            ("restart", var<string>(), "Send 'EXIT 126' to the given server")
            ("shutdown", po_switch(), "Shutdown the whole network 'DIS_DNS/KILL_SERVERS'")
            ("msg",     var<string>(), "Send a message to the chat server.")
            ;
    }

    conf.AddEnv("user", "USER");

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
        "The dim control is a central master for the dim network.\n"
        "\n"
        "The program can be started as a dim server, so that it is visible "
        "in the dim network to other clients. If started as a client (dimctrl), "
        "it can only interact passively with the dim network. The usual case "
        "should be to have one server running (dimserver) and control it from "
        "a dimctrl started.\n"
        "\n"
        "Usage: dimctrl [-c type] [OPTIONS]\n"
        "  or:  dimctrl [OPTIONS]\n"
        "  or:  dimserver [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineDimControl>();

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
    //chmod(argv[0], 04775);

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    if (conf.Get<bool>("force-console") && !conf.Has("console"))
        throw runtime_error("--force-console must be used with --console/-c");

#if BOOST_VERSION < 104600
    const string fname = fs::path(conf.GetName()).filename();
#else
    const string fname = fs::path(conf.GetName()).filename().string();
#endif

    if (fname=="dimserver" && !conf.Get<bool>("force-console"))
        conf.Remove("console");

    if (!conf.Has("console"))
        return RunShell<RemoteStream>(conf);

    if (conf.Get<int>("console")==0)
        return RunShell<RemoteShell>(conf);
    else
        return RunShell<RemoteConsole>(conf);
}
