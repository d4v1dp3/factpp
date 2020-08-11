#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "Database.h"

#include "pal.h"
#include "nova.h"
#include "tools.h"
#include "Time.h"
#include "Configuration.h"

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
        ("file",           var<string>()->required(),   "Corsika Input Card")
        ("force",          po_switch(),                 "Force processing even if there is no database connection")
        ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("no-insert",      po_switch(),      "Does not insert or update any data to any table")
        ("print-insert",   po_switch(),      "Print the INSERT/REPLACE queries")
        ("verbose,v",      var<uint16_t>(1), "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    po::positional_options_description p;
    p.add("file",  1); // The 1st positional options (n=1)

    conf.AddOptions(control);
    conf.AddOptions(debug);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "corsika2sql - Fill a Corsika Input Card into a SQL table\n"
        "\n\n"
        "Usage: corsika2sql input-card [-u URI] [options]\n"
        "\n"
        ;
    cout << endl;
}

bool ShowWarnings(Database &connection)
{
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
        if (resw.num_rows())
            cout << endl;
        return true;
    }
    catch (const exception &e)
    {
        cerr << "\nSHOW WARNINGS\n\n";
        cerr << "SQL query failed:\n" << e.what() << '\n' <<endl;
        return false;
    }
}


int main(int argc, const char* argv[])
{
    Time start;

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    // ----------------------------- Evaluate options --------------------------
    const string   uri          = conf.Get<string>("uri");
    const string   file         = conf.Get<string>("file");

    const bool     print_insert = conf.Get<bool>("print-insert");

    const bool     force        = conf.Get<bool>("force");
    const bool     noinsert     = conf.Get<bool>("no-insert");
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
    // create INSERT/UPDATE query (calculate positions)

    const mysqlpp::StoreQueryResult res =
        connection.query("EXPLAIN `CorsikaSetup`").store();

    map<string, uint16_t> fields;

    for (size_t i=0; i<res.num_rows(); i++)
    {
        auto field = string(res[i]["Field"]);

        int cnt = 0;

        boost::smatch match;
        if (boost::regex_match(field, match, boost::regex("([^\\[]+)(\\[([0-9]+)\\])$")))
        {
            cnt   = stoi(match[3])+1;
            field = match[1];
        }

        if (fields[field]<cnt)
            fields[field] = cnt;
    }

    fields["SEED"] = 3;
    fields["TELESCOPE"] = 5;

    // -------------------------------------------------------------------------
    // evaluate input card

    vector<string> insert;
    vector<string> seeds;
    vector<string> telescopes;

    string runnr;;
    string buf;

    ifstream fin(file);

    while (getline(fin, buf))
    {
        string line = buf;
        while (1)
        {
            boost::replace_all(buf, "  ", " ");
            if (line==buf)
                break;
            line = buf;
        }

        vector<string> vec = Tools::Split(Tools::Trim(line), " ");

        const auto it = fields.find(vec[0]);
        if (it==fields.end())
        {
            //if (verbose>1)
            //cout << "Not found: " << vec[0] << " " << vec.size()-1 << endl;
            continue;
        }

        const uint16_t N = ::max<uint16_t>(1, it->second);

        if (N<vec.size()-1)
        {
            //if (verbose>1)
            cerr << "Size mismatch: " << vec[0] << " " << N << "<" << vec.size()-1 << endl;
            return 2;
        }

        if (vec[0]=="SEED")
        {
            seeds.emplace_back("'"+vec[1]+"','"+vec[2]+"','"+vec[3]+"'");
            continue;
        }
        if (vec[0]=="TELESCOPE")
        {
            const auto tel = atoi((vec.size()>5?vec[5]:"0").c_str());
            telescopes.emplace_back("'"+vec[1]+"','"+vec[2]+"','"+vec[3]+"'"+"'"+vec[4]+"',"+to_string(tel));
            continue;
        }
        if (vec[0]=="RUNNR")
            runnr = vec[1];

        for (int i=1; i<N+1; i++)
        {
            if (vec[i]=="T")
                vec[i]="1";
            if (vec[i]=="F")
                vec[i]="0";
        }

        if (it->second==0)
            insert.emplace_back("`"+vec[0]+"`='"+vec[1]+"'");
        else
        {
            for (int i=1; i<N+1; i++)
                insert.emplace_back("`"+vec[0]+"["+to_string(i-1)+"]`='"+vec[i]+"'");
        }
    }

    cout << "RUNNR=" << runnr << " => " << file << endl;

    // -------------------------------------------------------------------------
    // insert card data into table

    const string query1 =
        "REPLACE CorsikaSetup SET "+boost::join(insert, ",");

    try
    {
        if (!noinsert)
        {
            const mysqlpp::SimpleResult res1 =
                connection.query(query1).execute();

            if (verbose>0 && res1.info())
                cout << res1.info() << '\n' << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << query1 << "\n\n";
        cerr << "SQL query (" << query1.length() << " bytes) failed:\n" << e.what() << endl;
        return 3;
    }

    if (print_insert)
        cout << query1 << endl;

    if (!ShowWarnings(connection))
        return 4;

    // -------------------------------------------------------------------------
    // insert seed data into table

    if (!seeds.empty())
    {
        int i=0;
        for (auto it=seeds.begin(); it!=seeds.end(); it++)
            it->insert(0, runnr+","+to_string(i++)+",");

        const string query2 =
            "INSERT CorsikaSeed (RUNNR,idx,`SEED[0]`,`SEED[1]`,`SEED[2]`) VALUES ("+boost::join(seeds, "),(")+")";

        try
        {

            const mysqlpp::SimpleResult res2a =
                connection.query("DELETE FROM CorsikaSeed WHERE RUNNR="+runnr).execute();

            if (verbose>0)
                cout << res2a.info() << '\n' << endl;

            if (!noinsert)
            {
                const mysqlpp::SimpleResult res2b =
                    connection.query(query2).execute();

                if (verbose>0)
                    cout << res2b.info() << '\n' << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << query2 << "\n\n";
            cerr << "SQL query (" << query2.length() << " bytes) failed:\n" << e.what() << endl;
            return 5;
        }

        if (print_insert)
            cout << query2 << endl;

        if (!ShowWarnings(connection))
            return 6;
    }

    // -------------------------------------------------------------------------
    // insert telescope data into table

    if (!telescopes.empty())
    {
        for (auto it=telescopes.begin(); it!=telescopes.end(); it++)
            it->insert(0, runnr+",");

        const string query2 =
            "INSERT CorsikaTelescope (RUNNR,X,Y,Z,R,ID) VALUES ("+boost::join(telescopes, "),(")+")";

        try
        {

            const mysqlpp::SimpleResult res2a =
                connection.query("DELETE FROM CorsikaTelescope WHERE RUNNR="+runnr).execute();

            if (verbose>0)
                cout << res2a.info() << '\n' << endl;

            if (!noinsert)
            {
                const mysqlpp::SimpleResult res2b =
                    connection.query(query2).execute();

                if (verbose>0)
                    cout << res2b.info() << '\n' << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << query2 << "\n\n";
            cerr << "SQL query (" << query2.length() << " bytes) failed:\n" << e.what() << endl;
            return 5;
        }

        if (print_insert)
            cout << query2 << endl;

        if (!ShowWarnings(connection))
            return 6;
    }

    // -------------------------------------------------------------------------

    if (verbose>0)
    {
        const auto sec = Time().UnixTime()-start.UnixTime();
        cout << "Total execution time: " << sec << "s\n";
    }

    return 0;
}
