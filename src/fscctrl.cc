#include <functional>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Console.h"
#include "Converter.h"
#include "Interpolator2D.h"

#include "tools.h"

#include "HeadersFSC.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace FSC;

// ------------------------------------------------------------------------

class ConnectionFSC : public Connection
{
    FSC::BinaryOutput_t fMsg;       // A single message

    bool fIsVerbose;
    bool fIsAutoReconnect;

    uint16_t fTempMin;
    uint16_t fTempMax;

    uint16_t fTempExceeded;
    float    fTempLimit;

    size_t fNumConsecutiveErrors;   // Number of consecutive messages with errors
    size_t fNumConsecutiveMessages; // Number of consecutive message which are ok

    boost::asio::deadline_timer fReconnectTimeout;

protected:
    vector<Interpolator2D::vec> fPositionsSensors;
    vector<Interpolator2D::vec> fPositionsBias;

    virtual void UpdateTemp(float, const vector<float> &)
    {
    }

    virtual void UpdateHum(float, const vector<float>&) 
    {
    }

    virtual void UpdateVolt(float, const vector<float>&) 
    {
    }

    virtual void UpdateCur(float, const vector<float>&) 
    {
    }

public:
    //
    // From: http://de.wikipedia.org/wiki/Pt100
    //
    static double GetTempPT1000(double R)
    {
        // This is precise within the range 5degC and 25degC
        // by 3e-3 degC. At 0degC and 30degC it overestimates the
        // temperature by 0.025 degC. At -10degC it is ~0.9degC
        // and at 40degC ~0.05degC.
        const double x = R/1000;
        return -193.804 + 96.0651*x + 134.673*x*x - 36.9091*x*x*x;

        //for a reasonable range:
        // R=970 -> -7.6 degC
        // R=1300 -> 77.6 degC

        //const double R0 = 1000; // 1kOhm
        //const double a = 3.893e-3;
        //return (R/R0 - 1)/a;
    }

private:
    bool CheckChecksum()
    {
        const uint16_t volt_checksum = Tools::Fletcher16(fMsg.adc_values,    kNumVoltageChannels);
        const uint16_t resi_checksum = Tools::Fletcher16(fMsg.ad7719_values, kNumResistanceChannels);

        const bool volt_ok = volt_checksum == fMsg.adc_values_checksum;
        const bool resi_ok = resi_checksum == fMsg.ad7719_values_checksum;

        if (volt_ok && resi_ok)
            return true;

        fNumConsecutiveErrors++;

        ostringstream out;
        out << "Checksum error (V:";
        out << hex << setfill('0');

        if (volt_ok)
            out << "----|----";
        else
        {
            out << setw(4) << volt_checksum;
            out << "|";
            out << setw(4) << fMsg.adc_values_checksum;
        }

        out << ", R:";

        if (resi_ok)
            out << "----|----";
        else
        {
            out << setw(4) << resi_checksum;
            out << "|";
            out << setw(4) << fMsg.ad7719_values_checksum;
        }

        out << ",  " << dec;
        out << "Nok=" << fNumConsecutiveMessages << ", ";
        out << "Nerr=" << fNumConsecutiveErrors << ")";

        Warn(out);

        fNumConsecutiveMessages = 0;

        return false;
    }

    bool ProcessMessage()
    {
        if (fIsVerbose)
           Out() << "Received one_message of FSC::BinaryOutput_t ... will now process it" << endl;

        if (!CheckChecksum())
            return false;

        // That looks a bit odd because it copies the values twice for no reason.
        // This is historical and keeps the following code consistent with the
        // previous code which was reading ascii data from the fsc
        vector<float> volt(kNumVoltageChannels);
        vector<float> resist(kNumResistanceChannels);

        const float time = fMsg.time_sec + fMsg.time_ms/1000.;

        // We want to convert the pure ADC values from the FSC board to mV and kOhm respectively
        // So we do:
        for (unsigned int i=0; i<volt.size(); i++)
            volt[i] = fMsg.adc_values[i]*0.1;

        for (unsigned int i=0; i<resist.size(); i++)
            resist[i] = fMsg.ad7719_values[i] * (6.25 * 1024) / (1 << 25);

        int mapv[] =
        {
            0, 16, 24,  8,
            1, 17, 25,  9,
            2, 18, 26, 10,
            //
            3, 19, 27, 11,
            4, 20, 28, 12,
            5, 21, 29, 13,
            //
            32, 36, 33, 34, 37, 38,
            //
            -1
        };

        int mapc[] =
        {
            40, 56, 64, 48,
            41, 57, 65, 49,
            42, 58, 66, 50,
            //
            43, 59, 67, 51,
            44, 60, 68, 52,
            45, 61, 69, 53,
            //
            72, 76, 73, 74, 77, 78,
            //
            -1
        };


        int maprh[] =
        {
            80, 81, 82, 83, -1
        };

        int offrh[] =
        {
            821, 822, 816, 822,
        };

        int mapt[] =
        {
            // sensor compartment temperatures
            0,  1,  2,  3,  4,  5,  6, 56, 57, 58, 59, 60,
            61, 62, 32, 33, 34, 35, 36, 63, 37, 38, 39, 24,
            25, 26, 27, 28, 29, 30, 31,
            // crate temperatures (0-3, back/front)
            12, 13, 52, 53, 44, 46, 20, 21,
            //crate power supply temperatures (0-3)
            8, 9, 48, 49, 40, 41, 16, 17,
            // aux power supplies (FTM-side top/bot, FSC-side top/bot)
            45, 50, 19, 42,
            // backpanel (FTM-side top/bot, FSC-side top/bot)
            11, 51, 18, 43,
            // switch boxes (top front/back, bottom front/back)
            15, 14, 47, 10,
            //
            -1
        };

        vector<float> voltages;
        vector<float> currents;
        vector<float> humidities;
        vector<float> temperatures;

        for (int *pv=mapv; *pv>=0; pv++)
            voltages.push_back(volt[*pv]*0.001);

        for (int *pc=mapc; *pc>=0; pc++)
            currents.push_back(volt[*pc]*0.005);

        for (int idx=0; idx<4; idx++)
        {
            voltages[idx +8] *= -1;
            voltages[idx+20] *= -1;
            currents[idx +8] *= -1;
            currents[idx+20] *= -1;
        }
        voltages[12] *=  2;
        voltages[13] *=  2;
        voltages[14] *=  2;
        voltages[15] *=  2;

        voltages[24] *=  2;
        voltages[25] *=  2;

        voltages[27] *= -1;
        voltages[29] *= -1;

        currents[27] *= -1;
        currents[29] *= -1;

        int idx=0;
        for (int *ph=maprh; *ph>=0; ph++, idx++)
            humidities.push_back((volt[*ph]-offrh[idx])*0.0313);


        //1019=4.8
        //1005=1.3
        //970=-7.6
        //1300=76
        //DD 2019/09/01: changed from 970 to 980 as sensor 19 had values slighly larger than -7.6
        // Note that these values are not only the temperature sensors in the compartment
        // but also all other tempereture sensors in the electronics!

        const auto min = GetTempPT1000(fTempMin);

        bool tempgt50 = false;
        for (int *pt=mapt; *pt>=0; pt++)
        {
            const bool valid = resist[*pt]>=fTempMin && resist[*pt]<=fTempMax;
            temperatures.push_back(valid ? GetTempPT1000(resist[*pt]) : 0);

            if (temperatures.back()>fTempLimit)
            {
                Warn("Temperature sensor "+to_string(temperatures.size()-1)+" exceeds "+Tools::Form("%.1f",fTempLimit)+" degC!");
                tempgt50 = true;
            }

            if (valid && temperatures.back()<min+1)
                Warn(Tools::Form("Temperature sensor %2d has reading (%d=%.2f degC) closer than 1K to lower limit (%d=%.2f degC)",
                                 temperatures.size()-1, resist[*pt], temperatures.back(), fTempMin, min));
        }

        if (tempgt50)
            fTempExceeded++;
        else
            fTempExceeded = 0;

        if (fTempExceeded==3)
        {
            Error("EMERGENCY: This is the third time in a row that any of the tempereture sensors exceeds "+Tools::Form("%.1f", fTempLimit)+" degC.");
            Dim::SendCommandNB("BIAS_CONTROL/VOLTAGE_OFF");
            Error("Sending 'BIAS_CONTROL/VOLTAGE_OFF'.");

            Dim::SendCommandNB("PWR_CONTROL/CAMERA_POWER", uint8_t(0));
            Error("Sending 'PWR_CONTROL/CAMERA_POWER off'.");

            Dim::SendCommandNB("AGILENT_CONTROL_50V/SET_POWER", uint8_t(0));
            Error("Sending 'AGILENT_CONTROL_50V/SET_POWER off'.");

            Dim::SendCommandNB("AGILENT_CONTROL_80V/SET_POWER", uint8_t(0));
            Error("Sending 'AGILENT_CONTROL_80V/SET_POWER off'.");
        }

        // 0 = 3-(3+0)%4
        // 3 = 3-(3+1)%4
        // 2 = 3-(3+2)%4
        // 1 = 3-(3+3)%4

        /*
         index	unit	offset	scale	crate	for board:
         0	mV	0	1	0	FAD  3.3V
         24	mV	0	1	1	FAD  3.3V
         16	mV	0	1	2	FAD  3.3V
         8	mV	0	1	3	FAD  3.3V

         1	mV	0	1	0	FAD  3.3V
         25	mV	0	1	1	FAD  3.3V
         17	mV	0	1	2	FAD  3.3V
         9	mV	0	1	3	FAD  3.3V

         2	mV	0	-1	0	FAD  -2.0V
         26	mV	0	-1	1	FAD  -2.0V
         18	mV	0	-1	2	FAD  -2.0V
         10	mV	0	-1	3	FAD  -2.0V

         --

         3	mV	0	1	0	FPA  5.0V
         27	mV	0	1	1	FPA  5.0V
         19	mV	0	1	2	FPA  5.0V
         11	mV	0	1	3	FPA  5.0V

         4	mV	0	1	0	FPA  3.3V
         28	mV	0	1	1	FPA  3.3V
         20	mV	0	1	2	FPA  3.3V
         12	mV	0	1	3	FPA  3.3V

         5	mV	0	-1	0	FPA  -3.3V
         29	mV	0	-1	1	FPA  -3.3V
         21	mV	0	-1	2	FPA  -3.3V
         13	mV	0	-1	3	FPA  -3.3V

         --

         32	mV	0	1	bottom	ETH   5V
         36	mV	0	1	top	ETH   5V

         33	mV	0	1	bottom	FTM   3.3V
         34	mV	0	-1	bottom	FTM  -3.3V

         37	mV	0	1	top	FFC   3.3V
         38	mV	0	-1	top	FLP  -3.3V

         -----

         40	mA	0	5	0	FAD
         64	mA	0	5	1	FAD
         56	mA	0	5	2	FAD
         48	mA	0	5	3	FAD

         41	mA	0	5	0	FAD
         65	mA	0	5	1	FAD
         57	mA	0	5	2	FAD
         49	mA	0	5	3	FAD

         42	mA	0	-5	0	FAD
         66	mA	0	-5	1	FAD
         58	mA	0	-5	2	FAD
         50	mA	0	-5	3	FAD

         --

         43	mA	0	5	0	FPA
         67	mA	0	5	1	FPA
         59	mA	0	5	2	FPA
         51	mA	0	5	3	FPA

         44	mA	0	5	0	FPA
         68	mA	0	5	1	FPA
         60	mA	0	5	2	FPA
         52	mA	0	5	3	FPA

         45	mA	0	-5	0	FPA
         69	mA	0	-5	1	FPA
         61	mA	0	-5	2	FPA
         53	mA	0	-5	3	FPA

         ---

         72	mA	0	5	bottom	ETH
         76	mA	0	5	top	ETH

         73	mA	0	5	bottom	FTM
         74	mA	0	-5	bottom	FTM

         77	mA	0	5	top	FFC
         78	mA	0	-5	top	FLP

         ----

         80	% RH	-821	0.0313		FSP000
         81	% RH	-822	0.0313		FSP221
         82	% RH	-816	0.0313		Sector0
         83	% RH	-822	0.0313		Sector2
         */

        // TEMPERATURES
        // 31 x Sensor plate
        //  8 x Crate
        // 12 x PS
        //  4 x Backpanel
        //  4 x Switchbox



        /*
         0	ohms	FSP	000
         1	ohms	FSP	010
         2	ohms	FSP	023
         3	ohms	FSP	043
         4	ohms	FSP	072
         5	ohms	FSP	080
         6	ohms	FSP	092
         56	ohms	FSP	103
         57	ohms	FSP	111
         58	ohms	FSP	121
         59	ohms	FSP	152
         60	ohms	FSP	163
         61	ohms	FSP	171
         62	ohms	FSP	192
         32	ohms	FSP	200
         33	ohms	FSP	210
         34	ohms	FSP	223
         35	ohms	FSP	233
         36	ohms	FSP	243
         63	ohms	FSP	252
         37	ohms	FSP	280
         38	ohms	FSP	283
         39	ohms	FSP	293
         24	ohms	FSP	311
         25	ohms	FSP	321
         26	ohms	FSP	343
         27	ohms	FSP	352
         28	ohms	FSP	363
         29	ohms	FSP	371
         30	ohms	FSP	381
         31	ohms	FSP	392
         8	ohms	Crate0	?
         9	ohms	Crate0	?
         48	ohms	Crate1	?
         49	ohms	Crate1	?
         40	ohms	Crate2	?
         41	ohms	Crate2	?
         16	ohms	Crate3	?
         17	ohms	Crate3	?
         10	ohms	PS	Crate 0
         11	ohms	PS	Crate 0
         50	ohms	PS	Crate 1
         51	ohms	PS	Crate 1
         42	ohms	PS	Crate 2
         43	ohms	PS	Crate 2
         18	ohms	PS	Crate 3
         19	ohms	PS	Crate 3
         12	ohms	PS	Aux0
         52	ohms	PS	Aux0
         20	ohms	PS	Aux1
         44	ohms	PS	Aux1
         13	ohms	Backpanel	?
         21	ohms	Backpanel	?
         45	ohms	Backpanel	?
         53	ohms	Backpanel	?
         14	ohms	Switchbox0	?
         15	ohms	Switchbox0	?
         46	ohms	Switchbox1	?
         47	ohms	Switchbox1	?
         7	ohms	nc	nc
         22	ohms	nc	nc
         23	ohms	nc	nc
         54	ohms	nc	nc
         55	ohms	nc	nc
         */

        if (fIsVerbose)
        {
            for (size_t i=0; i<resist.size(); i++)
                if (resist[i]>=fTempMin && resist[i]<=fTempMax)
                    Out() << setw(2) << i << " - " << setw(4) << (int)resist[i] << ": " << setprecision(1) << fixed << GetTempPT1000(resist[i]) << endl;
                else
                    Out() << setw(2) << i << " - " << setw(4) << (int)resist[i] << ": " << "----" << endl;
        }

        UpdateTemp(time, temperatures);
        UpdateVolt(time, voltages);
        UpdateCur( time, currents);
        UpdateHum( time, humidities);

        fNumConsecutiveErrors = 0;
        fNumConsecutiveMessages++;

        return true;
    }

    void StartRead()
    {
        ba::async_read(*this, ba::buffer(&fMsg, sizeof(FSC::BinaryOutput_t)),
                       boost::bind(&ConnectionFSC::HandleRead, this,
                                   dummy::error, dummy::bytes_transferred));

        AsyncWait(fInTimeout, 35000, &Connection::HandleReadTimeout); // 30s
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                return;

            // 107: Transport endpoint is not connected (bs::error_code(107, bs::system_category))
            // 125: Operation canceled
            if (err && err!=ba::error::eof &&                     // Connection closed by remote host
                err!=ba::error::basic_errors::not_connected &&    // Connection closed by remote host
                err!=ba::error::basic_errors::operation_aborted)  // Connection closed by us
            {
                ostringstream str;
                str << "Reading from " << URL() << ": " << err.message() << " (" << err << ")";// << endl;
                Error(str);
            }
            if(err!=ba::error::basic_errors::operation_aborted){
                fIsAutoReconnect = true;
                fReconnectTimeout.expires_from_now(boost::posix_time::seconds(10));
                fReconnectTimeout.async_wait(boost::bind(&ConnectionFSC::HandleReconnectTimeout,
                                                         this, dummy::error));
                PostClose(true);
            }else{
                PostClose(false);
            }
            return;
        }

        if (!ProcessMessage())
        {
            fIsAutoReconnect = true;
            fReconnectTimeout.expires_from_now(boost::posix_time::seconds(10));
            fReconnectTimeout.async_wait(boost::bind(&ConnectionFSC::HandleReconnectTimeout,
                                                     this, dummy::error));
            PostClose(true);
            return;
        }

        StartRead();
    }

    void ConnectionEstablished()
    {
        fNumConsecutiveErrors   = 0;
        fNumConsecutiveMessages = 0;
        fIsAutoReconnect = false;

        StartRead();
    }

    void HandleReconnectTimeout(const bs::error_code &)
    {
        fIsAutoReconnect = false;
    }

    void HandleReadTimeout(const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
            return;

        if (error)
        {
            ostringstream str;
            str << "Read timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose();
            return;

        }

        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fInTimeout.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        Error("Timeout reading data from "+URL());

        PostClose();
    }

public:
    ConnectionFSC(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(false), fIsAutoReconnect(false), 
        fTempMin(0), fTempMax(65535), fTempExceeded(0), fTempLimit(60),
        fReconnectTimeout(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetPositionsSensors(const vector<Interpolator2D::vec> &vec)
    {
        fPositionsSensors = vec;
    }

    void SetPositionsBias(const vector<Interpolator2D::vec> &vec)
    {
        fPositionsBias = vec;
    }

    void SetTempMin(const uint16_t &min)
    {
        fTempMin = min;
    }

    void SetTempMax(const uint16_t &max)
    {
        fTempMax = max;
    }

    void SetTempLimit(const float &lim)
    {
        fTempLimit = lim;
    }

    uint16_t GetTempMin() const
    {
        return fTempMin;
    }

    uint16_t GetTempMax() const
    {
        return fTempMax;
    }

    bool IsOpen() const
    {
        return IsConnected() || fIsAutoReconnect;
    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimFSC : public ConnectionFSC
{
private:

    vector<double> fLastRms;

    DimDescribedService fDimTemp;
    DimDescribedService fDimTemp2;
    DimDescribedService fDimHum;
    DimDescribedService fDimVolt;
    DimDescribedService fDimCurrent;

    void Update(DimDescribedService &svc, vector<float> data, float time) const
    {
        data.insert(data.begin(), time);
        svc.Update(data);
    }

    void UpdateTemp(float time, const vector<float> &temp)
    {
        Update(fDimTemp, temp, time);

        vector<double> T;
        vector<Interpolator2D::vec> xy;

        T.reserve(31);
        xy.reserve(31);

        double avg = 0;
        double rms = 0;

        // Create a list of all valid sensors
        for (int i=0; i<31; i++)
            if (temp[i]!=0)
            {
                T.emplace_back(temp[i]);
                xy.emplace_back(fPositionsSensors[i]);

                avg += temp[i];
                rms += temp[i]*temp[i];
            }

        if (T.size()==0)
        {
            Warn("No valid temperatures from compartment sensors.");
            return;
        }

        avg /= T.size();
        rms /= T.size();
        rms -= avg*avg;
        rms = rms<0 ? 0 : sqrt(rms);

        // Clean broken reports
        const double cut_val = 0.015;
        const bool reject = rms>4 || (fabs(fLastRms[0]-fLastRms[1])<=cut_val && fabs(rms-fLastRms[0])>cut_val);

        if (reject)
            Warn(Tools::Form("RMS of compartment temperature suspicous: T[0]:%6.2f (n=%2d) T[1]:%6.2f T[2]:%6.2f... rejected.", rms, T.size(), fLastRms[0], fLastRms[1]));

        fLastRms[1] = fLastRms[0];
        fLastRms[0] = rms;

        if (reject)
            return;

        // Create interpolator for the corresponding sensor positions
        Interpolator2D inter(xy);

        // Calculate weights for the output positions
        if (!inter.SetOutputGrid(fPositionsBias))
        {
            Warn("Interpolation for n="+to_string(xy.size())+" grid positions failed... rejected.");
            return;
        }

        // Interpolate the data
        T = inter.Interpolate(T);

        avg = 0;
        rms = 0;
        for (int i=0; i<320; i++)
        {
            avg += T[i];
            rms += T[i]*T[i];
        }

        avg /= 320;
        rms /= 320;
        rms -= avg*avg;
        rms = rms<0 ? 0 : sqrt(rms);

        vector<float> out;
        out.reserve(322);
        out.assign(T.cbegin(), T.cend());
        out.emplace_back(avg);
        out.emplace_back(rms);

        // Update the Dim service with the interpolated positions
        Update(fDimTemp2, out, time);
    }

    void UpdateHum(float time, const vector<float> &hum)
    {
        Update(fDimHum, hum, time);
    }

    void UpdateVolt(float time, const vector<float> &volt)
    {
        Update(fDimVolt, volt, time);
    }

    void UpdateCur(float time, const vector<float> &curr)
    {
        Update(fDimCurrent, curr, time);
    }

public:
    ConnectionDimFSC(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionFSC(ioservice, imp), fLastRms(2),
        fDimTemp   ("FSC_CONTROL/TEMPERATURE", "F:1;F:31;F:8;F:8;F:4;F:4;F:4",
                    "|t[s]:FSC uptime"
                    "|T_sens[deg C]:Sensor compartment temperatures"
                    "|T_crate[deg C]:Temperatures crate 0 (back/front), 1 (b/f), 2 (b/f), 3 (b/f)"
                    "|T_ps[deg C]:Temp power supplies crate 0 (back/front), 1, 2, 3"
                    "|T_aux[deg C]:Auxiliary power supply temperatures FTM (top/bottom), FSC (t/b)"
                    "|T_back[deg C]:FTM backpanel temperatures FTM (top/bottom), FSC (top/bottom)"
                    "|T_eth[deg C]:Ethernet switches temperatures top (front/back), bottom (f/b)"),
        fDimTemp2  ("FSC_CONTROL/BIAS_TEMP", "F:1;F:320;F:1;F:1",
                    "|t[s]:FSC uptime"
                    "|T[deg C]:Interpolated temperatures at bias patch positions"
                    "|T_avg[deg C]:Average temperature calculated from all patches"
                    "|T_rms[deg C]:Temperature RMS calculated from all patches"),
        fDimHum    ("FSC_CONTROL/HUMIDITY", "F:1;F:4",
                    "|t[s]:FSC uptime"
                    "|H[%]:Humidity sensors readout"),
        fDimVolt   ("FSC_CONTROL/VOLTAGE",
                    "F:1;F:4;F:4;F:4;F:4;F:4;F:4;F:2;F:2;F:1;F:1",
                    "|t[s]:FSC uptime"
                    "|FAD_Ud[V]:FAD digital (crate 0-3)"
                    "|FAD_Up[V]:FAD positive (crate 0-3)"
                    "|FAD_Un[V]:FAD negative (crate 0-3)"
                    "|FPA_Ud[V]:FPA digital (crate 0-3)"
                    "|FPA_Up[V]:FPA positive (crate 0-3)"
                    "|FPA_Un[V]:FPA negative (crate 0-3)"
                    "|ETH_U[V]:Ethernet switch (pos/neg)"
                    "|FTM_U[V]:FTM - trigger master (pos/neg)"
                    "|FFC_U[V]:FFC"
                    "|FLP_U[V]:FLP - light pulser"),
        fDimCurrent("FSC_CONTROL/CURRENT", "F:1;F:4;F:4;F:4;F:4;F:4;F:4;F:2;F:2;F:1;F:1",
                    "|t[s]:FSC uptime"
                    "|FAD_Id[A]:FAD digital (crate 0-3)"
                    "|FAD_Ip[A]:FAD positive (crate 0-3)"
                    "|FAD_In[A]:FAD negative (crate 0-3)"
                    "|FPA_Id[A]:FPA digital (crate 0-3)"
                    "|FPA_Ip[A]:FPA positive (crate 0-3)"
                    "|FPA_In[A]:FPA negative (crate 0-3)"
                    "|ETH_I[A]:Ethernet switch (pos/neg)"
                    "|FTM_I[A]:FTM - trigger master (pos/neg)"
                    "|FFC_I[A]:FFC"
                    "|FLP_I[A]:FLP - light pulser")
    {
        fLastRms[0] = 1.5;
    }

    // A B [C] [D] E [F] G H [I] J K [L] M N O P Q R [S] T U V W [X] Y Z
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineFSC : public StateMachineAsio<T>
{
private:
    S fFSC;

    int Disconnect()
    {
        // Close all connections
        fFSC.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fFSC.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fFSC.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fFSC.PostClose(true);

        return T::GetCurrentState();
    }

    int Execute()
    {
        return fFSC.IsOpen() ? State::kConnected : State::kDisconnected;
    }

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fFSC.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

public:
    StateMachineFSC(ostream &out=cout) :
        StateMachineAsio<T>(out, "FSC_CONTROL"), fFSC(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                     "FSC board not connected via ethernet.");

        T::AddStateName(State::kConnected, "Connected",
                     "Ethernet connection to FSC established.");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineFSC::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected)
            (bind(&StateMachineFSC::Disconnect, this))
            ("disconnect from ethernet");

        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected)
            (bind(&StateMachineFSC::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FSC, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

        fFSC.StartConnect();
    }

    void SetEndpoint(const string &url)
    {
        fFSC.SetEndpoint(url);
    }

    int EvalOptions(Configuration &conf)
    {
        fFSC.SetVerbose(!conf.Get<bool>("quiet"));

        fFSC.SetTempLimit(60);
        T::Info("Overheating temperature limit set to "+to_string(60)+" degC");

        fFSC.SetTempMin(conf.Get<uint16_t>("temp-adc-min"));
        fFSC.SetTempMax(conf.Get<uint16_t>("temp-adc-max"));

        T::Info(Tools::Form("Accepting temperatures between %.2f degC (%d) and %.2f degC (%d)",
                        S::GetTempPT1000(fFSC.GetTempMin()), fFSC.GetTempMin(),
                        S::GetTempPT1000(fFSC.GetTempMax()), fFSC.GetTempMax()));

        const string fname1 = conf.GetPrefixedString("sensor-pos-file");
        const auto v1 = Interpolator2D::ReadGrid(fname1);
        if (v1.size() != 31)
        {
            T::Error("Reading sensor positions from "+fname1+" failed ("+to_string(v1.size())+")");
            return 1;
        }

        const string fname2 = conf.GetPrefixedString("patch-pos-file");
        const auto v2 = Interpolator2D::ReadGrid(fname2);
        if (v2.size() != 320)
        {
            T::Error("Reading bias patch positions from "+fname2+" failed ("+to_string(v2.size())+")");
            return 1;
        }

        fFSC.SetPositionsSensors(v1);
        fFSC.SetPositionsBias(v2);

        SetEndpoint(conf.Get<string>("addr"));

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineFSC<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("FSC control options");
    control.add_options()
        ("no-dim",        po_bool(),  "Disable dim services")
        ("addr,a",        var<string>("localhost:5000"),  "Network address of FSC")
        ("sensor-pos-file", var<string>()->required(),  "File with the positions of the 31 temperature sensors")
        ("patch-pos-file",  var<string>()->required(),  "File with the positions of the 320 bias patches")
        ("quiet,q",       po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("temp-adc-min",  var<uint16_t>(uint16_t(980)),  "Minimum ADC value for allowed range of temperature ADCs (default ~ -7.4 degC)")
        ("temp-adc-max",  var<uint16_t>(uint16_t(1300)), "Maximum ADC value for allowed range of temperature ADCs (default ~ 76 degC)")
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
        "The fscctrl controls the FSC (FACT Slow Control) board.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: fscctrl [-c type] [OPTIONS]\n"
        "  or:  fscctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineFSC<StateMachine, ConnectionFSC>>();

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
            if (conf.Get<bool>("no-dim"))
                return RunShell<LocalStream, StateMachine, ConnectionFSC>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimFSC>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionFSC>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionFSC>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimFSC>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimFSC>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
