#include "Database.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include "tools.h"
#include "Time.h"
#include "Splitting.h"
#include "FileEntry.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>

using namespace std;

// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Database options");
    control.add_options()
        ("uri,u",         var<string>()->required(),   "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("query,q",       var<string>(""),             "MySQL query (overwrites --file)")
        ("file",          var<string>("rootify.sql"),  "An ASCII file with the MySQL query (overwrites --query)")
        ("ignore-null,i", po_switch(),                 "Do not skip rows containing any NULL field")
        ("display,d",     po_switch(),                 "Displays contents on the screen (most usefull in combination with mysql statements as SHOW or EXPLAIN)")
        ("explain",       po_switch(),                 "Requests an EXPLAIN from the server (shows the server optimized query)\nsee also https://dev.mysql.com/doc/refman/explain-output.html")
        ("profiling",     po_switch(),                 "Turn on profiling and print profile")
        ("var.*",         var<string>(),               "Predefined SQL user variables (@VAR)")
        ("env.*",         vars<string>(),              "Predefined environment for substitutions in the query ($ENV)")
        ("list.*",        var<string>(),               "Predefined environment for substitutions in the query ($ENV). The list is read from the given file (one list entry per line)")
        ("print-connection", po_switch(),              "Print database connection information")
        ("verbose,v",     var<uint16_t>(1),            "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    po::options_description ascii("ASCII output");
    ascii.add_options()
        ("write,w",       var<string>(""),             "Write output to an ascii file")
        ("delimiter",     var<string>(),               "The delimiter used if contents are displayed with --display (default=\\t)")
        ("copy-shabang",  po_switch(),                 "Copy the sha-bang line if exists to the output file")
        ("copy-header",   po_switch(),                 "Copy the header (all line starting with '#'  up to the first non-comment line to the output file")
        ("copy-query",    po_switch(),                 "Copy the query to the ascii output file")
        ("copy-comments", po_switch(),                 "Copy all lines starting with '#' to the output file which are not part of header")
        ("copy-all",      po_switch(),                 "An alias for --copy-header --copy-query --copy-comments")
        ;

    po::options_description root("Root file options");
    root.add_options()
        ("out,o",         var<string>("rootify.root"), "Output root file name")
        ("force,f",       po_switch(),                 "Force overwriting an existing root file ('RECREATE')")
        ("update",        po_switch(),                 "Update an existing root file with the new tree ('UPDATE')")
        ("compression,c", var<uint16_t>(1),            "zlib compression level for the root file")
        ("tree,t",        var<string>("Result"),       "Name of the root tree")
        ("accurate",      po_switch(),                 "Accurate type conversion, otherwise all branches are creates as double which is often more convenient.")
        ("ignore",        vars<string>(),              "Ignore the given columns")
        ("null,n",        po_switch(),                 "Redirect the root output file to /dev/null (mainly for debugging purposes, e.g. performance studies)")
        ("no-fill",       po_switch(),                 "Do not fill events into the root file (mainly for debugging purposes, e.g. performance studies)")
        ;

    po::positional_options_description p;
    p.add("file", 1); // The 1st positional options (n=1)
    p.add("out",  1); // The 2nd positional options (n=1)

    conf.AddOptions(control);
    conf.AddOptions(ascii);
    conf.AddOptions(root);
    conf.AddOptions(Tools::Splitting::options());
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "rootifysql - Converts the result of a mysql query into a root file\n"
        "\n"
        "For convenience, this documentation uses the extended version of the options, "
        "refer to the output below to get the abbreviations.\n"
        "\n"
        "Writes the result of a mysql query into a root file. For each column, a branch is "
        "created of type double with the field name as name. This is usually the column name "
        "if not specified otherwise by the AS mysql directive.\n"
        "\n"
        "Columns with CHAR or VARCHAR as field type are ignored. DATETIME, DATE and TIME "
        "columns are converted to unix time (time_t). Rows containing any file which is "
        "NULL are skipped if not suppressed by the --ignore-null option. Ideally, the query "
        "is compiled in a way that no NULL field is returned. With the --display option the "
        "result of the request is printed on the screen (NULL skipping still in action). "
        "This can be useful to create an ascii file or to show results as 'SHOW DATABASES' "
        "or 'EXPLAIN table'. To redirect the contents into an ascii file, the option -v0 "
        "is useful. To suppress writing to an output file --null can be used.\n"
        "\n"
        "The default is to read the query from a file called rootify.sql. Except if a different "
        "filename is specified by the --file option or a query is given with --query.\n"
        "\n"
        "As a trick, the rootify.sql file can be made excutable (chmod u+x rootify.sql). "
        "If the first line contains '#!rootifysql', the script can be executed directly.\n"
        "\n"
        "Columns whose name start with @ are skipped. If you want them in your output file "
        "give them a name using AS, e.g. 'SELECT @A:=5 AS A'.\n"
        "\n"
        "You can use variables in your sql query like @MyVar and define them on the "
        "command line. In this example with --var.MyVar=5\n"
        "\n"
        "You can use environment definitions for substitutions in your SQL query. "
        "For example --env.TEST=5 would replace $TEST or ${TEST} in your query by 5."
        "If you specify one environment variable more than once, a list is created. "
        "For example --env.TEST=1 --env.TEST=2 --env.TEST=3 would substitute "
        "$TEST or ${TEST} by '1, 2, 3'. This is useful for the SQL `IN` keyword. "
        "You can also read the values for an enviroment substitution from a file "
        "(one element per line), e.g. --env.TEST=file.txt. Empty lines and lines "
        "starting with a # are skipped.\n"
        "\n"
        "Comments in the query-file can be placed according to the SQL standard inline "
        "/*comment*/ or introduced with # (shell script style) or -- (SQL style).\n"
        "\n"
        << Tools::Splitting::usage() <<
        "\n"
        "In case of success, 0 is returned, a value>0 otherwise.\n"
        "\n"
        "Usage: rootifysql [rootify.sql [rootify.root]] [-u URI] [-q query|-f file] [-i] [-o out] [-f] [-cN] [-t tree] [-vN]\n"
        "\n"
        ;
    cout << endl;
}

struct ExplainParser
{
    string sql;

    vector<string> vec;

    string substitute(string _str, const boost::regex &expr)
    {
        boost::smatch match;
        while (boost::regex_search(_str, match, expr, boost::regex_constants::format_first_only))
        {
            const auto &len = match.length();
            const auto &pos = match.position();
            const auto &str = match.str();

            const auto it = find(vec.cbegin(), vec.cend(), str);
            const size_t id = it==vec.cend() ? vec.size() : it-vec.cbegin();

            _str.replace(pos, len, "{"+to_string(id)+"}");

            if (it==vec.cend())
                vec.push_back(str);//.substr(1, str.size()-2));
        }

        return _str;
    }

    string substitute(const string &str, const string &expr)
    {
        return substitute(str, boost::regex(expr));
    }

    vector<string> queries;

    string resub(string str)
    {
        // search for "KEYWORD expression"
        boost::regex reg("\\{[0-9]+\\}");

        boost::smatch match;
        while (boost::regex_search(str, match, reg, boost::regex_constants::format_first_only))
        {
            const auto &len = match.length();
            const auto &pos = match.position();
            const auto &arg = match.str();      // Argument

            const auto idx = atoi(arg.c_str()+1);

            str.replace(pos, len, resub(vec[idx]));
        }

        return str;
    }

    void expression(string expr, size_t indent=0)
    {
        if (expr[0]=='{')
        {
            const auto idx = atoi(expr.c_str()+1);

            // This is a subquery
            if (vec[idx].substr(0,3)=="(/*")
            {
                cout << setw(indent) << ' ' << "(\n";
                find_tokens(vec[idx], indent+4);
                cout << setw(indent) << ' ' << ") ";
            }
            else
                // This is just something to substitute back
                if (vec[idx].substr(0,2)=="({")
                {
                    cout << setw(indent) << ' ' << "(" << resub(vec[idx]) << ") ";
                }
                else
                {
                    if (indent>0)
                        cout << setw(indent) << ' ';
                    cout << resub(vec[idx]);
                }
        }
        else
        {
            if (indent>0)
                cout << setw(indent) << ' ';
            cout << resub(expr);
        }
    }

    void find_tokens(string str, size_t indent=0)
    {
        //         (            COMMENT                  )?(  TOKEN   )?((  {NNN}     | NNN  )(           AS|ON           (   {NNN})   ))?(,)?)
        //regex reg("(\\/\\*\\ select\\#[0-9]+\\ \\*\\/\\ *)?([a-zA-Z ]+)?((\\{[0-9]+\\}|[0-9]+)(\\ ?([Aa][Ss]|[Oo][Nn])\\ ?(\\{[0-9]+\\}))?(,)?)");

        const string _com = "\\/\\*\\ select\\#[0-9]+\\ \\*\\/\\ *";

        const string _tok = "[a-zA-Z_ ]+";

        const string _nnn = "\\{[0-9]+\\}|[0-9]+";

        const string _as  = "\\ ?([Aa][Ss])\\ ?";

        //                   (  _nnn  )     (  _as  (  _nnn  ))?(,)?     // can also match noting in between two {NNN}
        const string _exp = "("+_nnn+")" + "("+_as+"("+_nnn+"))?(,)?";

        // Matche: (  _com  )?       (     (  _tok  )?   (  _exp  )      |      (  _tok  )     )
        boost::regex reg("("+_com+")?"  +  "(" + "("+_tok+")?"+"("+_exp+")"  + "|" +  "("+_tok+")" + ")");

        boost::smatch match;
        while (boost::regex_search(str, match, reg, boost::regex_constants::format_first_only))
        {

            const auto &com   = match.str(1);               // comment
            const auto &tok1  = Tools::Trim(match.str(3));  // token with expression
            const auto &arg1  = match.str(5);               // argument 1
            const auto &as    = match.str(7);               // as
            const auto &arg2  = match.str(8);               // argument 2
            const auto &comma = match.str(9);               // comma
            const auto &tok2  = Tools::Trim(match.str(10)); // token without expression

            if (!com.empty())
                cout << setw(indent) << ' ' << "\033[34m" << com << "\033[0m" << '\n';

            if (!tok1.empty())
                cout << setw(indent) << ' ' << "\033[32m" << tok1 << "\033[0m" << '\n';
            if (!tok2.empty())
                cout << setw(indent) << ' ' << "\033[32m" << tok2 << "\033[0m" << '\n';

            if (!arg1.empty())
            {
                expression(arg1, indent+4);

                if (!as.empty())
                    cout << " \033[33m" << as << "\033[0m ";

                if (!arg2.empty())
                    expression(arg2);

                if (!comma.empty())
                    cout << ',';

                cout << '\n';
            }

            str = str.substr(match.position()+match.length());
        }
    }


    ExplainParser(const string &_sql) : sql(_sql)
    {
        // substitute all strings
        sql = substitute(sql, "'[^']*'");

        // substitute all escaped sequences  (`something`.`something-else`)
        sql = substitute(sql, "`[^`]*`(\\.`[^`]*`)*");

        // substitute all paranthesis
        sql = substitute(sql, "[a-zA-Z0-9_]*\\([^\\(\\)]*\\)");

        //cout << sql << "\n\n";
        find_tokens(sql);
        cout << endl;
    }
};

// Remove queries...
void format(string sql)
{
    ExplainParser p(sql);

    /*

    SELECT
    [ALL | DISTINCT | DISTINCTROW ]
      [HIGH_PRIORITY]
      [STRAIGHT_JOIN]
      [SQL_SMALL_RESULT] [SQL_BIG_RESULT] [SQL_BUFFER_RESULT]
      [SQL_CACHE | SQL_NO_CACHE] [SQL_CALC_FOUND_ROWS]
    select_expr [, select_expr ...]
    [FROM table_references
      [PARTITION partition_list]
    [WHERE where_condition]
    [GROUP BY {col_name | expr | position}, ... [WITH ROLLUP]]
    [HAVING where_condition]
    [WINDOW window_name AS (window_spec)
        [, window_name AS (window_spec)] ...]
    [ORDER BY {col_name | expr | position}
      [ASC | DESC], ... [WITH ROLLUP]]
    [LIMIT {[offset,] row_count | row_count OFFSET offset}]
    [INTO OUTFILE 'file_name'
        [CHARACTER SET charset_name]
        export_options
      | INTO DUMPFILE 'file_name'
      | INTO var_name [, var_name]]
    [FOR {UPDATE | SHARE} [OF tbl_name [, tbl_name] ...] [NOWAIT | SKIP LOCKED] 
      | LOCK IN SHARE MODE]]
      */

    /*
table_references:
    escaped_table_reference [, escaped_table_reference] ...

escaped_table_reference:
    table_reference
  | { OJ table_reference }

table_reference:
    table_factor
  | join_table

table_factor:
    tbl_name [PARTITION (partition_names)]
        [[AS] alias] [index_hint_list]
  | table_subquery [AS] alias [(col_list)]
  | ( table_references )

join_table:
    table_reference [INNER | CROSS] JOIN table_factor [join_condition]
  | table_reference STRAIGHT_JOIN table_factor
  | table_reference STRAIGHT_JOIN table_factor ON conditional_expr
  | table_reference {LEFT|RIGHT} [OUTER] JOIN table_reference join_condition
  | table_reference NATURAL [INNER | {LEFT|RIGHT} [OUTER]] JOIN table_factor

join_condition:
    ON conditional_expr
  | USING (column_list)

index_hint_list:
    index_hint [, index_hint] ...

index_hint:
    USE {INDEX|KEY}
      [FOR {JOIN|ORDER BY|GROUP BY}] ([index_list])
  | IGNORE {INDEX|KEY}
      [FOR {JOIN|ORDER BY|GROUP BY}] (index_list)
  | FORCE {INDEX|KEY}
      [FOR {JOIN|ORDER BY|GROUP BY}] (index_list)

index_list:
    index_name [, index_name] ...
    */

}

int finish(Database &connection, const uint16_t &verbose, const bool &profiling, const bool &print_connection)
{
    if (verbose>0)
    {
        try
        {
            const auto resw =
                connection.query("SHOW WARNINGS").store();

            if (resw.num_rows()>0)
                cout << "\n" << resw.num_rows() << " Warning(s) issued:\n\n";

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
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 1;
        }
    }

    if (profiling)
    {
        try
        {
            const auto N =
                connection.query("SHOW PROFILES").store().num_rows();

            const auto resp =
                connection.query("SHOW PROFILE ALL FOR QUERY "+to_string(verbose?N-1:N)).store();

            cout << '\n';
            cout << left;
            cout << setw(26) << "Status"     << ' ';
            cout << right;
            cout << setw(11) << "Duration"   << ' ';
            cout << setw(11) << "CPU User"   << ' ';
            cout << setw(11) << "CPU System" << '\n';
            cout << "--------------------------------------------------------------\n";
            for (size_t i=0; i<resp.num_rows(); i++)
            {
                const mysqlpp::Row &rowp = resp[i];

                cout << left;
                cout << setw(26) << rowp["Status"] << ' ';
                cout << right;
                cout << setw(11) << rowp["Duration"] << ' ';
                cout << setw(11) << rowp["CPU_user"] << ' ';
                cout << setw(11) << rowp["CPU_system"] << '\n';
            }
            cout << "--------------------------------------------------------------\n";
            cout << endl;
        }
        catch (const exception &e)
        {
            cerr << "\nSHOW PROFILE ALL\n\n";
            cerr << "SQL query failed:\n" << e.what() << '\n' <<endl;
            return 2;
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
            return 3;
        }
    }

    if (verbose>0)
        cout << "Success!\n" << endl;
    return 0;

}

template<typename T>
void Convert(FileEntry::Container &container, const mysqlpp::String &col)
{
    *reinterpret_cast<T*>(container.ptr) = static_cast<T>(col);
}


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
    const string   uri         = conf.Get<string>("uri");
    const string   out         = conf.Get<string>("out");
    const string   file        = conf.Get<string>("file");
    const string   tree        = conf.Get<string>("tree");
    const bool     force       = conf.Get<bool>("force");
    const bool     ignorenull  = conf.Get<bool>("ignore-null");
    const bool     update      = conf.Get<bool>("update");
    const bool     display     = conf.Get<bool>("display");
    const string   write       = conf.Get<string>("write");
    const bool     noout       = conf.Get<bool>("null");
    const bool     nofill      = conf.Get<bool>("no-fill");
    const bool     explain     = conf.Get<bool>("explain");
    const bool     profiling   = conf.Get<bool>("profiling");
    const bool     accurate    = conf.Get<bool>("accurate");
    const uint16_t verbose     = conf.Get<uint16_t>("verbose");
    const uint16_t compression = conf.Get<uint16_t>("compression");
    const string   delimiter   = conf.Has("delimiter") ? conf.Get<string>("delimiter") : "\t";

    const bool copy_all        = conf.Get<bool>("copy-all");
    const bool copy_shabang    = conf.Get<bool>("copy-shabang");
    const bool copy_header     = copy_all || conf.Get<bool>("copy-header");
    const bool copy_query      = copy_all || conf.Get<bool>("copy-query");
    const bool copy_comments   = copy_all || conf.Get<bool>("copy-comments");

    const vector<string> _ignore = conf.Vec<string>("ignore");
    const bool  print_connection = conf.Get<bool>("print-connection");
    //const vector<Map> mymap    = conf.Vec<Map>("map");

    // ----------------------- Setup splitting ---------------------------------

    const auto vars = conf.GetWildcardOptions("var.*");

    vector<string> variables;
    for (auto var=vars.cbegin(); var!=vars.cend(); var++)
        variables.emplace_back('@'+var->substr(4)+":="+Tools::Trim(conf.Get<string>(*var)));

    // -------------------------------------------------------------------------

    /*const*/ Tools::Splitting split(conf);

    if (verbose>0)
    {
        cout << "\n------------------------ Rootify SQL -------------------------" << endl;
        cout << "Start Time: " << Time::sql << Time(Time::local) << endl;
    }

    string query  = conf.Get<string>("query");
    if (query.empty())
    {
        if (verbose>0)
            cout << "Reading query from file '" << file << "'." << endl;

        ifstream fin(file);
        if (!fin)
        {
            cerr << "Could not open query in '" << file << "': " << strerror(errno) << endl;
            return 4;
        }

        getline(fin, query, (char)fin.eof());
    }

    if (query.empty())
    {
        cerr << "No query specified." << endl;
        return 5;
    }

    // -------------------------------------------------------------------------

    map<string, vector<string>> envs;

    const auto &envs1 = conf.GetWildcardOptions("env.*");
    for (auto env=envs1.cbegin(); env!=envs1.cend(); env++)
        envs[env->substr(4)] = conf.Vec<string>(*env);

    const auto &envs2 = conf.GetWildcardOptions("list.*");
    for (auto env=envs2.cbegin(); env!=envs2.cend(); env++)
    {
        const string  fname = conf.Get<string>(*env);
        const string &ident = env->substr(5);

        ifstream fin(fname);
        if (!fin)
        {
            cerr << "Could not open environment in '" << fname << "' for ${" << ident << "}: " << strerror(errno) << endl;
            return 6;
        }
        for (string line; getline(fin, line); )
        {
            const auto &l = Tools::Trim(line);
            if (!l.empty() && l[0]!='#')
                envs[ident].push_back(line);
        }

        if (verbose>0)
            cout << "Found " << envs[ident].size() << " list element(s) for ${" << ident << "}" << endl;
    }

    for (auto env=envs.cbegin(); env!=envs.cend(); env++)
    {
        boost::regex rexpr("\\$(\\{"+env->first+"\\}|"+env->first+"\\b)");
        query = boost::regex_replace(query, rexpr, boost::join(env->second, ", "));
    }

    // -------------------------- Check for file permssion ---------------------
    // Strictly speaking, checking for write permission and existance is not necessary,
    // but it is convenient that the user does not find out that it failed after
    // waiting long for the query result
    //

    // I am using root here instead of boost to be
    // consistent with the access pattern by TFile
    TString path(noout?"/dev/null":out.c_str());
    gSystem->ExpandPathName(path);

    if (!noout)
    {
        FileStat_t stat;
        const Int_t  exist  = !gSystem->GetPathInfo(path, stat);
        const Bool_t _write = !gSystem->AccessPathName(path,  kWritePermission) && R_ISREG(stat.fMode);

        if ((update && !exist) || (update && exist && !_write) || (force && exist && !_write))
        {
            cerr << "File '" << path << "' is not writable." << endl;
            return 7;
        }

        if (!update && !force && exist)
        {
            cerr << "File '" << path << "' already exists." << endl;
            return 8;
        }
    }

    Time start2;

    // --------------------------- Connect to database -------------------------------------------------

    if (*query.rbegin()!='\n')
        query += '\n';

    if (verbose>0)
    {
        cout << "Connecting to database...\n";
        cout << "Client Version: " << mysqlpp::Connection().client_version() << endl;
    }

    Database connection(uri); // Keep alive while fetching rows

    if (verbose>0)
        cout << "Server Version: " << connection.server_version() << endl;

    if (print_connection)
    {
        try
        {
            const auto &res1 = connection.query("SHOW STATUS LIKE 'Compression'").store();
            cout << "Compression of database connection is " << string(res1[0][1]) << endl;

            const auto &res2 = connection.query("SHOW STATUS LIKE 'Ssl_cipher'").store();
            cout << "Connection to databases is " << (string(res2[0][1]).empty()?"UNENCRYPTED":"ENCRYPTED ("+string(res2[0][1])+")") << endl;
        }
        catch (const exception &e)
        {
            cerr << "\nSHOW STATUS LIKE 'Compression'\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 9;
        }
    }

    try
    {
        if (profiling)
            connection.query("SET PROFILING=1").execute();
    }
    catch (const exception &e)
    {
        cerr << "\nSET profiling=1\n\n";
        cerr << "SQL query failed:\n" << e.what() << endl;
        return 10;
    }

    // -------------------------- Set user defined variables -------------------
    if (variables.size()>0)
    {
        if (verbose>0)
            cout << "Setting user defined variables..." << endl;

        const string varset =
            "SET\n   "+boost::algorithm::join(variables, ",\n   ");

        try
        {
            connection.query(varset).execute();
        }
        catch (const exception &e)
        {
            cerr << '\n' << varset << "\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 11;
        }

        if (verbose>2)
            cout << '\n' << varset << '\n' << endl;
    }

    // ------------------------- Explain query if requested --------------------

    if (explain)
    {
        try
        {
            const auto res0 =
                connection.query("EXPLAIN FORMAT=JSON "+query).store();

            cout << res0[0][0] << endl;
            cout << endl;

            const mysqlpp::StoreQueryResult res1 =
                connection.query("EXPLAIN "+query).store();

            for (size_t i=0; i<res1.num_rows(); i++)
            {
                const mysqlpp::Row &row = res1[i];

                cout << "\nid           : " << row["id"];
                cout << "\nselect type  : " << row["select_type"];

                if (!row["table"].is_null())
                    cout << "\ntable        : " << row["table"];

                if (!row["partitions"].is_null())
                    cout << "\npartitions   : " << row["partitions"];

                if (!row["key"].is_null())
                    cout << "\nselected key : " << row["key"] << " [len=" << row["key_len"] << "] out of (" << row["possible_keys"] << ")";

                if (!row["type"].is_null())
                    cout << "\njoin type    : " << row["type"];

                //if (!row["possible_keys"].is_null())
                //    cout << "\npossible_keys: " << row["possible_keys"];

                //if (!row["key_len"].is_null())
                //    cout << "\nkey_len      : " << row["key_len"];

                if (!row["ref"].is_null())
                    cout << "\nref          : (" << row["ref"] << ") compared to the index";

                if (!row["rows"].is_null())
                    cout << "\nrows         : " << row["rows"];

                if (!row["filtered"].is_null())
                    cout << "\nfiltered     : " << row["filtered"];

                if (!row["extra"].is_null())
                    cout << "\nExtra        : " << row["extra"];

                cout << endl;
            }

            cout << endl;

            const mysqlpp::StoreQueryResult res2 =
                connection.query("SHOW WARNINGS").store();

            for (size_t i=0; i<res2.num_rows(); i++)
            {
                const mysqlpp::Row &row = res2[i];

                // 1003 //
                cout << row["Level"] << '[' << row["Code"] << "]:\n";
                if (uint32_t(row["Code"])==1003)
                    format(row["Message"].c_str());
                else
                    cout << row["Message"] << '\n' << endl;

            }

        }
        catch (const exception &e)
        {
            cerr << '\n' << query << "\n\n";
            cerr << "SQL query failed:\n" << e.what() << endl;
            return 12;
        }

        return 0;
    }

    // -------------------------- Request data from database -------------------
    if (verbose>0)
        cout << "Requesting data... please be patient!" << endl;

    if (verbose>2)
        cout << '\n' << query << endl;

    const mysqlpp::UseQueryResult res =
        connection.query(query).use();

    // -------------------------------------------------------------------------

    if (verbose>0)
    {
        cout << "Opening file '" << path << "' [compression=" << compression << "]...\n";
        cout << "Writing data to tree '" << tree << "'" << (nofill?" (--skipped--)":"") << endl;
        split.print();
    }

    // ----------------------------- Open output file --------------------------
    TFile tfile(path, update?"UPDATE":(force?"RECREATE":"CREATE"), "Rootify SQL", compression);
    if (tfile.IsZombie())
        return 13;

    // -------------------------------------------------------------------------

    // get the first row to get the field description
    mysqlpp::Row row = res.fetch_row();
    if (!row)
    {
        cerr << "Empty set returned... nothing to write." << endl;
        return finish(connection, verbose, profiling, print_connection)+20;
    }

    if (verbose>0)
        cout << "Trying to setup " << row.size() << " branches..." << endl;

    if (verbose>1)
        cout << endl;

    const mysqlpp::FieldNames &l = *row.field_list().list;

    vector<FileEntry::Container> container;

    UInt_t cols = 0;

    // IMPLEMENT FILE SPLITTING!
    // OpenFile(tree, query)
    // SetupColumns
    // WriteRow
    // CloseFile

    // Ratio[3]: 50%, 20%, 30%
    // File[x3]: root, cout, fout


    // -------------------- Configure branches of TTree ------------------------
    vector<TTree*> ttree;

    if (split.empty())
        ttree.emplace_back(new TTree(tree.c_str(), query.c_str()));
    else
        for (size_t i=0; i<split.size(); i++)
            ttree.emplace_back(new TTree((tree+"["+to_string(i)+"]").c_str(), query.c_str()));

    size_t skipno  = 0;
    size_t skipat  = 0;
    size_t skipreg = 0;
    size_t skipch  = 0;
    for (size_t i=0; i<l.size(); i++)
    {
        string t = row[i].type().sql_name();

        bool skip = false;

        // Remove trailing " NULL"
        if (t.find(" NOT NULL")==t.size()-9)
            t = t.substr(0, t.size()-9);
        if (t.find(" NULL")==t.size()-5)
            t = t.substr(0, t.size()-5);

        // Get FileEntry description corresponding to the sql type
        const auto it = FileEntry::LUT.sql(t);

        // Skip all columns that do not follow a convertible type
        if (it==FileEntry::LUT.end())
        {
            skip = true;
            skipno++;
        }

        // For valid colums, check if they are of a type that can not be written to a root file
        if (!skip && (it->type==FileEntry::kVarchar || it->type==FileEntry::kChar))
        {
            skip = true;
            skipch++;
        }

        // Check if there is any user request for skipping a column
        if (!skip)
        {
            for (auto pattern=_ignore.cbegin(); pattern!=_ignore.cend(); pattern++)
            {
                if (boost::regex_match(l[i], boost::regex(*pattern)))
                {
                    skip = true;
                    skipreg++;
                    break;
                }
            }
        }

        // Skip all columns that start with an @ (variable names)
        if (!skip && l[i][0]=='@')
        {
            skip = true;
            skipat++;
        }

        // Create the 'leaflist'. If no accurate conversion is requested, create doubles for all leaves
        const string leaflist = l[i] + "/" + (accurate ? it->branch : 'D');

        if (verbose>1)
            cout << (skip?" - ":" + ") << leaflist.c_str() << " [" << t << "] {" << (it==FileEntry::LUT.end()?'-':it->branch) << "}\n";

        // Create the container entry (must be emplace_back due to the std::string)
        if (accurate)
            container.emplace_back(leaflist, "", it->type);
        else
            container.emplace_back(leaflist, it->type);

        if (skip)
            continue;

        // Create corresponding branches in all trees
        for (auto itree=ttree.begin(); itree!=ttree.end(); itree++)
            itree[0]->Branch(l[i].c_str(), container[i].ptr, leaflist.c_str());

        cols++;
    }
    // -------------------------------------------------------------------------

    if (verbose>1)
        cout << endl;
    if (verbose>0)
    {
        if (skipno)
            cout << skipno << " branches skipped because no suitable type available." << endl;
        if (skipch)
            cout << skipch << " branches skipped because type is a character string." << endl;
        if (skipreg)
            cout << skipreg << " branches skipped due to ignore list." << endl;
        if (skipat)
            cout << skipat  << " branches skipped due to name starting with @." << endl;
        cout << "Configured " << cols << " branches.\nFilling branches..." << endl;
    }

    // ------------------------- Open the ascii files --------------------------

    vector<ofstream> fout;
    if (!write.empty())
    {
        vector<string> names;
        if (split.empty())
            names.emplace_back(write);
        else
            for (size_t i=0; i<split.size(); i++)
                names.emplace_back(write+"-"+to_string(i));

        for (auto it=names.cbegin(); it!=names.cend(); it++)
        {
            fout.emplace_back(*it);
            if (!*fout.rbegin())
                cout << "WARNING: Writing to '" << write << "' failed: " << strerror(errno) << endl;
        }
    }

    // ----------------------- Prepare the ascii comment -----------------------

    string contents;

    istringstream istr(query);
    size_t line = 0;
    bool header = true;
    while (istr)
    {
        string ibuf;
        getline(istr, ibuf);
        const string sbuf = Tools::Trim(ibuf);

        const bool shabang = line==0 && ibuf[0]=='#' && ibuf[1]=='!';
        const bool comment = sbuf[0]=='#' && !shabang;
        const bool isquery = !shabang && !comment;
        if (isquery)
            header = false;

        line++;

        if ((copy_shabang  && shabang) ||
            (copy_header   && comment && header)  ||
            (copy_query    && isquery) ||
            (copy_comments && comment && !header))
            contents += '#' + ibuf + '\n';
    }

    // ----------------------- Write the ascii headers -------------------------

    ostringstream htxt;
    if (display || !fout.empty())
        htxt << row.field_list(delimiter.c_str());

    if (display)
    {
        cout << endl;
        cout << contents << endl;
        cout << "# " << htxt.str() << endl;
    }
    for (auto ff=fout.begin(); ff!=fout.end(); ff++)
    {
        *ff << contents;
        *ff << "# " << htxt.str() << endl;
    }

    // ---------------------- Fill TTree with DB data --------------------------

    size_t count = 0;
    size_t skip  = 0;
    do
    {
        size_t index = split.index(count++);

        ostringstream rtxt;
        if (display || !fout.empty())
            rtxt << row.value_list(delimiter.c_str(), mysqlpp::do_nothing);

        if (display)
            cout << rtxt.str() << '\n';
        if (!fout.empty())
            fout[index] << rtxt.str() << '\n';

        size_t idx=0;
        for (auto col=row.begin(); col!=row.end(); col++, idx++)
        {
            if (!ignorenull && col->is_null())
            {
                skip++;
                break;
            }

            if (accurate)
            {
                // Do an accurate type conversion and assign to the memory allocated as branch-address
                switch (container[idx].type)
                {
                case FileEntry::kBool:   Convert<bool>    (container[idx], *col); break;
                case FileEntry::kFloat:  Convert<float>   (container[idx], *col); break;
                case FileEntry::kDecimal:
                case FileEntry::kNumeric:
                case FileEntry::kDouble: Convert<double>  (container[idx], *col); break;
                case FileEntry::kUInt64: Convert<uint64_t>(container[idx], *col); break;
                case FileEntry::kInt64:  Convert<int64_t> (container[idx], *col); break;
                case FileEntry::kUInt32: Convert<uint32_t>(container[idx], *col); break;
                case FileEntry::kInt32:  Convert<int32_t> (container[idx], *col); break;
                case FileEntry::kUInt16: Convert<uint16_t>(container[idx], *col); break;
                case FileEntry::kInt16:  Convert<int16_t> (container[idx], *col); break;
                case FileEntry::kUInt8:  Convert<uint8_t> (container[idx], *col); break;
                case FileEntry::kInt8:   
                case FileEntry::kDate:
                    *reinterpret_cast<uint64_t*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::Date(*col));
                    break;
                case FileEntry::kDateTime:
                    *reinterpret_cast<uint64_t*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::DateTime(*col));
                    break;
                case FileEntry::kTime:
                    *reinterpret_cast<uint32_t*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::Time(*col));
                    break;
                default:
                    break;
                }
            }
            else
            {
                // Convert everything to double, no matter what... and assign to the memory allocated as branch-address
                switch (container[idx].type)
                {
                case FileEntry::kBool:
                case FileEntry::kFloat:
                case FileEntry::kDecimal:
                case FileEntry::kNumeric:
                case FileEntry::kDouble:
                case FileEntry::kUInt64:
                case FileEntry::kInt64:
                case FileEntry::kUInt32:
                case FileEntry::kInt32:
                case FileEntry::kUInt16:
                case FileEntry::kInt16:
                case FileEntry::kUInt8:
                case FileEntry::kInt8:
                    Convert<double>(container[idx], *col);
                    break;
                case FileEntry::kDate:
                    *reinterpret_cast<double*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::Date(*col));
                    break;
                case FileEntry::kDateTime:
                    *reinterpret_cast<double*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::DateTime(*col));
                    break;
                case FileEntry::kTime:
                    *reinterpret_cast<double*>(container[idx].ptr) = static_cast<time_t>(mysqlpp::Time(*col));
                    break;
                default:
                    break;
                }
            }
        }

        if (idx==row.size() && !nofill)
            ttree[index]->Fill();

        row = res.fetch_row();


    } while (row);

    // -------------------------------------------------------------------------

    if (display)
        cout << '\n' << endl;

    if (verbose>0)
    {
        cout << count << " rows fetched." << endl;
        if (skip>0)
            cout << skip << " rows skipped due to NULL field." << endl;

        for (size_t i=0; i<ttree.size(); i++)
            cout << ttree[i]->GetEntries() << " rows filled into tree #" << i << "." << endl;
    }

    for (auto it=ttree.begin(); it!=ttree.end(); it++)
        (*it)->Write();
    tfile.Close();

    if (verbose>0)
    {
        const auto sec = Time().UnixTime()-start.UnixTime();

        cout << Tools::Scientific(tfile.GetSize()) << "B written to disk.\n";
        cout << "File closed.\n";
        cout << "Execution time: " << sec << "s ";
        cout << "(" << Tools::Fractional(sec/count) << "s/row)\n";
        cout << "--------------------------------------------------------------" << endl;
    }

    return finish(connection, verbose, profiling, print_connection);
}
