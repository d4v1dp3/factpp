#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#include "Database.h"

#include "tools.h"
#include "Time.h"
#include "Configuration.h"

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TError.h>

#include "FileEntry.h"

using namespace std;
namespace fs = boost::filesystem;

// ------------------------------------------------------------------------

/*
struct Map : pair<string, string>
{
    Map() { }
};

std::istream &operator>>(std::istream &in, Map &m)
{
    const istreambuf_iterator<char> eos;
    string txt(istreambuf_iterator<char>(in), eos);

    const boost::regex expr("((.*)[^\\\\])/(.*)");
    boost::smatch match;
    if (!boost::regex_match(txt, match, expr))
        throw runtime_error("Could not evaluate map argument: "+txt);

    m.first  = match[1].str();
    m.second = match[3].str();

    return in;
}
*/

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Root to SQL");
    control.add_options()
        ("uri,u",          var<string>()->required(), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("file",           var<string>()->required(), "The root file to read from")
        ("create",         po_switch(),               "Create the database if not existing")
        ("drop",           po_switch(),               "Drop the table (implies create)")
        ("tree,t",         var<string>("Events"),     "Name of the root tree to convert")
        ("table",          var<string>(""),           "Name of the table to use (default is the tree name)")
        ("map",            vars<Configuration::Map>(),"A regular expression which is applied to the leaf name befoee it is used as SQL column name)")
        ("sql-type",       vars<Configuration::Map>(),"Allows to overwrite the calculated SQL type for a given column e.g. 'sql-column-name/UNSIGNED IN'")
        ("ignore",         vars<string>(),            "Ignore the given leaf, if the given regular expression matches")
        ("primary",        vars<string>(),            "List of columns to be used as primary keys during table creation (in connection with --create)")
        ("first",          var<int64_t>(int64_t(0)),  "First event to start with (default: 0), mainly for test purpose")
        ("max",            var<int64_t>(int64_t(0)),  "Maximum number of events to process (0: all), mainly for test purpose")
        ("engine",         var<string>(""),           "Database engine to be used when a new table is created")
        ("row-format",     var<string>(""),           "Defines the ROW_FORMAT keyword for table creation")
        ("duplicate",      vars<string>(),            "Specifies an assignment_list for an 'ON DUPLICATE KEY UPDATE' expression")
        ("ignore-errors",  po_switch(),               "Adds the IGNORE keyword to the INSERT query (turns errors into warnings, ignores rows with errors)")
        ("const.*",        var<string>(),             "Insert a constant number into the given column (--const.mycolumn=5). A special case is `/.../.../`")
        ("conditional",    po_switch(),               "Conditional insert. Only insert if no entry exists yet with the constants defined by --const")
        ("delete",         po_switch(),               "Delete all entries first which fit all constant columns defined by --const")
        ("index",          po_switch(),               "If a table is created, all const columns are used as a single index (INDEX)")
        ("unique",         po_switch(),               "If a table is created, all const columns are used as a unqiue index (UNIQUE)")
        ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("no-insert",      po_switch(),               "Does not insert any data into the table")
        ("dry-run",        po_switch(),               "Skip any query which changes the databse (might result in consecutive failures)")
        ("print-connection", po_switch(),             "Print database connection information")
        ("print-ls",       po_switch(),               "Calls TFile::ls()")
        ("print-branches", po_switch(),               "Print the branches found in the tree")
        ("print-leaves",   po_switch(),               "Print the leaves found in the tree (this is what is processed)")
        ("print-insert",   po_switch(),               "Print the INSERT query (note that it contains all data)")
        ("print-create",   po_switch(),               "Print the CREATE query")
        ("print-select",   po_switch(),               "Print the SELECT query for the conditional execution")
        ("print-delete",   po_switch(),               "Print the DELETE query")
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
        "root2sql - Fills the data from a root file into a database\n"
        "\n"
        "For convenience, this documentation uses the extended version of the options, "
        "refer to the output below to get the abbreviations.\n"
        "\n"
        "This is a general purpose tool to fill the contents of a root file into a database "
        "as long as this is technically possible and makes sense. Note that root can even "
        "write complex data like a TH1F into a database, this is not the purpose of this "
        "program.\n"
        "\n"
        "Each root tree has branches and leaves (the basic data types). These leaves can "
        "be read independently of the classes which were used to write the root file. "
        "The default tree to read from is 'Events' but the name can be overwritten "
        "using --tree. The default table name to fill the data into is identical to "
        "the tree name. It can be overwritten using --table.\n"
        "\n"
        "To get a list of the contents (keys and trees) of a root file, you can use --print-ls. "
        "The name of each column to which data is filled from a leave is obtained from "
        "the leaves' names. The leave names can be checked using --print-leaves. "
        "A --print-branches exists for convenience to print only the high-level branches. "
        "Sometimes these names might be quite unconvenient like MTime.fTime.fMilliSec or "
        "just MHillas.fWidth. To allow to simplify column names, regular expressions "
        "(using boost's regex) can be defined to change the names. Note that these regular "
        "expressions are applied one by one on each leaf's name. A valid expression could "
        "be:\n"
        "   --map=MHillas\\.f/\n"
        "which would remove all occurances of 'MHillas.f'. This option can be used more than "
        "once. They are applied in sequence. A single match does not stop the sequence.\n"
        "\n"
        "Sometimes it might also be convenient to skip a leaf. This can be done with "
        "the --ignore resource. If the given regular expresion yields a match, the "
        "leaf will be ignored. Note that the regular expression works on the raw-name "
        "of the leaf not the readily mapped SQL column names. Example:\n"
        "   --ignore=ThetaSq\\..*\n"
        "will skip all leaved which start with 'ThetaSq.'. This option can be used"
        "more than once.\n"
        "\n"
        "The data type of each column is kept as close as possible to the leaves' data "
        "types. If for some reason this is not wanted, the data type of the SQL column "
        "can be overwritten with --sql-type sql-column/sql-ytpe, for example:\n"
        "   --sql-type=FileId/UNSIGNED INT\n"
        "while the first argument of the name of the SQL column to which the data type "
        "should be applied. The second column is the basic SQL data type. The option can "
        "be given more than once.\n"
        "\n"
        "Database interaction:\n"
        "\n"
        "To drop an existing table, --drop can be used.\n"
        "\n"
        "To create a table according to theSQL  column names and data types, --create "
        "can be used. The query used can be printed with --print-create even --create "
        "has not been specified.\n"
        "\n"
        "To choose the columns which should become primary keys, use --primary, "
        "for example:\n"
        "   --primary=col1\n"
        "To define more than one column as primary key, the option can be given more than "
        "once. Note that the combination of these columns must be unique.\n"
        "\n"
        "All columns are created as NOT NULL as default. To force a database engine "
        "and/or a storage format, use --engine and --row-format.\n"
        "\n"
        "Usually, the INSERT query would fail if the PRIMARY key exists already. "
        "This can be avoided using the 'ON DUPLICATE KEY UPDATE' directive. With the "
        "--duplicate, you can specify what should be updated in case of a duplicate key. "
        "To keep the row untouched, you can just update the primary key "
        "with the identical primary key, e.g. --duplicate='MyPrimary=VALUES(MyPrimary)'. "
        "The --duplicate resource can be specified more than once to add more expressions "
        "to the assignment_list. For more details, see the MySQL manual.\n"
        "\n"
        "For debugging purpose, or to just create or drop a table, the final insert "
        "query can be skipped using --no-insert. Note that for performance reason, "
        "all data is collected in memory and a single INSERT query is issued at the "
        "end.\n"
        "\n"
        "Another possibility is to add the IGNORE keyword to the INSERT query by "
        "--ignore-errors, which essentially ignores all errors and turns them into "
        "warnings which are printed after the query succeeded.\n"
        "\n"
        "Using a higher verbosity level (-v), an overview of the written columns or all "
        "processed leaves is printed depending on the verbosity level. The output looks "
        "like the following\n"
        "   Leaf name [root data type] (SQL name)\n"
        "for example\n"
        "   MTime.fTime.fMilliSec [Long64_t] (MilliSec)\n"
        "which means that the leaf MTime.fTime.fMilliSec is detected to be a Long64_t "
        "which is filled into a column called MilliSec. Leaves with non basic data types "
        "are ignored automatically and are marked as (-n/a-). User ignored columns "
        "are marked as (-ignored-).\n"
        "\n"
        "A constant value for the given file can be inserted by using the --const directive. "
        "For example --const.mycolumn=42 would insert 42 into a column called mycolumn. "
        "The column is created as INT UNSIGNED as default which can be altered by "
        "--sql-type. A special case is a value of the form `/regex/format/`. Here, the given "
        "regular expression is applied to the filename and it is newly formated with "
        "the new format string. Uses the standard formatting rules to replace matches "
        "(those used by ECMAScript's replace method).\n"
        "\n"
        "Usually the previously defined constant values are helpful to create an index "
        "which relates unambiguously the inserted data to the file. It might be useful "
        "to delete all data which belongs to this particular file before new data is "
        "entered. This can be achieved with the `--delete` directive. It deletes all "
        "data from the table before inserting new data which fulfills the condition "
        "defined by the `--const` directives.\n"
        "\n"
        "The constant values can also be used for a conditional execution (--conditional). "
        "If any row with the given constant values are found, the execution is stopped "
        "(note that this happend after the table drop/create but before the delete/insert.\n"
        "\n"
        "To ensure efficient access for a conditonal execution, it makes sense to have "
        "an index created for those columns. This can be done during table creation "
        "with the --index option.\n"
        "\n"
        "To create the index as a UNIQUE INDEX, you can use the --unique option which "
        "implies --index.\n"
        "\n"
        "If a query failed, the query is printed to stderr together with the error message. "
        "For the main INSERT query, this is only true if the verbosity level is at least 2 "
        "or the query has less than 80*25 bytes.\n"
        "\n"
        "In case of success, 0 is returned, a value>0 otherwise.\n"
        "\n"
        "Usage: root2sql [options] -uri URI rootfile.root\n"
        "\n"
        ;
    cout << endl;
}

void ErrorHandlerAll(Int_t level, Bool_t abort, const char *location, const char *msg)
{
    if (string(msg).substr(0,24)=="no dictionary for class ")
        return;
    if (string(msg).substr(0,15)=="unknown branch ")
        return;

    DefaultErrorHandler(level, abort, location, msg);
}

int main(int argc, const char* argv[])
{
    Time start;

    gROOT->SetBatch();
    SetErrorHandler(ErrorHandlerAll);

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    // ----------------------------- Evaluate options --------------------------
    const string uri             = conf.Get<string>("uri");
    const string file            = conf.Get<string>("file");
    const string tree            = conf.Get<string>("tree");
    const string table           = conf.Get<string>("table").empty() ? tree : conf.Get<string>("table");

    const uint16_t verbose       = conf.Get<uint16_t>("verbose");
    const int64_t  first         = conf.Get<int64_t>("first");
    const int64_t  max           = conf.Get<int64_t>("max");

    const bool drop              = conf.Get<bool>("drop");
    const bool create            = conf.Get<bool>("create") || drop;
    const bool noinsert          = conf.Get<bool>("no-insert");
    const bool dry_run           = conf.Get<bool>("dry-run");
    const bool conditional       = conf.Get<bool>("conditional");
    const bool run_delete        = conf.Get<bool>("delete");
    const bool index             = conf.Get<bool>("index");
    const bool unique            = conf.Get<bool>("unique");

    const string engine          = conf.Get<string>("engine");
    const string row_format      = conf.Get<string>("row-format");

    const vector<string> duplicate = conf.Vec<string>("duplicate");

    const bool ignore_errors     = conf.Get<bool>("ignore-errors");

    const bool print_connection  = conf.Get<bool>("print-connection");
    const bool print_ls          = conf.Get<bool>("print-ls");
    const bool print_branches    = conf.Get<bool>("print-branches");
    const bool print_leaves      = conf.Get<bool>("print-leaves");
    const bool print_create      = conf.Get<bool>("print-create");
    const bool print_insert      = conf.Get<bool>("print-insert");
    const bool print_select      = conf.Get<bool>("print-select");
    const bool print_delete      = conf.Get<bool>("print-delete");

    const auto mymap    = conf.Vec<Configuration::Map>("map");
    const auto sqltypes = conf.Vec<Configuration::Map>("sql-type");

    const vector<string> _ignore = conf.Vec<string>("ignore");
    const vector<string> primary = conf.Vec<string>("primary");

    // -------------------------------------------------------------------------

    if (verbose>0)
    {
        cout << "\n-------------------------- Evaluating file -------------------------\n";
        cout << "Start Time: " << Time::sql << Time(Time::local) << endl;
    }

    TFile f(file.c_str());
    if (f.IsZombie())
    {
        cerr << "Could not open file " << file << endl;
        return 1;
    }

    if (verbose>0 && !print_ls)
        cout << "File: " << file << endl;

    if (print_ls)
    {
        cout << '\n';
        f.ls();
        cout << '\n';
    }

    TTree *T = 0;
    f.GetObject(tree.c_str(), T);
    if (!T)
    {
        cerr << "Could not open tree " << tree << endl;
        return 2;
    }

    if (verbose>0)
        cout << "Tree: " << tree << endl;

    T->SetMakeClass(1);

    TObjArray *branches = T->GetListOfBranches();
    TObjArray *leaves   = T->GetListOfLeaves();

    if (print_branches)
    {
        cout << '\n';
        branches->Print();
    }

    if (verbose>0)
        cout << T->GetEntriesFast() << " events found." << endl;


    if (verbose>0)
        cout << branches->GetEntries() << " branches found." << endl;

    if (print_leaves)
    {
        cout << '\n';
        leaves->Print();
    }
    if (verbose>0)
        cout << leaves->GetEntries() << " leaves found." << endl;

    string query =
        "CREATE TABLE IF NOT EXISTS `"+table+"`\n"
        "(\n";

    vector<FileEntry::Container> vec;

    const auto fixed = conf.GetWildcardOptions("const.*");

    string where;
    vector<string> vindex;
    for (auto it=fixed.cbegin(); it!=fixed.cend(); it++)
    {
        const string name = it->substr(6);
        string val  = conf.Get<string>(*it);

        boost::smatch match;
        if (boost::regex_match(val, match, boost::regex("\\/(.+)(?<!\\\\)\\/(.*)(?<!\\\\)\\/")))
        {
            const string reg = match[1];
            const string fmt = match[2];

            val = boost::regex_replace(file, boost::regex(reg), fmt.empty()?"$0":fmt,
                                       boost::regex_constants::format_default|boost::regex_constants::format_no_copy);

            if (verbose>0)
            {
                cout << "Regular expression detected for constant column `" << *it << "`\n";
                cout << "Filename converted with /" << reg << "/ to /" << fmt << "/\n";
                cout << "Filename: " << file << '\n';
                cout << "Result: " << val << endl;
            }
        }

        if (verbose>2)
            cout << "\n" << val << " [-const-]";
        if (verbose>1)
            cout << " (" << name << ")";

        string sqltype = "INT UNSIGNED";

        for (auto m=sqltypes.cbegin(); m!=sqltypes.cend(); m++)
            if (m->first==name)
                sqltype = m->second;

        if (!vec.empty())
            query += ",\n";
        query += "   `"+name+"` "+sqltype+" NOT NULL COMMENT '--user--'";

        vec.emplace_back(name, val);
        where += " AND `"+name+"`="+val;
        vindex.emplace_back(name);
    }

    const size_t nvec = vec.size();

    TIter Next(leaves);
    TObject *o = 0;
    while ((o=Next()))
    {
        TLeaf *L = dynamic_cast<TLeaf*>(o);//c.GetLeaf(o->GetName());
        if (!L)
            continue;

        if (verbose>2)
            cout << '\n' << L->GetTitle() << " {" << L->GetTypeName() << "}";

        if (L->GetLenStatic()!=L->GetLen())
        {
            if (verbose>2)
                cout << " (-skipped-)";
            continue;
        }


        string name = o->GetName();

        bool found = false;
        for (auto b=_ignore.cbegin(); b!=_ignore.cend(); b++)
        {
            if (boost::regex_match(name, boost::regex(*b)))
            {
                found = true;
                if (verbose>2)
                    cout << " (-ignored-)";
                break;
            }
        }
        if (found)
            continue;

        const string tn = L->GetTypeName();

        const auto it = FileEntry::LUT.root(tn);
        if (it==FileEntry::LUT.cend())
        {
            if (verbose>2)
                cout << " (-n/a-)";
            continue;
        }

        if (verbose==2)
            cout << '\n' << L->GetTitle() << " {" << L->GetTypeName() << "}";

        for (auto m=mymap.cbegin(); m!=mymap.cend(); m++)
            name = boost::regex_replace(name, boost::regex(m->first), m->second);

        if (verbose>1)
            cout << " (" << name << ")";

        string sqltype = it->sql;

        for (auto m=sqltypes.cbegin(); m!=sqltypes.cend(); m++)
            if (m->first==name)
                sqltype = m->second;

        if (!vec.empty())
            query += ",\n";

        const size_t N = L->GetLenStatic();
        for (size_t i=0; i<N; i++)
        {
            query += "   `"+name;
            if (N>1)
                query += "["+to_string(i)+"]";
            query += "` "+sqltype+" NOT NULL COMMENT '"+o->GetTitle()+"'";
            if (N>1 && i!=N-1)
                query += ",\n";
        }

        vec.emplace_back(o->GetTitle(), name, it->type, L->GetLenStatic());
        T->SetBranchAddress(o->GetTitle(), vec.back().ptr);
    }

    if (verbose>1)
        cout << "\n\n";
    if (verbose>0)
    {
        if (nvec>0)
            cout << nvec << " constant value column(s) configured." << endl;
        cout << vec.size()-nvec << " leaf/leaves setup for reading." << endl;
    }

    UInt_t datatype = 0;
    const bool has_datatype = T->SetBranchAddress("DataType.fVal", &datatype) >= 0;
    if (has_datatype && verbose>0)
        cout << "Rows with DataType.fVal!=1 will be skipped." << endl;

    // -------------------------------------------------------------------------
    // Setting up branch status (must be after all SetBranchAddress)
    T->SetBranchStatus("*", 0);

    Next.Reset();
    while ((o=Next()))
    {
        const TLeaf *L = dynamic_cast<TLeaf*>(o);//c.GetLeaf(o->GetName());
        if (!L)
            continue;

        const TBranch *B = L->GetBranch();
        if (!B)
            continue;

        if (!B->GetAddress())
            continue;

        T->SetBranchStatus(B->GetName(), 1);
        if (verbose>2)
            cout << "Enable Branch: " << B->GetName() << endl;
    }

    // -------------------------------------------------------------------------
    // Checking for database connection

    if (verbose>0)
    {
        cout << "Connecting to database...\n";
        cout << "Client Version: " << mysqlpp::Connection().client_version() << endl;
    }

    Database connection(uri);

    if (verbose>0)
        cout << "Server Version: " << (connection.connected()?connection.server_version():"<n/a>") << endl;

    if (print_connection && connection.connected())
    {
        try
        {
            const auto &res1 = connection.query("SHOW STATUS LIKE 'Compression'").store();
            cout << "Compression of databse connection is " << string(res1[0][1]) << endl;

            const auto &res2 = connection.query("SHOW STATUS LIKE 'Ssl_cipher'").store();
            cout << "Connection to databases is " << (string(res2[0][1]).empty()?"UNENCRYPTED":"ENCRYPTED ("+string(res2[0][1])+")") << endl;
        }
        catch (const exception &e)
        {
            cerr << "\nSHOW STATUS LIKE COMPRESSION\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 3;
        }
    }

    // -------------------------------------------------------------------------

    if (verbose>0)
        cout << "\n--------------------------- Database Table -------------------------" << endl;

    if (!primary.empty())
        query += ",\n   PRIMARY KEY USING BTREE (`"+boost::algorithm::join(primary, "`, `")+"`)";

    if (!vindex.empty() && (index || unique))
        query += ",\n   "+string(unique?"UNIQUE ":"")+"INDEX USING BTREE (`"+boost::algorithm::join(vindex, "`, `")+"`)";

    query +=
        "\n)\n"
        "DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci\n";
    if (!engine.empty())
        query += "ENGINE="+engine+"\n";
    if (!row_format.empty())
        query += "ROW_FORMAT="+row_format+"\n";
    query += "COMMENT='created by "+fs::path(conf.GetName()).filename().string()+"'\n";

    // FIXME: Can we omit the catching to be able to print the
    //        query 'autmatically'?
    try
    {
        if (drop)
        {
            // => Simple result
            if (!dry_run)
                connection.query("DROP TABLE `"+table+"`").execute();
            if (verbose>0)
            {
                if (!dry_run)
                    cout << "Table `" << table << "` dropped." << endl;
                else
                    cout << "Dropping table `" << table << "`skipped!" << endl;
            }

        }
    }
    catch (const exception &e)
    {
        cerr << "DROP TABLE `" << table << "`\n\n";
        cerr << "SQL query failed:\n" << e.what() << '\n' << endl;
        return 4;
    }

    try
    {
        if (create && !dry_run)
            connection.query(query).execute();
    }
    catch (const exception &e)
    {
        cerr << query << "\n\n";
        cerr << "SQL query failed:\n" << e.what() << '\n' << endl;
        return 5;
    }

    if (print_create)
        cout << query << endl;

    if (create && verbose>0)
    {
        if (!dry_run)
            cout << "Table `" << table << "` created." << endl;
        else
            cout << "Creating table `" << table << "`skipped!" << endl;
    }

    try
    {
        if (conditional && !fixed.empty() && !drop)
        {
            const mysqlpp::StoreQueryResult res =
                connection.query("SELECT 1 FROM `"+table+"` WHERE 1"+where+" LIMIT 1").store();

            if (res.num_rows()>0)
            {
                if (verbose>0)
                {
                    cout << "Conditional execution... detected existing rows!\n";
                    cout << "Exit.\n" << endl;
                }
                return 0;
            }
        }
    }
    catch (const exception &e)
    {
        cerr << "SELECT 1 FROM `" << table << "` WHERE 1" << where << " LIMIT 1\n\n";
        cerr << "SQL query failed: " << e.what() << endl;
        return 6;
    }

    if (print_select)
        cout << "SELECT 1 FROM `" << table << "` WHERE 1" << where << " LIMIT 1" << endl;

    try
    {
        if (run_delete)
        {
            if (verbose>0)
                cout << "Deleting rows...";

            if (!fixed.empty() && !drop && !dry_run)
            {
                if (verbose>0)
                    cout << endl;

                const mysqlpp::SimpleResult res =
                    connection.query("DELETE FROM `"+table+"` WHERE 1"+where).execute();

                if (verbose>0)
                    cout << res.rows() << " row(s) deleted.\n" << endl;
            }
            else
                if (verbose>0)
                    cout << " skipped." << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << "DELETE FROM `"+table+"` WHERE 1" << where << "\n\n";
        cerr << "SQL query failed: " << e.what() << endl;
        return 7;
    }

    if (print_delete)
        cout << "DELETE FROM `"+table+"` WHERE 1" << where << endl;


    // -------------------------------------------------------------------------

    if (verbose>0)
        cout << "\n---------------------------- Reading file --------------------------" << endl;

    //query = update ? "UPDATE" : "INSERT";
    query = "INSERT ";
    if (ignore_errors)
        query += "IGNORE ";
    query += "`"+table+"`\n"
        "(\n";

    for (auto c=vec.cbegin(); c!=vec.cend(); c++)
    {
        if (c!=vec.cbegin())
            query += ",\n";

        const size_t N = c->num;
        for (size_t i=0; i<N; i++)
        {
            if (N==1)
                query += "   `"+c->column+"`";
            else
                query += "   `"+c->column+"["+to_string(i)+"]`";

            if (N>1 && i!=N-1)
                query += ",\n";
        }
    }

    query +=
        "\n)\n"
        "VALUES\n";

    size_t count = 0;

    const size_t num = max>0 && first+max<T->GetEntriesFast() ? (first+max) : T->GetEntriesFast();
    for (size_t j=first; j<num; j++)
    {
        T->GetEntry(j);
        if (has_datatype && datatype!=1)
            continue;

        if (count>0)
            query += ",\n";

        query += "(\n";

        for (auto c=vec.cbegin(); c!=vec.cend(); c++)
        {
            if (c!=vec.cbegin())
                query += ",\n";

            const size_t N = c->num;
            for (size_t i=0; i<N; i++)
            {
                query += "   "+c->fmt(i);

                if (print_insert && i==0)
                    query += " /* "+c->column+" -> "+c->branch+" */";

                if (N>1 && i!=N-1)
                    query += ",\n";
            }
        }
        query += "\n)";

        count ++;
    }

    if (!duplicate.empty())
        query += "\nON DUPLICATE KEY UPDATE\n   " + boost::join(duplicate, ",\n   ");

    if (verbose>0)
        cout << count << " out of " << num << " row(s) read from file [N=" << first << ".." << num-1 << "]." << endl;

    if (count==0)
    {
        if (verbose>0)
        {
            cout << "Total execution time: " << Time().UnixTime()-start.UnixTime() << "s.\n";
            cout << "Success.\n" << endl;
        }
        return 0;
    }

    // -------------------------------------------------------------------------

    if (verbose>0)
    {
        cout << "\n--------------------------- Inserting data -------------------------" << endl;
        cout << "Sending INSERT query (" << query.length() << " bytes)"  << endl;
    }

    try
    {
        if (!noinsert && !dry_run)
        {
            auto q = connection.query(query);
            q.execute();
            cout << q.info() << '\n' << endl;
        }
        else
            cout << "Insert query skipped!" << endl;

        if (print_insert)
            cout << query << endl;
    }
    catch (const exception &e)
    {
        if (verbose>1 || query.length()<80*25)
            cerr << query << "\n\n";
        cerr << "SQL query failed (" << query.length() << " bytes):\n" << e.what() << '\n' << endl;
        return 8;
    }

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
            return 9;
        }
    }

    if (print_connection)
    {
        try
        {
            // Exchange _send and _received as it is the view of the server
            const auto &res1 = connection.query("SHOW STATUS LIKE 'Bytes_%'").store();
            cout << left << setw(16) << res1[1]["Variable_name"] << ' ' << Tools::Scientific(res1[0]["Value"]) << endl;
            cout << left << setw(16) << res1[0]["Variable_name"] << ' ' << Tools::Scientific(res1[1]["Value"]) << endl;
            cout << endl;
        }
        catch (const exception &e)
        {
            cerr << "\nSHOW STATUS LIKE 'Bytes_%'\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 10;
        }
    }

    cout << "Success!\n" << endl;
    return 0;
}
