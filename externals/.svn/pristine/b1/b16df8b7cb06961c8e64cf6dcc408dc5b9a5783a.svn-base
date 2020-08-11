#ifndef MARS_Nova
#define MARS_Nova

#include <map>
#include <cmath>
#include <string>
#include <limits>
#include <algorithm>

#include <libnova/solar.h>
#include <libnova/lunar.h>
#include <libnova/rise_set.h>
#include <libnova/transform.h>
#include <libnova/angular_separation.h>

namespace Nova
{
    typedef ln_rst_time RstTime; // Note that times are in JD not Mjd

    struct ZdAzPosn;
    struct RaDecPosn;

#ifndef __CINT__
#define OBS(lat,lng) ={lat,lng}
#else
#define OBS(lat,lng)
#endif

#ifndef PRESET_OBSERVATORY
#define PRESET_OBSERVATORY kORM
#endif

    const static double kSolarStandardHorizon = LN_SOLAR_STANDART_HORIZON;

    const static ln_lnlat_posn kORM  OBS(-(17.+53./60+26.525/3600), 28.+45./60+42.462/3600); // 2800m
    const static ln_lnlat_posn kHAWC OBS( -97.3084,                 18.9947               ); // 4100m;
    const static ln_lnlat_posn kSPM  OBS(-115.4637,                 31.0439               ); // 2800m;
    const static ln_lnlat_posn kRWTH OBS(   6.0657,                 50.7801               );

    struct LnLatPosn : public ln_lnlat_posn
    {
        typedef std::map<std::string, ln_lnlat_posn> LUT;

        bool operator==(const LnLatPosn &eq) const
        {
            return lng==eq.lng && lat==eq.lat;
        }

        static const LUT &lut()
        {
#ifndef __CINT__
            static const LUT lut =
            {
                { "ORM",  kORM  },
                { "HAWC", kHAWC },
                { "SPM",  kSPM  },
                { "RWTH", kRWTH },
            };
            return lut;
#else
            return LUT();
#endif
        }

        LnLatPosn() { }
        LnLatPosn(const ln_lnlat_posn &pos) : ln_lnlat_posn(pos) { }
        LnLatPosn(std::string obs)
        {
#ifndef __CINT__
            if (obs.empty())
            {
                *this = PRESET_OBSERVATORY;
                return;
            }

            for (auto it=obs.begin(); it!=obs.end(); it++)
                *it = toupper(*it);

            const auto it = lut().find(obs);
            *this = it==lut().end() ? ln_lnlat_posn({std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::quiet_NaN()}) : it->second;
#endif
        }
        bool isValid() const
        {
            return std::isfinite(lat) && std::isfinite(lng);
        }

        const std::string &name() const
        {
            for (auto it=lut().begin(); it!=lut().end(); it++)
                if (LnLatPosn(it->second)==*this)
                    return it->first;

            static const std::string dummy;
            return dummy;
        }

        static const std::string preset()
        {
            return LnLatPosn(PRESET_OBSERVATORY).name();
        }
    };

    // Warning: 0deg=South, 90deg=W
    struct HrzPosn : public ln_hrz_posn
    {
        HrzPosn() { }
        HrzPosn(const ZdAzPosn &);
    };

    struct ZdAzPosn
    {
        double zd; // [deg]
        double az; // [deg]

        ZdAzPosn(double z=0, double a=0) : zd(z), az(a) { }
        ZdAzPosn(const HrzPosn &hrz) : zd(90-hrz.alt), az(hrz.az-180) { }

        // This crahsed cint, but it could save up the dedicate structure HrzPosn :(
        //operator HrzPosn() const { const HrzPosn p = { az-180, 90-zd}; return p; }
    };

    inline HrzPosn::HrzPosn(const ZdAzPosn &za) { alt = 90-za.zd; az = za.az-180; }


    // Note that right ascension is stored in degree
    struct EquPosn : public ln_equ_posn
    {
        EquPosn() { }
        EquPosn(const RaDecPosn &);
    };

    struct RaDecPosn
    {
        double ra;  // [h]
        double dec; // [deg]

        RaDecPosn(double r=0, double d=0) : ra(r), dec(d) { }
        RaDecPosn(const EquPosn &equ) : ra(equ.ra/15), dec(equ.dec) { }

        // This crahsed cint, but it could save up the dedicate structure HrzPosn :(
        //operator HrzPosn() const { const HrzPosn p = { az-180, 90-zd}; return p; }
    };

    inline EquPosn::EquPosn(const RaDecPosn &rd) { ra = rd.ra*15; dec = rd.dec; }

    inline HrzPosn GetHrzFromEqu(const EquPosn &equ, const LnLatPosn &obs, const double &jd)
    {
        HrzPosn hrz;
        ln_get_hrz_from_equ(const_cast<EquPosn*>(&equ), const_cast<LnLatPosn*>(&obs), jd, &hrz);
        return hrz;
    }
    inline HrzPosn GetHrzFromEqu(const EquPosn &equ, const double &jd)
    {
        return GetHrzFromEqu(equ, PRESET_OBSERVATORY, jd);
    }

    inline EquPosn GetEquFromHrz(const HrzPosn &hrz, const LnLatPosn &obs, const double &jd)
    {
        EquPosn equ;
        ln_get_equ_from_hrz(const_cast<HrzPosn*>(&hrz), const_cast<LnLatPosn*>(&obs), jd, &equ);
        return equ;
    }
    inline EquPosn GetEquFromHrz(const HrzPosn &hrz, const double &jd)
    {
        return GetEquFromHrz(hrz, PRESET_OBSERVATORY, jd);
    }

    inline RstTime GetObjectRst(const EquPosn &equ, const LnLatPosn &obs, const double &jd, const double &hrz=0)
    {
        RstTime rst;
        // 0 for success, 1 for circumpolar (above the horizon), -1 for circumpolar (bellow the horizon)
        ln_get_object_next_rst_horizon(jd, const_cast<LnLatPosn*>(&obs), const_cast<EquPosn*>(&equ), hrz, &rst);
        return rst;
    }
    inline RstTime GetObjectRst(const EquPosn &equ, const double &jd, const double &hrz=0)
    {
        return GetObjectRst(equ, PRESET_OBSERVATORY, jd, hrz);
    }

    inline int GetObjectRst(RstTime &rst, const EquPosn &equ, const LnLatPosn &obs, const double &jd, const double &hrz=0)
    {
        // 0 for success, 1 for circumpolar (above the horizon), -1 for circumpolar (bellow the horizon)
        return ln_get_object_next_rst_horizon(jd, const_cast<LnLatPosn*>(&obs), const_cast<EquPosn*>(&equ), hrz, &rst);
    }
    inline int GetObjectRst(RstTime &rst, const EquPosn &equ, const double &jd, const double &hrz=0)
    {
        // 0 for success, 1 for circumpolar (above the horizon), -1 for circumpolar (bellow the horizon)
        return GetObjectRst(rst, equ, PRESET_OBSERVATORY, jd, hrz);
    }

    inline RstTime GetSolarRst(const double &jd, const LnLatPosn &obs, const double &hrz=kSolarStandardHorizon)
    {
        RstTime rst;
        ln_get_solar_rst_horizon(jd, const_cast<LnLatPosn*>(&obs), hrz, &rst);
        return rst;
    }

    inline RstTime GetSolarRst(const double &jd, const double &hrz=kSolarStandardHorizon)
    {
        return GetSolarRst(jd, PRESET_OBSERVATORY, hrz);
    }

    inline RstTime GetLunarRst(const double &jd, const LnLatPosn &obs=PRESET_OBSERVATORY)
    {
        RstTime rst;
        ln_get_lunar_rst(jd, const_cast<LnLatPosn*>(&obs), &rst);
        return rst;
    }
    inline EquPosn GetSolarEquCoords(const double &jd)
    {
        EquPosn equ;
        ln_get_solar_equ_coords(jd, &equ);
        return equ;
    }

    inline double GetLunarDisk(const double &jd)
    {
        return ln_get_lunar_disk(jd);
    }

    inline double GetLunarSdiam(const double &jd)
    {
        return ln_get_lunar_sdiam(jd);
    }

    inline double GetLunarPhase(const double &jd)
    {
        return ln_get_lunar_phase(jd);
    }

    inline EquPosn GetLunarEquCoords(const double &jd, double precision=0)
    {
        EquPosn equ;
        ln_get_lunar_equ_coords_prec(jd, &equ, precision);
        return equ;
    }

    inline double GetLunarEarthDist(const double &jd)
    {
        return ln_get_lunar_earth_dist(jd);
    }

    inline double GetAngularSeparation(const EquPosn &p1, const EquPosn &p2)
    {
        return ln_get_angular_separation(const_cast<EquPosn*>(&p1), const_cast<EquPosn*>(&p2));
    }

    inline double GetAngularSeparation(const HrzPosn &h1, const HrzPosn &h2)
    {
        EquPosn p1; p1.ra=h1.az; p1.dec=h1.alt;
        EquPosn p2; p2.ra=h2.az; p2.dec=h2.alt;
        return ln_get_angular_separation(&p1, &p2);
    }

    struct SolarObjects
    {
        double  fJD;

        EquPosn fSunEqu;
        HrzPosn fSunHrz;

        EquPosn fMoonEqu;
        HrzPosn fMoonHrz;
        double  fMoonDisk;

        double fEarthDist;

        SolarObjects() { }

        SolarObjects(const double &jd, const LnLatPosn &obs=Nova::PRESET_OBSERVATORY)
        {
            fJD = jd;

            // Sun properties
            fSunEqu    = GetSolarEquCoords(jd);
            fSunHrz    = GetHrzFromEqu(fSunEqu, obs, jd);

            // Moon properties
            fMoonEqu   = GetLunarEquCoords(jd, 0.01);
            fMoonHrz   = GetHrzFromEqu(fMoonEqu, obs, jd);

            fMoonDisk  = GetLunarDisk(jd);
            fEarthDist = GetLunarEarthDist(jd);
        }
    };
}

#endif
