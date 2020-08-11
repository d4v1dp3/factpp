#include <valarray>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "Connection.h"
#include "Configuration.h"
#include "Console.h"

#include "tools.h"

#include "LocalControl.h"

#include "HeadersFAD.h"
#include "HeadersEventServer.h"

#include "fits.h"
#include "nova.h"

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------

class StateMachineEventServer : public StateMachineDim
{
public:
    static bool fIsServer;

private:

    DimDescribedState fDimFadControl;

    string        fAuxPath;
    string        fOutPath;
    uint32_t      fStartDate;
    uint32_t      fNight;
    uint32_t      fInterval;

    fits         *fIn;

    uint32_t      fQoS;
    double        fTime;
    uint32_t      fRun;
    uint32_t      fEvt;
    //vector<float> fAvg;
    //vector<float> fRms;
    vector<float> fMax;
    //vector<float> fPos;

    Time fTimer;

    int Execute()
    {
        if (GetCurrentState()==StateMachineImp::kSM_Ready)
            return EventServer::State::kRunning;

        // Nothing to do if not started
        if (GetCurrentState()==EventServer::State::kIdle)
            return EventServer::State::kIdle;

        if (fDimFadControl.state()==FAD::State::kRunInProgress)
            return EventServer::State::kStandby;

        // Run only once in 5s
        const Time now;
        if (now<fTimer+boost::posix_time::milliseconds(fInterval))
            return GetCurrentState();
        fTimer = now;

        // Running is only allowed when sun is up
        const Nova::RstTime rst = Nova::GetSolarRst(now.JD()-0.5);
        const bool isUp =
            (rst.rise<rst.set && (now.JD()>rst.rise && now.JD()<rst.set)) ||
            (rst.rise>rst.set && (now.JD()<rst.set  || now.JD()>rst.rise));

        // FIXME: What about errors?
        if (!isUp)
            return EventServer::State::kStandby;

        // Check if file has to be changed
        const uint32_t night = fStartDate>0 ? fStartDate : Time(Time().Mjd()-1).NightAsInt();

        if (fNight!=night)
        {
            fNight = night;

            delete fIn;

            const string name = fAuxPath + Tools::Form("/%04d/%02d/%02d/%08d.FAD_CONTROL_EVENT_DATA.fits", night/10000, (night/100)%100, night%100, night);

            fIn = new fits(name);
            if (!fIn->is_open())
            {
                Error(string("Failed to open "+name+": ")+strerror(errno));
                return StateMachineImp::kSM_Error;
            }

            // Requiered columns
            try
            {
                fIn->SetRefAddress("QoS",  fQoS);
                fIn->SetRefAddress("Time", fTime);
                fIn->SetVecAddress("max",  fMax);
                //fIn->SetVecAddress(fRms);
                //fIn->SetVecAddress(fMax);
                //fIn->SetVecAddress(fPos);
            }
            catch (const runtime_error &e)
            {
                delete fIn;
                fIn = 0;

                Error("Failed to open "+name+": "+e.what());
                return StateMachineImp::kSM_Error;
            }

            // Non requiered columns
            fRun = 0;
            fEvt = 0;
            try
            {
                fIn->SetRefAddress("run",  fRun);
                fIn->SetRefAddress("evt",  fEvt);
            }
            catch (const runtime_error &e) { }

            Info("File "+name+" open.");
        }

        if (GetCurrentState()==StateMachineImp::kSM_Error)
            return  StateMachineImp::kSM_Error;

        // Get next data event
        vector<float> sorted;
        while (1)
        {
            if (!fIn->GetNextRow())
            {
                fNight = 0;
                return EventServer::State::kRunning;
            }

            // Select a
            if (fQoS & FAD::EventHeader::kAll)
                continue;

            for (int i=0; i<1440; i++)
                fMax[i] /= 1000;

            //for (int i=8; i<1440; i+=9)
            //    fMax[i] = fMax[i-2];

            // construct output
            sorted = fMax;
            sort(sorted.begin(), sorted.end());

            const double med = sorted[719];

            vector<float> dev(1440);
            for (int i=0; i<1440; i++)
                dev[i] = fabs(sorted[i]-med);
            sort(dev.begin(), dev.end());

            const double deviation = dev[uint32_t(0.682689477208650697*1440)];

            // In a typical shower or muon ring, the first
            // few pixels will have comparable brightness,
            // in a NSB event not. Therefore, the 4th brightest
            // pixel is a good reference.
            if (sorted[1439-3]>med+deviation*5)
                break;
        }

        const double scale = max(0.25f, sorted[1436]);

        ostringstream out;
        out << Time(fTime+40587).JavaDate() << '\n';
        out << "0\n";
        out << scale << '\n';
        out << setprecision(3);
        if (fRun>0)
            //out << "DEMO [" << fEvt << "]\nDEMO [" << fRun << "]\nDEMO\x7f";
            out << fEvt << '\n' << fRun << "\nDEMO\x7f";
        else
            out << "DEMO\nDEMO\nDEMO\x7f";

        //out << sorted[1439] << '\n';
        //out << sorted[719]  << '\n';
        //out << sorted[0]    << '\x7f';

        // The valid range is from 1 to 127
        // \0 is used to seperate different curves
        vector<uint8_t> val(1440);
        for (uint64_t i=0; i<1440; i++)
        {
            float range = nearbyint(126*fMax[i]/scale); // [-2V; 2V]
            if (range>126)
                range=126;
            if (range<0)
                range=0;
            val[i] = (uint8_t)range;
        }

        const char *ptr = reinterpret_cast<char*>(val.data());
        out.write(ptr, val.size()*sizeof(uint8_t));
        out << '\x7f';

        if (fOutPath=="-")
            Out() << out.str();
        else
            ofstream(fOutPath+"/cam-fadcontrol-eventdata.bin") << out.str();

        return EventServer::State::kRunning;
    }

    int StartServer()
    {
        fStartDate = 0;
        return EventServer::State::kRunning;
    }

    int StopServer()
    {
        delete fIn;
        fIn = 0;

        return EventServer::State::kIdle;
    }

    int StartDate(const EventImp &evt)
    {
        fStartDate = evt.GetUInt();
        return EventServer::State::kRunning;
    }

    int Print() const
    {
        Out() << fDimFadControl << endl;
        return GetCurrentState();
    }


public:
    StateMachineEventServer(ostream &out=cout) : StateMachineDim(out, fIsServer?"EVENT_SERVER":""),
        fDimFadControl("FAD_CONTROL"), fStartDate(0), fNight(0), fIn(0), fMax(1440)

    {
        fDimFadControl.Subscribe(*this);

        // State names
        AddStateName(EventServer::State::kIdle, "Idle",
                     "Event server stopped.");
        AddStateName(EventServer::State::kRunning, "Running",
                     "Reading events file and writing to output.");
        AddStateName(EventServer::State::kStandby, "Standby",
                     "No events are processed, either the sun is down or fadctrl in kRunInProgress.");


        AddEvent("START", EventServer::State::kIdle, EventServer::State::kRunning, StateMachineImp::kSM_Error)
            (bind(&StateMachineEventServer::StartServer, this))
            ("Start serving the smartfact camera file");

        AddEvent("START_DATE", "I:1", EventServer::State::kIdle, EventServer::State::kRunning, StateMachineImp::kSM_Error)
            (bind(&StateMachineEventServer::StartDate, this, placeholders::_1))
            ("Start serving the smartfact camera file with the events from the given date"
             "|uint32[yyyymmdd]:Integer representing the date from which the data should be read");

        AddEvent("STOP", EventServer::State::kRunning, EventServer::State::kStandby, StateMachineImp::kSM_Error)
            (bind(&StateMachineEventServer::StopServer, this))
            ("Stop serving the smartfact camera file");

        AddEvent("PRINT")
            (bind(&StateMachineEventServer::Print, this))
            ("Print current status");
    }
    ~StateMachineEventServer()
    {
        delete fIn;
    }

    int EvalOptions(Configuration &conf)
    {
        fAuxPath  = conf.Get<string>("aux-path");
        fOutPath  = conf.Get<string>("out-path");
        fInterval = conf.Get<uint32_t>("interval");

        return -1;
    }
};

bool StateMachineEventServer::fIsServer = false;

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    StateMachineEventServer::fIsServer = !conf.Get<bool>("client");
    return Main::execute<T, StateMachineEventServer>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Event server options");
    control.add_options()
        ("aux-path", var<string>("/fact/aux"), "The root path to the auxilary files.")
        ("out-path", var<string>("www/smartfact/data"), "Path where the output camera file should be written.")
        ("interval", var<uint32_t>(5000), "Interval of updates in milliseconds.")
        ("client",   po_bool(false), "For a standalone client choose this option.")
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
        "The event server copies events from fits files to the smartfact data.\n"
        "\n"
        "Usage: evtserver [-c type] [OPTIONS]\n"
        "  or:  evtserver [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineEventServer>();

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

    //try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
//            if (conf.Get<bool>("no-dim"))
//                return RunShell<LocalStream, StateMachine, ConnectionFSC>(conf);
//            else
                return RunShell<LocalStream>(conf);
        }
        // Cosole access w/ and w/o Dim
/*        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionFSC>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionFSC>(conf);
        }
        else
*/        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell>(conf);
            else
                return RunShell<LocalConsole>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
