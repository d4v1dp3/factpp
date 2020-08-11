#include "StateMachineDim.h"

#include "tools.h"
#include "Time.h"
#include "Configuration.h"
#include "LocalControl.h"

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

// ========================================================================
// ========================================================================
// ========================================================================

class StateMachineTimeCheck : public StateMachineDim
{
private:
    Time fLastUpdate;

    string   fServer;
    uint16_t fInterval;

    DimDescribedService fService;

    enum
    {
        kStateOutOfRange = 1,
        kStateRunning    = 2,
    };

    // ------------- Initialize variables before the Dim stuff ------------

    int Execute()
    {
        Time now;
        if (now-fLastUpdate<boost::posix_time::minutes(fInterval))
            return kStateRunning;

        fLastUpdate=now;

        const string cmd = "ntpdate -q "+fServer;

        Info("Calling '"+cmd+"'");

        // !!!!! Warning: this is a blocking operation !!!!!
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            const string err = strerror(errno);
            Error("Could not create pipe '"+cmd+"': "+err);
            return 0x100;
        }

        vector<string> args;

        string line;
        while (1)
        {
            const int rc = fgetc(pipe);
            if (rc==EOF || rc=='\n')
            {
                args.push_back(Tools::Trim(line));
                break;
            }

            if (rc==',')
            {
                args.push_back(Tools::Trim(line));
                line = "";
                continue;
            }

            line += static_cast<unsigned char>(rc);
        }
        pclose(pipe);

        if (args.size()!=4)
        {
            Error("First returned line contains other than four arguments (separated by commas)");
            return 0x100;
        }

        if (args[2].substr(0, 7)!="offset ")
        {
            Error("Argument 3 '"+args[2]+"' is not what it ought to be.");
            return 0x100;
        }

        try
        {
            const float offset = stof(args[2].substr(7));
            fService.Update(offset);

            const string msg = "NTP: "+fServer+" returned: "+args[2]+" ms";

            if (offset>=1000)
            {
                Warn(msg);
                return kStateOutOfRange;
            }

            Message(msg);

        }
        catch (const exception &e)
        {
            Error("Converting offset '"+args[2]+"' to float failed: "+e.what());
            return 0x100;
        }

        return kStateRunning;
    }

    int Trigger()
    {
        fLastUpdate = Time()-boost::posix_time::minutes(fInterval);
        return GetCurrentState();
    }

public:
    StateMachineTimeCheck(ostream &out=cout) : StateMachineDim(out, "TIME_CHECK"),
        fService("TIME_CHECK/OFFSET", "F:1", "Time offset measured with ntp|offset[ms]:Time offset in milliseconds")
    {
        // State names
        AddStateName(kStateRunning,    "Valid",      "Last check was valid.");
        AddStateName(kStateOutOfRange, "OutOfRange", "Last time check exceeded 1s.");

        AddEvent("TRIGGER");
            (bind(&StateMachineTimeCheck::Trigger, this))
            ("Trigger update");

    }
    int EvalOptions(Configuration &conf)
    {
        fServer   = conf.Get<string>("ntp-server");
        fInterval = conf.Get<uint16_t>("interval");
        if (fInterval==0)
            fInterval=1;

        Trigger();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineTimeCheck>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Time check");
    control.add_options()
        ("ntp-server", var<string>("hora.roa.es"),  "The ntp server to be queried")
        ("interval",   var<uint16_t>(15),           "Interval in minutes the ntp server should be queried")
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
    /*
    cout <<
        "SmartFACT is a tool writing the files needed for the SmartFACT web interface.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: smartfact [-c type] [OPTIONS]\n"
        "  or:  smartfact [OPTIONS]\n";
    cout << endl;*/
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineTimeCheck>();

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

    if (!conf.Has("console"))
        return RunShell<LocalStream>(conf);

    if (conf.Get<int>("console")==0)
        return RunShell<LocalShell>(conf);
    else
        return RunShell<LocalConsole>(conf);

    return 0;
}
