#include <boost/filesystem.hpp>

#include "dim.h"

#include "Configuration.h"
#include "ChatClient.h"
#include "DimSetup.h"

using namespace std;

template <class T>
void RunShell(Configuration &conf)
{
    // A normal kill will call its destructor! (Very nice feature ;) )
    static T shell(conf.GetName().c_str(), conf.Get<int>("console")!=1);

    WindowLog &win  = shell.GetStreamIn();
    WindowLog &wout = shell.GetStreamOut();

    if (conf.Has("log"))
        if (!wout.OpenLogFile(conf.Get<string>("log")))
            win << kRed << "ERROR - Couldn't open log-file " << conf.Get<string>("log") << ": " << strerror(errno) << endl;

    shell.Run();
}


// ========================================================================
void SetupConfiguration(Configuration &conf)
{
    const string n = conf.GetName()+".log";

    po::options_description config("Program options");
    config.add_options()
        ("dns",       var<string>("localhost"),       "Dim nameserver (overwites DIM_DNS_NODE environment variable)")
        ("port",      var<uint16_t>(DNS_PORT),        "Port to connect to dim nameserver.")
        ("host",      var<string>(""),                "Address with which the Dim nameserver can connect to this host (overwites DIM_HOST_NODE environment variable)")
        ("log,l",     var<string>(n), "Write log-file")
        ("console,c", var<int>(0),    "Use console (0=shell, 1=simple buffered, X=simple unbuffered)")
        ;

    conf.AddEnv("dns",  "DIM_DNS_NODE");
    conf.AddEnv("port", "DIM_DNS_PORT");
    conf.AddEnv("host", "DIM_HOST_NODE");

    conf.AddOptions(config);
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
        "The chatclient is a simple Dim based chatclient.\n"
        "\n"
        "The chatclient is always started with user intercation. "
        "Just enter a message. It will be broadcasted through the chatserv, "
        "which need to be running."
        "\n"
        "Usage: chatclient [-c type] [OPTIONS]\n"
        "  or:  chatclient [OPTIONS]\n";
    cout << endl;

}

void PrintHelp()
{
    /* Additional help text which is printed after the configuration
     options goes here */
}

int main(int argc, const char *argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    Dim::Setup(conf.Get<string>("dns"), conf.Get<string>("host"), conf.Get<uint16_t>("port"));

    if (conf.Get<int>("console")==0)
        RunShell<ChatShell>(conf);
    else
        RunShell<ChatConsole>(conf);


    return 0;
}
