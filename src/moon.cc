#include <libnova/solar.h>
#include <libnova/lunar.h>
#include <libnova/rise_set.h>
#include <libnova/transform.h>

#include "Time.h"
#include "Configuration.h"

using namespace std;

// ------------------------------------------------------------------------

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

    int state;

    Moon() : time(Time::none)
    {
    }

    // Could be done more efficient: Only recalcuate if
    // the current time exceeds at least on of the stored times
    Moon(double lon, double lat, const Time &t=Time()) : time(t)
    {
        const double JD = time.JD();

        ln_lnlat_posn observer;
        observer.lng = lon;
        observer.lat = lat;

        ln_rst_time moon;
        ln_get_lunar_rst(JD-0.5, &observer, &moon);

        fRise    = Time(moon.rise);
        fTransit = Time(moon.transit);
        fSet     = Time(moon.set);

        //visible =
        //    ((JD>moon.rise && JD<moon.set ) && moon.rise<moon.set) ||
        //    ((JD<moon.set  || JD>moon.rise) && moon.rise>moon.set);

        const bool is_up      = JD>moon.rise;
        const bool is_sinking = JD>moon.transit;
        const bool is_dn      = JD>moon.set;

        ln_get_lunar_rst(JD+0.5, &observer, &moon);
        if (is_up)
            fRise = Time(moon.rise);
        if (is_sinking)
            fTransit = Time(moon.transit);
        if (is_dn)
            fSet = Time(moon.set);

        ln_equ_posn pos;
        ln_get_lunar_equ_coords(JD, &pos);

        ln_hrz_posn hrz;
        ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);
        az =    hrz.az;
        zd = 90-hrz.alt;

        ra  = pos.ra/15;
        dec = pos.dec;

        disk = ln_get_lunar_disk(JD)*100;
        state = 0;
        if (fRise   <fTransit && fRise   <fSet)     state = 0;  // not visible
        if (fTransit<fSet     && fTransit<fRise)    state = 1;  // before culm
        if (fSet    <fRise    && fSet    <fTransit) state = 2;  // after culm

        visible = state!=0;

        // 0: not visible
        // 1: visible before cul
        // 2: visible after cul
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
};

// ========================================================================
// ========================================================================
// ========================================================================

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Smart FACT");
    control.add_options()
        ("ra",        var<double>(), "Source right ascension")
        ("dec",       var<double>(), "Source declination")
        ("date-time", var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
                                   , "SQL time (UTC)")
        ;

    po::positional_options_description p;
    p.add("date-time", 1); // The first positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "moon - The moon calculator\n"
        "\n"
        "Usage: moon sql-datetime [--ra={ra} --dec={dec}]\n";
    cout << endl;
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    if (conf.Has("ra")^conf.Has("dec"))
    {
        cout << "ERROR - Either --ra or --dec missing." << endl;
        return 1;
    }

    Time time;
    time.SetFromStr(conf.Get<string>("date-time"));

    ln_lnlat_posn observer;
    observer.lng = -(17.+53./60+26.525/3600);
    observer.lat =   28.+45./60+42.462/3600; 

    Moon moon(observer.lng, observer.lat, time);

    cout << setprecision(15);
    cout << time.GetAsStr() << '\n';

    ln_equ_posn pos;
    ln_hrz_posn hrz;
    ln_get_solar_equ_coords(time.JD(), &pos);
    ln_get_hrz_from_equ(&pos, &observer, time.JD(), &hrz);
    cout << 90-hrz.alt   << '\n';

    const double   kSynMonth = 29.53058868; // synodic month (new Moon to new Moon)
    const double   kEpoch0   = 44240.37917; // First full moon after 1980/1/1
    const double   kInstall  = 393;         // Moon period if FACT installation
    const uint32_t period    = floor(((time.Mjd()-kEpoch0)/kSynMonth-kInstall));

    cout << period       << '\n';
    cout << moon.visible << '\n';
    cout << moon.disk    << '\n';
    cout << moon.zd      << '\n';

    if (conf.Has("ra") && conf.Has("dec"))
    {
        pos.ra  = conf.Get<double>("ra")*15;
        pos.dec = conf.Get<double>("dec");

        cout << moon.Angle(pos.ra/15, pos.dec) << '\n';

        // Trick 17
        moon.ra  = pos.ra;
        moon.dec = pos.dec;

        // Sun distance
        cout << moon.Angle(pos.ra/15, pos.dec) << '\n';
    }

    cout << endl;

    return 0;
}
