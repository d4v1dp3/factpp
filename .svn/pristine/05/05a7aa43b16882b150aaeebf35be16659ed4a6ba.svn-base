#include "Prediction.h"

#include "Database.h"

#include "Time.h"
#include "Configuration.h"

using namespace std;
using namespace Nova;

// ========================================================================
// ========================================================================
// ========================================================================

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Smart FACT");
    control.add_options()
        ("source-name", var<string>(), "Source name")
        ("date-time", var<string>(), "SQL time (UTC)")
        ("source-database", var<string>(""), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("max-current", var<double>(75), "Maximum current to display in other plots.")
        ("max-zd", var<double>(75), "Maximum zenith distance to display in other plots")
        ("no-limits", po_switch(), "Switch off limits in plots")
        ;

    po::positional_options_description p;
    p.add("source-name", 1); // The 1st positional options
    p.add("date-time",   2); // The 2nd positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "makedata - The astronomy data listing\n"
        "\n"
//        "Calculates several plots for the sources in the database\n"
//        "helpful or needed for scheduling. The Plot is always calculated\n"
//        "for the night which starts at the same so. So no matter if\n"
//        "you specify '1974-09-09 00:00:00' or '1974-09-09 21:56:00'\n"
//        "the plots will refer to the night 1974-09-09/1974-09-10.\n"
//        "The advantage is that specification of the date as in\n"
//        "1974-09-09 is enough. Time axis starts and ends at nautical\n"
//        "twilight which is 12deg below horizon.\n"
//        "\n"
        "Usage: makedata sql-datetime [--ra={ra} --dec={dec}]\n";
    cout << endl;
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;
/*
    if (!conf.Has("source-name"))
    {
        cout << "ERROR - --source-name missing." << endl;
        return 1;
    }
*/
    // ------------------ Eval config ---------------------

    Time time;
    if (conf.Has("date-time"))
        time.SetFromStr(conf.Get<string>("date-time"));

    const double max_current = conf.Get<double>("max-current");
    const double max_zd      = conf.Get<double>("max-zd");
    const double no_limits   = conf.Get<bool>("no-limits");

    // -12: nautical
    const RstTime sun_set  = GetSolarRst(time.JD()-0.5, -12);   // Sun set with the same date than th provided date
    const RstTime sun_rise = GetSolarRst(time.JD()+0.5, -12);  // Sun rise on the following day

    const double jd  = floor(time.Mjd())+2400001;
    const double mjd = floor(time.Mjd())+49718+0.5;


    const double jd0 = fmod(sun_set.set,   1);   // ~0.3
    const double jd1 = fmod(sun_rise.rise, 1);   // ~0.8

    cout << Time::iso << time << ", " << mjd-49718 << ", ";
    cout << jd0  << ", ";
    cout << jd1 << "\n";

    if (!conf.Has("source-name"))
        return 1;

    const string source_name = conf.Get<string>("source-name");
    const string fDatabase   = conf.Get<string>("source-database");

    // ------------- Get Sources from databasse ---------------------

    const mysqlpp::StoreQueryResult res =
        Database(fDatabase).query("SELECT fRightAscension, fDeclination FROM Source WHERE fSourceName='"+source_name+"'").store();

    // ------------- Create canvases and frames ---------------------

    vector<mysqlpp::Row>::const_iterator row=res.begin();
    if (row==res.end())
        return 1;

    EquPosn pos;
    pos.ra  = double((*row)[0])*15;
    pos.dec = double((*row)[1]);

    // Loop over 24 hours
    for (int i=0; i<24*12; i++)
    {
        const double h = double(i)/(24*12);

        // check if it is betwene sun-rise and sun-set
        if (h<jd0 || h>jd1)
            continue;

        const SolarObjects so(jd+h);

        // get local position of source (note: az is south aligned!)
        const HrzPosn hrz = GetHrzFromEqu(pos, jd+h);

        // get current prediction
        const double cur = FACT::PredictI(so, pos);

        // Relative  energy threshold prediction
        const double ratio = pow(cos((90-hrz.alt)*M_PI/180), -2.664);

        // Add points to curve
        // const double axis = (mjd+h)*24*3600;

        Time t(mjd-49718);
        t += boost::posix_time::minutes(i*5);

        cout << t << ", " << h << ", ";

        if (no_limits || cur<max_current)
            cout << hrz.alt;
        cout << ", ";

        if (no_limits || 90-hrz.alt<max_zd)
            cout << cur;
        cout << ", ";

        if (no_limits || (cur<max_current && 90-hrz.alt<max_zd))
            cout << ratio*cur/6.2;
        cout << ", ";

        if (no_limits || (cur<max_current && 90-hrz.alt<max_zd))
            cout << GetAngularSeparation(so.fMoonEqu, pos);
        cout << "\n";
    }

    cout << flush;

    return 0;
}
