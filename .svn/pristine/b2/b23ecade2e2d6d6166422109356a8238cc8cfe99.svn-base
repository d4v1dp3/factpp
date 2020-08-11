#include "Prediction.h"

#include <boost/algorithm/string/join.hpp>

#include "Database.h"

#include "HeadersToO.h"
#include "HeadersGCN.h"

#include "EventImp.h"
#include "LocalControl.h"
#include "StateMachineDim.h"

#include "Dim.h"
#include "tools.h"
#include "Time.h"
#include "Configuration.h"

using namespace std;
using namespace Nova;

// -----------------------------------------------------------------------

enum VisibilityCriterion
{
    kMoonMin,
    kMoonMax,
    kSunMax,
    kZenithMax,
    kCurrentMax,
    kThresholdMax,
};

struct VisibilityConditions : map<VisibilityCriterion, float>
{
    VisibilityConditions()
    {
        operator[](kMoonMin)      =  10;
        operator[](kMoonMax)      = 170;
        operator[](kSunMax)       = -12;
        operator[](kZenithMax)    =  75;
        operator[](kCurrentMax)   = 110;
        operator[](kThresholdMax) =  10;
    }
};

struct CheckVisibility
{
    VisibilityConditions conditions;

    // Output
    Nova::SolarObjects solarobj;
    Nova::ZdAzPosn position;
    Nova::RstTime rst_sun;
    Nova::RstTime rst_obj;

    int vis_obj = -1;

    double moon_dist   = -1;
    double current     = -1;
    double threshold   = -1;

    bool valid_zd        = false;
    bool valid_current   = false;
    bool valid_sun       = false;
    bool valid_moon      = false;
    bool valid_threshold = false;

    bool visible       = false; // And of all the above except valid_threshold

    void calc(const Nova::EquPosn &equ, const double &jd)
    {
        solarobj  = Nova::SolarObjects(jd);
        position  = Nova::GetHrzFromEqu(equ, jd);
        moon_dist = Nova::GetAngularSeparation(equ, solarobj.fMoonEqu);
        rst_sun   = Nova::GetSolarRst(jd, -12);
        vis_obj   = Nova::GetObjectRst(rst_obj, equ, jd, 90-conditions[kZenithMax]);

        current   = FACT::PredictI(solarobj, equ);

        const double ratio = pow(cos(position.zd*M_PI/180), -2.664);

        threshold = position.zd<90 ? ratio*pow(current/6.2, 0.394) : -1;

        valid_moon      = moon_dist>conditions[kMoonMin] && moon_dist<conditions[kMoonMax];
        valid_zd        = position.zd<conditions[kZenithMax];
        valid_sun       = solarobj.fSunHrz.alt<conditions[kSunMax];
        valid_current   = current<conditions[kCurrentMax];
        valid_threshold = threshold>0 && threshold<conditions[kThresholdMax];

        visible = valid_moon && valid_zd && valid_sun && valid_current;
    }

    CheckVisibility(const VisibilityConditions &cond, const Nova::EquPosn &equ, const double &jd) : conditions(cond)
    {
        calc(equ, jd);
    }

    CheckVisibility(const VisibilityConditions &cond, const Nova::EquPosn &equ) : conditions(cond)
    {
        calc(equ, Time().JD());
    }

    CheckVisibility(const VisibilityConditions &cond, const double &ra, const double &dec, const double &jd): conditions(cond)
    {
        Nova::EquPosn equ;
        equ.ra  = ra;
        equ.dec = dec;
        calc(equ, jd);
    }

    CheckVisibility(const VisibilityConditions &cond, const double &ra, const double &dec) : conditions(cond)
    {
        Nova::EquPosn equ;
        equ.ra  = ra;
        equ.dec = dec;
        calc(equ, Time().JD());
    }
};

/*

 * Visibility check
 * Source eintragen
 * RELOAD SOURCES
 * Schedule eintragen
 * Send 'RESCHEDULE' interrupt

*/
/*
int mainx(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    // ------------------ Eval config ---------------------

    const string uri = conf.Get<string>("schedule-database");

    const uint16_t verbose = 1;
    const bool dry_run = true;

    Time stopwatch;

    Database connection(uri); // Keep alive while fetching rows

    cout << Time()-stopwatch << endl;

    if (verbose>0)
        cout << "Server Version: " << connection.server_version() << endl;

    const int source_key = 999;      // source_key of the source to be scheduled.

    const uint16_t contingency =   0; // The contingency to delete earlier entries from the Table
    const uint16_t duration    = 300; // The target duration of the ToO
    const uint16_t required    =  20; // If the ToO would be less than required, skip_it
    const uint16_t skip_time   =  20; // If following observation would be less than skip_time, skip it

    if (duration<required)
    {
        cerr << "Requested duration is smaller than required time." << endl;
        return 0;
    }

    Time day(2019, 1, 24, 17,  0,  0);
    Time now(2019, 1, 24, 19, 59,  0);
    Time next    = day+boost::posix_time::hours(14);
    Time restart = now+boost::posix_time::minutes(duration);

    // Safety margin
    now -= boost::posix_time::seconds(contingency);

    cout << '\n';
    cout << "Contingency:   " << contingency << " s\n";
    cout << "Start of ToO:  " << now.GetAsStr() << '\n';
    cout << "End   of ToO:  " << restart.GetAsStr() << '\n';
    cout << "Duration:      " << duration << "  min\n\n";

    cout << "Schedule:\n";

    const string queryS =
        "SELECT *,"
        " (fMeasurementTypeKey=4 AND fStart<='"+restart.GetAsStr()+"') AS `Reschedule`,"
        " (fStart>'"+restart.GetAsStr()+"') AS `Following`,"
        " (fMeasurementTypeKey=4 AND fStart BETWEEN '"+now.GetAsStr()+"' AND '"+restart.GetAsStr()+"') AS `Delete`"
        " FROM Schedule"
        " WHERE fStart BETWEEN '"+day.GetAsStr()+"' AND '"+next.GetAsStr()+"'"
        " AND fMeasurementID=0"
        " ORDER BY fStart ASC, fMeasurementID ASC";

    const mysqlpp::StoreQueryResult resS =
        connection.query(queryS).store();

    vector<string> list;

    int32_t firstdata  = -1;
    int32_t reschedule = -1;
    int32_t following  = -1;
    for (size_t i=0; i<resS.num_rows(); i++)
    {
        const mysqlpp::Row &row = resS[i];

        cout << setw(2) << i << " | ";
        cout << row["Reschedule"]     << " | ";
        cout << row["Following"]      << " | ";
        cout << row["fScheduleID"]    << " | ";
        cout << row["fStart"]         << " | ";
        if (bool(row["Delete"]))
        {
            cout << "     < delete >     | ";
            list.push_back((string)row["fScheduleID"]);
        }
        else
            cout << row["fLastUpdate"]    << " | ";
        cout << row["fMeasurementID"] << " | ";
        cout << row["fUser"]          << " | ";
        cout << row["fData"]          << " | " << setw(4);
        cout << row["fSourceKey"]     << " | ";
        cout << row["fMeasurementTypeKey"] << '\n';

        if (bool(row["Reschedule"]))
            reschedule = i;

        if (following==-1 && bool(row["Following"]))
            following = i;

        uint16_t type = row["fMeasurementTypeKey"];
        if (firstdata==-1 && type==4)
            firstdata = i;
    }

    // --------------------------------------------------------------------

    if (firstdata==-1)
    {
        cerr << "No data run found." << endl;
        return 0;
    }

    // --------------------------------------------------------------------

    if (reschedule>=0)
        cout << "\nLast data run before restart at " << restart.GetAsStr() << ": idx=" << reschedule << " / ID=" << resS[reschedule]["fScheduleID"] << "\n";

    // --------------------------------------------------------------------

    const Time first((string)resS[firstdata]["fStart"]);          // First data run of the night
    const Time last( (string)resS[resS.num_rows()-1]["fStart"]);  // Last scheduled observation of the night

    // --------------------------------------------------------------------

    if (restart<=first)
    {
        cout << "ToO ends before first data taking... skipped." << endl;
        return 0;
    }
    if (now>=last)
    {
        cout << "ToO starts after data taking... skipped." << endl;
        return 0;
    }

    // --------------------------------------------------------------------

    if (now<first)
    {
        cout << "ToO starts before first data taking... rescheduling start time." << endl;
        now = first;
    }
    if (restart>last)
    {
        cout << "ToO ends after data taking... rescheduling end time!" << endl;
        restart = last;
    }

    // --------------------------------------------------------------------

    if (restart<now+boost::posix_time::minutes(required))
    {
        cout << "Can not schedule more than " << required << "minutes... skipped." << endl;
        return 0;
    }

    // --------------------------------------------------------------------

    if (following>=0)
    {
        const Time follow((string)resS[following]["fStart"]);
        if (follow<restart+boost::posix_time::minutes(skip_time))
        {
            cout << "Following observation would be less than " << skip_time << " min... skipping rescheduled source." << endl;
            reschedule = -1;
        }
    }

    // ====================================================================

    if (!list.empty())
    {
        cout << "\nDelete entries:\n";
        for (const auto &l : list)
            cout << l << "\n";
        cout << endl;
    }

    // ====================================================================

    vector<string> insert;

    cout << "New entries:\n";
    cout << "       | auto  | " << now.GetAsStr()     << " |       < start >     | 0 | ToO  | NULL |      | 4\n";

    insert.emplace_back("('"+now.GetAsStr()+"',0,'ToO',NULL,"+to_string(source_key)+",4)");

    // --------------------------------------------------------------------

    if (reschedule>=0 && restart!=last)
    {
        cout << "       | auto  | " << restart.GetAsStr() << " |        < end >      | 0 | ToO  | NULL | " << setw(4) << resS[reschedule]["fSourceKey"] << " | 4\n";

        const string fData = (string)resS[reschedule]["fData"];
        const string fKey  = (string)resS[reschedule]["fSourceKey"];

        const string data  = resS[reschedule]["fData"] ? "'"+fData+"'" : "NULL";
        insert.emplace_back("('"+restart.GetAsStr()+"',0,'ToO',"+data+","+fKey+",4)");
    }

    // ====================================================================

    cout << Time()-stopwatch << endl;

    // ====================================================================

    if (insert.empty())
    {
        cout << "No new schedule." << endl;
        return 0;
    }

    const string queryD = string("DELETE FROM Schedule WHERE fScheduleID IN (")+boost::algorithm::join(list, ",")+")";

    const string queryI =
        "INSERT (fStart, fMeasurementID, fUser, fData, fSoureKey, fMeasurementTypeKey) "
        "VALUES\n "+string(boost::algorithm::join(insert, ",\n "));

    if (dry_run)
    {
        cout << "\n";
        if (!list.empty())
            cout << queryD << "\n\n";
        if (!insert.empty())
            cout << queryI << "\n\n";
        cout << flush;
        return 0;
    }

    // Empty interrupt to stop data taking as early as possible
    Dim::SendCommandNB("DIM_CONTROL/INTERRUPT");

    // Reload sources as early as possible
    Dim::SendCommandNB("DRIVE_CONTROL/RELOAD_SOURCES");

    // Start pointing procedure as early as possible
    //Dim::SendCommand("DRIVE_CONTROL/TRACK_WOBBLE", wobble, obs[sub].source);

    try
    {
        // ---------------------------------------------------------

        connection.query("LOCK TABLES Schedule WRITE");

        // ---------------------------------------------------------

        if (!list.empty())
        {
            const mysqlpp::SimpleResult res = connection.query(queryD).execute();
            cout << res.rows() << " row(s) deleted.\n" << endl;
        }

        // ---------------------------------------------------------

        if (!insert.empty())
        {
            auto q = connection.query(queryI);
            q.execute();
            cout << q.info() << '\n' << endl;
        }

        // ---------------------------------------------------------

        connection.query("UNLOCK TABLES");

        // ---------------------------------------------------------
    }
    catch (const exception &e)
    {
        cerr << "SQL query failed: " << e.what() << endl;
        return 7;
    }

    Dim::SendCommand("DIM_CONTROL/INTERRUPT", "reschedule");

    return 0;
}
*/
// ------------------------------------------------------------------------

class StateMachineToO : public StateMachineDim
{
private:
    map<uint16_t, VisibilityConditions> fVisibilityCriteria;

    string   fUri;
    Database fConnection;
    uint16_t fVerbose;

    Time     fKeepAliveDeadline;
    uint16_t fKeepAliveInterval;

    struct Source
    {
        int16_t key { -1 };
        bool isToO { false };
        bool satellite { false };
    };

    Source GetSourceKey(const string &name, const double &ra, const double &dec)
    {
        const double min_dist   = 0.1;
        const double check_dist = 2.0;

        // ----------------------- Check if source with same name exists -----------------------

        const string query2 =
            "SELECT fSourceKey, fIsToO FROM Source WHERE fSourceName='"+name+"'";

        const mysqlpp::StoreQueryResult res2 = fConnection.query(query2).store();

        if (res2.num_rows())
        {
            Source source;
            source.key   = res2[0]["fSourceKey"];
            source.isToO = res2[0]["fIsToO"];

            Info("A source with the same name (key="+to_string(source.key)+") was found in the Source table.");

            return source;
        }

        // ----------------- Check if source with similar coordinates exists -------------------

        const string query1 =
            "SELECT\n"
            " fSourceKey,\n"
            " fSourceName,\n"
            " ADIST(fDeclination, fRightAscension*15, "+to_string(dec)+", "+to_string(ra)+") AS Dist,\n"
            " fIsToO,\n"
            " fFollowSatellite\n"
            "FROM Source\n"
            "WHERE fSourceTypeKEY=1\n"
            "HAVING Dist<"+to_string(check_dist)+"\n"
            "ORDER BY Dist ASC";

        const mysqlpp::StoreQueryResult res1 = fConnection.query(query1).store();

        if (res1.num_rows())
        {
            Info("Found "+to_string(res1.num_rows())+" sources with a distance less than "+Tools::Form("%.2f", check_dist)+"\u00b0");

            for (size_t i=0; i<res1.num_rows(); i++)
            {
                const mysqlpp::Row &row = res1[i];
                Info(" "+string(row["fSourceName"])+" ["+string(row["fSourceKey"])+"]: D="+Tools::Form("%.2f", double(row["Dist"]))+"\u00b0");
            }

            const mysqlpp::Row &row = res1[0];
            if (double(row["Dist"])<min_dist)
            {
                Source source;
                source.key   = res1[0]["fSourceKey"];
                source.isToO = res1[0]["fIsToO"];
                source.satellite= res1[0]["fFollowSatellite"];

                Warn("Sources closer than "+Tools::Form("%.2f", check_dist)+"\u00b0 detected.");
                Warn("Overwriting source key with: "+string(row["fSourceName"])+" ["+to_string(source.key)+"]: D="+Tools::Form("%.2f", double(row["Dist"]))+"\u00b0");

                return source;
            }
        }

        return Source();
    }

    float GetWobbleAngle(const double &ra, const double &dec)
    {
        const double wobble_offset = 0.6;
        const double camera_radius = 2.3;
        const double magnitude_max = 4.5;

        /*
         Mag Cnt
         -2  1
         -1  3
          0  11
          1  33
          2  121
          3  321
          4  871
          5  1364
          6  404
          7  12
        */

        // The wobble position lay in the plane which is normal the to the plane source-center-star
        // and goes through

        double wobble_angle  = 0;

        const string query0 =
            "SELECT fSourceKEY, fRightAscension, fDeclination, fMagnitude,\n"
            " ADIST(fDeclination, fRightAscension*15, "+to_string(dec)+", "+to_string(ra)+") AS Dist\n"
            " FROM Source\n"
            " WHERE (fSourceTypeKey=2 OR fSourceTypeKey=3) AND fMagnitude<"+to_string(magnitude_max)+"\n"
            " HAVING Dist<"+to_string(camera_radius+wobble_offset)+"\n"
            " ORDER BY fMagnitude ASC";

        // Out() << query0 << endl;

        const mysqlpp::StoreQueryResult res0 = fConnection.query(query0).store();

        Info("Found "+to_string(res0.num_rows())+" stars in the camera field-of-view with magnitude less than "+to_string(magnitude_max));

        for (size_t i=0; i<::min<size_t>(10, res0.num_rows()); i++)
        {
            const mysqlpp::Row &row = res0[i];

            // TVector3 souce, star;
            // source.SetMagThetaPhi(1, rad(90-dec), rad(ra));
            // star.SetMagThetaPhi(  1, rad(90-dec), rad(ra));
            // 
            // star.RotateZ(  -source.Phi());
            // source.RotateZ(-source.Phi());
            // 
            // star.RotateY(  180-source.Theta());
            // source.RotateY(180-source.Theta());
            // 
            // rho = star.Phi();

            const double rad = M_PI/180;

            const double dec0 = rad * dec;
            const double ra0  = rad * ra;

            const double dec1 = rad * row["fDeclination"];
            const double ra1  = rad * row["fRightAscension"] * 15;

            const double s2 = sin(ra0-ra1);
            const double c2 = cos(ra0-ra1);

            const double s0 = cos(dec0);
            const double c0 = sin(dec0);

            const double c1 = cos(dec1);
            const double s1 = sin(dec1);

            const double k0 =           -s2*c1;
            const double k1 = s0*s1 - c0*c2*c1;

            const double rho = atan(k0/k1) / rad; // atan2(k0, k1)/rad

            if (i==0)
                wobble_angle = rho;

            Info(" "+Tools::Form("Mag=%5.2f  Dist=%5.1f  Phi=%6.1f", (double)row["fMagnitude"], (double)row["Dist"], rho));
        }

        if (res0.num_rows())
        {
            Info("The brightest star (M="+string(res0[0]["fMagnitude"])+") at distance is visible at a wobble angle of "+Tools::Form("%.2f", wobble_angle)+"\u00b0");
            Info("Wobble angles determined as "+Tools::Form("%.2f", wobble_angle-90)+"\u00b0 and "+Tools::Form("%.2f", wobble_angle+90)+"\u000b");
        }
        else
        {
            Info("Using default wobble angles.");
        }

        return wobble_angle;
    }

    uint32_t AddSource(const string &name, const double &ra, const double &dec, const bool fDryRun=false)
    {
        const double wobble_angle = GetWobbleAngle(ra, dec);

        const string query =
            "INSERT INTO Source\n"
            " (fSourceName, fRightAscension, fDeclination, fWobbleAngle0, fWobbleAngle1, fSourceTypeKey, fIsToO) VALUES\n"
            " ('"+name+"', "+to_string(ra/15.)+", "+to_string(dec)+", "+to_string(wobble_angle-90)+", "+to_string(wobble_angle+90)+", 1, 1)";

        if (fDryRun)
        {
            Out() << query << endl;
            return -1;
        }

        auto q = fConnection.query(query);
        q.execute();
        if (!q.info().empty())
            Info(q.info());

        const uint32_t source_key = q.insert_id();

        Info(string(fDryRun?"[dry-run] ":"")+"The new source got the key "+to_string(source_key));

        return source_key;
    }

    bool ScheduleImp(const string &name, const ToO::DataGRB &grb, const CheckVisibility &check, const bool &is_pointing)
    {
        const bool fDryRun = GetCurrentState()!=ToO::State::kArmed;

        if (fDryRun)
            Warn("Scheduler not armed!");

        Time stopwatch;

        // This is just a desperate try which will most likely fail
        if (!fConnection.connected())
            fConnection.reconnect();

        /*
         +-----------------+---------------------+------+-----+---------+----------------+
         | Field           | Type                | Null | Key | Default | Extra          |
         +-----------------+---------------------+------+-----+---------+----------------+
         | fSourceKEY      | int(11)             | NO   | PRI | NULL    | auto_increment |
         | fSourceName     | varchar(30)         | NO   |     | NULL    |                |
         | fRightAscension | double(10,6)        | NO   |     | NULL    |                |
         | fDeclination    | double(10,6)        | NO   |     | NULL    |                |
         | fEpochKEY       | tinyint(4)          | YES  |     | NULL    |                |
         | fFlux           | float               | YES  |     | NULL    |                |
         | fSlope          | float               | YES  |     | NULL    |                |
         | fSourceTypeKey  | int(11)             | NO   | MUL | NULL    |                |
         | fWobbleOffset   | float               | NO   |     | 0.6     |                |
         | fWobbleAngle0   | int(11)             | NO   |     | 90      |                |
         | fWobbleAngle1   | int(11)             | NO   |     | -90     |                |
         | fMagnitude      | float(4,2)          | YES  |     | NULL    |                |
         | fIsToO          | tinyint(3) unsigned | NO   |     | 0       |                |
         +-----------------+---------------------+------+-----+---------+----------------+
         */

        Source source = GetSourceKey(name, grb.ra, grb.dec);

        Out() << Time()-stopwatch << endl;

        // This is not a real alert but a pointing information of a satellite
        if (is_pointing)
        {
            // We only observe known sources in paralell with satellites...
            if (source.key<0)
            {
                Warn("Observation of only known sources requested but no known source found.");
                return false;
            }

            // ... and only if they are 'standard' sources and not ToO sources.
            if (source.isToO)
            {
                Warn("Known source with ToO flag... skipping.");
                return false;
            }

            // For non prioritized sources we require a threshold <3
            // and for prioritized sources a threshold <10

            // Special check for the threshold. Threshold condition atm only
            // applies for pointings and is different for prioritized sources
            if ((check.threshold>3 && !source.satellite) || !check.valid_threshold)
            {
                Warn(Tools::Form("Relative threshold [%6.2f] too high... skipping.", check.threshold));
                return false;
            }
        }


        /*
         +---------------------+--------------+------+-----+-------------------+-----------------------------+
         | Field               | Type         | Null | Key | Default           | Extra                       |
         +---------------------+--------------+------+-----+-------------------+-----------------------------+
         | fScheduleID         | int(11)      | NO   | PRI | NULL              | auto_increment              |
         | fStart              | datetime     | NO   | MUL | NULL              |                             |
         | fLastUpdate         | timestamp    | NO   |     | CURRENT_TIMESTAMP | on update CURRENT_TIMESTAMP |
         | fMeasurementID      | int(11)      | NO   |     | NULL              |                             |
         | fUser               | varchar(32)  | NO   |     | NULL              |                             |
         | fData               | varchar(256) | YES  |     | NULL              |                             |
         | fSourceKey          | smallint(6)  | YES  |     | NULL              |                             |
         | fMeasurementTypeKey | tinyint(4)   | NO   |     | NULL              |                             |
         +---------------------+--------------+------+-----+-------------------+-----------------------------+
         */

        const uint16_t contingency =   1; // The contingency to delete earlier entries from the Table
        const uint16_t duration    =  60; // The target duration of the ToO
        const uint16_t required    =  20; // If the ToO would be less than required, skip_it
        const uint16_t skip_time   =  20; // If following observation would be less than skip_time, skip it

        if (duration<required)
        {
            Error("Requested duration is smaller than required time.");
            return false;
        }

        const Time now;

        const Time sunset  = now.GetPrevSunSet();
        const Time sunrise = now.GetNextSunRise();

        if (sunset>sunrise || sunrise>sunset+boost::posix_time::hours(24))
        {
            Error("Unable to schedule a ToO during daytime.");
            return false;
        }

        // Safety margin
        Time schedtime = now-boost::posix_time::seconds(contingency);
        Time restart   = now+boost::posix_time::minutes(duration);

        Out() << '\n';
        Out() << "Contingency:   " << contingency << " s\n";
        Out() << "Start of ToO:  " << now.GetAsStr() << '\n';
        Out() << "End   of ToO:  " << restart.GetAsStr() << '\n';
        Out() << "Duration:      " << duration << " min\n\n";

        Out() << "Schedule:\n";

        const string queryS =
            "SELECT *,\n"
            " (fMeasurementTypeKey=4 AND fStart<='"+restart.GetAsStr()+"') AS `Reschedule`,\n"
            " (fStart>'"+restart.GetAsStr()+"') AS `Following`,\n"
            " (fMeasurementTypeKey=4 AND fStart BETWEEN '"+schedtime.GetAsStr()+"' AND '"+restart.GetAsStr()+"') AS `Delete`\n"
            " FROM Schedule\n"
            " WHERE fStart BETWEEN '"+sunset.GetAsStr()+"' AND '"+sunrise.GetAsStr()+"'\n"
            " AND fMeasurementID=0\n"
            " ORDER BY fStart ASC, fMeasurementID ASC";

        const mysqlpp::StoreQueryResult resS = fConnection.query(queryS).store();

        if (resS.num_rows()<2)
        {
            Error("Available schedule is too short to be evaluated.");
            return false;
        }

        // `Reschedule`:  Start time before            end time of ToO (type==data)
        // `Following`:   Start time after             end time of ToO (type==any)
        // `Delete`:      Start time between start and end time of ToO (type==data)

        vector<string> list;

        int32_t firstdata  = -1;  // First observation with type==data
        int32_t reschedule = -1;  // Last  observation with: Start time before end time of ToO (type==data)
        int32_t following  = -1;  // First observation with: Start time after  end time of ToO (type==any)
        int32_t last_resch = -1;  // Last  observation with: Start time before end time of ToO (type=data) and start time not between start and end time of ToO (type==any)
        for (size_t i=0; i<resS.num_rows(); i++)
        {
            const mysqlpp::Row &row = resS[i];

            Out() << setw(2) << i << " | ";
            Out() << row["Reschedule"]     << " | ";
            Out() << row["Following"]      << " | ";
            Out() << row["fScheduleID"]    << " | ";
            Out() << row["fStart"]         << " | ";
            if (bool(row["Delete"]))
            {
                Out() << "     < delete >     | ";
                list.push_back((string)row["fScheduleID"]);
            }
            else
                Out() << row["fLastUpdate"]    << " | ";
            Out() << row["fMeasurementID"] << " | ";
            Out() << row["fUser"]          << " | ";
            Out() << row["fData"]          << " | " << setw(4);
            Out() << row["fSourceKey"]     << " | ";
            Out() << row["fMeasurementTypeKey"] << '\n';

            if (bool(row["Reschedule"]))
                reschedule = i;

            if (bool(row["Reschedule"]) && !row["Delete"])
                last_resch = i;

            if (following==-1 && bool(row["Following"]))
                following = i;

            const uint16_t type = row["fMeasurementTypeKey"];
            if (firstdata==-1 && type==4)
                firstdata = i;
        }

        // --------------------------------------------------------------------

        if (firstdata==-1)
        {
            Warn("No data run found.");
            return false;
        }

        // --------------------------------------------------------------------

        if (reschedule>=0)
            Out() << "\nLast data run before restart at " << restart.GetAsStr() << ": idx=" << reschedule << " / ID=" << resS[reschedule]["fScheduleID"] << endl;

        // --------------------------------------------------------------------

        const Time first((string)resS[firstdata]["fStart"]);          // First data run of the night
        const Time last( (string)resS[resS.num_rows()-1]["fStart"]);  // Last scheduled observation of the night

        // --------------------------------------------------------------------

        if (restart<=first)
        {
            Warn("ToO ends before first data taking... skipped.");
            return false;
        }
        if (schedtime>=last)
        {
            Warn("ToO starts after data taking... skipped.");
            return false;
        }

        // --------------------------------------------------------------------

        if (schedtime<first)
        {
            Info("ToO starts before first data taking... rescheduling start time.");
            schedtime = first;
        }
        if (restart>last)
        {
            Info("ToO ends after data taking... rescheduling end time!");
            restart = last;
        }

        // --------------------------------------------------------------------

        if (restart<schedtime+boost::posix_time::minutes(required))
        {
            Warn("Could not schedule more than the required "+to_string(required)+" minutes... skipped.");
            return false;
        }

        // --------------------------------------------------------------------

        if (following>=0)
        {
            const Time follow((string)resS[following]["fStart"]);
            if (follow<restart+boost::posix_time::minutes(skip_time))
            {
                Info("Following observation would be less than "+to_string(skip_time)+" min... skipping rescheduled source.");
                reschedule = -1;
            }
        }

        // ====================================================================

        if (!list.empty())
        {
            Out() << "\nDelete entries:\n";
            for (const auto &l : list)
                Out() << l << "\n";
            Out() << endl;
        }

        // ====================================================================

        if (source.key<0)
            source.key = AddSource(name, grb.ra, grb.dec, fDryRun);

        Out() << Time()-stopwatch << endl;

        // ====================================================================

        Info("Source will be scheduled.");

        Out() << "New entries:\n";

        vector<string> insert;

        const bool ongoing = last_resch>=0 && source.key==uint16_t(resS[last_resch]["fSourceKey"]);

        if (ongoing)
            Out() << "           |       |                     |      < ongoing >    | 0 |      |           | " << setw(4) << source.key << " | 4\n";
        else
        {
            Out() << "           | auto  | " << schedtime.GetAsStr()     << " |       < start >     | 0 | ToO  | " << (is_pointing?"  nodrs  ":"nodrs,grb") << " | " << setw(4) << source.key << " | 4\n";
            insert.emplace_back("('"+schedtime.GetAsStr()+"',0,'ToO','"+(is_pointing?"nodrs:true":"nodrs:true,grb:true")+"',"+to_string(source.key)+",4)");
        }

        // --------------------------------------------------------------------

        if (reschedule>=0 && restart!=last)
        {
            Out() << "           | auto  | " << restart.GetAsStr() << " |        < end >      | 0 | ToO  |   NULL    | " << setw(4) << resS[reschedule]["fSourceKey"] << " | 4\n";

            const string fData = (string)resS[reschedule]["fData"];
            const string fKey  = (string)resS[reschedule]["fSourceKey"];

            const string data  = resS[reschedule]["fData"] ? "'"+fData+"'" : "NULL";
            insert.emplace_back("('"+restart.GetAsStr()+"',0,'ToO',"+data+","+fKey+",4)");
        }

        // ====================================================================

        Out() << Time()-stopwatch << endl;

        // ====================================================================

        const string queryD = string("DELETE FROM Schedule WHERE fScheduleID IN (")+boost::algorithm::join(list, ",")+")";

        const string queryI =
            "INSERT INTO Schedule\n (fStart,fMeasurementID,fUser,fData,fSourceKey,fMeasurementTypeKey)\n"
            "VALUES\n "+string(boost::algorithm::join(insert, ",\n "));

        if (fDryRun)
        {
            Out() << "\n";
            if (!list.empty())
                Out() << queryD << "\n\n";
            if (!insert.empty())
                Out() << queryI << "\n\n";
            Out() << flush;
            return false;
        }

        // Empty interrupt to stop data taking as early as possible
        // Triggers also RELOAD_SOURCES
        if (!ongoing)
            Dim::SendCommandNB("DIM_CONTROL/INTERRUPT", "prepare");

        // ---------------------------------------------------------

        fConnection.query("LOCK TABLES Schedule WRITE");

        // ---------------------------------------------------------
        if (!list.empty())
        {
            const mysqlpp::SimpleResult res = fConnection.query(queryD).execute();
            Info(to_string(res.rows())+" row(s) deleted from Schedule.");
        }

        // ---------------------------------------------------------

        if (!insert.empty())
        {
            auto q = fConnection.query(queryI);
            q.execute();
            if (!q.info().empty())
                Info(q.info());
        }
        // ---------------------------------------------------------

        fConnection.query("UNLOCK TABLES");

        // ---------------------------------------------------------

        if (!ongoing)
            Dim::SendCommand("DIM_CONTROL/INTERRUPT", "reschedule");

        ostringstream out;
        out << Time()-stopwatch;
        Info(out);

        // ---------------------------------------------------------

        const string queryT =
            "INSERT INTO ToOs\n"
            " (fTypeID, fRightAscension, fDeclination, fSourceKEY) VALUES\n"
            " ('"+to_string(grb.type)+"', "+to_string(grb.ra/15)+", "+to_string(grb.dec)+", "+to_string(source.key)+")";

        if (!fDryRun)
        {
            auto q = fConnection.query(queryT);
            q.execute();
            if (!q.info().empty())
                Info(q.info());
        }
        else
            Out() << queryT << endl;

        // ---------------------------------------------------------

        return true;
    }
/*
    bool Schedule(const string &name, const ToO::DataGRB &grb, const CheckVisibility &check, const bool &is_pointing)
    {
        try
        {
            return ScheduleImp(name, grb, check, is_pointing);
        }
        catch (const exception &e)
        {
            Error(string("SQL query failed: ")+e.what());
            fConnection.disconnect();
            return false;
        }
    }
*/
    // ========================================================================


    // ========================================================================

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        Fatal(msg);
        return false;
    }

    bool CheckMinEventSize(size_t has, const char *name, size_t size)
    {
        if (has>size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected more.";
        Fatal(msg);
        return false;
    }

    void EnterIntoAlertDB(const string &name, const ToO::DataGRB &grb, const CheckVisibility &check, const bool &is_pointing)
    {
        // Do not enter satellite pointing alerts (as there are too many)
        if (is_pointing)
            return;

        // Check for general visibility of the source during the night
        // If required: For better performance, both (sun and object)
        // RstTime could be calculated only here
        const double sun_set  = fmod(check.rst_sun.set,     1);
        const double sun_rise = fmod(check.rst_sun.rise,    1);

        const double rise     = fmod(check.rst_obj.rise,    1);
        const double trans    = fmod(check.rst_obj.transit, 1);
        const double set      = fmod(check.rst_obj.set,     1);

        // Object is at at least visible at some time during the
        // night (above the required horizon) or is circumpolar
        // above that horizon
        switch (check.vis_obj)
        {
        case -1:
            // Object is not visible above the requested horizon
            return;

        case 0:
            // Object crosses the local meridian
            //  Rises      after sun-rise or before sun-set
            //  Culminates after sun-rise or before sun-set
            //  Sets       after sun-rise or before sun-set
            if ((rise <sun_set || rise >sun_rise) &&
                (trans<sun_set || trans>sun_rise) &&
                (set  <sun_set || set  >sun_rise))
                return;

            break;

        case 1:
            // Object is always visible above the requested horizon
            break;
        }

        Info("Source "+name+" might be observable.");

        // Make an entry in the alter databse to issue a call
        const string queryS =
            "INSERT FlareAlerts.FlareTriggers SET\n"
            " fTriggerInserted=Now(),\n"
            " fNight="+to_string(Time().NightAsInt())+",\n"
            " fPacketTypeKey="+to_string(grb.type)+",\n"
            " fRa="+to_string(grb.ra/15)+",\n"
            " fDec="+to_string(grb.dec)+",\n"
            " fErr="+to_string(grb.err)+",\n"
            " fName=\""+name+"\",\n"
            " fTriggerType=7";

        const bool fDryRun = GetCurrentState()!=ToO::State::kArmed;

        if (!fDryRun)
        {
            auto q = fConnection.query(queryS);
            q.execute();
            if (!q.info().empty())
                Info(q.info());
        }
        else
            Out() << queryS << endl;
    }

    int ScheduleGCN(const EventImp &evt)
    {
        if (!CheckMinEventSize(evt.GetSize(), "ScheduleGCN", sizeof(ToO::DataGRB)))
            return kSM_FatalError;

        const ToO::DataGRB &grb = evt.Ref<ToO::DataGRB>();

        const auto it = GCN::PaketTypes.find(grb.type);
        if (it==GCN::PaketTypes.end())
        {
            Warn("Unknown Packet Type: "+to_string(grb.type));
            return GetCurrentState();
        }


        const string name = evt.Ptr<char>(sizeof(ToO::DataGRB));

        const auto &paket = it->second;

        ostringstream out;
        out << name << " [" << paket.name << ":" << grb.type << "]: RA=" << grb.ra/15. << "h DEC=" << grb.dec << "\u00b0 ERR=" << grb.err << "\u00b0";

        Info(out);
        Info(paket.description);

        // The []-operator automatically creates a default entry if not yet existing
        auto &conditions = fVisibilityCriteria[grb.type];

        const CheckVisibility check(conditions, grb.ra, grb.dec);

        Info("Sun altitude:  "+Tools::Form("%5.1f\u00b0   ", check.solarobj.fSunHrz.alt)+(check.valid_sun?"OK    ":"failed")+Tools::Form(" [alt < %5.1f\u00b0]", conditions[kSunMax]));
        if (check.valid_sun)
        {
            Info("Moon distance: "+Tools::Form("%5.1f\u00b0   ", check.moon_dist)  +(check.valid_moon   ?"OK    ":"failed")+Tools::Form(" [%5.1f\u00b0 < d < %5.1f\u00b0]", conditions[kMoonMin], conditions[kMoonMax]));
            Info("Zenith angle:  "+Tools::Form("%5.1f\u00b0   ", check.position.zd)+(check.valid_zd     ?"OK    ":"failed")+Tools::Form(" [zd < %5.1f\u00b0]", conditions[kZenithMax]));
            Info("Current:       "+Tools::Form("%5.1f\u00b5A  ", check.current)    +(check.valid_current?"OK    ":"failed")+Tools::Form(" [I < %5.1f\u00b5A]", conditions[kCurrentMax]));
            //Info(string("Rel. threshold: ")+(check.valid_threshold?"OK    ":"failed")+" ["+Tools::Form("%5.1f", check.threshold)+"]");
        }

/*
         * Swift: https://gcn.gsfc.nasa.gov/swift.html
         relevante Paket-Typen:
         60   BAT_Alert
         97   BAT_QL_Pos
         61   BAT_Pos
         62   BAT_Pos_Nack
         Es kann wohl vorkommen, dass ein Alert danach anders klassifiziert ist
         -> BAT_Trans (84) und es steht dass dann in der GRB notice eine catalog
         ID im comment steht. Da ist es mir noch nicht so ganz klar, woran man
         echte Alerts erkennt.

         * Fermi: https://gcn.gsfc.nasa.gov/fermi.html
         relevante Paket-Typen:
         110   GBM_Alert           P        B               B                B&A
         111   GBM_Flt_Pos         P        B               B                B&A
         112   GBM_Gnd_Pos         P        B               B                B&A
         115   GBM_Final_Pos       P        B               B                B&A
         Bei GBM müssen wir usn überlegen wie wir es mit den Positionsupdates
         machen - das können auch mal ein paar Grad sein. siehe zB
         https://gcn.gsfc.nasa.gov/other/568833894.fermi

         Wenn ich das richtig sehe, kommt bei GBM_Alert noch keine
         Position, sndern erst mit GBM_Flt_Pos (in dem Bsp kommt das 3 Sek
         später)

         * INTEGRAL: https://gcn.gsfc.nasa.gov/integral.html
         wenn ich es richtig verstehe, sind die interessanten Alerts die WAKEUP
         (tw kommt ein Positionsupdate via OFFLINE oder REFINED). Es gibt auch
         noch WEAK, aber das sind scheinbar subthreshold alerts - würde ich
         jetzt erst mal weglassen. Im letzten Jahr gab es 8 WAKEUP alerts. 2017
         sogar nur 3, 2016 7, 2015 waren es einige mehr, aber das war V404_Cyg -
         in dem Fall steht dann aber ein Object-Name dabei - ansonsten steht as
         "Possbibly real GRB event" - zT steht auch, dass es coincident mit GBM
         events ist

         * KONUS: https://gcn.gsfc.nasa.gov/konus.html
         Ist mir noch etwas unklar, was das genau ist - die schicken Lichtkurven
         von GRBs und Transients - muss man nochmal genauer schauen.

         * MAXI: https://gcn.gsfc.nasa.gov/maxi.html
         wenn ich das richtig verstehe, sind das nicht nur GRBs -> müsste man
         nochmal genauer schauen bevor man da was implementiert

         * AGILE: https://gcn.gsfc.nasa.gov/agile.html
         Wahrscheinlich eher nicht relevant, da steht was von 30min nach dem
         Burst kommt das 'Wakeup'. Die relevanten Paket-Typen wären 100
         (Wakeup_Pos), 101 (Ground_Pos) und 102 (Refined_Pos)

         * AMON: https://gcn.gsfc.nasa.gov/amon.html
         sind bisher nur neutrinos - da macht MAGIC glaub ich schon automatic
         Follow-Up - müssen wir also kucken was Sinn macht

         * Ligo-Virgo GW: https://gcn.gsfc.nasa.gov/lvc.html
         da ist natürlich die Error-Region groß - müsste man schauen, was man
         da will

         Fazit: am relevantesten sind Fermi/GBM und Swift/BAT. Evtl könnte
         INTEGRAL noch Sinn machen und über AMON und LVC sollte man nachdenken,
         ob/was man haben will.
*/

        const bool is_pointing = grb.type==51 || grb.type==83;

        try
        {
            if (!check.visible)
            {
                Info("Source not observable... skipped.");
                EnterIntoAlertDB(name, grb, check, is_pointing);

                return fConnection.connected() ? GetCurrentState() : ToO::State::kDisconnected;
            }

            Info("Source is visible... scheduling.");

            if (!ScheduleImp(name, grb, check, is_pointing))
                EnterIntoAlertDB(name, grb, check, is_pointing);
        }
        catch (const exception &e)
        {
            Error(string("SQL query failed: ")+e.what());
            fConnection.disconnect();
        }

        return fConnection.connected() ? GetCurrentState() : ToO::State::kDisconnected;
    }

    int AddNewSource(const EventImp &evt)
    {
        if (!CheckMinEventSize(evt.GetSize(), "AddNewSource", 2*sizeof(double)))
            return kSM_FatalError;

        const double &ra  = evt.Ptr<double>()[0];
        const double &dec = evt.Ptr<double>()[1];
        const string name = evt.Ptr<char>(sizeof(double)*2);

        ostringstream out;
        out << "New Source: '" << name << "' RA=" << ra << "h DEC=" << dec << "\u00b0";

        Info(out);

        try
        {
            const Source source = GetSourceKey(name, ra*15, dec);

            if (source.key>=0) // Not a known source
                Warn("Source already known with key="+to_string(source.key)+".");
            else
                /*source.key =*/ AddSource(name, ra*15, dec);

        }
        catch (const exception &e)
        {
            Error(string("SQL query failed: ")+e.what());
            fConnection.disconnect();
        }

        return fConnection.connected() ? GetCurrentState() : ToO::State::kDisconnected;
    }

    int Enable()
    {
        return GetCurrentState()==ToO::State::kConnected ? ToO::State::kArmed : GetCurrentState();
    }
    int Disable()
    {
        return GetCurrentState()==ToO::State::kArmed ? ToO::State::kConnected : GetCurrentState();
    }

    int Execute()
    {
        Time now;
        if (now>fKeepAliveDeadline)
        {
            static int ernum = 0;
            try
            {
                // Unfortunately, reconnecting [Takes about 1s] is
                // the only non-blocking way to ensure an open
                // connection. Ping would be nice but is blocking.
                fConnection = Database(fUri);
                ernum = 0;
            }
            catch (const mysqlpp::ConnectionFailed &e)
            {
                if (ernum!=e.errnum())
                    Error(e.what());
                ernum = e.errnum();
            }

            fKeepAliveDeadline += boost::posix_time::seconds(fKeepAliveInterval);
        }

        if (!fConnection.connected())
            return ToO::State::kDisconnected;

        if (fConnection.connected() && GetCurrentState()<=ToO::State::kDisconnected)
            return ToO::State::kArmed;

        return GetCurrentState();
    }

public:
    StateMachineToO(ostream &out=cout) : StateMachineDim(out, "SCHEDULER"),
        fConnection(""), fKeepAliveDeadline(Time())
    {
        AddStateName(ToO::State::kDisconnected, "Disconnected",
                     "The Dim DNS is reachable, but the required subsystems are not available.");

        AddStateName(ToO::State::kConnected, "Connected",
                     "All needed subsystems are connected to their hardware, no action is performed.");

        AddStateName(ToO::State::kArmed, "Armed",
                     "All needed subsystems are connected to their hardware, scheduling in progress.");

        AddEvent("GCN", "S:1;D:1;D:1;D:1;C")
            (bind(&StateMachineToO::ScheduleGCN, this, placeholders::_1))
            ("Schedule a ToO"
             "|type[int16]:Pakage or type ID (see HeadersToO.h)"
             "|ra[deg]:Right ascension"
             "|dec[deg]:Declination"
             "|err[deg]:Error radius"
             "|name[string]:Tentative or known source name");

        AddEvent("ADD_SOURCE", "D:1;D:1;C")
            (bind(&StateMachineToO::AddNewSource, this, placeholders::_1))
            ("Add a new source to the databse if not yet available"
             "|ra[h]:Right ascension"
             "|dec[deg]:Declination"
             "|name[string]:Monitored source name");

        AddEvent("START", "")
            (bind(&StateMachineToO::Enable, this/*, placeholders::_1*/))
            ("");

        AddEvent("STOP", "")
            (bind(&StateMachineToO::Disable, this/*, placeholders::_1*/))
            ("");
    }

    /*
    bool GetConfig(Configuration &conf, const string &name, const string &sub, uint16_t &rc)
    {
        if (conf.HasDef(name, sub))
        {
            rc = conf.GetDef<uint16_t>(name, sub);
            return true;
        }

        Error("Neither "+name+"default nor "+name+sub+" found.");
        return false;
    }*/

    void AddCriteria(const vector<uint16_t> &ids, const VisibilityCriterion &crit, const float &limit)
    {
        // The []-operator automatically creates a default entry if not yet existing
        for (auto id=ids.begin(); id!=ids.end(); id++)
            fVisibilityCriteria[*id][crit] = limit;
    }

    int EvalOptions(Configuration &conf)
    {
        fVerbose = !conf.Get<bool>("quiet");
        fKeepAliveInterval = conf.Get<uint16_t>("keep-alive");
        fUri = conf.Get<string>("schedule-database");

        const vector<uint16_t> GRBs =
        {
            60, 61, 62, 97, 110, 111, 112, 115, 53, 54, 55, 56, 100, 101, 102
        };

        AddCriteria(GRBs, kCurrentMax, 30);
        AddCriteria(GRBs, kZenithMax,  45);

        return -1;
    }
};


// ------------------------------------------------------------------------

#include "Main.h"

template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineToO>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Scheduler");
    control.add_options()
        ("quiet,q", po_bool(), "")
        ("keep-alive", var<uint16_t>(uint16_t(300)), "Interval in seconds to ping (reconnect) the database server")
        ("schedule-database", var<string>(), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ;

    conf.AddOptions(control);
}

void PrintUsage()
{
    cout <<
        "The scheduler program is able to schedule a new observation.\n"
        "\n"
        "Usage: scheduler [-c type] [OPTIONS]\n"
        "  or:  scheduler [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineToO>();
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

