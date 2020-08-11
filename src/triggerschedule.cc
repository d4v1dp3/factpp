#include <iostream>
#include <dic.hxx>

#include "Dim.h"
#include "Configuration.h"

using namespace std;

void SetupConfiguration(Configuration &conf)
{
    po::options_description config("Configuration");
    config.add_options()
        ("dns",                    var<string>("localhost"),  "Dim nameserver host name (Overwites DIM_DNS_NODE environment variable)")
        ("schedule-database-name", var<string>(), "Database name for scheduling")
        ;

    po::positional_options_description p;
    p.add("schedule-database-name", 1); // The first positional options

    conf.AddEnv("dns", "DIM_DNS_NODE");
    conf.AddOptions(config);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "The triggerschedule triggers the scheduler.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: triggerschedule [-c type] [OPTIONS] <schedule-database-name>\n"
        "  or:  triggerschedule [OPTIONS] <schedule-database-name>\n";
    cout << endl;
}

void PrintHelp()
{
    cout <<
        "\n"
        "The method sendCommand(...) will wait for the command to "
        "be actualy sent to the server and return a completion code "
        "of:\n"
        " 0 - if it was successfully sent.\n"
        " 1 - if it couldn't be delivered.\n "
        << endl;
    /* Additional help text which is printed after the configuration
     options goes here */
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return -1;

    const string dbname = conf.Get<string>("schedule-database-name");

    Dim::Setup(conf.Get<string>("dns"));

    const int rc = DimClient::sendCommand("SCHEDULER/SCHEDULE", dbname.c_str());
    if (!rc)
        cerr << "Sending failed!" << endl;
    else
        cout << "Command issued successfully." << endl;

    return !rc;
}
