#include <valarray>
#include <algorithm>

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

#include "HeadersFSC.h"
#include "HeadersBIAS.h"
#include "HeadersFeedback.h"

#include "DimState.h"
#include "DimDescriptionService.h"

using namespace std;

// ------------------------------------------------------------------------

class StateMachineFeedback : public StateMachineDim
{
private:
    PixelMap fMap;

    bool fIsVerbose;

    DimVersion fDim;

    DimDescribedState fDimFSC;
    DimDescribedState fDimBias;

    DimDescribedService fDimCalibration;
    DimDescribedService fDimCalibration2;
    DimDescribedService fDimCalibrationR8;
    DimDescribedService fDimCurrents;
    DimDescribedService fDimOffsets;

    vector<float>    fCalibCurrentMes[6]; // Measured calibration current at six different levels
    vector<float>    fCalibVoltage[6];    // Corresponding voltage as reported by biasctrl

    vector<int64_t>  fCurrentsAvg;
    vector<int64_t>  fCurrentsRms;

    vector<float>    fVoltGapd;     // Nominal breakdown voltage + 1.1V
    vector<float>    fBiasVolt;     // Output voltage as reported by bias crate (voltage between R10 and R8)
    vector<float>    fBiasR9;       // 
    vector<uint16_t> fBiasDac;      // Dac value corresponding to the voltage setting

    vector<float>    fCalibration;
    vector<float>    fCalibDeltaI;
    vector<float>    fCalibR8;

     int64_t fCursorCur;

    Time fTimeCalib;
    Time fTimeTemp;
    Time fTimeCritical;

    double fUserOffset;
    double fVoltageReduction;
    vector<double> fTempOffset;
    float fTempOffsetAvg;
    float fTempOffsetRms;
    double fTempCoefficient;
    double fTemp;

    vector<double> fVoltOffset;

    uint16_t fMoonMode;

    uint16_t fCurrentRequestInterval;
    uint16_t fNumCalibIgnore;
    uint16_t fNumCalibRequests;
    uint16_t fCalibStep;

    uint16_t fTimeoutCritical;

    // ============================= Handle Services ========================

    int HandleBiasStateChange()
    {
        if (fDimBias.state()==BIAS::State::kVoltageOn && GetCurrentState()==Feedback::State::kCalibrating)
        {
            Dim::SendCommandNB("BIAS_CONTROL/REQUEST_STATUS");
            Info("Starting calibration step "+to_string(fCalibStep));
        }

        if (fDimBias.state()==BIAS::State::kVoltageOff && GetCurrentState()>=Feedback::State::kInProgress)
            return Feedback::State::kCalibrated;

        return GetCurrentState();
    }
    // ============================= Handle Services ========================

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        // Disconnected
        if (has==0)
            return false;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        Fatal(msg);
        return false;
    }

    int HandleBiasNom(const EventImp &evt)
    {
        if (evt.GetSize()>=BIAS::kNumChannels*sizeof(float))
        {
            fVoltGapd.assign(evt.Ptr<float>(), evt.Ptr<float>()+BIAS::kNumChannels);
            fBiasR9.assign(evt.Ptr<float>()+2*BIAS::kNumChannels, evt.Ptr<float>()+3*BIAS::kNumChannels);

            for (int i=0; i<320; i++)
                fVoltGapd[i] += 1.1;

            Info("Nominal bias voltages and calibration resistor received.");
        }

        return GetCurrentState();
    }

    int HandleBiasVoltage(const EventImp &evt)
    {
        if (evt.GetSize()>=BIAS::kNumChannels*sizeof(float))
            fBiasVolt.assign(evt.Ptr<float>(), evt.Ptr<float>()+BIAS::kNumChannels);
        return GetCurrentState();
    }

    int HandleBiasDac(const EventImp &evt)
    {
        if (evt.GetSize()>=BIAS::kNumChannels*sizeof(uint16_t))
            fBiasDac.assign(evt.Ptr<uint16_t>(), evt.Ptr<uint16_t>()+BIAS::kNumChannels);
        return GetCurrentState();
    }

    int HandleCameraTemp(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "HandleCameraTemp", 323*sizeof(float)))
        {
            fTimeTemp = Time(Time::none);
            return GetCurrentState();
        }

        //fTempOffset = (avgt-25)*0.0561765; // [V] From Hamamatsu datasheet
        //fTempOffset = (avgt-25)*0.05678; // [V] From Hamamatsu datasheet plus our own measurement (gein vs. temperature)

        const float *ptr = evt.Ptr<float>(4);

        fTimeTemp = evt.GetTime();
        fTemp     = evt.Get<float>(321*4);

        fTempOffsetAvg = (fTemp-25)*fTempCoefficient;
        fTempOffsetRms =  evt.Get<float>(322*4)*fTempCoefficient;

        fTempOffset.resize(320);
        for (int i=0; i<320; i++)
            fTempOffset[i] = (ptr[i]-25)*fTempCoefficient;

        return GetCurrentState();
    }

    pair<vector<float>, vector<float>> AverageCurrents(const int16_t *ptr, int n)
    {
        if (fCursorCur++>=0)
        {
            for (int i=0; i<BIAS::kNumChannels; i++)
            {
                fCurrentsAvg[i] += ptr[i];
                fCurrentsRms[i] += ptr[i]*ptr[i];
            }
        }

        if (fCursorCur<n)
            return make_pair(vector<float>(), vector<float>());

        const double conv = 5e-3/4096;

        vector<float> rms(BIAS::kNumChannels);
        vector<float> avg(BIAS::kNumChannels);
        for (int i=0; i<BIAS::kNumChannels; i++)
        {
            avg[i]  = double(fCurrentsAvg[i])/fCursorCur * conv;
            rms[i]  = double(fCurrentsRms[i])/fCursorCur * conv * conv;
            rms[i] -= avg[i]*avg[i];
            rms[i]  = rms[i]<0 ? 0 : sqrt(rms[i]);
        }

        return make_pair(avg, rms);
    }

    int HandleCalibration(const EventImp &evt)
    {
        if (fDimBias.state()!=BIAS::State::kVoltageOn)
            return GetCurrentState();

        const uint16_t dac = 256+512*fCalibStep; // Command value

        // Only the channels which are no spare channels are ramped
        // Due to the shortcut, only 319 channels are ramped, so only
        // 320 and not 319 are expected to have the correct day setting
        if (std::count(fBiasDac.begin(), fBiasDac.end(), dac)!=319/*320*/)
            return GetCurrentState();

        const auto rc = AverageCurrents(evt.Ptr<int16_t>(), fNumCalibRequests);
        if (rc.first.size()==0)
        {
            Dim::SendCommandNB("BIAS_CONTROL/REQUEST_STATUS");
            return GetCurrentState();
        }

        const vector<float> &avg = rc.first;
        const vector<float> &rms = rc.second;

        // Current through resistor R8
        fCalibCurrentMes[fCalibStep] = avg;       // [A]
        fCalibVoltage[fCalibStep]    = fBiasVolt; // [V]

        // ------------------------- Update calibration data --------------------

        struct cal_data
        {
            uint32_t dac;
            float    U[BIAS::kNumChannels];
            float    Iavg[BIAS::kNumChannels];
            float    Irms[BIAS::kNumChannels];

            cal_data() { memset(this, 0, sizeof(cal_data)); }
        } __attribute__((__packed__));

        cal_data cal;
        cal.dac = dac;
        memcpy(cal.U,    fBiasVolt.data(), BIAS::kNumChannels*sizeof(float));
        memcpy(cal.Iavg, avg.data(),       BIAS::kNumChannels*sizeof(float));
        memcpy(cal.Irms, rms.data(),       BIAS::kNumChannels*sizeof(float));

        fDimCalibration2.setData(cal);
        fDimCalibration2.Update(fTimeCalib);

        // -------------------- Start next calibration steo ---------------------

        if (++fCalibStep<6)
        {
            fCursorCur  = -fNumCalibIgnore;
            fCurrentsAvg.assign(BIAS::kNumChannels, 0);
            fCurrentsRms.assign(BIAS::kNumChannels, 0);

            // Ramp all channels to the calibration setting except the one
            // with a shortcut
            vector<uint16_t> vec(BIAS::kNumChannels, uint16_t(256+512*fCalibStep));
            vec[272] = 0;
            Dim::SendCommandNB("BIAS_CONTROL/SET_ALL_CHANNELS_DAC", vec);

            //Dim::SendCommandNB("BIAS_CONTROL/SET_GLOBAL_DAC", uint16_t(256+512*fCalibStep));

            return GetCurrentState();
        }

        // --------------- Calculate old style calibration ----------------------

        fCalibration.resize(BIAS::kNumChannels*4);

        float *pavg  = fCalibration.data();
        float *prms  = fCalibration.data()+BIAS::kNumChannels;
        float *pres  = fCalibration.data()+BIAS::kNumChannels*2;
        float *pUmes = fCalibration.data()+BIAS::kNumChannels*3;

        for (int i=0; i<BIAS::kNumChannels; i++)
        {
            const double I = fCalibCurrentMes[5][i]; // [A]
            const double U = fBiasVolt[i];           // [V]

            pavg[i]  = I*1e6;                        // [uA]
            prms[i]  = rms[i]*1e6;                   // [uA]
            pres[i]  = U/I;                          // [Ohm]
            pUmes[i] = U;                            // [V]
        }

        fDimCalibration.setData(fCalibration);
        fDimCalibration.Update(fTimeCalib);

        // -------------------- New style calibration --------------------------

        fCalibDeltaI.resize(BIAS::kNumChannels);
        fCalibR8.resize(BIAS::kNumChannels);

        // Linear regression of the values at 256+512*N for N={ 3, 4, 5 }
        for (int i=0; i<BIAS::kNumChannels; i++)
        {
            // x: Idac
            // y: Iadc

            double x  = 0;
            double y  = 0;
            double xx = 0;
            double xy = 0;

            const int beg = 3;
            const int end = 5;
            const int len = end-beg+1;

            for (int j=beg; j<=end; j++)
            {
                const double Idac = (256+512*j)*1e-3/4096;

                x  += Idac;
                xx += Idac*Idac;
                y  += fCalibCurrentMes[j][i];
                xy += fCalibCurrentMes[j][i]*Idac;
            }

            const double m1 = xy - x*y / len;
            const double m2 = xx - x*x / len;

            const double m = m2==0 ? 0 : m1/m2;

            const double t = (y - m*x) / len;

            fCalibDeltaI[i] = t;     // [A]
            fCalibR8[i]     = 100/m; // [Ohm]
        }

        vector<float> v;
        v.reserve(BIAS::kNumChannels*2);
        v.insert(v.end(), fCalibDeltaI.begin(), fCalibDeltaI.end());
        v.insert(v.end(), fCalibR8.begin(),     fCalibR8.end());

        fDimCalibrationR8.setData(v);
        fDimCalibrationR8.Update(fTimeCalib);

        // ---------------------------------------------------------------------

        Info("Calibration successfully done.");
        Dim::SendCommandNB("BIAS_CONTROL/SET_ZERO_VOLTAGE");

        return Feedback::State::kCalibrated;
    }

    int CheckLimits(const float *I)
    {
        const float fAbsoluteMedianCurrentLimit   = 85;
        const float fRelativePixelCurrentLimit3   = 20;
        const float fRelativePixelCurrentLimit0   = 45;

        const float fAbsolutePixelCurrentLimit3   = fAbsoluteMedianCurrentLimit + fRelativePixelCurrentLimit3;
        const float fAbsolutePixelCurrentLimit0   = fAbsoluteMedianCurrentLimit + fRelativePixelCurrentLimit0;

        const float fRelativeCurrentLimitWarning  = 10;//10;
        const float fRelativeCurrentLimitCritical = 15;//20;
        const float fRelativeCurrentLimitShutdown = 25;

        fTimeoutCritical = 3000; // 5s

        // Copy the calibrated currents
        vector<float> v(I, I+320);

        // Exclude the crazy patches (that's currently the best which could be done)
        v[66]  = 0;
        v[191] = 0;
        v[193] = 0;

        sort(v.begin(), v.end());

        const float &imax0 = v[319];
        const float &imax3 = v[316];
        const float &imed  = v[161];

        const bool shutdown =
            imed >fAbsoluteMedianCurrentLimit+fRelativeCurrentLimitShutdown ||
            imax3>fAbsolutePixelCurrentLimit3+fRelativeCurrentLimitShutdown ||
            imax0>fAbsolutePixelCurrentLimit0+fRelativeCurrentLimitShutdown;

        const bool critical =
            imed >fAbsoluteMedianCurrentLimit+fRelativeCurrentLimitCritical ||
            imax3>fAbsolutePixelCurrentLimit3+fRelativeCurrentLimitCritical ||
            imax0>fAbsolutePixelCurrentLimit0+fRelativeCurrentLimitCritical;

        const bool warning =
            imed >fAbsoluteMedianCurrentLimit+fRelativeCurrentLimitWarning ||
            imax3>fAbsolutePixelCurrentLimit3+fRelativeCurrentLimitWarning ||
            imax0>fAbsolutePixelCurrentLimit0+fRelativeCurrentLimitWarning;

        bool standby = GetCurrentState()==Feedback::State::kOnStandby;

        if (standby)
        {
            // On Standby
            if (fVoltageReduction==0 &&
                imed <fAbsoluteMedianCurrentLimit &&
                imax3<fAbsolutePixelCurrentLimit3 &&
                imax0<fAbsolutePixelCurrentLimit0)
            {
                // Currents are back at nominal value and currents are again
                // below the current limit, switching back to standard operation.
                return Feedback::State::kInProgress;
            }
        }

        // Shutdown level
        if (!standby && shutdown)
        {
            // Currents exceed the shutdown limit, operation is switched
            // immediately to voltage reduced operation

            // Just in case (FIXME: Is that really the right location?)
            Dim::SendCommandNB("FAD_CONTROL/CLOSE_ALL_OPEN_FILES");

            Error("Current limit for shutdown exceeded.... switching to standby mode.");

            standby = true;
        }

        // Critical level
        if (!standby && critical)
        {
            // This is a state transition from InProgress or Warning to Critical.
            // Keep the transition time.
            if (GetCurrentState()==Feedback::State::kInProgress || GetCurrentState()==Feedback::State::kWarning)
            {
                Info("Critical current limit exceeded.... waiting for "+to_string(fTimeoutCritical)+" ms.");
                fTimeCritical = Time();
            }

            // Critical is only allowed for fTimeoutCritical milliseconds.
            // After this time, the operation is changed to reduced voltage.
            if (Time()<fTimeCritical+boost::posix_time::milliseconds(fTimeoutCritical))
                return Feedback::State::kCritical;

            // Just in case (FIXME: Is that really the right location?)
            Dim::SendCommandNB("FAD_CONTROL/CLOSE_ALL_OPEN_FILES");

            // Currents in critical state
            Warn("Critical current limit exceeded timeout.... switching to standby mode.");

            standby = true;
        }

        // Warning level (is just informational)
        if (!standby && warning)
            return Feedback::State::kWarning;

        // keep voltage
        return standby ? Feedback::State::kOnStandby : Feedback::State::kInProgress;
    }

    int HandleBiasCurrent(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "HandleBiasCurrent", BIAS::kNumChannels*sizeof(uint16_t)))
            return Feedback::State::kConnected;

        if (GetCurrentState()<Feedback::State::kCalibrating)
            return GetCurrentState();

        // ------------------------------- HandleCalibration -----------------------------------
        if (GetCurrentState()==Feedback::State::kCalibrating)
            return HandleCalibration(evt);

        // ---------------------- Calibrated, WaitingForData, InProgress -----------------------

        // We are waiting but no valid temperature yet, go on waiting
        if (GetCurrentState()==Feedback::State::kWaitingForData &&
            (!fTimeTemp.IsValid() || Time()-fTimeTemp>boost::posix_time::minutes(5)))
            return GetCurrentState();

        // We are waiting but biasctrl is still in ramping (this might
        // be the case if the feedback was started with a new overvoltage
        // while the last ramping command was still in progress)
        if (GetCurrentState()==Feedback::State::kWaitingForData &&
            fDimBias.state()==BIAS::State::kRamping)
            return GetCurrentState();

        // We are already in progress but no valid temperature update anymore
        if (GetCurrentState()>=Feedback::State::kInProgress &&
            (!fTimeTemp.IsValid() || Time()-fTimeTemp>boost::posix_time::minutes(5)))
        {
            Warn("Current control in progress, but last received temperature older than 5min... switching voltage off.");
            Dim::SendCommandNB("BIAS_CONTROL/SET_ZERO_VOLTAGE");
            return Feedback::State::kCalibrated;
        }

        // ---------------------- Calibrated, WaitingForData, InProgress -----------------------

        const int Navg = fDimBias.state()!=BIAS::State::kVoltageOn ? 1 : 3;

        const vector<float> &Imes = AverageCurrents(evt.Ptr<int16_t>(), Navg).first;
        if (Imes.size()==0)
            return GetCurrentState();

        fCurrentsAvg.assign(BIAS::kNumChannels, 0);
        fCurrentsRms.assign(BIAS::kNumChannels, 0);
        fCursorCur = 0;

        // -------------------------------------------------------------------------------------
        // Inner patches to be blocked (operated below the operation voltage) in moon mode

        static const array<int, 14> inner0 =
        {{
             62,  63, 130, 131, 132, 133, 134,
            135, 222, 223, 292, 293, 294, 295,
        }};

        static const array<int, 23> inner1 =
        {{
             58,  59,  60,  61, 129, 138, 139, 140, 141, 142, 143, 218,
            219, 220, 221, 290, 291, 298, 299, 300, 301, 302, 303,
        }};

        static const array<int, 43> inner2 =
        {{
             42,  43,  44,  45,  55,  56,  57,  70,  71,  78,  79,
             96,  97,  98,  99, 102, 103, 128, 136, 137, 159, 202,
            203, 204, 205, 214, 216, 217, 228, 230, 231, 256, 257,
            258, 259, 262, 263, 288, 289, 296, 297, 310, 318
        }};

        // -------------------------------------------------------------------------------------

        // Nominal overvoltage (w.r.t. the bias setup values)
        const double voltageoffset = GetCurrentState()<Feedback::State::kWaitingForData ? 0 : fUserOffset;

        double avg[2] = {   0,   0 };
        double min[2] = {  90,  90 };
        double max[2] = { -90, -90 };
        int    num[3] = {   0,   0,   0 };

        vector<double> med[3];
        med[0].resize(BIAS::kNumChannels);
        med[1].resize(BIAS::kNumChannels);
        med[2].resize(BIAS::kNumChannels);

        struct dim_data
        {
            float I[BIAS::kNumChannels];
            float Iavg;
            float Irms;
            float Imed;
            float Idev;
            uint32_t N;
            float Tdiff;
            float Uov[BIAS::kNumChannels];
            float Unom;
            float dUtemp;

            dim_data() { memset(this, 0, sizeof(dim_data)); }
        } __attribute__((__packed__));

        int Ndev[3] = { 0, 0, 0 };

        dim_data data;

        data.Unom   = voltageoffset;
        data.dUtemp = fTempOffsetAvg;

        vector<float> vec(BIAS::kNumChannels);

        // ================================= old =======================
        // Pixel  583: 5 31 == 191 (5)  C2 B3 P3
        // Pixel  830: 2  2 ==  66 (4)  C0 B8 P1
        // Pixel 1401: 6  1 == 193 (5)  C2 B4 P0

        double UdrpAvg = 0;
        double UdrpRms = 0;

        for (int i=0; i<320/*BIAS::kNumChannels*/; i++)
        {
            const PixelMapEntry &hv = fMap.hv(i);
            if (!hv)
                continue;

            // Check if this is a blocked channel
            // 272 is the one with the shortcut
            const bool blocked =
                (fMoonMode>0 && std::find(inner0.begin(), inner0.end(), i)!=inner0.end()) ||
                (fMoonMode>1 && std::find(inner1.begin(), inner1.end(), i)!=inner1.end()) ||
                (fMoonMode>2 && std::find(inner2.begin(), inner2.end(), i)!=inner2.end()) ||
                i==272;

            // Number of G-APDs in this patch
            const int N = hv.count();

            // Average measured ADC value for this channel
            // FIXME: This is a workaround for the problem with the
            // readout of bias voltage channel 263
            const double adc = Imes[i]/* * (5e-3/4096)*/; // [A]

            // Current through ~100 Ohm measurement resistor
            //const double I8 = (adc-fCalibDeltaI[i])*fCalibR8[i]/100;
            const double I8 = adc-fCalibDeltaI[i];

            // Current through calibration resistors (R9)
            // This is uncalibrated, but since the corresponding calibrated
            // value I8 is subtracted, the difference should yield a correct value
            const double I9 = fBiasDac[i] * (1e-3/4096);//U9/R9;   [A]

            // Current in R4/R5 branch
            //const double Iout = I8 - I9;//I8>I9 ? I8 - I9 : 0;
            const double Iout = I8 - I9*100/fCalibR8[i];//I8>I9 ? I8 - I9 : 0;

            // Applied voltage at calibration resistors, according to biasctrl
            const double U9 = fBiasVolt[i];

            //          new    I8 - I9*100/fCalibR8       100
            // change = --- = ---------------------- =  --------  = 0.8
            //          old    I8*fCalibR8/100 - I9     fCalibR8

            // Serial resistors (one 1kOhm at the output of the bias crate, one 1kOhm in the camera)
            const double R4 = 2000;

            // Serial resistor of the individual G-APDs plus 50 Ohm termination
            double R5 = 3900./N + 50;

            // This is assuming that the broken pixels have a 390 Ohm instead of 3900 Ohm serial resistor
            if (i==66 || i==193)               // Pixel 830(66) / Pixel 583(191)
                R5 = 1./((N-1)/3900.+1/1000.);
            if (i==191)                        // Pixel 1399(193)
                R5 = 1./((N-1)/3900.+1/390.);
            if (i==17 || i==206)               // dead pixel 923(80) / dead pixel 424(927)
                R5 = 3900./(N-1);              // cannot identify third dead pixel in light-pulser data

            // The measurement resistor
            const double R8 = 0;

            // Total resistance of branch with diodes (R4+R5)
            // Assuming that the voltage output of the OpAMP is linear
            // with the DAC setting and not the voltage at R9, the
            // additional voltage drop at R8 must be taken into account
            const double R = R4 + R5 + R8;

            // For the patches with a broken resistor - ignoring the G-APD resistance -
            // we get:
            //
            // I[R=3900] =  Iout *      1/(10+(N-1))  = Iout        /(N+9)
            // I[R= 390] =  Iout * (1 - 1/(10+(N-1))) = Iout * (N+8)/(N+9)
            //
            // I[R=390] / I[R=3900] = N+8
            //
            // Udrp = Iout*3900/(N+9) + Iout*1000 + Iout*1000 = Iout * R

            // Voltage drop in R4/R5 branch (for the G-APDs with correct resistor)
            // The voltage drop should not be <0, otherwise an unphysical value
            // would be amplified when Uset is calculated.
            const double Udrp = Iout<0 ? 0 : R*Iout;

            // Nominal operation voltage with correction for temperature dependence
            const double Uop = fVoltGapd[i] + fVoltOffset[i] + fTempOffset[i]
                + (blocked ? -5 : 0);

            // Current overvoltage (at a G-APD with the correct 3900 Ohm resistor)
            // expressed w.r.t. to the operation voltage
            const double Uov = (U9-Udrp)-Uop>-1.4 ? (U9-Udrp)-Uop : -1.4;

            // The current through one G-APD is the sum divided by the number of G-APDs
            // (assuming identical serial resistors)
            double Iapd = Iout/N;

            // Rtot = Uapd/Iout
            // Ich  = Uapd/Rch = (Rtot*Iout) / Rch = Rtot/Rch * Iout
            //
            // Rtot = 3900/N
            // Rch  = 3900
            //
            // Rtot = 1./((N-1)/3900 + 1/X)       X=390 or X=1000
            // Rch  = 3900
            //
            // Rtot/Rch =   1/((N-1)/3900 + 1/X)/3900
            // Rtot/Rch =   1/( [ X*(N-1) + 3900 ] / [ 3900 * X ])/3900
            // Rtot/Rch =   X/( [ X*(N-1)/3900 + 1 ] )/3900
            // Rtot/Rch =   X/( [ X*(N-1) + 3900 ] )
            // Rtot/Rch =   1/( [ (N-1) + 3900/X ] )
            //
            // Rtot/Rch[390Ohm]  =  1/( [ N + 9.0 ] )
            // Rtot/Rch[1000Ohm] =  1/( [ N + 2.9 ] )
            //
            // In this and the previosu case we neglect the resistance of the G-APDs, but we can make an
            // assumption: The differential resistance depends more on the NSB than on the PDE,
            // thus it is at least comparable for all G-APDs in the patch. In addition, although the
            // G-APD with the 390Ohm serial resistor has the wrong voltage applied, this does not
            // significantly influences the ohmic resistor or the G-APD because the differential
            // resistor is large enough that the increase of the overvoltage does not dramatically
            // increase the current flow as compared to the total current flow.
            if (i==66 || i==193)           // Iout/13 15.8   / Iout/14  16.8
                Iapd = Iout/(N+2.9);
            if (i==191)                    // Iout/7.9  38.3
                Iapd = Iout/(N+9);
            if (i==17 || i==206)
                Iapd = Iout/(N-1);

            // The differential resistance of the G-APD, i.e. the dependence of the
            // current above the breakdown voltage, is given by
            //const double Rapd = Uov/Iapd;
            // This allows us to estimate the current Iov at the overvoltage we want to apply
            //const double Iov = overvoltage/Rapd;

            // Estimate set point for over-voltage (voltage drop at the target point)
            // This estimation is based on the linear increase of the
            // gain with voltage and the increase of the crosstalk with
            // voltage, as measured with the overvoltage-tests (OVTEST)
            /*
             Uov+0.44<0.022 ?
                Ubd + overvoltage + Udrp*exp(0.6*(overvoltage-Uov))*pow((overvoltage+0.44), 0.6) :
                Ubd + overvoltage + Udrp*exp(0.6*(overvoltage-Uov))*pow((overvoltage+0.44)/(Uov+0.44), 0.6);
             */
            const double Uset =
                Uov+1.4<0.022 ?
                Uop + voltageoffset + Udrp*exp(0.6*(voltageoffset-Uov))*pow((voltageoffset+1.4),           0.6) :
                Uop + voltageoffset + Udrp*exp(0.6*(voltageoffset-Uov))*pow((voltageoffset+1.4)/(Uov+1.4), 0.6);

            if (fabs(voltageoffset-Uov)>0.033)
                Ndev[0]++;
            if (fabs(voltageoffset-Uov)>0.022)
                Ndev[1]++;
            if (fabs(voltageoffset-Uov)>0.011)
                Ndev[2]++;

            // Voltage set point
            vec[i] = Uset;

            const double iapd = Iapd*1e6; // A --> uA

            data.I[i]   = iapd;
            data.Uov[i] = Uov;

            if (!blocked)
            {
                const int g = hv.group();

                med[g][num[g]] = Uov;
                avg[g] += Uov;
                num[g]++;

                if (Uov<min[g])
                    min[g] = Uov;
                if (Uov>max[g])
                    max[g] = Uov;

                data.Iavg += iapd;
                data.Irms += iapd*iapd;

                med[2][num[2]++] = iapd;

                UdrpAvg += Udrp;
                UdrpRms += Udrp*Udrp;
            }
        }


        // ---------------------------- Calculate statistics ----------------------------------

        // average and rms
        data.Iavg /= num[2];
        data.Irms /= num[2];
        data.Irms -= data.Iavg*data.Iavg;

        data.N = num[2];
        data.Irms = data.Irms<0 ? 0: sqrt(data.Irms);

        // median
        sort(med[2].data(), med[2].data()+num[2]);

        data.Imed = num[2]%2 ? med[2][num[2]/2] : (med[2][num[2]/2-1]+med[2][num[2]/2])/2;

        // deviation
        for (int i=0; i<num[2]; i++)
            med[2][i] = fabs(med[2][i]-data.Imed);

        sort(med[2].data(), med[2].data()+num[2]);

        data.Idev = med[2][uint32_t(0.682689477208650697*num[2])];

        // time difference to calibration
        data.Tdiff = evt.GetTime().UnixTime()-fTimeCalib.UnixTime();

        // Average overvoltage
        const double Uov = (avg[0]+avg[1])/(num[0]+num[1]);

        // ------------------------------- Update voltages ------------------------------------

        int newstate = GetCurrentState();

        if (GetCurrentState()!=Feedback::State::kCalibrated) // WaitingForData, OnStandby, InProgress, kWarning, kCritical
        {
            if (fDimBias.state()!=BIAS::State::kRamping)
            {
                newstate = CheckLimits(data.I);

                // standby and change reduction level of voltage
                if (newstate==Feedback::State::kOnStandby)
                {
                    // Calculate average applied overvoltage and estimate an offset
                    // to reach fAbsoluteMedianCurrentLimit
                    float fAbsoluteMedianCurrentLimit = 85;
                    const double deltaU = (Uov+1.4)*(1-pow(fAbsoluteMedianCurrentLimit/data.Imed, 1./1.7));

                    if (fVoltageReduction+deltaU<0.033)
                        fVoltageReduction = 0;
                    else
                    {
                        fVoltageReduction += deltaU;

                        for (int i=0; i<320; i++)
                            vec[i] -= fVoltageReduction;
                    }
                }

                // FIXME: What if the brightest pixel gets too bright???
                // FIXME: What if fVolatgeReduction > U1.4V?

                // set voltage in 262 -> current in 262/263
                vec[263] = vec[262]-fVoltGapd[262]+fVoltGapd[263];

                // Do not ramp the channel with a shortcut
                vec[272] = 0;

//            if (fDimBias.state()!=BIAS::State::kRamping)
//            {
                DimClient::sendCommandNB("BIAS_CONTROL/SET_ALL_CHANNELS_VOLTAGE",
                                         vec.data(), BIAS::kNumChannels*sizeof(float));

                UdrpAvg /= 320;
                UdrpRms /= 320;
                UdrpRms -= UdrpAvg*UdrpAvg;
                UdrpRms  = UdrpRms<0 ? 0 : sqrt(UdrpRms);

                ostringstream msg;
                msg << fixed;
                msg << setprecision(2) << "dU(" << fTemp << "degC)="
                    << setprecision(3) << fTempOffsetAvg << "V+-" << fTempOffsetRms << "  Udrp="
                    << UdrpAvg << "V+-" << UdrpRms;
                msg.unsetf(ios_base::floatfield);

                if (fVoltageReduction==0)
                    msg << " Unom=" << voltageoffset << "V";
                else
                    msg << " Ured=" << fVoltageReduction << "V";

                msg << " Uov=" << Uov;
                msg << " Imed=" << data.Imed << "uA [N=" << Ndev[0] << "/" << Ndev[1] << "/" << Ndev[2] << "]";
                Info(msg);
            }
        }
        else
        {
            if (fDimBias.state()==BIAS::State::kVoltageOn)
            {
                ostringstream msg;
                msg << setprecision(4) << "Current status: dU(" << fTemp << "degC)=" << fTempOffsetAvg << "V+-" << fTempOffsetRms << ", Unom=" << voltageoffset << "V, Uov=" << (num[0]+num[1]>0?(avg[0]+avg[1])/(num[0]+num[1]):0) << " [N=" << Ndev[0] << "/" << Ndev[1] << "/" << Ndev[2] << "]";
                Info(msg);
            }
        }

        //if (GetCurrentState()>=Feedback::State::kOnStandby &&
        //    fDimBias.state()==BIAS::State::kRamping)
        //    return newstate;

        // --------------------------------- Console out --------------------------------------

        if (fIsVerbose && fDimBias.state()!=BIAS::State::kRamping)
        {
            sort(med[0].begin(), med[0].begin()+num[0]);
            sort(med[1].begin(), med[1].begin()+num[1]);

            ostringstream msg;
            msg << "   Avg0=" << setw(7) << avg[0]/num[0]    << "  |  Avg1=" << setw(7) << avg[1]/num[1];
            Debug(msg);

            msg.str("");
            msg << "   Med0=" << setw(7) << med[0][num[0]/2] << "  |  Med1=" << setw(7) << med[1][num[1]/2];
            Debug(msg);

            msg.str("");
            msg << "   Min0=" << setw(7) << min[0]           << "  |  Min1=" << setw(7) << min[1];
            Debug(msg);

            msg.str("");
            msg << "   Max0=" << setw(7) << max[0]           << "  |  Max1=" << setw(7) << max[1];
            Debug(msg);
        }

        // ---------------------------- Calibrated Currents -----------------------------------

        // FIXME:
        //  + Current overvoltage
        //  + Temp offset
        //  + User offset
        //  + Command overvoltage
        fDimCurrents.setQuality(GetCurrentState());
        fDimCurrents.setData(&data, sizeof(dim_data));
        fDimCurrents.Update(evt.GetTime());

        // FIXME: To be checked
        return GetCurrentState()==Feedback::State::kCalibrated ? Feedback::State::kCalibrated : newstate;
    }

    // ======================================================================

    int Print() const
    {
        Out() << fDim << endl;
        Out() << fDimFSC << endl;
        Out() << fDimBias << endl;

        return GetCurrentState();
    }

    int PrintCalibration()
    {
        /*
        if (fCalibration.size()==0)
        {
            Out() << "No calibration performed so far." << endl;
            return GetCurrentState();
        }

        const float *avg = fCalibration.data();
        const float *rms = fCalibration.data()+BIAS::kNumChannels;
        const float *res = fCalibration.data()+BIAS::kNumChannels*2;

        Out() << "Average current at " << fCalibrationOffset << "V below G-APD operation voltage:\n";

        for (int k=0; k<13; k++)
            for (int j=0; j<8; j++)
            {
                Out() << setw(2) << k << "|" << setw(2) << j*4 << "|";
                for (int i=0; i<4; i++)
                    Out() << Tools::Form(" %6.1f+-%4.1f", avg[k*32+j*4+i], rms[k*32+j*4+i]);
                Out() << '\n';
            }
        Out() << '\n';

        Out() << "Measured calibration resistor:\n";
        for (int k=0; k<13; k++)
            for (int j=0; j<4; j++)
            {
                Out() << setw(2) << k << "|" << setw(2) << j*8 << "|";
                for (int i=0; i<8; i++)
                    Out() << Tools::Form(" %5.0f", res[k*32+j*8+i]);
                Out() << '\n';
            }

        Out() << flush;
        */
        return GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return kSM_FatalError;

        fIsVerbose = evt.GetBool();

        return GetCurrentState();
    }

    int SetCurrentRequestInterval(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetCurrentRequestInterval", 2))
            return kSM_FatalError;

        fCurrentRequestInterval = evt.GetUShort();

        Info("New current request interval: "+to_string(fCurrentRequestInterval)+"ms");

        return GetCurrentState();
    }

    int SetMoonMode(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetMoonMode", 2))
            return kSM_FatalError;

        fMoonMode = evt.GetUShort();
        if (fMoonMode>3)
            fMoonMode=3;

        Info("New moon mode: "+to_string(fMoonMode));

        return GetCurrentState();
    }

    int Calibrate()
    {
        if (fDimBias.state()!=BIAS::State::kVoltageOff)
        {
            Warn("Calibration can only be started when biasctrl is in state VoltageOff.");
            return GetCurrentState();
        }

        Message("Starting calibration (ignore="+to_string(fNumCalibIgnore)+", N="+to_string(fNumCalibRequests)+")");

        fCursorCur  = -fNumCalibIgnore;
        fCurrentsAvg.assign(BIAS::kNumChannels, 0);
        fCurrentsRms.assign(BIAS::kNumChannels, 0);

        fBiasDac.assign(BIAS::kNumChannels, 0);

        fCalibStep = 3;
        fTimeCalib = Time();

        // Ramp all channels to the calibration setting except the one
        // with a shortcut
        vector<uint16_t> vec(BIAS::kNumChannels, uint16_t(256+512*fCalibStep));
        vec[272] = 0;
        Dim::SendCommandNB("BIAS_CONTROL/SET_ALL_CHANNELS_DAC", vec);

        //Dim::SendCommandNB("BIAS_CONTROL/SET_GLOBAL_DAC", uint16_t(256+512*fCalibStep));

        return Feedback::State::kCalibrating;
    }

    int Start(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Start", 4))
            return kSM_FatalError;

        /*
        if (fDimBias.state()==BIAS::State::kRamping)
        {
            Warn("Feedback can not be started when biasctrl is in state Ramping.");
            return GetCurrentState();
        }*/

        fUserOffset = evt.GetFloat()-1.1;
        fVoltageReduction = 0;

        fCursorCur = 0;

        fCurrentsAvg.assign(BIAS::kNumChannels, 0);
        fCurrentsRms.assign(BIAS::kNumChannels, 0);

        ostringstream out;
        out << "Starting feedback with an offset of " << fUserOffset << "V";
        Message(out);

        if (fMoonMode>0)
            Message("Moon mode "+to_string(fMoonMode)+" turned on.");

        return Feedback::State::kWaitingForData;
    }

    int StopFeedback()
    {
        if (GetCurrentState()==Feedback::State::kCalibrating)
            return Feedback::State::kConnected;

        if (GetCurrentState()>Feedback::State::kCalibrated)
            return Feedback::State::kCalibrated;

        return GetCurrentState();
    }

    bool LoadOffsets(const string &file)
    {
        vector<double> data(BIAS::kNumChannels);

        ifstream fin(file);

        int cnt = 0;
        while (fin && cnt<320)
            fin >> data[cnt++];

        if (cnt!=320)
        {
            Error("Reading offsets from "+file+" failed [N="+to_string(cnt-1)+"]");
            return false;
        }

        fVoltOffset = data;

        fDimOffsets.Update(fVoltOffset);

        Info("New voltage offsets loaded from "+file);
        return true;

    }

    int LoadOffset(const EventImp &evt)
    {
        LoadOffsets(evt.GetText());
        return GetCurrentState();
    }

    int ResetOffset()
    {
        fVoltOffset.assign(BIAS::kNumChannels, 0);

        fDimOffsets.Update(fVoltOffset);

        Info("Voltage offsets resetted.");
        return GetCurrentState();
    }

    int SaveCalibration()
    {
        ofstream fout("feedback-calib.bin");

        double mjd = fTimeCalib.Mjd();
        fout.write((char*)&mjd, sizeof(double));
        fout.write((char*)fCalibDeltaI.data(), BIAS::kNumChannels*sizeof(float));
        fout.write((char*)fCalibR8.data(),     BIAS::kNumChannels*sizeof(float));

        return GetCurrentState();
    }

    int LoadCalibration()
    {
        ifstream fin("feedback-calib.bin");

        double mjd;

        vector<float> di(BIAS::kNumChannels);
        vector<float> r8(BIAS::kNumChannels);

        fin.read((char*)&mjd, sizeof(double));
        fin.read((char*)di.data(), BIAS::kNumChannels*sizeof(float));
        fin.read((char*)r8.data(), BIAS::kNumChannels*sizeof(float));

        if (!fin)
        {
            Warn("Reading of calibration failed.");
            return GetCurrentState();
        }

        fTimeCalib.Mjd(mjd);
        fCalibDeltaI = di;
        fCalibR8 = r8;

        return Feedback::State::kCalibrated;
    }



    int Execute()
    {
        if (!fDim.online())
            return Feedback::State::kDimNetworkNA;

        const bool bias = fDimBias.state() >= BIAS::State::kConnecting;
        const bool fsc  = fDimFSC.state()  >= FSC::State::kConnected;

        // All subsystems are not connected
        if (!bias && !fsc)
            return Feedback::State::kDisconnected;

        // Not all subsystems are yet connected
        if (!bias || !fsc)
            return Feedback::State::kConnecting;

        if (GetCurrentState()<Feedback::State::kCalibrating)
            return Feedback::State::kConnected;

        if (GetCurrentState()==Feedback::State::kConnected)
            return GetCurrentState();
        if (GetCurrentState()==Feedback::State::kCalibrating)
            return GetCurrentState();

        // kCalibrated, kWaitingForData, kInProgress

        if (fDimBias.state()==BIAS::State::kVoltageOn || (fDimBias.state()==BIAS::State::kVoltageOff && GetCurrentState()==Feedback::State::kWaitingForData))
        {
            static Time past;
            if (fCurrentRequestInterval>0 && Time()-past>boost::posix_time::milliseconds(fCurrentRequestInterval))
            {
                Dim::SendCommandNB("BIAS_CONTROL/REQUEST_STATUS");
                past = Time();
            }
        }

        return GetCurrentState();
    }

public:
    StateMachineFeedback(ostream &out=cout) : StateMachineDim(out, "FEEDBACK"),
        fIsVerbose(false), 
        //---
        fDimFSC("FSC_CONTROL"),
        fDimBias("BIAS_CONTROL"),
        //---
        fDimCalibration("FEEDBACK/CALIBRATION", "F:416;F:416;F:416;F:416",
                        "Current offsets"
                        "|Avg[uA]:Average offset at dac=256+5*512"
                        "|Rms[uA]:Rms of Avg"
                        "|R[Ohm]:Measured calibration resistor"
                        "|U[V]:Corresponding voltage reported by biasctrl"),
        fDimCalibration2("FEEDBACK/CALIBRATION_STEPS", "I:1;F:416;F:416;F:416",
                        "Calibration of the R8 resistor"
                        "|DAC[dac]:DAC setting"
                        "|U[V]:Corresponding voltages reported by biasctrl"
                        "|Iavg[uA]:Averaged measured current"
                        "|Irms[uA]:Rms measured current"),
        fDimCalibrationR8("FEEDBACK/CALIBRATION_R8", "F:416;F:416",
                          "Calibration of R8"
                          "|DeltaI[uA]:Average offset"
                          "|R8[Ohm]:Measured effective resistor R8"),
        fDimCurrents("FEEDBACK/CALIBRATED_CURRENTS", "F:416;F:1;F:1;F:1;F:1;I:1;F:1;F:416;F:1;F:1",
                     "Calibrated currents"
                     "|I[uA]:Calibrated currents per pixel"
                     "|I_avg[uA]:Average calibrated current (N channels)"
                     "|I_rms[uA]:Rms of calibrated current (N channels)"
                     "|I_med[uA]:Median calibrated current (N channels)"
                     "|I_dev[uA]:Deviation of calibrated current (N channels)"
                     "|N[uint16]:Number of valid values"
                     "|T_diff[s]:Time difference to calibration"
                     "|U_ov[V]:Calculated overvoltage w.r.t. operation voltage"
                     "|U_nom[V]:Nominal overvoltage w.r.t. operation voltage"
                     "|dU_temp[V]:Correction calculated from temperature"
                    ),
        fDimOffsets("FEEDBACK/OFFSETS", "F:416",
                    "Offsets operation voltages"
                    "|U[V]:Offset per bias channels"),
        fVoltOffset(BIAS::kNumChannels),
        fMoonMode(0),
        fCurrentRequestInterval(0),
        fNumCalibIgnore(30),
        fNumCalibRequests(300)
    {
        fDim.Subscribe(*this);
        fDimFSC.Subscribe(*this);
        fDimBias.Subscribe(*this);

        fDimBias.SetCallback(bind(&StateMachineFeedback::HandleBiasStateChange, this));

        Subscribe("BIAS_CONTROL/CURRENT")
            (bind(&StateMachineFeedback::HandleBiasCurrent, this, placeholders::_1));
        Subscribe("BIAS_CONTROL/VOLTAGE")
            (bind(&StateMachineFeedback::HandleBiasVoltage, this, placeholders::_1));
        Subscribe("BIAS_CONTROL/DAC")
            (bind(&StateMachineFeedback::HandleBiasDac,     this, placeholders::_1));
        Subscribe("BIAS_CONTROL/NOMINAL")
            (bind(&StateMachineFeedback::HandleBiasNom,     this, placeholders::_1));
        Subscribe("FSC_CONTROL/BIAS_TEMP")
            (bind(&StateMachineFeedback::HandleCameraTemp,  this, placeholders::_1));

        // State names
        AddStateName(Feedback::State::kDimNetworkNA, "DimNetworkNotAvailable",
                     "The Dim DNS is not reachable.");

        AddStateName(Feedback::State::kDisconnected, "Disconnected",
                     "The Dim DNS is reachable, but the required subsystems are not available.");
        AddStateName(Feedback::State::kConnecting, "Connecting",
                     "Either biasctrl or fscctrl not connected.");
        AddStateName(Feedback::State::kConnected, "Connected",
                     "biasctrl and fscctrl are available and connected with their hardware.");

        AddStateName(Feedback::State::kCalibrating, "Calibrating",
                     "Bias crate calibrating in progress.");
        AddStateName(Feedback::State::kCalibrated, "Calibrated",
                     "Bias crate calibrated.");

        AddStateName(Feedback::State::kWaitingForData, "WaitingForData",
                     "Current control started, waiting for valid temperature and current data.");

        AddStateName(Feedback::State::kOnStandby, "OnStandby",
                     "Current control in progress but with limited voltage.");
        AddStateName(Feedback::State::kInProgress, "InProgress",
                     "Current control in progress.");
        AddStateName(Feedback::State::kWarning, "Warning",
                     "Current control in progress but current warning level exceeded.");
        AddStateName(Feedback::State::kCritical, "Critical",
                     "Current control in progress but critical current limit exceeded.");


        /*
        AddEvent("SET_CURRENT_REQUEST_INTERVAL")
            (bind(&StateMachineFeedback::SetCurrentRequestInterval, this, placeholders::_1))
            ("|interval[ms]:Interval between two current requests in modes which need that.");
        */

        AddEvent("CALIBRATE", Feedback::State::kConnected, Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::Calibrate, this))
            ("");

        AddEvent("START", "F:1", Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::Start, this, placeholders::_1))
            ("Start the current/temperature control loop"
             "|Uov[V]:Overvoltage to be applied (standard value is 1.1V)");

        AddEvent("STOP")
            (bind(&StateMachineFeedback::StopFeedback, this))
            ("Stop any control loop");

        AddEvent("LOAD_OFFSETS", "C", Feedback::State::kConnected, Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::LoadOffset, this, placeholders::_1))
            ("");
        AddEvent("RESET_OFFSETS", Feedback::State::kConnected, Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::ResetOffset, this))
            ("");


        AddEvent("SAVE_CALIBRATION", Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::SaveCalibration, this))
            ("");
        AddEvent("LOAD_CALIBRATION", Feedback::State::kConnected)
            (bind(&StateMachineFeedback::LoadCalibration, this))
            ("");

        AddEvent("SET_MOON_MODE", "S:1", Feedback::State::kConnected, Feedback::State::kCalibrated)
            (bind(&StateMachineFeedback::SetMoonMode, this, placeholders::_1))
            ("Operate central pixels at 5V below nominal voltage. 0:off, 1:minimal, 2:medium, 3:maximum size.");


        AddEvent("PRINT")
            (bind(&StateMachineFeedback::Print, this))
            ("");
        AddEvent("PRINT_CALIBRATION")
            (bind(&StateMachineFeedback::PrintCalibration, this))
            ("");

        // Verbosity commands
        AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineFeedback::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity when calculating overvoltage");
    }

    int EvalOptions(Configuration &conf)
    {
        fIsVerbose = !conf.Get<bool>("quiet");

        if (!fMap.Read(conf.GetPrefixedString("pixel-map-file")))
        {
            Error("Reading mapping table from "+conf.Get<string>("pixel-map-file")+" failed.");
            return 1;
        }

        fCurrentRequestInterval = conf.Get<uint16_t>("current-request-interval");
        fNumCalibIgnore         = conf.Get<uint16_t>("num-calib-ignore");
        fNumCalibRequests       = conf.Get<uint16_t>("num-calib-average");
        fTempCoefficient        = conf.Get<double>("temp-coefficient");

        if (conf.Has("offset-file"))
            if (!LoadOffsets(conf.GetPrefixedString("offset-file")))
                return 2;

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineFeedback>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Feedback options");
    control.add_options()
        ("quiet,q", po_bool(true), "Disable printing more information on average overvoltagecontents of all received messages (except dynamic data) in clear text.")
        ("pixel-map-file",      var<string>()->required(), "Pixel mapping file. Used here to get the default reference voltage.")
        ("current-request-interval",  var<uint16_t>(1000), "Interval between two current requests.")
        ("num-calib-ignore",    var<uint16_t>(30), "Number of current requests to be ignored before averaging")
        ("num-calib-average",   var<uint16_t>(300), "Number of current requests to be averaged")
        ("temp-coefficient",    var<double>()->required(), "Temp. coefficient [V/K]")
        ("offset-file",         var<string>(), "File with operation voltage offsets")
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
        "The feedback control the BIAS voltages based on the calibration signal.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: feedback [-c type] [OPTIONS]\n"
        "  or:  feedback [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineFeedback>();

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
