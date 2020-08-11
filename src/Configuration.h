#ifndef FACT_Configuration
#define FACT_Configuration

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;

class Configuration
{
private:
    /// Convienience enum to access the fOption* data memebers more verbosely.
    enum
    {
        kHidden  = 0, ///< Index for hidden options (not shown in PrintParsed)
        kVisible = 1  ///< Index for options visible in PrintParsed
    };

    const std::string fName; /// argv[0]

    std::map<std::string, std::string> fEnvMap;

    po::options_description fOptionsCommandline[2]; /// Description of the command-line options
    po::options_description fOptionsConfigfile[2];  /// Description of the options in the configuration file
    po::options_description fOptionsDatabase[2];    /// Description of options from the database
    po::options_description fOptionsEnvironment[2]; /// Description of options from the environment

    po::positional_options_description fArgumentPositions; /// Description of positional command-line options (arguments)

    std::vector<std::string> fUnknownCommandline;   /// Storage container for unrecognized commandline options
    std::vector<std::string> fUnknownConfigfile;    /// Storage container for unrecognized options from configuration files
    std::vector<std::string> fUnknownEnvironment;   /// Storage container for unrecognized options from the environment
    std::vector<std::string> fUnknownDatabase;      /// Storage container for unrecognized options retrieved from the database

    std::map<std::string, std::string> fWildcardOptions;  /// Options which were registered using wildcards

    std::string fPriorityFile;  /// File name of the priority configuration file (overwrites option from the databse)
    std::string fPrefixPath;    /// Path to the default configuration file
    std::string fDefaultFile;   /// File name of the default configuration file (usually {program}.rc)
    std::string fDatabase;      /// URL for database connection (see Configuration::parse_database)

    po::variables_map fVariables;  /// Variables as compiled by the Parse-function, which will be passed to the program

    /// A default mapper for environment variables skipping all of them
    std::string DefaultMapper(const std::string env)
    {
        return fEnvMap[env];
    }

    /// Pointer to the mapper function for environment variables
    std::function<std::string(std::string)> fNameMapper;
    std::function<void()>                   fPrintUsage;
    std::function<void(const std::string&)> fPrintVersion;

    /// Helper function which return the max of the two arguments in the first argument
    static void Max(int &val, const int &comp)
    {
        if (comp>val)
            val=comp;
    }

    /// Helper for Parse to create list of used wildcard options
    void CreateWildcardOptions();

    // Helper functions for PrintOptions and GetOptions
    template<class T>
        std::string VecAsStr(const po::variable_value &v) const;
    std::string VarAsStr(const po::variable_value &v) const;

    /// Print all options from a list of already parsed options
    void PrintParsed(const po::parsed_options &parsed) const;
    /// Print a list of all unkown options within the given vector
    void PrintUnknown(const std::vector<std::string> &vec, int steps=1) const;

    virtual void PrintUsage() const { }
    virtual void PrintVersion() const;

    std::string UnLibToolize(const std::string &src) const;

public:
    struct Map : std::pair<std::string, std::string>
    {
        Map() { }
    };

    Configuration(const std::string &prgname="");
    virtual ~Configuration() { }

    /// Retrieve data from a database and return them as options
    static po::basic_parsed_options<char>
        parse_database(const std::string &prgname, const std::string &database, const po::options_description& desc, bool allow_unregistered=false);

    // Setup
    void AddOptionsCommandline(const po::options_description &cl, bool visible=true);
    void AddOptionsConfigfile(const po::options_description &cf, bool visible=true);
    void AddOptionsEnvironment(const po::options_description &env, bool visible=true);
    void AddOptionsDatabase(const po::options_description &db, bool visible=true);
    void AddOptions(const po::options_description &opt, bool visible=true)
    {
        AddOptionsCommandline(opt, visible);
        AddOptionsConfigfile(opt, visible);
        AddOptionsEnvironment(opt, visible);
        AddOptionsDatabase(opt, visible);
    }

    void SetArgumentPositions(const po::positional_options_description &desc);

    void SetNameMapper(const std::function<std::string(std::string)> &func);
    void SetNameMapper();

    void SetPrintUsage(const std::function<void(void)> &func);
    void SetPrintUsage();

    void SetPrintVersion(const std::function<void(const std::string &)> &func);
    void SetPrintVersion();

    void AddEnv(const std::string &conf, const std::string &env)
    {
        fEnvMap[env] = conf;
    }

    // Output
    void PrintOptions() const;
    void PrintUnknown() const;
    void PrintWildcardOptions() const;

    const std::map<std::string,std::string> &GetWildcardOptions() const { return fWildcardOptions; }
    const std::vector<std::string> GetWildcardOptions(const std::string &opt) const;

    template<class T>
    const std::map<std::string,T> GetOptions(const std::string &opt)
    {
        const std::vector<std::string> rc = GetWildcardOptions(opt+'*');

        std::map<std::string,T> map;
        for (auto it=rc.begin(); it!=rc.end(); it++)
            map[it->substr(opt.length())] = Get<T>(*it);

        return map;
    }

    std::multimap<std::string, std::string> GetOptions() const;

    // Process command line arguments
    const po::variables_map &Parse(int argc, const char **argv, const std::function<void()> &func=std::function<void()>());
    const po::variables_map &ParseFile(const std::string &fname, const bool &checkf);
    bool DoParse(int argc, const char **argv, const std::function<void()> &func=std::function<void()>());
    bool ReadFile(const std::string &fname, const bool &checkf=false);

    bool HasVersion()
    {
        return Has("version");
    }

    bool HasHelp()
    {
        return Has("help") || Has("help-config") || Has("help-env") || Has("help-database");
    }

    bool HasPrint()
    {
        return Has("print-all") || Has("print") || Has("print-default") ||
            Has("print-database") || Has("print-config") ||
            Has("print-environment") || Has("print-unknown") ||
            Has("print-options") || Has("print-wildcards");
    }

    // Simplified access to the parsed options
    template<class T>
        T Get(const std::string &var) { fWildcardOptions.erase(var); return fVariables[var].as<T>(); }
    bool Has(const std::string &var) { fWildcardOptions.erase(var); return fVariables.count(var)>0; }

    template<class T>
        std::vector<T> Vec(const std::string &var) { return Has(var) ? fVariables[var].as<std::vector<T>>() : std::vector<T>(); }

    template<class T, class S>
    T Get(const std::string &var, const S &val)
    {
        std::ostringstream str;
        str << var << val;
        return Get<T>(str.str());
    }

    template<class T>
    bool Has(const std::string &var, const T &val)
    {
        std::ostringstream str;
        str << var << val;
        return Has(str.str());
    }

    template<class T, class S>
    T GetDef(const std::string &var, const S &val)
    {
        return Has(var, val) ? Get<T>(var, val) : Get<T>(var+"default");
    }

    template<class T>
    bool HasDef(const std::string &var, const T &val)
    {
        // Make sure the .default option is touched
        const bool rc = Has(var+"default");

        return Has(var, val) ? true : rc;
    }

    void Remove(const std::string &var)
    {
        fVariables.erase(var);
    }

    const std::string GetPrefixedString(const std::string &var)
    {
        const boost::filesystem::path ff(Get<std::string>(var));
        const boost::filesystem::path pp(fPrefixPath);
        return (ff.has_parent_path() ? ff : pp/ff).string();
    }

/*
    template<class T>
    std::map<std::string, T> GetMap(const std::string &var)
    {
        const size_t len = var.length();

        std::map<std::string, T> rc;
        for (std::map<std::string, boost::program_options::variable_value>::const_iterator it=fVariables.begin();
             it!=fVariables.end(); it++)
            if (it->first.substr(0, len)==var)
                rc[it->first] = it->second.as<T>();

        return rc;
    }

    template<class T>
    std::vector<std::string> GetKeys(const std::string &var)
    {
        const size_t len = var.length();

        std::vector<std::string> rc;
        for (std::map<std::string, boost::program_options::variable_value>::const_iterator it=fVariables.begin();
             it!=fVariables.end(); it++)
            if (it->first.substr(0, len)==var)
                rc.push_back(it->first);

        return rc;
    }
*/
    const std::string &GetName() const { return fName; }
    const boost::filesystem::path GetPrefixPath() const { return fPrefixPath; }
};

template<typename T>
struct Hex
{
    T val;
    Hex() { }
    Hex(const T &v) : val(v) { }
    operator T() const { return val; }
};
template<typename T>
std::istream &operator>>(std::istream &in, Hex<T> &rc)
{
    T val;
    in >> std::hex >> val;
    rc.val = val;
    return in;
}

template<class T>
inline po::typed_value<T> *var(T *ptr=0)
{ return po::value<T>(ptr); }

template<class T>
inline po::typed_value<T> *var(const T &val, T *ptr=0)
{ return po::value<T>(ptr)->default_value(val); }

template<class T>
inline po::typed_value<std::vector<T>> *vars()
{ return po::value<std::vector<T>>(); }

inline po::typed_value<bool> *po_switch()
{ return po::bool_switch(); }

inline po::typed_value<bool> *po_bool(bool def=false)
{ return po::value<bool>()->implicit_value(true)->default_value(def); }


std::istream &operator>>(std::istream &in, Configuration::Map &m);

#endif
