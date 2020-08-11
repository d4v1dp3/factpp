#ifndef FACT_Main
#define FACT_Main

#include <map>
#include <thread>
#include <functional>

#include <boost/filesystem.hpp>

#include "dim.h"

#include "Dim.h"
#include "Time.h"
#include "MainImp.h"
#include "Readline.h"
#include "WindowLog.h"
#include "MessageImp.h"
#include "Configuration.h"

namespace Main
{
    using namespace std;
    namespace fs = boost::filesystem;

    void SetupConfiguration(Configuration &conf)
    {
        const auto n = fs::path(conf.GetName()+".log");

        po::options_description config("Program options");
        config.add_options()
            ("dns",        var<string>("localhost"), "Dim nameserver (overwites DIM_DNS_NODE environment variable)")
            ("port",       var<uint16_t>(DNS_PORT),  "Port to connect to dim nameserver.")
            ("host",       var<string>(""),          "Address with which the Dim nameserver can connect to this host (overwites DIM_HOST_NODE environment variable)")
            ("log,l",      var<string>(n.filename().string()), "Name of local log-file")
            ("logpath",    var<string>(n.parent_path().string()),  "Path to log-files")
            ("no-log",     po_switch(),    "Supress log-file")
            ("append-log", po_bool(),      "Append log information to local log-file")
            ("null",       po_switch(),    "Suppresses almost all console output - including errors (only available without --console option)")
            ("console,c",  var<int>(),     "Use console (0=shell, 1=simple buffered, X=simple unbuffered)")
            ("cmd",        vars<string>(), "Execute one or more commands at startup")
            ("exec,e",     vars<string>(), "Execute one or more scrips at startup ('file:N' - start at label N)")
            ("arg:*",      var<string>(),  "Arguments for script execution with --exc, e.g. --arg:ra='12.5436'")
            ("home",       var<string>(),  "Path to home directory (used as default for logpath if standard log files not writable)")
            ("quit",       po_switch(),    "Quit after startup");
        ;

        conf.AddEnv("dns",  "DIM_DNS_NODE");
        conf.AddEnv("port", "DIM_DNS_PORT");
        conf.AddEnv("host", "DIM_HOST_NODE");
        conf.AddEnv("home", "HOME");

        conf.AddOptions(config);
    }

    void PrintUsage()
    {
        cout <<
            "Files:\n"
            "The following files are written by each program by default\n"
            "  program.evt:   A log of all executed of skipped events\n"
            "  program.his:   The history accessible by Pg-up/dn\n"
            "  program.log:   All output piped to the log-stream\n"
            << endl;
    }

    template<class T>
    void PrintHelp()
    {
        Dim::Setup();

        ofstream fout("/dev/null");

        T io_service(fout);

        io_service.PrintListOfStates(cout);
        cout << "\nList of available commands:\n";
        io_service.PrintListOfEvents(cout);
        cout << "\n";
    }

    void Thread(MainImp *io_service, bool dummy, int &rc)
    {
        // This is necessary so that the StateMachien Thread can signal the
        // Readline to exit
        rc = io_service->Run(dummy);
        Readline::Stop();
    }

    template<class T, class S>
    int execute(Configuration &conf, bool dummy=false)
    {
        Dim::Setup(conf.Get<string>("dns"), conf.Get<string>("host"), conf.Get<uint16_t>("port"));

        // -----------------------------------------------------------------
        const fs::path program(conf.GetName());

        // Split path to program into path and filename
        const string prgpath = program.parent_path().string();

#if BOOST_VERSION < 104600
        const string prgname = program.filename();
#else
        const string prgname = program.filename().string();
#endif

        fs::path path = conf.Has("logpath") ? conf.Get<string>("logpath") : "";

        // No explicit path given
        if (path.empty())
        {
            path = prgpath.empty() ? "." : prgpath;

            // default path not accessible
            if (access(prgpath.c_str(), W_OK))
            {
                path = ".";

                if (conf.Has("home"))
                {
                    path  = conf.Get<string>("home");
                    path /= ".fact++";
                }
            }
        }

        // Create directories if necessary
        if (path!=".")
            fs::create_directories(path);

        // -----------------------------------------------------------------

        static T shell((path/prgname).string().c_str(),
                       conf.Has("console") ? conf.Get<int>("console")!=1 : conf.Get<bool>("null"));

        WindowLog &win  = shell.GetStreamIn();
        WindowLog &wout = shell.GetStreamOut();

        // Switching off buffering is not strictly necessary, since
        // the destructor of shell should flush everything still buffered,
        // nevertheless it helps to debug problems in the initialization
        // sequence.
        const bool backlog = wout.GetBacklog();
        const bool null    = wout.GetNullOutput();
        if (conf.Has("console") || !conf.Get<bool>("null"))
        {
            wout.SetBacklog(false);
            wout.SetNullOutput(false);
            wout.Display(true);
        }

        if (conf.Has("log") && !conf.Get<bool>("no-log"))
        {
#if BOOST_VERSION < 104600
            const fs::path file = fs::path(conf.Get<string>("log")).filename();
#else
            const fs::path file = fs::path(conf.Get<string>("log")).filename();
#endif
            cerr << "Writing logfile to '" << (path/file).string() << endl;

            if (!wout.OpenLogFile((path/file).string(), conf.Get<bool>("append-log")))
                win << kYellow << "WARNING - Couldn't open log-file " << (path/file).string() << ": " << strerror(errno) << endl;
        }

        S io_service(wout);

        const Time now;
        io_service.Write(now, "/----------------------- Program ------------------------");
        io_service.Write(now, "| Program:  " PACKAGE_STRING " ("+prgname+":"+to_string(getpid())+")");
        io_service.Write(now, "| CallPath: "+prgpath);
        io_service.Write(now, "| Compiled: " __DATE__ " " __TIME__ );
        io_service.Write(now, "| Revision: " REVISION);
        io_service.Write(now, "| DIM:      v"+to_string(DIM_VERSION_NUMBER/100)+"r"+to_string(DIM_VERSION_NUMBER%100)+" ("+io_service.GetName()+")");
        io_service.Write(now, "| Contact:  " PACKAGE_BUGREPORT);
        io_service.Write(now, "| URL:      " PACKAGE_URL);
        io_service.Write(now, "| Start:    "+now.GetAsStr("%c"));
        io_service.Write(now, "\\----------------------- Options ------------------------");
        const multimap<string,string> mmap = conf.GetOptions();
        for (auto it=mmap.begin(); it!=mmap.end(); it++)
            io_service.Write(now, ": "+it->first+(it->second.empty()?"":" = ")+it->second);

        const map<string,string> &args = conf.GetOptions<string>("arg:");
        if (!args.empty())
        {
            io_service.Write(now, "------------------------ Arguments ----------------------", MessageImp::kMessage);

            for (auto it=args.begin(); it!=args.end(); it++)
            {
                ostringstream str;
                str.setf(ios_base::left);
                str << ": " << it->first << " = " << it->second;
                io_service.Write(now, str.str(), MessageImp::kMessage);
            }
        }

        io_service.Write(now, "\\------------------- Evaluating options -----------------");
        const int rc = io_service.EvalOptions(conf);
        if (rc>=0)
        {
            ostringstream str;
            str << "Exit triggered by EvalOptions with rc=" << rc;
            io_service.Write(now, str.str(), rc==0?MessageImp::kInfo:MessageImp::kError);
            return rc;
        }

        const map<string,string> &wco = conf.GetWildcardOptions();
        if (!wco.empty())
        {
            io_service.Write(now, "------------- Unrecognized wildcard options -------------", MessageImp::kWarn);

            size_t max = 0;
            for (auto it=wco.begin(); it!=wco.end(); it++)
                if (it->second.length()>max)
                    max = it->second.length();

            for (auto it=wco.begin(); it!=wco.end(); it++)
            {
                ostringstream str;
                str.setf(ios_base::left);
                str << setw(max+1) << it->second << " : " << it->first;
                io_service.Write(now, str.str(), MessageImp::kWarn);
            }
            io_service.Write(now, "Unrecognized options found, will exit with rc=127", MessageImp::kError);
            return 127;
        }

        io_service.Message("==================== Starting main loop =================");

        if (conf.Has("console") || !conf.Get<bool>("null"))
        {
            wout.SetNullOutput(null);
            wout.SetBacklog(backlog);
        }

        shell.SetReceiver(io_service);

        int ret = 0;
        thread t(bind(Main::Thread, &io_service, dummy, ref(ret)));

        // Wait until state machine is ready (The only case I can imagine
        // in which the state will never chane is when DIM triggers
        // an exit before the state machine has been started at all.
        // Hopefully checking the readline (see Threed) should fix
        // that -- difficult to test.)
        while ((io_service.GetCurrentState()<StateMachineImp::kSM_Ready ||
                !io_service.MessageQueueEmpty()) && !shell.IsStopped())
            usleep(1);

        // Execute command line commands
        const vector<string> v1 = conf.Vec<string>("cmd");
        for (vector<string>::const_iterator it=v1.begin(); it!=v1.end(); it++)
            shell.ProcessLine(*it);

        const vector<string> v2 = conf.Vec<string>("exec");
        for (vector<string>::const_iterator it=v2.begin(); it!=v2.end(); it++)
            shell.Execute(*it, args);

        // Run the shell if no immediate exit was requested
        if (!conf.Get<bool>("quit"))
            shell.Run();

        io_service.Stop();           // Signal Loop-thread to stop
        // io_service.Close();       // Obsolete, done by the destructor
        // wout << "join: " << t.timed_join(boost::posix_time::milliseconds(0)) << endl;

        // Wait until the StateMachine has finished its thread
        // before returning and destroying the dim objects which might
        // still be in use.
        t.join();

        return ret;
    }
}

#endif
