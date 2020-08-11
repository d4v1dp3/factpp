#include "FactGui.h"

#include <TQtWidget.h>
#include <TSystem.h>

#include "src/FACT.h"
#include "src/Dim.h"
#include "src/Configuration.h"

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
        "The FACT++ Graphical User Interfact (GUI).\n"
        "\n"
        "Usage: fact [-c type] [OPTIONS]\n"
        "  or:  fact [OPTIONS]\n";
    cout << endl;

}

void PrintHelp()
{
    /* Additional help text which is printed after the configuration
     options goes here */
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description config("Program options");
    config.add_options()
        ("dns",  var<string>("localhost"), "Dim nameserver (overwites DIM_DNS_NODE environment variable)")
        ("host", var<string>(""),          "Address with which the Dim nameserver can connect to this host (overwites DIM_HOST_NODE environment variable)")
        ("pixel-map-file",  var<string>("FACTmap111030.txt"), "Pixel mapping file. Used here to get the default reference voltage.")
        ("CommentDB",  var<string>(""), "")
        ;

    po::options_description runtype("Run type configuration");
    runtype.add_options()
        ("run-type",  vars<string>(),   "Names of available run-types")
        ("run-time",  vars<string>(),   "Possible run-times for runs")
        ("run-count", vars<uint32_t>(), "Number of events for a run")
        ;

    conf.AddEnv("dns",  "DIM_DNS_NODE");
    conf.AddEnv("host", "DIM_HOST_NODE");

    conf.AddOptions(config);
    conf.AddOptions(runtype);
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return -1;

    Dim::Setup(conf.Get<string>("dns"), conf.Get<string>("host"));

    cout << "LD_LIBRARY_PATH=" << gSystem->GetDynamicPath() << endl;

    cout << "--- Starting QApplication ---" << endl;
    QApplication app(argc, const_cast<char**>(argv));

    cout << "--- Working around a root bug ---" << endl;
    {
        // It seems sometimes TGQt::RegisterWid is called before
        // fWidgetArray is initialized, so we force that to happen here
//        TQtWidget(NULL);
//        cout << "LD_LIBRARY_PATH=" << gSystem->GetDynamicPath() << endl;
    }

    cout << "--- Instantiating GUI ---" << endl;
    FactGui gui(conf);

    cout << "--- Show GUI ---" << endl;
    gui.show();

    cout << "--- Main loop ---" << endl;

    const int rc = app.exec();

    cout << "The end." << endl;

    return rc;
}
