#include "Database.h"

#include "pal.h"
#include "nova.h"
#include "tools.h"
#include "Time.h"
#include "Configuration.h"

#include <TROOT.h>
#include <TVector3.h>
#include <TRotation.h>

using namespace std;

// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Calcsource options");
    control.add_options()
        ("uri,u",          var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
         , "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        //("source-key",     var<uint32_t>(5),                "")
        //("source-name",    var<string>(""),               "")
        ("file",           var<uint32_t>(uint32_t(0)),"FileId (YYMMDDXXX), defined the events to be processed (if omitted (=0), a list of possible numbers is printed)")
        ("table.events",   var<string>("Events"),     "Name of the table where the events are stored")
        ("table.runinfo",  var<string>("RunInfo"),    "Name of the table where the run-info data is stored")
        ("table.source",   var<string>("Source"),     "Name of the table where the sources are stored")
        ("table.position", var<string>("Position"),   "Name of the table where the calculated posiiton will be stored")
        ("engine",         var<string>(""),           "Database engine to be used when a new table is created")
        ("row-format",     var<string>(""),           "Defines the ROW_FORMAT keyword for table creation")
        ("ignore-errors",  po_switch(),               "Adds the IGNORE keyword to the INSERT query (turns errors into warnings, ignores rows with errors)")
        ("force",          po_switch(),               "Force processing even if there is no database connection")
        ("drop",           po_switch(),               "Drop the table (implies create)")
        ("create",         po_switch(),               "Create the table if it does not yet exist")
        ("update",         po_switch(),               "Uses the ON DUPLICATE KEY to update already existing extries")
        ("ra",             var<double>(),             "Right ascension of the source (use together with --dec)")
        ("dec",            var<double>(),             "Declination of the source (use together with --ra)")
        ("focal-dist",     var<double>(4889.),        "Focal distance of the camera in millimeter")
        ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("no-insert",      po_switch(),               "Does not insert or update any data to any table")
        ("dry-run",        po_switch(),               "Skip any query which changes the databse (might result in consecutive failures)")
        ("print-meta",     po_switch(),               "Print meta-queries (DROP, CREATE, DELETE, SELECT)")
        ("print-insert",   po_switch(),               "Print the INSERT/UPDATE queries")
        ("verbose,v",      var<uint16_t>(1),          "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    po::positional_options_description p;
    p.add("file", 1); // The 1st positional options (n=1)

    conf.AddOptions(control);
    conf.AddOptions(debug);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "calcsource - Fill a table with source positions\n"
        "\n"
        "This tool is to calculate the source position in the camera and fill "
        "it into a database table for all events which come from a single run. "
        "The run is idetified by its FileId (YYMMDDXXX), where YYMMDD is "
        "its date and XXX is the run-number. For this run the corresponding "
        "source key `fSourceKey` is obtained from the RunInfo table "
        "(--table.runinfo) and the corresponding fRightAscension and fDeclination "
        "from the Source table (--table.source). If --ra and --dec was specified "
        "by the user, this step is skipped and these two values are used instead."
        "\n\n"
        "Then for each event with the given FileId, its pointing position "
        "(`Ra`, `Dec`), its time (`MJD`, `MilliSec`) and its event number "
        "(`EvtNumber`) are requested from the table given by --table.events. "
        "Based on this information and the focal distance (--focal-distance) the "
        "`X` and `Y` coordinate of the source in the camera plane is calculated. "
        "The result can then be filled into a database."
        "\n\n"
        "The table to be filled or updated should contain the following columns:\n"
        "   - FileId INT UNSIGNED NOT NULL\n"
        "   - EvtNumber INT UNSIGNED NOT NULL\n"
        "   - X FLOAT NOT NULL\n"
        "   - Y FLOAT NOT NULL\n"
        "\n"
        "If the table does not exist, it can be created automatically specifying "
        "the --crate option. If a table already exists and it should be dropped "
        "before creation, the --drop option can be used:\n"
        "\n"
        "   calcsource 170909009 --create\n"
        "\n"
        "Process the data from the run 009 from 09/09/2017. If no table exists "
        "a table with the name given by --table.position is created.\n"
        "\n"
        "   calcsource 170909009 --create --drop\n"
        "\n"
        "Same as before, but if a table with the name given by --table.position "
        "exists, it is dropped before.\n"
        "\n"
        "For each event, a new row is inserted. If existing rows should be updated, "
        "use:\n"
        "\n"
        "   calcsource 170909009 --updated\n"
        "\n"
        "The --create option is compatible with that. The --drop option is ignored.\n"
        "\n"
        "To avoid failure in case an entry does already exist, you can add the IGNORE "
        "keyword to the INSERT query by --ignore-errors, which essentially ignores "
        "all errors and turns them into warnings which are printed after the query "
        "succeeded.\n"
        "\n"
        "\n"
        "For debugging purposes several print options and options to avoid irreversible "
        "changes to the database exist."
        "\n\n"
        "Usage: calcsource YYMMDDXXX [-u URI] [options]\n"
        "\n"
        ;
    cout << endl;
}

class MRotation : public TRotation
{
public:
    MRotation() : TRotation(1, 0, 0, 0, -1, 0, 0, 0, 1)
    {
    }
};


int main(int argc, const char* argv[])
{
    Time start;

    gROOT->SetBatch();

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    // ----------------------------- Evaluate options --------------------------
    if (conf.Has("ra")^conf.Has("dec"))
        throw runtime_error("--ra and --dec can only be used together");

    const bool has_radec = conf.Has("ra") && conf.Has("dec");

    double source_ra            = conf.Has("ra")  ? conf.Get<double>("ra")  : 0;
    double source_dec           = conf.Has("dec") ? conf.Get<double>("dec") : 0;

    //string  source_name      = conf.Get<string>("source-name");
    //uint32_t source_key      = conf.Has("source-key")  ? conf.Get<uint32_t>("source-key") : 0;

    const string   uri          = conf.Get<string>("uri");
    const string   tab_events   = conf.Get<string>("table.events");
    const string   tab_runinfo  = conf.Get<string>("table.runinfo");
    const string   tab_source   = conf.Get<string>("table.source");
    const string   tab_position = conf.Get<string>("table.position");

    const string engine         = conf.Get<string>("engine");
    const string row_format     = conf.Get<string>("row-format");
    const bool ignore_errors    = conf.Get<bool>("ignore-errors");

    const uint32_t file         = conf.Get<uint32_t>("file");

    const double   focal_dist   = conf.Get<double>("focal-dist");

    const bool     print_meta   = conf.Get<bool>("print-meta");
    const bool     print_insert = conf.Get<bool>("print-insert");

    const bool     force        = conf.Get<bool>("force");
    const bool     drop         = conf.Get<bool>("drop");
    const bool     create       = conf.Get<bool>("create") || drop;
    const bool     update       = conf.Get<bool>("update");
    const bool     noinsert     = conf.Get<bool>("no-insert");
    const bool     dry_run      = conf.Get<bool>("dry-run");
    const uint16_t verbose      = conf.Get<uint16_t>("verbose");

    // -------------------------------------------------------------------------
    // Checking for database connection

    Database connection(uri); // Keep alive while fetching rows

    try
    {
        if (!force)
            connection.connected();
    }
    catch (const exception &e)
    {
        cerr << "SQL connection failed: " << e.what() << endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // list file-ids in the events table

    if (file==0)
    {
        const string query =
            "SELECT FileId FROM `"+tab_events+"` GROUP BY FileId";

        if (print_meta)
            cout << query << endl;

        const mysqlpp::StoreQueryResult res =
            connection.query(query).store();

        for (size_t i=0; i<res.num_rows(); i++)
            cout << "calcsource " << res[i][0] << '\n';
        cout << endl;

        return 0;
    }

    // -------------------------------------------------------------------------
    // retrieve source from the database

    if (verbose>0)
    {
        cout << "\n------------------------- Evaluating source ------------------------" << endl;
        cout << "Requesting coordinates from " << tab_runinfo << "/" << tab_source << " table for 20" << file/1000 << "/" << file%1000 << endl;
    }

    double point_ra  = 0;
    double point_dec = 0;

    const string query =
        "SELECT "
        " `"+tab_runinfo+"`.fRightAscension, `"+tab_runinfo+"`.fDeclination, "
        " `"+tab_source+"`.fRightAscension, `"+tab_source+"`.fDeclination, "
        " `"+tab_source+"`.fSourceName"
        " FROM `"+tab_runinfo+"`"+
        " LEFT JOIN `"+tab_source+"`"+
        " USING (fSourceKey)"
        " WHERE fNight=20"+to_string(file/1000)+
        " AND fRunID="+to_string(file%1000);

    if (print_meta)
        cout << query << endl;

    try
    {
        const mysqlpp::StoreQueryResult res =
            connection.query(query).store();

        if (res.num_rows()!=1)
        {
            cerr << "No coordinates from " << tab_runinfo << " for " << file << endl;
            return 2;
        }

        point_ra  = res[0][0];
        point_dec = res[0][1];

        if (!has_radec)
        {
            source_ra  = res[0][2];
            source_dec = res[0][3];
        }

        if (verbose>0)
        {
            cout << "Using coordinates " << source_ra << "h / " << source_dec << " deg ";
            if (has_radec)
                cout << "for '" << res[0][4] << "'" << endl;
            else
                cout << "from resources." << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << query << "\n";
        cerr << "SQL query failed:\n" << e.what() << endl;
        return 3;
    }

/*
    if (!source_name.empty())
    {
        cout << "Requesting coordinates for '" << source_name << "'" << endl;

        const mysqlpp::StoreQueryResult res =
            connection.query("SELECT `Ra`, `Dec` WHERE fSourceName='"+source_name+"'").store();

        if (res.num_rows()!=1)
        {
            cerr << "No " << (res.num_rows()>1?"unique ":"") << "coordinates found for '" << source_name << "'" << endl;
            return 1;
        }

        source_ra  = res[0][0];
        source_dec = res[0][1];
    }
*/

    // -------------------------------------------------------------------------
    // create INSERT/UPDATE query (calculate positions)

    if (verbose>0)
    {
        cout << "\n-------------------------- Evaluating file ------------------------";
        cout << "\nRequesting data from table `" << tab_events << "`" << endl;
    }

    const string query1 =
        "SELECT EvtNumber, MJD, MilliSec"
        " FROM `"+tab_events+"`"
        " WHERE FileId="+to_string(file);

    if (print_meta)
        cout << query1 << endl;

    const mysqlpp::UseQueryResult res1 =
        connection.query(query1).use();

    const Nova::RaDecPosn source(source_ra, source_dec);

    //source_ra  *= M_PI/12;
    //source_dec *= M_PI/180;

    const Nova::RaDecPosn point(point_ra, point_dec);

    //point_ra  *= M_PI/12;
    //point_dec *= M_PI/180;

    //const auto obs = Nova::kORM;

    //obs.lng *= M_PI/180;
    //obs.lat *= M_PI/180;

    ostringstream ins;
    ins << setprecision(16);

    size_t count = 0;
    while (auto row=res1.fetch_row())
    {
        count++;

        const uint32_t event     = row[0];
        const uint32_t mjd       = row[1];
        const int64_t  millisec  = row[2];

        /*
         // ============================ Mars ================================

        TVector3 pos;  // pos: source position
        TVector3 pos0;  // pos: source position

        pos.SetMagThetaPhi(1, M_PI/2-source_dec, source_ra);
        pos0.SetMagThetaPhi(1, M_PI/2-point_dec, point_ra);

        const double ut = (nanosec/1e6+millisec)/(24*3600000);

        // Julian centuries since J2000.
        const double t = (ut -(51544.5-mjd)) / 36525.0;

        // GMST at this UT1
        const double r1 = 24110.54841+(8640184.812866+(0.093104-6.2e-6*t)*t)*t;
        const double r2 = 86400.0*ut;

        const double sum = (r1+r2)/(3600*24);

        double gmst = fmod(sum, 1) * 2*M_PI;

        MRotation conv;
        conv.RotateZ(gmst + obs.lng);
        conv.RotateY(obs.lat-M_PI/2);
        conv.RotateZ(M_PI);

        pos  *= conv;
        pos0 *= conv;

        pos.RotateZ(-pos0.Phi());
        pos.RotateY(-pos0.Theta());
        pos.RotateZ(-M_PI/2); // exchange x and y
        pos *= -focal_dist/pos.Z();

        TVector2 v = pos.XYvector();

        //if (fDeviation)
        //    v -= fDeviation->GetDevXY()/fGeom->GetConvMm2Deg();

        //cout << v.X() << " " << v.Y() << " " << v.Mod()*mm2deg << '\n';
        */

        // ================================= Nova ===============================

        Nova::ZdAzPosn ppos  = Nova::GetHrzFromEqu(source, 2400000.5+mjd+millisec/1000./3600/24);
        Nova::ZdAzPosn ppos0 = Nova::GetHrzFromEqu(point,  2400000.5+mjd+millisec/1000./3600/24);

        TVector3 pos;
        TVector3 pos0;
        pos.SetMagThetaPhi( 1, ppos.zd *M_PI/180, ppos.az *M_PI/180);
        pos0.SetMagThetaPhi(1, ppos0.zd*M_PI/180, ppos0.az*M_PI/180);

        pos.RotateZ(-pos0.Phi());
        pos.RotateY(-pos0.Theta());
        pos.RotateZ(-M_PI/2); // exchange x and y
        pos *= -focal_dist/pos.Z();

        TVector2 v = pos.XYvector();

        //cout << v.X() << " " << v.Y() << " " << v.Mod()*mm2deg << '\n';

        /*
        // =============================== Slalib ==========================
        const double height = 2200;
        const double temp   = 10;
        const double hum    = 0.25;
        const double press  = 780;

        const double _mjd = mjd+millisec/1000./3600/24;

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
                 obs.lng, obs.lat, height,     // long, lat, height
                 0, 0,                         // polar motion x, y-coordinate (radians)
                 273.155+temp, press, hum,     // temp, pressure, humidity
                 0.40, 0.0065,                 // wavelength, tropo lapse rate
                 fAoprms);

        // ---- Mean to apparent ----
        double r=0, d=0;
        palMapqkz(point_ra, point_dec, fAmprms, &r, &d);

        double _zd, _az, ha, ra, dec;
        // -- apparent to observed --
        palAopqk(r, d, fAoprms,
                 &_az,  // observed azimuth (radians: N=0,E=90) [-pi,   pi]
                 &_zd,  // observed zenith distance (radians)   [-pi/2, pi/2]
                 &ha,   // observed hour angle (radians)
                 &dec,  // observed declination (radians)
                 &ra);  // observed right ascension (radians)

        //cout << _zd*180/M_PI << " " << _az*180/M_PI << endl;

        pos0.SetMagThetaPhi(1, _zd, _az);

        r=0, d=0;
        palMapqkz(source_ra, source_dec, fAmprms, &r, &d);

        // -- apparent to observed --
        palAopqk(r, d, fAoprms,
                 &_az,  // observed azimuth (radians: N=0,E=90) [-pi, pi]
                 &_zd,  // observed zenith distance (radians)   [-pi/2, pi/2]
                 &ha,   // observed hour angle (radians)
                 &dec,  // observed declination (radians)
                 &ra);  // observed right ascension (radians)

        pos.SetMagThetaPhi(1, _zd, _az);

        //cout << _zd*180/M_PI << " " << _az*180/M_PI << endl;

        pos.RotateZ(-pos0.Phi());
        pos.RotateY(-pos0.Theta());
        pos.RotateZ(-M_PI/2); // exchange x and y
        pos *= -focal_dist/pos.Z();

        v = pos.XYvector();

        //cout << v.X() << " " << v.Y() << " " << v.Mod()*mm2deg << '\n';
        */

        ins << "( " << file << ", " << event << ", " << v.X() << ", " << v.Y() << " ),\n";
    }

    if (connection.errnum())
    {
        cerr << "SQL error fetching row: " << connection.error() << endl;
        return 4;
    }

    if (verbose>0)
        cout << "Processed " << count << " events.\n" << endl;

    if (count==0)
    {
        if (verbose>0)
            cout << "Total execution time: " << Time().UnixTime()-start.UnixTime() << "s\n" << endl;
        return 0;
    }

    // -------------------------------------------------------------------------
    // drop table if requested

    if (drop)
    {
        try
        {
            if (verbose>0)
                cout << "Dropping table `" << tab_position << "`" << endl;

            if (!dry_run)
                connection.query("DROP TABLE `"+tab_position+"`").execute();

            if (verbose>0)
                cout << "Table `" << tab_position << "` dropped.\n" << endl;
        }
        catch (const exception &e)
        {
            cerr << "DROP TABLE `" << tab_position << "`\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 5;
        }
    }

    // -------------------------------------------------------------------------
    // crate table if requested

    if (create)
    {
        if (verbose>0)
            cout << "Creating table `" << tab_position << "`" << endl;

        string query2 =
            "CREATE TABLE IF NOT EXISTS `"+tab_position+"`\n"
            "(\n"
            "   FileId INT UNSIGNED NOT NULL,\n"
            "   EvtNumber INT UNSIGNED NOT NULL,\n"
            "   X FLOAT NOT NULL,\n"
            "   Y FLOAT NOT NULL,\n"
            "   PRIMARY KEY (FileId, EvtNumber)\n"
            ")\n"
            "DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci\n";
        if (!engine.empty())
            query2 += "ENGINE="+engine+"\n";
        if (!row_format.empty())
            query2 += "ROW_FORMAT="+row_format+"\n";
        query2 += "COMMENT='created by "+conf.GetName()+"'\n";

        try
        {
            if (!dry_run)
                connection.query(query2).execute();
        }
        catch (const exception &e)
        {
            cerr << query2 << "\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 6;
        }

        if (print_meta)
            cout << query2 << endl;

        if (verbose>0)
            cout << "Table `" << tab_position << "` created.\n" << endl;
    }

    // -------------------------------------------------------------------------
    // delete old entries from table

    if (!drop)
    {
        if (verbose>0)
            cout << "Deleting old entries from table `" << tab_position << "`" << endl;

        const string query2 =
            "DELETE FROM `"+tab_position+"` WHERE FileId="+to_string(file);

        try
        {
            if (!dry_run)
            {
                const mysqlpp::SimpleResult res =
                    connection.query(query2).execute();

                if (verbose>0)
                    cout << res.rows() << " row(s) affected.\n" << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << query2 << "\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 7;
        }

        if (print_meta)
            cout << query2 << endl;
    }

    // -------------------------------------------------------------------------
    // insert data into table

    if (verbose>0)
        cout << "Inserting data into table " << tab_position << "." << endl;

    string query2 = "INSERT ";
    if (ignore_errors)
        query2 += "IGNORE ";
    query2 += "`"+tab_position+"` (FileId, EvtNumber, X, Y) VALUES\n"+
        ins.str().substr(0, ins.str().size()-2)+
        "\n";
    if (update)
        query2 += "ON DUPLICATE KEY UPDATE X=VALUES(X), Y=VALUES(Y)\n";

    try
    {
        if (!noinsert)
        {
            const mysqlpp::SimpleResult res =
                connection.query(query2).execute();

            if (verbose>0)
                cout << res.info() << '\n' << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << query2 << "\n\n";
        cerr << "SQL query (" << query2.length() << " bytes) failed:\n" << e.what() << endl;
        return 8;
    }

    if (print_insert)
        cout << query2 << endl;

    if (verbose>0)
    {
        const auto sec = Time().UnixTime()-start.UnixTime();
        cout << "Total execution time: " << sec << "s ";
        cout << "(" << Tools::Fractional(sec/count) << "s/row)\n";

        try
        {
            const auto resw =
                connection.query("SHOW WARNINGS").store();

            for (size_t i=0; i<resw.num_rows(); i++)
            {
                const mysqlpp::Row &roww = resw[i];

                cout << roww["Level"] << '[' << roww["Code"] << "]: ";
                cout << roww["Message"] << '\n';
            }
            cout << endl;

        }
        catch (const exception &e)
        {
            cerr << "\nSHOW WARNINGS\n\n";
            cerr << "SQL query failed:\n" << e.what() << '\n' <<endl;
            return 8;
        }
    }

    return 0;
}

/*
 TRotation & TRotation::RotateX(Double_t a) {
   //rotate around x
   Double_t c = TMath::Cos(a);
   Double_t s = TMath::Sin(a);
   Double_t x = fyx, y = fyy, z = fyz;
   fyx = c*x - s*fzx;
   fyy = c*y - s*fzy;
   fyz = c*z - s*fzz;
   fzx = s*x + c*fzx;
   fzy = s*y + c*fzy;
   fzz = s*z + c*fzz;
   return *this;
}

TRotation & TRotation::RotateY(Double_t a){
   //rotate around y
   Double_t c = TMath::Cos(a);
   Double_t s = TMath::Sin(a);
   Double_t x = fzx, y = fzy, z = fzz;
   fzx = c*x - s*fxx;
   fzy = c*y - s*fxy;
   fzz = c*z - s*fxz;
   fxx = s*x + c*fxx;
   fxy = s*y + c*fxy;
   fxz = s*z + c*fxz;
   return *this;
}

TRotation & TRotation::RotateZ(Double_t a) {
   //rotate around z
   Double_t c = TMath::Cos(a);
   Double_t s = TMath::Sin(a);
   Double_t x = fxx, y = fxy, z = fxz;
   fxx = c*x - s*fyx;
   fxy = c*y - s*fyy;
   fxz = c*z - s*fyz;
   fyx = s*x + c*fyx;
   fyy = s*y + c*fyy;
   fyz = s*z + c*fyz;
   return *this;
}

 */


// Reuired:
//     pointing_ra[8]
//     pointing_dec[8]
//     fNanoSec[4], fTime[fMiliSec,8], fMjd[4]
//     source_ra               -> rc
//     source_dec              -> rc
//     fLong, fLat             -> rc
//     fCameraDist             -> rc
// 32 byte per row, 1 Monat = 4mio rows  => 120 MB

/*

void TVector3::SetThetaPhi(Double_t theta, Double_t phi)
{
   //setter with mag, theta, phi
   fX = TMath::Sin(theta) * TMath::Cos(phi);
   fY = TMath::Sin(theta) * TMath::Sin(phi);
   fZ = TMath::Cos(theta);
}


    // Set Sky coordinates of source, taken from container "MSourcePos"
    // of type MPointingPos. The sky coordinates must be J2000, as the
    // sky coordinates of the camera center that we get from the container
    // "MPointingPos" filled by the Drive.
    //MVector3 pos;  // pos: source position
    //pos.SetRaDec(fSourcePos->GetRaRad(), fSourcePos->GetDecRad());

    TVector3 pos;  // pos: source position
    pos.SetMagThetaPhi(1, TMath::Pi()/2-source_dec, source_ra);

    // Set sky coordinates of camera center in pos0 (for details see below)
    //MVector3 pos0;  // pos0: camera center
    //pos0.SetRaDec(fPointPos->GetRaRad(), fPointPos->GetDecRad());

    TVector3 pos0;  // pos: source position
    pos0.SetMagThetaPhi(1, TMath::Pi()/2-pointing_dec, pointing_ra);

    // Convert sky coordinates of source to local coordinates. Warning! These are not the "true" local
    // coordinates, since this transformation ignores precession and nutation effects.

    //const MAstroSky2Local conv(*fTime, *fObservatory);
    //pos *= conv;

    const Double_t ut = (Double_t)(fNanoSec/1e6+(Long_t)fTime)/kDay;

    // Julian centuries since J2000.
    const Double_t t = (ut -(51544.5-fMjd)) / 36525.0;

    // GMST at this UT1
    const Double_t r1 = 24110.54841+(8640184.812866+(0.093104-6.2e-6*t)*t)*t;
    const Double_t r2 = 86400.0*ut;

    const Double_t sum = (r1+r2)/kDaySec;

    double gmst = fmod(sum, 1)*TMath::TwoPi();

    // TRotation::TRotation(Double_t mxx, Double_t mxy, Double_t mxz,
    //                      Double_t myx, Double_t myy, Double_t myz,
    //                      Double_t mzx, Double_t mzy, Double_t mzz)
    // : fxx(mxx), fxy(mxy), fxz(mxz),
    //   fyx(myx), fyy(myy), fyz(myz),
    //   fzx(mzx), fzy(mzy), fzz(mzz) {}

    TRotation conv(1, 0, 0, 0, -1, 0, 0, 0, 1);
    conv.RotateZ(gmst + obs.GetElong());
    conv.RotateY(obs.GetPhi()-TMath::Pi()/2);
    conv.RotateZ(TMath::Pi());

    //  TVector3 & TVector3::operator *= (const TRotation & m){
    //     return *this = m * (*this);
    //  }

    // inline TVector3 TRotation::operator * (const TVector3 & p) const {
    //     return TVector3(fxx*p.X() + fxy*p.Y() + fxz*p.Z(),
    //                     fyx*p.X() + fyy*p.Y() + fyz*p.Z(),
    //                     fzx*p.X() + fzy*p.Y() + fzz*p.Z());
    // }

    // Convert sky coordinates of camera center convert to local.
    // Same comment as above. These coordinates differ from the true
    // local coordinates of the camera center that one could get from
    // "MPointingPos", calculated by the Drive: the reason is that the Drive
    // takes into account precession and nutation corrections, while
    // MAstroSky2Local (as of Jan 27 2005 at least) does not. Since we just
    // want to get the source position on the camera from the local
    // coordinates of the center and the source, it does not matter that
    // the coordinates contained in pos and pos0 ignore precession and
    // nutation... since the shift would be the same in both cases. What
    // would be wrong is to set in pos0 directly the local coordinates
    // found in MPointingPos!
    pos0 *= conv;

    if (fDeviation)
    {
        // Position at which the starguider camera is pointing in real:
        //       pointing position = nominal position - dev
        //
        //vx = CalcXYinCamera(pos0, pos)*fGeom->GetCameraDist()*1000;
        pos0.SetZdAz(pos0.Theta()-fDeviation->GetDevZdRad(),
                     pos0.Phi()  -fDeviation->GetDevAzRad());
    }

    // Calculate source position in camera, and convert to mm:
    TVector2 v = MAstro::GetDistOnPlain(pos0, pos, -fGeom->GetCameraDist()*1000);

    pos.RotateZ(-pos0.Phi());
    pos.RotateY(-pos0.Theta());
    pos.RotateZ(-TMath::Pi()/2); // exchange x and y
    pos *= -fGeom->GetCameraDist()*1000/pos.Z();

    TVector2 v = pos.XYvector();

    if (fDeviation)
        v -= fDeviation->GetDevXY()/fGeom->GetConvMm2Deg();

    SetSrcPos(v);

    // ================================================

        if (fMode==kWobble)
    {
        // The trick here is that the anti-source position in case
        // of the off-source regions is always the on-source positon
        // thus a proper anti-source cut is possible.
        fSrcPosAnti->SetXY(v);
        if (fCallback)
            v = Rotate(v, fCallback->GetNumPass(), fCallback->GetNumPasses());
        else
            v *= -1;
        fSrcPosCam->SetXY(v);
    }
    else
    {
        // Because we don't process this three times like in the
        // wobble case we have to find another way to determine which
        // off-source region is the right anti-source position
        // We do it randomly.
        fSrcPosCam->SetXY(v);
        if (fNumRandomOffPositions>1)
            v = Rotate(v, gRandom->Integer(fNumRandomOffPositions), fNumRandomOffPositions);
        else
            v *= -1;
        fSrcPosAnti->SetXY(v);
    }



 */




/*
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

*/
