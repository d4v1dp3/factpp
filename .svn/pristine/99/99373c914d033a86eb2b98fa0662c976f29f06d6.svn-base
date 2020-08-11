#include "Configuration.h"

#include <iostream>

using namespace std;

// --------------------------------------------------------------------------
//
//!  Main Doxygen/Autotools integration example program.
//!
//!  @param conf  Number of command line options.
//!  @param opt  The command line options.
//!  @return      The exit status.
//
void SetupConfiguration(Configuration &conf, int &opt)
{
    /*
     // Default in case the option was not specified
     typed_value* default_value(const T& v)
     typed_value* default_value(const T& v, const std::string& textual)

     // Default value in case the option was given
     //    forces -o    to become -o5
     //    forces --opt to become --opt=3
     typed_value* implicit_value(const T &v)
     typed_value* implicit_value(const T &v, const std::string& textual)

     // notifier function when the final value is determined
     typed_value* notifier(function1<void, const T&> f)

     /// Merge values from different sources (e.g. file, command line)
     typed_value* composing()

     // Specifies that the value can span multiple tokens.
     typed_value* multitoken()
     typed_value* zero_tokens()

     // Specifies that the value must occur.
     typed_value* required()
     */

    // To merge the options from several parsers (e.g. comand_line and
    // config file) use po_strings()->composing()
    /*
    po::options_description generic("Generic options");
    generic.add_options()
        ("help-config",   "Print available configuration file options.")
        ("help-env",      "Print available environment variables.")
        ("help",          "Print available commandline options.")
        ("print-unknown", "Print unrecognized options.")
        ("config", po_string("config.txt"), "Set configuration file name.")
        ;


    // Declare the supported options.
    po::options_description generic("Generaic options");
    generic.add_options()
//        ("testreq",     po_int()->required(),         "set compression level (madatory)")
        ("default",     po_string("my_default"),      "set compression level")
        ("unknown",     po_int(1),                    "set compression level")
        ("U",           po_int(2)->implicit_value(1), "set compression level")
        ;
      */
    // Declare a group of options that will be
    // allowed both on command line and in
    // config file
    po::options_description config("Configuration");
    config.add_options()
        ("compression",    var<int>(),                      "set compression level")
        ("optimization",   var<int>(10, &opt),              "optimization level")
        ("test-def",       var<int>(42),                    "optimization level")
        ("include-path,I", vars<string>()/*->composing()*/, "include path")
        ("test,T",         vars<string>()/*->composing()*/, "include path")
        ("file1",          vars<string>(),                   "include path")
        ("int1",           var<int>(),                      "include path")
        ("Int2",           var<int>(),                      "include path")
        ("Int1",           var<int>(),                      "include path")
        ("test-db",        var<string>("database"),                      "include path")
        ("float1",         var<double>(),                   "include path")
//        (",A",          po_float(),                    "include path")
        ("radec",         po::value<vector<double>>(),                    "include path")
        ("switch",        po_switch(),                    "include path")
        ("bool",          var<bool>()->implicit_value(true),                    "include path")
        ;

    // !!! Option which are "shorted" must be placed last.
    //     Can this be switched off?

    po::options_description sections("Sections");
    config.add_options()
        ("unregistered",            var<string>(),                    "include path")
        ("Section1.unregistered",   var<string>(),                    "include path")
//        ("Section2*",               po_string(),                    "include path")
        // The latter accepts all options starting with Section2.
        ;

    // Hidden options, will be allowed both on command line and
    // in config file, but will not be shown to the user.
    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file",  vars<string>(), "input file")
        ("output-file", vars<string>(), "output file")
        ("test-file",   vars<string>(), "test file")
        ;

    po::options_description env("Environment options");
    env.add_options()
        ("linux", var<string>(), "LINUX env")
        ("path",  var<string>(), "PATH env")
        ("dns",   var<string>(), "DIM_DNS_SERVER env")
        ;

    conf.AddEnv("linux", "LINUX");
    conf.AddEnv("path",  "PATH");
    conf.AddEnv("dns",   "DIM_DNS_SERVER");

    // define translation from position to name
    po::positional_options_description p;
    p.add("output-file", 2); // The first 2 positional options is output-file
    p.add("test-file",   3); // The next three positional options is output-file
    p.add("input-file", -1); // All others go to...

    conf.AddOptionsCommandline(config);
    conf.AddOptionsCommandline(sections);
    conf.AddOptionsCommandline(hidden, false);

    conf.AddOptionsConfigfile(config);
    conf.AddOptionsConfigfile(sections);
    conf.AddOptionsConfigfile(hidden, false);

    conf.AddOptionsEnvironment(env);

    conf.AddOptionsDatabase(config);

    conf.SetArgumentPositions(p);
}


int main(int argc, const char **argv)
{
    int opt;

    Configuration conf(argv[0]);
    SetupConfiguration(conf, opt);

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

    if (conf.HasHelp() || conf.HasPrint())
        return -1;

    cout << "------------------------------" << endl;

    cout << "Program " << argv[0] << " started successfully." << endl;

    cout << conf.Has("switch") << " " << conf.Get<bool>("switch") << endl;
    cout << conf.Has("bool") << " " << conf.Get<bool>("bool") << endl;

    return 0;
/*
    if (vm.count("compression"))
        cout << "Compression level was set to " << vm["compression"].as<int>() << ".\n";
    else
        cout << "Compression level was not set.\n";


    cout << "Test default is always: " << vm["test-def"].as<int>() << "\n";
    cout << "Optimization level is " << vm["optimization"].as<int>() << "\n";
    //cout << "Int2: " << vm["Int2"].as<int>() << "\n";

    cout << conf.GetString("unregistered") << endl;
    cout << conf.GetString("Section1.unregistered") << endl;
    cout << conf.Has("Section2.unregistered") << endl;
    cout << conf.GetString("Section2.Section3.unregistered") << endl;
    cout << "test-db: " << conf.GetString("test-db") << endl;


    if (vm.count("include-path"))
    {
        vector<string> v = vm["include-path"].as< vector<string> >();
        for (vector<string>::iterator s=v.begin(); s<v.end(); s++)
            cout << "Incl P: " << *s << endl;
    }

    if (vm.count("input-file"))
    {
        vector<string> v = vm["input-file"].as< vector<string> >();
        for (vector<string>::iterator s=v.begin(); s<v.end(); s++)
            cout << "Incl F: " << *s << endl;
    }

    if (vm.count("output-file"))
    {
        vector<string> v = vm["output-file"].as< vector<string> >();
        for (vector<string>::iterator s=v.begin(); s<v.end(); s++)
            cout << "Out: " << *s << endl;
    }

    if (vm.count("test-file"))
    {
        vector<string> v = vm["test-file"].as< vector<string> >();
        for (vector<string>::iterator s=v.begin(); s<v.end(); s++)
            cout << "Testf: " << *s << endl;
    }

    cout << "Linux: " << conf.Get<string>("linux") << endl;

    if (vm.count("path"))
        cout << "Path: "   << vm["path"].as<string>()  << endl;
    if (vm.count("file1"))
        cout << "File1: "  << vm["file1"].as<string>() << endl;
    if (vm.count("int1"))
        cout << "Int1: "   << vm["int1"].as<int>()     << endl;
    if (vm.count("float1"))
        cout << "Float1: " << vm["float1"].as<float>() << endl;

    if (vm.count("test"))
    {
        vector<string> v = vm["test"].as< vector<string> >();
        for (vector<string>::iterator s=v.begin(); s<v.end(); s++)
            cout << "Test: " << *s << endl;
    }*/
}
// ***************************************************************************
/** @example argv.cc

Example for the usage of the class Configuration

**/
// ***************************************************************************
