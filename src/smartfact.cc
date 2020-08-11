#ifdef HAVE_NOVA
#include "Prediction.h"
#endif

#ifdef HAVE_SQL
#include "Database.h"
#endif

#include <sys/stat.h> //for file stats
#include <sys/statvfs.h> //for file statvfs

#include <numeric> // std::accumulate

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "Connection.h"
#include "Configuration.h"
#include "Console.h"
#include "DimWriteStatistics.h"
#include "PixelMap.h"

#include "tools.h"

#include "LocalControl.h"

#include "HeadersFAD.h"
#include "HeadersBIAS.h"
#include "HeadersFTM.h"
#include "HeadersFSC.h"
#include "HeadersGPS.h"
#include "HeadersSQM.h"
#include "HeadersMCP.h"
#include "HeadersLid.h"
#include "HeadersDrive.h"
#include "HeadersPower.h"
#include "HeadersPFmini.h"
#include "HeadersBiasTemp.h"
#include "HeadersAgilent.h"
#include "HeadersFeedback.h"
#include "HeadersRateScan.h"
#include "HeadersRateControl.h"
#include "HeadersTNGWeather.h"
#include "HeadersGTC.h"
#include "HeadersMagicLidar.h"
#include "HeadersMagicWeather.h"
#include "HeadersTemperature.h"
#include "HeadersRainSensor.h"

#include <boost/filesystem.hpp>

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------
/*
template<class T>
    class buffer : public deque<T>
    {
        int32_t max_size;

    public:
        buffer(int32_t max=-1) : max_size(max) { }
        const T &operator=(const T &t) const { push_back(t); if (max_size>0 && deque<T>::size()>max_size) deque<T>::pop_front(); }
        operator T() const { return deque<T>::size()>0 ? deque<T>::back() : T(); }
        bool valid() const { return deque<T>::size()>0; }
    };
*/

// ------------------------------------------------------------------------

namespace HTML
{
    const static string kWhite  = "#ffffff";
    const static string kYellow = "#fffff0";
    const static string kRed    = "#fff8f0";
    const static string kGreen  = "#f0fff0";
    const static string kBlue   = "#f0f0ff";
};

// ========================================================================
// ========================================================================
// ========================================================================

class Sun
{
public:
    Time time;

    // This is always the time of the next...
    Time fSunRise00;
    Time fSunRise06;
    Time fSunRise12;
    Time fSunRise18;

    Time fSunSet00;
    Time fSunSet06;
    Time fSunSet12;
    Time fSunSet18;

    int state;
    string description;
    string color;

    bool isday;
    bool visible;

#ifdef HAVE_NOVA
    Nova::RstTime Rst(double jd, double hrz=LN_SOLAR_STANDART_HORIZON)
    {
        Nova::RstTime rs = Nova::GetSolarRst(jd-0.5, hrz);
        if (jd>rs.rise || jd>rs.set)
        {
            const Nova::RstTime rs2 = Nova::GetSolarRst(jd+0.5, hrz);
            if (jd>rs.rise)
                rs.rise = rs2.rise;
            if (jd>rs.set)
                rs.set = rs2.set;
        }
        return rs;
    }
#endif

public:
    Sun() : time(Time::none)
    {
    }

    // Could be done more efficient: Only recalcuate if
    // the current time exceeds at least on of the stored times
    Sun(const Time &t) :  time(t)
    {
#ifdef HAVE_NOVA
        // get Julian day from local time
        const double JD = time.JD();

        // >0deg           : day
        //  -6deg -   0deg : civil        twilight
        // -12deg -  -6deg : nautical     twilight
        // -18deg - -12deg : astronomical twilight
        // <-18deg         : night

        const Nova::RstTime sun00 = Rst(JD);
        const Nova::RstTime sun06 = Rst(JD,  -6);
        const Nova::RstTime sun12 = Rst(JD, -12);
        const Nova::RstTime sun18 = Rst(JD, -18);

        fSunRise00 = sun00.rise;
        fSunRise06 = sun06.rise;
        fSunRise12 = sun12.rise;
        fSunRise18 = sun18.rise;

        fSunSet00  = sun00.set;
        fSunSet06  = sun06.set;
        fSunSet12  = sun12.set;
        fSunSet18  = sun18.set;

        array<double,8> arr =
        {{
            sun00.set,
            sun06.set,
            sun12.set,
            sun18.set,
            sun18.rise,
            sun12.rise,
            sun06.rise,
            sun00.rise,
        }};


        state = std::min_element(arr.begin(), arr.end())-arr.begin();

        string name[] =
        {
            "day time",
            "civil twilight",
            "nautical twilight",
            "astron. twilight",
            "dark time",
            "astron. twilight",
            "nautical twilight",
            "civil twilight"
        };

        description = name[state];

        const string txt = fSunRise18<fSunSet18 ?
            time.MinutesTo(fSunRise18)+"&uarr;" :
            time.MinutesTo(fSunSet18)+"&darr;";

        description += " ["+txt+"]";

        isday = state==0;

        switch (state)
        {
        case 0:                  color = HTML::kRed;     break;
        case 1: case 2:          color = HTML::kYellow;  break;
        case 3: case 4:  case 5: color = HTML::kGreen;   break;
        case 6: case 7:          color = HTML::kYellow;  break;
        }

        visible = state==0;

        /*
         // Warning: return code of 1 means circumpolar and is not checked!
        Nova::RstTime sun_day          = Nova::GetSolarRst(JD-0.5);
        Nova::RstTime sun_civil        = Nova::GetSolarRst(JD-0.5,  -6);
        Nova::RstTime sun_astronomical = Nova::GetSolarRst(JD-0.5, -12);
        Nova::RstTime sun_dark         = Nova::GetSolarRst(JD-0.5, -18);

        fSetDayTime       = Time(sun_day.set);
        fSetCivil         = Time(sun_civil.set);
        fSetAstronomical  = Time(sun_astronomical.set);
        fSetDarkTime      = Time(sun_dark.set);

        fRiseDayTime      = Time(sun_day.rise);
        fRiseCivil        = Time(sun_civil.rise);
        fRiseAstronomical = Time(sun_astronomical.rise);
        fRiseDarkTime     = Time(sun_dark.rise);

        const bool is_day   = JD>sun_day.rise;
        const bool is_night = JD>sun_dark.set;

        sun_day          = Nova::GetSolarRst(JD+0.5);
        sun_civil        = Nova::GetSolarRst(JD+0.5,  -6);
        sun_astronomical = Nova::GetSolarRst(JD+0.5, -12);
        sun_dark         = Nova::GetSolarRst(JD+0.5, -18);

        if (is_day)
        {
            fRiseDayTime      = Time(sun_day.rise);
            fRiseCivil        = Time(sun_civil.rise);
            fRiseAstronomical = Time(sun_astronomical.rise);
            fRiseDarkTime     = Time(sun_dark.rise);
        }

        if (is_night)
        {
            fSetDayTime      = Time(sun_day.set);
            fSetCivil        = Time(sun_civil.set);
            fSetAstronomical = Time(sun_astronomical.set);
            fSetDarkTime     = Time(sun_dark.set);
        }

        // case 0: midnight to sun-rise | !is_day && !is_night | rise/set  | -> isday=0
        // case 1: sun-rise to sun-set  |  is_day && !is_night | set /rise | -> isday=1
        // case 2: sun-set  to midnight |  is_day &&  is_night | rise/set  | -> isday=0

        isday = is_day^is_night;

        Time fRiseDayTime;      //   0: Start of day time (=end of civil twilight)
        Time fRiseCivil;        //  -6: End of nautical twilight
        Time fRiseAstronomical; // -12: End of astron. twilight
        Time fRiseDarkTime;     // -18: End of dark time

        Time fSetDayTime;       //   0: End of day time (=start of civil twilight)
        Time fSetCivil;         //  -6: Start of nautical twilight
        Time fSetAstronomical;  // -12: Start of astron. twilight
        Time fSetDarkTime;      // -18: Start of dark time

        state = isday ? 4 : 0;               // 0 [-> Day time       ]
        if (time>fSetDayTime)       state++; // 1 [-> Civil  twilight]
        if (time>fSetCivil)         state++; // 2 [-> Naut.  twilight]
        if (time>fSetAstronomical)  state++; // 3 [-> Astro. twilight]
        if (time>fSetDarkTime)      state++; // 4 [-> Dark time      ]

        if (time>fRiseDarkTime)     state++; // 5 [-> Astro. twilight]
        if (time>fRiseAstronomical) state++; // 6 [-> Naut.  twilight]
        if (time>fRiseCivil)        state++; // 7 [-> Civil  twilight]
        if (time>fRiseDayTime)      state++; // 8 [-> Day time       ]

        string name[] =
        {
            "dark time",          // 0
            "astron. twilight",   // 1
            "civil twilight",     // 2
            "sunrise",            // 3
            "day time",           // 4
            "sunset",             // 5
            "civil twilight",     // 6
            "astron. twilight",   // 7
            "dark time"           // 8
        };

        description = name[state];

        const string arr = isday ?
            fSetDarkTime.MinutesTo(time)+"&darr;" :
            fRiseDarkTime.MinutesTo(time)+"&uarr;";

        description += " ["+arr+"]";

        switch (state)
        {
        case 0: case 1:  color = HTML::kGreen;   break;
        case 2: case 3:  color = HTML::kYellow;  break;
        case 4:          color = HTML::kRed;     break;
        case 5: case 6:  color = HTML::kYellow;  break;
        case 7: case 8:  color = HTML::kGreen;   break;
        }

        visible = state>=3 && state<=5;
        */
#endif
    }
};

class Moon
{
public:
    Time time;

    double ra;
    double dec;

    double zd;
    double az;

    double disk;

    bool visible;

    Time fRise;
    Time fTransit;
    Time fSet;

    string description;
    string color;

    int state;

    Moon() : time(Time::none)
    {
    }

    // Could be done more efficient: Only recalcuate if
    // the current time exceeds at least on of the stored times
    Moon(const Time &t) : time(t)
    {
#ifdef HAVE_NOVA
        const double JD = time.JD();

        Nova::RstTime moon = Nova::GetLunarRst(JD-0.5);

        fRise    = Time(moon.rise);
        fTransit = Time(moon.transit);
        fSet     = Time(moon.set);

        //visible =
        //    ((JD>moon.rise && JD<moon.set ) && moon.rise<moon.set) ||
        //    ((JD<moon.set  || JD>moon.rise) && moon.rise>moon.set);

        const bool is_up      = JD>moon.rise;
        const bool is_sinking = JD>moon.transit;
        const bool is_dn      = JD>moon.set;

        moon = Nova::GetLunarRst(JD+0.5);
        if (is_up)
            fRise = Time(moon.rise);
        if (is_sinking)
            fTransit = Time(moon.transit);
        if (is_dn)
            fSet = Time(moon.set);

        const Nova::EquPosn  pos = Nova::GetLunarEquCoords(JD);
        const Nova::ZdAzPosn hrz = Nova::GetHrzFromEqu(pos, JD);

        az = hrz.az;
        zd = hrz.zd;

        ra  = pos.ra/15;
        dec = pos.dec;

        disk = Nova::GetLunarDisk(JD)*100;
        state = 0;
        if (fRise   <fTransit && fRise   <fSet)     state = 0;  // not visible
        if (fTransit<fSet     && fTransit<fRise)    state = 1;  // before culm
        if (fSet    <fRise    && fSet    <fTransit) state = 2;  // after culm

        visible = state!=0;

        // 0: not visible
        // 1: visible before cul
        // 2: visible after cul

        if (!visible || disk<25)
            color = HTML::kGreen;
        else
            color = disk>75 ? HTML::kRed : HTML::kYellow;

        const string arr = fSet<fRise ?
            fSet.MinutesTo(time) +"&darr;" :
            fRise.MinutesTo(time)+"&uarr;";

        ostringstream out;
        out << setprecision(2);
        out << (visible?"visible ":"") << (disk<0.1?0:disk) << "% [" << arr << "]";

        description = out.str();
#endif
    }

    double Angle(double r, double d) const
    {
        const double theta0 = M_PI/2-d*M_PI/180;
        const double phi0   = r*M_PI/12;

        const double theta1 = M_PI/2-dec*M_PI/180;
        const double phi1   = ra*M_PI/12;

        const double x0 = sin(theta0) * cos(phi0);
        const double y0 = sin(theta0) * sin(phi0);
        const double z0 = cos(theta0);

        const double x1 = sin(theta1) * cos(phi1);
        const double y1 = sin(theta1) * sin(phi1);
        const double z1 = cos(theta1);

        double arg = x0*x1 + y0*y1 + z0*z1;
        if(arg >  1.0) arg =  1.0;
        if(arg < -1.0) arg = -1.0;

        return acos(arg) * 180/M_PI;
    }

    static string Color(double angle)
    {
        if (angle<10 || angle>150)
            return HTML::kRed;
        if (angle<20 || angle>140)
            return HTML::kYellow;
        return HTML::kGreen;
    }
};

// ========================================================================
// ========================================================================
// ========================================================================

class StateMachineSmartFACT : public StateMachineDim
{
public:
    static bool fIsServer;

private:
    enum states_t
    {
        kStateDimNetworkNA = 1,
        kStateRunning,
    };

    // ------------------------- History classes -----------------------

    struct EventElement
    {
        Time time;
        string msg;

        EventElement(const Time &t, const string &s) : time(t), msg(s) { }
    };

    class EventHist : public list<EventElement>
    {
        const boost::posix_time::time_duration deltat; //boost::posix_time::pos_infin
        const uint64_t max;

    public:
        EventHist(const boost::posix_time::time_duration &dt=boost::posix_time::hours(12), uint64_t mx=UINT64_MAX) : deltat(dt), max(mx) { }

        void add(const string &s, const Time &t=Time())
        {
            while (!empty() && (front().time+deltat<t || size()>max))
                pop_front();

            emplace_back(t, s);
        }

        void clean()
        {
            for (auto it=begin(); it!=end();)
                if (!it->time)
                {
                    const auto is = it++;
                    erase(is);
                }
        }

        string get() const
        {
            ostringstream out;

            string last = "";
            for (auto it=begin(); it!=end(); it++)
            {
                const string tm = it->time.GetAsStr("%H:%M:%S ");
                out << (tm!=last?tm:"--:--:-- ") << it->msg << "<br/>";
                last = tm;
            }

            return out.str();
        }
        string rget() const
        {
            ostringstream out;

            for (auto it=rbegin(); it!=rend(); it++)
                out << it->time.GetAsStr("%H:%M:%S ") << it->msg << "<br/>";

            return out.str();
        }
    };

    // ------------------------- Internal variables -----------------------

    const Time fRunTime;

    PixelMap fPixelMap;

    string fDatabase;

    Time fLastUpdate;
    Time fLastAstroCalc;

    string fPath;

    // ----------------------------- Data storage -------------------------

    EventHist fControlMessageHist;
    EventHist fControlAlarmHist;

     int32_t  fMcpConfigurationState;   // For consistency
     int64_t  fMcpConfigurationMaxTime;
     int64_t  fMcpConfigurationMaxEvents;
    string    fMcpConfigurationName;
    Time      fMcpConfigurationRunStart;
    EventHist fMcpConfigurationHist;
    bool fLastRunFinishedWithZeroEvents;

    enum weather_t { kWeatherBegin=0, kTemp = kWeatherBegin, kDew, kHum, kPress, kWind, kGusts, kDir, kWeatherEnd = kDir+1 };
    deque<float> fMagicWeatherHist[kWeatherEnd];

    deque<float> fTngWeatherDustHist;
    Time  fTngWeatherDustTime;

    deque<float> fGtcDustHist;
    Time  fGtcDustTime;

    deque<float> fRainSensorDataHist;
    Time  fRainSensorDataTime;

    vector<float> fBiasControlVoltageVec;

    float  fBiasControlPowerTot;
    float  fBiasControlVoltageMed;
    float  fBiasControlCurrentMed;
    float  fBiasControlCurrentMax;

    deque<float> fBiasControlCurrentHist;
    deque<float> fFscControlTemperatureHist;

    float fFscControlHumidityAvg;

    deque<float> fPfMiniHumidityHist;
    deque<float> fPfMiniTemperatureHist;

    deque<float> fTemperatureControlHist;

    float  fDriveControlPointingZd;
    string fDriveControlPointingAz;
    string fDriveControlSourceName;
    float  fDriveControlMoonDist;

    deque<float> fDriveControlTrackingDevHist;

     int64_t fFadControlNumEvents;
     int64_t fFadControlStartRun;
     int32_t fFadControlDrsStep;
    vector<uint32_t> fFadControlDrsRuns;

    deque<float> fFtmControlTriggerRateHist;
     int32_t     fFtmControlTriggerRateTooLow;
     int         fFtmControlState;

    float fFtmPatchThresholdMed;
    float fFtmBoardThresholdMed;

    bool fFtmControlFtuOk;

    deque<float> fRateControlThreshold;

    uint64_t  fRateScanDataId;
    uint8_t   fRateScanBoard;
    deque<float> fRateScanDataHist[41];

    set<string> fErrorList;
    EventHist   fErrorHist;
    EventHist   fChatHist;

    uint64_t fFreeSpace;

    Sun   fSun;
    Moon  fMoon;

    // --------------------------- File header ----------------------------

    Time   fAudioTime;
    string fAudioName;

    string Header(const Time &d)
    {
        ostringstream msg;
        msg << d.JavaDate() << '\t' << fAudioTime.JavaDate() << '\t' << fAudioName;
        return msg.str();
    }

    string Header(const EventImp &d)
    {
        return Header(d.GetTime());
    }

    void SetAudio(const string &name)
    {
        fAudioName = name;
        fAudioTime = Time();
    }

    // ------------- Initialize variables before the Dim stuff ------------

    DimVersion fDimDNS;
    DimControl fDimControl;
    DimDescribedState fDimMcp;
    DimDescribedState fDimDataLogger;
    DimDescribedState fDimDriveControl;
    DimDescribedState fDimTimeCheck;
    DimDescribedState fDimMagicWeather;
    DimDescribedState fDimMagicLidar;
    DimDescribedState fDimTngWeather;
    DimDescribedState fDimGtcDust;
    DimDescribedState fDimTemperature;
    DimDescribedState fDimRainSensor;
    DimDescribedState fDimFeedback;
    DimDescribedState fDimBiasControl;
    DimDescribedState fDimFtmControl;
    DimDescribedState fDimFadControl;
    DimDescribedState fDimFscControl;
    DimDescribedState fDimPfMiniControl;
    DimDescribedState fDimBiasTemp;
    DimDescribedState fDimGpsControl;
    DimDescribedState fDimSqmControl;
    DimDescribedState fDimAgilentControl24;
    DimDescribedState fDimAgilentControl50;
    DimDescribedState fDimAgilentControl80;
    DimDescribedState fDimPwrControl;
    DimDescribedState fDimLidControl;
    DimDescribedState fDimRateControl;
    DimDescribedState fDimRateScan;
    DimDescribedState fDimChat;
    DimDescribedState fDimSkypeClient;

    // -------------------------------------------------------------------

    string GetDir(const double angle)
    {
        static const char *dir[] =
        {
            "N", "NNE", "NE", "ENE",
            "E", "ESE", "SE", "SSE",
            "S", "SSW", "SW", "WSW",
            "W", "WNW", "NW", "NNW"
        };

        const uint16_t idx = uint16_t(floor(angle/22.5+16.5))%16;
        return dir[idx];
    }

    // -------------------------------------------------------------------

    bool CheckDataSize(const EventImp &d, const char *name, size_t size, bool min=false)
    {
        if (d.GetSize()==0)
            return false;

        if ((!min && d.GetSize()==size) || (min && d.GetSize()>size))
            return true;

        ostringstream msg;
        msg << name << " - Received service has " << d.GetSize() << " bytes, but expected ";
        if (min)
            msg << "more than ";
        msg << size << ".";
        Warn(msg);
        return false;
    }

    // -------------------------------------------------------------------

    template<class T>
        void WriteBinaryVec(const Time &tm, const string &fname, const vector<T> &vec, double scale, double offset=0, const string &title="", const string &col="")
    {
        if (vec.empty())
            return;

        ostringstream out;
        out << tm.JavaDate() << '\n';
        out << offset << '\n';
        out << offset+scale << '\n';
        out << setprecision(3);
        if (!title.empty())
            out << title <<  '\x7f';
        else
        {
            const Statistics stat(vec[0]);
            out << stat.min << '\n';
            out << stat.med << '\n';
            out << stat.max << '\x7f';
        }
        if (!col.empty())
            out << col;
        for (auto it=vec.cbegin(); it!=vec.cend(); it++)
        {
            // The valid range is from 1 to 127
            // \0 is used to seperate different curves
            vector<uint8_t> val(it->size());
            for (uint64_t i=0; i<it->size(); i++)
            {
                float range = nearbyint(126*(double(it->at(i))-offset)/scale); // [-2V; 2V]
                if (range>126)
                    range=126;
                if (range<0)
                    range=0;
                val[i] = (uint8_t)range;
            }

            const char *ptr = reinterpret_cast<char*>(val.data());
            out.write(ptr, val.size()*sizeof(uint8_t));
            out << '\x7f';
        }

        ofstream(fPath+"/"+fname+".bin") << out.str();
    }
    /*
    template<class T>
        void WriteBinaryVec(const EventImp &d, const string &fname, const vector<T> &vec, double scale, double offset=0, const string &title="")
    {
        WriteBinaryVec(d.GetTime(), fname, vec, scale, offset, title);
    }

    template<class T>
        void WriteBinary(const Time &tm, const string &fname, const T &t, double scale, double offset=0)
    {
        WriteBinaryVec(tm, fname, vector<T>(&t, &t+1), scale, offset);
    }

    template<class T>
        void WriteBinary(const EventImp &d, const string &fname, const T &t, double scale, double offset=0)
    {
        WriteBinaryVec(d.GetTime(), fname, vector<T>(&t, &t+1), scale, offset);
    }*/

    template<class T>
        void WriteHist(const EventImp &d, const string &fname, const T &t, double scale, double offset=0)
    {
        WriteBinaryVec(d.GetTime(), fname, vector<T>(&t, &t+1), scale, offset, "", "000");
    }

    template<class T>
        void WriteCam(const EventImp &d, const string &fname, const T &t, double scale, double offset=0)
    {
        WriteBinaryVec(d.GetTime(), fname, vector<T>(&t, &t+1), scale, offset, "", "");
    }


    // -------------------------------------------------------------------

    struct Statistics
    {
        float min;
        float max;
        float med;
        float avg;
        //float rms;

        template<class T>
            Statistics(const T &t, size_t offset_min=0, size_t offset_max=0)
            : min(0), max(0), med(0), avg(0)
        {
            if (t.empty())
                return;

            T copy(t);
            sort(copy.begin(), copy.end());

            if (offset_min>t.size())
                offset_min = 0;
            if (offset_max>t.size())
                offset_max = 0;

            min = copy[offset_min];
            max = copy[copy.size()-1-offset_max];
            avg = accumulate (t.begin(), t.end(), 0.)/t.size();

            const size_t p = copy.size()/2;
            med = copy.size()%2 ? copy[p] : (copy[p-1]+copy[p])/2.;
        }
    };

    void HandleControlMessageImp(const EventImp &d)
    {
        if (d.GetSize()==0)
            return;

        fControlMessageHist.add(d.GetText(), d.GetTime());

        ostringstream out;
        out << setprecision(3);
        out << Header(d) << '\n';
        out << HTML::kWhite << '\t';
        out << "<->" << fControlMessageHist.get() << "</->";
        out << '\n';

        ofstream(fPath+"/scriptlog.data") << out.str();
    }

    int HandleDimControlMessage(const EventImp &d)
    {
        if (d.GetSize()==0)
            return GetCurrentState();

        if (d.GetQoS()==MessageImp::kAlarm)
        {
            if (d.GetSize()<2)
                for (auto it=fControlAlarmHist.begin(); it!=fControlAlarmHist.end(); it++)
                    it->time = Time(Time::none);
            else
                fControlAlarmHist.add(d.GetText(), d.GetTime());
        }

        if (d.GetQoS()==MessageImp::kComment && d.GetSize()>1)
            HandleControlMessageImp(d);

        return GetCurrentState();
    }

    int HandleControlStateChange(const EventImp &d)
    {
        if (d.GetSize()==0)
            return StateMachineImp::kSM_KeepState;

        if (fDimControl.scriptdepth>0)
            return StateMachineImp::kSM_KeepState;

        if (d.GetQoS()>=2)
            return StateMachineImp::kSM_KeepState;

#if BOOST_VERSION < 104600
        const string file = boost::filesystem::path(fDimControl.file).filename();
#else
        const string file = boost::filesystem::path(fDimControl.file).filename().string();
#endif

        // [0] DimControl::kIdle
        // [1] DimControl::kLoading
        // [2] DimControl::kCompiling
        // [3] DimControl::kRunning
        if (d.GetQoS()==1)
        {
            fControlMessageHist.clear();
            HandleControlMessageImp(Event(d, "========================================", 41));
        }

        HandleControlMessageImp(Event(d, ("----- "+fDimControl.shortmsg+" -----").data(), fDimControl.shortmsg.length()+13));
        if (!file.empty() && d.GetQoS()<2)
            HandleControlMessageImp(Event(d, file.data(), file.length()+1));

        // Note that this will also "ding" just after program startup
        // if the dimctrl is still in state -3
        if (d.GetQoS()==0)
        {
            HandleControlMessageImp(Event(d, "========================================", 41));
            if (fDimControl.last.second!=DimState::kOffline)
                SetAudio("ding");
        }

        return StateMachineImp::kSM_KeepState;
    }

    void AddMcpConfigurationHist(const EventImp &d, const string &msg)
    {
        fMcpConfigurationHist.add(msg, d.GetTime());

        ostringstream out;
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t';
        out << "<->" << fMcpConfigurationHist.rget() << "</->";
        out << '\n';

        ofstream(fPath+"/observations.data") << out.str();
    }

    int HandleFscControlStateChange(const EventImp &d)
    {
        const int32_t &last  = fDimFscControl.last.second;
        const int32_t &state = fDimFscControl.state();

        if (last==DimState::kOffline || state==DimState::kOffline)
            return StateMachineImp::kSM_KeepState;

        if (last<FSC::State::kConnected && state==FSC::State::kConnected)
        {
            AddMcpConfigurationHist(d, "<B>FSC swiched on</B>");
            //SetAudio("startup");
        }

        if (last==FSC::State::kConnected && state<FSC::State::kConnected)
        {
            AddMcpConfigurationHist(d, "<B>FSC swiched off</B>");
            //SetAudio("shutdown");
        }

        return StateMachineImp::kSM_KeepState;
    }

    int HandleMcpConfiguration(const EventImp &d)
    {
        if (!CheckDataSize(d, "Mcp:Configuration", 16, true))
        {
            fMcpConfigurationState     = DimState::kOffline;
            fMcpConfigurationMaxTime   = 0;
            fMcpConfigurationMaxEvents = 0;
            fMcpConfigurationName      = "";
            fMcpConfigurationRunStart  = Time(Time::none);
            return GetCurrentState();
        }

        // If a run ends...
        if (fMcpConfigurationState==MCP::State::kTakingData && d.GetQoS()==MCP::State::kIdle)
        {
            // ...and no script is running just play a simple 'tick'
            // ...and a script is running just play a simple 'tick'
            if (/*fDimControl.state()<-2 &&*/ fDimControl.scriptdepth==0)
                SetAudio("dong");
            else
                SetAudio("losticks");

            fLastRunFinishedWithZeroEvents = fFadControlNumEvents==0;

            ostringstream out;
            out << "<#darkred>" << d.Ptr<char>(16);
            if (!fDriveControlSourceName.empty())
                out << " [" << fDriveControlSourceName << ']';
            out << " (N=" << fFadControlNumEvents << ')';
            out << "</#>";

            AddMcpConfigurationHist(d, out.str());
        }

        if (d.GetQoS()==MCP::State::kTakingData)
        {
            fMcpConfigurationRunStart = Time();
            SetAudio("losticks");

            ostringstream out;
            out << "<#darkgreen>" << fMcpConfigurationName;
            if (!fDriveControlSourceName.empty())
                out << " [" << fDriveControlSourceName << ']';
            if (fFadControlStartRun>0)
                out << " (Run " << fFadControlStartRun << ')';
            out << "</#>";

            AddMcpConfigurationHist(d, out.str());
        }

        fMcpConfigurationState     = d.GetQoS();
        fMcpConfigurationMaxTime   = d.Get<uint64_t>();
        fMcpConfigurationMaxEvents = d.Get<uint64_t>(8);
        fMcpConfigurationName      = d.Ptr<char>(16);

        return GetCurrentState();
    }

    void WriteWeather(const EventImp &d, const string &name, int i, float min, float max)
    {
        const Statistics stat(fMagicWeatherHist[i]);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';

        out << HTML::kWhite << '\t' << fMagicWeatherHist[i].back() << '\n';
        out << HTML::kWhite << '\t' << stat.min << '\n';
        out << HTML::kWhite << '\t' << stat.avg << '\n';
        out << HTML::kWhite << '\t' << stat.max << '\n';

        ofstream(fPath+"/"+name+".data") << out.str();

        WriteHist(d, "hist-magicweather-"+name, fMagicWeatherHist[i], max-min, min);
    }

    int HandleMagicWeatherData(const EventImp &d)
    {
        if (!CheckDataSize(d, "MagicWeather:Data", 7*4+2))
            return GetCurrentState();

        // Store a history of the last 300 entries
        for (int i=kWeatherBegin; i<kWeatherEnd; i++)
        {
            fMagicWeatherHist[i].push_back(d.Ptr<float>(2)[i]);
            if (fMagicWeatherHist[i].size()>300)
                fMagicWeatherHist[i].pop_front();
        }

        ostringstream out;
        out << d.GetJavaDate() << '\n';
        if (fSun.time.IsValid() && fMoon.time.IsValid())
        {
            out << fSun.color << '\t' << fSun.description << '\n';
            out << setprecision(2);
            out << (fSun.isday?HTML::kWhite:fMoon.color) << '\t' << fMoon.description << '\n';
        }
        else
            out << "\n\n";
        out << setprecision(3);
        for (int i=0; i<6; i++)
            out << HTML::kWhite << '\t' << fMagicWeatherHist[i].back() << '\n';
        out << HTML::kWhite << '\t' << GetDir(fMagicWeatherHist[kDir].back()) << '\n';
        out << HTML::kWhite << '\t';
        if (!fTngWeatherDustHist.empty())
            out << fTngWeatherDustHist.back() << '\t' << fTngWeatherDustTime.GetAsStr("%H:%M") << '\n';
        else
            out << "\t\n";
        out << HTML::kWhite << '\t';
        if (!fGtcDustHist.empty())
            out << fGtcDustHist.back() << '\t' << fGtcDustTime.GetAsStr("%H:%M") << '\n';
        else
            out << "\t\n";

        ofstream(fPath+"/weather.data") << out.str();

        WriteWeather(d, "temp",  kTemp,   -5,   35);
        WriteWeather(d, "dew",   kDew,    -5,   35);
        WriteWeather(d, "hum",   kHum,     0,  100);
        WriteWeather(d, "wind",  kWind,    0,  100);
        WriteWeather(d, "gusts", kGusts,   0,  100);
        WriteWeather(d, "press", kPress, 700, 1000);

        return GetCurrentState();
    }

    int HandleTngWeatherDust(const EventImp &d)
    {
        if (!CheckDataSize(d, "TngWeather:Dust", 4))
            return GetCurrentState();

        fTngWeatherDustTime = d.GetTime();

        fTngWeatherDustHist.push_back(d.GetFloat());
        if (fTngWeatherDustHist.size()>300)
                fTngWeatherDustHist.pop_front();

        const Statistics stat(fTngWeatherDustHist);

        const double scale = stat.max>0 ? pow(10, ceil(log10(stat.max))) : 0;

        WriteHist(d, "hist-tng-dust", fTngWeatherDustHist, scale);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        ofstream(fPath+"/tngdust.data") << out.str();

        return GetCurrentState();
    }

    int HandleTngWeatherData(const EventImp &d)
    {
        if (!CheckDataSize(d, "TngWeather:Data", sizeof(TNGWeather::DimWeather)))
            return GetCurrentState();

        const auto &data = d.Ref<TNGWeather::DimWeather>();

        ostringstream out;
        out << d.GetJavaDate() << '\n';
        out << setprecision(3);
        out << HTML::kWhite << '\t' << data.fTemperature << '\n';
        out << HTML::kWhite << '\t' << data.fTempTrend << '\n';
        out << HTML::kWhite << '\t' << data.fDewPoint << '\n';
        out << HTML::kWhite << '\t' << data.fHumidity << '\n';
        out << HTML::kWhite << '\t' << data.fAirPressure << '\n';
        out << HTML::kWhite << '\t' << data.fWindSpeed << '\n';
        out << HTML::kWhite << '\t' << data.fWindDirection << '\n';
        out << HTML::kWhite << '\t' << data.fDustTotal << '\n';
        out << HTML::kWhite << '\t' << data.fSolarimeter << '\n';

        ofstream(fPath+"/tngdata.data") << out.str();

        return GetCurrentState();
    }

    int HandleGtcDustData(const EventImp &d)
    {
        if (!CheckDataSize(d, "GtcDust:Data", 4))
            return GetCurrentState();

        fGtcDustTime = d.GetTime();

        fGtcDustHist.push_back(d.GetFloat());
        if (fGtcDustHist.size()>300)
                fGtcDustHist.pop_front();

        const Statistics stat(fGtcDustHist);

        const double scale = stat.max>0 ? pow(10, ceil(log10(stat.max))) : 0;

        WriteHist(d, "hist-gtc-dust", fGtcDustHist, scale);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        ofstream(fPath+"/gtcdust.data") << out.str();

        return GetCurrentState();
    }


    int HandleRainSensorData(const EventImp &d)
    {
        if (!CheckDataSize(d, "RainSensor:Data", 12)) // F:1;X:1
            return GetCurrentState();

        fRainSensorDataTime = d.GetTime();

        fRainSensorDataHist.push_back(d.GetFloat());
        if (fRainSensorDataHist.size()>300)
                fRainSensorDataHist.pop_front();

        const Statistics stat(fRainSensorDataHist);

        const double scale = stat.max>0 ? pow(10, ceil(log10(stat.max))) : 0;

        WriteHist(d, "hist-rain-sensor-data", fRainSensorDataHist, scale);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        ofstream(fPath+"/rainsensor.data") << out.str();

        return GetCurrentState();
    }

    int HandleDriveControlStateChange(const EventImp &d)
    {
        const int32_t &last  = fDimFscControl.last.second;
        const int32_t &state = fDimFscControl.state();

        if (last==DimState::kOffline || state==DimState::kOffline)
            return StateMachineImp::kSM_KeepState;

        if (last<Drive::State::kInitialized && state>=Drive::State::kInitialized)
            AddMcpConfigurationHist(d, "Drive ready");

        if (last>=Drive::State::kInitialized && state<Drive::State::kInitialized)
            AddMcpConfigurationHist(d, "Drive not ready");

        return StateMachineImp::kSM_KeepState;
    }

    int HandleDrivePointing(const EventImp &d)
    {
        if (!CheckDataSize(d, "DriveControl:Pointing", 16))
            return GetCurrentState();

        fDriveControlPointingZd = d.Get<double>();

        const double az = d.Get<double>(8);

        fDriveControlPointingAz = GetDir(az);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        out << setprecision(0) << fixed;
        out << HTML::kWhite << '\t' << az << '\t' << fDriveControlPointingAz << '\n';
        out << HTML::kWhite << '\t' << fDriveControlPointingZd << '\n';

        ofstream(fPath+"/pointing.data") << out.str();

        return GetCurrentState();
    }

    int HandleDriveTracking(const EventImp &d)
    {
        if (!CheckDataSize(d, "DriveControl:Tracking", 96))
            return GetCurrentState();



        const double Ra  = d.Get<double>(0*8);
        const double Dec = d.Get<double>(1*8);
        const double Zd  = d.Get<double>(6*8);
        const double Az  = d.Get<double>(7*8);

        const double dev = d.Get<double>(11*8);

        fDriveControlTrackingDevHist.push_back(dev);
        if (fDriveControlTrackingDevHist.size()>300)
            fDriveControlTrackingDevHist.pop_front();

        WriteHist(d, "hist-control-deviation", fDriveControlTrackingDevHist, 120);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        out << HTML::kWhite << '\t' << fDriveControlSourceName << '\n';
        out << setprecision(5);
        out << HTML::kWhite << '\t' << Ra  << '\n';
        out << HTML::kWhite << '\t' << Dec << '\n';
        out << setprecision(3);
        out << HTML::kWhite << '\t' << Zd  << '\n';
        out << HTML::kWhite << '\t' << Az  << '\n';
        out << HTML::kWhite << '\t' << dev << '\n';

        fDriveControlMoonDist = -1;

        if (fMoon.visible)
        {
            const double angle = fMoon.Angle(Ra, Dec);
            out << Moon::Color(angle) << '\t' << setprecision(3) << angle << '\n';

            fDriveControlMoonDist = angle;
        }
        else
            out << HTML::kWhite << "\t&mdash; \n";

        ofstream(fPath+"/tracking.data") << out.str();

        return GetCurrentState();
    }

    int HandleDriveSource(const EventImp &d)
    {
        if (!CheckDataSize(d, "DriveControl:Source", 5*8+31))
            return GetCurrentState();

        const double *ptr = d.Ptr<double>();

        const double ra     = ptr[0];  // Ra[h]
        const double dec    = ptr[1];  // Dec[deg]
        const double woff   = ptr[2];  // Wobble offset [deg]
        const double wang   = ptr[3];  // Wobble angle  [deg]
        const double period = ptr[4];  // Wobble angle  [deg]

        fDriveControlSourceName = d.Ptr<char>(5*8);

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        out << HTML::kWhite << '\t' << fDriveControlSourceName << '\n';
        out << setprecision(5);
        out << HTML::kWhite << '\t' << ra  << '\n';
        out << HTML::kWhite << '\t' << dec << '\n';
        out << setprecision(3);
        out << HTML::kWhite << '\t' << woff << '\n';
        out << HTML::kWhite << '\t' << wang << '\n';
        out << HTML::kWhite << '\t' << period << '\n';

        ofstream(fPath+"/source.data") << out.str();

        return GetCurrentState();
    }

    int HandleFeedbackCalibratedCurrents(const EventImp &d)
    {
        if (!CheckDataSize(d, "Feedback:CalibratedCurrents", (416+1+1+1+1+1+416+1+1)*sizeof(float)+sizeof(uint32_t)))
            return GetCurrentState();

        const float *ptr = d.Ptr<float>();

        double power_tot = 0;
        double power_apd = 0;

        if (fBiasControlVoltageVec.size()>0)
        {
            // Calibrate the data (subtract offset)
            for (int i=0; i<320; i++)
            {
                // Exclude crazy pixels
                if (i==66 || i==191 || i==193)
                    continue;

                // Group index (0 or 1) of the of the pixel (4 or 5 pixel patch)
                const int N = fPixelMap.hv(i).count();

                // Serial resistor of the individual G-APDs
                double R5 = 3900/N;

                // This is also valid for the patches with wrong resistors,
                // because Iapd is a factor f larger but R a factor f smaller
                double Iapd = ptr[i] * 1e-6;  // [A]
                double Iout = Iapd*N;         // [A]

                double UdrpCam =     1000 *Iout;  // Voltage seen by everything in Camera
                double UdrpApd = (R5+2000)*Iout;  // Voltage seen by G-APD

                const double pwrCam = Iout * (fBiasControlVoltageVec[i]-UdrpCam);
                const double pwrApd = Iout * (fBiasControlVoltageVec[i]-UdrpApd);

                // Total power participated in the camera at the G-APD
                // and the serial resistors (total voltage minus voltage
                // drop at resistors in bias crate)
                power_tot += pwrCam;

                // Power consumption per G-APD
                power_apd += pwrApd;
            }
        }

        // Divide by number of summed channels, convert to mW
        power_apd /= (320-3)*1e-3; // [mW]

        if (power_tot<1e-3)
            power_tot = 0;
        if (power_apd<1e-3)
            power_apd = 0;

        fBiasControlPowerTot = power_tot;

        // --------------------------------------------------------

        // Get the maximum of each patch
        vector<float> val(320, 0);
        for (int i=0; i<320; i++)
        {
            const int idx = (fPixelMap.hv(i).hw()/9)*2+fPixelMap.hv(i).group();
            val[idx] = ptr[i];
        }

        // Write the 160 patch values to a file
        WriteCam(d, "cam-biascontrol-current", val, 100);

        // --------------------------------------------------------

        // After being displayed, exclude the patches with
        // the crazy pixels from the statsitics

        vector<float> cpy(ptr, ptr+320);
        cpy[66]  = 0;
        cpy[191] = 0;
        cpy[193] = 0;
        const Statistics stat(cpy);

        // Exclude the three crazy channels
        fBiasControlCurrentMed = stat.med;
        fBiasControlCurrentMax = stat.max;

        // Store a history of the last 60 entries
        fBiasControlCurrentHist.push_back(fBiasControlCurrentMed);
        if (fBiasControlCurrentHist.size()>360)
            fBiasControlCurrentHist.pop_front();

        // write the history to a file
        WriteHist(d, "hist-biascontrol-current", fBiasControlCurrentHist, 125);

        // --------------------------------------------------------

        string col1 = HTML::kGreen;
        string col2 = HTML::kGreen;
        string col3 = HTML::kGreen;
        string col4 = HTML::kGreen;

        if (stat.min>90)
            col1 = HTML::kYellow;
        if (stat.min>110)
            col1 = HTML::kRed;

        if (stat.med>90)
            col2 = HTML::kYellow;
        if (stat.med>110)
            col2 = HTML::kRed;

        if (stat.avg>90)
            col3 = HTML::kYellow;
        if (stat.avg>110)
            col3 = HTML::kRed;

        if (stat.max>90)
            col4 = HTML::kYellow;
        if (stat.max>110)
            col4 = HTML::kRed;

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kGreen << '\t' << "yes" << '\n';
        out << col1 << '\t' << stat.min << '\n';
        out << col2 << '\t' << stat.med << '\n';
        out << col3 << '\t' << stat.avg << '\n';
        out << col4 << '\t' << stat.max << '\n';
        out << HTML::kWhite << '\t' << power_tot << "W [" << power_apd << "mW]\n";
        ofstream(fPath+"/current.data") << out.str();

        // --------------------------------------------------------

        const float Unom = ptr[2*416+6];
        const float Utmp = ptr[2*416+7];

        vector<float> Uov(ptr+416+6, ptr+416+6+320);

        WriteCam(d, "cam-feedback-overvoltage", Uov, 0.2, -0.1);

        const Statistics stat2(Uov);

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << setprecision(3);
        out << HTML::kWhite << '\t' << Utmp << '\n';
        out << HTML::kWhite << '\t' << Unom << '\n';
        out << HTML::kWhite << '\t' << stat2.min << '\n';
        out << HTML::kWhite << '\t' << stat2.med << '\n';
        out << HTML::kWhite << '\t' << stat2.avg << '\n';
        out << HTML::kWhite << '\t' << stat2.max << '\n';
        ofstream(fPath+"/feedback.data") << out.str();

        return GetCurrentState();
    }

    int HandleBiasCurrent(const EventImp &d)
    {
        if (fDimFeedback.state()>=Feedback::State::kCalibrated)
            return GetCurrentState();

        if (!CheckDataSize(d, "BiasControl:Current", 832))
            return GetCurrentState();

        // Convert dac counts to uA
        vector<float> v(320);
        for (int i=0; i<320; i++)
            v[i] = d.Ptr<uint16_t>()[i] * 5000./4096;

        fBiasControlPowerTot = 0;

        // Get the maximum of each patch
        vector<float> val(320, 0);
        for (int i=0; i<320; i++)
        {
            const PixelMapEntry &hv = fPixelMap.hv(i);
            if (!hv)
                continue;

            const int idx = (hv.hw()/9)*2+hv.group();
            val[idx] = v[i];
        }

        // Write the 160 patch values to a file
        WriteCam(d, "cam-biascontrol-current", val, 1000);

        const Statistics stat(v, 0, 3);

        // Exclude the three crazy channels
        fBiasControlCurrentMed = stat.med;
        fBiasControlCurrentMax = stat.max;

        // Store a history of the last 60 entries
        fBiasControlCurrentHist.push_back(fBiasControlCurrentMed);
        if (fBiasControlCurrentHist.size()>360)
            fBiasControlCurrentHist.pop_front();

        // write the history to a file
        WriteHist(d, "hist-biascontrol-current", fBiasControlCurrentHist, 1000);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite<< '\t' << "no" << '\n';
        out << HTML::kWhite << '\t' << stat.min << '\n';
        out << HTML::kWhite << '\t' << stat.med << '\n';
        out << HTML::kWhite << '\t' << stat.avg << '\n';
        out << HTML::kWhite << '\t' << stat.max << '\n';
        out << HTML::kWhite << '\t' << "---\n";
        ofstream(fPath+"/current.data") << out.str();

        return GetCurrentState();
    }

    int HandleBiasVoltage(const EventImp &d)
    {
        if (!CheckDataSize(d, "BiasControl:Voltage", 1664))
        {
            fBiasControlVoltageVec.clear();
            return GetCurrentState();
        }

        fBiasControlVoltageVec.assign(d.Ptr<float>(), d.Ptr<float>()+320);

        const Statistics stat(fBiasControlVoltageVec);

        fBiasControlVoltageMed = stat.med;

        vector<float> val(320, 0);
        for (int i=0; i<320; i++)
        {
            const int idx = (fPixelMap.hv(i).hw()/9)*2+fPixelMap.hv(i).group();
            val[idx] = fBiasControlVoltageVec[i];
        }

        if (fDimBiasControl.state()==BIAS::State::kVoltageOn || fDimBiasControl.state()==BIAS::State::kRamping)
            WriteCam(d, "cam-biascontrol-voltage", val, 10, 65);
        else
            WriteCam(d, "cam-biascontrol-voltage", val, 75);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << stat.min << '\n';
        out << HTML::kWhite << '\t' << stat.med << '\n';
        out << HTML::kWhite << '\t' << stat.avg << '\n';
        out << HTML::kWhite << '\t' << stat.max << '\n';
        ofstream(fPath+"/voltage.data") << out.str();

        return GetCurrentState();
    }

    int HandleFadEvents(const EventImp &d)
    {
        if (!CheckDataSize(d, "FadControl:Events", 4*4))
        {
            fFadControlNumEvents = -1;
            return GetCurrentState();
        }

        fFadControlNumEvents = d.Get<uint32_t>();

        return GetCurrentState();
    }

    int HandleFadStartRun(const EventImp &d)
    {
        if (!CheckDataSize(d, "FadControl:StartRun", 16))
        {
            fFadControlStartRun = -1;
            return GetCurrentState();
        }

        fFadControlStartRun = d.Get<int64_t>();

        return GetCurrentState();
    }

    int HandleFadDrsRuns(const EventImp &d)
    {
        if (!CheckDataSize(d, "FadControl:DrsRuns", 5*4))
        {
            fFadControlDrsStep = -1;
            return GetCurrentState();
        }

        const uint32_t *ptr = d.Ptr<uint32_t>();
        fFadControlDrsStep    = ptr[0];
        fFadControlDrsRuns[0] = ptr[1];
        fFadControlDrsRuns[1] = ptr[2];
        fFadControlDrsRuns[2] = ptr[3];

        return GetCurrentState();
    }

    int HandleFadConnections(const EventImp &d)
    {
        if (!CheckDataSize(d, "FadControl:Connections", 41))
        {
            //fStatusEventBuilderLabel->setText("Offline");
            return GetCurrentState();
        }

        string rc(40, '-'); // orange/red [45]

        const uint8_t *ptr = d.Ptr<uint8_t>();

        int c[4] = { '.', '.', '.', '.' };

        for (int i=0; i<40; i++)
        {
            const uint8_t stat1 = ptr[i]&3;
            const uint8_t stat2 = ptr[i]>>3;

            if (stat1==0 && stat2==0)
                rc[i] = '.'; // gray [46]
            else
                if (stat1>=2 && stat2==8)
                    rc[i] = stat1==2?'+':'*';  // green [43] : check [42]

            if (rc[i]<c[i/10])
                c[i/10] = rc[i];
        }

        string col[4];
        for (int i=0; i<4; i++)
            switch (c[i])
            {
            case '.': col[i]=HTML::kWhite;  break;
            case '-': col[i]=HTML::kRed;    break;
            case '+': col[i]=HTML::kYellow; break;
            case '*': col[i]=HTML::kGreen;  break;
            }

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << col[0] << '\t' << rc.substr( 0, 10) << '\n';
        out << col[1] << '\t' << rc.substr(10, 10) << '\n';
        out << col[2] << '\t' << rc.substr(20, 10) << '\n';
        out << col[3] << '\t' << rc.substr(30, 10) << '\n';
        ofstream(fPath+"/fad.data") << out.str();

        return GetCurrentState();
    }

    /*
    int HandleFtmControlStateChange()
    {
        const int32_t &last  = fDimFtmControl.last.second;
        const int32_t &state = fDimFtmControl.state();

        // If a new run has been started ensure that the counter
        // is reset. The reset in HandleFtmTriggerRates might
        // arrive only after the run was started.
        if (last!=FTM::State::kTriggerOn && state==MCP::State::kTriggerOn)
            fFtmControlTriggerRateTooLow = -1;

        return StateMachineImp::kSM_KeepState;
    }*/


    int HandleFtmTriggerRates(const EventImp &d)
    {
        if (!CheckDataSize(d, "FtmControl:TriggerRates", 24+160+640+8))
        {
            fFtmControlTriggerRateTooLow = 0;
            return GetCurrentState();
        }

        const FTM::DimTriggerRates &dim = d.Ref<FTM::DimTriggerRates>();

        // If the trigger rate is too low...
        // ... and the run was not just started (can lead to very small elapsed times)
        // ... and the trigger is switched on
        // ... and there was no state change (then the trigger was started or stopped)
        fFtmControlTriggerRateTooLow =
            dim.fTriggerRate<1 && dim.fElapsedTime>0.45 &&
            (fFtmControlState&FTM::kFtmStates)==FTM::kFtmRunning &&
            (fFtmControlState&FTM::kFtmStates)==(d.GetQoS()&FTM::kFtmStates);

        fFtmControlState = d.GetQoS();

        const float *brates = dim.fBoardRate; // Board rate
        const float *prates = dim.fPatchRate; // Patch rate

        // Store a history of the last 60 entries
        fFtmControlTriggerRateHist.push_back(dim.fTriggerRate);
        if (fFtmControlTriggerRateHist.size()>300)
            fFtmControlTriggerRateHist.pop_front();

        // FIXME: Add statistics for all kind of rates

        WriteHist(d, "hist-ftmcontrol-triggerrate",
                  fFtmControlTriggerRateHist, 100);
        WriteCam(d, "cam-ftmcontrol-boardrates",
                 vector<float>(brates, brates+40), 10);
        WriteCam(d, "cam-ftmcontrol-patchrates",
                 vector<float>(prates, prates+160), 10);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << dim.fTriggerRate << '\n';

        ofstream(fPath+"/trigger.data") << out.str();

        const Statistics bstat(vector<float>(brates, brates+ 40));
        const Statistics pstat(vector<float>(prates, prates+160));

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << bstat.min << '\n';
        out << HTML::kWhite << '\t' << bstat.med << '\n';
        out << HTML::kWhite << '\t' << bstat.avg << '\n';
        out << HTML::kWhite << '\t' << bstat.max << '\n';
        ofstream(fPath+"/boardrates.data") << out.str();

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << pstat.min << '\n';
        out << HTML::kWhite << '\t' << pstat.med << '\n';
        out << HTML::kWhite << '\t' << pstat.avg << '\n';
        out << HTML::kWhite << '\t' << pstat.max << '\n';
        ofstream(fPath+"/patchrates.data") << out.str();

        return GetCurrentState();
    }

    int HandleFtmStaticData(const EventImp &d)
    {
        if (!CheckDataSize(d, "FtmControl:StaticData", sizeof(FTM::DimStaticData)))
            return GetCurrentState();

        // If the FTM is in state Configuring, the clock conditioner
        // is always reported to be unlocked
        fFtmControlState = d.GetQoS();

        const FTM::DimStaticData &dat = d.Ref<FTM::DimStaticData>();

        vector<uint16_t> vecp(dat.fThreshold, dat.fThreshold+160);
        vector<uint16_t> vecb(dat.fMultiplicity, dat.fMultiplicity+40);

        WriteCam(d, "cam-ftmcontrol-thresholds-patch", vecp, 1000);
        WriteCam(d, "cam-ftmcontrol-thresholds-board", vecb,  100);

        const Statistics statp(vecp);
        const Statistics statb(vecb);

        fFtmPatchThresholdMed = statp.med;
        fFtmBoardThresholdMed = statb.med;

        ostringstream out;
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << statb.min << '\n';
        out << HTML::kWhite << '\t' << statb.med << '\n';
        out << HTML::kWhite << '\t' << statb.max << '\n';
        ofstream(fPath+"/thresholds-board.data") << out.str();

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << statp.min << '\n';
        out << HTML::kWhite << '\t' << statp.med << '\n';
        out << HTML::kWhite << '\t' << statp.max << '\n';
        ofstream(fPath+"/thresholds-patch.data") << out.str();

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << statb.med << '\n';
        out << HTML::kWhite << '\t' << statp.med << '\n';
        ofstream(fPath+"/thresholds.data") << out.str();

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << dat.fTriggerInterval << '\n';
        out << HTML::kWhite << '\t';
        if (dat.HasPedestal())
            out << dat.fTriggerSeqPed;
        else
            out << "&ndash;";
        out << ':';
        if (dat.HasLPext())
            out << dat.fTriggerSeqLPext;
        else
            out << "&ndash;";
        out << ':';
        if (dat.HasLPint())
            out << dat.fTriggerSeqLPint;
        else
            out << "&ndash;";
        out << '\n';

        out << HTML::kWhite << '\t' << (dat.HasTrigger()?"on":"off") << " / " << (dat.HasExt1()?"on":"off") << " / " << (dat.HasExt2()?"on":"off") << '\n';
        out << HTML::kWhite << '\t' << (dat.HasVeto()?"on":"off") << " / " << (dat.HasClockConditioner()?"time cal":"marker") << '\n';
        out << HTML::kWhite << '\t' << dat.fMultiplicityPhysics << " / " << dat.fMultiplicityCalib << '\n';
        out << HTML::kWhite << '\t' << dat.fWindowPhysics << '\t' << dat.fWindowCalib << '\n';
        out << HTML::kWhite << '\t' << dat.fDelayTrigger << '\t' << dat.fDelayTimeMarker << '\n';
        out << HTML::kWhite << '\t' << dat.fDeadTime << '\n';

        int64_t vp = dat.fPrescaling[0];
        for (int i=1; i<40; i++)
            if (vp!=dat.fPrescaling[i])
                vp = -1;

        if (vp<0)
            out << HTML::kYellow << "\tdifferent\n";
        else
            out << HTML::kWhite  << '\t' << 0.5*vp << "\n";

        ofstream(fPath+"/ftm.data") << out.str();

        // Active FTUs: IsActive(i)
        // Enabled Pix: IsEnabled(i)

        return GetCurrentState();
    }

    int HandleFtmFtuList(const EventImp &d)
    {
        if (!CheckDataSize(d, "FtmControl:FtuList", sizeof(FTM::DimFtuList)))
            return GetCurrentState();

        const FTM::DimFtuList &sdata = d.Ref<FTM::DimFtuList>();

        ostringstream out;
        out << d.GetJavaDate() << '\n';

        int cnt = 0;
        for (int i=0; i<4; i++)
        {
            out << HTML::kWhite << '\t';
            for (int j=0; j<10; j++)
                if (sdata.IsActive(i*10+j))
                {
                    if (sdata.fPing[i*10+j]==1)
                    {
                        out << '*';
                        cnt++;
                    }
                    else
                        out << sdata.fPing[i*10+j];
                }
                else
                    out << '-';
            out << '\n';
        }

        fFtmControlFtuOk = cnt==40;

        ofstream(fPath+"/ftu.data") << out.str();

        return GetCurrentState();
    }

    int HandleFadEventData(const EventImp &d)
    {
        if (!CheckDataSize(d, "FadControl:EventData", 23048))
            return GetCurrentState();

        //const uint32_t run = d.GetUint();
        //const uint32_t evt = d.GetUint(4);

        const float *dat = d.Ptr<float>(8+1440*sizeof(float)*2);

        /*
        vector<float> max(320, 0);
        for (int i=0; i<1440; i++)
        {
            if (i%9==8)
                continue;

            const int idx = (fPixelMap.hw(i).hw()/9)*2+fPixelMap.hw(i).group();
            const double v = dat[i]/1000;
            //if (v>max[idx])
            //    max[idx]=v;

            max[idx] += v/4;
            } */

        vector<float> val(1440);
        for (int i=0; i<1440; i++)
            val[i] = dat[i]/1000;

        vector<float> sorted(val);
        nth_element(sorted.begin(), sorted.begin()+3, sorted.end(),
                    std::greater<float>());

        const uint32_t trig = d.GetQoS() & FAD::EventHeader::kLPext;

        const float min = fFadControlDrsRuns[0]==0 ? -1 : 0;

        float scale = 2;
        if (trig&FAD::EventHeader::kLPext)
            scale = 1;
        if (trig&FAD::EventHeader::kPedestal)
            scale = 0.25;
        if (trig==0)
            scale = max(0.25f, sorted[3]);

        // assume it is drs-gain
        //if ((trig&FAD::EventHeader::kPedestal) && fFadControlDrsRuns[0]>0 && fFadControlDrsRuns[1]==0)
        //    min = 0.75;

        WriteCam(d, "cam-fadcontrol-eventdata", val, scale, min);

        return GetCurrentState();
    }

    int HandleStats(const EventImp &d)
    {
        if (!CheckDataSize(d, "Stats", 4*8))
        {
            fFreeSpace = UINT64_MAX;
            return GetCurrentState();
        }

        const DimWriteStatistics::Stats &s = d.Ref<DimWriteStatistics::Stats>();
        fFreeSpace = s.freeSpace;

        return GetCurrentState();
    }

    int HandleFscTemperature(const EventImp &d)
    {
        if (!CheckDataSize(d, "FscControl:Temperature", 240))
            return GetCurrentState();

        const float *ptr = d.Ptr<float>(4);

        double avg =   0;
        double rms =   0;
        double min =  99;
        double max = -99;

        int num = 0;
        for (const float *t=ptr; t<ptr+31; t++)
        {
            if (*t==0)
                continue;

            if (*t>max)
                max = *t;

            if (*t<min)
                min = *t;

            avg += *t;
            rms += *t * *t;

            num++;
        }

        avg /= num;
        rms /= num;
        rms += avg*avg;
        rms = rms<0 ? 0 : sqrt(rms);

        // Clean broken reports
        static double pre_rms1 = 1.5;
        static double pre_rms2 = 0;

        const double cut = pre_rms1 + 0.1;

        const bool reject = rms>cut && pre_rms2<cut;

        pre_rms2 = pre_rms1;
        pre_rms1 = rms;

        if (reject)
            return GetCurrentState();


        if (!fMagicWeatherHist[kTemp].empty())
        {
            fFscControlTemperatureHist.push_back(avg-fMagicWeatherHist[kTemp].back());
            if (fFscControlTemperatureHist.size()>300)
                fFscControlTemperatureHist.pop_front();
        }

        const Statistics stat(fFscControlTemperatureHist);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << fFscControlHumidityAvg << '\n';
        out << HTML::kWhite << '\t' << stat.min << '\n';
        out << HTML::kWhite << '\t' << stat.avg << '\n';
        out << HTML::kWhite << '\t' << stat.max << '\n';

        ofstream(fPath+"/fsc.data") << out.str();

        WriteHist(d, "hist-fsccontrol-temperature",
                  fFscControlTemperatureHist, 10);

        out.str("");
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << max << '\n';
        out << HTML::kWhite << '\t' << avg << '\n';
        out << HTML::kWhite << '\t' << min << '\n';

        ofstream(fPath+"/camtemp.data") << out.str();

        return GetCurrentState();
    }

    int HandleFscBiasTemp(const EventImp &d)
    {
        if (!CheckDataSize(d, "FscControl:BiasTemp", 323*4))
            return GetCurrentState();

        const float *ptr = d.Ptr<float>(4);
        const float avg = d.Get<float>(321*4);
        //const float rms = d.Get<float>(322*4);

        vector<double> tout(320);
        for (int i=0; i<320; i++)
        {
            const int idx = (fPixelMap.hv(i).hw()/9)*2+fPixelMap.hv(i).group();
            tout[idx] = ptr[i];
        }

        WriteCam(d, "cam-fsccontrol-temperature", tout, 3, avg-1.75);

        return GetCurrentState();
    }

    int HandleFscHumidity(const EventImp &d)
    {
        if (!CheckDataSize(d, "FscControl:Humidity", 5*4))
            return GetCurrentState();

        const float *ptr = d.Ptr<float>(4);

        double avg =0;
        int num = 0;

        for (const float *t=ptr; t<ptr+4; t++)
            if (*t>0 && *t<=100 && t!=ptr+2 /*excl broken sensor*/)
            {
                avg += *t;
                num++;
            }

        fFscControlHumidityAvg = num>0 ? avg/num : 0;

        return GetCurrentState();
    }

    int HandlePfMiniData(const EventImp &d)
    {
        if (!CheckDataSize(d, "PfMini:Data", sizeof(PFmini::Data)))
            return GetCurrentState();

        const PFmini::Data &data = d.Ref<PFmini::Data>();

        ostringstream out;

        out << fixed << setprecision(1);
        out << d.GetJavaDate() << '\n';

        out << HTML::kGreen << '\t' << data.temp << '\n';
        out << HTML::kGreen << '\t' << data.hum  << '\n';

        ofstream(fPath+"/pfmini.data") << out.str();

        fPfMiniTemperatureHist.push_back(data.temp);
        if (fPfMiniTemperatureHist.size()>60*4) // 1h
            fPfMiniTemperatureHist.pop_front();

        fPfMiniHumidityHist.push_back(data.hum);
        if (fPfMiniHumidityHist.size()>60*4) // 1h
            fPfMiniHumidityHist.pop_front();

        WriteHist(d, "hist-pfmini-temp",
                  fPfMiniTemperatureHist, 45, 0);

        WriteHist(d, "hist-pfmini-hum",
                  fPfMiniHumidityHist, 100, 0);

        return GetCurrentState();
    }

    int HandleBiasTemp(const EventImp &d)
    {
        if (!CheckDataSize(d, "BiasTemp:Data", sizeof(BiasTemp::Data)))
            return GetCurrentState();

        const BiasTemp::Data &data = d.Ref<BiasTemp::Data>();

        ostringstream out;

        out << fixed << setprecision(1);
        out << d.GetJavaDate() << '\n';

        out << HTML::kGreen << '\t' << data.time << '\n';
        out << HTML::kGreen << '\t' << data.avg << '\n';
        out << HTML::kGreen << '\t' << data.rms  << '\n';

        ofstream(fPath+"/biastemp.data") << out.str();

        return GetCurrentState();
    }

    int HandleGpsNema(const EventImp &d)
    {
        if (!CheckDataSize(d, "GpsControl:Nema", sizeof(GPS::NEMA)))
            return GetCurrentState();

        const GPS::NEMA &nema = d.Ref<GPS::NEMA>();

        ostringstream out;

        out << fixed;
        out << d.GetJavaDate() << '\n';

        switch (nema.qos)
        {
        case 1:  out << HTML::kGreen << "\tGPS fix [1]\n"; break;
        case 2:  out << HTML::kGreen << "\tDifferential fix [2]\n"; break;
        default: out << HTML::kRed << "\tinvalid [" << nema.qos << "]\n"; break;
        }

        out << HTML::kWhite << '\t' << nema.count << '\n';
        out << HTML::kWhite << '\t' << Time(floor(Time().Mjd())+nema.time).GetAsStr("%H:%M:%S") << '\n';
        out << HTML::kWhite << '\t' << setprecision(4) << nema.lat    << '\n';
        out << HTML::kWhite << '\t' << setprecision(4) << nema.lng    << '\n';
        out << HTML::kWhite << '\t' << setprecision(1) << nema.height << "\n";
        out << HTML::kWhite << '\t' << setprecision(1) << nema.hdop   << "\n";
        out << HTML::kWhite << '\t' << setprecision(1) << nema.geosep << "\n";

        ofstream(fPath+"/gps.data") << out.str();

        return GetCurrentState();
    }

    int HandleSqmData(const EventImp &d)
    {
        if (!CheckDataSize(d, "SqmControl:Data", sizeof(SQM::Data)))
            return GetCurrentState();

        const SQM::Data &data = d.Ref<SQM::Data>();

        ostringstream out;

        out << fixed;
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << setprecision(2) << data.mag    << '\n';
        out << HTML::kWhite << '\t' <<                    data.freq   << '\n';
        out << HTML::kWhite << '\t' <<                    data.counts << '\n';
        out << HTML::kWhite << '\t' << setprecision(3) << data.period << '\n';
        out << HTML::kWhite << '\t' << setprecision(1) << data.temp   << "\n";

        ofstream(fPath+"/sqm.data") << out.str();

        return GetCurrentState();
    }

    string GetTempColor(float t)
    {
        if (t>25 && t<30)
            return HTML::kGreen;

        if (t<20 || t>35)
            return HTML::kRed;

        return HTML::kYellow;
    }

    int HandleTemperatureData(const EventImp &d)
    {
        if (!CheckDataSize(d, "Temperature:Data", 3*sizeof(float)))
            return GetCurrentState();

        const float *temp = d.Ptr<float>();

        ostringstream out;

        out << fixed << setprecision(1);
        out << d.GetJavaDate() << '\n';

        out << GetTempColor(temp[1]) << '\t' << temp[1] << '\n';
        out << GetTempColor(temp[0]) << '\t' << temp[0] << '\n';
        out << GetTempColor(temp[2]) << '\t' << temp[2] << '\n';

        ofstream(fPath+"/temperature.data") << out.str();

        fTemperatureControlHist.push_back(temp[0]);
        if (fTemperatureControlHist.size()>60) // 1h
            fTemperatureControlHist.pop_front();

        WriteHist(d, "hist-temperaturecontrol",
                  fTemperatureControlHist, 45, 0);

        return GetCurrentState();
    }

    int HandleAgilentData(const EventImp &d, const string &ext)
    {
        if (!CheckDataSize(d, ("Agilent"+ext+":Data").c_str(), 4*sizeof(float)))
            return GetCurrentState();

        const float *data = d.Ptr<float>();

        ostringstream out;

        out << fixed << setprecision(1);
        out << d.GetJavaDate() << '\n';

        out << HTML::kWhite << '\t' << data[0] << '\n';
        out << HTML::kWhite << '\t' << data[1] << '\n';
        out << HTML::kWhite << '\t' << data[2] << '\n';
        out << HTML::kWhite << '\t' << data[3] << '\n';

        ofstream(fPath+"/agilent"+ext+".data") << out.str();

        return GetCurrentState();
    }

    int HandleRateScanData(const EventImp &d)
    {
        if (!CheckDataSize(d, "RateScan:Data", 824))
            return GetCurrentState();

        const uint64_t id   = d.Get<uint64_t>();
        const float   *rate = d.Ptr<float>(20);

        if (fRateScanDataId!=id)
        {
            for (int i=0; i<41; i++)
                fRateScanDataHist[i].clear();
            fRateScanDataId = id;
        }
        fRateScanDataHist[0].push_back(log10(rate[0]));

        double max = 0;
        for (int i=1; i<41; i++)
        {
            fRateScanDataHist[i].push_back(log10(rate[i]));
            if (rate[i]>max)
                max = rate[i];
        }

        // Cycle by time!
        fRateScanBoard ++;
        fRateScanBoard %= 40;

        WriteHist(d, "hist-ratescan",      fRateScanDataHist[0],                10, -2);
        WriteCam(d,  "cam-ratescan-board", fRateScanDataHist[fRateScanBoard+1], 10, -4);

        ostringstream out;
        out << setprecision(3);
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << fFtmBoardThresholdMed << '\n';
        out << HTML::kWhite << '\t' << fFtmPatchThresholdMed << '\n';
        out << HTML::kWhite << '\t' << floor(pow(10, fRateScanDataHist[0].back())+.5) << '\n';
        out << HTML::kWhite << '\t' << floor(max+.5) << '\n';

        ofstream(fPath+"/ratescan.data") << out.str();

        out.str("");
        out << d.GetJavaDate() << '\n';
        out << HTML::kWhite << '\t' << int(fRateScanBoard) << '\n';
        out << HTML::kWhite << '\t' << pow(10, fRateScanDataHist[fRateScanBoard+1].back()) << '\n';

        ofstream(fPath+"/ratescan_board.data") << out.str();

        return GetCurrentState();
    }

    int HandleRateControlThreshold(const EventImp &d)
    {
        if (!CheckDataSize(d, "RateControl:Threshold", 18))
            return GetCurrentState();

        const uint16_t th = d.Get<uint16_t>();

        fRateControlThreshold.push_back(th);
        if (fRateControlThreshold.size()>300)
            fRateControlThreshold.pop_front();

        WriteHist(d, "hist-ratecontrol-threshold", fRateControlThreshold, 1000);

        return GetCurrentState();
    }

    int HandleChatMsg(const EventImp &d)
    {
        if (d.GetSize()==0 || d.GetQoS()!=MessageImp::kComment)
            return GetCurrentState();

        if (Time()<d.GetTime()+boost::posix_time::minutes(1))
            SetAudio("message");

        fChatHist.add(d.GetText(), d.GetTime());

        ostringstream out;
        out << setprecision(3);
        out << Header(d) << '\n';
        out << HTML::kWhite << '\t';
        out << "<->" << fChatHist.rget() << "</->";
        out << '\n';

        ofstream(fPath+"/chat.data") << out.str();

        return GetCurrentState();
    }

    // -------------------------------------------------------------------

    int HandleDoTest(const EventImp &d)
    {
        ostringstream out;
        out << d.GetJavaDate() << '\n';

        switch (d.GetQoS())
        {
        case -3: out << HTML::kWhite << "\tNot running\n"; break;
        case -2: out << HTML::kBlue  << "\tLoading\n";     break;
        case -1: out << HTML::kBlue  << "\tStarted\n";     break;
        default: out << HTML::kGreen << "\tRunning [" << d.GetQoS() << "]\n"; break;
        }

        ofstream(fPath+"/dotest.data") << out.str();

        return StateMachineImp::kSM_KeepState;
    }

    // -------------------------------------------------------------------

    /*
    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        Fatal(msg);
        return false;
    }*/

    int Print() const
    {
        Out() << fDimDNS            << endl;
        Out() << fDimMcp            << endl;
        Out() << fDimControl        << endl;
        Out() << fDimDataLogger     << endl;
        Out() << fDimDriveControl   << endl;
        Out() << fDimTimeCheck      << endl;
        Out() << fDimFadControl     << endl;
        Out() << fDimFtmControl     << endl;
        Out() << fDimBiasControl    << endl;
        Out() << fDimFeedback       << endl;
        Out() << fDimRateControl    << endl;
        Out() << fDimFscControl     << endl;
        Out() << fDimAgilentControl24 << endl;
        Out() << fDimAgilentControl50 << endl;
        Out() << fDimAgilentControl80 << endl;
        Out() << fDimPwrControl     << endl;
        Out() << fDimLidControl     << endl;
        Out() << fDimMagicWeather   << endl;
        Out() << fDimTngWeather     << endl;
        Out() << fDimGtcDust        << endl;
        Out() << fDimMagicLidar     << endl;
        Out() << fDimTemperature    << endl;
        Out() << fDimRainSensor     << endl;
        Out() << fDimRateScan       << endl;
        Out() << fDimChat           << endl;
        Out() << fDimSkypeClient    << endl;

        return GetCurrentState();
    }

    string GetStateHtml(const DimState &state, int green) const
    {
        if (!state.online())
            return HTML::kWhite+"\t&mdash;\n";

        if (&state==&fDimControl)
            return HTML::kGreen +'\t'+(state.state()==0?"Idle":fDimControl.shortmsg)+'\n';

        const State rc = state.description();

        // Sate not found in list, server online (-3: offline; -2: not found)
        if (rc.index==-2)
        {
            ostringstream out;
            out << HTML::kWhite << '\t' << state.state() << '\n';
            return out.str();
        }

        //ostringstream msg;
        //msg << HTML::kWhite << '\t' << rc.name << " [" << rc.index << "]\n";
        //return msg.str();

        if (rc.index<0)
            return HTML::kWhite + "\t&mdash;\n";

        string col = HTML::kGreen;
        if (rc.index<green)
            col = HTML::kYellow;
        if (rc.index>0xff)
            col = HTML::kRed;

        return col + '\t' + rc.name + '\n';
    }

    bool SetError(bool b, const string &err)
    {
        if (!b)
        {
            fErrorList.erase(err);
            return 0;
        }

        const bool isnew = fErrorList.insert(err).second;
        if (isnew)
            fErrorHist.add(err);

        return isnew;
    }

#ifdef HAVE_NOVA

    //vector<pair<Nova::EquPosn, double>> fMoonCoords;

    vector<Nova::SolarObjects> fCoordinates;

    void CalcCoordinates(double jd)
    {
        jd = floor(jd);

        fCoordinates.clear();
        for (double h=0; h<1; h+=1./(24*12))
            fCoordinates.emplace_back(jd+h);
    }

    pair<vector<float>, pair<Time, float>> GetVisibility(Nova::EquPosn *src=0)
    {
        const double sunset  = fSun.fSunSet12.JD()-1;
        const double sunrise = fSun.fSunRise12.JD();

        Nova::EquPosn  moon;
        Nova::EquPosn *pos = src ? src : &moon;

        double max   = 0;
        double maxjd = 0;

        int cnt = 0;

        vector<float> alt;
        for (auto it=fCoordinates.begin(); it!=fCoordinates.end(); it++)
        {
            if (src==0)
                moon = it->fMoonEqu;

            const Nova::HrzPosn hrz = Nova::GetHrzFromEqu(*pos, it->fJD);

            if (it->fJD>sunset && it->fJD<sunrise)
                alt.push_back(hrz.alt);

            if (hrz.alt>max)
            {
                max   = hrz.alt;
                maxjd = it->fJD;
            }

            if (it->fJD>sunset && it->fJD<sunrise && hrz.alt>15)
                cnt++;
        }

        if (max<=15 || cnt==0)
            return make_pair(vector<float>(), make_pair(Time(), 0));

        return make_pair(alt, make_pair(maxjd, maxjd>sunset&&maxjd<sunrise?max:0));
    }

    pair<vector<float>, pair<Time, float>> GetLightCondition(const Nova::EquPosn &src_pos)
    {
        const double sunset  = fSun.fSunSet12.JD()-1;
        const double sunrise = fSun.fSunRise12.JD();

        double max   = -1;
        double maxjd =  0;

        int cnt = 0;

        vector<float> vec;
        for (auto it=fCoordinates.begin(); it!=fCoordinates.end(); it++)
        {
            double cur = -1;

            if (it->fJD>sunset && it->fJD<sunrise)
            {
                cur = FACT::PredictI(*it, src_pos);
                vec.push_back(cur);
            }

            if (cur>max)
            {
                max   = cur;
                maxjd = it->fJD;
            }

            if (it->fJD>sunset && it->fJD<sunrise && cur>0)
                cnt++;
        }

        if (max<=0 || cnt==0)
            return make_pair(vector<float>(), make_pair(Time(), 0));

        return make_pair(vec, make_pair(maxjd, maxjd>sunset&&maxjd<sunrise?max:-1));
    }
#endif

    void UpdateAstronomy()
    {
        Time now;

#ifdef HAVE_NOVA
        CalcCoordinates(now.JD());
#endif

        fSun  = Sun (now);
        fMoon = Moon(now);

        vector<string> color(8, HTML::kWhite);
        color[fSun.state] = HTML::kBlue;

        ostringstream out;
        out << setprecision(3);
        out << now.JavaDate() << '\n';
        out << color[4] << '\t' << fSun.fSunRise18.GetAsStr("%H:%M") << '\n';
        out << color[5] << '\t' << fSun.fSunRise12.GetAsStr("%H:%M") << '\n';
        out << color[6] << '\t' << fSun.fSunRise06.GetAsStr("%H:%M") << '\n';
        out << color[7] << '\t' << fSun.fSunRise00.GetAsStr("%H:%M") << '\n';

        out << color[0] << '\t' << fSun.fSunSet00.GetAsStr("%H:%M") << '\n';
        out << color[1] << '\t' << fSun.fSunSet06.GetAsStr("%H:%M") << '\n';
        out << color[2] << '\t' << fSun.fSunSet12.GetAsStr("%H:%M") << '\n';
        out << color[3] << '\t' << fSun.fSunSet18.GetAsStr("%H:%M") << '\n';

        ofstream(fPath+"/sun.data") << out.str();

        color.assign(3, HTML::kWhite);
        color[fMoon.state%3] = HTML::kBlue;

        out.str("");
        out << now.JavaDate() << '\n';

        out << color[0] << '\t' << fMoon.fRise.GetAsStr("%H:%M") << '\n';
        out << color[1] << '\t' << fMoon.fTransit.GetAsStr("%H:%M") << '\n';
        out << color[2] << '\t' << fMoon.fSet.GetAsStr("%H:%M") << '\n';

        out << (fSun.isday?HTML::kWhite:fMoon.color) << '\t' << fMoon.description << '\n';

        if (!fMoon.visible)
            out << HTML::kWhite << "\t&mdash;\t\n";
        else
        {
            string col = HTML::kWhite;
            if (!fSun.isday)
            {
                col = HTML::kGreen;
                if (fMoon.zd>25)
                    col = HTML::kYellow;
                if (fMoon.zd>45 && fMoon.zd<80)
                    col = HTML::kRed;
                if (fMoon.zd>=80)
                    col = HTML::kRed;
            }
            out << col << '\t' << fMoon.zd << '\t' << GetDir(fMoon.az) << '\n';
        }

        ostringstream out2, out3, out4;
        out2 << setprecision(3);
        out2 << now.JavaDate() << '\n';
        out3 << now.JavaDate() << '\n';
        out4 << now.JavaDate() << '\n';

        struct Entry
        {
            string name;
            float value;
            int color;
            Entry(const string &n, float v, int c) : name(n), value(v), color(c%8) { }

            const string &Col() const
            {
                // If this list is updatd the number count in the constructor needs
                // to be updated, too
                static const string hcol[] = { "888", "8cf", "c8f", "bbb", "8fc", "cf8", "f8c", "fc8" };
                return hcol[color];
            }

            vector<float> GetColor(double scale, double offset=0) const
            {
                vector<float> rc(3);
                rc[0] = double(Col()[0])*scale/126+offset;
                rc[1] = double(Col()[1])*scale/126+offset;
                rc[2] = double(Col()[2])*scale/126+offset;
                return rc;
            }
        };

        multimap<Time, Entry> culmination;
        multimap<Time, Entry> lightcond;
        vector<vector<float>> alt;
        vector<vector<float>> cur;

#ifdef HAVE_NOVA
        int ccol = 0;
        int lcol = 0;

        /*const*/ pair<vector<float>, pair<Time, float>> vism = GetVisibility();
        if (!vism.first.empty())
        {
            const Entry entry("Moon", vism.second.second, ccol);
            culmination.insert(make_pair(vism.second.first, entry));
            const vector<float> col = entry.GetColor(75, 15);
            vism.first.insert(vism.first.begin(), col.begin(), col.end());
            alt.push_back(vism.first);

            ccol++;
        }
#endif

#ifdef HAVE_SQL
        try
        {
            const mysqlpp::StoreQueryResult res =
                Database(fDatabase).query("SELECT fSourceName, fRightAscension, fDeclination FROM Source WHERE fSourceTypeKEY=1").store();

            out  << HTML::kWhite << '\t';
            out2 << HTML::kWhite << '\t';
            out3 << HTML::kWhite << '\t';
            out4 << HTML::kWhite << '\t';

            for (vector<mysqlpp::Row>::const_iterator v=res.begin(); v<res.end(); v++)
            {
                const string name = (*v)[0].c_str();
                const double ra   = (*v)[1];
                const double dec  = (*v)[2];
#ifdef HAVE_NOVA
                Nova::EquPosn pos;
                pos.ra  = ra*15;
                pos.dec = dec;

                const Nova::ZdAzPosn hrz = Nova::GetHrzFromEqu(pos, now.JD());

                /*const*/ pair<vector<float>, pair<Time, float>> vis = GetVisibility(&pos);
                if (!vis.first.empty())
                {
                    const Entry entry(name, vis.second.second, ccol);
                    culmination.insert(make_pair(vis.second.first, entry));
                    const vector<float> col = entry.GetColor(75, 15);
                    vis.first.insert(vis.first.begin(), col.begin(), col.end());
                    alt.push_back(vis.first);

                    ccol++;

                    /*const*/ pair<vector<float>, pair<Time, float>> lc = GetLightCondition(pos);
                    if (!lc.first.empty())
                    {
                        const Entry entry2(name, lc.second.second, lcol);
                        lightcond.insert(make_pair(lc.second.first, entry2));
                        const vector<float> col2 = entry2.GetColor(100);
                        lc.first.insert(lc.first.begin(), col2.begin(), col2.end());
                        cur.push_back(lc.first);

                        lcol++;
                    }
                }

                string col = HTML::kWhite;
                if (hrz.zd<85)
                    col = HTML::kRed;
                if (hrz.zd<65)
                    col = HTML::kYellow;
                if (hrz.zd<30)
                    col = HTML::kGreen;

                out2 << "<tr bgcolor='" << col << "'>";
                out2 << "<td>" << name << "</td>";
                if (hrz.zd<85)
                {
                    out2 << "<td>" << hrz.zd << "&deg;</td>";
                    out2 << "<td>" << GetDir(hrz.az) << "</td>";
                }
                else
                    out2 << "<td/><td/>";
                out2 << "</tr>";
#endif
                const int32_t angle = fMoon.Angle(ra, dec);

                out << "<tr bgcolor='" << Moon::Color(angle) << "'>";
                out << "<td>" << name << "</td>";
                out << "<td>" << round(angle) << "&deg;</td>";
                out << "</tr>";
            }

            for (auto it=culmination.begin(); it!=culmination.end(); it++)
            {
                const Entry &e = it->second;
                if (it!=culmination.begin())
                    out3 << ", ";
                out3 << "<B#" << e.Col() << ">" << e.name << "</B>";
                if (e.value>0)
                    out3 << " [" << nearbyint(90-e.value) << "&deg;]";
            }

            out4 << setprecision(3);

            for (auto it=lightcond.begin(); it!=lightcond.end(); it++)
            {
                const Entry &e = it->second;
                if (it!=lightcond.begin())
                    out4 << ", ";
                out4 << "<B#" << e.Col() << ">" << e.name << "</B>";
                if (e.value>0)
                    out4 << " [" << nearbyint(e.value) << "]";
            }

            const Time st = fSun.fSunSet12;;
            const Time rs = fSun.fSunRise12;

            ostringstream title;
            title << st.GetAsStr("%H:%M");
            title << " / ";
            title << ((rs>st?rs-st:st-rs)/20).minutes();
            title << "' / ";
            title << rs.GetAsStr("%H:%M");

            out  << '\n';
            out2 << '\n';
            out3 << '\n';
            out4 << '\n';
            out  << HTML::kWhite << '\t' << Time()-now << '\n';
            out2 << HTML::kWhite << '\t' << Time()-now << '\n';

            WriteBinaryVec(now, "hist-visibility",         alt,  75, 15, "Alt "+title.str());
            WriteBinaryVec(now, "hist-current-prediction", cur, 100,  0, "I "  +title.str());
        }
        catch (const exception &e)
        {
            out  << '\n';
            out2 << '\n';
            out  << HTML::kWhite << '\t' << "ERROR  - "+string(e.what()) << '\n';
            out2 << HTML::kWhite << '\t' << "ERROR  - "+string(e.what()) << '\n';
            out3 << HTML::kWhite << '\t' << "ERROR  - "+string(e.what()) << '\n';
            out4 << HTML::kWhite << '\t' << "ERROR  - "+string(e.what()) << '\n';
        }
#endif

        ofstream(fPath+"/moon.data") << out.str();
        ofstream(fPath+"/source-list.data") << out2.str();
        ofstream(fPath+"/visibility.data") << out3.str();
        ofstream(fPath+"/current-prediction.data") << out4.str();
    }

    int Execute()
    {
        Time now;
        if (now-fLastUpdate<boost::posix_time::seconds(1))
            return fDimDNS.online() ? kStateRunning : kStateDimNetworkNA;
        fLastUpdate=now;

        // ==============================================================

        bool reqscript = false;

#ifdef HAVE_SQL
        try
        {
            const string query = Tools::Form("SELECT COUNT(*) FROM calendar.Data WHERE NOT u LIKE 'moon' AND y=%d AND m=%d AND d=%d",
                                             now.NightAsInt()/10000, (now.NightAsInt()/100)%100-1, now.NightAsInt()%100);

            const mysqlpp::StoreQueryResult res = Database(fDatabase).query(query).store();

            const uint32_t cnt = res[0][0];

            reqscript = cnt>0 && (fSun.state==3 || fSun.state==4);
        }
        catch (const exception &e)
        {
            Out() << e.what() << endl;
        }
#endif
        // ==============================================================

        struct statvfs vfs;
        statvfs("/newdata", &vfs);

        const uint64_t free_newdata = vfs.f_bsize*vfs.f_bavail;

        // ==============================================================

        const bool data_taking =
            fDimMcp.state()==MCP::State::kTriggerOn ||
            fDimMcp.state()==MCP::State::kTakingData;

        const bool data_run =
            fMcpConfigurationName=="data" ||
            fMcpConfigurationName=="data-rt";

        const bool bias_on =
            fDimBiasControl.state()==BIAS::State::kRamping     ||
            fDimBiasControl.state()==BIAS::State::kOverCurrent ||
            fDimBiasControl.state()==BIAS::State::kVoltageOn;

        const bool calibrated =
            fDimFeedback.state()>=Feedback::State::kCalibrated;

        const bool haderr = !fErrorList.empty();

        bool newerr = false;

        newerr |= SetError(!fDimDNS.online(),
                           "<b><#darkred>DIM network not available</#></b>");
        newerr |= SetError(!fDimControl.online(),
                           "<b>no dimctrl server available</b>");
        newerr |= SetError(fDimDataLogger.state()<20 || fDimDataLogger.state()>40,
                           "<b>datalogger not ready</b>");

        newerr |= SetError(fDimControl.state()!=3 && reqscript,
                           "<b>No script running during datataking time.</b>");

        //newerr |= SetError(fDimDriveControl.state()==Drive::State::kLocked,
        //                   "<b><#darkred>Drive in LOCKED state, drive was automatically parked</#></b>");

        newerr |= SetError(fDimDriveControl.state()>0xff && data_taking && data_run,
                           "Drive in ERROR state during data-run");
        newerr |= SetError(fDriveControlMoonDist>155,
                           "Moon within the field-of-view of the cones");
        newerr |= SetError(fDriveControlMoonDist>=0 && fDriveControlMoonDist<3,
                           "Moon within the field-of-view of the camera");

        newerr |= SetError(fDimBiasControl.state()<BIAS::State::kRamping && data_taking && data_run,
                           "BIAS not operating during data-run");
        newerr |= SetError(fDimBiasControl.state()==BIAS::State::kOverCurrent,
                           "BIAS channels in OverCurrent");
        newerr |= SetError(fDimBiasControl.state()==BIAS::State::kNotReferenced,
                           "BIAS voltage not at reference");

        newerr |= SetError(fDimFeedback.state()==Feedback::State::kOnStandby,
                           "Feedback in standby due to high currents");


        newerr |= SetError(bias_on && calibrated && fBiasControlCurrentMed>115,
                           "Median current (excl. crazy) exceeds 115&micro;A/pix");
        newerr |= SetError(bias_on && calibrated && fBiasControlCurrentMax>160,
                           "Maximum current (excl. crazy) exceeds 160&micro;A/pix");

        newerr |= SetError(fFscControlHumidityAvg>60,
                           "Average camera humidity exceed 60%");

        newerr |= SetError(!fPfMiniHumidityHist.empty() && fPfMiniHumidityHist.back()>50,
                           "Camera humidity inside camera exceeds 50% (PFmini)");
        newerr |= SetError(!fTemperatureControlHist.empty() && (fTemperatureControlHist.back()<18.0 || fTemperatureControlHist.back()>22.0),
                           "Container temperature outside [18.0;22.0]&deg;C");

        newerr |= SetError(!fMagicWeatherHist[kHum].empty() && fMagicWeatherHist[kHum].back()>98 && fDimLidControl.state()==Lid::State::kOpen,
                           "Outside humidity exceeds 98% while lid is open");
        newerr |= SetError(!fMagicWeatherHist[kGusts].empty() && fMagicWeatherHist[kGusts].back()>50 && (fDimDriveControl.state()==Drive::State::kTracking||fDimDriveControl.state()==Drive::State::kOnTrack),
                           "Wind gusts exceed 50km/h during tracking");

        newerr |= SetError(fDimFscControl.state()>=FSC::State::kConnected && !fFscControlTemperatureHist.empty() && fFscControlTemperatureHist.back()>15,
                           "Sensor temperature exceeds outside temperature by more than 15&deg;C");

        newerr |= SetError(fFtmControlTriggerRateTooLow>0,
                           "Trigger rate below 1Hz while trigger switched on");

        newerr |= SetError(fFtmControlState!=FTM::kFtmConfig && (fFtmControlState&FTM::kFtmLocked)==0,
                           "FTM - clock conditioner not locked!");

        newerr |= SetError(fDimTimeCheck.state()==1,
                           "Warning NTP time difference of drive PC exceeds 1s");
        newerr |= SetError(fDimTimeCheck.state()<1,
                           "Warning timecheck not running");

        newerr |= SetError(fDimBiasControl.state()==BIAS::State::kVoltageOn &&
                           fDimFeedback.state()<Feedback::State::kCalibrating &&
                           fBiasControlVoltageMed>3,
                           "Bias voltage switched on, but bias crate not calibrated");

        newerr |= SetError(fLastRunFinishedWithZeroEvents,
                           "Last run finshed, but contained zero events.");

        newerr |= SetError(fFreeSpace<uint64_t(50000000000),
                           "Less than 50GB disk space left on newdaq.");

        newerr |= SetError(free_newdata<uint64_t(800000000000),
                           "Less than 800GB disk space left on /newdata.");

        newerr |= SetError(fDimPwrControl.state()==Power::State::kCoolingFailure,
                           "Cooling unit reports failure!");

        for (auto it=fControlAlarmHist.begin(); it!=fControlAlarmHist.end(); it++)
            newerr |= SetError(it->time.IsValid(), it->msg);
        fControlAlarmHist.clean();;

        fLastRunFinishedWithZeroEvents = false;

        // FTM in Connected instead of Idle --> power cyclen

        /* // Check offline and disconnected status?
          Out() << fDimMcp          << endl;
          Out() << fDimControl      << endl;
          Out() << fDimDataLogger   << endl;
          Out() << fDimDriveControl << endl;
          Out() << fDimFadControl   << endl;
          Out() << fDimFtmControl   << endl;
          Out() << fDimBiasControl  << endl;
          Out() << fDimFeedback     << endl;
          Out() << fDimRateControl  << endl;
          Out() << fDimFscControl   << endl;
          Out() << fDimMagicWeather << endl;
          Out() << fDimRateScan     << endl;
          Out() << fDimChat         << endl;
          */

        // FTU in error
        // FAD lost

        // --------------------------------------------------------------
        ostringstream out;

        if (newerr)
        {
            SetAudio("error");

            out << now.JavaDate() << '\n';
            out << HTML::kWhite << '\t';
            out << "<->" << fErrorHist.rget() << "<->";
            out << '\n';

            ofstream(fPath+"/errorhist.data") << out.str();
        }

        out.str("");
        out << Header(now) << '\t' << (!fErrorList.empty()) << '\t' << (fDimControl.state()>0) << '\n';
        out << setprecision(3);
        out << HTML::kWhite << '\t';
        for (auto it=fErrorList.begin(); it!=fErrorList.end(); it++)
            out << *it << "<br/>";
        out << '\n';

        if (haderr || !fErrorList.empty())
            ofstream(fPath+"/error.data") << out.str();

        // ==============================================================

        out.str("");
        out << Header(now) << '\t' << (!fErrorList.empty()) << '\t' << (fDimControl.state()>0) << '\n';
        out << setprecision(3);

        // -------------- System status --------------
        if (fDimDNS.online() && fDimMcp.state()>=MCP::State::kIdle) // Idle
        {
            string col = HTML::kBlue;
            switch (fMcpConfigurationState)
            {
            case MCP::State::kIdle:
            case DimState::kOffline:
                col = HTML::kWhite;
                break;
            case MCP::State::kConfiguring1:
            case MCP::State::kConfiguring2:
            case MCP::State::kConfiguring3:
            case MCP::State::kConfigured:
            case MCP::State::kTriggerOn:
                col = HTML::kBlue;
                break;
            case MCP::State::kTakingData:
                col = HTML::kBlue;
                if (fDimFadControl.state()==FAD::State::kRunInProgress)
                    col = HTML::kGreen;
                break;
            }

            const bool other =
                fDimRateControl.state()==RateControl::State::kSettingGlobalThreshold ||
                fDimLidControl.state()==Lid::State::kMoving ||
                fDimRateScan.state()==RateScan::State::kInProgress;

            if (other)
                col = HTML::kBlue;

            out << col << '\t';

            if (!other)
            {
                const string conf = fMcpConfigurationName.length()>0?" ["+fMcpConfigurationName+"]":"";
                switch (fMcpConfigurationState)
                {
                case MCP::State::kIdle:
                    out << "Idle" << conf;
                    break;
                case MCP::State::kConfiguring1:
                case MCP::State::kConfiguring2:
                case MCP::State::kConfiguring3:
                    out << "Configuring" << conf;
                    break;
                case MCP::State::kConfigured:
                    out << "Configured" << conf;
                    break;
                case MCP::State::kTriggerOn:
                case MCP::State::kTakingData:
                    out << fMcpConfigurationName;
                    if (fFadControlDrsRuns[2]>0)
                        out << "(" << fFadControlDrsRuns[2] << ")";
                    break;

                case MCP::State::kCrateReset0:
                    out << "Crate reset phase 0";
                    break;
                case MCP::State::kCrateReset1:
                    out << "Crate reset phase 1";
                    break;
                case MCP::State::kCrateReset2:
                    out << "Crate reset phase 2";
                    break;
                case MCP::State::kCrateReset3:
                    out << "Crate reset phase 3";
                    break;
                }
            }
            else
                if (fDimRateControl.state()==RateControl::State::kSettingGlobalThreshold)
                    out << "Calibrating threshold";
                else
                    if (fDimRateScan.state()==RateScan::State::kInProgress)
                        out << "Rate scan in progress";
                    else
                        if (fDimLidControl.state()==Lid::State::kMoving)
                            out << "Lid moving";


            if (fMcpConfigurationState>MCP::State::kConfigured &&
                fDimRateControl.state()!=RateControl::State::kSettingGlobalThreshold)
            {
                ostringstream evt;
                if (fMcpConfigurationMaxEvents>0)
                {
                    const int64_t de = int64_t(fMcpConfigurationMaxEvents) - int64_t(fFadControlNumEvents);
                    if (de>=0 && fMcpConfigurationState==MCP::State::kTakingData)
                        evt << de;
                    else
                        evt << fMcpConfigurationMaxEvents;
                }
                else
                {
                    if (fMcpConfigurationState==MCP::State::kTakingData)
                    {
                        if (fFadControlNumEvents>2999)
                            evt << floor(fFadControlNumEvents/1000) << 'k';
                        else
                            evt << fFadControlNumEvents;
                    }
                }

                ostringstream tim;
                if (fMcpConfigurationMaxTime>0)
                {
                    const uint32_t dt = (Time()-fMcpConfigurationRunStart).total_seconds();
                    if (dt<=fMcpConfigurationMaxTime && fMcpConfigurationState==MCP::State::kTakingData)
                        tim << fMcpConfigurationMaxTime-dt << 's';
                    else
                        tim << fMcpConfigurationMaxTime << 's';
                }
                else
                {
                    if (fMcpConfigurationState==MCP::State::kTakingData)
                        tim << fMcpConfigurationRunStart.SecondsTo();
                }

                const bool has_evt = !evt.str().empty();
                const bool has_tim = !tim.str().empty();

                if (has_evt || has_tim)
                    out << " [";
                out << evt.str();
                if (has_evt && has_tim)
                    out << '/';
                out << tim.str();
                if (has_evt || has_tim)
                    out << ']';
            }
        }
        else
            out << HTML::kWhite;
        out << '\n';

        // ------------------ Drive -----------------
        if (fDimDNS.online() && fDimDriveControl.state()>=Drive::State::kInitialized)   // Armed, Moving, Tracking, OnTrack, Error
        {
            const uint32_t dev = !fDriveControlTrackingDevHist.empty() ? round(fDriveControlTrackingDevHist.back()) : 0;
            const State rc = fDimDriveControl.description();
            string col = HTML::kGreen;
            if (fDimDriveControl.state()==Drive::State::kInitialized)  // Armed
                col = HTML::kWhite;
            if (fDimDriveControl.state()>Drive::State::kInitialized && // Moving
                fDimDriveControl.state()<Drive::State::kTracking)
                col = HTML::kBlue;
            if (fDimDriveControl.state()==Drive::State::kTracking ||   // Tracking
                fDimDriveControl.state()==Drive::State::kOnTrack) 
            {
                if (dev>60)   // ~1.5mm
                    col = HTML::kYellow;
                if (dev>120)  // ~1/4 of a pixel ~ 2.5mm
                    col = HTML::kRed;
            }
            if (fDimDriveControl.state()>0xff)
                col = HTML::kRed;
            out << col << '\t';

            //out << rc.name << '\t';
            out << fDriveControlPointingAz << ' ';
            out << fDriveControlPointingZd  << "&deg;";
            out << setprecision(2);
            if (fDimDriveControl.state()==Drive::State::kTracking ||
                fDimDriveControl.state()==Drive::State::kOnTrack)      // Tracking
            {
                out << " &plusmn; " << dev << '"';
                if (!fDriveControlSourceName.empty())
                    out << " [" << fDriveControlSourceName  << ']';
            }
            if (fDimDriveControl.state()>Drive::State::kInitialized && // Moving
                fDimDriveControl.state()<Drive::State::kTracking)
                out << " &#10227;";
            out << setprecision(3);
        }
        else
            out << HTML::kWhite << '\t';

        if (fSun.time.IsValid() && fMoon.time.IsValid())
        {
            if (fSun.visible)
            {
                out << " &#9788;";
                if (fDimDriveControl.state()<Drive::State::kInitialized)
                    out << " [" << fSun.fSunSet12.MinutesTo() << "&darr;]";
            }
            else
                if (!fSun.visible && fMoon.visible)
                {
                    out << " &#9790;";
                    if (fDimDriveControl.state()<Drive::State::kInitialized)
                        out << " [" << fMoon.disk << "%]";
                }
        }
        if (fDimDNS.online() && fDimDriveControl.state()>0xff)
            out << " <ERR>";
        if (fDimDNS.online() && fDimDriveControl.state()==Drive::State::kLocked)
            out << " &otimes;";
        out << '\n';

        // ------------------- FSC ------------------
        if (fDimDNS.online() && fDimFscControl.state()>FSC::State::kDisconnected && !fFscControlTemperatureHist.empty())
        {
            string col = HTML::kGreen;
            if (fFscControlTemperatureHist.back()>9)
                col = HTML::kYellow;
            if (fFscControlTemperatureHist.back()>15)
                col = HTML::kRed;

            out << col << '\t' << fFscControlTemperatureHist.back() << '\n';
        }
        else
            out << HTML::kWhite << '\n';

        // --------------- MagicWeather -------------
        if (fDimDNS.online() && fDimMagicWeather.state()==MagicWeather::State::kReceiving && !fMagicWeatherHist[kWeatherBegin].empty())
        {
            /*
            const float diff = fMagicWeatherHist[kTemp].back()-fMagicWeatherHist[kDew].back();
            string col1 = HTML::kRed;
            if (diff>0.3)
                col1 = HTML::kYellow;
            if (diff>0.7)
                col1 = HTML::kGreen;
                */

            const float wind = fMagicWeatherHist[kGusts].back();
            const float hum  = fMagicWeatherHist[kHum].back();
            string col = HTML::kGreen;
            if (wind>35 || hum>95)
                col = HTML::kYellow;
            if (wind>45 || hum>98)
                col = HTML::kRed;

            if (fDimRainSensor.state()==RainSensor::State::kValid && !fRainSensorDataHist.empty() && fRainSensorDataHist.back()>0)
            {
                out << HTML::kRed << '\t';
                out << "RAIN" << '\t';
            }
            else
            {
                out << col << '\t';
                out << fMagicWeatherHist[kHum].back() << '\t';
            }
            out << setprecision(2);
            out << fMagicWeatherHist[kGusts].back() << '\n';
            out << setprecision(3);
        }
        else
            out << HTML::kWhite << "\n";

        // --------------- FtmControl -------------
        if (fDimDNS.online() && fDimFtmControl.state()==FTM::State::kTriggerOn)
        {
            string col = HTML::kGreen;
            if (!fFtmControlTriggerRateHist.empty())
            {
                if (fFtmControlTriggerRateHist.back()<15)
                    col = HTML::kYellow;
                if (fFtmControlTriggerRateHist.back()>100)
                    col = HTML::kRed;

                out << col << '\t' << fFtmControlTriggerRateHist.back() << " Hz";
            }

            if (bias_on)
                out << " (" << setprecision(4) << fFtmPatchThresholdMed << ')';
            out << '\n';
        }
        else
            out << HTML::kWhite << '\n';

        // --------------- BiasControl -------------
        const bool bias_off = fDimBiasControl.state()==BIAS::State::kVoltageOff;
        const bool bias_oc  = fDimBiasControl.state()==BIAS::State::kOverCurrent;

        if (fDimDNS.online() && (bias_on || bias_off))
        {

            string col = fBiasControlVoltageMed>3?HTML::kGreen:HTML::kWhite;
            if (bias_on)
            {
                if (fBiasControlCurrentMed>95 || fBiasControlCurrentMax>135)
                    col = HTML::kYellow;
                if (fBiasControlCurrentMed>100 || fBiasControlCurrentMax>140)
                    col = HTML::kRed;
            }

            // Bias in overcurrent => Red
            if (bias_oc)
                col = HTML::kRed;

            // MCP in ReadyForDatataking/Configuring/Configured/TriggerOn/TakingData
            // and Bias not in "data-taking state' => Red
            if (fMcpConfigurationState>MCP::State::kIdle && !bias_on)
                col = HTML::kWhite;

            const bool cal = fDimFeedback.state()>=Feedback::State::kCalibrated;

            // Feedback is currently calibrating => Blue
            if (fDimFeedback.state()==Feedback::State::kCalibrating)
            {
                out << HTML::kBlue << '\t';
                out << "***\t";
                out << "***\t";
            }
            else
            {
                out << col << '\t';
                out << setprecision(fBiasControlCurrentMed<100?2:3);
                out << (bias_off ? 0 : (fBiasControlCurrentMed<10?fBiasControlCurrentMed:floor(fBiasControlCurrentMed))) << '\t';
                if (bias_oc)
                    out << "(OC) ";
                else
                {
                    if (cal)
                    {
                        out << setprecision(fBiasControlCurrentMax<100?2:3);
                        out << (bias_off ? 0 : (fBiasControlCurrentMax<10?fBiasControlCurrentMax:floor(fBiasControlCurrentMax)));
                    }
                    else
                        out << "&mdash; ";
                }
                out << '\t';
            }
            if (cal && fDimFeedback.state()!=Feedback::State::kCalibrating)
                out << setprecision(2) << fBiasControlPowerTot << " W";
            else
                out << setprecision(3) << (bias_off ? 0 : fBiasControlVoltageMed) << " V";
            out << '\n';
        }
        else
            out << HTML::kWhite << '\n';

        ofstream(fPath+"/fact.data") << out.str();

        // ==============================================================

        out.str("");
        out << Header(now) << '\t' << (!fErrorList.empty()) << '\t' << (fDimControl.state()>0) << '\n';

        if (!fDimDNS.online())
            out << HTML::kWhite << "\tOffline\n\n\n\n\n\n\n\n\n\n\n\n\n";
        else
        {
            ostringstream dt;
            dt << (Time()-fRunTime);

            out << HTML::kGreen << '\t' << fDimDNS.version() << '\n';

            out << GetStateHtml(fDimControl,        0);
            out << GetStateHtml(fDimMcp,            MCP::State::kConnected);
            out << GetStateHtml(fDimDataLogger,     1);
            out << GetStateHtml(fDimDriveControl,   Drive::State::kConnected);
            out << GetStateHtml(fDimTimeCheck,      1);
            out << GetStateHtml(fDimFadControl,     FAD::State::kConnected);
            out << GetStateHtml(fDimFtmControl,     FTM::State::kConnected);
            out << GetStateHtml(fDimBiasControl,    BIAS::State::kConnected);
            out << GetStateHtml(fDimFeedback,       Feedback::State::kConnected);
            out << GetStateHtml(fDimRateControl,    RateControl::State::kConnected);
            out << GetStateHtml(fDimFscControl,     FSC::State::kConnected);
            out << GetStateHtml(fDimPfMiniControl,  PFmini::State::kConnected);
            out << GetStateHtml(fDimBiasTemp,       BiasTemp::State::kConnected);
            out << GetStateHtml(fDimGpsControl,     GPS::State::kConnected);
            out << GetStateHtml(fDimSqmControl,     SQM::State::kConnected);
            out << GetStateHtml(fDimAgilentControl24, Agilent::State::kVoltageOff);
            out << GetStateHtml(fDimAgilentControl50, Agilent::State::kVoltageOff);
            out << GetStateHtml(fDimAgilentControl80, Agilent::State::kVoltageOff);
            out << GetStateHtml(fDimPwrControl,     Power::State::kSystemOff);
            out << GetStateHtml(fDimLidControl,     Lid::State::kConnected);
            out << GetStateHtml(fDimRateScan,       RateScan::State::kConnected);
            out << GetStateHtml(fDimMagicWeather,   MagicWeather::State::kConnected);
            out << GetStateHtml(fDimTngWeather,     TNGWeather::State::kConnected);
            out << GetStateHtml(fDimGtcDust,        GTC::State::kReceiving);
            out << GetStateHtml(fDimMagicLidar,     MagicLidar::State::kConnected);
            out << GetStateHtml(fDimTemperature,    Temperature::State::kValid);
            out << GetStateHtml(fDimRainSensor,     RainSensor::State::kValid);
            out << GetStateHtml(fDimChat,           0);
            out << GetStateHtml(fDimSkypeClient,    1);

            string col = HTML::kRed;
            if (fFreeSpace>uint64_t(199999999999))
                col = HTML::kYellow;
            if (fFreeSpace>uint64_t(999999999999))
                col = HTML::kGreen;
            if (fFreeSpace==UINT64_MAX)
                col = HTML::kWhite;

            out << col << '\t' << Tools::Scientific(fFreeSpace) << "B\n";

            col = HTML::kRed;
            if (free_newdata > uint64_t(999999999999))
                col = HTML::kYellow;
            if (free_newdata > uint64_t(149999999999))
                col = HTML::kGreen;
            if (free_newdata == UINT64_MAX)
                col = HTML::kWhite;

            out << col << '\t' << Tools::Scientific(free_newdata) << "B\n";

            out << HTML::kGreen << '\t' << dt.str().substr(0, dt.str().length()-7) << '\n';
        }

        ofstream(fPath+"/status.data") << out.str();

        if (now-fLastAstroCalc>boost::posix_time::seconds(15))
        {
            UpdateAstronomy();
            fLastAstroCalc = now;
        }

        return fDimDNS.online() ? kStateRunning : kStateDimNetworkNA;
    }


public:
    StateMachineSmartFACT(ostream &out=cout) : StateMachineDim(out, fIsServer?"SMART_FACT":""),
        fLastAstroCalc(boost::date_time::neg_infin),
        fPath("www/smartfact/data"),
        fMcpConfigurationState(DimState::kOffline),
        fMcpConfigurationMaxTime(0),
        fMcpConfigurationMaxEvents(0),
        fLastRunFinishedWithZeroEvents(false),
        fTngWeatherDustTime(Time::none),
        fGtcDustTime(Time::none),
        fBiasControlVoltageMed(0),
        fBiasControlCurrentMed(0),
        fBiasControlCurrentMax(0),
        fFscControlHumidityAvg(0),
        fDriveControlMoonDist(-1),
        fFadControlNumEvents(0),
        fFadControlDrsRuns(3),
        fFtmControlState(FTM::kFtmLocked),
        fRateScanDataId(0),
        fRateScanBoard(0),
        fFreeSpace(UINT64_MAX),
        // ---
        fDimMcp           ("MCP"),
        fDimDataLogger    ("DATA_LOGGER"),
        fDimDriveControl  ("DRIVE_CONTROL"),
        fDimTimeCheck     ("TIME_CHECK"),
        fDimMagicWeather  ("MAGIC_WEATHER"),
        fDimMagicLidar    ("MAGIC_LIDAR"),
        fDimTngWeather    ("TNG_WEATHER"),
        fDimGtcDust       ("GTC_DUST"),
        fDimTemperature   ("TEMPERATURE"),
        fDimRainSensor    ("RAIN_SENSOR"),
        fDimFeedback      ("FEEDBACK"),
        fDimBiasControl   ("BIAS_CONTROL"),
        fDimFtmControl    ("FTM_CONTROL"),
        fDimFadControl    ("FAD_CONTROL"),
        fDimFscControl    ("FSC_CONTROL"),
        fDimPfMiniControl ("PFMINI_CONTROL"),
        fDimBiasTemp      ("BIAS_TEMP"),
        fDimGpsControl    ("GPS_CONTROL"),
        fDimSqmControl    ("SQM_CONTROL"),
        fDimAgilentControl24("AGILENT_CONTROL_24V"),
        fDimAgilentControl50("AGILENT_CONTROL_50V"),
        fDimAgilentControl80("AGILENT_CONTROL_80V"),
        fDimPwrControl    ("PWR_CONTROL"),
        fDimLidControl    ("LID_CONTROL"),
        fDimRateControl   ("RATE_CONTROL"),
        fDimRateScan      ("RATE_SCAN"),
        fDimChat          ("CHAT"),
        fDimSkypeClient   ("SKYPE_CLIENT")
    {
        fDimDNS.Subscribe(*this);
        fDimControl.Subscribe(*this);
        fDimMcp.Subscribe(*this);
        fDimDataLogger.Subscribe(*this);
        fDimDriveControl.Subscribe(*this);
        fDimTimeCheck.Subscribe(*this);
        fDimMagicWeather.Subscribe(*this);
        fDimMagicLidar.Subscribe(*this);
        fDimTngWeather.Subscribe(*this);
        fDimGtcDust.Subscribe(*this);
        fDimTemperature.Subscribe(*this);
        fDimRainSensor.Subscribe(*this);
        fDimFeedback.Subscribe(*this);
        fDimBiasControl.Subscribe(*this);
        fDimFtmControl.Subscribe(*this);
        fDimFadControl.Subscribe(*this);
        fDimFscControl.Subscribe(*this);
        fDimPfMiniControl.Subscribe(*this);
        fDimBiasTemp.Subscribe(*this);
        fDimGpsControl.Subscribe(*this);
        fDimSqmControl.Subscribe(*this);
        fDimAgilentControl24.Subscribe(*this);
        fDimAgilentControl50.Subscribe(*this);
        fDimAgilentControl80.Subscribe(*this);
        fDimPwrControl.Subscribe(*this);
        fDimLidControl.Subscribe(*this);
        fDimRateControl.Subscribe(*this);
        fDimRateScan.Subscribe(*this);
        fDimChat.Subscribe(*this);
        fDimSkypeClient.Subscribe(*this);

        fDimFscControl.SetCallback(bind(&StateMachineSmartFACT::HandleFscControlStateChange, this, placeholders::_1));
        //fDimFtmControl.SetCallback(bind(&StateMachineSmartFACT::HandleFtmControlStateChange, this));
        fDimDriveControl.SetCallback(bind(&StateMachineSmartFACT::HandleDriveControlStateChange, this, placeholders::_1));
        fDimControl.SetCallback(bind(&StateMachineSmartFACT::HandleControlStateChange, this, placeholders::_1));
        fDimControl.AddCallback("dotest.dim", bind(&StateMachineSmartFACT::HandleDoTest, this, placeholders::_1));

        Subscribe("DIM_CONTROL/MESSAGE")
            (bind(&StateMachineSmartFACT::HandleDimControlMessage,   this, placeholders::_1));

        Subscribe("MCP/CONFIGURATION")
            (bind(&StateMachineSmartFACT::HandleMcpConfiguration,    this, placeholders::_1));

        Subscribe("DRIVE_CONTROL/POINTING_POSITION")
            (bind(&StateMachineSmartFACT::HandleDrivePointing,       this, placeholders::_1));
        Subscribe("DRIVE_CONTROL/TRACKING_POSITION")
            (bind(&StateMachineSmartFACT::HandleDriveTracking,       this, placeholders::_1));
        Subscribe("DRIVE_CONTROL/SOURCE_POSITION")
            (bind(&StateMachineSmartFACT::HandleDriveSource,         this, placeholders::_1));

        Subscribe("FSC_CONTROL/TEMPERATURE")
            (bind(&StateMachineSmartFACT::HandleFscTemperature,      this, placeholders::_1));
        Subscribe("FSC_CONTROL/HUMIDITY")
            (bind(&StateMachineSmartFACT::HandleFscHumidity,         this, placeholders::_1));
        Subscribe("FSC_CONTROL/BIAS_TEMP")
            (bind(&StateMachineSmartFACT::HandleFscBiasTemp,         this, placeholders::_1));

        Subscribe("PFMINI_CONTROL/DATA")
            (bind(&StateMachineSmartFACT::HandlePfMiniData,          this, placeholders::_1));

        Subscribe("BIAS_TEMP/DATA")
            (bind(&StateMachineSmartFACT::HandleBiasTemp,            this, placeholders::_1));

        Subscribe("GPS_CONTROL/NEMA")
            (bind(&StateMachineSmartFACT::HandleGpsNema,             this, placeholders::_1));

        Subscribe("SQM_CONTROL/DATA")
            (bind(&StateMachineSmartFACT::HandleSqmData,             this, placeholders::_1));

        Subscribe("TEMPERATURE/DATA")
            (bind(&StateMachineSmartFACT::HandleTemperatureData,     this, placeholders::_1));

        Subscribe("AGILENT_CONTROL_24V/DATA")
            (bind(&StateMachineSmartFACT::HandleAgilentData,         this, placeholders::_1, "24"));
        Subscribe("AGILENT_CONTROL_50V/DATA")
            (bind(&StateMachineSmartFACT::HandleAgilentData,         this, placeholders::_1, "50"));
        Subscribe("AGILENT_CONTROL_80V/DATA")
            (bind(&StateMachineSmartFACT::HandleAgilentData,         this, placeholders::_1, "80"));

        Subscribe("MAGIC_WEATHER/DATA")
            (bind(&StateMachineSmartFACT::HandleMagicWeatherData,    this, placeholders::_1));
        Subscribe("TNG_WEATHER/DUST")
            (bind(&StateMachineSmartFACT::HandleTngWeatherDust,      this, placeholders::_1));
        Subscribe("TNG_WEATHER/DATA")
            (bind(&StateMachineSmartFACT::HandleTngWeatherData,      this, placeholders::_1));
        Subscribe("GTC_DUST/DATA")
            (bind(&StateMachineSmartFACT::HandleGtcDustData,         this, placeholders::_1));
        Subscribe("RAIN_SENSOR/DATA")
            (bind(&StateMachineSmartFACT::HandleRainSensorData,      this, placeholders::_1));

        Subscribe("FEEDBACK/CALIBRATED_CURRENTS")
            (bind(&StateMachineSmartFACT::HandleFeedbackCalibratedCurrents, this, placeholders::_1));

        Subscribe("BIAS_CONTROL/VOLTAGE")
            (bind(&StateMachineSmartFACT::HandleBiasVoltage,         this, placeholders::_1));
        Subscribe("BIAS_CONTROL/CURRENT")
            (bind(&StateMachineSmartFACT::HandleBiasCurrent,         this, placeholders::_1));

        Subscribe("FAD_CONTROL/CONNECTIONS")
            (bind(&StateMachineSmartFACT::HandleFadConnections,      this, placeholders::_1));
        Subscribe("FAD_CONTROL/EVENTS")
            (bind(&StateMachineSmartFACT::HandleFadEvents,           this, placeholders::_1));
        Subscribe("FAD_CONTROL/START_RUN")
            (bind(&StateMachineSmartFACT::HandleFadStartRun,         this, placeholders::_1));
        Subscribe("FAD_CONTROL/DRS_RUNS")
            (bind(&StateMachineSmartFACT::HandleFadDrsRuns,          this, placeholders::_1));
        Subscribe("FAD_CONTROL/EVENT_DATA")
            (bind(&StateMachineSmartFACT::HandleFadEventData,        this, placeholders::_1));
        Subscribe("FAD_CONTROL/STATS")
            (bind(&StateMachineSmartFACT::HandleStats,               this, placeholders::_1));

        Subscribe("DATA_LOGGER/STATS")
            (bind(&StateMachineSmartFACT::HandleStats,               this, placeholders::_1));

        Subscribe("FTM_CONTROL/TRIGGER_RATES")
            (bind(&StateMachineSmartFACT::HandleFtmTriggerRates,     this, placeholders::_1));
        Subscribe("FTM_CONTROL/STATIC_DATA")
            (bind(&StateMachineSmartFACT::HandleFtmStaticData,       this, placeholders::_1));
        Subscribe("FTM_CONTROL/FTU_LIST")
            (bind(&StateMachineSmartFACT::HandleFtmFtuList,          this, placeholders::_1));

        Subscribe("RATE_CONTROL/THRESHOLD")
            (bind(&StateMachineSmartFACT::HandleRateControlThreshold,this, placeholders::_1));

        Subscribe("RATE_SCAN/DATA")
            (bind(&StateMachineSmartFACT::HandleRateScanData,        this, placeholders::_1));

        Subscribe("CHAT/MESSAGE")
            (bind(&StateMachineSmartFACT::HandleChatMsg,             this, placeholders::_1));


        // =================================================================

        // State names
        AddStateName(kStateDimNetworkNA, "DimNetworkNotAvailable",
                     "The Dim DNS is not reachable.");

        AddStateName(kStateRunning, "Running", "");

        // =================================================================

        AddEvent("PRINT")
            (bind(&StateMachineSmartFACT::Print, this))
            ("Print a list of the states of all connected servers.");

    }
    int EvalOptions(Configuration &conf)
    {
        if (!fPixelMap.Read(conf.GetPrefixedString("pixel-map-file")))
        {
            Error("Reading mapping table from "+conf.Get<string>("pixel-map-file")+" failed.");
            return 1;
        }

        fPath     = conf.Get<string>("path");
        fDatabase = conf.Get<string>("source-database");

        struct stat st;
        if (stat(fPath.c_str(), &st))
        {
            Error(fPath+" does not exist!");
            return 2;
        }

        if ((st.st_mode&S_IFDIR)==0)
        {
            Error(fPath+" not a directory!");
            return 3;
        }

        if ((st.st_mode&S_IWUSR)==0)
        {
            Error(fPath+" has no write permission!");
            return 4;
        }

        if ((st.st_mode&S_IXUSR)==0)
        {
            Error(fPath+" has no execute permission!");
            return 5;
        }

        ostringstream out;
        out << Time().JavaDate() << '\n';

        ofstream(fPath+"/error.data") << out.str();

        return -1;
    }
};

bool StateMachineSmartFACT::fIsServer = false;

// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    StateMachineSmartFACT::fIsServer = !conf.Get<bool>("client");
    return Main::execute<T, StateMachineSmartFACT>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Smart FACT");
    control.add_options()
        ("pixel-map-file",  var<string>()->required(),     "Pixel mapping file. Used here to get the default reference voltage")
        ("path",            var<string>("www/smartfact/data"), "Output path for the data-files")
        ("source-database", var<string>(""), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("client",          po_bool(false), "For a standalone client choose this option.")
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
        "SmartFACT is a tool writing the files needed for the SmartFACT web interface.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: smartfact [-c type] [OPTIONS]\n"
        "  or:  smartfact [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineSmartFACT>();

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
