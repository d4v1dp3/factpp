#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#include "tools.h"
#include "Time.h"
#include "Splitting.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TError.h>
#include <TObjArray.h>

using namespace std;
namespace fs = boost::filesystem;

// ------------------------------------------------------------------------


// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Root to SQL");
    control.add_options()
        ("file",           var<string>("-"),          "The csv input file. The default ('-') is reading from stdin.")
        ("out,o",          var<string>(""),           "Output root file name")
        ("force,f",        po_switch(),               "Force overwrite if output file already exists.")
        ("update,u",       po_switch(),               "Update an existing file")
        ("tree,t",         var<string>("Events"),     "Name of the root tree to convert")
        ("compression,c",  var<uint16_t>(1),          "zlib compression level for the root file")
        ("no-header,n",    po_switch(),               "Use if the first line contains no header")
        ("rename.*",       var<string>(),             "Can be used to rename a column")
        ("delimiter",      var<string>(";:, \t"),     "List of possible delimiters")
        ("null",           po_switch(),               "Enable detection of NULL and replace it with 0")
        ("empty",          po_switch(),               "Enable detection of empty fields (two immediately consecutive delimiters) and replace them with 0")
        ("dry-run",        po_switch(),               "Do not create or manipulate any output file")
        ("verbose,v",      var<uint16_t>(1),          "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    po::positional_options_description p;
    p.add("file", 1); // All positional options
    p.add("out",  1); // All positional options
    p.add("tree", 1); // All positional options

    conf.AddOptions(control);
    conf.AddOptions(Tools::Splitting::options());
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "csv2root - Converts a data table from a csv file to a root tree\n"
        "\n"
        "For convenience, this documentation uses the extended version of the options, "
        "refer to the output below to get the abbreviations.\n"
        "\n"
        "As a default, the first row in the file is considered to contain the column "
        "names separated by a whitespace. Column names must not contain whitespaces "
        "themselves and special characters (':') are replaces by an underscore. "
        "If the first line contains the first data row, the --no-header directive "
        "can be used to instruct the program to consider the first line as the first "
        "data row and use it only for column count. The branch names in the tree "
        "are then 'colN' where N is the column index starting from 0.\n"
        "\n"
        "Each consecutive row in the file is supposed to contain an identical number "
        "of floating point values. Leading and trailing whitespaces are ignored. "
        "Empty lines or lines starting with a '#' are discarded.\n"
        "\n"
        "Input and output file are given either as first and second positional argument "
        "or with the --file and --out command line option. If no output file name is "
        "provided then the input file is used instead and the extension replaced by .root. "
        "The target tree name of the root file is given with the --tree command line "
        "option or the third positional argument. The default tree name is 'Events'.\n"
        "\n"
        "As a default, existing files are not overwritten. If overwriting is intended, "
        "it can be turned on with --force. To update an existing root file, the "
        "--update option can be used. If a tree with the same name already exists, "
        "the tree is updated. The compression level for a new root file can be set "
        "with --compression.\n"
        "\n"
        "Columns can be renamed with --rename.new=old\n"
        "\n"
        << Tools::Splitting::usage() <<
        "\n"
        "In case of success, 0 is returned, a value>0 otherwise.\n"
        "\n"
        "Usage: csv2root input.csv [output.root] [-t tree] [-u] [-f] [-n] [-vN] [-cN]\n"
        "\n"
        ;
    cout << endl;
}

/*
void ErrorHandlerAll(Int_t level, Bool_t abort, const char *location, const char *msg)
{
    if (string(msg).substr(0,24)=="no dictionary for class ")
        return;
    if (string(msg).substr(0,15)=="unknown branch ")
        return;

    DefaultErrorHandler(level, abort, location, msg);
}*/


bool AddTree(vector<TTree*> &ttree, TFile &file, const string &tree, bool update, int verbose)
{
    bool found = false;

    TTree *T = 0;
    if (update)
    {
        file.GetObject(tree.c_str(), T);
        if (T)
        {
            ttree.emplace_back(T);
            found = true;
            if (verbose>0)
                cout << "Updating tree: " << tree << endl;
        }
    }
    if (!T)
        ttree.emplace_back(new TTree(tree.c_str(), "csv2root"));

    return found;
}

int main(int argc, const char* argv[])
{
    Time start;

    gROOT->SetBatch();
    //SetErrorHandler(ErrorHandlerAll);

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    // ----------------------------- Evaluate options --------------------------
    const string file            = conf.Get<string>("file");
    const string tree            = conf.Get<string>("tree");

    const bool force             = conf.Get<bool>("force");
    const bool update            = conf.Get<bool>("update");
    const bool detectnull        = conf.Get<bool>("null");
    const bool detectempty       = conf.Get<bool>("empty");
//    const bool dryrun            = conf.Get<bool>("dry-run");
    const bool noheader          = conf.Get<bool>("no-header");
    const string delimiter       = conf.Get<string>("delimiter");

    const uint16_t verbose       = conf.Get<uint16_t>("verbose");
//    const int64_t  first         = conf.Get<int64_t>("first");
//    const int64_t  max           = conf.Get<int64_t>("max");

    const uint16_t compression   = conf.Get<uint16_t>("compression");

    string out = conf.Get<string>("out");
    if (out.empty())
    {
        out = file.empty() || file=="-" ? "csv2root" : file;
        const auto p = out.find_last_of('.');
        if (p!=string::npos)
            out.erase(p);
        out += ".root";
    }

    // -------------------------------------------------------------------------

    /*const*/ Tools::Splitting split(conf);

    if (verbose>0)
    {
        cout << "\n-------------------------- Evaluating input ------------------------\n";
        cout << "Start Time: " << Time::sql << Time(Time::local) << endl;
    }


    // -------------------------------------------------------------------------

    cout << "Reading from '" << file << "'.\n";

    ifstream ifs(file.c_str());

    istream &fin = file=="-" ? cin : ifs;
    if (!fin.good())
    {
        cerr << file << ": " << strerror(errno) << endl;
        return 1;
    }

    TString buf;
    buf.ReadLine(fin);
    if (!fin)
    {
        cerr << file << ": " << strerror(errno) << endl;
        return 2;
    }

    buf = buf.Strip(TString::kBoth);
    TObjArray *title = buf.Tokenize(delimiter);
    if (title->GetEntries()==0)
    {
        cerr << "First line empty." << endl;
        return 3;
    }

    if (title->At(0)->GetName()[0]=='#')
    {
        title->RemoveAt(0);
        title->Compress();
    }

    const auto numcol = title->GetEntries();

    if (verbose>0)
        cout << "Found " << numcol << " columns." << endl;

    if (noheader)
    {
        fin.seekg(0);
        if (verbose>0)
            cout << "No header line interpreted." << endl;
    }

    // -------------------------------------------------------------------------

    TString path(out.c_str());
    gSystem->ExpandPathName(path);

//    if (!dryrun)
    {
        FileStat_t stat;
        const Int_t  exist  = !gSystem->GetPathInfo(path, stat);
        const Bool_t _write = !gSystem->AccessPathName(path,  kWritePermission) && R_ISREG(stat.fMode);

        if ((update && !exist) || (update && exist && !_write) || (force && exist && !_write))
        {
            cerr << "File '" << path << "' is not writable." << endl;
            return 4;
        }

        if (!update && !force && exist)
        {
            cerr << "File '" << path << "' already exists." << endl;
            return 5;
        }
    }

    TFile tfile(path, update?"UPDATE":(force?"RECREATE":"CREATE"), file.c_str(), compression);
    if (tfile.IsZombie())
    {
        cerr << "Opening file '" << path << "' failed." << endl;
        return 6;
    }

    if (verbose>0)
    {
        cout << "Opened root file '" << path << "'.\n";
        cout << "Writing to tree: " << tree << ".\n";
        split.print();
    }

    // -------------------- Configure branches of TTree ------------------------
    vector<TTree*> ttree;

    size_t entries = 0;
    if (split.empty())
    {
        if (AddTree(ttree, tfile, tree, update, verbose))
        {
            entries = ttree[0]->GetEntries();
            if (verbose>0)
                cout << "Tree has " << entries << " entries." << endl;
        }
    }
    else
    {
        bool found = false;
        for (size_t i=0; i<split.size(); i++)
            found |= AddTree(ttree, tfile, tree+"["+to_string(i)+"]", update, verbose);

        if (found && update)
        {
            cerr << "Trees can not be updated in split mode, only files!" << endl;
            return 7;
        }
    }
    const auto rename = conf.GetWildcardOptions("rename.*");

    vector<float> vec(numcol);
    for (int i=0; i<numcol; i++)
    {
        string col = noheader ? Tools::Form("col%d", i) : title->At(i)->GetName();

        if (verbose>1)
            cout << "Column: " << col;

        boost::regex rexpr(":");
        col = boost::regex_replace(col, rexpr, "");

        if (col[0]=='.')
            col.erase(0, 1);

        if (verbose>1)
            cout << " -> " << col;

        for (auto it=rename.cbegin(); it!=rename.cend(); it++)
        {
            if (col!=it->substr(7))
                continue;

            col = conf.Get<string>(*it);
            if (verbose>1)
                cout << " -> " << col;
            break;
        }
        if (verbose>1)
            cout << endl;

        for (auto it=ttree.begin(); it!=ttree.end(); it++)
            it[0]->Branch(col.c_str(), vec.data()+i);
    }

    delete title;

    // -------------------------------------------------------------------------

    size_t line = 0;
    size_t valid = 0;

    while (1)
    {
        buf.ReadLine(fin);
        if (!fin)
            break;

        line++;

        buf = buf.Strip(TString::kBoth);
        if (buf.IsNull() || buf[0]=='#')
            continue;

        if (detectempty)
        {
            string delim = delimiter;


            for (size_t i=0; i<delimiter.size(); i++)
                if (delimiter[i]!=' ')
                {
                    boost::regex rexpr1("(["+delimiter+"])(["+delimiter+"])");
                    buf = boost::regex_replace(string(buf.Data()), rexpr1, "\\10\\2");

                    boost::regex rexpr2("^(["+delimiter+"])");
                    buf = boost::regex_replace(string(buf.Data()), rexpr2, "0\\1");

                    boost::regex rexpr3("(["+delimiter+"])$");
                    buf = boost::regex_replace(string(buf.Data()), rexpr3, "\\10");
                }
        }

        TObjArray *arr = buf.Tokenize(delimiter);
        if (arr->GetEntries()!=numcol)
        {
            cerr << buf << endl;
            cerr << "Column count [" << arr->GetEntries() << "] mismatch in line " << line+1 << "!" << endl;
            return 7;
        }

        for (int i=0; i<numcol; i++)
        {
            try
            {
                if (detectnull && arr->At(i)->GetName()==string("NULL"))
                {
                    vec[i] = 0;
                    continue;
                }

                vec[i] = stof(arr->At(i)->GetName());
            }
            catch (const exception &e)
            {
                cerr << buf << endl;
                cerr << "Conversion of field " << i << " '" << arr->At(i)->GetName() << "' in line " << line+1 <<  " failed!" << endl;
                return 8;
            }
        }

        delete arr;

        const size_t index = split.index(valid++);

        // Fill only branches for which an adress was set
        // If we fill the tree, we get empty entries at the
        // end of the already written branches
        TIter NextBranch(ttree[index]->GetListOfBranches());
        TBranch *b=0;
        while ((b=static_cast<TBranch*>(NextBranch())))
            if (b->GetAddress())
                b->Fill();
    }

    for (auto it=ttree.begin(); it!=ttree.end(); it++)
    {
        TIter NextBranch((*it)->GetListOfBranches());
        TBranch *b=0;
        while ((b=static_cast<TBranch*>(NextBranch())))
            if (b->GetAddress() && b->GetEntries()>0)
            {
                (*it)->SetEntries(b->GetEntries());
                break;
            }

        (*it)->Write("", TObject::kOverwrite);
    }

    if (verbose>0)
    {
        cout << valid << " data rows found in " << line << " lines (excl. title)." << endl;
        for (size_t i=0; i<ttree.size(); i++)
            cout << ttree[i]->GetEntries() << " rows filled into tree #" << i << "." << endl;
    }

    if (entries && entries!=line)
        cerr << "\nWARNING - Number of updated entries does not match number of entries in tree!\n" << endl;

    tfile.Close();

    if (verbose>0)
    {
        const auto sec = Time().UnixTime()-start.UnixTime();

        cout << Tools::Scientific(tfile.GetSize()) << "B written to disk.\n";
        cout << "File closed.\n";
        cout << "Execution time: " << sec << "s ";
        cout << "(" << Tools::Fractional(sec/line) << "s/row)\n";
        cout << "--------------------------------------------------------------" << endl;
    }

    return 0;
}
