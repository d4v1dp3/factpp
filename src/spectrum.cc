#include <thread>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "Database.h"
#include "tools.h"
#include "Time.h"
#include "Configuration.h"

#ifdef HAVE_HIGHLIGHT
#include "srchilite/sourcehighlight.h"
#include "srchilite/langmap.h"
#endif

#ifdef HAVE_ROOT
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TRolke.h"
#include "TFeldmanCousins.h"
#endif

using namespace std;
using boost::adaptors::transformed;

namespace fs = boost::filesystem;

// ------------------------------- Binning ----------------------------------

struct Binning : std::set<double>
{
    bool equidist { false };

    Binning() { }
    Binning(const Binning &m) : std::set<double>(m), equidist(m.equidist) { }
    Binning(size_t cnt, double mn, double mx) { set(cnt, mn, mx); }
    void set(size_t cnt, double mn, double mx)
    {
        if (cnt==0)
            return;

        if (empty())
            equidist = true;

        const double min = ::min(mn, mx);
        const double max = ::max(mn, mx);

        const double stp = (max-min)/cnt;

        for (size_t i=0; i<=cnt; i++)
            emplace(min+i*stp);
    }

    void add(double val)
    {
        emplace(val);
        equidist = false;
    }

    Binning &operator+=(const vector<double> &v)
    {
        if (!v.empty())
        {
            insert(v.cbegin(), v.cend());
            equidist = false;
        }
        return *this;
    }

    Binning operator+(const vector<double> &v)
    {
        return Binning(*this) += v;
    }

    void add(const double &val)
    {
        emplace(val);
        equidist = false;
    }

    string list() const
    {
        return boost::join(*this | transformed([](double d) { return std::to_string(d); }), ",");
    }

    string str() const
    {
        if (!equidist)
            return list();

        return to_string(size()-1)+":"+to_string(*begin())+","+to_string(*rbegin());
    }

    vector<double> vec() const { return vector<double>(begin(), end()); }

    //double operator[](size_t i) const { return vec().at(i); }
};

std::istream &operator>>(std::istream &in, Binning &m)
{
    const istreambuf_iterator<char> eos;
    string txt(istreambuf_iterator<char>(in), eos);

    const boost::regex expr(
                            "([0-9]+)[ /,:;]+"
                            "([-+]?[0-9]*[.]?[0-9]+([eE][-+]?[0-9]+)?)[ /,:;]+"
                            "([-+]?[0-9]*[.]?[0-9]+([eE][-+]?[0-9]+)?)"
                           );
    boost::smatch match;
    if (!boost::regex_match(txt, match, expr))
        throw runtime_error("Could not evaluate binning: "+txt);

    m = Binning(stoi(match[1].str()), stof(match[2].str()), stof(match[4].str()));

    return in;
}

std::ostream &operator<<(std::ostream &out, const Binning &m)
{
    return out << m.str();
}

// ---------------------------- Configuration -------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Calcsource options");
    control.add_options()
        ("uri,u",          var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
         , "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("out,o", var<string>(conf.GetName()), "Defines the prefix (with path) of the output files.")
        ("confidence-level,c", var<double>(0.99), "Confidence level for the calculation of the upper limits.")
        ("feldman-cousins", po_bool(), "Calculate Feldman-Cousins ULs (slow and only minor difference to Rolke).")
        ;

    po::options_description binnings("Binnings");
    binnings.add_options()
        ("theta",             var<Binning>(Binning(90, 0, 90)),  "Add equidistant bins in theta (degrees). Syntax: N,lo,hi (Number N of equidistant bins between lo and hi)")
        ("theta-bin",         vars<double>(),                    "Add a bin-edge to the theta binning (degree)")
        ("energy-dense",      var<Binning>(Binning(30, 2, 5)),   "Add equidistant bins in log10 simulated energy. Syntax: N,lo,hi (Number N of equidistant bins between lo and hi)")
        ("energy-dense-bin",  vars<double>(),                    "Add a bin-edge to the binnnig in log10 simulated enegry")
        ("energy-sparse",     var<Binning>(Binning(15, 2, 5)),   "Add equidistant bins in log10 estimated energy. Syntax: N,lo,hi (Number N of equidistant bins between lo and hi)")
        ("energy-sparse-bin", vars<double>(),                    "Add a bin-edge to the binning in log10 estimated enegry")
        ("impact",            var<Binning>(Binning(28, 0, 280)), "Add equidistant bins in impact in meter. Syntax: N,lo,hi (Number N of equidistant bins between lo and hi)")
        ("impact-bin",        vars<double>(),                    "Add a bin-edge to the binning in impact in meter")
        ;

    po::options_description analysis("Analysis Setup");
    analysis.add_options()
        ("analysis",    var<string>("analysis.sql"),   "File with the analysis query. A default file is created automatically in the <prefix> directory it does not exist.")
        ("source-key", var<uint16_t>(5),          "Source key to be used in data file selection.")
        ("selector",   vars<string>(),            "WHERE clause to be used in data file selection.")
        ("estimator",  var<string>()->required(), "Energy estimator to be used.")
        ("spectrum",   var<string>()->required(), "Spectral shape for re-weighting of simulated 'Energy'")
        ("env.*",      var<string>(),             "Define a variable that is replaced in all queries automatically.")
        ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("dry-run",          po_bool(),         "Only write the queries to the query.log file. Internally overwrites uri with an empty string.")
        ("print-connection", po_bool(),         "Print database connection information")
#ifdef HAVE_HIGHLIGHT
        ("print-queries",    po_bool(),         "Print all queries to the console. They are automatically highlighted.")
#else
        ("print-queries",    po_bool(),         "Print all queries to the console. (For highlighting recompile with 'libsource-highlight-dev' installed)")
#endif
        ("mc-only",          po_bool(),         "Do not run a data related queries (except observation times)")
        ("verbose,v",        var<uint16_t>(1),  "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    //po::positional_options_description p;
    //p.add("file", 1); // The 1st positional options (n=1)

    conf.AddOptions(control);
    conf.AddOptions(binnings);
    conf.AddOptions(analysis);
    conf.AddOptions(debug);
    //conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    //78 .............................................................................
    cout <<
        "spectrum - Calculate a spectrum with classical algorithms\n"
        "\n\n"
        "Usage: spectrum [-u URI] [options]\n"
        "\n"
        ;
    cout << endl;
}



// ----------------------------- Indentation --------------------------------

class sindent : public std::streambuf
{
    std::streambuf *sbuf   { nullptr };
    std::ostream   *owner  { nullptr };
    int             lastch { 0       }; // start-of-line
    std::string     str;

protected:
    virtual int overflow(int ch)
    {
        if (lastch=='\n' && ch != '\n' )
            sbuf->sputn(str.data(), str.size());

        return sbuf->sputc(lastch = ch);
    }

public:
    explicit sindent(std::streambuf *dest, uint16_t indent=0)
        : sbuf(dest), str(indent, ' ')
    {
    }

    explicit sindent(std::ostream& dest, uint16_t indent=0)
        : sbuf(dest.rdbuf()), owner(&dest), str(indent, ' ')
    {
        owner->rdbuf(this);
    }

    virtual ~sindent()
    {
        if (owner)
            owner->rdbuf(sbuf);
    }

    void set(uint16_t w) { str.assign(w, ' '); }
};

struct indent
{
    uint16_t w;
    indent(uint16_t _w=0) : w(_w) { }
};

std::ostream &operator<<(std::ostream &out, indent m)
{
    sindent *buf=dynamic_cast<sindent*>(out.rdbuf());
    if (buf)
        buf->set(m.w);
    return out;
}

struct separator
{
    string s;
    uint16_t n { 0 };
    separator(string _s="") : s(_s) { };
    separator(char c='-', uint16_t _n=78) : s(_n, c), n(_n) { };
};

std::ostream &operator<<(std::ostream &out, separator m)
{
    if (m.n)
        out << m.s;
    else
    {
        const uint8_t l = (78-m.s.size())/2-3;
        out << string(l<1?1:l, '-') << "={ " << m.s << " }=" << string(l<1?1:l, '-');
        if (m.s.size()%2)
            out << '-';
    }
    return out;
}

// ------------------------------- MySQL++ ----------------------------------

bool ShowWarnings(Database &connection)
{
    if (!connection.connected())
        return true;

    try
    {
        const auto resw =
            connection.query("SHOW WARNINGS").store();

        for (size_t i=0; i<resw.num_rows(); i++)
        {
            const mysqlpp::Row &roww = resw[i];

            cout << "\033[31m";
            cout << roww["Level"] << '[' << roww["Code"] << "]: ";
            cout << roww["Message"] << "\033[0m" << '\n';
        }
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

size_t Dump(ostream &out, Database &connection, const string &table)
{
    if (!connection.connected())
        return 0;

    out << connection.query("SHOW CREATE TABLE `"+table+"`").store()[0][1] << ";\n\n";

    const mysqlpp::StoreQueryResult res = connection.query("SELECT * FROM `"+table+"`").store();

    const string fields = boost::join(res.fields() | transformed([](const mysqlpp::Field &r) { return "`"+string(r.name())+"`"; }), ", ");

    out << "INSERT INTO `" << table << "` ( " << fields << " ) VALUES\n";
    out << boost::join(res | transformed([](const mysqlpp::Row &row) { return "  ( "+boost::join(row | transformed([](const mysqlpp::String &s) { return string(s);}), ", ")+" )";}), ",\n") << ";";
    out << "\n" << endl;

    return res.num_rows();
}

void PrintQuery(const string &query)
{
#ifdef HAVE_HIGHLIGHT
    stringstream qstr(query);
    srchilite::SourceHighlight sourceHighlight("esc256.outlang");
    sourceHighlight.setStyleFile("esc256.style");
    sourceHighlight.setGenerateLineNumbers();
    sourceHighlight.setLineNumberDigits(3);
    //sourceHighlight.setLineNumberPad(' ')
    sourceHighlight.highlight(qstr, cout, "sql.lang");
    cout << endl;
#else
    cout << query << endl;
#endif
}

void CreateBinning(Database &connection, ostream &qlog, const Binning &bins, const string &name, const string &comment)
{
    mysqlpp::Query query0(&connection);
    query0 <<
        "CREATE TEMPORARY TABLE Binning" << name << "\n"
        "(\n"
        "   bin INT    NOT NULL COMMENT 'Bin index (" << name << ")',\n"
        "   lo  DOUBLE NOT NULL COMMENT 'Lower bin edge (" << name << ")',\n"
        "   hi  DOUBLE NOT NULL COMMENT 'Upper bin edge (" << name << ")',\n"
        "   PRIMARY KEY (bin) USING HASH\n"
        ") COMMENT='" << comment << "'";

    qlog << query0 << ";\n" << endl;
    if (connection.connected())
        query0.execute();

    mysqlpp::Query query1(&connection);
    query1 <<
        "INSERT INTO Binning" << name << " ( bin, lo, hi ) VALUES\n";

    // FIXME: Case of 1 and 2 bins

    // Bin  0: is the underflow bin...
    // Bin  N: is the overflow  bin...
    // Bin -1: if argument is NULL

    const auto vec = bins.vec();
    for (size_t i=1; i<vec.size()-1; i++)
        query1 << "  ( " << i << ", " << vec[i-1] << ", " << vec[i] << " ),\n";
    query1 << "  ( " << vec.size()-1 << ", " << vec[vec.size()-2] << ", " << vec[vec.size()-1] << " )\n";

    qlog << query1 << ";\n" << endl;

    if (connection.connected())
        cout << query1.execute().info() << endl;
    ShowWarnings(connection);
}

// ----------------------------- ROOT Histogram -----------------------------

/*
A bit of hackery, so just sharing for fun.

   #define with(T, ...) ([&]{ T ${}; __VA_ARGS__; return $; }())

And use it like:

   MyFunction(with(Params,
      $.Name = "Foo Bar",
      $.Age  = 18
   ));

which expands to:

   MyFunction(([&] {
      Params ${};
    $.Name = "Foo Bar", $.Age = 18;
    return $;
   }()));
*/
struct Histogram
{
    // A workaround to be able to set a default also in C++11
    /*
    template<typename T, T def>
    struct Value
    {
        T t { def };
        Value() = default;
        Value(const T &_t) : t(_t) { }
        operator T() const { return t; }
    };*/

    string  name;
    string  title;
    string  dir;
    Binning binningx;
    Binning binningy;
    string  table;
    string  x;
    string  y;
    string  v;
    string  err;
    string  axisx;
    string  axisy;
    string  axisz;
    bool    stats;
    //Value<bool,true> stats;
};

#ifdef HAVE_ROOT

TH1 *CreateHistogram(const Histogram &hist)
{
    const char *name = hist.name.empty() ? hist.v.c_str() : hist.name.c_str();

    cout << "Creating Histogram '" << hist.dir << "/" << name << "'" << endl;

    const auto vecx = hist.binningx.vec();
    const auto vecy = hist.binningy.vec();

    TH1 *h = 0;

    if (hist.y.empty())
    {
        h = hist.binningx.equidist ?
            new TH1D(name, hist.title.c_str(), vecx.size()-1, vecx.front(), vecx.back()) :
            new TH1D(name, hist.title.c_str(), vecx.size()-1, vecx.data());
    }
    else
    {
        if (hist.binningy.equidist)
        {
            h = hist.binningx.equidist ?
                new TH2D(name, hist.title.c_str(), vecx.size()-1, vecx.front(), vecx.back(), vecy.size()-1, vecy.front(), vecy.back()) :
                new TH2D(name, hist.title.c_str(), vecx.size()-1, vecx.data(), vecy.size()-1, vecy.front(), vecy.back());
        }
        else
        {
            h = hist.binningx.equidist ?
                new TH2D(name, hist.title.c_str(), vecx.size()-1, vecx.front(), vecx.back(), vecy.size()-1, vecy.front(), vecy.back()) :
                new TH2D(name, hist.title.c_str(), vecx.size()-1, vecx.data(), vecy.size()-1, vecy.front(), vecy.back());
        }
    }

    h->SetXTitle(hist.axisx.c_str());
    h->SetYTitle(hist.axisy.c_str());
    h->SetZTitle(hist.axisz.c_str());

    h->SetMarkerStyle(kFullDotMedium);

    h->SetStats(hist.stats);

    return h;
}

void WriteHistogram(TH1 *h, const string &directory)
{
    gFile->cd();
    TDirectory *dir = gFile->GetDirectory(directory.c_str());
    if (dir)
        dir->cd();
    else
    {
        gFile->mkdir(directory.c_str());
        gFile->cd(directory.c_str());
    }
    h->Write();
}
#endif

void WriteHistogram(Database &connection, const Histogram &hist)
{
#ifdef HAVE_ROOT
    if (!connection.connected())
        return;

    TH1 *h = CreateHistogram(hist);

    mysqlpp::Query query(&connection);
    query << "SELECT `%0:x` AS X,%1:y `%2:v` AS V%3:err FROM `%4:table`";
    query.parse();

    query.template_defaults["table"] = hist.table.c_str();

    query.template_defaults["x"] = hist.x.c_str();
    query.template_defaults["v"] = hist.v.c_str();
    if (!hist.y.empty())
        query.template_defaults["y"]   = (" `"+hist.y+"` AS Y,").c_str();
    if (!hist.err.empty())
        query.template_defaults["err"] = (", `"+hist.err+"` AS E").c_str();

    // PrintQuery(query.str());

    const mysqlpp::StoreQueryResult res = query.store();

    for (auto ir=res.cbegin(); ir!=res.cend(); ir++)
    {
        const auto &row = *ir;

        try
        {
            const uint32_t x = row["X"];
            const double   v = row["V"];
            if (x>uint32_t(h->GetNbinsX()+1))
                continue;

            try
            {
                const uint32_t y = row["Y"];
                if (y>uint32_t(h->GetNbinsY()+1))
                    continue;

                h->SetBinContent(x, y, v);

            }
            catch (const mysqlpp::BadFieldName &)
            {
                h->SetBinContent(x, v);
                try
                {
                    h->SetBinError(x, double(row["E"]));
                }
                catch (const mysqlpp::BadFieldName &)
                {
                }
            }
        }
        catch (const mysqlpp::BadConversion &b)
        {
            cerr << b.what() << endl;
        }
    }

    WriteHistogram(h, hist.dir);
    delete h;
#endif
}

void WriteHistogram(Database &connection, const Histogram &hist, const map<size_t, double> &data)
{
#ifdef HAVE_ROOT
    TH1 *h = CreateHistogram(hist);

    for (auto ir=data.cbegin(); ir!=data.cend(); ir++)
        h->SetBinContent(ir->first, ir->second);

    WriteHistogram(h, hist.dir);
    delete h;
#endif
}

// -------------------------------- Resources -------------------------------

#define RESOURCE(TYPE,RC) ([]() { \
    extern const char _binary_##RC##_start[]; \
    extern const char _binary_##RC##_end[];   \
    return TYPE(_binary_##RC##_start, _binary_##RC##_end); \
})()

string CreateResource(Configuration &conf, const string id, const string def, const string resource)
{
    string file = conf.Get<string>(id);

    if (file!=def)
    {
        file = conf.GetPrefixedString(id);
        cout << "Using " << file << "." << endl;
        return file;
    }

    file = conf.GetPrefixedString(id);

    cout << "Searching " << file << "... ";

    if (fs::exists(file))
    {
        cout << "found." << endl;
        return file;
    }

    cout << "creating!" << endl;

    ofstream(file) << resource;

    return file;
}

// ================================== MAIN ==================================


int main(int argc, const char* argv[])
{
    Time start;

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    // ----------------------------- Evaluate options --------------------------

    const string   uri        = conf.Get<bool>("dry-run") ? "" : conf.Get<string>("uri");
    const string   out        = conf.Get<string>("out");
    const uint16_t verbose    = conf.Get<uint16_t>("verbose");
    const double   confidence = conf.Get<double>("confidence-level");
    const bool     feldman    = conf.Get<bool>("feldman-cousins");

    const bool print_connection = conf.Get<bool>("print-connection");
    const bool print_queries    = conf.Get<bool>("print-queries");
    const bool mc_only          = conf.Get<bool>("mc-only");

    Binning binning_theta  = conf.Get<Binning>("theta")+conf.Vec<double>("theta-bin");
    Binning binning_dense  = conf.Get<Binning>("energy-dense");
    Binning binning_sparse = conf.Get<Binning>("energy-sparse");
    Binning binning_impact = conf.Get<Binning>("impact");

    cout << '\n';
    cout << "Binning 'theta':  " << binning_theta.str()  << endl;
    cout << "Binning 'dense':  " << binning_dense.str()  << endl;
    cout << "Binning 'sparse': " << binning_sparse.str() << endl;
    cout << "Binning 'impact': " << binning_impact.str() << endl;

    const uint16_t source_key = conf.Get<uint16_t>("source-key");
    const string   where      = boost::join(conf.Vec<string>("selector"), " AND\n      ");
    const string   estimator  = conf.Get<string>("estimator");
    const string   spectrum   = conf.Get<string>("spectrum");
    const auto     env        = conf.GetOptions<string>("env.");

    cout << "\n";
    const string analysis_sql    = CreateResource(conf, "analysis", "analysis.sql", RESOURCE(std::string, spectrum_analysis_sql));
    const string data_sql        = RESOURCE(std::string, spectrum_data_sql);
    const string simulation_sql  = RESOURCE(std::string, spectrum_simulation_sql);
    const string spectrum_sql    = RESOURCE(std::string, spectrum_spectrum_sql);
    const string summary_sim_sql = RESOURCE(std::string, spectrum_summary_sim_sql);
    const string summary_est_sql = RESOURCE(std::string, spectrum_summary_est_sql);
    cout << endl;

    const string str_theta  = binning_theta.list();
    const string str_dense  = binning_dense.list();
    const string str_sparse = binning_sparse.list();
    const string str_impact = binning_impact.list();

    // -------------------------------------------------------------------------
    // Checking for database connection

    Database connection(uri); // Keep alive while fetching rows

    if (!uri.empty())
    {
        if (verbose>0)
        {
            cout << "Connecting to database...\n";
            cout << "Client Version: " << mysqlpp::Connection().client_version() << endl;
        }

        try
        {
            connection.connected();
        }
        catch (const exception &e)
        {
            cerr << "SQL connection failed: " << e.what() << endl;
            return 1;
        }

        if (verbose>0)
        {
            cout << "Server Version: " << connection.server_version() << endl;
            cout << "Connected to " << connection.ipc_info() << endl;
        }

        if (print_connection)
        {
            try
            {
                const auto res1 = connection.query("SHOW STATUS LIKE 'Compression'").store();
                cout << "Compression of database connection is " << string(res1[0][1]) << endl;

                const auto res2 = connection.query("SHOW STATUS LIKE 'Ssl_cipher'").store();
                cout << "Connection to databases is " << (string(res2[0][1]).empty()?"UNENCRYPTED":"ENCRYPTED ("+string(res2[0][1])+")") << endl;
            }
            catch (const exception &e)
            {
                cerr << "\nSHOW STATUS LIKE 'Compression'\n\n";
                cerr << "SQL query failed:\n" << e.what() << endl;
                return 9;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Create log streams

    ofstream qlog(out+".query.sql");
    ofstream flog(connection.connected() ? out+".dump.sql" : "");
    ofstream mlog(connection.connected() ? out+".C" : "");

    cout << "\n";
    cout << "Queries    will be logged  to " << out << ".query.sql\n";
    if (connection.connected())
    {
        cout << "Tables     will be dumped  to " << out << ".dump.sql\n";
        cout << "ROOT macro will be written to " << out << ".C\n";
    }

#ifdef HAVE_ROOT
    TFile root(connection.connected() ? (out+".hist.root").c_str() : "", "RECREATE");
    if (connection.connected())
    {
        if (root.IsZombie())
            return 10;
        cout << "Histograms will be written to " << out << ".hist.root\n";
    }
    if (verbose>0)
        cout << "\nCalculating upper limits for a confidence interval of " << confidence << endl;
#endif

    cout << endl;

    // FIMXE: Implement SYNTAX check on spectrum, estimator and selector

    // -------------------------------------------------------------------
    // ---------------------------- Binnings -----------------------------
    // -------------------------------------------------------------------

    cout << separator("Binnings") << '\n';

    CreateBinning(connection, qlog, binning_theta,  "Theta",         "Binning in zenith angle");
    CreateBinning(connection, qlog, binning_dense,  "Energy_dense",  "Dense binning in log10 Energy");
    CreateBinning(connection, qlog, binning_sparse, "Energy_sparse", "Sparse binning in log10 Energy");
    CreateBinning(connection, qlog, binning_impact, "Impact",        "Binning in impact distance");

    Dump(flog, connection, "BinningTheta");
    Dump(flog, connection, "BinningEnergy_dense");
    Dump(flog, connection, "BinningEnergy_sparse");
    Dump(flog, connection, "BinningImpact");

    // -------------------------------------------------------------------
    // ---------------------------- DataFiles ----------------------------
    // -------------------------------------------------------------------

    cout << separator("DataFiles") << '\n';

    Time start1;

    /* 01:get-data-files.sql */
    mysqlpp::Query query1(&connection);
    query1 <<
        "CREATE TEMPORARY TABLE DataFiles\n"
        "(\n"
//        "   FileId INT UNSIGNED NOT NULL,\n"
        "   PRIMARY KEY (FileId) USING HASH\n"
        ") ENGINE=MEMORY\n"
        "AS\n"
        "(\n"
        "   SELECT\n"
        "      FileId\n"
        "   FROM\n"
        "      factdata.RunInfo\n"
        "   WHERE\n"
        "      fRunTypeKEY=1 AND fSourceKEY=%100:source AND\n"
        "      %101:where\n"
        "   ORDER BY\n"
        "      FileId\n"  // In order: faster
        ")";

    query1.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query1.template_defaults[it->first.c_str()] = it->second.c_str();

    query1.template_defaults["source"] = to_string(source_key).c_str();
    query1.template_defaults["where"]  = where.c_str();

    if (print_queries)
        PrintQuery(query1.str());

    qlog << query1 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query1.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "DataFiles");

        const auto sec1 = Time().UnixTime()-start1.UnixTime();
        cout << "Execution time: " << sec1 << "s\n" << endl;
    }

    // FIXME: Setup Zd binning depending on Data

    // -------------------------------------------------------------------
    // ------------------------- ObservationTime -------------------------
    // -------------------------------------------------------------------

    cout << separator("ObservationTime") << '\n';

    Time start2;

    // For some reason, the comments do not appear in the "EXPLAIN CREATE TABLE" query
    mysqlpp::Query query2(&connection);
    query2 <<
        "CREATE TEMPORARY TABLE ObservationTime\n"
        "(\n"
        //"   `.theta` INT COMMENT 'Zenith Angle bin index',\n"
        "   OnTime DOUBLE NOT NULL,\n"// COMMENT 'Effective on time in seconds per bin',\n"
        "   PRIMARY KEY (`.theta`) USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Effective on time of selected data files binning in zenith angle'\n"
        "AS\n"
        "(\n"
        "   SELECT\n"
        "      INTERVAL(fZenithDistanceMean, %100:bins) AS `.theta`,\n"
        "      SUM(TIME_TO_SEC(TIMEDIFF(fRunStop,fRunStart))*fEffectiveOn) AS OnTime\n"
        "   FROM\n"
        "      DataFiles\n"
        "   LEFT JOIN \n"
        "      factdata.RunInfo USING (FileId)\n"
        "   GROUP BY\n"
        "      `.theta`\n"
        "   ORDER BY\n"
        "      `.theta`\n"
        ")";

    query2.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query2.template_defaults[it->first.c_str()] = it->second.c_str();

    query2.template_defaults["bins"] = str_theta.c_str();

    if (print_queries)
        PrintQuery(query2.str());

    qlog << query2 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query2.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "ObservationTime");

        const auto sec2 = Time().UnixTime()-start2.UnixTime();
        cout << "Execution time: " << sec2 << "s\n\n";
    }

    // -------------------------------------------------------------------
    // -------------------------- MonteCarloFiles ------------------------
    // -------------------------------------------------------------------

    cout << separator("MonteCarloFiles") << '\n';

    Time start3;

    mysqlpp::Query query3(&connection);
    query3 <<
        "CREATE TEMPORARY TABLE MonteCarloFiles\n"
        "(\n"
//        "   FileId INT UNSIGNED NOT NULL,\n"
        "   PRIMARY KEY (FileId) USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Monte Carlo files selected by data Zenith Angle range'\n"
        "AS\n"
        "(\n"
        "   SELECT\n"
        "      FileId\n"
        "   FROM\n"
        "      ObservationTime\n"
        "   LEFT JOIN\n"
        "      BinningTheta ON `.theta`=bin\n"
        "   LEFT JOIN\n"
        "      factmc.RunInfoMC\n"
        "   ON\n"
        "      (ThetaMin>=lo AND ThetaMin<hi) OR (ThetaMax>lo AND ThetaMax<=hi)\n"
        "   WHERE\n"
        "      PartId=1 AND\n"
        "      FileId%%2=0\n"
        "   ORDER BY\n"
        "      FileId\n" // In order: faster
        ")";

    query3.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query3.template_defaults[it->first.c_str()] = it->second.c_str();

    if (print_queries)
        PrintQuery(query3.str());

    qlog << query3 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query3.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "MonteCarloFiles");

        const auto sec3 = Time().UnixTime()-start3.UnixTime();
        cout << "Execution time: " << sec3 << "s\n\n";
    }

    // -------------------------------------------------------------------
    // ------------------------- Monte Carlo Area ------------------------
    // -------------------------------------------------------------------

    cout << separator("MonteCarloArea") << '\n';

    Time start4;

    mysqlpp::Query query4(&connection);
    query4 <<
        "CREATE TEMPORARY TABLE MonteCarloArea ENGINE=MEMORY COMMENT='Minimum and maximum impact radius of selected Monte Carlo files'"
        "AS\n"
        "(\n"
        "   SELECT\n"
        "      MIN(`CSCAT[1]`) AS MinImpactLo,\n"
        "      MAX(`CSCAT[1]`) AS MaxImpactLo,\n"
        "      MIN(`CSCAT[2]`) AS MinImpactHi,\n"
        "      MAX(`CSCAT[2]`) AS MaxImpactHi\n"
        "   FROM\n"
        "      MonteCarloFiles\n"
        "   LEFT JOIN\n"
        "      factmc.CorsikaSetup ON FileId=RUNNR\n"
        // "   GROUP BY\n"
        // "      `CSCAT[1]`, `CSCAT[2]`\n"
        "   ORDER BY\n"
        "      MaxImpactHi\n"
        ")";

    query4.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query4.template_defaults[it->first.c_str()] = it->second.c_str();

    if (print_queries)
        PrintQuery(query4.str());

    qlog << query4 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query4.execute().info() << endl;
        ShowWarnings(connection);
        if (Dump(flog, connection, "MonteCarloArea")!=1)
        {
            cerr << "Impact range inconsistent!" << endl;
            return 1;
        }

        const auto sec4 = Time().UnixTime()-start4.UnixTime();
        cout << "Execution time: " << sec4 << "s\n\n";
    }

    // -------------------------------------------------------------------
    // ----------------------------- SummaryMC ---------------------------
    // -------------------------------------------------------------------

    cout << separator("SummaryOriginalMC") << '\n';

    Time start5;

    // This table combines the analysis results vs. Binning in Estimated Energy and Simulated Energy
    mysqlpp::Query query5(&connection);
    query5 <<
        "CREATE TEMPORARY TABLE Summary%100:table\n"
        "(\n"
//        "   `.theta`      SMALLINT UNSIGNED NOT NULL COMMENT 'Zenith Angle bin index',\n"
//        "   `.sparse_sim` SMALLINT UNSIGNED NOT NULL COMMENT 'Energy bin index (sparse binning)',\n"
//        "   `.dense_sim`  SMALLINT UNSIGNED NOT NULL COMMENT 'Energy bin index (dense binning)',\n"
        "   CountN        INT UNSIGNED      NOT NULL COMMENT 'Event count per bin',\n"
        "   SumW          DOUBLE            NOT NULL,\n"// COMMENT 'Sum of spectral weights',\n"
        "   SumW2         DOUBLE            NOT NULL,\n"// COMMENT 'Sum of squared spectral weights',\n"
        "   INDEX (`.theta`)      USING HASH,\n"
        "   INDEX (`.sparse_sim`) USING HASH,\n"
        "   INDEX (`.dense_sim`)  USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Event counts and sums of (squared) spectral weights for selected Monte Carlo data binned in log10 energy'\n"
        "AS\n"
        "(\n"
        "   WITH BinnedData AS\n"
        "   (\n"
        "      SELECT\n"
        "         INTERVAL(%101:column, %102:theta) AS `.theta`,\n"
        "         INTERVAL(LOG10(Energy), %103:sparse) AS `.sparse_sim`,\n"
        "         INTERVAL(LOG10(Energy), %104:dense)  AS `.dense_sim`,\n"
        "         (%105:spectrum)/pow(Energy, SpectralIndex) AS SpectralWeight\n"
        "      FROM\n"
        "         MonteCarloFiles\n"
        "      LEFT JOIN\n"
        "         factmc.RunInfoMC USING (FileId)\n"
        "      LEFT JOIN\n"
        "         factmc.%100:table USING (FileId)\n"
        "   )\n"
        "   SELECT\n"
        "      `.theta`,\n"
        "      `.sparse_sim`,\n"
        "      `.dense_sim`,\n"
        "      COUNT(*)                    AS  CountN,\n"
        "      SUM(    SpectralWeight   )  AS  SumW,\n"
        "      SUM(POW(SpectralWeight,2))  AS  SumW2\n"
        "   FROM\n"
        "      BinnedData\n"
        "   GROUP BY\n"
        "      `.theta`, `.sparse_sim`, `.dense_sim`\n"
        ")";

    query5.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query5.template_defaults[it->first.c_str()] = it->second.c_str();

    query5.template_defaults["table"]    = "OriginalMC";
    query5.template_defaults["column"]   = "DEGREES(Theta)";
    query5.template_defaults["sparse"]   = str_sparse.c_str();
    query5.template_defaults["dense"]    = str_dense.c_str();
    query5.template_defaults["theta"]    = str_theta.c_str();
    query5.template_defaults["spectrum"] = spectrum.c_str();

    if (print_queries)
        PrintQuery(query5.str());

    qlog << query5 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query5.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "SummaryOriginalMC");

        const auto sec5 = Time().UnixTime()-start5.UnixTime();
        cout << "Execution time: " << sec5 << "s\n\n";
    }

    // -------------------------------------------------------------------

    cout << separator("SummaryEventsMC") << '\n';

    Time start5b;

    query5.template_defaults["table"]  = "EventsMC";
    query5.template_defaults["column"] = "DEGREES(Theta)";

    if (print_queries)
        PrintQuery(query5.str());

    qlog << query5 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query5.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "SummaryEventsMC");

        const auto sec5b = Time().UnixTime()-start5b.UnixTime();
        cout << "Execution time: " << sec5b << "s\n\n";
    }

    // -------------------------------------------------------------------
    // ---------------------------- ThetDist -----------------------------
    // -------------------------------------------------------------------

    Time start6;

    // This table combines the analysis results vs. Binning in Estimated Energy and Simulated Energy
    mysqlpp::Query query6(&connection);
    query6 <<
        "CREATE TEMPORARY TABLE ThetaDist\n"
        "(\n"
        "   CountN      INT UNSIGNED NOT NULL,\n"
        "   ErrCountN   DOUBLE       NOT NULL,\n"
        "   ZdWeight    DOUBLE       NOT NULL,\n"
        "   ErrZdWeight DOUBLE       NOT NULL,\n"
        "   INDEX (`.theta`) USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Event counts and sums of (squared) spectral weights for selected Monte Carlo data binned in theta'\n"
        "AS\n"
        "(\n"
        "   WITH ThetaCount AS\n"
        "   (\n"
        "      SELECT\n"
        "         `.theta`,\n"
        "         SUM(CountN) AS CountN\n"
        "      FROM\n"
        "         SummaryOriginalMC\n"
        "      GROUP BY\n"
        "         `.theta`\n"
        "   )\n"
        "   SELECT\n"
        "      `.theta`,\n"
        "      CountN,\n"
        "      SQRT(CountN) AS ErrCountN,\n"
        "      OnTime,\n"
        "      OnTime/CountN AS ZdWeight,\n"
        "      (OnTime/CountN)*SQRT(POW(1/300, 2) + 1/CountN) AS ErrZdWeight\n"
        "   FROM\n"
        "      ObservationTime\n"
        "   LEFT JOIN\n"
        "      ThetaCount USING (`.theta`)\n"
        "   LEFT JOIN\n"
        "      BinningTheta ON `.theta`=bin\n"
        "   ORDER BY\n"
        "      `.theta`\n"
        ")";

    query6.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query6.template_defaults[it->first.c_str()] = it->second.c_str();

    if (print_queries)
        PrintQuery(query6.str());

    qlog << query6 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query6.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "ThetaDist");

        const auto sec6 = Time().UnixTime()-start6.UnixTime();
        cout << "Execution time: " << sec6 << "s\n\n";
    }

    WriteHistogram(connection, {
             .name     = "OnTime",
             .title    = "Effective on time",
             .dir      = "Zd",
             .binningx = binning_theta,
             .binningy = {},
             .table    = "ThetaDist",
             .x        = ".theta",
             .y        = "",
             .v        = "OnTime",
             .err      = "",
             .axisx    = "Zenith Distance #theta [#circ]",
             .axisy    = "Eff. on time [s]",
             .axisz    = "",
             .stats    = true
         });

    WriteHistogram(connection, {
             .name     = "CountN",
             .title    = "Simulated Zenith Distance",
             .dir      = "Zd",
             .binningx = binning_theta,
             .binningy = {},
             .table    = "ThetaDist",
             .x        = ".theta",
             .y        = "",
             .v        = "CountN",
             .err      = "ErrCountN",
             .axisx    = "Zenith Distance #theta [#circ]",
             .axisy    = "Counts",
             .axisz    = "",
             .stats    = true
         });

    WriteHistogram(connection, {
             .name     = "ZdWeight",
             .title    = "Zenith Distance Weight",
             .dir      = "Zd",
             .binningx = binning_theta,
             .binningy = {},
             .table    = "ThetaDist",
             .x        = ".theta",
             .y        = "",
             .v        = "ZdWeight",
             .err      = "ErrZdWeight",
             .axisx    = "Zenith Distance #theta [#circ]",
             .axisy    = "Weight [s]",
             .axisz    = "",
             .stats    = true
         });

    // -------------------------------------------------------------------
    // --------------------------- WeightedMC ----------------------------
    // -------------------------------------------------------------------

    Time start7;

    // This table combines the analysis results vs. Binning in Estimated Energy and Simulated Energy
    mysqlpp::Query query7(&connection);
    query7 <<
        "CREATE TEMPORARY TABLE Weighted%100:table\n"
        "(\n"
        "   SumW2 DOUBLE NOT NULL,\n"
        "   INDEX (`.theta`)      USING HASH,\n"
        "   INDEX (`.sparse_sim`) USING HASH,\n"
        "   INDEX (`.dense_sim`)  USING HASH\n"
        ")\n"
        "ENGINE=MEMORY COMMENT='Table Summary%100:table but with theta-weights applied'\n"
        "AS\n"
        "(\n"
        "   SELECT\n"
        "      `.theta`,\n"
        "      `.sparse_sim`,\n"
        "      `.dense_sim`,\n"
        "      S.CountN,\n"
        "      S.SumW*ZdWeight AS SumW,\n"
        "      S.SumW2*POW(ErrZdWeight, 2) AS SumW2\n"
        "   FROM\n"
        "      Summary%100:table S\n"
        "   INNER JOIN\n"
        "      ThetaDist USING (`.theta`)\n"
        ")";

    query7.parse();
    //for (auto it=env.cbegin(); it!=env.cend(); it++)
    //    query7.template_defaults[it->first.c_str()] = it->second.c_str();

    query7.template_defaults["table"] = "OriginalMC";

    if (print_queries)
        PrintQuery(query7.str());

    qlog << query7 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query7.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "WeightedOriginalMC");

        const auto sec7 = Time().UnixTime()-start7.UnixTime();
        cout << "Execution time: " << sec7 << "s\n\n";
    }

    // -------------------------------------------------------------------

    Time start7b;

    query7.template_defaults["table"] = "EventsMC";

    if (print_queries)
        PrintQuery(query7.str());

    qlog << query7 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query7.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "WeightedEventsMC");

        const auto sec7b = Time().UnixTime()-start7b.UnixTime();
        cout << "Execution time: " << sec7b << "s\n\n";
    }

    // -------------------------------------------------------------------
    // --------------------------- AnalysisMC ----------------------------
    // -------------------------------------------------------------------

    cout << separator("AnalysisMC") << '\n';

    Time start8;

    // This table combines the analysis results vs. Binning in Estimated Energy and Simulated Energy
    mysqlpp::Query query8(&connection);
    sindent indent8(query8);
    query8 <<
/*
        "CREATE TEMPORARY TABLE AnalysisMC\n"
        "(\n"
        "   `.sparse`     SMALLINT UNSIGNED NOT NULL,\n"
        "   `.dense`      SMALLINT UNSIGNED NOT NULL,\n"
        "   SignalW       DOUBLE            NOT NULL,\n" // vs Eest (for spectral analysis)
        "   SignalN       DOUBLE            NOT NULL,\n" // vs Eest
        "   BackgroundN   DOUBLE            NOT NULL,\n" // vs Eest
        "   BackgroundW   DOUBLE            NOT NULL,\n" // vs Eest
        "   ExcessW       DOUBLE            NOT NULL,\n" // vs Eest
        "   ExcessN       DOUBLE            NOT NULL,\n" // vs Eest
        "   ErrExcessW    DOUBLE            NOT NULL,\n" // vs Eest
        "   ErrExcessN    DOUBLE            NOT NULL,\n" // vs Eest
        "   ThresholdW    DOUBLE            NOT NULL,\n" // vs Esim (for simulation analysis)
        "   ThresholdN    DOUBLE            NOT NULL,\n" // vs Esim
        "   BiasEst       DOUBLE            NOT NULL,\n" // vs Eest
        "   BiasSim       DOUBLE            NOT NULL,\n" // vs Esim
        "   ResolutionEst DOUBLE,\n"
        "   ResolutionSim DOUBLE,\n"
        "   INDEX (`.sparse`) USING HASH,\n"
        "   INDEX (`.dense`)  USING HASH\n"
        ") ENGINE=Memory\n"
*/
        "CREATE TEMPORARY TABLE AnalysisMC\n"
        "(\n"
        "   SignalN       INT UNSIGNED NOT NULL,\n"
        "   SignalW       DOUBLE       NOT NULL,\n"
        "   SignalW2      DOUBLE       NOT NULL,\n"
        "   BackgroundN   INT UNSIGNED NOT NULL,\n"
        "   BackgroundW   DOUBLE       NOT NULL,\n"
        "   BackgroundW2  DOUBLE       NOT NULL,\n"
        "   ResidualW     DOUBLE       NOT NULL,\n"
        "   ResidualW2    DOUBLE       NOT NULL,\n"
        "   SumEnergySimW DOUBLE       NOT NULL,\n"
        "   SumEnergyEstW DOUBLE       NOT NULL,\n"
        "   INDEX (`.theta`)                     USING HASH,\n"
        "   INDEX (`.sparse_est`)                USING HASH,\n"
        "   INDEX (`.sparse_sim`)                USING HASH,\n"
        "   INDEX (`.dense_est`)                 USING HASH,\n"
        "   INDEX (`.dense_sim`)                 USING HASH,\n"
        "   INDEX (`.sparse_est`, `.sparse_sim`) USING HASH,\n"
        "   INDEX (`.dense_est`, `.dense_sim`)   USING HASH,\n"
        "   INDEX (`.impact`)                    USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Sum of counts and (squared) weightes of Monte Carlo Data after analysis'\n"
        "AS\n"
        "(\n"
        "   WITH Excess AS\n"
        "   (\n"                            << indent(6)
        << ifstream(analysis_sql).rdbuf()   << indent(0) <<
        "   ),\n"                           <<
        "   Result AS\n"
        "   (\n"                            << indent(6)
        << simulation_sql << indent(0) << // Must end with EOL and not in the middle of a comment
        "   )\n"
        "   SELECT * FROM Result\n"
        ")";

    query8.parse();
    for (auto it=env.cbegin(); it!=env.cend(); it++)
        query8.template_defaults[it->first.c_str()] = it->second.c_str();

    //query6.template_defaults["columns"]   = "FileId, EvtNumber, CorsikaNumReuse,";
    query8.template_defaults["columns"]   = "Energy, SpectralIndex, Impact,";
    query8.template_defaults["zenith"]    = "DEGREES(Theta)";
    query8.template_defaults["files"]     = "MonteCarloFiles";
    query8.template_defaults["runinfo"]   = "factmc.RunInfoMC";
    query8.template_defaults["events"]    = "factmc.EventsMC";
    query8.template_defaults["positions"] = "factmc.PositionMC";

    query8.template_defaults["sparse"]    = str_sparse.c_str();
    query8.template_defaults["dense"]     = str_dense.c_str();
    query8.template_defaults["theta"]     = str_theta.c_str();
    query8.template_defaults["impact"]    = str_impact.c_str();
    query8.template_defaults["spectrum"]  = spectrum.c_str();
    query8.template_defaults["estimator"] = estimator.c_str();

    if (print_queries)
        PrintQuery(query8.str());

    qlog << query8 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query8.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "AnalysisMC");

        const auto sec8 = Time().UnixTime()-start8.UnixTime();
        cout << "Execution time: " << sec8 << "s\n\n";
    }


    // -------------------------------------------------------------------
    // ------------------------- SimulatedSpectrum -----------------------
    // -------------------------------------------------------------------

    const vector<string> binnings = { "dense", "sparse", "theta" };

    for (auto ib=binnings.cbegin(); ib!=binnings.cend(); ib++)
    {
        const string table = "Summary"+(*ib=="theta" ? "Theta" : "TrueEnergy_"+*ib);

        cout << separator(table) << '\n';

        // [lo^(1+gammaa) - hi^(1+gamma)] / (1+gamma)

        Time start9;

        /*
        "CREATE TEMPORARY TABLE SummarySimulated\n"
        "(\n"
        "   `.energy` SMALLINT UNSIGNED NOT NULL COMMENT 'Bin Index [MC Energy]',\n"
        "   CountN    DOUBLE            NOT NULL,\n"  // COMMENT 'Number of events',\n"
        "   CountW    DOUBLE            NOT NULL,\n"  // COMMENT 'Sum of weights, reweighted sum',\n"
        "   CountW2   DOUBLE            NOT NULL,\n"  // COMMENT 'Sum of square of weights'\n"
        "   PRIMARY KEY (`.energy`) USING HASH\n"
        ") ENGINE=Memory\n"
        */

        mysqlpp::Query query9(&connection);
        sindent indent9(query9);
        query9 <<
            "CREATE TEMPORARY TABLE %100:table\n"
            "(\n"
            "   SimCountN                  INT UNSIGNED NOT NULL,\n"
            "   TrigCountN                 INT UNSIGNED NOT NULL,\n"
            "   SignalN                    INT UNSIGNED NOT NULL,\n"
            "   BackgroundN                DOUBLE       NOT NULL,\n"
            //
            "   ErrSimCountN               DOUBLE       NOT NULL,\n"
            "   ErrTrigCountN              DOUBLE       NOT NULL,\n"
            "   ErrSignalN                 DOUBLE       NOT NULL,\n"
            "   ErrBackgroundN             DOUBLE       NOT NULL,\n"
            //
            "   SimSumW                    DOUBLE       NOT NULL,\n"
            "   TrigSumW                   DOUBLE       NOT NULL,\n"
            "   SignalW                    DOUBLE       NOT NULL,\n"
            "   BackgroundW                DOUBLE       NOT NULL,\n"
            "   ExcessW                    DOUBLE       NOT NULL,\n"
            //
            "   SimSumW2                   DOUBLE       NOT NULL,\n"
            "   TrigSumW2                  DOUBLE       NOT NULL,\n"
            "   SignalW2                   DOUBLE       NOT NULL,\n"
            "   BackgroundW2               DOUBLE       NOT NULL,\n"
            "   ExcessW2                   DOUBLE       NOT NULL,\n"
            //
            "   SimFluxW                   DOUBLE       NOT NULL,\n"
            "   TrigFluxW                  DOUBLE       NOT NULL,\n"
            "   SignalFluxW                DOUBLE       NOT NULL,\n"
            "   BackgroundFluxW            DOUBLE       NOT NULL,\n"
            "   ExcessFluxW                DOUBLE       NOT NULL,\n"
            //
            "   ErrSimFluxW                DOUBLE       NOT NULL,\n"
            "   ErrTrigFluxW               DOUBLE       NOT NULL,\n"
            "   ErrSignalFluxW             DOUBLE       NOT NULL,\n"
            "   ErrBackgroundFluxW         DOUBLE       NOT NULL,\n"
            "   ErrExcessFluxW             DOUBLE       NOT NULL,\n"
            //
            "   ResidualW                  DOUBLE       NOT NULL,\n"
            "   ResidualW2                 DOUBLE       NOT NULL,\n"
            "   BiasW                      DOUBLE       NOT NULL,\n"
            "   ErrBiasW                   DOUBLE       NOT NULL,\n"
            "   ResolutionW                DOUBLE,\n"
            //
            "   SumEnergyEstW              DOUBLE       NOT NULL,\n"
            "   SumEnergySimW              DOUBLE       NOT NULL,\n"
            //
            "   AvgEnergyEstW              DOUBLE       NOT NULL,\n"
            "   AvgEnergySimW              DOUBLE       NOT NULL,\n"
            //
            "   CutEfficiencyN             DOUBLE       NOT NULL,\n"
            "   CutEfficiencyW             DOUBLE       NOT NULL,\n"
            "   TriggerEfficiencyN         DOUBLE       NOT NULL,\n"
            "   TriggerEfficiencyW         DOUBLE       NOT NULL,\n"
            "   EffectiveAreaN             DOUBLE       NOT NULL,\n"
            "   EffectiveAreaW             DOUBLE       NOT NULL,\n"
            //
            "   ErrCutEfficiencyN          DOUBLE       NOT NULL,\n"
            "   ErrCutEfficiencyW          DOUBLE       NOT NULL,\n"
            "   ErrEffectiveAreaN          DOUBLE       NOT NULL,\n"
            "   ErrEffectiveAreaW          DOUBLE       NOT NULL,\n"
            "   ErrTriggerEfficiencyN      DOUBLE       NOT NULL,\n"
            "   ErrTriggerEfficiencyW      DOUBLE       NOT NULL,\n"
            //
            "   IntegralSimFluxW           DOUBLE       NOT NULL,\n"
            "   IntegralSimFluxW2          DOUBLE       NOT NULL,\n"
            "   IntegralSignalW            DOUBLE       NOT NULL,\n"
            "   IntegralSignalFluxW        DOUBLE       NOT NULL,\n"
            "   IntegralSignalFluxW2       DOUBLE       NOT NULL,\n"
            "   IntegralBackgroundFluxW    DOUBLE       NOT NULL,\n"
            "   IntegralBackgroundFluxW2   DOUBLE       NOT NULL,\n"
            "   IntegralExcessFluxW        DOUBLE       NOT NULL,\n"
            //
            "   ErrIntegralExcessFluxW     DOUBLE       NOT NULL,\n"
            "   ErrIntegralSignalFluxW     DOUBLE       NOT NULL,\n"
            "   ErrIntegralBackgroundFluxW DOUBLE       NOT NULL,\n"
            "   ErrIntegralSimFluxW        DOUBLE       NOT NULL,\n"
            //
            "   IntegralEnergySimW         DOUBLE       NOT NULL,\n"
            "   IntegralEnergyEstW         DOUBLE       NOT NULL,\n"
            //
            "   AvgIntegralEnergyEstW      DOUBLE       NOT NULL,\n"
            "   AvgIntegralEnergySimW      DOUBLE       NOT NULL,\n"
            //
            "   ObsTime                    DOUBLE       NOT NULL,\n"
            "   Area                       DOUBLE       NOT NULL,\n"
            "   AreaTime                   DOUBLE       NOT NULL,\n"
            "   Width                      DOUBLE       NOT NULL,\n"
            //
            "   INDEX (%102:bin) USING HASH\n"
            ") ENGINE=MEMORY COMMENT='Summary of all Monte Carlo quantities, binned in true energy or zenith angle'\n"
            "AS\n"
            "(\n"
            << indent(3) << summary_sim_sql << indent(0) <<
            ")";

        // [ Sa^2/a^2 + Sb^2/b^2 ] * a^2/b^2
        // [ (sc^2)/c^2+(sb^2)/b^2+(sa^2)/a^2 ]  * a^2*b^2/c^2

        // [ a/b - c^2/d^2] --> (4*Sc^2*c^2)/d^4 + (4*Sd^2*c^4)/d^6 + Sa^2/b^2 + (Sb^2*a^2)/b^4

        query9.parse();
        //for (auto it=env.cbegin(); it!=env.cend(); it++)
        //    query9.template_defaults[it->first.c_str()] = it->second.c_str();

        query9.template_defaults["table"]    = table.c_str();
        query9.template_defaults["binning"]  = *ib=="theta" ? "BinningTheta" : ("BinningEnergy_"+*ib).c_str();
        query9.template_defaults["bin"]      = *ib=="theta" ? "`.theta`"     : ("`."+*ib+"_sim`").c_str();
        query9.template_defaults["binwidth"] = *ib=="theta" ? "1"            : "(POW(10,hi)-POW(10,lo))/1000";

        if (print_queries)
            PrintQuery(query9.str());

        qlog << query9 << ";\n" << endl;
        if (connection.connected())
        {
            cout << query9.execute().info() << endl;
            ShowWarnings(connection);
            Dump(flog, connection, table);

            const auto sec9 = Time().UnixTime()-start9.UnixTime();
            cout << "Execution time: " << sec9 << "s\n";
        }

        Histogram hist_sim;
        hist_sim.table = table;
        hist_sim.dir   = *ib=="theta" ? "MC/theta" : "MC/"+*ib+"/TrueEnergy";
        hist_sim.x     = *ib=="theta" ? ".theta" : "."+*ib+"_sim";
        hist_sim.axisx = *ib=="theta" ? "Zenith Angle #theta [#circ]" : "Energy lg(E_{sim}/GeV)";
        hist_sim.stats = false;

        if (*ib=="theta")
            hist_sim.binningx=binning_theta;
        if (*ib=="dense")
            hist_sim.binningx=binning_dense;
        if (*ib=="sparse")
            hist_sim.binningx=binning_sparse;

        hist_sim.axisy = "Counts";

        hist_sim.title = "";
        hist_sim.v     = "SimCountN";
        hist_sim.err   = "ErrSimCountN";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "TrigCountN";
        hist_sim.err   = "ErrTrigCountN";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "SignalN";
        hist_sim.err   = "ErrSignalN";
        WriteHistogram(connection, hist_sim);


        hist_sim.axisy = *ib=="theta"?"dN/dE [cm^{-2} s^{-1}]":"dN/dE [cm^{-2} s^{-1} TeV^{-1}]";

        hist_sim.title = "";
        hist_sim.v     = "SimFluxW";
        hist_sim.err   = "ErrSimFluxW";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "TrigFluxW";
        hist_sim.err   = "ErrTrigFluxW";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "ExcessFluxW";
        hist_sim.err   = "ErrExcessFluxW";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "SignalFluxW";
        hist_sim.err   = "ErrSignalFluxW";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "BackgroundFluxW";
        hist_sim.err   = "ErrBackgroundFluxW";
        WriteHistogram(connection, hist_sim);


        hist_sim.title = "";
        hist_sim.v     = "AvgEnergySimW";
        hist_sim.err   = "";
        hist_sim.axisy = "<E_{sim}>/GeV";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "AvgEnergyEstW";
        hist_sim.err   = "";
        hist_sim.axisy = "<E_{est}>/GeV";
        WriteHistogram(connection, hist_sim);


        hist_sim.title = "";
        hist_sim.v     = "EffectiveAreaN";
        hist_sim.err   = "ErrEffectiveAreaN";
        hist_sim.axisy = "A_{eff} [cm^{2}]";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "EffectiveAreaW";
        hist_sim.err   = "ErrEffectiveAreaW";
        hist_sim.axisy = "A_{eff} [cm^{2}]";
        WriteHistogram(connection, hist_sim);


        hist_sim.title = "";
        hist_sim.v     = "BiasW";
        hist_sim.err   = "ErrBiasW";
        hist_sim.axisy = "<lg(E_{est}/E_{sim})>";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "ResolutionW";
        hist_sim.err   = "";
        hist_sim.axisy = "#sigma(lg(E_{est}/E_{sim}))";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "TriggerEfficiencyN";
        hist_sim.err   = "ErrTriggerEfficiencyN";
        hist_sim.axisy = "Efficiency";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "TriggerEfficiencyW";
        hist_sim.err   = "ErrTriggerEfficiencyW";
        hist_sim.axisy = "Efficiency";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "CutEfficiencyN";
        hist_sim.err   = "ErrCutEfficiencyN";
        hist_sim.axisy = "Efficiency";
        WriteHistogram(connection, hist_sim);

        hist_sim.title = "";
        hist_sim.v     = "CutEfficiencyW";
        hist_sim.err   = "ErrCutEfficiencyW";
        hist_sim.axisy = "Efficiency";
        WriteHistogram(connection, hist_sim);

        // -------------------------------------------------------------------
        // ------------------------ ImpactDistribution -----------------------
        // -------------------------------------------------------------------

        cout << separator("ImpactDistribution_"+*ib) << '\n';

        Time start13;

        mysqlpp::Query query13(&connection);
        query13 <<
            "CREATE TEMPORARY TABLE ImpactDistribution_%100:binning\n"
            "(\n"
            "   SignalN INT UNSIGNED NOT NULL\n"
            ") ENGINE=MEMORY COMMENT='Impact Distribution of unweighted signal events after cuts'\n"
            "AS\n"
            "(\n"
            "   SELECT\n"
            "      %101:bin,\n"
            "      `.impact`,\n"
            "      SUM(SignalN) AS SignalN\n"
            "   FROM\n"
            "      AnalysisMC\n"
            "   GROUP BY\n"
            "      %101:bin, `.impact`\n"
            "   ORDER BY\n"
            "      %101:bin, `.impact`\n"
            ")";

        query13.parse();

        query13.template_defaults["binning"] = ib->c_str();
        query13.template_defaults["bin"]     = *ib=="theta" ? "`.theta`" : ("`."+*ib+"_sim`").c_str();

        if (print_queries)
            PrintQuery(query13.str());

        qlog << query13 << ";\n" << endl;
        if (connection.connected())
        {
            cout << query13.execute().info() << endl;
            ShowWarnings(connection);
            Dump(flog, connection, "ImpactDistribution_"+*ib);

            const auto sec13 = Time().UnixTime()-start13.UnixTime();
            cout << "Execution time: " << sec13 << "s\n";
        }

        hist_sim.name     = "Impact";
        hist_sim.title    = "Distribution of Impact Distance";
        hist_sim.y        = ".impact";
        hist_sim.v        = "SignalN";
        hist_sim.err      = "";
        hist_sim.table    = "ImpactDistribution_"+*ib;
        hist_sim.binningy = binning_impact;
        hist_sim.axisy    = "Impact Distance [m]";
        hist_sim.axisz    = "Counts";

        WriteHistogram(connection, hist_sim);

        // ===================================================================
        // ===================================================================

        if (*ib=="theta")
            continue;

        // ===================================================================
        // ===================================================================


        // -------------------------------------------------------------------
        // ------------------------- SimulatedSpectrum -----------------------
        // -------------------------------------------------------------------

        cout << separator("SummaryEstimatedEnergy_"+*ib) << '\n';

        // [lo^(1+gammaa) - hi^(1+gamma)] / (1+gamma)

        Time start10;

        mysqlpp::Query query10(&connection);
        sindent indent10(query10);
        query10 <<
            "CREATE TEMPORARY TABLE SummaryEstimatedEnergy_%100:binning\n"
            "(\n"
            "   SignalN                    INT UNSIGNED NOT NULL,\n"
            "   BackgroundN                DOUBLE       NOT NULL,\n"
            "   ExcessN                    DOUBLE       NOT NULL,\n"
            //
            "   ErrSignalN                 DOUBLE       NOT NULL,\n"
            "   ErrBackgroundN             DOUBLE       NOT NULL,\n"
            "   ErrExcessN                 DOUBLE       NOT NULL,\n"
            //
            "   SignalW                    DOUBLE       NOT NULL,\n"
            "   BackgroundW                DOUBLE       NOT NULL,\n"
            "   ExcessW                    DOUBLE       NOT NULL,\n"
            //
            "   SignalW2                   DOUBLE       NOT NULL,\n"
            "   BackgroundW2               DOUBLE       NOT NULL,\n"
            //
            "   ErrSignalW                 DOUBLE       NOT NULL,\n"
            "   ErrBackgroundW             DOUBLE       NOT NULL,\n"
            "   ErrExcessW                 DOUBLE       NOT NULL,\n"
            //
            "   SignalFluxW                DOUBLE       NOT NULL,\n"
            "   BackgroundFluxW            DOUBLE       NOT NULL,\n"
            "   ExcessFluxW                DOUBLE       NOT NULL,\n"
            //
            "   ErrSignalFluxW             DOUBLE       NOT NULL,\n"
            "   ErrBackgroundFluxW         DOUBLE       NOT NULL,\n"
            "   ErrExcessFluxW             DOUBLE       NOT NULL,\n"
            //
            "   ResidualW                  DOUBLE       NOT NULL,\n"
            "   ResidualW2                 DOUBLE       NOT NULL,\n"
            "   BiasW                      DOUBLE       NOT NULL,\n"
            "   ErrBiasW                   DOUBLE       NOT NULL,\n"
            "   ResolutionW                DOUBLE,\n"
            //
            "   SumEnergyEstW              DOUBLE       NOT NULL,\n"
            "   SumEnergySimW              DOUBLE       NOT NULL,\n"
            //
            "   AvgEnergyEstW              DOUBLE       NOT NULL,\n"
            "   AvgEnergySimW              DOUBLE       NOT NULL,\n"
            //
            "   IntegralSignalW            DOUBLE       NOT NULL,\n"
            "   IntegralSignalFluxW        DOUBLE       NOT NULL,\n"
            "   IntegralSignalFluxW2       DOUBLE       NOT NULL,\n"
            "   IntegralBackgroundFluxW    DOUBLE       NOT NULL,\n"
            "   IntegralBackgroundFluxW2   DOUBLE       NOT NULL,\n"
            "   IntegralExcessFluxW        DOUBLE       NOT NULL,\n"
            //
            "   ErrIntegralExcessFluxW     DOUBLE       NOT NULL,\n"
            "   ErrIntegralSignalFluxW     DOUBLE       NOT NULL,\n"
            "   ErrIntegralBackgroundFluxW DOUBLE       NOT NULL,\n"
            //
            "   IntegralEnergySimW         DOUBLE       NOT NULL,\n"
            "   IntegralEnergyEstW         DOUBLE       NOT NULL,\n"
            //
            "   AvgIntegralEnergyEstW      DOUBLE       NOT NULL,\n"
            "   AvgIntegralEnergySimW      DOUBLE       NOT NULL,\n"
            //
            "   ObsTime                    DOUBLE       NOT NULL,\n"
            "   Area                       DOUBLE       NOT NULL,\n"
            "   AreaTime                   DOUBLE       NOT NULL,\n"
            "   Width                      DOUBLE       NOT NULL,\n"
            //
            "   INDEX (`.%100:binning:_est`) USING HASH\n"
            ") ENGINE=MEMORY COMMENT='Summary of all Monte Carlo quantities binned in estimated energy'\n"
            "AS\n"
            "(\n"
            << indent(3) << summary_est_sql << indent(6) <<
            ")";

        query10.parse();
        //for (auto it=env.cbegin(); it!=env.cend(); it++)
        //    query10.template_defaults[it->first.c_str()] = it->second.c_str();

        query10.template_defaults["binning"] = ib->c_str();

        if (print_queries)
            PrintQuery(query10.str());

        qlog << query10 << ";\n" << endl;
        if (connection.connected())
        {
            cout << query10.execute().info() << endl;
            ShowWarnings(connection);
            Dump(flog, connection, "SummaryEstimatedEnergy_"+*ib);

            const auto sec10 = Time().UnixTime()-start10.UnixTime();
            cout << "Execution time: " << sec10 << "s\n";
        }

        Histogram hist_est;
        hist_est.dir      = "MC/"+*ib+"/EstimatedEnergy";
        hist_est.binningx = *ib=="dense"?binning_dense:binning_sparse;
        hist_est.table    = "SummaryEstimatedEnergy_"+*ib;
        hist_est.x        = "."+*ib+"_est";
        hist_est.axisx    = "Energy lg(E_{est}/GeV)";
        hist_est.stats    = false;

        hist_est.axisy   = "Counts";

        hist_est.title   = "";
        hist_est.v       = "SignalN";
        hist_est.err     = "ErrSignalN";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "BackgroundN";
        hist_est.err     = "ErrBackgroundN";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "ExcessN";
        hist_est.err     = "ErrExcessN";
        WriteHistogram(connection, hist_est);


        hist_est.title   = "";
        hist_est.v       = "AvgEnergySimW";
        hist_est.err     = "";
        hist_est.axisy   = "<E_{sim}>/GeV";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "AvgEnergyEstW";
        hist_est.err     = "";
        hist_est.axisy   = "<E_{est}>/GeV";
        WriteHistogram(connection, hist_est);


        hist_est.axisy   = "dN/dE [cm^{-2} s^{-1} TeV^{-1}]";

        hist_est.title   = "";
        hist_est.v       = "SignalFluxW";
        hist_est.err     = "ErrSignalFluxW";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "BackgroundFluxW";
        hist_est.err     = "ErrBackgroundFluxW";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "ExcessFluxW";
        hist_est.err     = "ErrExcessFluxW";
        WriteHistogram(connection, hist_est);


        hist_est.title   = "";
        hist_est.v       = "BiasW";
        hist_est.err     = "ErrBiasW";
        hist_est.axisy   = "<lg(E_{est}/E_{sim})>";
        WriteHistogram(connection, hist_est);

        hist_est.title   = "";
        hist_est.v       = "ResolutionW";
        hist_est.err     = "";
        hist_est.axisy   = "#sigma(lg(E_{est}/E_{sim}))";
        WriteHistogram(connection, hist_est);


        // -------------------------------------------------------------------
        // -------------------------- MigrationMatrix ------------------------
        // -------------------------------------------------------------------

        cout << separator("EnergyMigration_"+*ib) << '\n';

        Time start11;

        mysqlpp::Query query11(&connection);
        query11 <<
            "CREATE TEMPORARY TABLE EnergyMigration_%100:binning\n"
            "(\n"
            "   SignalN INT UNSIGNED NOT NULL\n"
            ") ENGINE=MEMORY COMMENT='Energy Migration: Monte Carlo Event counts binned in true and estimated energy'\n"
            "AS\n"
            "(\n"
            "   SELECT\n"
            "      `.%100:binning:_est`,\n"
            "      `.%100:binning:_sim`,\n"
            "      SUM(SignalN)  AS SignalN\n"
            "   FROM\n"
            "      AnalysisMC\n"
            "   GROUP BY\n"
            "      `.%100:binning:_est`, `.%100:binning:_sim`\n"
            "   ORDER BY\n"
            "      `.%100:binning:_est`, `.%100:binning:_sim`\n"
        ")";

        query11.parse();
        //for (auto it=env.cbegin(); it!=env.cend(); it++)
        //    query11.template_defaults[it->first.c_str()] = it->second.c_str();

        query11.template_defaults["binning"] = ib->c_str();

        if (print_queries)
            PrintQuery(query11.str());

        qlog << query11 << ";\n" << endl;
        if (connection.connected())
        {
            cout << query11.execute().info() << endl;
            ShowWarnings(connection);
            Dump(flog, connection, "EnergyMigration_"+*ib);

            const auto sec11 = Time().UnixTime()-start11.UnixTime();
            cout << "Execution time: " << sec11 << "s\n";
        }

        WriteHistogram(connection, {
             .name     = "Migration",
             .title    = "",
             .dir      = "MC/"+*ib,
             .binningx = *ib=="dense"?binning_dense:binning_sparse,
             .binningy = *ib=="dense"?binning_dense:binning_sparse,
             .table    = "EnergyMigration_"+*ib,
             .x        = "."+*ib+"_sim",
             .y        = "."+*ib+"_est",
             .v        = "SignalN",
             .err      = "",
             .axisx    = "Energy lg(E_{sim}/GeV)",
             .axisy    = "Energy lg(E_{est}/GeV)",
             .axisz    = "Counts",
             .stats    = false
         });
    }

    if (mc_only)
    {
        cout << separator("Summary") << '\n';
        const auto sec = Time().UnixTime()-start.UnixTime();
        cout << "Total execution time: " << sec << "s\n" << endl;

        return 0;
    }

    // -------------------------------------------------------------------
    // --------------------------- AnalysisData --------------------------
    // -------------------------------------------------------------------

    cout << separator("AnalysisData") << '\n';

    Time start12;

    mysqlpp::Query query12(&connection);
    sindent indent12(query12);
    query12 <<
        "CREATE TEMPORARY TABLE AnalysisData\n"
        "(\n"
        "   `Signal`        INT UNSIGNED  NOT NULL,\n"
        "   `Background`    INT UNSIGNED  NOT NULL,\n"
        "   `SumEnergyEst`  DOUBLE        NOT NULL,\n"
        "   `SumW`          DOUBLE        NOT NULL,\n"
        "   INDEX (`.theta`)      USING HASH,\n"
        "   INDEX (`.sparse_est`) USING HASH\n"
        ") ENGINE=MEMORY COMMENT='Sum of counts and (squared) weightes of selected data after analysis'\n"
        "AS\n"
        "(\n"
        "   WITH Excess AS\n"
        "   (\n"                          << indent(6)
        << ifstream(analysis_sql).rdbuf() << indent(0) <<
        "   ),\n"                         <<
        "   Result AS\n"
        "   (\n"                          << indent(6)
        << data_sql << indent(0)          << // Must end with EOL and not in the middle of a comment
        "   )\n"
        "   SELECT * FROM Result\n"
        ")";

    query12.parse();
    for (auto it=env.cbegin(); it!=env.cend(); it++)
        query12.template_defaults[it->first.c_str()] = it->second.c_str();

    //query5.template_defaults["columns"]   = "FileId, EvtNumber,";
    query12.template_defaults["columns"]   = "";
    query12.template_defaults["zenith"]    = "fZenithDistanceMean";
    query12.template_defaults["files"]     = "DataFiles";
    query12.template_defaults["runinfo"]   = "factdata.RunInfo";
    query12.template_defaults["events"]    = "factdata.Images";
    query12.template_defaults["positions"] = "factdata.Position";

    query12.template_defaults["sparse"]    = str_sparse.c_str();
    query12.template_defaults["theta"]     = str_theta.c_str();
    query12.template_defaults["estimator"] = estimator.c_str();

    if (print_queries)
        PrintQuery(query12.str());

    qlog << query12 << ";\n" << endl;
    if (connection.connected())
    {
        cout << query12.execute().info() << endl;
        ShowWarnings(connection);
        Dump(flog, connection, "AnalysisData");

        const auto sec12 = Time().UnixTime()-start12.UnixTime();
        cout << "Execution time: " << sec12 << "s\n";
    }

    // -------------------------------------------------------------------
    // --------------------------- Spectrum ------------------------------
    // -------------------------------------------------------------------

    sindent mindent(mlog);
    mlog << "void spectrum()\n";
    mlog << "{\n" << indent(4);
    mlog <<
        "TGraphErrors *g = new TGraphErrors;\n"
        "g->SetMarkerStyle(kFullDotMedium);\n\n"
        "TGraph *ul = new TGraph;\n"
        "ul->SetMarkerStyle(23);\n\n";

    const vector<string> targets = { "Theta", "Energy" };

    for (auto ib=targets.cbegin(); ib!=targets.cend(); ib++)
    {
        const string table = "Spectrum"+*ib;

        cout << separator(table) << '\n';

        Time start13;
        /*
         "CREATE TEMPORARY TABLE Spectrum\n"
        "(\n"
        "   `.energy`      SMALLINT UNSIGNED NOT NULL COMMENT 'Bin Index [Energy]' PRIMARY KEY,\n"
        "   lo             DOUBLE            NOT NULL COMMENT 'Lower edge of energy bin in lg(E/GeV)',\n"
        "   hi             DOUBLE            NOT NULL COMMENT 'Upper edge of energy bin in lg(E/GeV)',\n"
        "   `Signal`       DOUBLE            NOT NULL COMMENT 'Number of signal events',\n"
        "   `Background`   DOUBLE            NOT NULL COMMENT 'Average number of background events',\n"
        "   `Excess`       DOUBLE            NOT NULL COMMENT 'Number of excess events',\n"
        "   ErrSignal      DOUBLE            NOT NULL COMMENT 'Poisson error on number of signal events',\n"
        "   ErrBackground  DOUBLE            NOT NULL COMMENT 'Poisson error on number of background events',\n"
        "   `ErrExcess`    DOUBLE            NOT NULL COMMENT 'Error of excess events',\n"
        "   `Significance` DOUBLE            NOT NULL COMMENT 'Li/Ma sigficance',\n"
        "   `ExcessN`      DOUBLE            NOT NULL COMMENT 'Number of excess events in simulated data',\n"
        "   `ExcessW`      DOUBLE            NOT NULL COMMENT 'Weighted number of excess events in simulated data',\n"
        "   `ErrExcessN`   DOUBLE            NOT NULL COMMENT 'Error or number of excess events in simulated data',\n"
        "   `ErrExcessW`   DOUBLE            NOT NULL COMMENT 'Error of weighted number of excess events in simulated data',\n"
        "   SignalW        DOUBLE            NOT NULL COMMENT 'Weighted number of signal events in simulated data',\n"
        "   BackgroundW    DOUBLE            NOT NULL COMMENT 'Weighted number of background events in simulated data',\n"
        "   ErrSignalW     DOUBLE            NOT NULL COMMENT 'Error of weighted number of signal events in simulated data',\n"
        "   ErrBackgroundW DOUBLE            NOT NULL COMMENT 'Error of weighted number of background events in simulated data',\n"
        "   Flux           DOUBLE            NOT NULL COMMENT 'Measured Monte Carlo Flux dN/dA/dt [cm^-2 s-^1 TeV^-1]',\n"
        "   ErrFlux        DOUBLE            NOT NULL COMMENT 'Error of measures Monte Carlo Flux dN/dA/dt [cm^-2 s-^1 TeV^-1]',\n"
        //"   CountSim       DOUBLE            NOT NULL COMMENT 'Simulated Monte Carlo Events',\n"
        //"   ErrCountSim    DOUBLE            NOT NULL COMMENT 'Error of Simulated Monte Carlo Events',\n"
        //"   FluxSim        DOUBLE            NOT NULL COMMENT 'Simulated Monte Carlo Flux dN/dA/dt [cm^-2 s-^1 TeV^-1]',\n"
        //"   ErrFluxSim     DOUBLE            NOT NULL COMMENT 'Error of Simulated Monte Carlo Flux dN/dA/dt [cm^-2 s-^1 TeV^-1]',\n"
        "   Bias           DOUBLE            NOT NULL COMMENT 'Energy Bias, average residual in lg(E)',\n"
        "   Resolution     DOUBLE            NOT NULL COMMENT 'Energy resolution, standard divation of residual in lg(E)',\n"
        //"   EfficiencyN    DOUBLE            NOT NULL COMMENT 'Simulated cut efficiency (weighted)',\n"
        //"   EfficiencyW    DOUBLE            NOT NULL COMMENT 'Simulated cut efficiency (unweighted)',\n"
        //"   ErrEfficiencyN DOUBLE            NOT NULL COMMENT 'Error of simulated cut efficiency (weighted)',\n"
        //"   ErrEfficiencyW DOUBLE            NOT NULL COMMENT 'Error of simulated cut efficiency (unweighted)'\n"
        ") ENGINE=Memory\n"
*/

        mysqlpp::Query query13(&connection);
        sindent indent13(query13);
        query13 <<
            "CREATE TEMPORARY TABLE %100:table ENGINE=MEMORY COMMENT='Combined information from different sources into final spectrum' AS\n"
            "(\n"
            << indent(3) << spectrum_sql << indent(0) <<
            ")";

        // [ Sa^2/a^2 + Sb^2/b^2 ] * a^2/b^2
        // [ (sc^2)/c^2+(sb^2)/b^2+(sa^2)/a^2 ]  * a^2*b^2/c^2

        query13.parse();
        //for (auto it=env.cbegin(); it!=env.cend(); it++)
        //    query13.template_defaults[it->first.c_str()] = it->second.c_str();

        query13.template_defaults["table"]     = table.c_str();
        query13.template_defaults["bin"]       = *ib=="Theta" ? "`.theta`"    : "`.sparse_est`";
        query13.template_defaults["id"]        = *ib=="Theta" ? "Sim"         : "Est";
        query13.template_defaults["weight"]    = *ib=="Theta" ? "ZdWeight"    : "1";
        query13.template_defaults["errweight"] = *ib=="Theta" ? "ErrZdWeight" : "1";
        if (*ib=="Theta")
        {
            query13.template_defaults["join1"] = "SummaryTheta Sim USING (`.theta`)";
            query13.template_defaults["join2"] = "ThetaDist USING (`.theta`)";
        }
        else
        {
            query13.template_defaults["join1"] = "SummaryEstimatedEnergy_sparse Est USING (`.sparse_est`)";
            query13.template_defaults["join2"] = "SummaryTrueEnergy_sparse Sim ON Est.`.sparse_est`=Sim.`.sparse_sim`";
        }

        if (print_queries)
            PrintQuery(query13.str());

        qlog << query13 << ";\n" << endl;
        if (connection.connected())
        {
            cout << query13.execute().info() << endl;
            ShowWarnings(connection);
            Dump(flog, connection, table);

            const auto sec13 = Time().UnixTime()-start13.UnixTime();
            cout << "Execution time: " << sec13 << "s\n";


            const mysqlpp::StoreQueryResult res13 = connection.query("SELECT * FROM "+table).store();

            // --------------------------------------------------------------------------
#ifdef HAVE_ROOT
            TFeldmanCousins fc;
            fc.SetCL(confidence);
            fc.SetMuStep(0.2);
            // f.SetMuMax(10*(sig+bg)); //has to be higher than sig+bg!!
            // Double_t ul=f.CalculateUpperLimit(x,y);
            // x=Dat.Signal       : number of observed events in the experiment
            // y=Dat.Background/5 : average number of observed events in background region

            TRolke rolke(confidence);
            // rolke.SetPoissonBkgBinomEff(x,y,z,tau,m)
            // x=Dat.Signal     : number of observed events in the experiment
            // y=Dat.Background : number of observed events in background region
            // tau=0.2          : the ratio between signal and background region
            // m=Sim.SimFluxN   : number of MC events generated
            // z=Sim.AnaFluxN   : number of MC events observed
#endif
            // --------------------------------------------------------------------------

            // Crab Nebula: 1 TeV: 3e-7  /  m^2 / s / TeV
            // Crab Nebula: 1 TeV: 3e-11 / cm^2 / s / TeV

            map<size_t, double> feldman_ul;
            map<size_t, double> rolke_ul;
            map<size_t, double> rolke_ll;
            map<size_t, double> rolke_int;

            if (verbose>0)
                cout << "Bin     Excess     Significance          Flux ErrFlux" << endl;

            for (auto ir=res13.cbegin(); ir!=res13.cend(); ir++)
            {
                // This is not efficient but easier to read and safer
                const mysqlpp::Row &row = *ir;

                const size_t bin = row[*ib=="Theta" ? ".theta" : ".sparse_est"];

                const double flux   = row["Flux"];
                const double error  = row["ErrFlux"];
                const double center = row["center"];
                const double sigma  = row["SigmaFlux"];

                if (*ib=="Energy" && flux>0)
                {
                    mlog << "g->SetPoint(g->GetN(), pow(10, " << center << "), " << flux << ");\n";
                    mlog << "g->SetPointError(g->GetN()-1, 0, " << error << ");\n";
                }

#ifdef HAVE_ROOT
                const double dat_sig  = row["Signal"];
                const double dat_bg   = row["Background"];

                const double dat_isig = row["SignalI"];
                const double dat_ibg  = row["BackgroundI"];

                const double eff      = row["Efficiency"];
                const double ieff     = row["EfficiencyI"];

                const double areatime = row["AreaTime"];
                const double width    = row["Width"];

                fc.SetMuMax(10*(dat_sig+dat_bg)); //has to be higher than sig+bg!!

                if (feldman)
                    feldman_ul[bin] = fc.CalculateUpperLimit(dat_sig, dat_bg)/width/areatime/eff;

                rolke.SetPoissonBkgKnownEff(dat_sig,  dat_bg*5, 5, eff);
                rolke_ll[bin]  = rolke.GetLowerLimit()/width/areatime;
                rolke_ul[bin]  = rolke.GetUpperLimit()/width/areatime;

                rolke.SetPoissonBkgKnownEff(dat_isig, dat_ibg*5, 5, ieff);
                rolke_int[bin] = rolke.GetUpperLimit()/areatime;

                if (*ib=="Energy" && (sigma<1 || dat_sig<10 || dat_bg<2))
                    mlog << "ul->SetPoint(ul->GetN(), pow(10, " << center << "), " << double(rolke_ul[bin]) << ");\n";
#endif

                if (verbose>0)
                {
                    cout << setw(5) << center << ":";
                    cout << " " << setw(10) << row["Excess"];
                    cout << " " << setw(10) << row["Significance"];
                    cout << " " << setw(10) << flux;
                    cout << " " << setw(10) << error;
                    cout << endl;
                }
            }

            Histogram hist;
            hist.table    = table;
            hist.binningx = *ib=="Theta" ? binning_theta : binning_sparse;
            hist.x        = *ib=="Theta" ? ".theta" : ".sparse_est";
            hist.axisx    = *ib=="Theta" ? "Zenith Distance #theta [#circ]" : "Energy lg(E/GeV)";
            hist.stats    = false;

            const vector<string> types = *ib=="Theta" ? vector<string>{ "" } : vector<string>{ "", "I" };
            for (auto it=types.cbegin(); it!=types.cend(); it++)
            {
                const bool integral = *ib=="Energy" && !it->empty();

                hist.dir = *ib=="Theta" ? "Data/Theta" : (it->empty() ? "Data/Energy/Differential" : "Data/Energy/Integral");

                hist.axisy = "Counts";
                if (integral)
                    hist.axisy += " (E>E_{lo})";

                hist.title = "";
                hist.v     = "Signal"+*it;
                hist.err   = "ErrSignal"+*it;
                WriteHistogram(connection, hist);

                hist.title = "";
                hist.v     = "Background"+*it;
                hist.err   = "ErrBackground"+*it;
                WriteHistogram(connection, hist);

                hist.title = "";
                hist.v     = "Excess"+*it;
                hist.err   = "ErrExcess"+*it;
                WriteHistogram(connection, hist);

                hist.title = "";
                hist.v     = "Significance"+*it;
                hist.err   = "";
                hist.axisy = "#sigma";
                if (integral)
                    hist.axisy += " (E>E_{lo})";
                WriteHistogram(connection, hist);

                hist.title = "";
                hist.v     = "AvgEnergyEst"+*it;
                hist.err   = "";
                hist.axisy = "<E_{est}>/GeV";
                if (integral)
                    hist.axisy += " (E>E_{lo})";
                WriteHistogram(connection, hist);

                hist.title = "";
                hist.v     = "ExcessRatio"+*it;
                hist.err   = "ErrExcessRatio"+*it;
                hist.axisy = "Ratio";
                if (integral)
                    hist.axisy += " (E>E_{lo})";
                WriteHistogram(connection, hist);
            }

            hist.dir = *ib=="Theta" ? "Data/Theta" : "Data/Energy/Differential";
            hist.axisy = *ib=="Theta" ? "dN/dE [cm^{-2} s^{-1}]" : "dN/dE [cm^{-2} s^{-1} TeV^{-1}]";

            hist.name = "Spectrum";
            hist.v    = "Flux";
            hist.err  = "ErrFlux";
            WriteHistogram(connection, hist);

            hist.name  = "SigmaFlux";
            hist.v     = "SigmaFlux";
            hist.err   = "";
            hist.axisy = "Relative standard deviations";
            WriteHistogram(connection, hist);

            if (*ib=="Energy")
            {
                hist.axisy = "dN/dE (E>E_{lo}) [cm^{-2} s^{-1}]";

                hist.dir   = "Data/Energy/Integral";
                hist.name  = "Spectrum";
                hist.v     = "FluxI";
                hist.err   = "ErrFluxI";
                WriteHistogram(connection, hist);

                hist.dir   = "Data/Energy/Differential";
                hist.name  = "IntegratedSpectrum";
                hist.v     = "IntegratedFlux";
                hist.err   = "ErrIntegratedFlux";
                WriteHistogram(connection, hist);

                hist.dir   = "Data/Energy/Integral";
                hist.name  = "SigmaFlux";
                hist.v     = "SigmaFluxI";
                hist.err   = "";
                hist.axisy = "Relative standard deviations (E>E_{lo})";
                WriteHistogram(connection, hist);
            }

#ifdef HAVE_ROOT
            hist.dir   = *ib=="Theta" ? "Data/Theta" : "Data/Energy/Differential";
            hist.axisy = *ib=="Theta" ? "UL [cm^{-2} s^{-1}]" : "UL [cm^{-2} s^{-1} TeV^{-1}]";

            if (feldman)
            {
                hist.name = "FeldmanCousins";
                WriteHistogram(connection, hist, feldman_ul);
            }

            hist.name = "RolkeUL";
            WriteHistogram(connection, hist, rolke_ul);

            hist.name = "RolkeLL";
            WriteHistogram(connection, hist, rolke_ll);

            if (*ib=="Energy")
            {
                hist.dir  = "Data/Energy/Integral";
                hist.name = "RolkeUL";
                WriteHistogram(connection, hist, rolke_int);
            }
#endif
        }
    }

    mlog << "\n"
        //"g.DrawClone(\"AP\");\n"
        //"ul.DrawClone(\"P\");\n\n"
        "TMultiGraph mg;\n"
        "mg.SetTitle(\"Differential Energy Spectrum;E [GeV];dN/dE [cm^{-2} s^{-1} TeV^{-1}]\");\n"
        "mg.Add(g,  \"P\");\n"
        "mg.Add(ul, \"P\");\n"
        "mg.DrawClone(\"A\");\n\n"
        "gPad->SetLogx();\n"
        "gPad->SetLogy();\n";
    mlog << indent(0) << "}" << endl;

    // -------------------------------------------------------------------
    // ----------------------------- Summary -----------------------------
    // -------------------------------------------------------------------

    cout << separator("Summary") << '\n';
    const auto sec = Time().UnixTime()-start.UnixTime();
    cout << "Total execution time: " << sec << "s\n" << endl;

    return 0;
}
