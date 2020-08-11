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

#include "HeadersFTM.h"
#include "HeadersRateScan.h"

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------

class StateMachineRateScan : public StateMachineDim
{
private:
    struct config
    {
        int fCounterMax;
        float fResolution;
    };
    map<string, config> fTypes;

    DimVersion fDim;

    DimDescribedState   fDimFTM;
    DimDescribedService fDimData;
    DimDescribedService fDimProc;

    bool fAutoPause;

    int fCounter;
    int fCounterMax;

    int fThreshold;
    int fThresholdMin;
    int fThresholdMax;
    int fThresholdStep;
    int fThresholdStepDyn;

    double fRate;
    double fRateBoard[40];
    double fRatePatch[160];

    double fOnTime;

    uint64_t fStartTime;

    float fResolution;

    enum reference_t
    {
        kCamera,
        kBoard,
        kPatch
    };

    reference_t fReference;
    uint16_t    fReferenceIdx;

    string fCommand;

    void UpdateProc()
    {
        const array<uint32_t,3> v = {{ uint32_t(fThresholdMin), uint32_t(fThresholdMax), uint32_t(fThresholdStep) }};
        fDimProc.Update(v);
    }

    bool CheckEventSize(const EventImp &evt, size_t size)
    {
        if (size_t(evt.GetSize())==size)
            return true;

        if (evt.GetSize()==0)
            return false;

        ostringstream msg;
        msg << evt.GetName() << " - Received event has " << evt.GetSize() << " bytes, but expected " << size << ".";
        Fatal(msg);
        return false;
    }

    int HandleTriggerRates(const EventImp &evt)
    {
        if (!CheckEventSize(evt, sizeof(FTM::DimTriggerRates)))
            return GetCurrentState();

        if (GetCurrentState()<RateScan::State::kInProgress)
            return GetCurrentState();

        const FTM::DimTriggerRates &sdata = *static_cast<const FTM::DimTriggerRates*>(evt.GetData());

        if (++fCounter<0)
            return GetCurrentState();

        if (GetCurrentState()==RateScan::State::kPaused)
            fCounter=0;

        if (fCounter==0)
        {
            fRate = 0;

            memset(fRateBoard, 0,  40*sizeof(double));
            memset(fRatePatch, 0, 160*sizeof(double));

            fOnTime = 0;
            return GetCurrentState();
        }
/*
        if (sdata.fTriggerRate==0)
        {
            Message("Rate scan stopped due zero trigger rate.");
            fThreshold = -1;
            return;
        }
*/

        fRate += sdata.fTriggerRate;
        for (int i=0; i<40; i++)
            fRateBoard[i] += sdata.fBoardRate[i];
        for (int i=0; i<160; i++)
            fRatePatch[i] += sdata.fPatchRate[i];

        double reference = fRate;
        if (fReference==kBoard)
            reference = fRateBoard[fReferenceIdx];
        if (fReference==kPatch)
            reference = fRatePatch[fReferenceIdx];

        fOnTime += sdata.fOnTime;

        reference *= sdata.fElapsedTime;

        if ((reference==0 || sqrt(reference)>fResolution*reference) && fCounter<fCounterMax)
        {
            ostringstream out;
            out << "Triggers so far: " << reference;
            if (reference>0)
                out << " (" << sqrt(reference)/reference << ")";
            Info(out);

            return GetCurrentState();
        }

        const double   time = sdata.fElapsedTime*fCounter;
        const uint32_t th   = fThreshold;

        float data[2+3+1+40+160];
        memcpy(data,   &fStartTime, 8);
        memcpy(data+2, &th, 4);
        data[3] = time;         // total elapsed time
        data[4] = fOnTime/time; // relative on time
        data[5] = fRate/fCounter;
        for (int i=0; i<40; i++)
            data[i+6] = fRateBoard[i]/fCounter;
        for (int i=0; i<160; i++)
            data[i+46] = fRatePatch[i]/fCounter;

        ostringstream sout1, sout2, sout3;

        sout1 << th << " " << data[5];
        for (int i=0; i<200; i++)
            sout2 << " " << data[i+6];
        sout3 << " " << data[3] << " " << data[4];

        Info(sout1.str());

        //ofstream fout("ratescan.txt", ios::app);
        //fout << sout1.str() << sout2.str() << sout3.str() << endl;

        fDimData.setQuality(fCommand=="FTM_CONTROL/SET_THRESHOLD");
        fDimData.setData(data, sizeof(data));
        fDimData.Update();

        fThreshold += fThresholdStep;

        if (fCounter>=fCounterMax)
        {
            Message("Rate scan stopped due to timeout.");
            //Dim::SendCommandNB("FTM_CONTROL/RESET_CONFIGURE");
            return RateScan::State::kConnected;
        }

        if (fThreshold>fThresholdMax)
        {
            Message("Rate scan finished.");
            //Dim::SendCommandNB("FTM_CONTROL/RESET_CONFIGURE");
            return RateScan::State::kConnected;
        }

        // Does this need to be shifted upwards?
        if (fCounter>1 && fThresholdStepDyn>0)
        {
            //const double scale = fCounter/reference/fResolution/fResolution;
            //const double step  = floor(scale*fThresholdStepDyn);

            fThresholdStep = fCounter*fThresholdStepDyn;
        }

        //fCounter = -2;  // FIXME: In principle one missed report is enough
        fCounter = -1;

        const int32_t cmd[2] = { -1, fThreshold };
        Dim::SendCommandNB(fCommand.c_str(), cmd);

        return GetCurrentState();
    }

    int Print() const
    {
        Out() << fDim << endl;
        Out() << fDimFTM << endl;

        return GetCurrentState();
    }

    int StartRateScan(const EventImp &evt, const string &command)
    {
        //FIXME: check at least that size>12
        //if (!CheckEventSize(evt, 12))
        //    return kSM_FatalError;

        const string fType = evt.Ptr<char>(12);

        auto it = fTypes.find(fType);
        if (it==fTypes.end())
        {
            Info("StartRateScan - Type '"+fType+"' not found... trying 'default'.");

            it = fTypes.find("default");
            if (it==fTypes.end())
            {
                Error("StartRateScan - Type 'default' not found.");
                return GetCurrentState();
            }
        }

        fCounterMax = it->second.fCounterMax;
        fResolution = it->second.fResolution;

        fCommand = "FTM_CONTROL/"+command;

        const int32_t step = evt.Get<int32_t>(8);

        fThresholdMin  = evt.Get<uint32_t>();
        fThresholdMax  = evt.Get<uint32_t>(4);
        fThresholdStep = abs(step);

        fThresholdStepDyn = step<0 ? -step : 0;

        UpdateProc();

        //Dim::SendCommand("FAD_CONTROL/SET_FILE_FORMAT", uint16_t(0));
        Dim::SendCommandNB("FTM_CONTROL/CONFIGURE", string("ratescan"));

        Message("Configuration for ratescan started.");

        return RateScan::State::kConfiguring;
    }

    int HandleFtmStateChange(/*const EventImp &evt*/)
    {
        // ftmctrl connected to FTM
        if (GetCurrentState()!=RateScan::State::kConfiguring)
            return GetCurrentState();

        if (fDimFTM.state()!=FTM::State::kConfigured1)
            return GetCurrentState();

        const int32_t data[2] = { -1, fThresholdMin };

        Dim::SendCommandNB("FTM_CONTROL/RESET_CONFIGURE");
        Dim::SendCommandNB(fCommand, data);

        fThreshold = fThresholdMin;
        fCounter = -2;

        const Time now;
        fStartTime = trunc(now.UnixTime());

        /*
        ofstream fout("ratescan.txt", ios::app);
        fout << "# ----- " << now << " (" << fStartTime << ") -----\n";
        fout << "# Command: " << fCommand << '\n';
        fout << "# Reference: ";
        switch (fReference)
        {
        case kCamera: fout << "Camera"; break;
        case kBoard:  fout << "Board #" << fReferenceIdx; break;
        case kPatch:  fout << "Patch #" << fReferenceIdx; break;
        }
        fout << '\n';
        fout << "# -----" << endl;
        */

        ostringstream msg;
        msg << "Rate scan " << now << "(" << fStartTime << ") from " << fThresholdMin << " to ";
        msg << fThresholdMax << " in steps of " << fThresholdStep;
        msg << " with a resolution of " << fResolution ;
        msg << " and max-wait " << fCounterMax  ;
        msg << " started.";
        Message(msg);

        if (!fAutoPause)
            return RateScan::State::kInProgress;

        fAutoPause = false;

        return RateScan::State::kPaused;
    }

    int StopRateScan()
    {
        if (GetCurrentState()<RateScan::State::kConfiguring)
            return GetCurrentState();

        Dim::SendCommandNB("FTM_CONTROL/RESET_CONFIGURE");
        Message("Rate scan manually stopped.");

        return RateScan::State::kConnected;
    }

    int SetReferenceCamera()
    {
        fReference = kCamera;

        return GetCurrentState();
    }

    int SetReferenceBoard(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        if (evt.GetUInt()>39)
        {
            Error("SetReferenceBoard - Board index out of range [0;39]");
            return GetCurrentState();
        }

        fReference    = kBoard;
        fReferenceIdx = evt.GetUInt();

        return GetCurrentState();
    }

    int SetReferencePatch(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        if (evt.GetUInt()>159)
        {
            Error("SetReferencePatch - Patch index out of range [0;159]");
            return GetCurrentState();
        }

        fReference    = kPatch;
        fReferenceIdx = evt.GetUInt();

        return GetCurrentState();
    }

    int ChangeStepSize(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        fThresholdStep = evt.Get<uint32_t>();

        ostringstream msg;
        msg << "New step size " << fThresholdStep;
        Info(msg);

        UpdateProc();

        return GetCurrentState();
    }

    int ChangeMaximum(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        fThresholdMax = evt.Get<uint32_t>();

        return GetCurrentState();
    }

    int TriggerAutoPause()
    {
        fAutoPause = true;
        return GetCurrentState();
    }

    int Pause()
    {
        return RateScan::State::kPaused;
    }

    int Resume()
    {
        return RateScan::State::kInProgress;
    }

    int Execute()
    {
        if (!fDim.online())
            return RateScan::State::kDimNetworkNA;

        // All subsystems are not connected
        if (fDimFTM.state()<FTM::State::kConnected)
            return RateScan::State::kDisconnected;

        // ftmctrl connected to FTM
        if (GetCurrentState()<=RateScan::State::kDisconnected)
            return RateScan::State::kConnected;

        return GetCurrentState();
    }

public:
    StateMachineRateScan(ostream &out=cout) : StateMachineDim(out, "RATE_SCAN"),
        fDimFTM("FTM_CONTROL"),
        fDimData("RATE_SCAN/DATA", "X:1;I:1;F:1;F:1;F:1;F:40;F:160",
                 "|Id[s]:Start time used to identify measurement (UnixTime)"
                 "|Threshold[dac]:Threshold in DAC counts"
                 "|ElapsedTime[s]:Real elapsed time"
                 "|RelOnTime[ratio]:Relative on time"
                 "|TriggerRate[Hz]:Camera trigger rate"
                 "|BoardRate[Hz]:Board trigger rates"
                 "|PatchRate[Hz]:Patch trigger rates"),
        fDimProc("RATE_SCAN/PROCESS_DATA", "I:1;I:1;I:1",
                 "Rate scan process data"
                 "|min[DAC]:Value at which scan was started"
                 "|max[DAC]:Value at which scan will end"
                 "|step[DAC]:Step size for scan"),
        fAutoPause(false), fThreshold(-1), fReference(kCamera), fReferenceIdx(0)
    {
        // ba::io_service::work is a kind of keep_alive for the loop.
        // It prevents the io_service to go to stopped state, which
        // would prevent any consecutive calls to run()
        // or poll() to do nothing. reset() could also revoke to the
        // previous state but this might introduce some overhead of
        // deletion and creation of threads and more.

        fDim.Subscribe(*this);
        fDimFTM.Subscribe(*this);
        fDimFTM.SetCallback(bind(&StateMachineRateScan::HandleFtmStateChange, this));

        Subscribe("FTM_CONTROL/TRIGGER_RATES")
            (bind(&StateMachineRateScan::HandleTriggerRates, this, placeholders::_1));

        // State names
        AddStateName(RateScan::State::kDimNetworkNA, "DimNetworkNotAvailable",
                     "The Dim DNS is not reachable.");

        AddStateName(RateScan::State::kDisconnected, "Disconnected",
                     "The Dim DNS is reachable, but the required subsystems are not available.");

        AddStateName(RateScan::State::kConnected, "Connected",
                     "All needed subsystems are connected to their hardware, no action is performed.");

        AddStateName(RateScan::State::kConfiguring, "Configuring",
                     "Waiting for FTM to get 'Configured'.");

        AddStateName(RateScan::State::kInProgress, "InProgress",
                     "Rate scan in progress.");

        AddStateName(RateScan::State::kPaused, "Paused",
                     "Rate scan in progress but paused.");

        AddEvent("START_THRESHOLD_SCAN", "I:3;C", RateScan::State::kConnected)
            (bind(&StateMachineRateScan::StartRateScan, this, placeholders::_1, "SET_THRESHOLD"))
            ("Start rate scan for the threshold in the defined range"
             "|min[int]:Start value in DAC counts"
             "|max[int]:Limiting value in DAC counts"
             "|step[int]:Single step in DAC counts"
             "|type[text]:Ratescan type");

        AddEvent("START_N_OUT_OF_4_SCAN", "I:3", RateScan::State::kConnected)
            (bind(&StateMachineRateScan::StartRateScan, this, placeholders::_1, "SET_N_OUT_OF_4"))
            ("Start rate scan for N-out-of-4 in the defined range"
             "|min[int]:Start value in DAC counts"
             "|max[int]:Limiting value in DAC counts"
             "|step[int]:Single step in DAC counts");

        AddEvent("CHANGE_STEP_SIZE", "I:1", RateScan::State::kPaused, RateScan::State::kInProgress)
            (bind(&StateMachineRateScan::ChangeStepSize, this, placeholders::_1))
            ("Change the step size during a ratescan in progress"
             "|step[int]:Single step in DAC counts");

        AddEvent("CHANGE_MAXIMUM", "I:1", RateScan::State::kPaused, RateScan::State::kInProgress)
            (bind(&StateMachineRateScan::ChangeMaximum, this, placeholders::_1))
            ("Change the maximum limit during a ratescan in progress"
             "|max[int]:Limiting value in DAC counts");

        AddEvent("STOP", RateScan::State::kConfiguring, RateScan::State::kPaused, RateScan::State::kInProgress)
            (bind(&StateMachineRateScan::StopRateScan, this))
            ("Stop a ratescan in progress");

        AddEvent("SET_REFERENCE_CAMERA", RateScan::State::kDimNetworkNA, RateScan::State::kDisconnected, RateScan::State::kConnected)
            (bind(&StateMachineRateScan::SetReferenceCamera, this))
            ("Use the camera trigger rate as reference for the reolution");
        AddEvent("SET_REFERENCE_BOARD", "I:1", RateScan::State::kDimNetworkNA, RateScan::State::kDisconnected, RateScan::State::kConnected)
            (bind(&StateMachineRateScan::SetReferenceBoard, this, placeholders::_1))
            ("Use the given board trigger-rate as reference for the reolution"
             "|board[idx]:Index of the board (4*crate+board)");
        AddEvent("SET_REFERENCE_PATCH", "I:1", RateScan::State::kDimNetworkNA, RateScan::State::kDisconnected, RateScan::State::kConnected)
            (bind(&StateMachineRateScan::SetReferencePatch, this, placeholders::_1))
            ("Use the given patch trigger-rate as reference for the reolution"
             "|patch[idx]:Index of the patch (360*crate+36*board+patch)");

        AddEvent("TRIGGER_AUTO_PAUSE", RateScan::State::kDimNetworkNA, RateScan::State::kDisconnected, RateScan::State::kConnected)
            (bind(&StateMachineRateScan::TriggerAutoPause, this))
            ("Enable an automatic pause for the next ratescan, after it got configured.");

        AddEvent("PAUSE", RateScan::State::kInProgress)
            (bind(&StateMachineRateScan::Pause, this))
            ("Pause a ratescan in progress");
        AddEvent("RESUME", RateScan::State::kPaused)
            (bind(&StateMachineRateScan::Resume, this))
            ("Resume a paused ratescan");

        AddEvent("PRINT")
            (bind(&StateMachineRateScan::Print, this))
            ("");
    }

    int EvalOptions(Configuration &conf)
    {
        // ---------- Setup run types ---------
        const vector<string> types = conf.Vec<string>("type");
        if (types.empty())
            Warn("No types defined.");
        else
            Message("Defining types");

        for (auto it=types.begin(); it!=types.end(); it++)
        {
            Message(" -> "+ *it);

            if (fTypes.count(*it)>0)
            {
                Error("Type "+*it+" defined twice.");
                return 1;
            }

            config &c = fTypes[*it];
            if (conf.HasDef("max-wait.", *it))
                c.fCounterMax = conf.GetDef<int>("max-wait.", *it);
            else
            {
                Error("Neither max-wait.default nor max-wait."+*it+" found.");
                return 2;
            }
            if (conf.HasDef("resolution.", *it))
                c.fResolution = conf.GetDef<double>("resolution.", *it);
            else
            {
                Error("Neither resolution.default nor resolution."+*it+" found.");
                return 2;
            }
        }
        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineRateScan>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description type("Ratescan type configuration");
    type.add_options()
        ("type",           vars<string>(),  "Name of ratescan types (replace the * in the following configuration by the case-sensitive names defined here)")
        ("max-wait.*",   var<int>(), "The maximum number of seconds to wait to get the anticipated resolution for a point.")
        ("resolution.*", var<double>() , "The minimum resolution required for a single data point.")
    ;

    conf.AddOptions(type);
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
        "The ratescan program is a tool for automation of rate scans.\n"
        "\n"
        "Usage: ratescan [-c type] [OPTIONS]\n"
        "  or:  ratescan [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineRateScan>();

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
