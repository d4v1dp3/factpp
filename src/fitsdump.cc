//****************************************************************
/** @class FitsDumper

  @brief Dumps contents of fits tables to stdout or a file

 */
 //****************************************************************
#include "Configuration.h"

#include <float.h>

#include <map>
#include <fstream>

#include <boost/regex.hpp>

#include "tools.h"
#include "Time.h"
#include "factfits.h"

#ifdef HAVE_ROOT
#include "TFile.h"
#include "TTree.h"
#include "TFormula.h"
#endif

using namespace std;

struct MyColumn
{
    string name;

    fits::Table::Column col;

    uint32_t first;
    uint32_t last;

    void *ptr;
};

struct minMaxStruct
{
    double min;
    double max;
    long double average;
    long double squared;
    long numValues;
    minMaxStruct() : min(FLT_MAX), max(-FLT_MAX), average(0), squared(0), numValues(0) { }

    void add(long double val)
    {
        average += val;
        squared += val*val;

        if (val<min)
            min = val;

        if (val>max)
            max = val;

        numValues++;
    }
};


class FitsDumper : public factfits
{
private:
    string fFilename;
    bool fStream;

    // Convert CCfits::ValueType into a human readable string
    string ValueTypeToStr(char type) const;

    /// Lists all columns of an open file
    void List();                          
    void ListFileContent() const;
    void ListHeader(const string& filename);
    void ListKeywords(ostream &);

    vector<MyColumn> InitColumns(vector<string> list);
    vector<MyColumn> InitColumnsRoot(vector<string> &list);

    double GetDouble(const MyColumn &, size_t) const;
    int64_t GetInteger(const MyColumn &, size_t) const;
    string Format(const string &fmt, const double &val) const;
    string Format(const string &fmt, const MyColumn &, size_t) const;

    ///Display the selected columns values VS time
    int  Dump(ostream &, const vector<string> &, const vector<MyColumn> &, const string &, size_t, size_t, const string &, Configuration &);
    void DumpRoot(ostream &, const vector<string> &, const string &, size_t, size_t, const string &);
    void DumpMinMax(ostream &, const vector<MyColumn> &, size_t, size_t, bool);
    void DumpStats(ostream &, const vector<MyColumn> &, const string &, size_t, size_t);

public:
    FitsDumper(const string &fname, const string &tablename);

    ///Configures the fitsLoader from the config file and/or command arguments.
    int Exec(Configuration& conf);
};

// --------------------------------------------------------------------------
//
//! Constructor
//! @param out
//!        the ostream where to redirect the outputs
//
FitsDumper::FitsDumper(const string &fname, const string &tablename) : factfits(fname, tablename), fFilename(fname), fStream(false)
{
}

string FitsDumper::ValueTypeToStr(char type) const
{
    switch (type)
    {
        case 'L': return "bool(8)";
        case 'A': return "char(8)";
        case 'B': return "byte(8)";
        case 'I': return "short(16)";
        case 'J': return "int(32)";
        case 'K': return "int(64)";
        case 'E': return "float(32)";
        case 'D': return "double(64)";
    default:
        return "unknown";
    }
}

void FitsDumper::List()
{
    const fits::Table::Columns &fColMap = GetColumns();

    cout << endl;

    cout << "\nFile: " << fFilename << "\n";

    cout << " "  << Get<std::string>("EXTNAME", "");
    cout << " [" << GetNumRows() << "]\n";

    for (auto it=fColMap.cbegin(); it!=fColMap.cend(); it++)
    {
        cout << " " << setw(3)  << it->second.id;
        cout << "|" << setw(10) << ValueTypeToStr(it->second.type);
        cout << " " << it->first << "[" << it->second.num << "]";
        cout << " / " << it->second.comment;
        cout << " (" << it->second.unit << ")" << endl;
    }

    cout << endl;
}

void FitsDumper::ListKeywords(ostream &fout)
{
    const fits::Table::Keys &fKeyMap = GetKeys();

    for (auto it=fKeyMap.begin(); it != fKeyMap.end(); it++)
    {
        fout << "## " << ::left << setw(8) << it->first << "= ";

        if (it->second.type=='T')
            fout << ::left  << setw(20) << ("'"+it->second.value+"'");
        else
            fout << ::right << setw(20) << it->second.value;

        if (!it->second.comment.empty())
            fout << " / " << it->second.comment;
        fout << '\n';
    }

    fout << flush;
}

void FitsDumper::ListFileContent() const
{
    const std::vector<std::string> &tables = GetTables();

    cout << "File " << fFilename << " has " << tables.size() << " table(s): " << endl;
    for (auto it=tables.begin(); it!=tables.end(); it++)
        cout << " * " << *it << endl;
}

void FitsDumper::ListHeader(const string& filename)
{
    ostream fout(cout.rdbuf());

    ofstream sout;
    if (filename!="-")
    {
        sout.open(filename);
        if (!sout)
        {
            cerr << "Cannot open output stream " << filename << ": " << strerror(errno) << endl;
            return;
        }
        fout.rdbuf(sout.rdbuf());
    }

    const fits::Table::Keys &fKeyMap = GetKeys();

    fout << "\nTable: " << fKeyMap.find("EXTNAME")->second.value << " (rows=" << GetNumRows() << ")\n";
    if (fKeyMap.find("COMMENT") != fKeyMap.end())
        fout << "Comment: \t" << fKeyMap.find("COMMENT")->second.value << "\n";

    ListKeywords(fout);
    fout << endl;
}

vector<MyColumn> FitsDumper::InitColumns(vector<string> names)
{
    static const boost::regex expr("([[:word:].]+)(\\[([[:digit:]]+)?(:)?([[:digit:]]+)?\\])?");

    const fits::Table::Columns &fColMap = GetColumns();

    if (names.empty())
        for (auto it=fColMap.begin(); it!=fColMap.end(); it++)
            if (it->second.num>0)
                names.push_back(it->first);

    vector<MyColumn> vec;

    for (auto it=names.begin(); it!=names.end(); it++)
    {
        boost::smatch what;
        if (!boost::regex_match(*it, what, expr, boost::match_extra))
        {
            cerr << "Couldn't parse expression '" << *it << "' " << endl;
            return vector<MyColumn>();
        }

        const string name = what[1];

        const auto iter = fColMap.find(name);
        if (iter==fColMap.end())
        {
            cerr << "ERROR - Column '" << name << "' not found in table." << endl;
            return vector<MyColumn>();
        }

        const fits::Table::Column &col = iter->second;

        const string val0  = what[3];
        const string delim = what[4];
        const string val1  = what[5];

        const uint32_t first = atol(val0.c_str());
        const uint32_t last  = (val0.empty() && delim.empty()) ? col.num-1 : (val1.empty() ? first : atoi(val1.c_str()));

        if (first>=col.num)
        {
            cerr << "ERROR - First index " << first << " for column " << name << " exceeds number of elements " << col.num << endl;
            return vector<MyColumn>();
        }

        if (last>=col.num)
        {
            cerr << "ERROR - Last index " << last << " for column " << name << " exceeds number of elements " << col.num << endl;
            return vector<MyColumn>();
        }

        if (first>last)
        {
            cerr << "ERROR - Last index " << last << " for column " << name << " exceeds first index " << first << endl;
            return vector<MyColumn>();
        }

        MyColumn mycol;

        mycol.name  = name;
        mycol.col   = col;
        mycol.first = first;
        mycol.last  = last;
        mycol.ptr   = SetPtrAddress(name);

        vec.push_back(mycol);
    }

    return vec;
}

double FitsDumper::GetDouble(const MyColumn &it, size_t i) const
{
    switch (it.col.type)
    {
    case 'A':
        return reinterpret_cast<const char*>(it.ptr)[i];

    case 'L':
        return reinterpret_cast<const bool*>(it.ptr)[i];

    case 'B':
        return (unsigned int)reinterpret_cast<const uint8_t*>(it.ptr)[i];

    case 'I':
        return reinterpret_cast<const int16_t*>(it.ptr)[i];

    case 'J':
        return reinterpret_cast<const int32_t*>(it.ptr)[i];

    case 'K':
        return reinterpret_cast<const int64_t*>(it.ptr)[i];

    case 'E':
        return reinterpret_cast<const float*>(it.ptr)[i];

    case 'D':
        return reinterpret_cast<const double*>(it.ptr)[i];
    }

    return 0;
}

int64_t FitsDumper::GetInteger(const MyColumn &it, size_t i) const
{
    switch (it.col.type)
    {
    case 'A':
        return reinterpret_cast<const char*>(it.ptr)[i];

    case 'L':
        return reinterpret_cast<const bool*>(it.ptr)[i];

    case 'B':
        return (unsigned int)reinterpret_cast<const uint8_t*>(it.ptr)[i];

    case 'I':
        return reinterpret_cast<const int16_t*>(it.ptr)[i];

    case 'J':
        return reinterpret_cast<const int32_t*>(it.ptr)[i];

    case 'K':
        return reinterpret_cast<const int64_t*>(it.ptr)[i];

    case 'E':
        return reinterpret_cast<const float*>(it.ptr)[i];

    case 'D':
        return reinterpret_cast<const double*>(it.ptr)[i];
    }

    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
string FitsDumper::Format(const string &format, const MyColumn &col, size_t i) const
{
    switch (*format.rbegin())
    {
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        return Tools::Form(format.c_str(), GetDouble(col, i));

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        return Tools::Form(format.c_str(), GetInteger(col, i));

    case 'h':
        {
            string rc = Tools::Scientific(GetDouble(col, i));
            *remove_if(rc.begin(), rc.end(), ::isspace)=0;
            return rc;
        }
    }

    return "";
}

string FitsDumper::Format(const string &format, const double &val) const
{
    switch (*format.rbegin())
    {
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        return Tools::Form(format.c_str(), int64_t(val));

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        return Tools::Form(format.c_str(), val);

    case 'h':
        {
            string rc = Tools::Scientific(val);
            *remove_if(rc.begin(), rc.end(), ::isspace)=0;
            return rc;
        }
    }

    return "";
}
#pragma GCC diagnostic pop


#ifdef HAVE_ROOT
template<typename T>
void SetBranch(TTree *ttree, const string &bname, const MyColumn &col)
{
    TBranch *b = ttree->GetBranch(bname.c_str());

    T *address = reinterpret_cast<T*>(col.ptr)+col.first;
    if (b)
        b->SetAddress(address);
    else
        ttree->Branch(bname.c_str(), address);
}
#endif

// --------------------------------------------------------------------------
//
//! Perform the actual dump, based on the current parameters
//
#ifdef HAVE_ROOT
int FitsDumper::Dump(ostream &fout, const vector<string> &format, const vector<MyColumn> &cols, const string &filter, size_t first, size_t limit, const string &filename, Configuration &conf)
#else
int FitsDumper::Dump(ostream &fout, const vector<string> &format, const vector<MyColumn> &cols, const string &, size_t first, size_t limit, const string &filename, Configuration &)
#endif
{
    const fits::Table::Keys &fKeyMap = GetKeys();

#ifdef HAVE_ROOT
    TFormula select;
    if (!filter.empty() && select.Compile(filter.c_str()))
        throw runtime_error("Syntax Error: TFormula::Compile failed for '"+filter+"'");
#endif

    fout << "## --------------------------------------------------------------------------\n";
    fout << "## Fits file:  \t" << fFilename << '\n';
    if (filename!="-")
        fout << "## File:      \t" << filename << '\n';
    fout << "## Table:     \t" << fKeyMap.find("EXTNAME")->second.value << '\n';
    fout << "## NumRows:   \t" << GetNumRows() << '\n';
    fout << "## Comment:   \t" << ((fKeyMap.find("COMMENT") != fKeyMap.end()) ? fKeyMap.find("COMMENT")->second.value : "") << '\n';
#ifdef HAVE_ROOT
    if (!filter.empty())
        fout << "## Selection: \t" << select.GetExpFormula() << '\n';
#endif
    fout << "## --------------------------------------------------------------------------\n";
    ListKeywords(fout);
    fout << "## --------------------------------------------------------------------------\n";
    fout << "#\n";

#ifdef HAVE_ROOT
    const bool rootfile = filename.rfind(".root")==filename.size()-5;

    TFile *tfile = 0;
    if (rootfile)
    {
        tfile = new TFile(filename.c_str(), conf.Get<bool>("update")?"UPDATE":(conf.Get<bool>("overwrite")?"RECREATE":"CREATE"), fFilename.c_str(), conf.Get<bool>("force"));
        if (tfile->IsZombie())
        {
            cerr << "Could not open root file '" << filename << "'" << endl;
            return -2;
        }
    }

    // FIXME: Add header values
    TTree *ttree = 0;
    if (tfile)
    {
        const string tname = fKeyMap.find("EXTNAME")->second.value;
        tfile->GetObject(tname.c_str(), ttree);
        if (!ttree)
            ttree = new TTree(tname.c_str(), Tools::Form("From N=%d [cnt=%d] {filter=%s}", first, limit, filter.c_str()).c_str());
    }
#endif

    size_t num = 0;
    for (auto it=cols.begin(); it!=cols.end(); it++)
    {
        fout << "# " << it->name;

        if (it->first==it->last)
        {
            if (it->first!=0)
                fout << "[" << it->first << "]";
        }
        else
            fout << "[" << it->first << ":" << it->last << "]";

        if (!it->col.unit.empty())
            fout << ": " << it->col.unit;
        fout << '\n';

        num += it->last-it->first+1;
    }
    fout << "#" << endl;

    // -----------------------------------------------------------------

#ifdef HAVE_ROOT
    if (ttree)
    {
        fout << "## --------------------------------------------------------------------------\n";

        fout << "#\n";
        fout << "# Tree: " << fKeyMap.find("EXTNAME")->second.value << "\n";
        fout << "#\n";

        for (auto it=cols.begin(); it!=cols.end(); it++)
        {
            string branch = it->name;
            if (it->last!=it->first)
                branch += "["+to_string(it->last-it->first+1)+"]";

            fout << "# Branch: " << branch << "\n";

            switch (it->col.type)
            {
            case 'B':
                SetBranch<unsigned char>(ttree, branch, *it);
                break;
            case 'L':
                SetBranch<bool>(ttree, branch, *it);
                break;
            case 'I':
                SetBranch<int16_t>(ttree, branch, *it);
                break;
            case 'J':
                SetBranch<int32_t>(ttree, branch, *it);
                break;
            case 'K':
                SetBranch<int64_t>(ttree, branch, *it);
                break;
            case 'E':
                SetBranch<float>(ttree, branch, *it);
                break;
            case 'D':
                SetBranch<double>(ttree, branch, *it);
                break;
            default:
                ;
            }
        }

        fout << "#\n";
        fout << "## --------------------------------------------------------------------------\n";
}
#endif


    // -----------------------------------------------------------------



#ifdef HAVE_ROOT
    vector<Double_t> data(num+1);
#endif

    const size_t last = limit ? first + limit : size_t(-1);

    while (GetRow(first++, !fStream))
    {
        const size_t row = GetRow();
        if ((!fStream && row==GetNumRows()) || row==last)
            break;

        size_t p = 0;

#ifdef HAVE_ROOT
        data[p++] = first-1;
#endif

        ostringstream sout;
        sout.precision(fout.precision());
        sout.flags(fout.flags());

        uint32_t col = 0;
        for (auto it=cols.begin(); it!=cols.end(); it++, col++)
        {
            string msg;
            for (uint32_t i=it->first; i<=it->last; i++, p++)
            {
                if (col<format.size())
                    sout << Format("%"+format[col], *it, i) << " ";
                else
                {
                    switch (it->col.type)
                    {
                    case 'A':
                        msg += reinterpret_cast<const char*>(it->ptr)[i];
                        break;
                    case 'B':
                        sout << (unsigned int)reinterpret_cast<const unsigned char*>(it->ptr)[i] << " ";
                        break;
                    case 'L':
                        sout << reinterpret_cast<const bool*>(it->ptr)[i] << " ";
                        break;
                    case 'I':
                        sout << reinterpret_cast<const int16_t*>(it->ptr)[i] << " ";
                        break;
                    case 'J':
                        sout << reinterpret_cast<const int32_t*>(it->ptr)[i] << " ";
                        break;
                    case 'K':
                        sout << reinterpret_cast<const int64_t*>(it->ptr)[i] << " ";
                        break;
                    case 'E':
                        sout << reinterpret_cast<const float*>(it->ptr)[i] << " ";
                        break;
                    case 'D':
                        sout << reinterpret_cast<const double*>(it->ptr)[i] << " ";
                        break;
                    default:
                        ;
                    }
                }
#ifdef HAVE_ROOT
                if (!filter.empty())
                    data[p] = GetDouble(*it, i);
#endif
            }

            if (it->col.type=='A')
                sout << "'" << msg.c_str() << "' ";
        }
#ifdef HAVE_ROOT
        if (!filter.empty() && select.EvalPar(0, data.data())<0.5)
            continue;

        if (ttree)
        {
            ttree->Fill();
            continue;
        }
#endif

        fout << sout.str() << endl;
    }

#ifdef HAVE_ROOT
    if (tfile)
    {
        fout << "#\n";
        fout << "# " << ttree->GetEntries() << " rows filled into tree.\n";
        fout << "#\n";
        fout << "## --------------------------------------------------------------------------\n";
        fout << endl;

        ttree->Write("", TObject::kWriteDelete);
        delete tfile;
    }
#endif

    return 0;
}

vector<MyColumn> FitsDumper::InitColumnsRoot(vector<string> &names)
{
    static const boost::regex expr("[^\\[]([[:word:].]+)(\\[([[:digit:]]+)\\])?");

    const fits::Table::Columns &cols = GetColumns();

    vector<MyColumn> vec;

    for (auto it=names.begin(); it!=names.end(); it++)
    {
        if (it->empty())
            continue;

        *it = ' '+*it;

        string::const_iterator ibeg = it->begin();
        string::const_iterator iend = it->end();

        boost::smatch what;
        while (boost::regex_search(ibeg, iend, what, expr, boost::match_extra))
        {
            const string all  = what[0];
            const string name = what[1];
            const size_t idx  = atol(string(what[3]).c_str());

            // Check if found colum is valid
            const auto ic = cols.find(name);
            if (ic==cols.end())
            {
                ibeg++;
                //cout << "Column '" << name << "' does not exist." << endl;
                //return vector<MyColumn>();
                continue;
            }
            if (idx>=ic->second.num)
            {
                cout << "Column '" << name << "' has no index " << idx << "." << endl;
                return vector<MyColumn>();
            }

            // find index if column already exists
            size_t p = 0;
            for (; p<vec.size(); p++)
                if (vec[p].name==name)
                    break;

            const string id = '['+to_string(p)+']';

            // Replace might reallocate the memory. Therefore, we cannot use what[0].first
            // directly but have to store the offset
            const size_t offset = what[0].first - it->begin();

            it->replace(ibeg-it->begin()+what.position(1), what.length()-1, id);

            ibeg = it->begin() + offset + id.size();
            iend = it->end();

            if (p<vec.size())
                continue;

            // Column not found, add new column
            MyColumn mycol;

            mycol.name  = name;
            mycol.col   = ic->second;
            mycol.first = idx;
            mycol.last  = idx;
            mycol.ptr   = SetPtrAddress(name);

            vec.push_back(mycol);
        }
    }

    ostringstream id;
    id << '[' << vec.size() << ']';

    for (auto it=names.begin(); it!=names.end(); it++)
    {
        while (1)
        {
            auto p = it->find_first_of('#');
            if (p==string::npos)
                break;

            it->replace(p, 1, id.str());
        }
    }

    //cout << endl;
    //for (size_t i=0; i<vec.size(); i++)
    //    cout << "val[" << i << "] = " << vec[i].name << '[' << vec[i].first << ']' << endl;
    //cout << endl;

    return vec;
}

#ifdef HAVE_ROOT
void FitsDumper::DumpRoot(ostream &fout, const vector<string> &cols, const string &filter, size_t first, size_t limit, const string &filename)
#else
void FitsDumper::DumpRoot(ostream &, const vector<string> &, const string &, size_t, size_t, const string &)
#endif
{
#ifdef HAVE_ROOT
    vector<string> names(cols);
    names.insert(names.begin(), filter);

    const vector<MyColumn> vec = InitColumnsRoot(names);
    if (vec.empty())
        return;

    vector<TFormula> form(names.size());

    auto ifo = form.begin();
    for (auto it=names.begin(); it!=names.end(); it++, ifo++)
    {
        if (!it->empty() && ifo->Compile(it->c_str()))
            throw runtime_error("Syntax Error: TFormula::Compile failed for '"+*it+"'");
    }

    const fits::Table::Keys &fKeyMap = GetKeys();

    fout << "## --------------------------------------------------------------------------\n";
    fout << "## Fits file:  \t" << fFilename << '\n';
    if (filename!="-")
        fout << "## File:      \t" << filename << '\n';
    fout << "## Table:     \t" << fKeyMap.find("EXTNAME")->second.value << '\n';
    fout << "## NumRows:   \t" << GetNumRows() << '\n';
    fout << "## Comment:   \t" << ((fKeyMap.find("COMMENT") != fKeyMap.end()) ? fKeyMap.find("COMMENT")->second.value : "") << '\n';
    fout << "## --------------------------------------------------------------------------\n";
    ListKeywords(fout);
    fout << "## --------------------------------------------------------------------------\n";
    fout << "##\n";
    if (!filter.empty())
        fout << "## Selection: " << form[0].GetExpFormula() << "\n##\n";

    size_t num = 0;
    for (auto it=vec.begin(); it!=vec.end(); it++, num++)
    {
        fout << "## [" << num << "] = " << it->name;

        if (it->first==it->last)
        {
            if (it->first!=0)
                fout << "[" << it->first << "]";
        }
        else
            fout << "[" << it->first << ":" << it->last << "]";

        if (!it->col.unit.empty())
            fout << ": " << it->col.unit;
        fout << '\n';
    }
    fout << "##\n";
    fout << "## --------------------------------------------------------------------------\n";
    fout << "#\n";

    fout << "# ";
    for (auto it=form.begin()+1; it!=form.end(); it++)
        fout << " \"" << it->GetExpFormula() << "\"";
    fout << "\n#" << endl;

    // -----------------------------------------------------------------

    vector<Double_t> data(vec.size()+1);

    const size_t last = limit ? first + limit : size_t(-1);

    while (GetRow(first++, !fStream))
    {
        const size_t row = GetRow();
        if ((!fStream && row==GetNumRows()) || row==last)
            break;

        size_t p = 0;
        for (auto it=vec.begin(); it!=vec.end(); it++, p++)
            data[p] = GetDouble(*it, it->first);

        data[p] = first;

        if (!filter.empty() && form[0].EvalPar(0, data.data())<0.5)
            continue;

        for (auto iform=form.begin()+1; iform!=form.end(); iform++)
            fout << iform->EvalPar(0, data.data()) << " ";

        fout << endl;
    }
#endif
}


void FitsDumper::DumpMinMax(ostream &fout, const vector<MyColumn> &cols, size_t first, size_t limit, bool fNoZeroPlease)
{
    vector<minMaxStruct> statData(cols.size());

    // Loop over all columns in our list of requested columns
    const size_t last = limit ? first + limit : size_t(-1);

    while (GetRow(first++, !fStream))
    {
        const size_t row = GetRow();
        if ((!fStream && row==GetNumRows()) || row==last)
            break;

        auto statsIt = statData.begin();

        for (auto it=cols.begin(); it!=cols.end(); it++, statsIt++)
        {
            if ((it->name=="UnixTimeUTC" || it->name=="PCTime") && it->first==0 && it->last==1)
            {
                const uint32_t *val = reinterpret_cast<const uint32_t*>(it->ptr);
                if (fNoZeroPlease && val[0]==0 && val[1]==0)
                    continue;

                statsIt->add(Time(val[0], val[1]).Mjd());
                continue;
            }

            for (uint32_t i=it->first; i<=it->last; i++)
            {
                const double cValue = GetDouble(*it, i);

                if (fNoZeroPlease && cValue == 0)
                    continue;

                statsIt->add(cValue);
            }
        }
    }

    // okay. So now I've got ALL the data, loaded.
    // let's do the summing and averaging in a safe way (i.e. avoid overflow
    // of variables as much as possible)
    auto statsIt = statData.begin();
    for (auto it=cols.begin(); it!=cols.end(); it++, statsIt++)
    {
        fout << "\n[" << it->name << ':' << it->first;
        if (it->first!=it->last)
            fout << ':' << it->last;
        fout << "]\n";

        if (statsIt->numValues == 0)
        {
            fout << "Min: -\nMax: -\nAvg: -\nRms: -" << endl;
            continue;
        }

        const long &num = statsIt->numValues;

        long double &avg = statsIt->average;
        long double &rms = statsIt->squared;

        avg /= num;
        rms /= num;
        rms += avg*avg;
        rms  = rms<0 ? 0 : sqrt(rms);

        fout << "Min: " << statsIt->min << '\n';
        fout << "Max: " << statsIt->max << '\n';
        fout << "Avg: " << avg << '\n';
        fout << "Rms: " << rms << endl;
    }
}

template<typename T>
void displayStats(vector<char> &array, ostream& out)
{
    const size_t numElems = array.size()/sizeof(T);
    if (numElems == 0)
    {
        out << "Min: -\nMax: -\nMed: -\nAvg: -\nRms: -" << endl;
        return;
    }

    T *val = reinterpret_cast<T*>(array.data());

    sort(val, val+numElems);

    out << "Min: " << double(val[0]) << '\n';
    out << "Max: " << double(val[numElems-1]) << '\n';

    if (numElems%2 == 0)
        out << "Med: " << (double(val[numElems/2-1]) + double(val[numElems/2]))/2 << '\n';
    else
        out << "Med: " << double(val[numElems/2]) << '\n';

    long double avg = 0;
    long double rms = 0;
    for (uint32_t i=0;i<numElems;i++)
    {
        const long double v = val[i];
        avg += v;
        rms += v*v;
    }

    avg /= numElems;
    rms /= numElems;
    rms -= avg*avg;
    rms  = rms<0 ? 0 : sqrt(rms);


    out << "Avg: " << avg << '\n';
    out << "Rms: " << rms << endl;
}

#ifdef HAVE_ROOT
void FitsDumper::DumpStats(ostream &fout, const vector<MyColumn> &cols, const string &filter, size_t first, size_t limit)
#else
void FitsDumper::DumpStats(ostream &fout, const vector<MyColumn> &cols, const string &, size_t first, size_t limit)
#endif
{
#ifdef HAVE_ROOT
    TFormula select;
    if (!filter.empty() && select.Compile(filter.c_str()))
        throw runtime_error("Syntax Error: TFormula::Compile failed for '"+filter+"'");
#endif

    // Loop over all columns in our list of requested columns
    vector<vector<char>> statData;

    const size_t rows = limit==0 || GetNumRows()<limit ? GetNumRows() : limit;

    for (auto it=cols.begin(); it!=cols.end(); it++)
        statData.emplace_back(vector<char>(it->col.size*rows*(it->last-it->first+1)));

#ifdef HAVE_ROOT
    size_t num = 0;
    for (auto it=cols.begin(); it!=cols.end(); it++)
        num += it->last-it->first+1;

    vector<Double_t> data(num+1);
#endif

    // Loop over all columns in our list of requested columns
    const size_t last = limit ? first + limit : size_t(-1);

    uint64_t counter = 0;

    while (GetRow(first++, !fStream))
    {
        const size_t row = GetRow();
        if ((!fStream && row==GetNumRows()) || row==last)
            break;

#ifdef HAVE_ROOT
        if (!filter.empty())
        {
            size_t p = 0;

            data[p++] = first-1;

            for (auto it=cols.begin(); it!=cols.end(); it++)
                for (uint32_t i=it->first; i<=it->last; i++, p++)
                    data[p] = GetDouble(*it, i);

            if (select.EvalPar(0, data.data())<0.5)
                continue;
        }
#endif

        auto statsIt = statData.begin();
        for (auto it=cols.begin(); it!=cols.end(); it++, statsIt++)
        {
            const char *src = reinterpret_cast<const char*>(it->ptr);
            const size_t sz = (it->last-it->first+1)*it->col.size;
            memcpy(statsIt->data()+counter*sz, src+it->first*it->col.size, sz);
        }

        counter++;
    }

    auto statsIt = statData.begin();
    for (auto it=cols.begin(); it!=cols.end(); it++, statsIt++)
    {
        fout << "\n[" << it->name << ':' << it->first;
        if (it->last!=it->first)
            fout << ':' << it->last;
        fout << "]\n";

        const size_t sz = (it->last-it->first+1)*it->col.size;
        statsIt->resize(counter*sz);

        switch (it->col.type)
        {
        case 'L':
            displayStats<bool>(*statsIt, fout);
            break;
        case 'B':
            displayStats<char>(*statsIt, fout);
            break;
        case 'I':
            displayStats<int16_t>(*statsIt, fout);
            break;
        case 'J':
            displayStats<int32_t>(*statsIt, fout);
            break;
        case 'K':
            displayStats<int64_t>(*statsIt, fout);
            break;
        case 'E':
            displayStats<float>(*statsIt, fout);
            break;
        case 'D':
            displayStats<double>(*statsIt, fout);
            break;
        default:
            ;
        }
    }
}

// --------------------------------------------------------------------------
//
//! Retrieves the configuration parameters
//! @param conf
//!             the configuration object
//
int FitsDumper::Exec(Configuration& conf)
{
    fStream = conf.Get<bool>("stream");

    if (conf.Get<bool>("autocal"))
        resetCalibration();

    if (conf.Get<bool>("list"))
        List();

    if (conf.Get<bool>("filecontent"))
        ListFileContent();

    if (conf.Get<bool>("header"))
        ListHeader(conf.Get<string>("outfile"));


    if (conf.Get<bool>("header") || conf.Get<bool>("list") || conf.Get<bool>("filecontent"))
        return 1;

    // ------------------------------------------------------------

    if (conf.Get<bool>("minmax") && conf.Get<bool>("stat"))
    {
        cerr << "Invalid combination of options: cannot do stats and minmax." << endl;
        return -1;
    }
    if (conf.Get<bool>("stat") && conf.Get<bool>("nozero"))
    {
        cerr << "Invalid combination of options: nozero only works with minmax." << endl;
        return -1;
    }

    if (conf.Get<bool>("scientific") && conf.Get<bool>("fixed"))
    {
        cerr << "Switched --scientific and --fixed are mutually exclusive." << endl;
        return -1;
    }

    if (conf.Has("%") && conf.Has("%%"))
    {
        cerr << "Switched --% and --%% are mutually exclusive." << endl;
        return -1;
    }

    // ------------------------------------------------------------

    string filename = conf.Get<string>("outfile");

    if (!filename.empty() && filename[0]=='.')
    {
        if (fFilename.rfind(".fits")==fFilename.size()-5)
            filename.insert(0, fFilename.substr(0, fFilename.size()-5));
        if (fFilename.rfind(".fits.fz")==fFilename.size()-8)
            filename.insert(0, fFilename.substr(0, fFilename.size()-8));
    }

    ostream fout(cout.rdbuf());

    ofstream sout;
    if (filename!="-" && filename.rfind(".root")!=filename.length()-5)
    {
        sout.open(filename);
        if (!sout)
        {
            cerr << "Cannot open output stream " << filename << ": " << strerror(errno) << endl;
            return false;
        }
        fout.rdbuf(sout.rdbuf());
    }

    fout.precision(conf.Get<int>("precision"));
    if (conf.Get<bool>("fixed"))
        fout << fixed;
    if (conf.Get<bool>("scientific"))
        fout << scientific;

    const string filter = conf.Has("filter") ? conf.Get<string>("filter") : "";
    const size_t first  = conf.Get<size_t>("first");
    const size_t limit  = conf.Get<size_t>("limit");

#ifdef HAVE_ROOT
    if (conf.Get<bool>("root"))
    {
        DumpRoot(fout, conf.Vec<string>("col"), filter, first, limit, filename);
        return 0;
    }
#endif

    const vector<string> format = conf.Vec<string>("%");
    for (auto it=format.begin(); it<format.end(); it++)
    {
        static const boost::regex expr("-?[0-9]*[.]?[0-9]*[diouxXeEfFgGaAh]");

        boost::smatch what;
        if (!boost::regex_match(*it, what, expr, boost::match_extra))
        {
            cerr << "Format '" << *it << "' not supported." << endl;
            return -1;
        }
    }

    const vector<MyColumn> cols = InitColumns(conf.Vec<string>("col"));
    if (cols.empty())
        return false;

    if (conf.Get<bool>("minmax"))
    {
        DumpMinMax(fout, cols, first, limit, conf.Get<bool>("nozero"));
        return 0;
    }

    if (conf.Get<bool>("stat"))
    {
        DumpStats(fout, cols, filter, first, limit);
        return 0;
    }

    return Dump(fout, format, cols, filter, first, limit, filename, conf);
}

void PrintUsage()
{
    cout <<
        "fitsdump is a tool to dump data from a FITS table as ascii.\n"
        "\n"
        "Usage: fitsdump [OPTIONS] fitsfile col col ... \n"
        "  or:  fitsdump [OPTIONS]\n"
        "\n"
        "Addressing a column:\n"
        "  ColumnName:         Will address all fields of a column\n"
        "  ColumnName[n]:      Will address the n-th field of a column (starts with 0)\n"
        "  ColumnName[n1:n2]:  Will address all fields between n1 and including n2\n"
#ifdef HAVE_ROOT
        "\n"
        "Selecting a column:\n"
        "  Commandline option:  --filter\n"
        "  Explanation:  Such a selection is evaluated using TFormula, hence, every "
        "mathematical operation allowed in TFormula is allowed there, too. "
        "The reference is the column index as printed in the output stream, "
        "starting with 1. The index 0 is reserved for the row number.\n"
#endif
        ;
    cout << endl;
}

void PrintHelp()
{
#ifdef HAVE_ROOT
    cout <<
        "\n\n"
        "Examples:\n"
        "In --root mode, fitsdump support TFormula's syntax for all columns and the filter "
        "You can then refer to a column or a (single) index of the column just by its name "
        "If the index is omitted, 0 is assumed. Note that the [x:y] syntax in this mode is "
        "not supported\n"
        "\n"
        "  fitsdump Zd --filter=\"[0]>20 && cos([1])*TMath::RadToDeg()<45\"\n"
        "\n"
        "The columns can also be addressed with their names\n"
        "\n"
        "  fitsdump -r \"(Zd+Err)*TMath::DegToRad()\" --filter=\"[0]<25 && [1]<0.05\"\n"
        "\n"
        "is identical to\n"
        "\n"
        "  fitsdump -r \"(Zd[0]+Err[0])*TMath::DegToRad()\" --filter=\"[0]<25 && [1]<0.05\"\n"
        "\n"
        "A special placeholder exists for the row number\n"
        "\n"
        "  fitsdump -r \"#\" --filter=\"#>10 && #<100\"\n"
        "\n"
        "To format a single column you can do\n"
        "\n"
        "  fitsdump col1 -%.1f col2 -%d\n"
        "\n"
        "A special format is provided converting to 'human readable format'\n"
        "\n"
        "  fitsdump col1 -%h\n"
        "\n"
        "To convert a fits table to a root file, use --outfile, for example\n\n"
        "  fitsdump --outfile outfile.root [--compression N] [--overwrite|--update]\n\n"
        "Note  that this option can not be combined  with  the  root mode (--root). "
        "According to the FITS standard, all columns (except bytes) are written as "
        "signed values.\n"
        "\n";
    cout << endl;
#endif
}


void SetupConfiguration(Configuration& conf)
{
    po::options_description configs("Fitsdump options");
    configs.add_options()
        ("filecontent", po_switch(),            "List the number of tables in the file, along with their name")
        ("header,h",    po_switch(),            "Dump header of given table")
        ("list,l",      po_switch(),            "List all tables and columns in file")
        ("fitsfile",    var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
                                              , "Name of FITS file")
        ("col,c",       vars<string>(),         "List of columns to dump\narg is a list of columns, separated by a space.\nAdditionnally, a list of sub-columns can be added\ne.g. Data[3] will dump sub-column 3 of column Data\nData[3:4] will dump sub-columns 3 and 4\nOmitting this argument dump the entire column\nnote: all indices start at zero")
        ("outfile,o",   var<string>("-"),       "Name of output file (-:/dev/stdout), if outfile starts with a dot, the extension .fits and .fits.fz are replaced automatically.")
        ("precision,p", var<int>(20),           "Precision of ofstream")
        ("stat,s",      po_switch(),            "Perform statistics instead of dump")
        ("minmax,m",    po_switch(),            "Calculates min and max of data")
        ("nozero,z",    po_switch(),            "skip 0 values for stats")
        ("fixed",       po_switch(),            "Switch output stream to floating point values in fixed-point notation")
        ("scientific",  po_switch(),            "Switch output stream to floating point values in scientific notation")
        ("%,%",         vars<string>(),         "Format for the output (currently not available in root-mode)")
        ("force",       po_switch(),            "Force reading the fits file even if END key is missing")
        ("stream",      po_switch(),            "Stream reading the fits file (ignore the row count in the header)")
        ("first",       var<size_t>(size_t(0)), "First number of row to read")
        ("limit",       var<size_t>(size_t(0)), "Limit for the maximum number of rows to read (0=unlimited)")
        ("tablename,t", var<string>(""),        "Name of the table to open. If not specified, first binary table is opened")
        ("autocal",     po_switch(),            "This option can be used to skip the application of the noise calibration when reading back a compressed fits file. This is identical in using zfits instead of factfits.")
#ifdef HAVE_ROOT
        ("root,r",      po_switch(),            "Enable root mode")
        ("filter,f",    var<string>(""),        "Filter to restrict the selection of events (e.g. '[0]>10 && [0]<20';  does not work with stat and minmax yet)")
        ("overwrite",   po_switch(),            "Force overwriting an existing root file ('RECREATE')")
        ("update",      po_switch(),            "Update an existing root file ('UPDATE') or an existing tree with more data.")
        ("compression", var<uint16_t>(1),       "zlib compression level for the root file")
#endif
        ;

    po::positional_options_description p;
    p.add("fitsfile",  1); // The first positional options
    p.add("col",      -1); // All others

    conf.AddOptions(configs);
    conf.SetArgumentPositions(p);
}

int main(int argc, const char** argv)
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return -1;

    if (!conf.Has("fitsfile"))
    {
        cerr << "Filename required." << endl;
        return -1;
    }

    FitsDumper loader(conf.Get<string>("fitsfile"), conf.Get<string>("tablename"));
    if (!loader)
    {
        cerr << "ERROR - Opening " << conf.Get<string>("fitsfile");
        cerr << " failed: " << strerror(errno) << endl;
        return -1;
    }

    return loader.Exec(conf);
}
