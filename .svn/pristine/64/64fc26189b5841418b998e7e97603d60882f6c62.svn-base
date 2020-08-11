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
        ("table.runinfo",  var<string>("RunInfoMC"),  "Name of the table where the run info is stored")
        ("table.events",   var<string>("EventsMC"),   "Name of the table where the events are stored")
        ("table.position", var<string>("PositionMC"), "Name of the table where the calculated posiiton will be stored")
        ("engine",         var<string>(""),           "Database engine to be used when a new table is created")
        ("row-format",     var<string>(""),           "Defines the ROW_FORMAT keyword for table creation")
        ("ignore-errors",  po_switch(),               "Adds the IGNORE keyword to the INSERT query (turns errors into warnings, ignores rows with errors)")
        ("force",          po_switch(),               "Force processing even if there is no database connection")
        ("drop",           po_switch(),               "Drop the table (implies create)")
        ("create",         po_switch(),               "Create the table if it does not yet exist")
        ("update",         po_switch(),               "Uses the ON DUPLICATE KEY to update already existing extries")
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
        "calcsourcemc - Fill a MC table with MC source positions\n"
        "\n"
        "This tool is to calculate the source position in the camera and fill "
        "it into a database table for all events which come from a single run. "
        "The run is idetified by its FileId."
        "\n\n"
        "Then for each event with the given FileId, the telescope pointing "
        "position (`Zd`, `Az`) and the primary particle direction "
        "(`ParticleTheta`,`ParticlePhi`) and its (`EvtNumber`) are requested "
        "from the table given by --table.events. "
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

/*
class MRotation : public TRotation
{
public:
    MRotation() : TRotation(1, 0, 0, 0, -1, 0, 0, 0, 1)
    {
    }
};*/


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
    const string   uri          = conf.Get<string>("uri");
    const string   tab_runinfo  = conf.Get<string>("table.runinfo");
    const string   tab_events   = conf.Get<string>("table.events");
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
            "SELECT FileId FROM `"+tab_runinfo+"`";

        if (print_meta)
            cout << query << endl;

        const mysqlpp::StoreQueryResult res =
            connection.query(query).store();

        for (size_t i=0; i<res.num_rows(); i++)
            cout << "calcsourcemc " << res[i][0] << '\n';
        cout << endl;

        return 0;
    }

    // -------------------------------------------------------------------------
    // create INSERT/UPDATE query (calculate positions)

    if (verbose>0)
    {
        cout << "\n-------------------------- Evaluating file ------------------------";
        cout << "\nRequesting data from table `" << tab_events << "`" << endl;
    }

    const string query =
        "SELECT ParticleTheta, ParticlePhi, Zd, Az, EvtNumber"
        " FROM `"+tab_events+"`"
        " WHERE FileId="+to_string(file);

    if (print_meta)
        cout << query << endl;

    const mysqlpp::UseQueryResult res1 =
        connection.query(query).use();

    ostringstream ins;
    ins << setprecision(16);

    size_t count = 0;
    while (auto row=res1.fetch_row())
    {
        count++;

        const double   part_zd = row[0];
        const double   part_az = row[1];
        const double   tel_zd  = row[2]*M_PI/180;
        const double   tel_az  = row[3]*M_PI/180;
        const uint32_t event   = row[4];

        /*
         // ============================ Mars ================================

         MVector3 pos0, pos;
         pos0.SetZdAz(fPointPos->GetZdRad(),      fPointPos->GetAzRad());
         pos.SetZdAz( fMcEvt->GetParticleTheta(), fMcEvt->GetParticlePhi());
         
         srcpos = MAstro::GetDistOnPlain(pos0, pos, -fGeom->GetCameraDist()*1000);
         */

        // ================================= Nova ===============================

        TVector3 pos;
        TVector3 pos0;
        pos.SetMagThetaPhi( 1, part_zd, part_az);
        pos0.SetMagThetaPhi(1, tel_zd,  tel_az);

        pos.RotateZ(-pos0.Phi());
        pos.RotateY(-pos0.Theta());
        pos.RotateZ(-M_PI/2); // exchange x and y
        pos *= -focal_dist/pos.Z();

        TVector2 v = pos.XYvector();

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
            cerr << "SQL query failed: " << e.what() << endl;
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
            cerr << "SQL query failed: " << e.what() << endl;
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
            cerr << "SQL query failed: " << e.what() << endl;
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
