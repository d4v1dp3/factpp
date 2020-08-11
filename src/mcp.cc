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
#include "HeadersFAD.h"
#include "HeadersMCP.h"
#include "HeadersRateControl.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------

class StateMachineMCP : public StateMachineDim
{
private:
    vector<bool> fFadConnected;
    vector<bool> fFadNeedsReset;

    vector<bool> fFadCratesForReset;
    vector<bool> fFadBoardsForConnection;

    uint16_t fNumConnectedFtu;
    uint16_t fNumConnectedFad;

    uint16_t fNumReset;

    DimVersion fDim;
    DimDescribedState fDimFTM;
    DimDescribedState fDimFAD;
    DimDescribedState fDimLog;
    DimDescribedState fDimRC;

    DimDescribedService fService;

    Time fFadTimeout;

    int HandleFadConnections(const EventImp &d)
    {
        if (d.GetSize()!=41)
            return GetCurrentState();

        const uint8_t *ptr = d.Ptr<uint8_t>();

        fNumConnectedFad = 0;
        fFadConnected.assign(40, false);

        vector<bool> reset(4);

        for (int i=0; i<40; i++)
        {
            const uint8_t stat1 = ptr[i]&3;
            const uint8_t stat2 = ptr[i]>>3;

            // disconnected: ignore
            if (stat1==0 && stat2==0)
                continue;

            fFadConnected[i] = true;

            if (stat1>=2 && stat2==8)
                fNumConnectedFad++;

            // Does not need reset
            if (stat1>2 && stat2==8)
                continue;

            // Not configured (stat1==2?kLedGreen:kLedGreenCheck)
            // Connection problem (stat1==1&&stat2==1?kLedRed:kLedOrange)
            reset[i/10] = true;
        }
        return GetCurrentState();
    }

    int HandleFtmStaticData(const EventImp &d)
    {
        if (d.GetSize()!=sizeof(FTM::DimStaticData))
            return GetCurrentState();

        const FTM::DimStaticData &sdata = d.Ref<FTM::DimStaticData>();

        fNumConnectedFtu = 0;
        for (int i=0; i<40; i++)
        {
            if (sdata.IsActive(i))
                fNumConnectedFtu++;
        }
        return GetCurrentState();
    }

    int Print() const
    {
        Out() << fDim    << endl;
        Out() << fDimFTM << endl;
        Out() << fDimFAD << endl;
        Out() << fDimLog << endl;
        Out() << fDimRC  << endl;

        return GetCurrentState();
    }

    int GetReady()
    {
        return GetCurrentState();
    }

    int StopRun()
    {
	if (fDimFTM.state()==FTM::State::kTriggerOn)
	{
            Message("Stopping FTM");
	    Dim::SendCommandNB("FTM_CONTROL/STOP_TRIGGER");
	}

        // FIXME: Do step 2 only when FTM is stopped
        if (fDimFAD.state()==FAD::State::kConnected || fDimFAD.state()==FAD::State::kRunInProgress)
        {
            //Dim::SendCommand("FAD_CONTROL/ENABLE_TRIGGER_LINE",      bool(false));
	    Message("Stopping FAD");
            Dim::SendCommandNB("FAD_CONTROL/ENABLE_CONTINOUS_TRIGGER", bool(false));
            if (fDimFAD.state()==FAD::State::kRunInProgress)
                Dim::SendCommandNB("FAD_CONTROL/CLOSE_OPEN_FILES");
        }

        return GetCurrentState();
    }

    int Reset()
    {
        if (GetCurrentState()<MCP::State::kConfiguring1 ||
            GetCurrentState()>MCP::State::kConfigured)
            return GetCurrentState();

        fRunType = "";
	Message("Reseting configuration states of FAD and FTM");

        Dim::SendCommandNB("FTM_CONTROL/RESET_CONFIGURE");
	Dim::SendCommandNB("FAD_CONTROL/RESET_CONFIGURE");
	Dim::SendCommandNB("RATE_CONTROL/STOP");

        Update(MCP::State::kIdle);
        return MCP::State::kIdle;
        /*
        // FIMXE: Handle error states!
        if (fDimLog.state()>=20)//kSM_NightlyOpen
            Dim::SendCommand("DATA_LOGGER/STOP");

        if (fDimLog.state()==0)
            Dim::SendCommand("DATA_LOGGER/WAIT_FOR_RUN_NUMBER");

        if (fDimFAD.state()==FAD::State::kConnected)
        {
            Dim::SendCommand("FAD_CONTROL/ENABLE_TRIGGER_LINE", bool(false));
            Dim::SendCommand("FAD_CONTROL/ENABLE_CONTINOUS_TRIGGER", bool(false));
        }

        if (fDimFTM.state()==FTM::State::kTakingData)
            Dim::SendCommand("FTM_CONTROL/STOP");

        return GetCurrentState(); */
    }

    int64_t fMaxTime;
    int64_t fNumEvents;
    string  fRunType;

    int StartRun(const EventImp &evt)
    {
        if (!fDimFTM.online())
        {
            Error("No connection to ftmcontrol (see PRINT).");
            return GetCurrentState();
        }
        if (!fDimFAD.online())
        {
            Warn("No connection to fadcontrol (see PRINT).");
            return GetCurrentState();
        }
        if (!fDimLog.online())
        {
            Warn("No connection to datalogger (see PRINT).");
            return GetCurrentState();
        }
        if (!fDimRC.online())
        {
            Warn("No connection to ratecontrol (see PRINT).");
            return GetCurrentState();
        }

        fMaxTime   = evt.Get<int64_t>();
        fNumEvents = evt.Get<int64_t>(8);
        fRunType   = evt.Ptr<char>(16);

        fNumReset  = 0;

        ostringstream str;
        str << "Starting configuration '" << fRunType << "' for new run";
        if (fNumEvents>0 || fMaxTime>0)
            str << " [";
        if (fNumEvents>0)
            str << fNumEvents << " events";
        if (fNumEvents>0 && fMaxTime>0)
            str << " / ";
        if (fMaxTime>0)
            str << fMaxTime << "s";
        if (fNumEvents>0 || fMaxTime>0)
            str << "]";
        Message(str);

        // Strictly speaking, it is not necessary, but
        // stopping the ratecontrol before we configure
        // the FTM ensures that no threshold setting commands
        // interfere with the configuration of the FTM.
        if (fDimRC.state()!=RateControl::State::kConnected)
        {
            Dim::SendCommandNB("RATE_CONTROL/STOP");
            Message("Stopping ratecontrol");
        }

        if (fDimLog.state()<30/*kSM_WaitForRun*/)
        {
            Dim::SendCommandNB("DATA_LOGGER/START_RUN_LOGGING");
            Message("Starting datalogger");
        }

        Update(MCP::State::kConfiguring1);
        return MCP::State::kConfiguring1;
    }

    struct Value
    {
        uint64_t time;
        uint64_t nevts;
        char type[];
    };

    Value *GetBuffer()
    {
        const size_t len = sizeof(Value)+fRunType.length()+1;

        char *buf = new char[len];

        Value *val = reinterpret_cast<Value*>(buf);

        val->time  = fMaxTime;
        val->nevts = fNumEvents;

        strcpy(val->type, fRunType.c_str());

        return val;
    }

    void Update(int newstate)
    {
        Value *buf = GetBuffer();
        fService.setQuality(newstate);
        fService.setData(buf, sizeof(Value)+fRunType.length()+1);
        fService.Update();
        delete buf;
    }

    void ConfigureFAD()
    {
        Value *buf = GetBuffer();

        Dim::SendCommandNB("FAD_CONTROL/CONFIGURE", buf, sizeof(Value)+fRunType.length()+1);
	Message("Configuring FAD");

        delete buf;
    }

    int HandleStateChange()
    {
        if (!fDim.online())
            return MCP::State::kDimNetworkNA;

        if (fDimFTM.state() >= FTM::State::kConnected &&
            fDimFAD.state() >= FAD::State::kConnected &&
            fDimLog.state() >= kSM_Ready)
            return GetCurrentState()<=MCP::State::kIdle ? MCP::State::kIdle : GetCurrentState();

        if (fDimFTM.state() >-2 &&
            fDimFAD.state() >-2 &&
            fDimLog.state() >-2 &&
            fDimRC.state()  >-2)
            return MCP::State::kConnected;

        if (fDimFTM.state() >-2 ||
            fDimFAD.state() >-2 ||
            fDimLog.state() >-2 ||
            fDimRC.state()  >-2)
            return MCP::State::kConnecting;

        return MCP::State::kDisconnected;
    }

    int Execute()
    {
        // ========================================================

        if (GetCurrentState()==MCP::State::kConfiguring1)
        {
            if (fDimRC.state()!=RateControl::State::kConnected)
                return MCP::State::kConfiguring1;

            Dim::SendCommandNB("FTM_CONTROL/CONFIGURE", fRunType);
            Message("Configuring Trigger (FTM)");

            Update(MCP::State::kConfiguring2);
            return MCP::State::kConfiguring2;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kConfiguring2)
        {
            if (fDimFTM.state() != FTM::State::kConfigured1 ||
                fDimLog.state()<30 || fDimLog.state()>0xff ||
                fDimRC.state()!=RateControl::State::kConnected)
                return MCP::State::kConfiguring2;

            // For calibration, ratecontrol will globally set all threshold
            // to make sure that does not interfer with the configuration,
            // it is only done when the ftm reports Configured
            Dim::SendCommandNB("RATE_CONTROL/CALIBRATE_RUN", fRunType);
            Message("Starting Rate Control");

            ConfigureFAD();

            fFadTimeout = Time();

            Update(MCP::State::kConfiguring3);
            return MCP::State::kConfiguring3;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kConfiguring3)
        {
            /*
            // If everything is configured but the FADs
            // we run into a timeout and some FAD need to be reset
            // then we start an automatic crate reset
            if (fDimFTM.state() == FTM::State::kConfigured &&
                fDimFAD.state() != FAD::State::kConfigured &&
                //fDimRC.state()  >  RateControl::State::kSettingGlobalThreshold &&
                fFadTimeout+boost::posix_time::seconds(15)<Time() &&
                count(fFadNeedsReset.begin(), fFadNeedsReset.end(), true)>0)
            {
                Update(MCP::State::kCrateReset0);
                return MCP::State::kCrateReset0;
            }
            */
            // If something is not yet properly configured: keep state
            if (fDimFTM.state() != FTM::State::kConfigured1 ||
                fDimFAD.state() != FAD::State::kConfigured ||
                fDimRC.state()  <= RateControl::State::kSettingGlobalThreshold)
                return MCP::State::kConfiguring3;

            // Note that before the trigger is started, the ratecontrol
            // must not be InProgress. In rare cases there is interference.
            Dim::SendCommandNB("FTM_CONTROL/START_TRIGGER");
            Message("Starting Trigger (FTM)");

            Update(MCP::State::kConfigured);
            return MCP::State::kConfigured;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kConfigured)
        {
            if (fDimFTM.state() != FTM::State::kTriggerOn)
                return MCP::State::kConfigured;

            Update(MCP::State::kTriggerOn);
            return MCP::State::kTriggerOn;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kTriggerOn)
        {
            if (fDimFTM.state() != FTM::State::kTriggerOn)
            {
                Update(MCP::State::kIdle);
                return MCP::State::kIdle;
            }

            if (fDimFAD.state() != FAD::State::kRunInProgress)
                return MCP::State::kTriggerOn;

            Update(MCP::State::kTakingData);
            return MCP::State::kTakingData;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kTakingData)
        {
            if (/*fDimFTM.state()==FTM::State::kTriggerOn &&*/
                fDimFAD.state()==FAD::State::kRunInProgress)
                return MCP::State::kTakingData;

            Update(MCP::State::kIdle);
            return MCP::State::kIdle;
        }

        // ========================================================
        /*
        if (GetCurrentState()==MCP::State::kCrateReset0)
        {
            static const struct Data { int32_t id; char on; } __attribute__((__packed__)) d = { -1, 0 };

            Dim::SendCommandNB("FTM_CONTROL/ENABLE_FTU", &d, sizeof(Data));

            fFadCratesForReset      = fFadNeedsReset;
            fFadBoardsForConnection = fFadConnected;

            for (int c=0; c<4; c++)
                if (fFadNeedsReset[c])
                    for (int b=0; b<10; b++)
                        Dim::SendCommandNB("FAD_CONTROL/DISCONNECT", uint16_t(c*10+b));

            fNumReset++;

            Update(MCP::State::kCrateReset1);
            return MCP::State::kCrateReset1;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kCrateReset1)
        {
            if (fNumConnectedFtu>0 || count(fFadNeedsReset.begin(), fFadNeedsReset.end(), true)>0)
                return MCP::State::kCrateReset1;

            for (int i=0; i<4; i++)
                if (fFadCratesForReset[i])
                    Dim::SendCommandNB("FAD_CONTROL/RESET_CRATE", uint16_t(i));

            fFadTimeout = Time();

            Update(MCP::State::kCrateReset2);
            return MCP::State::kCrateReset2;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kCrateReset2)
        {
            if (fFadTimeout+boost::posix_time::seconds(45)>Time())
                return MCP::State::kCrateReset2;

            static const struct Data { int32_t id; char on; } __attribute__((__packed__)) d = { -1, 1 };

            Dim::SendCommandNB("FTM_CONTROL/ENABLE_FTU", &d, sizeof(Data));

            for (int c=0; c<4; c++)
                if (fFadCratesForReset[c])
                    for (int b=0; b<10; b++)
                        if (fFadBoardsForConnection[c*10+b])
                            Dim::SendCommandNB("FAD_CONTROL/CONNECT", uint16_t(c*10+b));

            Update(MCP::State::kCrateReset3);
            return MCP::State::kCrateReset3;
        }

        // --------------------------------------------------------

        if (GetCurrentState()==MCP::State::kCrateReset3)
        {
            if (fNumConnectedFtu<40 || fFadBoardsForConnection!=fFadConnected)
                return MCP::State::kCrateReset3;

            if (count(fFadNeedsReset.begin(), fFadNeedsReset.end(), true)>0 && fNumReset<6)
            {
                Update(MCP::State::kCrateReset0);
                return MCP::State::kCrateReset0;
            }

            // restart configuration
            Update(MCP::State::kConfiguring1);
            return MCP::State::kConfiguring1;
        }
        */
        // ========================================================

        return GetCurrentState();
    }

public:
    StateMachineMCP(ostream &out=cout) : StateMachineDim(out, "MCP"),
        fFadNeedsReset(4), fNumConnectedFtu(40),
        fDimFTM("FTM_CONTROL"),
        fDimFAD("FAD_CONTROL"),
        fDimLog("DATA_LOGGER"),
        fDimRC("RATE_CONTROL"),
        fService("MCP/CONFIGURATION", "X:1;X:1;C", "Run configuration information"
                 "|MaxTime[s]:Maximum time before the run gets stopped"
                 "|MaxEvents[num]:Maximum number of events before the run gets stopped"
                 "|Name[text]:Name of the chosen configuration")
    {
        // ba::io_service::work is a kind of keep_alive for the loop.
        // It prevents the io_service to go to stopped state, which
        // would prevent any consecutive calls to run()
        // or poll() to do nothing. reset() could also revoke to the
        // previous state but this might introduce some overhead of
        // deletion and creation of threads and more.

        fDim.Subscribe(*this);
        fDimFTM.Subscribe(*this);
        fDimFAD.Subscribe(*this);
        fDimLog.Subscribe(*this);
        fDimRC.Subscribe(*this);

        fDim.SetCallback(bind(&StateMachineMCP::HandleStateChange, this));
        fDimFTM.SetCallback(bind(&StateMachineMCP::HandleStateChange, this));
        fDimFAD.SetCallback(bind(&StateMachineMCP::HandleStateChange, this));
        fDimLog.SetCallback(bind(&StateMachineMCP::HandleStateChange, this));
        fDimRC.SetCallback(bind(&StateMachineMCP::HandleStateChange, this));

        Subscribe("FAD_CONTROL/CONNECTIONS")
            (bind(&StateMachineMCP::HandleFadConnections, this, placeholders::_1));
        Subscribe("FTM_CONTROL/STATIC_DATA")
            (bind(&StateMachineMCP::HandleFtmStaticData, this, placeholders::_1));

        // State names
        AddStateName(MCP::State::kDimNetworkNA, "DimNetworkNotAvailable",
                     "DIM dns server not available.");
        AddStateName(MCP::State::kDisconnected, "Disconnected",
                     "Neither ftmctrl, fadctrl, datalogger nor rate control online.");
        AddStateName(MCP::State::kConnecting, "Connecting",
                     "Either ftmctrl, fadctrl, datalogger or rate control not online.");
        AddStateName(MCP::State::kConnected, "Connected",
                     "All needed subsystems online.");
        AddStateName(MCP::State::kIdle, "Idle",
                     "Waiting for next configuration command");
        AddStateName(MCP::State::kConfiguring1, "Configuring1",
                     "Starting configuration procedure, checking datalogger/ratecontrol state");
        AddStateName(MCP::State::kConfiguring2, "Configuring2",
                     "Starting ratecontrol, waiting for FTM to get configured and Datalogger to get ready");
        AddStateName(MCP::State::kConfiguring3, "Configuring3",
                     "Waiting for FADs and ratecontrol to get ready");
        /*
        AddStateName(MCP::State::kCrateReset0, "CrateReset0",
                     "Disabling FTUs, disconnecting FADs");
        AddStateName(MCP::State::kCrateReset1, "CrateReset1",
                     "Waiting for FTUs to be disabled and for FADs to be disconnected");
        AddStateName(MCP::State::kCrateReset2, "CrateReset2",
                     "Waiting 45s");
        AddStateName(MCP::State::kCrateReset3, "CrateReset3",
                     "Waiting for FTUs to be enabled and for FADs to be re-connected");
        */
        AddStateName(MCP::State::kConfigured, "Configured",
                     "Everything is configured, trigger will be switched on now");
        AddStateName(MCP::State::kTriggerOn, "TriggerOn",
                     "The trigger is switched on, waiting for FAD to receive data");
        AddStateName(MCP::State::kTakingData, "TakingData",
                     "The trigger is switched on, FADs are sending data");


        AddEvent("START", "X:2;C")//, MCP::State::kIdle)
            (bind(&StateMachineMCP::StartRun, this, placeholders::_1))
            ("Start the configuration and data taking for a run-type of a pre-defined setup"
             "|TimeMax[s]:Maximum number of seconds before the run will be closed automatically"
             "|NumMax[count]:Maximum number events before the run will be closed automatically"
             "|Name[text]:Name of the configuration to be used for taking data");

        AddEvent("STOP")
            (bind(&StateMachineMCP::StopRun, this))
            ("Stops the trigger (either disables the FTM trigger or the internal DRS trigger)");

        AddEvent("RESET")
            (bind(&StateMachineMCP::Reset, this))
            ("If a configuration blockes because a system cannot configure itself properly, "
             "this command can be called to leave the configuration procedure. The command "
             "is also propagated to FTM and FAD");

        AddEvent("PRINT")
            (bind(&StateMachineMCP::Print, this))
            ("Print the states and connection status of all systems connected to the MCP.");
    }

    int EvalOptions(Configuration &)
    {
        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineMCP>(conf);
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
        "The Master Control Program (MCP) coordinates the system to take runs.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: mcp [-c type] [OPTIONS]\n"
        "  or:  mcp [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineMCP>();

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
