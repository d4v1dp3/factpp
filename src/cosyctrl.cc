#include <boost/regex.hpp>

#ifdef HAVE_SQL
#include "Database.h"
#endif

#include "FACT.h"
#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Timers.h"
#include "Console.h"

#include "HeadersDrive.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace Drive;

// ------------------------------------------------------------------------

class ConnectionDrive : public Connection
{
    int  fState;

    bool fIsVerbose;

    // --verbose
    // --hex-out
    // --dynamic-out
    // --load-file
    // --leds
    // --trigger-interval
    // --physcis-coincidence
    // --calib-coincidence
    // --physcis-window
    // --physcis-window
    // --trigger-delay
    // --time-marker-delay
    // --dead-time
    // --clock-conditioner-r0
    // --clock-conditioner-r1
    // --clock-conditioner-r8
    // --clock-conditioner-r9
    // --clock-conditioner-r11
    // --clock-conditioner-r13
    // --clock-conditioner-r14
    // --clock-conditioner-r15
    // ...

    virtual void UpdatePointing(const Time &, const array<double, 2> &)
    {
    }

    virtual void UpdateTracking(const Time &, const array<double, 8> &)
    {
    }

    virtual void UpdateStatus(const Time &, const array<uint8_t, 3> &)
    {
    }

    virtual void UpdateStarguider(const Time &, const DimStarguider &)
    {
    }

    virtual void UpdateTPoint(const Time &, const DimTPoint &, const string &)
    {
    }

public:
    virtual void UpdateSource(const string & = "", bool = false)
    {
    }
    virtual void UpdateSource(const array<double, 6> &, const string& = "")
    {
    }

protected:
    map<uint16_t, int> fCounter;

    ba::streambuf fBuffer;

public:
    static Time ReadTime(istream &in)
    {
        uint16_t y, m, d, hh, mm, ss, ms;
        in >> y >> m >> d >> hh >> mm >> ss >> ms;

        return Time(y, m, d, hh, mm, ss, ms*1000);
    }

    static double ReadAngle(istream &in)
    {
        char     sgn;
        uint16_t d, m;
        float    s;

        in >> sgn >> d >> m >> s;

        const double ret = ((60.0 * (60.0 * (double)d + (double)m) + s))/3600.;
        return sgn=='-' ? -ret : ret;
    }

    double GetDevAbs(double nomzd, double meszd, double devaz)
    {
        nomzd *= M_PI/180;
        meszd *= M_PI/180;
        devaz *= M_PI/180;

        const double x = sin(meszd) * sin(nomzd) * cos(devaz);
        const double y = cos(meszd) * cos(nomzd);

        return acos(x + y) * 180/M_PI;
    }

    uint16_t fDeviationLimit;
    uint16_t fDeviationCounter;
    uint16_t fDeviationMax;

    vector<double> fDevBuffer;
    uint64_t       fDevCount;

    uint64_t fTrackingCounter;

    void ProcessDriveStatus(const string &line)
    {
        Message(line);
    }

    bool ProcessStargReport(const string &line)
    {
        istringstream stream(line);

        // 0: Error
        // 1: Standby
        // 2: Monitoring
        uint16_t status1;
        stream >> status1;
        /*const Time t1 = */ReadTime(stream);

        uint16_t status2;
        stream >> status2;
        /*const Time t2 = */ReadTime(stream);

        double misszd, missaz;
        stream >> misszd >> missaz;

        const double zd = ReadAngle(stream);
        const double az = ReadAngle(stream);

        double cx, cy;
        stream >> cx >> cy;

        int ncor;
        stream >> ncor;

        double bright, mjd;
        stream >> bright >> mjd;

        int nled, nring, nstars;
        stream >> nled >> nring >> nstars;

        if (stream.fail())
            return false;

        DimStarguider data;

        data.fMissZd = misszd;
        data.fMissAz = missaz;
        data.fNominalZd = zd;
        data.fNominalAz = az;
        data.fCenterX = cx;
        data.fCenterY = cy;
        data.fNumCorrelated = ncor;
        data.fBrightness = bright;
        data.fNumLeds = nled;
        data.fNumRings = nring;
        data.fNumStars = nstars;

        UpdateStarguider(Time(mjd), data);

        return true;
    }

    bool ProcessTpointReport(const string &line)
    {
        istringstream stream(line);

        uint16_t status1;
        stream >> status1;
        const Time t1 = ReadTime(stream);

        uint16_t status2;
        stream >> status2;
        /*const Time t2 =*/ ReadTime(stream);

        char type;
        stream >> type;
        if (type != 'T')
            return false;

        double az1, alt1, az2, alt2, ra, dec, dzd, daz;
        stream >> az1 >> alt1 >> az2 >> alt2 >> ra >> dec >> dzd >> daz;

        // c: center, s:start
        double mjd, cmag, smag, cx, cy, sx, sy;
        stream >> mjd >> cmag >> smag >> cx >> cy >> sx >> sy;

        int nled, nring, nstar, ncor;
        stream >> nled >> nring >> nstar >> ncor;

        double bright, mag;
        stream >> bright >> mag;

        string name;
        stream >> name;

        if (stream.fail())
            return false;

        DimTPoint tpoint;

        tpoint.fRa         = ra;
        tpoint.fDec        = dec;

        tpoint.fNominalZd  = 90-alt1-dzd;
        tpoint.fNominalAz  = az1 +daz;

        tpoint.fPointingZd = 90-alt1;
        tpoint.fPointingAz = az1;

        tpoint.fFeedbackZd = 90-alt2;
        tpoint.fFeedbackAz = az2;

        tpoint.fNumLeds    = nled;
        tpoint.fNumRings   = nring;

        tpoint.fCenterX    = cx;
        tpoint.fCenterY    = cy;
        tpoint.fCenterMag  = cmag;

        tpoint.fStarX      = sx;
        tpoint.fStarY      = sy;
        tpoint.fStarMag    = smag;

        tpoint.fRealMag    = mag;

        UpdateTPoint(t1, tpoint, name);

        return true;
    }

    bool ProcessDriveReport(const string &line)
    {
        // DRIVE-REPORT M1
        // 01 2011 05 14 11 31 19 038
        // 02 1858 11 17 00 00 00 000
        // + 000 00 000 + 000 00 000
        // + 000 00 000
        // 55695.480081
        // + 000 00 000 + 000 00 000
        // + 000 00 000 + 000 00 000
        // 0000.000 0000.000
        // 0 2

        // status
        // year month day hour minute seconds millisec
        // year month day hour minute seconds millisec
        // ra(+ h m s) dec(+ d m s) ha(+ h m s)
        // mjd
        // zd(+ d m s) az(+ d m s)
        // zd(+ d m s) az(+ d m s)
        // zd_err az_err
        // armed(0=unlocked, 1=locked)
        // stgmd(0=none, 1=starguider, 2=starguider off)
        istringstream stream(line);

        uint16_t status1;
        stream >> status1;
        const Time t1 = ReadTime(stream);

        uint16_t status2;
        stream >> status2;
        /*const Time t2 =*/ ReadTime(stream);

        const double ra  = ReadAngle(stream);
        const double dec = ReadAngle(stream);
        const double ha  = ReadAngle(stream);

        double mjd;
        stream >> mjd;

        const double zd1 = ReadAngle(stream);  // Nominal (zd/az asynchronous, dev  synchronous, mjd  synchronous with zd)
        const double az1 = ReadAngle(stream);  // Nominal (zd/az asynchronous, dev  synchronous, mjd  synchronous with z)
        const double zd2 = ReadAngle(stream);  // Masured (zd/az  synchronous, dev asynchronous, mjd asynchronous)
        const double az2 = ReadAngle(stream);  // Measurd (zd/az  synchronous, dev asynchronous, mjd asynchronous)

        double zd_err, az_err;
        stream >> zd_err;                      // Deviation = Nominal - Measured
        stream >> az_err;                      // Deviation = Nominal - Measured

        uint16_t armed, stgmd;
        stream >> armed;
        stream >> stgmd;

        uint32_t pdo3;
        stream >> hex >> pdo3;

        if (stream.fail())
            return false;

        // Status 0: Error
        // Status 1: Stopped
        // Status 3: Stopping || Moving
        // Status 4: Tracking
        if (status1==0)
            status1 = StateMachineImp::kSM_Error - Drive::State::kNotReady;

        const bool ready = (pdo3&0xef00ef)==0xef00ef;
        if (!ready)
            fState = Drive::State::kNotReady;
        else
            fState = status1==1 ?
                Drive::State::kReady+armed :
                Drive::State::kNotReady+status1;

        // kDisconnected = 1,
        // kConnected,
        // kNotReady,
        // kReady,
        // kArmed,
        // kMoving,
        // kTracking,
        // kOnTrack,

        // pdo3:
        //   1 Ab
        //   2 1
        //   4 Emergency
        //   8 OverVolt
        //  10 Move (Drehen-soll)
        //  20 Af
        //  40 1
        //  80 Power on Az
        // ------------------
        // 100 NOT UPS Alarm
        // 200 UPS on Battery
        // 400 UPS charging

        // Power cut: 2ef02ef
        // charging:  4ef04ef

        // Convert to deg
        zd_err /= 3600;
        az_err /= 3600;

        // Calculate absolut deviation on the sky

        const double dev = GetDevAbs(zd1, zd1-zd_err, az_err)*3600;

        fDevBuffer[fDevCount++%5] = dev;

        const uint8_t cnt    = fDevCount<5 ? fDevCount : 5;
        const double  avgdev = accumulate(fDevBuffer.begin(), fDevBuffer.begin()+cnt, 0)/cnt;

        // If any other state than tracking or a deviation
        // larger than 60, reset the counter
        if (fState!=State::kTracking || avgdev>fDeviationLimit)
            fTrackingCounter = 0;
        else
            fTrackingCounter++;

        // If in tracking, at least five consecutive reports (5s)
        // must be below 60arcsec deviation, this is considered OnTrack
        if (fState==State::kTracking && fTrackingCounter>=fDeviationCounter)
            fState = State::kOnTrack;

        // Having th state as Tracking will reset the counter
        if (fState==State::kOnTrack && avgdev>fDeviationMax)
            fState = State::kTracking;

        if (fState!=State::kTracking && fState!=State::kOnTrack)
            fDevCount = 0;

        // 206 206         ce ce       pwr vlt emcy fs        |  pwr vlt emcy fs
        // 239 239         ef ef       pwr vlt emcy fs bb rf  |  pwr vlt emcy fs bb rf
        // 111  78         6f 4e           vlt emcy fs bb rf  |          emcy fs bb

        /*
        fArmed     = data[3]&0x01; // armed status
        fPosActive = data[3]&0x02; // positioning active
        fRpmActive = data[3]&0x04; // RPM mode switched on
                  // data[3]&0x08; //  - unused -
                  // data[3]&0x10; //  - unused -
                  // data[3]&0x20; //  - unused -
        //fInControl = data[3]&0x40; // motor uncontrolled
                  // data[3]&0x80; // axis resetted (after errclr, motor stop, motor on)

        fStatus = data[3];
    }

    const LWORD_t stat = data[0] | (data[1]<<8);
    if (fStatusPdo3!=stat)
    {
        gLog << inf << MTime(-1) << ": " << GetNodeName() << " - PDO3(0x" << hex << (int)stat << dec << ") = ";
        const Bool_t ready  = stat&0x001;
        const Bool_t fuse   = stat&0x002;
        const Bool_t emcy   = stat&0x004;
        const Bool_t vltg   = stat&0x008;
        const Bool_t mode   = stat&0x010;
        const Bool_t rf     = stat&0x020;
        const Bool_t brake  = stat&0x040;
        const Bool_t power  = stat&0x080;
        const Bool_t alarm  = stat&0x100;  // UPS Alarm      (FACT only)
        const Bool_t batt   = stat&0x200;  // UPS on battery (FACT only)
        const Bool_t charge = stat&0x400;  // UPS charging   (FACT only)
        if (ready)  gLog << "DKC-Ready ";
        if (fuse)   gLog << "FuseOk ";
        if (emcy)   gLog << "EmcyOk ";
        if (vltg)   gLog << "OvervoltOk ";
        if (mode)   gLog << "SwitchToManualMode ";
        if (rf)     gLog << "RF ";
        if (brake)  gLog << "BrakeOpen ";
        if (power)  gLog << "PowerOn ";
        if (alarm)  gLog << "UPS-PowerLoss ";
        if (batt)   gLog << "UPS-OnBattery ";
        if (charge) gLog << "UPS-Charging ";
        gLog << endl;

        fStatusPdo3 = stat;
        }*/

        // ((stat1&0xffff)<<16)|(stat2&0xffff)
                                                                              // no alarm, no batt, no charge
        const array<uint8_t, 3> state = {{ uint8_t(pdo3>>16), uint8_t(pdo3), uint8_t(pdo3>>24) }};
        UpdateStatus(t1, state);

        const array<double, 2> point = {{ zd2, az2 }};
        UpdatePointing(t1, point);

        const array<double, 8> track =
        {{
            ra, dec, ha,
            zd1, az1,
            zd_err, az_err,
            dev
        }};
        if (mjd>0)
            UpdateTracking(Time(mjd), track);

        // ---- DIM ----> t1 as event time
        //                status1
        //                mjd
        //                ra/dec/ha
        //                zd/az (nominal)
        //                zd/az (current)
        //                err(zd/az)
        //                [armed] [stgmd]

        // Maybe:
        // POINTING_POSITION --> t1, zd/az (current), [armed, stgmd, status1]
        //
        // if (mjd>0)
        // TRACKING_POSITION --> mjd, zd/az (nominal), err(zd/az)
        //                       ra/dec, ha(not well defined),
        //                       [Nominal + Error == Current]

        // MJD is the time which corresponds to the nominal position
        // t1  is the time which corresponds to the current position/HA

        return true;
    }

protected:
    void HandleReceivedReport(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host (cosy).");

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
            PostClose(err!=ba::error::basic_errors::operation_aborted);
            return;
        }

        istream is(&fBuffer);

        string line;
        getline(is, line);

        if (fIsVerbose)
            Out() << line << endl;

        StartReadReport();

        if (line.substr(0, 13)=="DRIVE-STATUS ")
        {
            ProcessDriveStatus(line.substr(70));
            return;
        }

        if (line.substr(0, 13)=="STARG-REPORT ")
        {
            ProcessStargReport(line.substr(16));
            return;
        }

        if (line.substr(0, 14)=="TPOINT-REPORT ")
        {
            ProcessTpointReport(line.substr(17));
            return;
        }

        if (line.substr(0, 13)=="DRIVE-REPORT ")
        {
            ProcessDriveReport(line.substr(16));
            return;
        }
    }

    void StartReadReport()
    {
        boost::asio::async_read_until(*this, fBuffer, '\n',
                                      boost::bind(&ConnectionDrive::HandleReceivedReport, this,
                                                  dummy::error, dummy::bytes_transferred));
    }

    boost::asio::deadline_timer fKeepAlive;

    void KeepAlive()
    {
        PostMessage(string("KEEP_ALIVE"));

        fKeepAlive.expires_from_now(boost::posix_time::seconds(10));
        fKeepAlive.async_wait(boost::bind(&ConnectionDrive::HandleKeepAlive,
                                          this, dummy::error));
    }

    void HandleKeepAlive(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Write timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
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
        if (fKeepAlive.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        KeepAlive();
    }


private:
    // This is called when a connection was established
    void ConnectionEstablished()
    {
        StartReadReport();
        KeepAlive();
    }

    /*
    void HandleReadTimeout(const bs::error_code &error)
    {
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            stringstream str;
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
    }*/


public:

    static const uint16_t kMaxAddr;

public:
    ConnectionDrive(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fState(-1), fIsVerbose(true),
        fDeviationLimit(120), fDeviationCounter(5), fDeviationMax(240),
        fDevBuffer(5), fDevCount(0),
        fTrackingCounter(0), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetDeviationCondition(uint16_t limit, uint16_t counter, uint16_t max)
    {
        fDeviationLimit   = limit;
        fDeviationCounter = counter;
        fDeviationMax     = max;
    }

    int GetState() const
    {
        if (!IsConnected())
            return 1;
        if (IsConnected() && fState<0)
            return 2;
        return fState;
    }
};

const uint16_t ConnectionkMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimDrive : public ConnectionDrive
{
private:
    DimDescribedService fDimPointing;
    DimDescribedService fDimTracking;
    DimDescribedService fDimSource;
    DimDescribedService fDimTPoint;
    DimDescribedService fDimStatus;

    void UpdatePointing(const Time &t, const array<double, 2> &arr)
    {
        fDimPointing.setData(arr);
        fDimPointing.Update(t);
    }

    void UpdateTracking(const Time &t,const array<double, 8> &arr)
    {
        fDimTracking.setData(arr);
        fDimTracking.Update(t);
    }

    void UpdateStatus(const Time &t, const array<uint8_t, 3> &arr)
    {
        fDimStatus.setData(arr);
        fDimStatus.Update(t);
    }

    void UpdateTPoint(const Time &t, const DimTPoint &data,
                      const string &name)
    {
        vector<char> dim(sizeof(data)+name.length()+1);
        memcpy(dim.data(), &data, sizeof(data));
        memcpy(dim.data()+sizeof(data), name.c_str(), name.length()+1);

        fDimTPoint.setData(dim);
        fDimTPoint.Update(t);
    }

public:
    ConnectionDimDrive(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionDrive(ioservice, imp),
        fDimPointing("DRIVE_CONTROL/POINTING_POSITION", "D:1;D:1",
                     "|Zd[deg]:Zenith distance (encoder readout)"
                     "|Az[deg]:Azimuth angle (encoder readout)"),
        fDimTracking("DRIVE_CONTROL/TRACKING_POSITION", "D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1",
                     "|Ra[h]:Command right ascension"
                     "|Dec[deg]:Command declination"
                     "|Ha[h]:Corresponding hour angle"
                     "|Zd[deg]:Nominal zenith distance"
                     "|Az[deg]:Nominal azimuth angle"
                     "|dZd[deg]:Control deviation Zd"
                     "|dAz[deg]:Control deviation Az"
                     "|dev[arcsec]:Absolute control deviation"),
        fDimSource("DRIVE_CONTROL/SOURCE_POSITION", "D:1;D:1;D:1;D:1;D:1;D:1;C:31",
                     "|Ra_src[h]:Source right ascension"
                     "|Dec_src[deg]:Source declination"
                     "|Ra_cmd[h]:Command right ascension"
                     "|Dec_cmd[deg]:Command declination"
                     "|Offset[deg]:Wobble offset"
                     "|Angle[deg]:Wobble angle"
                     "|Name[string]:Source name if available"),
        fDimTPoint("DRIVE_CONTROL/TPOINT_DATA", "D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;S:1;S:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;C",
                   "|Ra[h]:Command right ascension"
                   "|Dec[deg]:Command declination"
                   "|Zd_nom[deg]:Nominal zenith distance"
                   "|Az_nom[deg]:Nominal azimuth angle"
                   "|Zd_cur[deg]:Current zenith distance (calculated from image)"
                   "|Az_cur[deg]:Current azimuth angle (calculated from image)"
                   "|Zd_enc[deg]:Feedback zenith axis (from encoder)"
                   "|Az_enc[deg]:Feedback azimuth angle (from encoder)"
                   "|N_leds[cnt]:Number of detected LEDs"
                   "|N_rings[cnt]:Number of rings used to calculate the camera center"
                   "|Xc[pix]:X position of center in CCD camera frame"
                   "|Yc[pix]:Y position of center in CCD camera frame"
                   "|Ic[au]:Average intensity (LED intensity weighted with their frequency of occurance in the calculation)"
                   "|Xs[pix]:X position of start in CCD camera frame"
                   "|Ys[pix]:Y position of star in CCD camera frame"
                   "|Ms[mag]:Artifical magnitude of star (calculated form image))"
                   "|Mc[mag]:Catalog magnitude of star"
                   "|name[string]:Name of star"),
        fDimStatus("DRIVE_CONTROL/STATUS", "C:2;C:1", "")

    {
    }

    void UpdateSource(const string &name="", bool tracking=false)
    {
        vector<char> dat(6*sizeof(double)+31, 0);
        strncpy(dat.data()+6*sizeof(double), name.c_str(), 30);

        fDimSource.setQuality(tracking);
        fDimSource.Update(dat);
    }

    void UpdateSource(const array<double, 6> &arr, const string &name="")
    {
        vector<char> dat(6*sizeof(double)+31, 0);
        memcpy(dat.data(), arr.data(), 6*sizeof(double));
        strncpy(dat.data()+6*sizeof(double), name.c_str(), 30);

        fDimSource.setQuality(1);
        fDimSource.Update(dat);
    }

    // A B [C] [D] E [F] G H [I] J K [L] M N O P Q R [S] T U V W [X] Y Z
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineDrive : public StateMachineAsio<T>
{
private:
    S fDrive;

    string fDatabase;

    typedef map<string, Source> sources;
    sources fSources;

    string fLastCommand;  // Last tracking (RADEC) command
    int fAutoResume;      // 0: disabled, 1: enables, 2: resuming
    Time fAutoResumeTime;

    // Status 0: Error
    // Status 1: Unlocked
    // Status 2: Locked
    // Status 3: Stopping || Moving
    // Status 4: Tracking

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    enum Coordinates
    {
        kPoint,
        kTrackSlow,
        kTrackFast
    };

    string AngleToStr(double angle)
    {
        /* Handle sign */
        const char sgn = angle<0?'-':'+';

        /* Round interval and express in smallest units required */
        double a = round(3600. * fabs(angle)); // deg to seconds

        /* Separate into fields */
        const double ad = trunc(a/3600.);
        a -= ad * 3600.;
        const double am = trunc(a/60.);
        a -= am * 60.;
        const double as = trunc(a);

        /* Return results */
        ostringstream str;
        str << sgn << " " << uint16_t(ad) << " " << uint16_t(am) << " " << as;
        return str.str();
    }

    int SendCommand(const string &str)
    {
        // This happens if fLastCommand should be send,
        // but the last command was not a tracking command
        if (str.empty())
        {
            T::Info("Last command was not a tracking command. RESUME ignored.");
            return T::GetCurrentState();
        }

        fLastCommand = str.compare(0, 6, "RADEC ")==0 ? str : "";

        fDrive.PostMessage(str);
        T::Message("Sending: "+str);

        return T::GetCurrentState();
    }

    int TrackCelest(const string &cmd, const string &source)
    {
        SendCommand(cmd);

        fDrive.UpdateSource(source, true);

        return T::GetCurrentState();
    }

    int Park()
    {
        SendCommand("PREPS Park");
        fDrive.UpdateSource("Park");

        // FIXME: Go to locked state only when park position properly reached
        return Drive::State::kLocked;
    }

    int SendStop()
    {
        SendCommand("STOP!");
        fDrive.UpdateSource();

        return T::GetCurrentState();
    }

    int Resume()
    {
        if (fLastCommand.empty())
        {
            T::Info("Last command was not a tracking command. RESUME ignored.");
            return T::GetCurrentState();
        }

        
        T::Info("Resume: "+fLastCommand);
        return SendCommand(fLastCommand);
    }

    int SendCoordinates(const EventImp &evt, const Coordinates type)
    {
        if (!CheckEventSize(evt.GetSize(), "SendCoordinates", 16))
            return T::kSM_FatalError;

        const double *dat = evt.Ptr<double>();

        string command;

        switch (type)
        {
        case kPoint:      command += "ZDAZ ";  break;
        case kTrackSlow:  command += "RADEC "; break;
        case kTrackFast:  command += "GRB ";   break;
        }

        if (type!=kPoint)
        {
            const array<double, 6> dim = {{ dat[0], dat[1], dat[0], dat[1], 0, 0 }};
            fDrive.UpdateSource(dim);
        }
        else
            fDrive.UpdateSource("", false);


        command += AngleToStr(dat[0]) + ' ' + AngleToStr(dat[1]);
        return SendCommand(command);
    }

    int StartWobble(const double &srcra,  const double &srcdec,
                    const double &woboff, const double &wobang,
                    const string name="")
    {
        const double ra  = srcra *M_PI/12;
        const double dec = srcdec*M_PI/180;
        const double off = woboff*M_PI/180;
        const double dir = wobang*M_PI/180;

        const double cosdir = cos(dir);
        const double sindir = sin(dir);
        const double cosoff = cos(off);
        const double sinoff = sin(off);
        const double cosdec = cos(dec);
        const double sindec = sin(dec);

        if (off==0)
        {
            const array<double, 6> dim = {{ srcra, srcdec, srcra, srcdec, 0, 0 }};
            fDrive.UpdateSource(dim, name);

            string command = "RADEC ";
            command += AngleToStr(srcra) + ' ' + AngleToStr(srcdec);
            return SendCommand(command);
        }

        const double sintheta = sindec*cosoff + cosdec*sinoff*cosdir;
        if (sintheta >= 1)
        {
            T::Error("cos(Zd) > 1");
            return T::GetCurrentState();
        }

        const double costheta = sqrt(1 - sintheta*sintheta);

        const double cosdeltara = (cosoff - sindec*sintheta)/(cosdec*costheta);
        const double sindeltara = sindir*sinoff/costheta;

        const double ndec = asin(sintheta)*180/M_PI;
        const double nra  = (atan2(sindeltara, cosdeltara) + ra)*12/M_PI;

        const array<double, 6> dim = {{ srcra, srcdec, nra, ndec, woboff, wobang }};
        fDrive.UpdateSource(dim, name);

        string command = "RADEC ";
        command += AngleToStr(nra) + ' ' + AngleToStr(ndec);
        return SendCommand(command);
    }

    int Wobble(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Wobble", 32))
            return T::kSM_FatalError;

        const double *dat = evt.Ptr<double>();

        return StartWobble(dat[0], dat[1], dat[2], dat[3]);
    }

    const sources::const_iterator GetSourceFromDB(const char *ptr, const char *last)
    {
        if (find(ptr, last, '\0')==last)
        {
            T::Fatal("TrackWobble - The name transmitted by dim is not null-terminated.");
            throw uint32_t(T::kSM_FatalError);
        }

        const string name(ptr);

        const sources::const_iterator it = fSources.find(name);
        if (it==fSources.end())
        {
            T::Error("Source '"+name+"' not found in list.");
            throw uint32_t(T::GetCurrentState());
        }

        return it;
    }

    int TrackWobble(const EventImp &evt)
    {
        if (evt.GetSize()<=2)
        {
            ostringstream msg;
            msg << "Track - Received event has " << evt.GetSize() << " bytes, but expected at least 3.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        const uint16_t wobble = evt.GetUShort();
        if (wobble!=1 && wobble!=2)
        {
            ostringstream msg;
            msg << "TrackWobble - Wobble id " << wobble << " undefined, only 1 and 2 allowed.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        const char *ptr  = evt.Ptr<char>(2);
        const char *last = ptr+evt.GetSize()-2;

        try
        {
            const sources::const_iterator it = GetSourceFromDB(ptr, last);

            const string &name = it->first;
            const Source &src  = it->second;

            return StartWobble(src.ra, src.dec, src.offset, src.angle[wobble-1], name);
        }
        catch (const uint32_t &e)
        {
            return e;
        }
    }

    int StartTrackWobble(const char *ptr, size_t size, const double &offset=0, const double &angle=0)
    {
        const char *last = ptr+size;

        try
        {
            const sources::const_iterator it = GetSourceFromDB(ptr, last);

            const string &name = it->first;
            const Source &src  = it->second;

            return StartWobble(src.ra, src.dec, offset, angle, name);
        }
        catch (const uint32_t &e)
        {
            return e;
        }

    }

    int Track(const EventImp &evt)
    {
        if (evt.GetSize()<=16)
        {
            ostringstream msg;
            msg << "Track - Received event has " << evt.GetSize() << " bytes, but expected at least 17.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        const double *dat  = evt.Ptr<double>();
        const char   *ptr  = evt.Ptr<char>(16);
        const size_t  size = evt.GetSize()-16;

        return StartTrackWobble(ptr, size, dat[0], dat[1]);
    }

    int TrackOn(const EventImp &evt)
    {
        if (evt.GetSize()==0)
        {
            ostringstream msg;
            msg << "TrackOn - Received event has " << evt.GetSize() << " bytes, but expected at least 1.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        return StartTrackWobble(evt.Ptr<char>(), evt.GetSize());
    }


    int TakeTPoint(const EventImp &evt)
    {
        if (evt.GetSize()<=4)
        {
            ostringstream msg;
            msg << "TakePoint - Received event has " << evt.GetSize() << " bytes, but expected at least 5.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        const float  mag  = evt.Get<float>();
        const char  *ptr  = evt.Ptr<char>(4);

        string src(ptr);

        while (src.find_first_of(' ')!=string::npos)
            src.erase(src.find_first_of(' '), 1);

        SendCommand("TPOIN "+src+" "+to_string(mag));;

        return T::GetCurrentState();
    }

    int SetLedBrightness(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetLedBrightness", 8))
            return T::kSM_FatalError;

        const uint32_t *led = evt.Ptr<uint32_t>();

        return SendCommand("LEDS "+to_string(led[0])+" "+to_string(led[1]));
    }


    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fDrive.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetAutoResume(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetAutoResume", 1))
            return T::kSM_FatalError;

        fAutoResume = evt.GetBool();

        return T::GetCurrentState();
    }

    int Unlock()
    {
        if (fDrive.GetState()==StateMachineImp::kSM_Error)
        {
            T::Warn("Drive in error - maybe no connection to electronics... trying to send STOP.");
            SendStop();
        }

        return Drive::State::kNotReady;
    }

    int Print()
    {
        for (auto it=fSources.begin(); it!=fSources.end(); it++)
        {
            const string &name = it->first;
            const Source &src  = it->second;

            T::Out() << name << ",";
            T::Out() << src.ra       << "," << src.dec      << "," << src.offset << ",";
            T::Out() << src.angle[0] << "," << src.angle[1] << endl;
        }
        return T::GetCurrentState();
    }

    int ReloadSources()
    {
        try
        {
            ReadDatabase();
        }
        catch (const exception &e)
        {
            T::Error("Reading sources from databse failed: "+string(e.what()));
        }
        return T::GetCurrentState();
    }

    int Disconnect()
    {
        // Close all connections
        fDrive.PostClose(false);

        /*
         // Now wait until all connection have been closed and
         // all pending handlers have been processed
         poll();
         */

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fDrive.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fDrive.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fDrive.PostClose(true);

        return T::GetCurrentState();
    }

    Time fSunRise;
    /*
    int ShiftSunRise()
    {
        const Time sunrise = fSunRise;

        fSunRise = Time().GetNextSunRise();

        if (sunrise==fSunRise)
            return Drive::State::kLocked;

        ostringstream msg;
        msg << "Next sun-rise will be at " << fSunRise;
        T::Info(msg);

        return Drive::State::kLocked;
    }*/

    int Execute()
    {
        /*
        if (T::GetCurrentState()==Drive::State::kLocked)
            return ShiftSunRise();

        if (T::GetCurrentState()>Drive::State::kLocked)
        {
            if (Time()>fSunRise)
                return Park();
        }*/

        const Time now;


        if (now>fSunRise)
        {
            if (T::GetCurrentState()>Drive::State::kLocked)
                return Park();

            if (T::GetCurrentState()==Drive::State::kLocked)
            {
                fSunRise = now.GetNextSunRise();

                ostringstream msg;
                msg << "Next sun-rise will be at " << fSunRise;
                T::Info(msg);

                return Drive::State::kLocked;
            }
        }

        if (T::GetCurrentState()==Drive::State::kLocked)
            return Drive::State::kLocked;

        const int state = fDrive.GetState();

        if (!fLastCommand.empty())
        {
            // If auto resume is enabled and the drive is in error,
            // resume tracking
            if (state==StateMachineImp::kSM_Error)
            {
                if (fAutoResume==1)
                {
                    Resume();
                    fAutoResume = 2;
                    fAutoResumeTime = now;
                }

                if (fAutoResume==2 && fAutoResumeTime+boost::posix_time::seconds(5)<now)
                {
                    Resume();
                    fAutoResume = 3;
                }
            }
            else
            {
                // If drive got out of the error state,
                // enable auto resume again
                if (fAutoResume>1)
                    fAutoResume = 1;
            }
        }

        return state;
    }


public:
    StateMachineDrive(ostream &out=cout) :
        StateMachineAsio<T>(out, "DRIVE_CONTROL"), fDrive(*this, *this),
        fAutoResume(false), fSunRise(Time().GetNextSunRise())
    {
        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                     "No connection to cosy");

        T::AddStateName(State::kConnected, "Connected",
                        "Cosy connected, drive stopped");

        T::AddStateName(State::kNotReady, "NotReady",
                        "Drive system not ready for movement");

        T::AddStateName(State::kLocked, "Locked",
                        "Drive system is locked (will not accept commands)");

        T::AddStateName(State::kReady, "Ready",
                        "Drive system ready for movement");

        T::AddStateName(State::kArmed, "Armed",
                        "Cosy armed, drive stopped");

        T::AddStateName(State::kMoving, "Moving",
                        "Telescope moving");

        T::AddStateName(State::kTracking, "Tracking",
                        "Telescope is in tracking mode");

        T::AddStateName(State::kOnTrack, "OnTrack",
                        "Telescope tracking stable");

        // State::kIdle
        // State::kArmed
        // State::kMoving
        // State::kTracking

        // Init
        // -----------
        // "ARM lock"
        // "STGMD off"

        /*
         [ ] WAIT   -> WM_WAIT
         [x] STOP!  -> WM_STOP
         [x] RADEC  ra(+ d m s.f)  dec(+ d m s.f)
         [x] GRB    ra(+ d m s.f)  dec(+ d m s.f)
         [x] ZDAZ   zd(+ d m s.f)  az (+ d m s.f)
         [ ] CELEST id offset angle
         [ ] MOON   wobble offset
         [ ] PREPS  string
         [ ] TPOIN  star mag
         [ ] ARM    lock/unlock
         [ ] STGMD  on/off
         */

        // Drive Commands
        T::AddEvent("MOVE_TO", "D:2", State::kArmed)  // ->ZDAZ
            (bind(&StateMachineDrive::SendCoordinates, this, placeholders::_1, kPoint))
            ("Move the telescope to the given local coordinates"
             "|Zd[deg]:Zenith distance"
             "|Az[deg]:Azimuth");

        T::AddEvent("TRACK", "D:2", State::kArmed, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::SendCoordinates, this, placeholders::_1, kTrackSlow))
            ("Move the telescope to the given sky coordinates and start tracking them"
             "|Ra[h]:Right ascension"
             "|Dec[deg]:Declination");

        T::AddEvent("WOBBLE", "D:4", State::kArmed, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::Wobble, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given sky coordinates and start tracking them"
             "|Ra[h]:Right ascension"
             "|Dec[deg]:Declination"
             "|Offset[deg]:Wobble offset"
             "|Angle[deg]:Wobble angle");

        T::AddEvent("TRACK_SOURCE", "D:2;C", State::kArmed, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::Track, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given source and start tracking"
             "|Offset[deg]:Wobble offset"
             "|Angle[deg]:Wobble angle"
             "|Name[string]:Source name");

        T::AddEvent("TRACK_WOBBLE", "S:1;C", State::kArmed, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackWobble, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given source and start tracking"
             "|id:Wobble angle id (1 or 2)"
             "|Name[string]:Source name");

        T::AddEvent("TRACK_ON", "C", State::kArmed, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackOn, this, placeholders::_1))
            ("Move the telescope to the given position and start tracking"
             "|Name[string]:Source name");

        T::AddEvent("RESUME", StateMachineImp::kSM_Error)
            (bind(&StateMachineDrive::Resume, this))
            ("If drive is in Error state, this can b used to resume the last tracking command, if the last command sent to cosy was a tracking command.");

        T::AddEvent("MOON", State::kArmed, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, "MOON 0 0", "Moon"))
            ("Start tracking the moon");
        T::AddEvent("VENUS", State::kArmed, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, "CELEST 2 0 0", "Venus"))
            ("Start tracking Venus");
        T::AddEvent("MARS", State::kArmed, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, "CELEST 4 0 0", "Mars"))
            ("Start tracking Mars");
        T::AddEvent("JUPITER", State::kArmed, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, "CELEST 5 0 0", "Jupiter"))
            ("Start tracking Jupiter");
        T::AddEvent("SATURN", State::kArmed, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, "CELEST 6 0 0", "Saturn"))
            ("Start tracking Saturn");

        T::AddEvent("PARK", State::kArmed, State::kMoving, State::kTracking, State::kOnTrack, 0x100)
            (bind(&StateMachineDrive::Park, this))
            ("Park the telescope");

        T::AddEvent("TAKE_TPOINT")
            (bind(&StateMachineDrive::SendCommand, this, "TPOIN FACT 0"))
            ("Take a TPoint");

        T::AddEvent("TPOINT", "F:1;C")
            (bind(&StateMachineDrive::TakeTPoint, this, placeholders::_1))
            ("Take a TPoint (given values will be written to the TPoint files)"
             "|mag[float]:Magnitude of the star"
             "|name[string]:Name of the star");

        T::AddEvent("SET_LED_BRIGHTNESS", "I:2")
            (bind(&StateMachineDrive::SetLedBrightness, this, placeholders::_1))
            ("Set the LED brightness of the top and bottom leds"
             "|top[au]:Allowed range 0-32767 for top LEDs"
             "|bot[au]:Allowed range 0-32767 for bottom LEDs");

        T::AddEvent("LEDS_OFF")
            (bind(&StateMachineDrive::SendCommand, this, "LEDS 0 0"))
            ("Switch off TPoint LEDs");

        T::AddEvent("STOP")
            (bind(&StateMachineDrive::SendStop, this))
            ("Stop any kind of movement.");

//        T::AddEvent("ARM", State::kConnected)
//            (bind(&StateMachineSendCommand, this, "ARM lock"))
//            ("");

        T::AddEvent("UNLOCK", Drive::State::kLocked)
            (bind(&StateMachineDrive::Unlock, this))
            ("Unlock locked state.");

        T::AddEvent("SET_AUTORESUME", "B:1")
            (bind(&StateMachineDrive::SetAutoResume, this, placeholders::_1))
            ("Enable/disable auto resume"
             "|resume[bool]:if enabled, drive is tracking and goes to error state, the last tracking command is repeated automatically.");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineDrive::SetVerbosity, this, placeholders::_1))
            ("Set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected, State::kArmed)
            (bind(&StateMachineDrive::Disconnect, this))
            ("disconnect from ethernet");

        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected, State::kArmed)
            (bind(&StateMachineDrive::Reconnect, this, placeholders::_1))
            ("(Re)connect Ethernet connection to cosy, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");


        T::AddEvent("PRINT")
            (bind(&StateMachineDrive::Print, this))
            ("Print source list.");

        T::AddEvent("RELOAD_SOURCES")
            (bind(&StateMachineDrive::ReloadSources, this))
            ("Reload sources from database after database has changed..");

        fDrive.StartConnect();
    }

    void SetEndpoint(const string &url)
    {
        fDrive.SetEndpoint(url);
    }

    bool AddSource(const string &name, const Source &src)
    {
        const auto it = fSources.find(name);
        if (it!=fSources.end())
            T::Warn("Source '"+name+"' already in list... overwriting.");

        fSources[name] = src;
        return it==fSources.end();
    }

    void ReadDatabase(bool print=true)
    {
#ifdef HAVE_SQL
        Database db(fDatabase);

        T::Message("Connected to '"+db.uri()+"'");

        const mysqlpp::StoreQueryResult res =
            db.query("SELECT fSourceName, fRightAscension, fDeclination, fWobbleOffset, fWobbleAngle0, fWobbleAngle1 FROM Source").store();

        fSources.clear();
        for (vector<mysqlpp::Row>::const_iterator v=res.begin(); v<res.end(); v++)
        {
            const string name = (*v)[0].c_str();

            Source src;
            src.ra  = (*v)[1];
            src.dec = (*v)[2];
            src.offset = (*v)[3];
            src.angle[0] = (*v)[4];
            src.angle[1] = (*v)[5];
            AddSource(name, src);

            if (!print)
                continue;

            ostringstream msg;
            msg << " " << name << setprecision(8) << ":   Ra=" << src.ra << "h Dec=" << src.dec << "deg";
            msg << " Wobble=[" << src.offset << "," << src.angle[0] << "," << src.angle[1] << "]";
            T::Message(msg);
        }
#else
        T::Warn("MySQL support not compiled into the program.");
#endif
    }

    int EvalOptions(Configuration &conf)
    {
        if (!fSunRise)
            return 1;

        fDrive.SetVerbose(!conf.Get<bool>("quiet"));

        const vector<string> &vec = conf.Vec<string>("source");

        for (vector<string>::const_iterator it=vec.begin(); it!=vec.end(); it++)
        {
            istringstream stream(*it);

            string name;

            int i=0;

            Source src;

            string buffer;
            while (getline(stream, buffer, ','))
            {
                istringstream is(buffer);

                switch (i++)
                {
                case 0: name = buffer; break;
                case 1: src.ra  = ConnectionDrive::ReadAngle(is); break;
                case 2: src.dec = ConnectionDrive::ReadAngle(is); break;
                case 3: is >> src.offset; break;
                case 4: is >> src.angle[0]; break;
                case 5: is >> src.angle[1]; break;
                }

                if (is.fail())
                    break;
            }

            if (i==3 || i==6)
            {
                AddSource(name, src);
                continue;
            }

            T::Warn("Resource 'source' not correctly formatted: '"+*it+"'");
        }

        fDrive.SetDeviationCondition(conf.Get<uint16_t>("deviation-limit"),
                                     conf.Get<uint16_t>("deviation-count"),
                                     conf.Get<uint16_t>("deviation-max"));

        fAutoResume = conf.Get<bool>("auto-resume");

        if (conf.Has("source-database"))
        {
            fDatabase = conf.Get<string>("source-database");
            ReadDatabase();
        }

        if (fSunRise.IsValid())
        {
            ostringstream msg;
            msg << "Next sun-rise will be at " << fSunRise;
            T::Message(msg);
        }

        // The possibility to connect should be last, so that
        // everything else is already initialized.
        SetEndpoint(conf.Get<string>("addr"));

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineDrive<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    const string def = "localhost:7404";

    po::options_description control("Drive control options");
    control.add_options()
        ("no-dim,d",        po_switch(),        "Disable dim services")
        ("addr,a",          var<string>(def),   "Network address of cosy")
        ("quiet,q",         po_bool(true),      "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("source-database", var<string>(),      "Database link as in\n\tuser:password@server[:port]/database.")
        ("source",          vars<string>(),     "Additional source entry in the form \"name,hh:mm:ss,dd:mm:ss\"")
        ("deviation-limit", var<uint16_t>(90),  "Deviation limit in arcsec to get 'OnTrack'")
        ("deviation-count", var<uint16_t>(3),   "Minimum number of reported deviation below deviation-limit to get 'OnTrack'")
        ("deviation-max",   var<uint16_t>(180), "Maximum deviation in arcsec allowed to keep status 'OnTrack'")
        ("auto-resume",     po_bool(false),     "Enable auto result during tracking if connection is lost")
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
        "The cosyctrl is an interface to cosy.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: cosyctrl [-c type] [OPTIONS]\n"
        "  or:  cosyctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineDrive<StateMachine,ConnectionDrive>>();

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
                return RunShell<LocalStream, StateMachine, ConnectionDrive>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimDrive>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionDrive>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionDrive>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimDrive>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimDrive>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
