#include <numeric> // std::accumulate

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

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

#include "pal.h"
#include "nova.h"

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace std;
using namespace Drive;

// ------------------------------------------------------------------------

// The Nova classes are in degree. This is to be used in rad
struct RaDec
{
    double ra;  // [rad]
    double dec; // [rad]
    RaDec() : ra(0), dec(0) { }
    RaDec(double _ra, double _dec) : ra(_ra), dec(_dec) { }
};

struct RaDecHa : RaDec
{
    double ha;  // [rad]
    RaDecHa() : ha(0) { }
    RaDecHa(double _ra, double _dec, double _ha) : RaDec(_ra, _dec), ha(_ha) { }
};

struct Local
{
    double zd;
    double az;

    Local(double _zd=0, double _az=0) : zd(_zd), az(_az) { }
};

struct Velocity : Local
{
    Velocity(double _zd=0, double _az=0) : Local(_zd, _az) { }
    Velocity operator/(double f) const { return Velocity(zd/f, az/f); }
    Velocity operator*(double f) const { return Velocity(zd*f, az*f); }
};

struct Encoder : Local // [units: revolutions]
{
    Encoder(double _zd=0, double _az=0) : Local(_zd, _az) { }

    Encoder &operator*=(double f) { zd*=f; az*=f; return *this; }
    Encoder &operator-=(const Encoder &enc) { zd-=enc.zd; az-=enc.az; return *this; }
    Encoder operator*(double f) const { return Encoder(zd*f, az*f); }
    Velocity operator/(double t) const { return Velocity(zd/t, az/t); }
    Encoder Abs() const { return Encoder(fabs(zd), fabs(az)); }
};

struct ZdAz : Local // [units: rad]
{
    ZdAz(double _zd=0, double _az=0) : Local(_zd, _az) { }
    ZdAz operator*(const double &f) const { return ZdAz(zd*f, az*f); }
};

struct Acceleration : Local
{
    Acceleration(double _zd=0, double _az=0) : Local(_zd, _az) { }
    bool operator>(const Acceleration &a) const
    {
        return zd>a.zd || az>a.az;
    }
};

Encoder operator-(const Encoder &a, const Encoder &b)
{
    return Encoder(a.zd-b.zd, a.az-b.az);
}
Velocity operator-(const Encoder &a, const Velocity &b)
{
    return Velocity(a.zd-b.zd, a.az-b.az);
}
Velocity operator-(const Velocity &a, const Velocity &b)
{
    return Velocity(a.zd-b.zd, a.az-b.az);
}
Encoder operator/(const Encoder &a, const Encoder &b)
{
    return Encoder(a.zd/b.zd, a.az/b.az);
}

struct Weather
{
    float hum;
    float temp;
    float press;
    Time time;
};

struct Source
{
    Source() : ra(0), dec(0), mag(0), offset(0)
    {
        angles[0] = -90;
        angles[1] =  90;
    }

    string name;
    double ra;    // [h]
    double dec;   // [deg]
    double mag;

    double offset;
    array<double, 2> angles;

    bool operator!=(const Source &cmp)
    {
        return
            name      != cmp.name      ||
            ra        != cmp.ra        ||
            dec       != cmp.dec       ||
            mag       != cmp.mag       ||
            offset    != cmp.offset    ||
            angles[0] != cmp.angles[0] ||
            angles[1] != cmp.angles[1];
    }
};

enum Planets_t
{
    kENone     = -1,
    kESun      =  0,
    kEMercury  =  1,
    kEVenus    =  2,
    kEMoon     =  3, // earth moon barycentre
    kEMars     =  4,
    kEJupiter  =  5,
    kESaturn   =  6,
    kEUranus   =  7,
    kENeptune  =  8,
    kEPluto    =  9,
};

// ------------------------------------------------------------------------

struct PointingSetup
{
    Source    source;        // Informations about source to track       [h/deg]
    Planets_t planet;        // Id of the planet if tracking a planet
    double    start;         // Starting time of wobble observation      [mjd]
    double    orbit_period;  // Time for one revolution (0:off)          [day]
    double    wobble_offset; // Distance of wobble position              [rad]
    double    wobble_angle;  // Starting phi angle of wobble observation [rad]

    PointingSetup(Planets_t p=kENone) : planet(p), start(Time::none), orbit_period(0) { }
};

struct PointingData
{
    // Pointing direction of the opticl axis of the telescope
    RaDec     source;        // Informations about source to track      [rad/rad]
    RaDec     pointing;      // Catalog coordinates (J2000, FK5)        [rad/rad] pointing position
    RaDecHa   apparent;      // Apparent position on the sky            [rad/rad]
    ZdAz      sky;           // Apparent position on the sky            [rad/rad]
    Encoder   mount;         // Encoder position corresponding to 'sky' [deg/deg]
    double    mjd;
};

class PointingModel
{
private:
    double fIe;    // [rad] Index Error in Elevation
    double fIa;    // [rad] Index Error in Azimuth
    double fFlop;  // [rad] Vertical Sag
    double fNpae;  // [rad] Az-El Nonperpendicularity
    double fCa;    // [rad] Left-Right Collimation Error
    double fAn;    // [rad] Azimuth Axis Misalignment (N-S, 1st order)
    double fAw;    // [rad] Azimuth Axis Misalignment (E-W, 1st order)
    double fAn2;   // [rad] Azimuth Axis Misalignment (N-S, 2nd order)
    double fAw2;   // [rad] Azimuth Axis Misalignment (E-W, 2nd order)
    double fTf;    // [rad] Tube fluxture (sin)
    double fTx;    // [rad] Tube fluxture (tan)
    double fNrx;   // [rad] Nasmyth rotator displacement, horizontal
    double fNry;   // [rad] Nasmyth rotator displacement, vertical
    double fCrx;   // [rad] Alt/Az Coude Displacement (N-S)
    double fCry;   // [rad] Alt/Az Coude Displacement (E-W)
    double fEces;  // [rad] Elevation Centering Error (sin)
    double fAces;  // [rad] Azimuth Centering Error (sin)
    double fEcec;  // [rad] Elevation Centering Error (cos)
    double fAcec;  // [rad] Azimuth Centering Error (cos)

public:

    void Load(const string &name)
    {
        /*
         ! MMT 1987 July 8
         ! T   36   7.3622   41.448  -0.0481
         !   IA        -37.5465    20.80602
         !   IE        -13.9180     1.25217
         !   NPAE       +7.0751    26.44763
         !   CA         -6.9149    32.05358
         !   AN         +0.5053     1.40956
         !   AW         -2.2016     1.37480
         ! END
         */

        ifstream fin(name);
        if (!fin)
            throw runtime_error("Cannot open file "+name+": "+strerror(errno));

        map<string,double> coeff;

        string buf;
        while (getline(fin, buf))
        {
            buf = Tools::Trim(buf);

            vector<string> vec;
            boost::split(vec, buf, boost::is_any_of(" "), boost::token_compress_on);
            if (vec.size()<2)
                continue;

            coeff[vec[0]] = atof(vec[1].c_str()) * M_PI/180;
        }

        fIe    = coeff["IE"];    // [rad] Index Error in Elevation
        fIa    = coeff["IA"];    // [rad] Index Error in Azimuth
        fFlop  = coeff["FLOP"];  // [rad] Vertical Sag
        fNpae  = coeff["NPAE"];  // [rad] Az-El Nonperpendicularity
        fCa    = coeff["CA"];    // [rad] Left-Right Collimation Error
        fAn    = coeff["AN"];    // [rad] Azimuth Axis Misalignment (N-S, 1st order)
        fAw    = coeff["AW"];    // [rad] Azimuth Axis Misalignment (E-W, 1st order)
        fAn2   = coeff["AN2"];   // [rad] Azimuth Axis Misalignment (N-S, 2nd order)
        fAw2   = coeff["AW2"];   // [rad] Azimuth Axis Misalignment (E-W, 2nd order)
        fTf    = coeff["TF"];    // [rad] Tube fluxture (sin)
        fTx    = coeff["TX"];    // [rad] Tube fluxture (tan)
        fNrx   = coeff["NRX"];   // [rad] Nasmyth rotator displacement, horizontal
        fNry   = coeff["NRY"];   // [rad] Nasmyth rotator displacement, vertical
        fCrx   = coeff["CRX"];   // [rad] Alt/Az Coude Displacement (N-S)
        fCry   = coeff["CRY"];   // [rad] Alt/Az Coude Displacement (E-W)
        fEces  = coeff["ECES"];  // [rad] Elevation Centering Error (sin)
        fAces  = coeff["ACES"];  // [rad] Azimuth Centering Error (sin)
        fEcec  = coeff["ECEC"];  // [rad] Elevation Centering Error (cos)
        fAcec  = coeff["ACEC"];  // [rad] Azimuth Centering Error (cos)
    }

    void print(ostream &out)
    {
        out << "IE    " << Tools::Form("%10.5f", 180/M_PI*fIe)   << "\u00b0   # Index Error in Elevation\n";
        out << "IA    " << Tools::Form("%10.5f", 180/M_PI*fIa)   << "\u00b0   # Index Error in Azimuth\n";
        out << "FLOP  " << Tools::Form("%10.5f", 180/M_PI*fFlop) << "\u00b0   # Vertical Sag\n";
        out << "NPAE  " << Tools::Form("%10.5f", 180/M_PI*fNpae) << "\u00b0   # Az-El Nonperpendicularity\n";
        out << "CA    " << Tools::Form("%10.5f", 180/M_PI*fCa)   << "\u00b0   # Left-Right Collimation Error\n";
        out << "AN    " << Tools::Form("%10.5f", 180/M_PI*fAn)   << "\u00b0   # Azimuth Axis Misalignment (N-S, 1st order)\n";
        out << "AW    " << Tools::Form("%10.5f", 180/M_PI*fAw)   << "\u00b0   # Azimuth Axis Misalignment (E-W, 1st order)\n";
        out << "AN2   " << Tools::Form("%10.5f", 180/M_PI*fAn2)  << "\u00b0   # Azimuth Axis Misalignment (N-S, 2nd order)\n";
        out << "AW2   " << Tools::Form("%10.5f", 180/M_PI*fAw2)  << "\u00b0   # Azimuth Axis Misalignment (E-W, 2nd order)\n";
        out << "TF    " << Tools::Form("%10.5f", 180/M_PI*fTf)   << "\u00b0   # Tube fluxture (sin)\n";
        out << "TX    " << Tools::Form("%10.5f", 180/M_PI*fTx)   << "\u00b0   # Tube fluxture (tan)\n";
        out << "NRX   " << Tools::Form("%10.5f", 180/M_PI*fNrx)  << "\u00b0   # Nasmyth rotator displacement, horizontal\n";
        out << "NRY   " << Tools::Form("%10.5f", 180/M_PI*fNry)  << "\u00b0   # Nasmyth rotator displacement, vertical\n";
        out << "CRX   " << Tools::Form("%10.5f", 180/M_PI*fCrx)  << "\u00b0   # Alt/Az Coude Displacement (N-S)\n";
        out << "CRY   " << Tools::Form("%10.5f", 180/M_PI*fCry)  << "\u00b0   # Alt/Az Coude Displacement (E-W)\n";
        out << "ECES  " << Tools::Form("%10.5f", 180/M_PI*fEces) << "\u00b0   # Elevation Centering Error (sin)\n";
        out << "ACES  " << Tools::Form("%10.5f", 180/M_PI*fAces) << "\u00b0   # Azimuth Centering Error (sin)\n";
        out << "ECEC  " << Tools::Form("%10.5f", 180/M_PI*fEcec) << "\u00b0   # Elevation Centering Error (cos)\n";
        out << "ACEC  " << Tools::Form("%10.5f", 180/M_PI*fAcec) << "\u00b0   # Azimuth Centering Error (cos)" << endl;
    }

    struct AltAz
    {
        double alt;
        double az;

        AltAz(double _alt, double _az) : alt(_alt), az(_az) { }
        AltAz(const ZdAz &za) : alt(M_PI/2-za.zd), az(za.az) { }

        AltAz &operator+=(const AltAz &aa) { alt += aa.alt; az+=aa.az; return *this; }
        AltAz &operator-=(const AltAz &aa) { alt -= aa.alt; az-=aa.az; return *this; }
    };

    double Sign(double val, double alt) const
    {
        // Some pointing corrections are defined as Delta ZA, which
        // is (P. Wallace) defined [0,90]deg while Alt is defined
        // [0,180]deg
        return (M_PI/2-alt < 0 ? -val : val);
    }

    Encoder SkyToMount(AltAz p)
    {
        const AltAz CRX(-fCrx*sin(p.az-p.alt),  fCrx*cos(p.az-p.alt)/cos(p.alt));
        const AltAz CRY(-fCry*cos(p.az-p.alt), -fCry*sin(p.az-p.alt)/cos(p.alt));
        p += CRX;
        p += CRY;

        const AltAz NRX(fNrx*sin(p.alt), -fNrx);
        const AltAz NRY(fNry*cos(p.alt), -fNry*tan(p.alt));
        p += NRX;
        p += NRY;

        const AltAz CES(-fEces*sin(p.alt), -fAces*sin(p.az));
        const AltAz CEC(-fEcec*cos(p.alt), -fAcec*cos(p.az));
        p += CES;
        p += CEC;

        const AltAz TX(Sign(fTx/tan(p.alt), p.alt), 0);
        const AltAz TF(Sign(fTf*cos(p.alt), p.alt), 0);
        //p += TX;
        p += TF;

        const AltAz CA(0, -fCa/cos(p.alt));
        p += CA;

        const AltAz NPAE(0, -fNpae*tan(p.alt));
        p += NPAE;

        const AltAz AW2( fAw2*sin(p.az*2), -fAw2*cos(p.az*2)*tan(p.alt));
        const AltAz AN2(-fAn2*cos(p.az*2), -fAn2*sin(p.az*2)*tan(p.alt));
        const AltAz AW1( fAw *sin(p.az),   -fAw *cos(p.az)  *tan(p.alt));
        const AltAz AN1(-fAn *cos(p.az),   -fAn *sin(p.az)  *tan(p.alt));
        p += AW2;
        p += AN2;
        p += AW1;
        p += AN1;

        const AltAz FLOP(Sign(fFlop, p.alt), 0);
        p += FLOP;

        const AltAz I(fIe, fIa);
        p += I;

        return Encoder(90 - p.alt*180/M_PI, p.az *180/M_PI);
    }

    ZdAz MountToSky(const Encoder &mnt) const
    {
        AltAz p(M_PI/2-mnt.zd*M_PI/180, mnt.az*M_PI/180);

        const AltAz I(fIe, fIa);
        p -= I;

        const AltAz FLOP(Sign(fFlop, p.alt), 0);
        p -= FLOP;

        const AltAz AW1( fAw *sin(p.az),   -fAw *cos(p.az)  *tan(p.alt));
        const AltAz AN1(-fAn *cos(p.az),   -fAn *sin(p.az)  *tan(p.alt));
        const AltAz AW2( fAw2*sin(p.az*2), -fAw2*cos(p.az*2)*tan(p.alt));
        const AltAz AN2(-fAn2*cos(p.az*2), -fAn2*sin(p.az*2)*tan(p.alt));
        p -= AW1;
        p -= AN1;
        p -= AW2;
        p -= AN2;

        const AltAz NPAE(0, -fNpae*tan(p.alt));
        p -= NPAE;

        const AltAz CA(0, -fCa/cos(p.alt));
        p -= CA;

        const AltAz TF(Sign(fTf*cos(p.alt), p.alt), 0);
        const AltAz TX(Sign(fTx/tan(p.alt), p.alt), 0);
        p -= TF;
        //p -= TX;

        const AltAz CEC(-fEcec*cos(p.alt), -fAcec*cos(p.az));
        const AltAz CES(-fEces*sin(p.alt), -fAces*sin(p.az));
        p -= CEC;
        p -= CES;

        const AltAz NRY(fNry*cos(p.alt), -fNry*tan(p.alt));
        const AltAz NRX(fNrx*sin(p.alt), -fNrx);
        p -= NRY;
        p -= NRX;

        const AltAz CRY(-fCry*cos(p.az-p.alt), -fCry*sin(p.az-p.alt)/cos(p.alt));
        const AltAz CRX(-fCrx*sin(p.az-p.alt),  fCrx*cos(p.az-p.alt)/cos(p.alt));
        p -= CRY;
        p -= CRX;

        return ZdAz(M_PI/2-p.alt, p.az);
    }

    PointingData CalcPointingPos(const PointingSetup &setup, double _mjd, const Weather &weather, uint16_t timeout, bool tpoint=false)
    {
        PointingData out;
        out.mjd = _mjd;

        const double elong  = Nova::kORM.lng * M_PI/180;
        const double lat    = Nova::kORM.lat * M_PI/180;
        const double height = 2200;

        const bool   valid  = weather.time+boost::posix_time::seconds(timeout) > Time();

        const double temp   = valid ? weather.temp  :   10;
        const double hum    = valid ? weather.hum   : 0.25;
        const double press  = valid ? weather.press :  780;

        const double dtt = palDtt(_mjd);  // 32.184 + 35

        const double tdb = _mjd + dtt/3600/24;
        const double dut = 0;

        // prepare calculation: Mean Place to geocentric apperent
        // (UTC would also do, except for the moon?)
        double fAmprms[21];
        palMappa(2000.0, tdb, fAmprms);        // Epoche, TDB

        // prepare: Apperent to observed place
        double fAoprms[14];
        palAoppa(_mjd, dut,                    // mjd, Delta UT=UT1-UTC
                 elong, lat, height,           // long, lat, height
                 0, 0,                         // polar motion x, y-coordinate (radians)
                 273.155+temp, press, hum,     // temp, pressure, humidity
                 0.40, 0.0065,                 // wavelength, tropo lapse rate
                 fAoprms);

        out.source.ra  = setup.source.ra  * M_PI/ 12;
        out.source.dec = setup.source.dec * M_PI/180;

        if (setup.planet!=kENone)
        {
            // coordinates of planet: topocentric, equatorial, J2000
            // One can use TT instead of TDB for all planets (except the moon?)
            double ra, dec, diam;
            palRdplan(tdb, setup.planet, elong, lat, &ra, &dec, &diam);

            // ---- apparent to mean ----
            palAmpqk(ra, dec, fAmprms, &out.source.ra, &out.source.dec);
        }

        if (setup.wobble_offset<=0 || tpoint)
        {
            out.pointing.dec = out.source.dec;
            out.pointing.ra  = out.source.ra;
        }
        else
        {
            const double dphi =
                setup.orbit_period==0 ? 0 : 2*M_PI*(_mjd-setup.start)/setup.orbit_period;

            const double phi = setup.wobble_angle + dphi;

            const double cosdir = cos(phi);
            const double sindir = sin(phi);
            const double cosoff = cos(setup.wobble_offset);
            const double sinoff = sin(setup.wobble_offset);
            const double cosdec = cos(out.source.dec);
            const double sindec = sin(out.source.dec);

            const double sintheta = sindec*cosoff + cosdec*sinoff*cosdir;

            const double costheta = sintheta>1 ? 0 : sqrt(1 - sintheta*sintheta);

            const double cosdeltara = (cosoff - sindec*sintheta)/(cosdec*costheta);
            const double sindeltara = sindir*sinoff/costheta;

            out.pointing.dec = asin(sintheta);
            out.pointing.ra  = atan2(sindeltara, cosdeltara) + out.source.ra;
        }

        // ---- Mean to apparent ----
        double r=0, d=0;
        palMapqkz(out.pointing.ra, out.pointing.dec, fAmprms, &r, &d);

        //
        // Doesn't work - don't know why
        //
        //    slaMapqk (radec.Ra(), radec.Dec(), rdpm.Ra(), rdpm.Dec(),
        //              0, 0, (double*)fAmprms, &r, &d);
        //

        // -- apparent to observed --
        palAopqk(r, d, fAoprms,
                 &out.sky.az,        // observed azimuth (radians: N=0,E=90) [-pi, pi]
                 &out.sky.zd,        // observed zenith distance (radians)   [-pi/2, pi/2]
                 &out.apparent.ha,   // observed hour angle (radians)
                 &out.apparent.dec,  // observed declination (radians)
                 &out.apparent.ra);  // observed right ascension (radians)

        // ----- fix ambiguity -----
        if (out.sky.zd<0)
        {
            out.sky.zd  = -out.sky.zd;
            out.sky.az +=  out.sky.az<0 ? M_PI : -M_PI;
        }

        // Star culminating behind zenith and Az between ~90 and ~180deg
        if (out.source.dec<lat && out.sky.az>0)
            out.sky.az -= 2*M_PI;

        out.mount = SkyToMount(out.sky);

        return out;
    }
};

// ------------------------------------------------------------------------


class ConnectionDrive : public Connection
{
    uint16_t fVerbosity;

public:
    virtual void UpdatePointing(const Time &, const array<double, 2> &)
    {
    }

    virtual void UpdateTracking(const Time &, const array<double, 12> &)
    {
    }

    virtual void UpdateStatus(const Time &, const array<uint8_t, 3> &)
    {
    }

    virtual void UpdateTPoint(const Time &, const DimTPoint &, const string &)
    {
    }

    virtual void UpdateSource(const Time &, const string &, bool)
    {
    }
    virtual void UpdateSource(const Time &,const array<double, 5> &, const string& = "")
    {
    }

private:
    enum NodeId_t
    {
        kNodeAz = 1,
        kNodeZd = 3
    };

    enum
    {
        kRxNodeguard = 0xe,
        kRxPdo1      = 3,
        kRxPdo2      = 5,
        kRxPdo3      = 7,
        kRxPdo4      = 9,
        kRxSdo       = 0xb,
        kRxSdo4      = 0x40|0x3,
        kRxSdo2      = 0x40|0xb,
        kRxSdo1      = 0x40|0xf,
        kRxSdoOk     = 0x60,
        kRxSdoErr    = 0x80,

        kTxSdo       = 0x40,
        kTxSdo4      = 0x20|0x3,
        kTxSdo2      = 0x20|0xb,
        kTxSdo1      = 0x20|0xf,
    };

    void SendCanFrame(uint16_t cobid,
                      uint8_t m0=0, uint8_t m1=0, uint8_t m2=0, uint8_t m3=0,
                      uint8_t m4=0, uint8_t m5=0, uint8_t m6=0, uint8_t m7=0)
    {
        const uint16_t desc = (cobid<<5) | 8;

        vector<uint8_t> data(11);
        data[0] = 10;
        data[1] = desc>>8;
        data[2] = desc&0xff;

        const uint8_t msg[8] = { m0, m1, m2, m3, m4, m5, m6, m7 };
        memcpy(data.data()+3, msg, 8);

        PostMessage(data);
    }

    enum Index_t
    {
        kReqArmed      = 0x1000,
        kReqPDO        = 0x1001,
        kReqErrStat    = 0x1003,
        kReqSoftVer    = 0x100a,
        kReqKeepAlive  = 0x100b,
        kReqVel        = 0x2002,
        kReqVelRes     = 0x6002,
        kReqVelMax     = 0x6003,
        kReqPos        = 0x6004,
        kReqPosRes     = 0x6501,

        kSetArmed      = 0x1000,
        kSetPointVel   = 0x2002,
        kSetAcc        = 0x2003,
        kSetRpmMode    = 0x3006,
        kSetTrackVel   = 0x3007,
        kSetLedVoltage = 0x4000,
        kSetPosition   = 0x6004,
    };

    static uint32_t String(uint8_t b0=0, uint8_t b1=0, uint8_t b2=0, uint8_t b3=0)
    {
        return uint32_t(b0)<<24 | uint32_t(b1)<<16 | uint32_t(b2)<<8 | uint32_t(b3);
    }

    uint32_t fVelRes[2];
    uint32_t fVelMax[2];
    uint32_t fPosRes[2];

    uint32_t fErrCode[2];

    void HandleSdo(const uint8_t &node, const uint16_t &idx, const uint8_t &subidx,
                   const uint32_t &val, const Time &tv)
    {
        if (fVerbosity>0)
        {
            ostringstream out;
            out << hex;
            out << "SDO[" << int(node) << "] " << idx << "/" << int(subidx) << ": " << val << dec;
            Out() << out.str() << endl;
        }

        switch (idx)
        {
        case kReqArmed:
            //fArmed = val==1;
            return;

        case kReqErrStat:
            {
                fErrCode[node/2] = (val>>8);
                LogErrorCode(node);
            }
            return;

        case kReqSoftVer:
            //fSoftVersion = val;
            return;

        case kReqKeepAlive:
            // Do not display, this is used for CheckConnection
            fIsInitialized[node/2] = true;
            return;

        case kReqVel:
            //fVel = val;
            return;

        case kReqPos:
            switch (subidx)
            {
            case 0:
                fPdoPos1[node/2] = val;
                fPdoTime1[node/2] = tv;
                fHasChangedPos1[node/2] = true;
                return;
            case 1:
                fPdoPos2[node/2] = val;
                fPdoTime2[node/2] = tv;
                fHasChangedPos2[node/2] = true;
                return;
            }
            break;

        case kReqVelRes:
            fVelRes[node/2] = val;
            return;

        case kReqVelMax:
            fVelMax[node/2] = val;
            return;

        case kReqPosRes:
            fPosRes[node/2] = val;
            return;
        }

        ostringstream str;
        str << "HandleSDO: Idx=0x"<< hex << idx << "/" << (int)subidx;
        str << ", val=0x" << val;
        Warn(str);
    }

    void HandleSdoOk(const uint8_t &node, const uint16_t &idx, const uint8_t &subidx,
                     const Time &)
    {
        ostringstream out;
        out << hex;
        out << "SDO-OK[" << int(node) << "] " << idx << "/" << int(subidx) << dec << "   ";

        switch (idx)
        {
        case kSetArmed:
            out << "(Armed state set)";
            break;
            /*
        case 0x1001:
            Out() << inf2 << "- " << GetNodeName() << ": PDOs requested." << endl;
            return;
            */
        case kSetPointVel:
            out << "(Pointing velocity set)";
            break;

        case kSetAcc:
            out << "(Acceleration set)";
            break;

        case kSetRpmMode:
            out << "(RPM mode set)";
            break;

        case kSetLedVoltage:
            out << "(LED Voltage set)";
            Info(out);
            return;
              /*
        case 0x3007:
            //Out() << inf2 << "- Velocity set (" << GetNodeName() << ")" << endl;
            return;

        case 0x4000:
            HandleNodeguard(tv);
            return;

        case 0x6000:
            Out() << inf2 << "- " << GetNodeName() << ": Rotation direction set." << endl;
            return;

        case 0x6002:
            Out() << inf2 << "- " << GetNodeName() << ": Velocity resolution set." << endl;
            return;
            */
        case kSetPosition:
            out << "(Absolute positioning started)";
            break;
              /*
        case 0x6005:
            Out() << inf2 << "- " << GetNodeName() << ": Relative positioning started." << endl;
            fPosActive = kTRUE; // Make sure that the status is set correctly already before the first PDO
            return;*/
        }
        /*
        Out() << warn << setfill('0') << "WARNING - Nodedrv::HandleSDOOK: ";
        Out() << "Node #" << dec << (int)fId << ": Sdo=" << hex << idx  << "/" << (int)subidx << " set.";
        Out() << endl;
        */

        if (fVerbosity>1)
            Out() << out.str() << endl;
    }

    void HandleSdoError(const uint8_t &node, const uint16_t &idx, const uint8_t &subidx,
                        const Time &)
    {
        ostringstream out;
        out << hex;
        out << "SDO-ERR[" << int(node) << "] " << idx << "/" << int(subidx) << dec;
        Out() << out.str() << endl;
    }


    int32_t fPdoPos1[2];
    int32_t fPdoPos2[2];

    Time fPdoTime1[2];
public:
    Time fPdoTime2[2];
private:
    bool fHasChangedPos1[2];
    bool fHasChangedPos2[2];

    void HandlePdo1(const uint8_t &node, const uint8_t *data, const Time &tv)
    {
        const uint32_t pos1 = (data[3]<<24) | (data[2]<<16) | (data[1]<<8) | data[0];
        const uint32_t pos2 = (data[7]<<24) | (data[6]<<16) | (data[5]<<8) | data[4];

        if (fVerbosity>2)
            Out() << Time().GetAsStr("%M:%S.%f") << " PDO1[" << (int)node << "] " << 360.*int32_t(pos1)/fPosRes[node/2] << " " << 360.*int32_t(pos2)/fPosRes[node/2] << endl;

        // Once every few milliseconds!

        fPdoPos1[node/2]  = pos1;
        fPdoTime1[node/2] = tv;
        fHasChangedPos1[node/2] = true;

        fPdoPos2[node/2]  = pos2;
        fPdoTime2[node/2] = tv;
        fHasChangedPos2[node/2] = true;
    }

    uint8_t  fStatusAxis[2];
    uint8_t  fStatusSys;

    enum {
        kUpsAlarm     = 0x01,  // UPS Alarm      (FACT only)
        kUpsBattery   = 0x02,  // UPS on battery (FACT only)
        kUpsCharging  = 0x04,  // UPS charging   (FACT only)
        kEmergencyOk  = 0x10,  // Emergency button released
        kOvervoltOk   = 0x20,  // Overvoltage protection ok
        kManualMode   = 0x40,  // Manual mode button pressed

        kAxisBb       = 0x01,  // IndraDrive reports Bb (Regler betriebsbereit)
        kAxisMoving   = 0x02,  // SPS reports
        kAxisRpmMode  = 0x04,  // SPS reports
        kAxisRf       = 0x20,  // IndraDrive reports Rf (Regler freigegeben)
        kAxisError    = 0x40,  // IndraDrive reports an error
        kAxisHasPower = 0x80   // IndraDrive reports axis power on
    };

    //std::function<void(const Time &, const array<uint8_t, 3>&)> fUpdateStatus;

    void HandlePdo3(const uint8_t &node, const uint8_t *data, const Time &tv)
    {
        /*
         TX1M_STATUS.0  := 1;
         TX1M_STATUS.1  := ((NOT X_in_Standstill OR NOT X_in_AntriebHalt) AND (NOT X_PC_VStart AND NOT X_in_Pos)) OR X_PC_AnnounceStartMovement;
         TX1M_STATUS.2  := X_PC_VStart;
         TX1M_STATUS.6  := NOT X_ist_freigegeben;

         TX3M_STATUS.0  := X_ist_betriebsbereit;
         TX3M_STATUS.1  := 1;
         TX3M_STATUS.2  := Not_Aus_IO;
         TX3M_STATUS.3  := UeberspannungsSchutz_OK;
         TX3M_STATUS.4  := FB_soll_drehen_links OR FB_soll_drehen_rechts OR FB_soll_schwenk_auf OR FB_soll_schwenk_ab;
         TX3M_STATUS.5  := X_ist_freigegeben;
         TX3M_STATUS.6  := NOT X_Fehler; (only in MATE, FACT==1)
         TX3M_STATUS.7  := LeistungEinAz;

         TX3M_STATUS.8  := NOT UPS_ALARM;
         TX3M_STATUS.9  := UPS_BattMode;
         TX3M_STATUS.10 := UPS_Charging;
         */

        const uint8_t sys = ((data[0] & 0x1c)<<2) | (data[1]);
        if (fStatusSys!=sys)
        {
            fStatusSys = sys;

            const bool alarm  = sys&kUpsAlarm;    // 01     TX3M.8  100
            const bool batt   = sys&kUpsBattery;  // 02     TX3M.9  200
            const bool charge = sys&kUpsCharging; // 04     TX3M.10 400
            const bool emcy   = sys&kEmergencyOk; // 10     TX3M.2  04
            const bool vltg   = sys&kOvervoltOk;  // 20     TX3M.3  08
            const bool mode   = sys&kManualMode;  // 40     TX3M.4  10

            ostringstream out;
            if (alarm)  out << " UPS-PowerLoss";
            if (batt)   out << " UPS-OnBattery";
            if (charge) out << " UPS-Charging";
            if (emcy)   out << " EmcyOk";
            if (vltg)   out << " OvervoltOk";
            if (mode)   out << " ManualMove";

            Info("New system status["+string(node==kNodeAz?"Az":"Zd")+"]:"+out.str());
            if (fVerbosity>1)
                Out() << "PDO3[" << (int)node << "] StatusSys=" << hex << (int)fStatusSys << dec << endl;
        }

        const uint8_t axis = (data[0]&0xa1) | (data[3]&0x46);
        if (fStatusAxis[node/2]!=axis)
        {
            fStatusAxis[node/2] = axis;

            const bool ready  = axis&kAxisBb;       // 01
            const bool move   = axis&kAxisMoving;   // 02
            const bool rpm    = axis&kAxisRpmMode;  // 04
            const bool rf     = axis&kAxisRf;       // 20
            const bool err    = axis&kAxisError;    // 40
            const bool power  = axis&kAxisHasPower; // 80

            ostringstream out;
            if (ready)  out << " DKC-Ready";
            if (move && !err)
                        out << " Moving";
            if (rpm)    out << " RpmMode";
            if (rf)     out << " RF";
            if (power)  out << " PowerOn";
            if (err)    out << " ERROR";

            Info("New axis status["+string(node==kNodeAz?"Az":"Zd")+"]:"+out.str());
            if (fVerbosity>1)
                Out() << "PDO3[" << (int)node << "] StatusAxis=" << hex << (int)fStatusAxis[node/2] << dec << endl;
        }

        array<uint8_t, 3> arr = {{ fStatusAxis[0], fStatusAxis[1], fStatusSys }};
        UpdateStatus(tv, arr);
    }

    string ErrCodeToString(uint32_t code) const
    {
        switch (code)
        {
        case 0: return "offline";
        case 0xa000: case 0xa0000:
        case 0xa001: case 0xa0001:
        case 0xa002: case 0xa0002:
        case 0xa003: case 0xa0003: return "Communication phase "+to_string(code&0xf);
        case 0xa010: case 0xa0010: return "Drive HALT";
        case 0xa012: case 0xa0012: return "Control and power section ready for operation";
        case 0xa013: case 0xa0013: return "Ready for power on";
        case 0xa100: case 0xa0100: return "Drive in Torque mode";
        case 0xa101: case 0xa0101: return "Drive in Velocity mode";
        case 0xa102: case 0xa0102: return "Position control mode with encoder 1";
        case 0xa103: case 0xa0103: return "Position control mode with encoder 2";
        case 0xa104: case 0xa0104: return "Position control mode with encoder 1, lagless";
        case 0xa105: case 0xa0105: return "Position control mode with encoder 2, lagless";
        case 0xa106: case 0xa0106: return "Drive controlled interpolated positioning with encoder 1";
        case 0xa107: case 0xa0107: return "Drive controlled interpolated positioning with encoder 2";
        case 0xa108: case 0xa0108: return "Drive controlled interpolated positioning with encoder 1, lagless";
        case 0xa109: case 0xa0109: return "Drive controlled interpolated positioning with encoder 2, lagless";
        //case 0xa146: return "Drive controlled interpolated relative positioning with encoder 1";
        //case 0xa147: return "Drive controlled interpolated relative positioning with encoder 2";
        //case 0xa148: return "Drive controlled interpolated relative positioning lagless with encoder 1";
        //case 0xa149: return "Drive controlled interpolated relative positioning lagless with encoder 2";
        case 0xa150: case 0xa0150: return "Drive controlled positioning with encoder 1";
        case 0xa151: case 0xa0151: return "Drive controlled positioning with encoder 1, lagless";
        case 0xa152: case 0xa0152: return "Drive controlled positioning with encoder 2";
        case 0xa153: case 0xa0153: return "Drive controlled positioning with encoder 2, lagless";
        case 0xa208:               return "Jog mode positive";
        case 0xa218:               return "Jog mode negative";
        case 0xa400: case 0xa4000: return "Automatic drive check and adjustment";
        case 0xa401: case 0xa4001: return "Drive decelerating to standstill";
        case 0xa800: case 0xa0800: return "Unknown operation mode";
        case 0xc217:               return "Motor encoder reading error";
        case 0xc218:               return "Shaft encoder reading error";
        case 0xc220:               return "Motor encoder initialization error";
        case 0xc221:               return "Shaft encoder initialization error";
        case 0xc300:               return "Command: set absolute measure";
        case 0xc400: case 0xc0400: return "Switching to parameter mode";
        case 0xc401: case 0xc0401: return "Drive active, switching mode not allowed";
        case 0xc500: case 0xc0500: return "Error reset";
        case 0xc600: case 0xc0600: return "Drive controlled homing procedure";
        case 0xe225:               return "Motor overload";
        case 0xe249: case 0xe2049: return "Positioning command velocity exceeds limit bipolar";
        case 0xe250:               return "Drive overtemp warning";
        case 0xe251:               return "Motor overtemp warning";
        case 0xe252:               return "Bleeder overtemp warning";
        case 0xe257:               return "Continous current limit active";
                     case 0xe2819: return "Main power failure";
        case 0xe259:               return "Command velocity limit active";
                     case 0xe8260: return "Torque limit active";
        case 0xe264:               return "Target position out of numerical range";
        case 0xe829: case 0xe8029: return "Positive position limit exceeded";
        case 0xe830: case 0xe8030: return "Negative position limit exceeded";
        case 0xe831:               return "Position limit reached during jog";
        case 0xe834:               return "Emergency-Stop";
        case 0xe842:               return "Both end-switches activated";
        case 0xe843:               return "Positive end-switch activated";
        case 0xe844:               return "Negative end-switch activated";
        case 0xf218: case 0xf2018: return "Amplifier overtemp shutdown";
        case 0xf219: case 0xf2019: return "Motor overtemp shutdown";
        case 0xf220:               return "Bleeder overload shutdown";
        case 0xf221: case 0xf2021: return "Motor temperature surveillance defective";
                     case 0xf2022: return "Unit temperature surveillance defective";
        case 0xf224:               return "Maximum breaking time exceeded";
                     case 0xf2025: return "Drive not ready for power on";
        case 0xf228: case 0xf2028: return "Excessive control deviation";
        case 0xf250:               return "Overflow of target position preset memory";
        case 0xf257: case 0xf2057: return "Command position out of range";
        case 0xf269:               return "Error during release of the motor holding brake";
        case 0xf276:               return "Absolute encoder moved out of monitoring window";
                     case 0xf2074: return "Absolute encoder 1 moved out of monitoring window";
                     case 0xf2075: return "Absolute encoder 2 moved out of monitoring window";
                     case 0xf2174: return "Lost reference of motor encoder";
        case 0xf409: case 0xf4009: return "Bus error on Profibus interface";
        case 0xf434:               return "Emergency-Stop";
        case 0xf629:               return "Positive position limit exceeded";
        case 0xf630:               return "Negative position limit exceeded";
        case 0xf634:               return "Emergency-Stop";
        case 0xf643:               return "Positive end-switch activated";
        case 0xf644:               return "Negative end-switch activated";
                     case 0xf8069: return "15V DC error";
        case 0xf870: case 0xf8070: return "24V DC error";
        case 0xf878: case 0xf8078: return "Velocity loop error";
                     case 0xf8079: return "Velocity limit exceeded";
                     case 0xf2026: return "Undervoltage in power section";
        }
        return "unknown";
    }

    void LogErrorCode(uint32_t node)
    {
        const uint8_t typ = fErrCode[node/2]>>16;

        ostringstream out;
        out << "IndraDrive ";
        out << (node==kNodeAz?"Az":"Zd");
        out << " [" << hex << fErrCode[node/2];
        out << "]: ";
        out << ErrCodeToString(fErrCode[node/2]);
        out << (typ==0xf || typ==0xe ? "!" : ".");

        switch (typ)
        {
        case 0xf: Error(out);   break;
        case 0xe: Warn(out);    break;
        case 0xa: Info(out);    break;
        case 0x0:
        case 0xc:
        case 0xd: Message(out); break;
        default:  Fatal(out);   break;
        }
    }

    void HandlePdo2(const uint8_t &node, const uint8_t *data, const Time &)
    {
        fErrCode[node/2] = (data[4]<<24) | (data[5]<<16) | (data[6]<<8) | data[7];

        if (fVerbosity>0)
            Out() << "PDO2[" << int(node) << "] err=" << hex << fErrCode[node/2] << endl;

        LogErrorCode(node);
   }

    struct SDO
    {
        uint8_t  node;
        uint8_t  req;
        uint16_t idx;
        uint8_t  subidx;
        uint32_t val;

        SDO(uint8_t n, uint8_t r, uint16_t i, uint8_t s, uint32_t v=0)
            : node(n), req(r&0xf), idx(i), subidx(s), val(v) { }

        bool operator==(const SDO &s) const
        {
            return node==s.node && idx==s.idx && subidx==s.subidx;
        }
    };

    struct Timeout_t : SDO, ba::deadline_timer
    {

        Timeout_t(ba::io_service& ioservice,
                  uint8_t n, uint8_t r, uint16_t i, uint8_t s, uint32_t v, uint16_t millisec) : SDO(n, r, i, s, v),
            ba::deadline_timer(ioservice)
        {
            expires_from_now(boost::posix_time::milliseconds(millisec));
        }
        // get_io_service()
    };

    std::list<Timeout_t> fTimeouts;

    vector<uint8_t> fData;

    void HandleReceivedData(const boost::system::error_code& err, size_t bytes_received, int)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received!=11 || fData[0]!=10 || err)
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

        Time now;

        const uint16_t desc  = fData[1]<<8 | fData[2];
        const uint16_t cobid = desc>>5;

        const uint8_t *data  = fData.data()+3;

        const uint16_t fcode = cobid >> 7;
        const uint8_t  node  = cobid & 0x1f;

        switch (fcode)
        {
        case kRxNodeguard:
            Out() << "Received nodeguard" << endl;
            //HandleNodeguard(node, now);
            break;

        case kRxSdo:
            {
                const uint8_t  cmd    = data[0];
                const uint16_t idx    = data[1] | (data[2]<<8);
                const uint8_t  subidx = data[3];
                const uint32_t dat    = data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24);

                const auto it = find(fTimeouts.begin(), fTimeouts.end(), SDO(node, cmd, idx, subidx));
                if (it!=fTimeouts.end())
                {
                    // This will call the handler and in turn remove the object from the list
                    it->cancel();
                }
                else
                {
                    ostringstream str;
                    str << hex;
                    str << "Unexpected SDO (";
                    str << uint32_t(node) << ": ";
                    str << ((cmd&0xf)==kTxSdo?"RX ":"TX ");
                    str << idx << "/" << uint32_t(subidx) << ")";

                    Warn(str);
                }

                switch (cmd)
                {
                case kRxSdo4:       // answer to 0x40 with 4 bytes of data
                    HandleSdo(node, idx, subidx, dat, now);
                    break;

                case kRxSdo2:       // answer to 0x40 with 2 bytes of data
                    HandleSdo(node, idx, subidx, dat&0xffff, now);
                    break;

                case kRxSdo1:       // answer to 0x40 with 1 byte  of data
                    HandleSdo(node, idx, subidx, dat&0xff, now);
                    break;

                case kRxSdoOk:     // answer to a SDO_TX message
                    HandleSdoOk(node, idx, subidx, now);
                    break;

                case kRxSdoErr:   // error message
                    HandleSdoError(node, idx, subidx, now);
                    break;

                default:
                    {
                        ostringstream out;
                        out << "Invalid SDO command code " << hex << cmd << " received.";
                        Error(out);
                        PostClose(false);
                        return;
                    }
                }
            }
            break;

        case kRxPdo1:
            HandlePdo1(node, data, now);
            break;

        case kRxPdo2:
            HandlePdo2(node, data, now);
            break;

        case kRxPdo3:
            HandlePdo3(node, data, now);
            break;

        default:
            {
                ostringstream out;
                out << "Invalid function code " << hex << fcode << " received.";
                Error(out);
                PostClose(false);
                return;
            }
        }

        StartReadReport();
    }

    void StartReadReport()
    {
        ba::async_read(*this, ba::buffer(fData),
                       boost::bind(&ConnectionDrive::HandleReceivedData, this,
                                   ba::placeholders::error, ba::placeholders::bytes_transferred, 0));

        //AsyncWait(fInTimeout, 35000, &Connection::HandleReadTimeout); // 30s
    }

    bool fIsInitialized[2];

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        //Info("Connection to PLC established.");

        fIsInitialized[0] = false;
        fIsInitialized[1] = false;

        SendSdo(kNodeZd, kSetArmed, 1);
        SendSdo(kNodeAz, kSetArmed, 1);

        RequestSdo(kNodeZd, kReqErrStat);
        RequestSdo(kNodeAz, kReqErrStat);

        SetRpmMode(false);

        RequestSdo(kNodeZd, kReqPosRes);
        RequestSdo(kNodeAz, kReqPosRes);

        RequestSdo(kNodeZd, kReqVelRes);
        RequestSdo(kNodeAz, kReqVelRes);

        RequestSdo(kNodeZd, kReqVelMax);
        RequestSdo(kNodeAz, kReqVelMax);

        RequestSdo(kNodeZd, kReqPos, 0);
        RequestSdo(kNodeAz, kReqPos, 0);
        RequestSdo(kNodeZd, kReqPos, 1);
        RequestSdo(kNodeAz, kReqPos, 1);

        RequestSdo(kNodeZd, kReqKeepAlive);
        RequestSdo(kNodeAz, kReqKeepAlive);

        StartReadReport();
    }

    void HandleTimeoutImp(const std::list<Timeout_t>::iterator &ref, const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
            return;

        if (error)
        {
            ostringstream str;
            str << "SDO timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            //PostClose();
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
        if (ref->expires_at() > ba::deadline_timer::traits_type::now())
            return;

        ostringstream str;
        str << hex;
        str << "SDO timeout (";
        str << uint32_t(ref->node) << ": ";
        str << (ref->req==kTxSdo?"RX ":"TX ");
        str << ref->idx << "/" << uint32_t(ref->subidx) << " [" << ref->val << "] ";
        str << to_simple_string(ref->expires_from_now());
        str << ")";

        Warn(str);

        //PostClose();
    }

    void HandleTimeout(const std::list<Timeout_t>::iterator &ref, const bs::error_code &error)
    {
        HandleTimeoutImp(ref, error);
        fTimeouts.erase(ref);
    }

    void SendSdoRequest(uint8_t node, uint8_t req,
                        uint16_t idx, uint8_t subidx, uint32_t val=0)
    {
        if (fVerbosity>1)
            Out() << "SDO-" << (req==kTxSdo?"REQ":"SET") << "[" << int(node) << "] " << idx << "/" << int(subidx) << " = " << val << endl;


        SendCanFrame(0x600|(node&0x1f), req, idx&0xff, idx>>8, subidx,
                     val&0xff, (val>>8)&0xff, (val>>16)&0xff, (val>>24)&0xff);

        // - The boost::asio::basic_deadline_timer::expires_from_now()
        //   function cancels any pending asynchronous waits, and returns
        //   the number of asynchronous waits that were cancelled. If it
        //   returns 0 then you were too late and the wait handler has
        //   already been executed, or will soon be executed. If it
        //   returns 1 then the wait handler was successfully cancelled.
        // - If a wait handler is cancelled, the bs::error_code passed to
        //   it contains the value bs::error::operation_aborted.

        const uint32_t milliseconds = 3000;
        fTimeouts.emplace_front(get_io_service(), node, req, idx, subidx, val, milliseconds);

        const std::list<Timeout_t>::iterator &timeout = fTimeouts.begin();

        timeout->async_wait(boost::bind(&ConnectionDrive::HandleTimeout, this, timeout, ba::placeholders::error));
    }

public:
    ConnectionDrive(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fVerbosity(0), fData(11)
    {
        SetLogStream(&imp);
    }

    void SetVerbosity(const uint16_t &v)
    {
        fVerbosity = v;
    }

    uint16_t GetVerbosity() const
    {
        return fVerbosity;
    }

    void RequestSdo(uint8_t node, uint16_t idx, uint8_t subidx=0)
    {
        SendSdoRequest(node, kTxSdo, idx, subidx);
    }
    void SendSdo(uint8_t node, uint16_t idx, uint8_t subidx, uint32_t val)
    {
        SendSdoRequest(node, kTxSdo4, idx, subidx, val);
    }

    void SendSdo(uint8_t node, uint16_t idx, uint32_t val)
    {
        SendSdo(node, idx, 0, val);
    }

    bool IsMoving() const
    {
        return (fStatusAxis[0]&kAxisMoving)  || (fStatusAxis[1]&kAxisMoving)
            || (fStatusAxis[0]&kAxisRpmMode) || (fStatusAxis[1]&kAxisRpmMode);
    }

    bool IsInitialized() const
    {
        // All important information has been successfully requested from the
        // SPS and the power control units are in RF (Regler freigegeben)
        return fIsInitialized[0] && fIsInitialized[1];
    }

    bool HasWarning() const
    {
        const uint8_t typ0 = fErrCode[0]>>16;
        const uint8_t typ1 = fErrCode[1]>>16;
        return typ0==0xe || typ1==0xe;
    }

    bool HasError() const
    {
        const uint8_t typ0 = fErrCode[0]>>16;
        const uint8_t typ1 = fErrCode[1]>>16;
        return typ0==0xf || typ1==0xf;
    }

    bool IsOnline() const
    {
        return fErrCode[0]!=0 && fErrCode[1]!=0;
    }

    bool IsReady() const
    {
        return fStatusAxis[0]&kAxisRf && fStatusAxis[1]&kAxisRf;
    }

    bool IsBlocked() const
    {
        return (fStatusSys&kEmergencyOk)==0 || (fStatusSys&kManualMode);
    }

    Encoder GetSePos() const // [rev]
    {
        return Encoder(double(fPdoPos2[1])/fPosRes[1], double(fPdoPos2[0])/fPosRes[0]);
    }

    double GetSeTime() const // [rev]
    {
        // The maximum difference here should not be larger than 100ms.
        // So th error we make on both axes should not exceed 50ms;
        return (Time(fPdoTime2[0]).Mjd()+Time(fPdoTime2[1]).Mjd())/2;
    }

    Encoder GetVelUnit() const
    {
        return Encoder(fVelMax[1], fVelMax[0]);
    }

    void SetRpmMode(bool mode)
    {
        const uint32_t val = mode ? String('s','t','r','t') : String('s','t','o','p');
        SendSdo(kNodeAz, kSetRpmMode, val);
        SendSdo(kNodeZd, kSetRpmMode, val);
    }

    void SetAcceleration(const Acceleration &acc)
    {
        SendSdo(kNodeAz, kSetAcc, lrint(acc.az*1000000000+0.5));
        SendSdo(kNodeZd, kSetAcc, lrint(acc.zd*1000000000+0.5));
    }

    void SetPointingVelocity(const Velocity &vel, double scale=1)
    {
        SendSdo(kNodeAz, kSetPointVel, lrint(vel.az*fVelMax[0]*scale));
        SendSdo(kNodeZd, kSetPointVel, lrint(vel.zd*fVelMax[1]*scale));
    }
    void SetTrackingVelocity(const Velocity &vel)
    {
        SendSdo(kNodeAz, kSetTrackVel, lrint(vel.az*fVelRes[0]));
        SendSdo(kNodeZd, kSetTrackVel, lrint(vel.zd*fVelRes[1]));
    }

    void StartAbsolutePositioning(const Encoder &enc, bool zd, bool az)
    {
        if (az) SendSdo(kNodeAz, kSetPosition, lrint(enc.az*fPosRes[0]));
        if (zd) SendSdo(kNodeZd, kSetPosition, lrint(enc.zd*fPosRes[1]));

        // Make sure that the status is set correctly already before the first PDO
        if (az) fStatusAxis[0] |= 0x02;
        if (zd) fStatusAxis[1] |= 0x02;

        // FIXME: UpdateDim?
    }

    void SetLedVoltage(const uint32_t &v1, const uint32_t &v2)
    {
        SendSdo(kNodeAz, 0x4000, v1);
        SendSdo(kNodeZd, 0x4000, v2);
    }
};


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

    // Update dim from a different thread to ensure that these
    // updates cannot block the main eventloop which eventually
    // also checks the timeouts
    Queue<pair<Time,array<double, 2>>>   fQueuePointing;
    Queue<pair<Time,array<double, 12>>>  fQueueTracking;
    Queue<tuple<Time,vector<char>,bool>> fQueueSource;
    Queue<pair<Time,vector<char>>>       fQueueTPoint;
    Queue<pair<Time,array<uint8_t, 3>>>  fQueueStatus;

    bool SendPointing(const pair<Time,array<double,2>> &p)
    {
        fDimPointing.setData(p.second);
        fDimPointing.Update(p.first);
        return true;
    }

    bool SendTracking(const pair<Time,array<double, 12>> &p)
    {
        fDimTracking.setData(p.second);
        fDimTracking.Update(p.first);
        return true;
    }

    bool SendSource(const tuple<Time,vector<char>,bool> &t)
    {
        const Time         &time     = get<0>(t);
        const vector<char> &data     = get<1>(t);
        const bool         &tracking = get<2>(t);

        fDimSource.setQuality(tracking);
        fDimSource.setData(data);
        fDimSource.Update(time);
        return true;
    }

    bool SendStatus(const pair<Time,array<uint8_t, 3>> &p)
    {
        fDimStatus.setData(p.second);
        fDimStatus.Update(p.first);
        return true;
    }

    bool SendTPoint(const pair<Time,vector<char>> &p)
    {
        fDimTPoint.setData(p.second);
        fDimTPoint.Update(p.first);
        return true;
    }

public:
    void UpdatePointing(const Time &t, const array<double, 2> &arr)
    {
        fQueuePointing.emplace(t, arr);
    }

    void UpdateTracking(const Time &t,const array<double, 12> &arr)
    {
        fQueueTracking.emplace(t, arr);
    }

    void UpdateStatus(const Time &t, const array<uint8_t, 3> &arr)
    {
        fQueueStatus.emplace(t, arr);
    }

    void UpdateTPoint(const Time &t, const DimTPoint &data,
                      const string &name)
    {
        vector<char> dim(sizeof(data)+name.length()+1);
        memcpy(dim.data(), &data, sizeof(data));
        memcpy(dim.data()+sizeof(data), name.c_str(), name.length()+1);

        fQueueTPoint.emplace(t, dim);
    }

    void UpdateSource(const Time &t, const string &name, bool tracking)
    {
        vector<char> dat(5*sizeof(double)+31, 0);
        strncpy(dat.data()+5*sizeof(double), name.c_str(), 30);

        fQueueSource.emplace(t, dat, tracking);
    }

    void UpdateSource(const Time &t, const array<double, 5> &arr, const string &name="")
    {
        vector<char> dat(5*sizeof(double)+31, 0);
        memcpy(dat.data(), arr.data(), 5*sizeof(double));
        strncpy(dat.data()+5*sizeof(double), name.c_str(), 30);

        fQueueSource.emplace(t, dat, true);
    }

public:
    ConnectionDimDrive(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionDrive(ioservice, imp),
        fDimPointing("DRIVE_CONTROL/POINTING_POSITION", "D:1;D:1",
                     "|Zd[deg]:Zenith distance (derived from encoder readout)"
                     "|Az[deg]:Azimuth angle (derived from encoder readout)"),
        fDimTracking("DRIVE_CONTROL/TRACKING_POSITION", "D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1",
                     "|Ra[h]:Command right ascension pointing direction (J2000)"
                     "|Dec[deg]:Command declination pointing direction (J2000)"
                     "|Ha[h]:Hour angle pointing direction"
                     "|SrcRa[h]:Right ascension source (J2000)"
                     "|SrcDec[deg]:Declination source (J2000)"
                     "|SrcHa[h]:Hour angle source"
                     "|Zd[deg]:Nominal zenith distance"
                     "|Az[deg]:Nominal azimuth angle"
                     "|dZd[deg]:Control deviation Zd"
                     "|dAz[deg]:Control deviation Az"
                     "|dev[arcsec]:Absolute control deviation"
                     "|avgdev[arcsec]:Average control deviation used to define OnTrack"),
        fDimSource("DRIVE_CONTROL/SOURCE_POSITION", "D:1;D:1;D:1;D:1;D:1;C:31",
                     "|Ra_src[h]:Source right ascension"
                     "|Dec_src[deg]:Source declination"
                     "|Offset[deg]:Wobble offset"
                     "|Angle[deg]:Wobble angle"
                     "|Period[min]:Time for one orbit"
                     "|Name[string]:Source name if available"),
        fDimTPoint("DRIVE_CONTROL/TPOINT_DATA", "D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;S:1;S:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;D:1;C",
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
                   "|Ms[mag]:Artifical magnitude of star (calculated from image)"
                   "|Phi[deg]:Rotation angle of image derived from detected LEDs"
                   "|Mc[mag]:Catalog magnitude of star"
                   "|Dx[arcsec]:De-rotated dx"
                   "|Dy[arcsec]:De-rotated dy"
                   "|Name[string]:Name of star"),
        fDimStatus("DRIVE_CONTROL/STATUS", "C:2;C:1", ""),
        fQueuePointing(std::bind(&ConnectionDimDrive::SendPointing, this, placeholders::_1)),
        fQueueTracking(std::bind(&ConnectionDimDrive::SendTracking, this, placeholders::_1)),
        fQueueSource(  std::bind(&ConnectionDimDrive::SendSource,   this, placeholders::_1)),
        fQueueTPoint(  std::bind(&ConnectionDimDrive::SendTPoint,   this, placeholders::_1)),
        fQueueStatus(  std::bind(&ConnectionDimDrive::SendStatus,   this, placeholders::_1))
    {
    }

    // A B [C] [D] E [F] G H [I] J K [L] M N O P Q R [S] T U V W [X] Y Z
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineDrive : public StateMachineAsio<T>
{
private:
    S fDrive;

    ba::deadline_timer fTrackingLoop;

    string fDatabase;

    typedef map<string, Source> sources;
    sources fSources;

    Weather fWeather;
    uint16_t fWeatherTimeout;

    ZdAz fParkingPos;

    PointingModel fPointingModel;
    PointingSetup fPointingSetup;
    Encoder       fMovementTarget;

    Time fSunRise;

    Encoder fPointingMin;
    Encoder fPointingMax;

    uint16_t fDeviationLimit;
    uint16_t fDeviationCounter;
    uint16_t fDeviationMax;

    float fApproachingLimit;

    vector<double> fDevBuffer;
    uint64_t       fDevCount;

    uint64_t fTrackingCounter;


    // --------------------- DIM Sending ------------------

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    // --------------------- DIM Receiving ------------------

    int HandleWeatherData(const EventImp &evt)
    {
        if (evt.GetSize()==0)
        {
            T::Warn("MAGIC_WEATHER disconnected... using default weather values");
            fWeather.time = Time(Time::none);
            return T::GetCurrentState();
        }

        if (!CheckEventSize(evt.GetSize(), "HandleWeatherData", 7*4+2))
        {
            fWeather.time = Time(Time::none);
            return T::GetCurrentState();
        }

        const float *ptr = evt.Ptr<float>(2);

        fWeather.temp  = ptr[0];
        fWeather.hum   = ptr[2];
        fWeather.press = ptr[3];
        fWeather.time  = evt.GetTime();

        return T::GetCurrentState();
    }

    int HandleTPoint(const EventImp &evt)
    {
        // Skip disconnect events
        if (evt.GetSize()==0)
            return T::GetCurrentState();

        // skip invalid events
        if (!CheckEventSize(evt.GetSize(), "HandleTPoint", 11*8))
            return T::GetCurrentState();

        // skip event which are older than one minute
        if (Time().UnixTime()-evt.GetTime().UnixTime()>60)
            return T::GetCurrentState();

        // Original code in slaTps2c:
        //
        // From the tangent plane coordinates of a star of known RA,Dec,
        // determine the RA,Dec of the tangent point.

        const double *ptr = evt.Ptr<double>();

        // Tangent plane rectangular coordinates
        const double dx = ptr[0] * M_PI/648000; // [arcsec -> rad]
        const double dy = ptr[1] * M_PI/648000; // [arcsec -> rad]

        const PointingData data = fPointingModel.CalcPointingPos(fPointingSetup, evt.GetTime().Mjd(), fWeather, fWeatherTimeout, true);

        const double x2 =     dx*dx;
        const double y2 = 1 + dy*dy;

        const double sd  = cos(data.sky.zd);//sin(M_PI/2-sky.zd);
        const double cd  = sin(data.sky.zd);//cos(M_PI/2-sky.zd);
        const double sdf = sd*sqrt(x2+y2);
        const double r2  = cd*cd*y2 - sd*sd*x2;

        // Case of no solution ("at the pole") or
        // two solutions ("over the pole solution")
        if (r2<0 || fabs(sdf)>=1)
        {
            T::Warn("Could not determine pointing direction from TPoint.");
            return T::GetCurrentState();
        }

        const double r   = sqrt(r2);
        const double s   = sdf - dy * r;
        const double c   = sdf * dy + r;
        const double phi = atan2(dx, r);

        // Spherical coordinates of tangent point
        const double az = fmod(data.sky.az-phi + 2*M_PI, 2*M_PI);
        const double zd = M_PI/2 - atan2(s, c);

        const Encoder dev = fDrive.GetSePos()*360 - data.mount;

        // --- Output TPoint ---

        const string fname = "tpoints-"+to_string(evt.GetTime().NightAsInt())+".txt";
        //time.GetAsStr("/%Y/%m/%d");

        const bool exist = boost::filesystem::exists(fname);

        ofstream fout(fname, ios::app);
        if (!exist)
        {
            fout << "FACT Model  TPOINT data file" << endl;
            fout << ": ALTAZ" << endl;
            fout << "49 48 0 ";
            fout << evt.GetTime() << endl;
        }
        fout << setprecision(7);
        fout << fmod(az*180/M_PI+360, 360) << " ";
        fout << 90-zd*180/M_PI << " ";
        fout << fmod(data.mount.az+360, 360) << " ";
        fout << 90-data.mount.zd << " ";
        fout << dev.az  << " ";    // delta az
        fout << -dev.zd << " ";    // delta el
        fout << 90-data.sky.zd * 180/M_PI << " ";
        fout << data.sky.az * 180/M_PI << " ";
        fout << setprecision(10);
        fout << data.mjd << " ";
        fout << setprecision(7);
        fout << ptr[6] << " ";  // center.mag
        fout << ptr[9] << " ";  // star.mag
        fout << ptr[4] << " ";  // center.x
        fout << ptr[5] << " ";  // center.y
        fout << ptr[7] << " ";  // star.x
        fout << ptr[8] << " ";  // star.y
        fout << ptr[2] << " ";  // num leds
        fout << ptr[3] << " ";  // num rings
        fout << ptr[0] << " ";  // dx (de-rotated)
        fout << ptr[1] << " ";  // dy (de-rotated)
        fout << ptr[10] << " "; // rotation angle
        fout << fPointingSetup.source.mag << " ";
        fout << fPointingSetup.source.name;
        fout << endl;

        DimTPoint dim;
        dim.fRa         = data.pointing.ra  *  12/M_PI;
        dim.fDec        = data.pointing.dec * 180/M_PI;
        dim.fNominalZd  = data.sky.zd * 180/M_PI;
        dim.fNominalAz  = data.sky.az * 180/M_PI;
        dim.fPointingZd = zd * 180/M_PI;
        dim.fPointingAz = az * 180/M_PI;
        dim.fFeedbackZd = data.mount.zd;
        dim.fFeedbackAz = data.mount.az;
        dim.fNumLeds    = uint16_t(ptr[2]);
        dim.fNumRings   = uint16_t(ptr[3]);
        dim.fCenterX    = ptr[4];
        dim.fCenterY    = ptr[5];
        dim.fCenterMag  = ptr[6];
        dim.fStarX      = ptr[7];
        dim.fStarY      = ptr[8];
        dim.fStarMag    = ptr[9];
        dim.fRotation   = ptr[10];
        dim.fDx         = ptr[0];
        dim.fDy         = ptr[1];
        dim.fRealMag    = fPointingSetup.source.mag;

        fDrive.UpdateTPoint(evt.GetTime(), dim, fPointingSetup.source.name);

        ostringstream txt;
        txt << "TPoint recorded [" << zd*180/M_PI << "/" << az*180/M_PI << " | "
            << data.sky.zd*180/M_PI << "/" << data.sky.az*180/M_PI << " | "
            << data.mount.zd << "/" << data.mount.az << " | "
            << dx*180/M_PI << "/" << dy*180/M_PI << "]";
        T::Info(txt);

        return T::GetCurrentState();
    }

    // -------------------------- Helpers -----------------------------------

    double GetDevAbs(double nomzd, double meszd, double devaz)
    {
        nomzd *= M_PI/180;
        meszd *= M_PI/180;
        devaz *= M_PI/180;

        const double x = sin(meszd) * sin(nomzd) * cos(devaz);
        const double y = cos(meszd) * cos(nomzd);

        return acos(x + y) * 180/M_PI;
    }

    double ReadAngle(istream &in)
    {
        char     sgn;
        uint16_t d, m;
        float    s;

        in >> sgn >> d >> m >> s;

        const double ret = ((60.0 * (60.0 * (double)d + (double)m) + s))/3600.;
        return sgn=='-' ? -ret : ret;
    }

    bool CheckRange(ZdAz pos)
    {
        if (pos.zd<fPointingMin.zd)
        {
            T::Error("Zenith distance "+to_string(pos.zd)+" below limit "+to_string(fPointingMin.zd));
            return false;
        }

        if (pos.zd>fPointingMax.zd)
        {
            T::Error("Zenith distance "+to_string(pos.zd)+" exceeds limit "+to_string(fPointingMax.zd));
            return false;
        }

        if (pos.az<fPointingMin.az)
        {
            T::Error("Azimuth angle "+to_string(pos.az)+" below limit "+to_string(fPointingMin.az));
            return false;
        }

        if (pos.az>fPointingMax.az)
        {
            T::Error("Azimuth angle "+to_string(pos.az)+" exceeds limit "+to_string(fPointingMax.az));
            return false;
        }

        return true;
    }

    PointingData CalcPointingPos(double mjd)
    {
        return fPointingModel.CalcPointingPos(fPointingSetup, mjd, fWeather, fWeatherTimeout);
    }

    // ----------------------------- SDO Commands ------------------------------

    int RequestSdo(const EventImp &evt)
    {
        // FIXME: STop telescope
        if (!CheckEventSize(evt.GetSize(), "RequestSdo", 6))
            return T::kSM_FatalError;

        const uint16_t node   = evt.Get<uint16_t>();
        const uint16_t index  = evt.Get<uint16_t>(2);
        const uint16_t subidx = evt.Get<uint16_t>(4);

        if (node!=1 && node !=3)
        {
            T::Error("Node id must be 1 (az) or 3 (zd).");
            return T::GetCurrentState();
        }

        if (subidx>0xff)
        {
            T::Error("Subindex must not be larger than 255.");
            return T::GetCurrentState();
        }

        fDrive.RequestSdo(node, index, subidx);

        return T::GetCurrentState();
    }

    int SendSdo(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SendSdo", 6+8))
            return T::kSM_FatalError;

        const uint16_t node   = evt.Get<uint16_t>();
        const uint16_t index  = evt.Get<uint16_t>(2);
        const uint16_t subidx = evt.Get<uint16_t>(4);
        const uint64_t value  = evt.Get<uint64_t>(6);

        if (node!=1 && node!=3)
        {
            T::Error("Node id must be 1 (az) or 3 (zd).");
            return T::GetCurrentState();
        }

        if (subidx>0xff)
        {
            T::Error("Subindex must not be larger than 255.");
            return T::GetCurrentState();
        }

        fDrive.SendSdo(node, index, subidx, value);

        return T::GetCurrentState();
    }

    // --------------------- Moving and tracking ---------------------

    uint16_t fStep;
    bool     fIsTracking;
    Acceleration fAccPointing;
    Acceleration fAccTracking;
    Acceleration fAccMax;
    double fMaxPointingResidual;
    double fMaxParkingResidual;
    double fPointingVelocity;

    int InitMovement(const ZdAz &sky, bool tracking=false, const string &name="")
    {
        fMovementTarget = fPointingModel.SkyToMount(sky);

        // Check whether bending is valid!
        if (!CheckRange(sky*(180/M_PI)))
            return StopMovement();

        fStep = 0;
        fIsTracking = tracking;

        fDrive.SetRpmMode(false); // *NEW*  (Stop a previous tracking to avoid the pointing command to be ignored)
        fDrive.SetAcceleration(fAccPointing);

        if (!tracking)
            fDrive.UpdateSource(Time(), name, false);
        else
        {
            const array<double, 5> dim =
            {{
                fPointingSetup.source.ra,
                fPointingSetup.source.dec,
                fPointingSetup.wobble_offset * 180/M_PI,
                fPointingSetup.wobble_angle  * 180/M_PI,
                fPointingSetup.orbit_period  * 24*60
            }};
            fDrive.UpdateSource(fPointingSetup.start, dim, fPointingSetup.source.name);
        }

        return State::kMoving;
    }

    int MoveTo(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "MoveTo", 16))
            return T::kSM_FatalError;

        const double *dat = evt.Ptr<double>();

        ostringstream out;
        out << "Pointing telescope to Zd=" << dat[0] << "deg Az=" << dat[1] << "deg";
        T::Message(out);

        return InitMovement(ZdAz(dat[0]*M_PI/180, dat[1]*M_PI/180));
    }

    int InitTracking()
    {
        fPointingSetup.start = Time().Mjd();

        const PointingData data = CalcPointingPos(fPointingSetup.start);

        ostringstream out;
        out << "Tracking position now at Zd=" << data.sky.zd*180/M_PI << "deg Az=" << data.sky.az*180/M_PI << "deg";
        T::Info(out);

        return InitMovement(data.sky, true);
    }

    int StartTracking(const Source &src, double offset, double angle, double period=0)
    {
        if (src.ra<0 || src.ra>=24)
        {
            ostringstream out;
            out << "Right ascension out of range [0;24[: Ra=" << src.ra << "h Dec=" << src.dec << "deg";
            if (!src.name.empty())
                out << " [" << src.name << "]";
            T::Error(out);
            return State::kInvalidCoordinates;
        }
        if (src.dec<-90 || src.dec>90)
        {
            ostringstream out;
            out << "Declination out of range [-90;90]: Ra=" << src.ra << "h Dec=" << src.dec << "deg";
            if (!src.name.empty())
                out << " [" << src.name << "]";
            T::Error(out);
            return State::kInvalidCoordinates;
        }

        ostringstream out;
        out << "Tracking Ra=" << src.ra << "h Dec=" << src.dec << "deg";
        if (!src.name.empty())
            out << " [" << src.name << "]";
        T::Info(out);

        fPointingSetup.planet        = kENone;
        fPointingSetup.source        = src;
        fPointingSetup.orbit_period  = period / 1440;      // [min->day]
        fPointingSetup.wobble_angle  = angle  * M_PI/180;  // [deg->rad]
        fPointingSetup.wobble_offset = offset * M_PI/180;  // [deg->rad]

        return InitTracking();
    }

    int TrackCelest(const Planets_t &p)
    {
        switch (p)
        {
        case kEMoon:    fPointingSetup.source.name = "Moon";    break;
        case kEVenus:   fPointingSetup.source.name = "Venus";   break;
        case kEMars:    fPointingSetup.source.name = "Mars";    break;
        case kEJupiter: fPointingSetup.source.name = "Jupiter"; break;
        case kESaturn:  fPointingSetup.source.name = "Saturn";  break;
        default:
             T::Error("TrackCelest - Celestial object "+to_string(p)+" not yet supported.");
             return T::GetCurrentState();
        }

        fPointingSetup.planet = p;
        fPointingSetup.wobble_offset = 0;

        fDrive.UpdateSource(Time(), fPointingSetup.source.name, true);

        return InitTracking();
    }

    int Park()
    {
        ostringstream out;
        out << "Parking telescope at Zd=" << fParkingPos.zd << "deg Az=" << fParkingPos.az << "deg";
        T::Message(out);

        const int rc = InitMovement(ZdAz(fParkingPos.zd*M_PI/180, fParkingPos.az*M_PI/180), false, "Park");
        return rc==State::kMoving ? State::kParking : rc;
    }

    int Wobble(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Wobble", 32))
            return T::kSM_FatalError;

        const double *dat = evt.Ptr<double>();

        Source src;
        src.ra  = dat[0];
        src.dec = dat[1];
        return StartTracking(src, dat[2], dat[3]);
    }

    int Orbit(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Orbit", 40))
            return T::kSM_FatalError;

        const double *dat = evt.Ptr<double>();

        Source src;
        src.ra  = dat[0];
        src.dec = dat[1];
        return StartTracking(src, dat[2], dat[3], dat[4]);
    }

    const sources::const_iterator GetSourceFromDB(const char *ptr, const char *last)
    {
        if (find(ptr, last, '\0')==last)
        {
            T::Fatal("TrackWobble - The name transmitted by dim is not null-terminated.");
            throw uint32_t(T::kSM_FatalError);
        }

        const string name(ptr);

        sources::const_iterator it = fSources.find(name);
        if (it!=fSources.end())
            return it;

        T::Warn("Source '"+name+"' not found in list... reloading database.");

        ReloadSources();

        it = fSources.find(name);
        if (it!=fSources.end())
            return it;

        T::Error("Source '"+name+"' still not found in list.");
        throw uint32_t(T::GetCurrentState());
    }

    int TrackWobble(const EventImp &evt)
    {
        if (evt.GetSize()<2)
        {
            ostringstream msg;
            msg << "TrackWobble - Received event has " << evt.GetSize() << " bytes, but expected at least 3.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        if (evt.GetSize()==2)
        {
            ostringstream msg;
            msg << "TrackWobble - Source name missing.";
            T::Error(msg);
            return T::GetCurrentState();
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

            const Source &src = it->second;
            return StartTracking(src, src.offset, src.angles[wobble-1]);
        }
        catch (const uint32_t &e)
        {
            return e;
        }
    }

    int StartTrackWobble(const char *ptr, size_t size, const double &offset=0, const double &angle=0, double time=0)
    {
        const char *last = ptr+size;

        try
        {
            const sources::const_iterator it = GetSourceFromDB(ptr, last);

            const Source &src = it->second;
            return StartTracking(src, offset<0?0.6/*src.offset*/:offset, angle, time);
        }
        catch (const uint32_t &e)
        {
            return e;
        }
    }

    int Track(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Track", 16))
            return T::kSM_FatalError;

        Source src;

        src.name   = "";
        src.ra     = evt.Get<double>(0);
        src.dec    = evt.Get<double>(8);

        return StartTracking(src, 0, 0);
    }

    int TrackSource(const EventImp &evt)
    {
        if (evt.GetSize()<16)
        {
            ostringstream msg;
            msg << "TrackOn - Received event has " << evt.GetSize() << " bytes, but expected at least 17.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        if (evt.GetSize()==16)
        {
            ostringstream msg;
            msg << "TrackOn - Source name missing.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        const double offset = evt.Get<double>(0);
        const double angle  = evt.Get<double>(8);

        return StartTrackWobble(evt.Ptr<char>(16), evt.GetSize()-16, offset, angle);
    }

    int TrackOn(const EventImp &evt)
    {
        if (evt.GetSize()==0)
        {
            ostringstream msg;
            msg << "TrackOn - Source name missing.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        return StartTrackWobble(evt.Ptr<char>(), evt.GetSize());
    }

    int TrackOrbit(const EventImp &evt)
    {
        if (evt.GetSize()<16)
        {
            ostringstream msg;
            msg << "TrackOrbit - Received event has " << evt.GetSize() << " bytes, but expected at least 17.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }
        if (evt.GetSize()==16)
        {
            ostringstream msg;
            msg << "TrackOrbit - Source name missing.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        const double angle = evt.Get<double>(0);
        const double time  = evt.Get<double>(8);

        return StartTrackWobble(evt.Ptr<char>(16), evt.GetSize()-16, -1, angle, time);
    }

    int StopMovement(bool reload=false)
    {
        fDrive.SetAcceleration(fAccMax);
        fDrive.SetRpmMode(false);

        fTrackingLoop.cancel();

        fDrive.UpdateSource(Time(), "", false);

        if (reload)
            ReloadSources();

        return State::kStopping;
    }

    int ResetError()
    {
        const int rc = CheckState();
        return rc>0 ? rc : State::kInitialized;
    }

    // --------------------- Others ---------------------

    int TPoint()
    {
        T::Info("TPoint initiated.");
        Dim::SendCommandNB("TPOINT/EXECUTE");
        return T::GetCurrentState();
    }

    int Screenshot(const EventImp &evt)
    {
        if (evt.GetSize()<2)
        {
            ostringstream msg;
            msg << "Screenshot - Received event has " << evt.GetSize() << " bytes, but expected at least 2.";
            T::Fatal(msg);
            return T::kSM_FatalError;
        }

        if (evt.GetSize()==2)
        {
            ostringstream msg;
            msg << "Screenshot - Filename missing.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        T::Info("Screenshot initiated.");
        Dim::SendCommandNB("TPOINT/SCREENSHOT", evt.GetData(), evt.GetSize());
        return T::GetCurrentState();
    }

    int SetLedBrightness(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetLedBrightness", 8))
            return T::kSM_FatalError;

        const uint32_t *led = evt.Ptr<uint32_t>();

        fDrive.SetLedVoltage(led[0], led[1]);

        return T::GetCurrentState();
    }

    int SetLedsOff()
    {
        fDrive.SetLedVoltage(0, 0);
        return T::GetCurrentState();
    }

    // --------------------- Internal ---------------------

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 2))
            return T::kSM_FatalError;

        fDrive.SetVerbosity(evt.GetUShort());

        return T::GetCurrentState();
    }

    int Print()
    {
        for (auto it=fSources.begin(); it!=fSources.end(); it++)
        {
            const string &name = it->first;
            const Source &src  = it->second;

            T::Out() << name << ",";
            T::Out() << src.ra        << "," << src.dec       << "," << src.offset << ",";
            T::Out() << src.angles[0] << "," << src.angles[1] << endl;
        }
        return T::GetCurrentState();
    }

    int PrintPointingModel()
    {
        fPointingModel.print(T::Out());
        return T::GetCurrentState();
    }

    int Unlock()
    {
        const int rc = CheckState();
        return rc<0 ? State::kInitialized : rc;
    }

    int ReloadSources()
    {
        try
        {
            ReadDatabase(false);
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

    // ========================= Tracking code =============================

    int UpdateTrackingPosition()
    {
        // First calculate deviation between
        // command position and nominal position
        //fPointing.mount = sepos; // [deg] ref pos for alignment
        const PointingData data = CalcPointingPos(fDrive.GetSeTime());

        // Get current position and calculate deviation
        const Encoder sepos = fDrive.GetSePos()*360; // [deg]
        const Encoder dev   = sepos - data.mount;

        // Calculate absolut deviation on the sky
        const double absdev = GetDevAbs(data.mount.zd, sepos.zd, dev.az)*3600;

        // Smoothing
        fDevBuffer[fDevCount++%5] = absdev;

        // Calculate average
        const uint8_t cnt    = fDevCount<5 ? fDevCount : 5;
        const double  avgdev = accumulate(fDevBuffer.begin(), fDevBuffer.begin()+cnt, 0.)/cnt;

        // Count the consecutive number of avgdev below fDeviationLimit
        if (avgdev<fDeviationLimit)
            fTrackingCounter++;
        else
            fTrackingCounter = 0;

        const double ha = fmod(fDrive.GetSeTime(),1)*24 - Nova::kORM.lng/15;

        array<double, 12> dim;
        dim[0]  = data.pointing.ra      *  12/M_PI; // Ra     [h]   optical axis
        dim[1]  = data.pointing.dec     * 180/M_PI; // Dec    [deg] optical axis
        dim[2]  = ha - data.pointing.ra;            // Ha     [h]   optical axis
        dim[3]  = data.source.ra        *  12/M_PI; // SrcRa  [h]   source position
        dim[4]  = data.source.dec       * 180/M_PI; // SrcDec [deg] source position
        dim[5]  = ha - data.source.ra;              // SrcHa  [h]   source position
        dim[6]  = data.sky.zd           * 180/M_PI; // Zd     [deg] optical axis
        dim[7]  = data.sky.az           * 180/M_PI; // Az     [deg] optical axis
        dim[8]  = dev.zd;                           // dZd    [deg] control deviation
        dim[9]  = dev.az;                           // dAz    [deg] control deviation
        dim[10] = absdev;                           // dev [arcsec] absolute control deviation
        dim[11] = avgdev;                           // dev [arcsec] average control deviation

        fDrive.UpdateTracking(fDrive.GetSeTime(), dim);

        if (fDrive.GetVerbosity())
            T::Out() << Time().GetAsStr("    %H:%M:%S.%f") << " - Deviation   [deg]    " << absdev << "\"|" << avgdev << "\"|" << fDevCount<< "  dZd=" << dev.zd*3600 << "\" dAz=" << dev.az*3600 << "\"" << endl;

        // Maximum deviation execeeded -> fall back to Tracking state
        if (T::GetCurrentState()==State::kOnTrack && avgdev>fDeviationMax)
            return State::kTracking;

        // Condition for OnTrack state achieved -> enhance to OnTrack state
        if (T::GetCurrentState()==State::kTracking && fTrackingCounter>=fDeviationCounter)
            return State::kOnTrack;

        // No state change
        return T::GetCurrentState();
    }

    void UpdatePointingPosition()
    {
        const Encoder sepos = fDrive.GetSePos()*360; // [deg] ref pos for alignment

        const ZdAz pos = fPointingModel.MountToSky(sepos);

        array<double, 2> data;
        data[0] = pos.zd*180/M_PI;   // Zd  [deg]
        data[1] = pos.az*180/M_PI;   // Az  [deg]
        fDrive.UpdatePointing(fDrive.GetSeTime(), data);

        if (fDrive.GetVerbosity())
        T::Out() << Time().GetAsStr("    %H:%M:%S.%f") << " - Position    [deg]    " << pos.zd*180/M_PI << " " << pos.az*180/M_PI << endl;
    }

    void TrackingLoop(const boost::system::error_code &error=boost::system::error_code())
    {
        if (error==ba::error::basic_errors::operation_aborted)
            return;

        if (error)
        {
            ostringstream str;
            str << "TrackingLoop: " << error.message() << " (" << error << ")";// << endl;
            T::Error(str);
            return;
        }

        if (T::GetCurrentState()!=State::kTracking &&
            T::GetCurrentState()!=State::kOnTrack)
            return;

        //
        // Update speed as often as possible.
        // make sure, that dt is around 10 times larger than the
        // update time
        //
        // The loop should not be executed faster than the ramp of
        // a change in the velocity can be followed.
        //
        fTrackingLoop.expires_from_now(boost::posix_time::milliseconds(250));

        const double mjd = Time().Mjd();

        // I assume that it takes about 50ms for the value to be
        // transmitted and the drive needs time to follow as well (maybe
        // more than 50ms), therefore the calculated speec is calculated
        // for a moment 50ms in the future
        const PointingData data  = CalcPointingPos(fDrive.GetSeTime());
        const PointingData data0 = CalcPointingPos(mjd-0.45/24/3600);
        const PointingData data1 = CalcPointingPos(mjd+0.55/24/3600);

        const Encoder dest  = data.mount *(1./360);  // [rev]
        const Encoder dest0 = data0.mount*(1./360);  // [rev]
        const Encoder dest1 = data1.mount*(1./360);  // [rev]
 
        if (!CheckRange(data1.sky))
        {
            StopMovement();
            T::HandleNewState(State::kAllowedRangeExceeded, 0, "by TrackingLoop");
            return;
        }

        // Current position
        const Encoder sepos = fDrive.GetSePos(); // [rev]

        // Now calculate the current velocity
        const Encoder dist = dest1 - dest0;      // [rev] Distance between t-1s and t+1s
        const Velocity vel = dist/(1./60);       // [rev/min] Actual velocity of the pointing position

        const Encoder dev  = sepos - dest;       // [rev] Current control deviation
        const Velocity vt  = vel - dev/(1./60);  // [rev/min] Correct velocity by recent control deviation
                                                 // correct control deviation with 5s
        if (fDrive.GetVerbosity()>1)
        {
             T::Out() << "Ideal position [deg]  " << dest.zd *360    << " " << dest.az *360    << endl;
             T::Out() << "Encoder pos.   [deg]  " << sepos.zd*360    << " " << sepos.az*360    << endl;
             T::Out() << "Deviation   [arcmin]  " << dev.zd  *360*60 << " " << dev.az  *360*60 << endl;
             T::Out() << "Distance 1s [arcmin]  " << dist.zd *360*60 << " " << dist.az *360*60 << endl;
             T::Out() << "Velocity 1s    [rpm]  " << vt.zd           << " " << vt.az           << endl;
             T::Out() << "Delta T (enc)   [ms]  " << fabs(mjd-fDrive.fPdoTime2[0].Mjd())*24*3600*1000 << endl;
             T::Out() << "Delta T (now)   [ms]  " << (Time().Mjd()-mjd)*24*3600*1000 << endl;
        }

        // Tracking loop every 250ms
        // Vorsteuerung 2s
        // Delta T (enc) 5ms, every 5th, 25ms
        // Delta T (now) equal dist 5ms-35 plus equal dist 25-55 (0.2%-2% of 2s)

        //
        // FIXME: check if the drive is fast enough to follow the star
        //
        // Velocity units (would be 100 for %)

        fDrive.SetTrackingVelocity(vt);

        fTrackingLoop.async_wait(boost::bind(&StateMachineDrive::TrackingLoop,
                                             this, ba::placeholders::error));
    }

    // =====================================================================

    int CheckState()
    {
        if (!fDrive.IsConnected())
            return State::kDisconnected;

        if (!fDrive.IsOnline())
            return State::kUnavailable;

        // FIXME: This can prevent parking in case e.g.
        // of e8029 Position limit exceeded
        if (fDrive.HasWarning() || fDrive.HasError())
        {
            if (T::GetCurrentState()==State::kOnTrack     ||
                T::GetCurrentState()==State::kTracking    ||
                T::GetCurrentState()==State::kMoving      ||
                T::GetCurrentState()==State::kApproaching ||
                T::GetCurrentState()==State::kParking)
                return StopMovement();

            if (T::GetCurrentState()==State::kStopping && fDrive.IsMoving())
                return State::kStopping;

            if (fDrive.HasError())
                return State::kHardwareError;

            if (fDrive.HasWarning())
                return State::kHardwareWarning;

            return StateMachineImp::kSM_Error;
        }

        // This can happen if one of the drives is not in RF.
        // Usually this only happens when the drive is not yet in RF
        // or an error was just cleared. Usually there is no way that
        // a drive goes below the RF state during operation without
        // a warning or error message.
        if (fDrive.IsOnline() && fDrive.IsBlocked())
            return State::kBlocked;

        if (fDrive.IsOnline() && !fDrive.IsReady())
            return State::kAvailable;

        // This is the case as soon as the init commands were send
        // after a connection to the SPS was established
        if (fDrive.IsOnline() && fDrive.IsReady() && !fDrive.IsInitialized())
            return State::kArmed;

        return -1;
    }

    int Execute()
    {
        const Time now;
        if (now>fSunRise && T::GetCurrentState()!=State::kParking)
        {
            fSunRise = now.GetNextSunRise();

            ostringstream msg;
            msg << "Next sun-rise will be at " << fSunRise;
            T::Info(msg);

            if (T::GetCurrentState()>State::kArmed && T::GetCurrentState()!=StateMachineImp::kError)
                return Park();
        }

        if (T::GetCurrentState()==State::kLocked)
            return State::kLocked;

        // FIXME: Send STOP if IsPositioning or RpmActive but no
        // Moving or Tracking state

        const int rc = CheckState();
        if (rc>0)
            return rc;

        // Once every second
        static time_t lastTime = 0;
        const time_t tm = time(NULL);
        if (lastTime!=tm && fDrive.IsInitialized())
        {
            lastTime=tm;

            UpdatePointingPosition();

            if (T::GetCurrentState()==State::kTracking || T::GetCurrentState()==State::kOnTrack)
                return UpdateTrackingPosition();
        }

        if (T::GetCurrentState()==State::kStopping && !fDrive.IsMoving())
            return State::kArmed;

        if ((T::GetCurrentState()==State::kMoving || T::GetCurrentState()==State::kApproaching ||
             T::GetCurrentState()==State::kParking) && !fDrive.IsMoving())
        {
            if (fIsTracking && fStep==1)
            {
                // Init tracking
                fDrive.SetAcceleration(fAccTracking);
                fDrive.SetRpmMode(true);

                fDevCount = 0;
                fTrackingCounter = 0;

                fTrackingLoop.expires_from_now(boost::posix_time::milliseconds(1));
                fTrackingLoop.async_wait(boost::bind(&StateMachineDrive::TrackingLoop,
                                                     this, ba::placeholders::error));

                fPointingSetup.start = Time().Mjd();

                const PointingData data = CalcPointingPos(fPointingSetup.start);

                ostringstream out;
                out << "Start tracking at Ra=" << data.pointing.ra*12/M_PI << "h Dec=" << data.pointing.dec*180/M_PI << "deg";
                T::Info(out);

                return State::kTracking;
            }

            // Get feedback 2
            const Encoder dest  = fMovementTarget*(1./360); // [rev]
            const Encoder sepos = fDrive.GetSePos();        // [rev]

            // Calculate residual to move deviation
            const Encoder dist  = dest - sepos;             // [rev]

            // Check which axis should still be moved
            Encoder cd = dist;              // [rev]
            cd *= T::GetCurrentState()==State::kParking ? 1./fMaxParkingResidual : 1./fMaxPointingResidual;  // Scale to units of the maximum residual
            cd = cd.Abs();

            // Check if there is a control deviation on the axis
            const bool cdzd = cd.zd>1;
            const bool cdaz = cd.az>1;

            if (!fIsTracking)
            {
                // check if we reached the correct position already
                if (!cdzd && !cdaz)
                {
                    T::Info("Target position reached in "+to_string(fStep)+" steps.");
                    return T::GetCurrentState()==State::kParking ? State::kLocked : State::kArmed;
                }

                if (fStep==10)
                {
                    T::Error("Target position not reached in "+to_string(fStep)+" steps.");
                    return State::kPositioningFailed;
                }
            }

            const Encoder t = dist.Abs()/fDrive.GetVelUnit();

            const Velocity vel =
                t.zd > t.az ?
                Velocity(1, t.zd==0?0:t.az/t.zd) :
                Velocity(t.az==0?0:t.zd/t.az, 1);

            if (fDrive.GetVerbosity())
            {
                T::Out() << "Moving step         " << fStep << endl;
                T::Out() << "Encoder      [deg]  " << sepos.zd*360 << " " << sepos.az*360 << endl;
                T::Out() << "Destination  [deg]  " << dest.zd *360 << " " << dest.az *360 << endl;
                T::Out() << "Residual     [deg]  " << dist.zd *360 << " " << dist.az *360 << endl;
                T::Out() << "Residual/max  [1]   " << cd.zd        << " " << cd.az        << endl;
                T::Out() << "Rel. time     [1]   " << t.zd         << " " << t.az         << endl;
                T::Out() << "Rel. velocity [1]   " << vel.zd       << " " << vel.az       << endl;
            }

            fDrive.SetPointingVelocity(vel, fPointingVelocity);
            fDrive.StartAbsolutePositioning(dest, cdzd, cdaz);

            ostringstream out;
            if (fStep==0)
                out << "Moving to encoder Zd=" << dest.zd*360 << "deg Az=" << dest.az*360 << "deg";
            else
                out << "Moving residual of dZd=" << dist.zd*360*60 << "' dAz=" << dist.az*360*60 << "'";
            T::Info(out);

            fStep++;
        }

        if (T::GetCurrentState()==State::kMoving && fDrive.IsMoving() && fIsTracking)
        {
            // First calculate deviation between
            // command position and nominal position
            //fPointing.mount = sepos; // [deg] ref pos for alignment
            const PointingData data = CalcPointingPos(fDrive.GetSeTime());

            // Get current position and calculate deviation
            const Encoder sepos = fDrive.GetSePos()*360; // [deg]
            const Encoder dev   = sepos - data.mount;

            // Calculate absolut deviation on the sky
            const double absdev = GetDevAbs(data.mount.zd, sepos.zd, dev.az);

            if (absdev<fApproachingLimit)
                return State::kApproaching;
        }

        return T::GetCurrentState()>=State::kInitialized ?
            T::GetCurrentState() : State::kInitialized;
    }

public:
    StateMachineDrive(ostream &out=cout) :
        StateMachineAsio<T>(out, "DRIVE_CONTROL"), fDrive(*this, *this),
        fTrackingLoop(*this), fSunRise(Time().GetNextSunRise()), fDevBuffer(5)
    {

        T::Subscribe("MAGIC_WEATHER/DATA")
            (bind(&StateMachineDrive::HandleWeatherData, this, placeholders::_1));

        T::Subscribe("TPOINT/DATA")
            (bind(&StateMachineDrive::HandleTPoint, this, placeholders::_1));

        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                        "No connection to SPS");
        T::AddStateName(State::kConnected, "Connected",
                        "Connection to SPS, no information received yet");

        T::AddStateName(State::kLocked, "Locked",
                        "Drive system is locked (will not accept commands)");

        T::AddStateName(State::kUnavailable, "Unavailable",
                        "Connected to SPS, no connection to at least one IndraDrives");
        T::AddStateName(State::kAvailable, "Available",
                        "Connected to SPS and to IndraDrives, but at least one drive not in RF");
        T::AddStateName(State::kBlocked, "Blocked",
                        "Drive system is blocked by manual operation or a pressed emergeny button");
        T::AddStateName(State::kArmed, "Armed",
                        "Connected to SPS and IndraDrives in RF, but not yet initialized");
        T::AddStateName(State::kInitialized, "Initialized",
                        "Connected to SPS and IndraDrives in RF and initialized");

        T::AddStateName(State::kStopping, "Stopping",
                        "Stop command sent, waiting for telescope to be still");
        T::AddStateName(State::kParking, "Parking",
                        "Telescope in parking operation, waiting for telescope to be still");
        T::AddStateName(State::kMoving, "Moving",
                        "Telescope moving");
        T::AddStateName(State::kApproaching, "Approaching",
                        "Telescope approaching destination");
        T::AddStateName(State::kTracking, "Tracking",
                        "Telescope in tracking mode");
        T::AddStateName(State::kOnTrack, "OnTrack",
                        "Telescope tracking stable");

        T::AddStateName(State::kPositioningFailed, "PositioningFailed",
                        "Target position was not reached within ten steps");
        T::AddStateName(State::kAllowedRangeExceeded, "OutOfRange",
                        "Telecope went out of range during tracking");
        T::AddStateName(State::kInvalidCoordinates, "InvalidCoordinates",
                        "Tracking coordinates out of range");

        T::AddStateName(State::kHardwareWarning, "HardwareWarning",
                        "At least one IndraDrive in a warning condition... check carefully!");
        T::AddStateName(State::kHardwareError, "HardwareError",
                        "At least one IndraDrive in an error condition... this is a serious incident!");


        T::AddEvent("REQUEST_SDO", "S:3", State::kArmed)
            (bind(&StateMachineDrive::RequestSdo, this, placeholders::_1))
            ("Request an SDO from the drive"
             "|node[uint32]:Node identifier (1:az, 3:zd)"
             "|index[uint32]:SDO index"
             "|subindex[uint32]:SDO subindex");

        T::AddEvent("SET_SDO", "S:3;X:1", State::kArmed)
            (bind(&StateMachineDrive::SendSdo, this, placeholders::_1))
            ("Request an SDO from the drive"
             "|node[uint32]:Node identifier (1:az, 3:zd)"
             "|index[uint32]:SDO index"
             "|subindex[uint32]:SDO subindex"
             "|value[uint64]:Value");

        // Drive Commands
        T::AddEvent("MOVE_TO", "D:2", State::kInitialized)  // ->ZDAZ
            (bind(&StateMachineDrive::MoveTo, this, placeholders::_1))
            ("Move the telescope to the given local sky coordinates"
             "|Zd[deg]:Zenith distance"
             "|Az[deg]:Azimuth");

        T::AddEvent("TRACK", "D:2", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::Track, this, placeholders::_1))
            ("Move the telescope to the given sky coordinates and start tracking them"
             "|Ra[h]:Right ascension"
             "|Dec[deg]:Declination");

        T::AddEvent("WOBBLE", "D:4", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::Wobble, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given sky coordinates and start tracking them"
             "|Ra[h]:Right ascension"
             "|Dec[deg]:Declination"
             "|Offset[deg]:Wobble offset"
             "|Angle[deg]:Wobble angle");

        T::AddEvent("ORBIT", "D:5", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::Orbit, this, placeholders::_1))
            ("Move the telescope in a circle around the source"
             "|Ra[h]:Right ascension"
             "|Dec[deg]:Declination"
             "|Offset[deg]:Wobble offset"
             "|Angle[deg]:Starting angle"
             "|Period[min]:Time for one orbit");

        T::AddEvent("TRACK_SOURCE", "D:2;C", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackSource, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given source and start tracking"
             "|Offset[deg]:Wobble offset"
             "|Angle[deg]:Wobble angle"
             "|Name[string]:Source name");

        T::AddEvent("TRACK_WOBBLE", "S:1;C", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackWobble, this, placeholders::_1))
            ("Move the telescope to the given wobble position around the given source and start tracking"
             "|Id:Wobble angle id (1 or 2)"
             "|Name[string]:Source name");

        T::AddEvent("TRACK_ORBIT", "D:2;C", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackOrbit, this, placeholders::_1))
            ("Move the telescope in a circle around the source"
             "|Angle[deg]:Starting angle"
             "|Period[min]:Time for one orbit"
             "|Name[string]:Source name");

        T::AddEvent("TRACK_ON", "C", State::kInitialized, State::kTracking, State::kOnTrack)   // ->RADEC/GRB
            (bind(&StateMachineDrive::TrackOn, this, placeholders::_1))
            ("Move the telescope to the given position and start tracking"
             "|Name[string]:Source name");

        T::AddEvent("MOON", State::kInitialized, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, kEMoon))
            ("Start tracking the moon");
        T::AddEvent("VENUS", State::kInitialized, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, kEVenus))
            ("Start tracking Venus");
        T::AddEvent("MARS", State::kInitialized, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, kEMars))
            ("Start tracking Mars");
        T::AddEvent("JUPITER", State::kInitialized, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, kEJupiter))
            ("Start tracking Jupiter");
        T::AddEvent("SATURN", State::kInitialized, State::kTracking, State::kOnTrack)
            (bind(&StateMachineDrive::TrackCelest, this, kESaturn))
            ("Start tracking Saturn");

        // FIXME: What to do in error state?
        T::AddEvent("PARK")(State::kInitialized)(State::kMoving)(State::kApproaching)(State::kTracking)(State::kOnTrack)(State::kHardwareWarning)
            (bind(&StateMachineDrive::Park, this))
            ("Park the telescope");

        T::AddEvent("STOP")(State::kUnavailable)(State::kAvailable)(State::kArmed)(State::kInitialized)(State::kStopping)(State::kParking)(State::kMoving)(State::kApproaching)(State::kTracking)(State::kOnTrack)
            (bind(&StateMachineDrive::StopMovement, this, false))
            ("Stop any kind of movement.");

        T::AddEvent("PREPARE_GRB")(State::kUnavailable)(State::kAvailable)(State::kArmed)(State::kInitialized)(State::kStopping)(State::kParking)(State::kMoving)(State::kApproaching)(State::kTracking)(State::kOnTrack)
            (bind(&StateMachineDrive::StopMovement, this, true))
            ("Combines STOP and RELOAD_SOURCES");

        T::AddEvent("RESET", State::kPositioningFailed, State::kAllowedRangeExceeded, State::kInvalidCoordinates, State::kHardwareWarning)
            (bind(&StateMachineDrive::ResetError, this))
            ("Acknowledge an internal drivectrl error (PositioningFailed, AllowedRangeExceeded, InvalidCoordinates)");

        T::AddEvent("TPOINT", State::kOnTrack)
            (bind(&StateMachineDrive::TPoint, this))
            ("Take a TPoint");

        T::AddEvent("SCREENSHOT", "B:1;C")
            (bind(&StateMachineDrive::Screenshot, this, placeholders::_1))
            ("Take a screenshot"
             "|color[bool]:False if just the gray image should be saved."
             "|name[string]:Filename");

        T::AddEvent("SET_LED_BRIGHTNESS", "I:2")
            (bind(&StateMachineDrive::SetLedBrightness, this, placeholders::_1))
            ("Set the LED brightness of the top and bottom leds"
             "|top[au]:Allowed range 0-32767 for top LEDs"
             "|bot[au]:Allowed range 0-32767 for bottom LEDs");

        T::AddEvent("LEDS_OFF")
            (bind(&StateMachineDrive::SetLedsOff, this))
            ("Switch off TPoint LEDs");

        T::AddEvent("UNLOCK", Drive::State::kLocked)
            (bind(&StateMachineDrive::Unlock, this))
            ("Unlock locked state.");

        // Verbosity commands
        T::AddEvent("SET_VERBOSITY", "S:1")
            (bind(&StateMachineDrive::SetVerbosity, this, placeholders::_1))
            ("Set verbosity state"
             "|verbosity[uint16]:disable or enable verbosity for received data (yes/no), except dynamic data");

        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected, State::kUnavailable)
            (bind(&StateMachineDrive::Disconnect, this))
            ("disconnect from ethernet");

        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected, State::kUnavailable)
            (bind(&StateMachineDrive::Reconnect, this, placeholders::_1))
            ("(Re)connect Ethernet connection to SPS, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");


        T::AddEvent("PRINT_POINTING_MODEL")
            (bind(&StateMachineDrive::PrintPointingModel, this))
            ("Print the ponting model.");


        T::AddEvent("PRINT")
            (bind(&StateMachineDrive::Print, this))
            ("Print source list.");

        T::AddEvent("RELOAD_SOURCES", State::kDisconnected, State::kConnected, State::kArmed, State::kInitialized, State::kLocked)
            (bind(&StateMachineDrive::ReloadSources, this))
            ("Reload sources from database after database has changed..");


        //fDrive.SetUpdateStatus(std::bind(&StateMachineDrive::UpdateStatus, this, placeholders::_1, placeholders::_2));
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

        T::Message("Loading sources from '"+db.uri()+"'");

        const mysqlpp::StoreQueryResult res =
            db.query("SELECT fSourceName, fRightAscension, fDeclination, fWobbleOffset, fWobbleAngle0, fWobbleAngle1, fMagnitude FROM Source").store();

        auto old = fSources;

        fSources.clear();
        for (vector<mysqlpp::Row>::const_iterator v=res.begin(); v<res.end(); v++)
        {
            const string name = (*v)[0].c_str();

            Source src;
            src.name = name;
            src.ra  = (*v)[1];
            src.dec = (*v)[2];
            src.offset = (*v)[3];
            src.angles[0] = (*v)[4];
            src.angles[1] = (*v)[5];
            src.mag = (*v)[6] ? double((*v)[6]) : 0;
            AddSource(name, src);

            ostringstream msg;
            msg << "> " << name << setprecision(8) << ":   Ra=" << src.ra << "h Dec=" << src.dec << "deg";
            msg << " Wobble=[" << src.offset << "," << src.angles[0] << "," << src.angles[1] << "] Mag=" << src.mag;

            const auto it = old.find(name);
            if (it==old.end())
               T::Message(" <a"+msg.str());
            else
            {
                if (print || it->second!=src)
                   T::Message(" <c"+msg.str());
                old.erase(it);
            }
        }

        for (auto it=old.begin(); it!=old.end(); it++)
        {
            const auto &src = it->second;

            ostringstream msg;
            msg << " <r> " << it->first << setprecision(8) << ":   Ra=" << src.ra << "h Dec=" << src.dec << "deg";
            msg << " Wobble=[" << src.offset << "," << src.angles[0] << "," << src.angles[1] << "] Mag=" << src.mag;
            T::Message(msg.str());
        }

        T::Message("Loaded "+to_string(fSources.size())+" sources from '"+db.uri()+"'");

#else
        T::Warn("MySQL support not compiled into the program.");
#endif
    }

    int EvalOptions(Configuration &conf)
    {
        if (!fSunRise)
            return 1;

        fDrive.SetVerbose(!conf.Get<bool>("quiet"));

        fMaxPointingResidual = conf.Get<double>("pointing.max.residual");
        fPointingVelocity    = conf.Get<double>("pointing.velocity");

        fPointingMin = Encoder(conf.Get<double>("pointing.min.zd"),
                               conf.Get<double>("pointing.min.az"));
        fPointingMax = Encoder(conf.Get<double>("pointing.max.zd"),
                               conf.Get<double>("pointing.max.az"));

        fParkingPos.zd = conf.Has("parking-pos.zd") ? conf.Get<double>("parking-pos.zd") : 90;
        fParkingPos.az = conf.Has("parking-pos.az") ? conf.Get<double>("parking-pos.az") :  0;
        fMaxParkingResidual = conf.Get<double>("parking-pos.residual");

        if (!CheckRange(fParkingPos))
            return 2;

        fAccPointing = Acceleration(conf.Get<double>("pointing.acceleration.zd"),
                                    conf.Get<double>("pointing.acceleration.az"));
        fAccTracking = Acceleration(conf.Get<double>("tracking.acceleration.zd"),
                                    conf.Get<double>("tracking.acceleration.az"));
        fAccMax      = Acceleration(conf.Get<double>("acceleration.max.zd"),
                                    conf.Get<double>("acceleration.max.az"));

        fWeatherTimeout = conf.Get<uint16_t>("weather-timeout");

        if (fAccPointing>fAccMax)
        {
            T::Error("Pointing acceleration exceeds maximum acceleration.");
            return 3;
        }

        if (fAccTracking>fAccMax)
        {
            T::Error("Tracking acceleration exceeds maximum acceleration.");
            return 4;
        }

        fApproachingLimit = conf.Get<float>("approaching-limit");

        fDeviationLimit   = conf.Get<uint16_t>("deviation-limit");
        fDeviationCounter = conf.Get<uint16_t>("deviation-count");
        fDeviationMax     = conf.Get<uint16_t>("deviation-max");

        const string fname = conf.GetPrefixedString("pointing.model-file");

        try
        {
            fPointingModel.Load(fname);
        }
        catch (const exception &e)
        {
            T::Error(e.what());
            return 5;
        }

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
                case 1: src.ra  = ReadAngle(is); break;
                case 2: src.dec = ReadAngle(is); break;
                case 3: is >> src.offset; break;
                case 4: is >> src.angles[0]; break;
                case 5: is >> src.angles[1]; break;
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

        //fAutoResume = conf.Get<bool>("auto-resume");

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
    po::options_description control("Drive control options");
    control.add_options()
        ("quiet,q",                  po_bool(),                 "Disable debug messages")
        ("no-dim,d",                 po_switch(),               "Disable dim services")
        ("addr,a",                   var<string>("sps:5357"),   "Network address of cosy")
        ("verbosity,v",              var<uint16_t>(0),          "Vervosity level (0=off; 1=major updates; 2=most updates; 3=frequent updates)")
        ("pointing.model-file",      var<string>()->required(), "Name of the file with the pointing model in use")
        ("pointing.max.zd",          var<double>( 104.9),       "Maximum allowed zenith angle in sky pointing coordinates [deg]")
        ("pointing.max.az",          var<double>(  85.0),       "Maximum allowed azimuth angle in sky pointing coordinates [deg]")
        ("pointing.min.zd",          var<double>(-104.9),       "Minimum allowed zenith angle in sky pointing coordinates [deg]")
        ("pointing.min.az",          var<double>(-295.0),       "Minimum allowed azimuth angle in sky pointing coordinates [deg]")
        ("pointing.max.residual",    var<double>(1./32768),     "Maximum residual for a pointing operation [revolutions]")
        ("pointing.velocity",        var<double>(0.3),          "Moving velocity when pointing [% max]")
        ("pointing.acceleration.az", var<double>(0.01),         "Acceleration for azimuth axis for pointing operations")
        ("pointing.acceleration.zd", var<double>(0.03),         "Acceleration for zenith axis for pointing operations")
        ("tracking.acceleration.az", var<double>(0.01),         "Acceleration for azimuth axis during tracking operations")
        ("tracking.acceleration.zd", var<double>(0.01),         "Acceleration for zenith axis during tracking operations")
        ("parking-pos.zd",           var<double>(101),          "Parking position zenith angle in sky pointing coordinates [deg]")
        ("parking-pos.az",           var<double>(0),            "Parking position azimuth angle in sky pointing coordinates [deg]")
        ("parking-pos.residual",     var<double>(0.5/360),      "Maximum residual for a parking position [revolutions]")
        ("acceleration.max.az",      var<double>(0.03),         "Maximum allowed acceleration value for azimuth axis")
        ("acceleration.max.zd",      var<double>(0.09),         "Maximum allowed acceleration value for zenith axis")
        ("weather-timeout",          var<uint16_t>(300),        "Timeout [sec] for weather data (after timeout default values are used)")
        ("approaching-limit",        var<float>(2.25),          "Limit to get 'Approaching' state")
        ("deviation-limit",          var<uint16_t>(90),         "Deviation limit in arcsec to get 'OnTrack'")
        ("deviation-count",          var<uint16_t>(3),          "Minimum number of reported deviation below deviation-limit to get 'OnTrack'")
        ("deviation-max",            var<uint16_t>(180),        "Maximum deviation in arcsec allowed to keep status 'OnTrack'")
        ("source-database",          var<string>(),             "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("source",                   vars<string>(),            "Additional source entry in the form \"name,hh:mm:ss,dd:mm:ss\"")
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
        "The drivectrl is an interface to the drive PLC.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: drivectrl [-c type] [OPTIONS]\n"
        "  or:  drivectrl [OPTIONS]\n";
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
