#include <iostream>

#include "dim.h"

#include "Dim.h"
#include "EventImp.h"
#include "WindowLog.h"
#include "Configuration.h"
#include "StateMachineDim.h"
#include "ReadlineColor.h"

using namespace std;

void SetupConfiguration(Configuration &conf)
{
    const string n = conf.GetName()+".log";

    po::options_description config("Program options");
    config.add_options()
        ("dns",       var<string>("localhost"),       "Dim nameserver (overwites DIM_DNS_NODE environment variable)")
        ("port",      var<uint16_t>(DNS_PORT),        "Port to connect to dim nameserver.")
        ("host",      var<string>(""),                "Address with which the Dim nameserver can connect to this host (overwites DIM_HOST_NODE environment variable)")
        ("log,l",     var<string>(n), "Write log-file")
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
    cout <<
        "The chatserv is a Dim-based chat server.\n"
        "\n"
        "It is a non-interactive program which acts as a relay of messages "
        "sent via a Dim command CHAT/MSG and which are redirected to the "
        "logging service CHAT/MESSAGE.\n"
        "\n"
        "Usage: chatserv [OPTIONS]\n"
        "  or:  chatserv [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    /* Additional help text which is printed after the configuration
     options goes here */
}

class ChatServer : public StateMachineDim
{
private:
    int HandleMsg(const EventImp &evt)
    {
        Comment(evt.GetString());
        return GetCurrentState();
    }
public:
    ChatServer(ostream &lout) : StateMachineDim(lout, "CHAT")
    {
        AddEvent("MSG", "C")
            (bind(&ChatServer::HandleMsg, this, placeholders::_1))
            ("|msg[string]:message to be distributed");
    }
};

int main(int argc, const char *argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    Dim::Setup(conf.Get<string>("dns"), conf.Get<string>("host"), conf.Get<uint16_t>("port"));

    WindowLog log;

    ReadlineColor::PrintBootMsg(log, conf.GetName(), false);

    if (conf.Has("log"))
        if (!log.OpenLogFile(conf.Get<string>("log")))
            cerr << "ERROR - Couldn't open log-file " << conf.Get<string>("log") << ": " << strerror(errno) << endl;

    return ChatServer(log).Run();
}

// **************************************************************************
/** @example chatserv.cc

The program is stopped by CTRL-C

*/
// **************************************************************************
