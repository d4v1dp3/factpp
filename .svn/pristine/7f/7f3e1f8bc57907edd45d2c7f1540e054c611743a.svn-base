#include <valarray>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "Connection.h"
#include "Configuration.h"
#include "Console.h"
#include "PixelMap.h"

#include "tools.h"

#include "LocalControl.h"

#include "HeadersFTM.h"
#include "HeadersLid.h"
#include "HeadersDrive.h"
#include "HeadersRateScan.h"
#include "HeadersRateControl.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------

class StateMachineRateControl : public StateMachineDim//, public DimInfoHandler
{
private:
    struct config
    {
        uint16_t fCalibrationType;
        uint16_t fTargetRate;
        uint16_t fMinThreshold;
        uint16_t fAverageTime;
        uint16_t fRequiredEvents;
    };

    map<string, config> fRunTypes;

    PixelMap fMap;

    bool fPhysTriggerEnabled;
    bool fTriggerOn;

    vector<bool> fBlock;

    DimVersion fDim;
    DimDescribedState fDimFTM;
    DimDescribedState fDimRS;
    DimDescribedState fDimLid;
    DimDescribedState fDimDrive;

    DimDescribedService fDimThreshold;

    float  fTargetRate;
    float  fTriggerRate;

    uint16_t fThresholdMin;
    uint16_t fThresholdReference;

    uint16_t fAverageTime;
    uint16_t fRequiredEvents;

    list<pair<Time,float>> fCurrentsMed;
    list<pair<Time,float>> fCurrentsDev;
    list<pair<Time,vector<float>>> fCurrentsVec;

    bool fVerbose;
    bool fCalibrateByCurrent;

    uint64_t fCounter;

    Time fCalibrationTimeStart;

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

    vector<uint32_t> fThresholds;

    void PrintThresholds(const FTM::DimStaticData &sdata)
    {
        if (!fVerbose)
            return;

        if (fThresholds.empty())
            return;

        if (GetCurrentState()<=RateControl::State::kConnected)
            return;

        Out() << "Min. DAC=" << fThresholdMin << endl;

        for (int j=0; j<10; j++)
        {
            for (int k=0; k<4; k++)
            {
                for (int i=0; i<4; i++)
                {
                    const int p = i + k*4 + j*16;

                    if (fThresholds[p]!=fThresholdMin)
                        Out() << setw(3) << fThresholds[p];
                    else
                        Out() << " - ";

                    if (fThresholds[p]!=sdata.fThreshold[p])
                        Out() << "!";
                    else
                        Out() << " ";
                }

                Out() << "   ";
            }
            Out() << endl;
        }
        Out() << endl;
    }

    // RETURN VALUE
    bool Step(int idx, float step)
    {
        uint32_t diff = fThresholds[idx]+int16_t(truncf(step));
        if (diff<fThresholdMin)
            diff=fThresholdMin;
        if (diff>0xffff)
            diff = 0xffff;

        if (diff==fThresholds[idx])
            return false;

        if (fVerbose)
        {
            Out() << "Apply: Patch " << setw(3) << idx << " [" << idx/40 << "|" << (idx/4)%10 << "|" << idx%4 << "]";
            Out() << (step>0 ? " += " : " -= ");
            Out() << fabs(step) << " (old=" << fThresholds[idx] << ", new=" << diff << ")" << endl;
        }

        fThresholds[idx] = diff;
        fBlock[idx/4] = true;

        return true;
    }

    void ProcessPatches(const FTM::DimTriggerRates &sdata)
    {

        // Caluclate Median and deviation
        vector<float> medb(sdata.fBoardRate, sdata.fBoardRate+40);
        vector<float> medp(sdata.fPatchRate, sdata.fPatchRate+160);

        sort(medb.begin(), medb.end());
        sort(medp.begin(), medp.end());

        vector<float> devb(40);
        for (int i=0; i<40; i++)
            devb[i] = fabs(sdata.fBoardRate[i]-medb[i]);

        vector<float> devp(160);
        for (int i=0; i<160; i++)
            devp[i] = fabs(sdata.fPatchRate[i]-medp[i]);

        sort(devb.begin(), devb.end());
        sort(devp.begin(), devp.end());

        const double mb = (medb[19]+medb[20])/2;
        const double mp = (medp[79]+medp[80])/2;

        const double db = devb[27];
        const double dp = devp[109];

        // If any is zero there is something wrong
        if (mb==0 || mp==0 || db==0 || dp==0)
            return;

        if (fVerbose)
            Out() << Tools::Form("Boards: Med=%3.1f +- %3.1f Hz   Patches: Med=%3.1f +- %3.1f Hz", mb, db, mp, dp) << endl;

        bool changed = false;

        for (int i=0; i<40; i++)
        {
            if (fBlock[i])
            {
                fBlock[i] = false;
                continue;
            }

            int maxi = -1;

            const float dif = fabs(sdata.fBoardRate[i]-mb)/db;
            if (dif>3)
            {
                if (fVerbose)
                    Out() << "Board " << setw(3) << i << ": " << dif << " dev away from med" << endl;

                float max = sdata.fPatchRate[i*4];
                maxi = 0;

                for (int j=1; j<4; j++)
                    if (sdata.fPatchRate[i*4+j]>max)
                    {
                        max = sdata.fPatchRate[i*4+j];
                        maxi = j;
                    }
            }

            for (int j=0; j<4; j++)
            {
                // For the noise pixel correct down to median+3*deviation
                if (maxi==j)
                {
                    // This is the step which has to be performed to go from
                    // a NSB rate of sdata.fPatchRate[i*4+j]


                    const float step = (log10(sdata.fPatchRate[i*4+j])-log10(mp+3.5*dp))/0.039;
                    //  * (dif-5)/dif
                    changed |= Step(i*4+j, step);
                    continue;
                }

                // For pixels below the median correct also back to median+3*deviation
                if (sdata.fPatchRate[i*4+j]<mp)
                {
                    const float step = (log10(sdata.fPatchRate[i*4+j])-log10(mp+3.5*dp))/0.039;
                    changed |= Step(i*4+j, step);
                    continue;
                }

                const float step =  -1.5*(log10(mp+dp)-log10(mp))/0.039;
                changed |= Step(i*4+j, step);
            }
        }

        if (changed)
            Dim::SendCommandNB("FTM_CONTROL/SET_SELECTED_THRESHOLDS", fThresholds);
    }

    int ProcessCamera(const FTM::DimTriggerRates &sdata)
    {
        if (fCounter++==0)
            return GetCurrentState();

        // Caluclate Median and deviation
        vector<float> medb(sdata.fBoardRate, sdata.fBoardRate+40);

        sort(medb.begin(), medb.end());

        vector<float> devb(40);
        for (int i=0; i<40; i++)
            devb[i] = fabs(sdata.fBoardRate[i]-medb[i]);

        sort(devb.begin(), devb.end());

        double mb = (medb[19]+medb[20])/2;
        double db = devb[27];

        // If any is zero there is something wrong
        if (mb==0 || db==0)
        {
            Warn("The median or the deviation of all board rates is zero... cannot calibrate.");
            return GetCurrentState();
        }

        double avg = 0;
        int    num = 0;

        for (int i=0; i<40; i++)
        {
            if ( fabs(sdata.fBoardRate[i]-mb)<2.5*db)
            {
                avg += sdata.fBoardRate[i];
                num++;
            }
        }

        fTriggerRate = avg/num * 40;

        if (fVerbose)
        {
            Out() << "Board:  Median=" << mb << " Dev=" << db << endl;
            Out() << "Camera: " << fTriggerRate << " (" << sdata.fTriggerRate << ", n=" << num << ")" << endl;
            Out() << "Target: " << fTargetRate << endl;
        }

        if (sdata.fTriggerRate<fTriggerRate)
            fTriggerRate = sdata.fTriggerRate;

        // ----------------------

        /*
        if (avg>0 && avg<fTargetRate)
        {
            // I am assuming here (and at other places) the the answer from the FTM when setting
            // the new threshold always arrives faster than the next rate update.
            fThresholdMin = fThresholds[0];
            Out() << "Setting fThresholdMin to " << fThresholds[0] << endl;
        }
        */

        if (fTriggerRate>0 && fTriggerRate<fTargetRate)
        {
            fThresholds.assign(160, fThresholdMin);

            const RateControl::DimThreshold data = { fThresholdMin, fCalibrationTimeStart.Mjd(), Time().Mjd() };
            fDimThreshold.setQuality(0);
            fDimThreshold.Update(data);

            ostringstream out;
            out << setprecision(3);
            out << "Measured rate " << fTriggerRate << "Hz below target rate " << fTargetRate << "... minimum threshold set to " << fThresholdMin;
            Info(out);

            fTriggerOn = false;
            fPhysTriggerEnabled = false;
            return RateControl::State::kGlobalThresholdSet;
        }

        // This is a step towards a threshold at which the NSB rate is equal the target rate
        // +1 to avoid getting a step of 0
        const float step = (log10(fTriggerRate)-log10(fTargetRate))/0.039 + 1;

        const uint16_t diff = fThresholdMin+int16_t(truncf(step));
        if (diff<=fThresholdMin)
        {
            const RateControl::DimThreshold data = { fThresholdMin, fCalibrationTimeStart.Mjd(), Time().Mjd() };
            fDimThreshold.setQuality(1);
            fDimThreshold.Update(data);

            ostringstream out;
            out << setprecision(3);
            out << "Next step would be 0... minimum threshold set to " << fThresholdMin;
            Info(out);

            fTriggerOn = false;
            fPhysTriggerEnabled = false;
            return RateControl::State::kGlobalThresholdSet;
        }

        if (fVerbose)
        {
            //Out() << idx/40 << "|" << (idx/4)%10 << "|" << idx%4;
            Out() << fThresholdMin;
            Out() << (step>0 ? " += " : " -= ");
            Out() << step << " (" << diff << ")" << endl;
        }

        const uint32_t val[2] = { uint32_t(-1),  diff };
        Dim::SendCommandNB("FTM_CONTROL/SET_THRESHOLD", val);

        fThresholdMin = diff;

        return GetCurrentState();
    }

    int HandleStaticData(const EventImp &evt)
    {
        if (!CheckEventSize(evt, sizeof(FTM::DimStaticData)))
            return GetCurrentState();

        const FTM::DimStaticData &sdata = *static_cast<const FTM::DimStaticData*>(evt.GetData());
        fPhysTriggerEnabled = sdata.HasTrigger();
        fTriggerOn = (evt.GetQoS()&FTM::kFtmStates)==FTM::kFtmRunning;

        Out() << "\n" << evt.GetTime() << ": " << (bool)fTriggerOn << " " << (bool)fPhysTriggerEnabled << endl;
        PrintThresholds(sdata);

        if (GetCurrentState()==RateControl::State::kSettingGlobalThreshold && fCalibrateByCurrent)
        {
            if (fThresholds.empty())
                return RateControl::State::kSettingGlobalThreshold;

            if (!std::equal(sdata.fThreshold, sdata.fThreshold+160, fThresholds.begin()))
                return RateControl::State::kSettingGlobalThreshold;

            return RateControl::State::kGlobalThresholdSet;
        }

        fThresholds.assign(sdata.fThreshold, sdata.fThreshold+160);

        return GetCurrentState();
    }

    int HandleTriggerRates(const EventImp &evt)
    {
        fTriggerOn = (evt.GetQoS()&FTM::kFtmStates)==FTM::kFtmRunning;

        if (fThresholds.empty())
            return GetCurrentState();

        if (GetCurrentState()<=RateControl::State::kConnected ||
            GetCurrentState()==RateControl::State::kGlobalThresholdSet)
            return GetCurrentState();

        if (!CheckEventSize(evt, sizeof(FTM::DimTriggerRates)))
            return GetCurrentState();

        const FTM::DimTriggerRates &sdata = *static_cast<const FTM::DimTriggerRates*>(evt.GetData());

        if (GetCurrentState()==RateControl::State::kSettingGlobalThreshold && !fCalibrateByCurrent)
            return ProcessCamera(sdata);

        if (GetCurrentState()==RateControl::State::kInProgress)
            ProcessPatches(sdata);

        return GetCurrentState();
    }

    int HandleCalibratedCurrents(const EventImp &evt)
    {
        // Check if received event is valid
        if (!CheckEventSize(evt, (2*416+8)*4))
            return GetCurrentState();

        // Record only currents when the drive is tracking to avoid
        // bias from the movement
        if (fDimDrive.state()<Drive::State::kTracking || fDimLid.state()==Lid::State::kClosed)
            return GetCurrentState();

        // Get time and median current (FIXME: check N?)
        const Time &time = evt.GetTime();
        const float med  = evt.Get<float>(416*4+4+4);
        const float dev  = evt.Get<float>(416*4+4+4+4);
        const float *cur = evt.Ptr<float>();

        // Keep all median currents of the past 10 seconds
        fCurrentsMed.emplace_back(time, med);
        fCurrentsDev.emplace_back(time, dev);
        fCurrentsVec.emplace_back(time, vector<float>(cur, cur+320));
        while (!fCurrentsMed.empty())
        {
            if (time-fCurrentsMed.front().first<boost::posix_time::seconds(fAverageTime))
                break;

            fCurrentsMed.pop_front();
            fCurrentsDev.pop_front();
            fCurrentsVec.pop_front();
        }

        // If we are not doing a calibration no further action necessary
        if (!fCalibrateByCurrent)
            return GetCurrentState();

        // We are not setting thresholds at all
        if (GetCurrentState()!=RateControl::State::kSettingGlobalThreshold)
            return GetCurrentState();

        // Target thresholds have been assigned already
        if (!fThresholds.empty())
            return GetCurrentState();

        // We want at least 8 values for averaging
        if (fCurrentsMed.size()<fRequiredEvents)
            return GetCurrentState();

        // Calculate avera and rms of median
        double avg = 0;
        double rms = 0;
        for (auto it=fCurrentsMed.begin(); it!=fCurrentsMed.end(); it++)
        {
            avg += it->second;
            rms += it->second*it->second;
        }
        avg /= fCurrentsMed.size();
        rms /= fCurrentsMed.size();
        rms -= avg*avg;
        rms = rms<0 ? 0 : sqrt(rms);

        double avg_dev = 0;
        for (auto it=fCurrentsDev.begin(); it!=fCurrentsDev.end(); it++)
            avg_dev += it->second;
        avg_dev /= fCurrentsMed.size();

        // One could recalculate the median of all pixels including the
        // correction for the three crazy pixels, but that is three out
        // of 320. The effect on the median should be negligible anyhow.
        vector<double> vec(160);
        for (auto it=fCurrentsVec.begin(); it!=fCurrentsVec.end(); it++)
            for (int i=0; i<320; i++)
            {
                const PixelMapEntry &hv = fMap.hv(i);
                if (hv)
                    vec[hv.hw()/9] += it->second[i]*hv.count();
            }

        //fThresholdMin = max(uint16_t(36.0833*pow(avg, 0.638393)+184.037), fThresholdReference);
        //fThresholdMin = max(uint16_t(42.4*pow(avg, 0.642)+182), fThresholdReference);
        //fThresholdMin = max(uint16_t(41.6*pow(avg+1, 0.642)+175), fThresholdReference);
        //fThresholdMin = max(uint16_t(42.3*pow(avg, 0.655)+190), fThresholdReference);
        //fThresholdMin = max(uint16_t(46.6*pow(avg, 0.627)+187), fThresholdReference);
        fThresholdMin = max(uint16_t(156.3*pow(avg, 0.3925)+1), fThresholdReference);
        //fThresholdMin = max(uint16_t(41.6*pow(avg, 0.642)+175), fThresholdReference);
        fThresholds.assign(160, fThresholdMin);

        int counter = 1;

        double avg2 = 0;
        for (int i=0; i<160; i++)
        {
            vec[i] /= fCurrentsVec.size()*9;

            avg2 += vec[i];

            if (vec[i]>avg+3.5*avg_dev)
            {
                fThresholds[i] = max(uint16_t(40.5*pow(vec[i], 0.642)+164), fThresholdMin);

                counter++;
            }
        }
        avg2 /= 160;


        Dim::SendCommandNB("FTM_CONTROL/SET_ALL_THRESHOLDS", fThresholds);


        const RateControl::DimThreshold data = { fThresholdMin, fCalibrationTimeStart.Mjd(), Time().Mjd() };
        fDimThreshold.setQuality(2);
        fDimThreshold.Update(data);

        //Info("Sent a total of "+to_string(counter)+" commands for threshold setting");

        ostringstream out;
        out << setprecision(3);
        out << "Measured average current " << avg << "uA +- " << rms << "uA [N=" << fCurrentsMed.size() << "]... minimum threshold set to " << fThresholdMin;
        Info(out);
        Info("Set "+to_string(counter)+" individual thresholds.");

        fTriggerOn = false;
        fPhysTriggerEnabled = false;

        return RateControl::State::kSettingGlobalThreshold;
    }

    int Calibrate()
    {
        const int32_t val[2] = { -1, fThresholdReference };
        Dim::SendCommandNB("FTM_CONTROL/SET_THRESHOLD", val);

        fThresholds.assign(160, fThresholdReference);

        fThresholdMin = fThresholdReference;
        fTriggerRate  = -1;
        fCounter      = 0;
        fBlock.assign(160, false);

        fCalibrateByCurrent = false;
        fCalibrationTimeStart = Time();

        ostringstream out;
        out << "Rate calibration started at a threshold of " << fThresholdReference << " with a target rate of " << fTargetRate << " Hz";
        Info(out);

        return RateControl::State::kSettingGlobalThreshold;
    }

    int CalibrateByCurrent()
    {
        fCounter = 0;
        fCalibrateByCurrent = true;
        fCalibrationTimeStart = Time();
        fBlock.assign(160, false);

        fThresholds.clear();

        ostringstream out;
        out << "Rate calibration by current with min. threshold of " << fThresholdReference << ".";
        Info(out);

        return RateControl::State::kSettingGlobalThreshold;
    }

    int CalibrateRun(const EventImp &evt)
    {
        const string name = evt.GetText();

        auto it = fRunTypes.find(name);
        if (it==fRunTypes.end())
        {
            Info("CalibrateRun - Run-type '"+name+"' not found... trying 'default'.");

            it = fRunTypes.find("default");
            if (it==fRunTypes.end())
            {
                Error("CalibrateRun - Run-type 'default' not found.");
                return GetCurrentState();
            }
        }

        const config &conf = it->second;

        if (conf.fCalibrationType!=0)
        {

            if (!fPhysTriggerEnabled)
            {
                Info("Calibration requested, but physics trigger not enabled... CALIBRATE command ignored.");

                fTriggerOn = false;
                fPhysTriggerEnabled = false;
                return RateControl::State::kGlobalThresholdSet;
            }

            if (fDimLid.state()==Lid::State::kClosed)
            {
                Info("Calibration requested, but lid closed... setting all thresholds to "+to_string(conf.fMinThreshold)+".");

                const int32_t val[2] = { -1, conf.fMinThreshold };
                Dim::SendCommandNB("FTM_CONTROL/SET_THRESHOLD", val);

                fThresholds.assign(160, conf.fMinThreshold);

                const double mjd = Time().Mjd();

                const RateControl::DimThreshold data = { conf.fMinThreshold, mjd, mjd };
                fDimThreshold.setQuality(3);
                fDimThreshold.Update(data);

                fCalibrateByCurrent = true;
                fTriggerOn = false;
                fPhysTriggerEnabled = false;
                return RateControl::State::kSettingGlobalThreshold;
            }

            if (fDimDrive.state()<Drive::State::kMoving)
                Warn("Calibration requested, but drive not even moving...");
        }

        switch (conf.fCalibrationType)
        {
        case 0:
            Info("No calibration requested.");
            fTriggerOn = false;
            fPhysTriggerEnabled = false;
            return RateControl::State::kGlobalThresholdSet;
            break;

        case 1:
            fThresholdReference = conf.fMinThreshold;
            fTargetRate = conf.fTargetRate;
            return Calibrate();

        case 2:
            fThresholdReference = conf.fMinThreshold;
            fAverageTime = conf.fAverageTime;
            fRequiredEvents = conf.fRequiredEvents;
            return CalibrateByCurrent();

        case 3: // This is a fast mode which keeps the system running if already running
            if (GetCurrentState()==RateControl::State::kInProgress)
            {
                Info("Keeping previous calibration.");
                return GetCurrentState();
            }

            // If not yet running
            fThresholdReference = conf.fMinThreshold;
            fAverageTime = conf.fAverageTime;
            fRequiredEvents = conf.fRequiredEvents;
            return CalibrateByCurrent();
        }

        Error("CalibrateRun - Calibration type "+to_string(conf.fCalibrationType)+" unknown.");
        return GetCurrentState();
    }

    int StopRC()
    {
        Info("Stop received.");
        return RateControl::State::kConnected;
    }

    int SetMinThreshold(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        // FIXME: Check missing

        fThresholdReference = evt.GetUShort();

        return GetCurrentState();
    }

    int SetTargetRate(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 4))
            return kSM_FatalError;

        fTargetRate = evt.GetFloat();

        return GetCurrentState();
    }

    int Print() const
    {
        Out() << fDim << endl;
        Out() << fDimFTM << endl;
        Out() << fDimRS << endl;
        Out() << fDimLid << endl;
        Out() << fDimDrive << endl;

        return GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt, 1))
            return kSM_FatalError;

        fVerbose = evt.GetBool();

        return GetCurrentState();
    }

    int Execute()
    {
        if (!fDim.online())
            return RateControl::State::kDimNetworkNA;

        // All subsystems are not connected
        if (fDimFTM.state()<FTM::State::kConnected || fDimDrive.state()<Drive::State::kConnected)
            return RateControl::State::kDisconnected;

        // Do not allow any action while a ratescan is configured or in progress
        if (fDimRS.state()>=RateScan::State::kConfiguring)
            return RateControl::State::kConnected;

        switch (GetCurrentState())
        {
        case RateControl::State::kSettingGlobalThreshold:
            return RateControl::State::kSettingGlobalThreshold;

        case RateControl::State::kGlobalThresholdSet:

            // Wait for the trigger to get switched on before starting control loop
            if (fTriggerOn && fPhysTriggerEnabled)
                return RateControl::State::kInProgress;

            return RateControl::State::kGlobalThresholdSet;

        case RateControl::State::kInProgress:

            // Go back to connected when the trigger has been switched off
            if (!fTriggerOn || !fPhysTriggerEnabled)
                return RateControl::State::kConnected;

            return RateControl::State::kInProgress;
        }

        return RateControl::State::kConnected;
    }

public:
    StateMachineRateControl(ostream &out=cout) : StateMachineDim(out, "RATE_CONTROL"),
        fPhysTriggerEnabled(false), fTriggerOn(false), fBlock(40),
        fDimFTM("FTM_CONTROL"),
        fDimRS("RATE_SCAN"),
        fDimLid("LID_CONTROL"),
        fDimDrive("DRIVE_CONTROL"),
        fDimThreshold("RATE_CONTROL/THRESHOLD", "S:1;D:1;D:1",
                      "Resulting threshold after calibration"
                      "|threshold[dac]:Resulting threshold from calibration"
                      "|begin[mjd]:Start time of calibration"
                      "|end[mjd]:End time of calibration")
    {
        // ba::io_service::work is a kind of keep_alive for the loop.
        // It prevents the io_service to go to stopped state, which
        // would prevent any consecutive calls to run()
        // or poll() to do nothing. reset() could also revoke to the
        // previous state but this might introduce some overhead of
        // deletion and creation of threads and more.

        fDim.Subscribe(*this);
        fDimFTM.Subscribe(*this);
        fDimRS.Subscribe(*this);
        fDimLid.Subscribe(*this);
        fDimDrive.Subscribe(*this);

        Subscribe("FTM_CONTROL/TRIGGER_RATES")
            (bind(&StateMachineRateControl::HandleTriggerRates, this, placeholders::_1));
        Subscribe("FTM_CONTROL/STATIC_DATA")
            (bind(&StateMachineRateControl::HandleStaticData,   this, placeholders::_1));
        Subscribe("FEEDBACK/CALIBRATED_CURRENTS")
            (bind(&StateMachineRateControl::HandleCalibratedCurrents, this, placeholders::_1));

        // State names
        AddStateName(RateControl::State::kDimNetworkNA, "DimNetworkNotAvailable",
                     "The Dim DNS is not reachable.");

        AddStateName(RateControl::State::kDisconnected, "Disconnected",
                     "The Dim DNS is reachable, but the required subsystems are not available.");

        AddStateName(RateControl::State::kConnected, "Connected",
                     "All needed subsystems are connected to their hardware, no action is performed.");

        AddStateName(RateControl::State::kSettingGlobalThreshold, "Calibrating",
                     "A global minimum threshold is currently determined.");

        AddStateName(RateControl::State::kGlobalThresholdSet, "GlobalThresholdSet",
                     "A global threshold has ben set, waiting for the trigger to be switched on.");

        AddStateName(RateControl::State::kInProgress, "InProgress",
                     "Rate control in progress.");

        AddEvent("CALIBRATE")
            (bind(&StateMachineRateControl::Calibrate, this))
            ("Start a search for a reasonable minimum global threshold");

        AddEvent("CALIBRATE_BY_CURRENT")
            (bind(&StateMachineRateControl::CalibrateByCurrent, this))
            ("Set the global threshold from the median current");

        AddEvent("CALIBRATE_RUN", "C")
            (bind(&StateMachineRateControl::CalibrateRun, this, placeholders::_1))
            ("Start a threshold calibration as defined in the setup for this run-type, state change to InProgress is delayed until trigger enabled");

        AddEvent("STOP", RateControl::State::kSettingGlobalThreshold, RateControl::State::kGlobalThresholdSet, RateControl::State::kInProgress)
            (bind(&StateMachineRateControl::StopRC, this))
            ("Stop a calibration or ratescan in progress");

        AddEvent("SET_MIN_THRESHOLD", "I:1")
            (bind(&StateMachineRateControl::SetMinThreshold, this, placeholders::_1))
            ("Set a minimum threshold at which th rate control starts calibrating");

        AddEvent("SET_TARGET_RATE", "F:1")
            (bind(&StateMachineRateControl::SetTargetRate, this, placeholders::_1))
            ("Set a target trigger rate for the calibration");

        AddEvent("PRINT")
            (bind(&StateMachineRateControl::Print, this))
            ("Print current status");

        AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineRateControl::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

    }

    bool GetConfig(Configuration &conf, const string &name, const string &sub, uint16_t &rc)
    {
        if (conf.HasDef(name, sub))
        {
            rc = conf.GetDef<uint16_t>(name, sub);
            return true;
        }

        Error("Neither "+name+"default nor "+name+sub+" found.");
        return false;
    }

    int EvalOptions(Configuration &conf)
    {
        fVerbose = !conf.Get<bool>("quiet");

        if (!fMap.Read(conf.GetPrefixedString("pixel-map-file")))
        {
            Error("Reading mapping table from "+conf.Get<string>("pixel-map-file")+" failed.");
            return 1;
        }

        fThresholdReference = 300;
        fThresholdMin       = 300;
        fTargetRate         =  75;

        fAverageTime        =  10;
        fRequiredEvents     =   8;

        // ---------- Setup run types ---------
        const vector<string> types = conf.Vec<string>("run-type");
        if (types.empty())
            Warn("No run-types defined.");
        else
            Message("Defining run-types");

        for (auto it=types.begin(); it!=types.end(); it++)
        {
            Message(" -> "+ *it);

            if (fRunTypes.count(*it)>0)
            {
                Error("Run-type "+*it+" defined twice.");
                return 1;
            }

            config &c = fRunTypes[*it];
            if (!GetConfig(conf, "calibration-type.", *it, c.fCalibrationType) ||
                !GetConfig(conf, "target-rate.",      *it, c.fTargetRate)      ||
                !GetConfig(conf, "min-threshold.",    *it, c.fMinThreshold)    ||
                !GetConfig(conf, "average-time.",     *it, c.fAverageTime)     ||
                !GetConfig(conf, "required-events.",  *it, c.fRequiredEvents))
                return 2;
        }

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineRateControl>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Rate control options");
    control.add_options()
        ("quiet,q", po_bool(),  "Disable printing more informations during rate control.")
        ("pixel-map-file", var<string>()->required(), "Pixel mapping file. Used here to get the default reference voltage.")
       //("max-wait",   var<uint16_t>(150), "The maximum number of seconds to wait to get the anticipated resolution for a point.")
       // ("resolution", var<double>(0.05) , "The minimum resolution required for a single data point.")
        ;

    conf.AddOptions(control);

    po::options_description runtype("Run type configuration");
    runtype.add_options()
        ("run-type",           vars<string>(),  "Name of run-types (replace the * in the following configuration by the case-sensitive names defined here)")
        ("calibration-type.*", var<uint16_t>(), "Calibration type (0: none, 1: by rate, 2: by current)")
        ("target-rate.*",      var<uint16_t>(), "Target rate for calibration by rate")
        ("min-threshold.*",    var<uint16_t>(), "Minimum threshold which can be applied in a calibration")
        ("average-time.*",     var<uint16_t>(), "Time in seconds to average the currents for a calibration by current.")
        ("required-events.*",  var<uint16_t>(), "Number of required current events to start a calibration by current.");
    ;

    conf.AddOptions(runtype);
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
        "The ratecontrol program is a keep the rate reasonable low.\n"
        "\n"
        "Usage: ratecontrol [-c type] [OPTIONS]\n"
        "  or:  ratecontrol [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineRateControl>();

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
