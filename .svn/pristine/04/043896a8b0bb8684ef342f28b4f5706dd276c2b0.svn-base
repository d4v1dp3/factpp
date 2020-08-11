#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#include "tools.h"
#include "Time.h"
#include "Splitting.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TChain.h>
#include <TLeaf.h>
#include <TError.h>
#include <TTreeFormula.h>
#include <TTreeFormulaManager.h>

#include "FileEntry.h"

using namespace std;
namespace fs = boost::filesystem;

// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Root to SQL");
    control.add_options()
        ("file",           vars<string>()->required(),"The root files to read from")
        ("out,o",          var<string>()->required(), "Output file name")
        ("force,f",        po_switch(),               "Force overwrite if output file already exists.")
        ("append,a",       po_switch(),               "Append to an existing file (not check for the format is done!)")
        ("tree,t",         var<string>("Events"),     "Name of the root tree to convert")
        ("ignore",         vars<string>(),            "Ignore the given leaf, if the given regular expression matches")
        ("alias.*",        var<string>(),             "Define an alias")
        ("auto-alias",     vars<Configuration::Map>(),"Regular expression to define aliases from the branch names automatically")
        ("header",         var<uint16_t>(uint16_t(0)),"Type of header line (0: preceeding #, 1: without preceeding #, 2: none)")
        ("add.*",          var<string>(),             "Define an additional column")
        ("selector,s",     var<string>("1"),          "Define a selector for the columns (colums where this evaluates to a value <=0 are discarded)")
        ("skip",           po_switch(),               "Discards all default leaves and writes only the columns defined by --add.*")
        ("first",          var<int64_t>(int64_t(0)),  "First event to start with (default: 0), mainly for test purpose")
        ("max",            var<int64_t>(int64_t(0)),  "Maximum number of events to process (0: all), mainly for test purpose")
        //("const.*",        var<string>(),             "Insert a constant number into the given column (--const.mycolumn=5). A special case is `/.../.../`")
        ("dry-run",        po_switch(),               "Do not create or manipulate any output file")
        ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("print-ls",       po_switch(),               "Calls TFile::ls()")
        ("print-branches", po_switch(),               "Print the branches found in the tree")
        ("print-leaves",   po_switch(),               "Print the leaves found in the tree (this is what is processed)")
        ("verbose,v",      var<uint16_t>(1),          "Verbosity (0: quiet, 1: default, 2: more, 3, ...)")
        ;

    po::positional_options_description p;
    p.add("file", -1); // All positional options

    conf.AddOptions(control);
    conf.AddOptions(Tools::Splitting::options());
    conf.AddOptions(debug);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "root2csv - Reads data from a root tree and writes a csv file\n"
        "\n"
        "For convenience, this documentation uses the extended version of the options, "
        "refer to the output below to get the abbreviations.\n"
        "\n"
        "Similar functionaliy is also provided by root2sql. In addition to root2sql, "
        "this tool is more flexible in the slection of columns and adds the possibility "
        "to use formulas (implemented through TTreeFormula) to calculate values for "
        "additional columns. Note that root can even write complex data like a TH1F "
        "into a file. Here, only numeric columns are supported.\n"
        "\n"
        "Input files are given as positional arguments or with --file. "
        "As files are read by adding them through TChain::Add, wildcards are "
        "supported in file names. Note that on the command lines, file names "
        "with wildcards have to be escaped in quotation marks if the wildcards "
        "should be evaluated by the program and not by the shell. The output base "
        "name of the output file(s) is given with --out.\n"
        "\n"
        "The format of the first line on the file is defined with the --header option:\n"
        "   0: '# Col1 Col2 Col3 ...'\n"
        "   1: 'Col1 Col2 Col3 ...'\n"
        "   2: first data row\n"
        "\n"
        "As default, existing files are not overwritten. To force overwriting use "
        "--force. To append data to existing files use --append. Note that no "
        "check is done if this created valid and reasonable files.\n"
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
        "A --print-branches exists for convenience to print only the high-level branches.\n"
        "\n"
        "Assuming a leaf with name MHillas.fWidth and a leaf with MHillas.fLength, "
        "a new column can be added with name Area by\n"
        "   --add.Area='TMath::TwoPi()*MHillas.fWidth*MHillas.fLength'\n"
        "\n"
        "To simplify expression, root allows to define aliases, for example\n"
        "   --alias.Width='MHillas.fWidth'\n"
        "   --alias.Length='MHillas.fLength'\n"
        "\n"
        "This can then be used to simplyfy the above expression as\n"
        "   --add.Area='TMath::TwoPi()*Width*Length'\n"
        "\n"
        "Details on interpretation can be found in the documentation of TFormula::Analyze. "
        "Random numbers are supported by rndm().\n"
        "\n"
#if ROOT_VERSION_CODE < ROOT_VERSION(6,18,00)
        "Note that functions which require two arguments (e.g. atan2) can be used in "
        "a column (i.e. with --add) but can not be references in an alias.\n"
        "\n"
#endif
        "Sometimes leaf names might be quite unconvenient like MTime.fTime.fMilliSec or "
        "just MHillas.fWidth. To allow to simplify column names, regular expressions "
        "(using boost's regex) can be defined to change the names. Note that these regular "
        "expressions are applied one by one on each leaf's name. A valid expression could "
        "be:\n"
        "   --auto-alias=MHillas\\.f/\n"
        "which would remove all occurances of 'MHillas.f'. This option can be used more than "
        "once. They are applied in sequence. A single match does not stop the sequence. "
        "In addition to replacing the column names accordingly, a alias is created "
        "automatically allowing to access the columns in a formula with the new name.\n"
        "\n"
        "Sometimes it might also be convenient to skip a leaf, i.e. not writing the "
        "coresponding column in the output file. This can be done with "
        "the --ignore resource. If the given regular expresion yields a match, the "
        "leaf will be ignored. An automatic alias would still be created and the "
        "leaf could still be used in a formula. Example\n"
        "   --ignore=ThetaSq\\..*\n"
        "will skip all leaved which start with 'ThetaSq.'. This directive can be given "
        "more than once. The so defined ignore list is applied entry-wise, first to the "
        "raw leaf names, then to the aliased names.\n"
        "\n"
        "To select only certain extries from the file, a selector (cut) can be defined "
        "in the same style as the --add directives, for exmple:\n"
        "   --selector='MHillas.fLength*Width<0'\n"
        "Note that the selctor is not evaluated to a boolean expression (==0 or !=0) "
        "but all positive none zero values are considered 'true' (select the entry) "
        "and all negative values are considered 'fales' (discard the entry).\n"
        "\n"
        << Tools::Splitting::usage() <<
        "\n"
        "In case of success, 0 is returned, a value>0 otherwise.\n"
        "\n"
        "Usage: root2csv input1.root [input2.root ...] -o output.csv [-t tree] [-u] [-f] [-n] [-vN] [-cN]\n"
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

// --------------------------- Write Header --------------------------------
void WriteHeader(ostream &out, const vector<FileEntry::Container> &vec, const vector<TTreeFormula*> &form, bool skip, uint16_t header)
{
    if (header>1)
        return;
    if (header==0)
        out << "# ";

    vector<string> join;

    if (!skip)
    {
        for (auto v=vec.cbegin(); v!=vec.cend(); v++)
        {
            const size_t N = v->num;
            for (size_t i=0; i<N; i++)
            {
                string name = v->column;
                if (N!=1)
                    name += "["+to_string(i)+"]";
                join.emplace_back(name);
            }
        }
    }

    for (auto v=form.cbegin(); v!=form.cend(); v++)
        join.emplace_back((*v)->GetName());

    out << boost::join(join, " ") << "\n";
}

int CheckFile(TString &path, bool force, int verbose)
{
    gSystem->ExpandPathName(path);

    FileStat_t stat;
    const Int_t  exist  = !gSystem->GetPathInfo(path, stat);
    const Bool_t _write = !gSystem->AccessPathName(path,  kWritePermission) && R_ISREG(stat.fMode);

    if (exist)
    {
        if (!_write)
        {
            cerr << "File '" << path << "' is not writable." << endl;
            return 8;
        }

        if (!force)
        {
            cerr << "File '" << path << "' already exists." << endl;
            return 9;
        }

        if (verbose>0)
            cerr << "File '" << path << "' will be overwritten." << endl;
    }
    return exist ? 0 : -1;
}

class Formula : public TTreeFormula
{
public:
    void CollectLeaves(set<string> &list, Formula *formula)
    {
        if (!formula)
            return;

        TObject *o = 0;

        TIter NextN(&formula->fLeafNames);
        while ((o=NextN()))
            list.emplace(o->GetName());

        TIter NextF(&formula->fAliases);
        while ((o=NextF()))
            CollectLeaves(list, static_cast<Formula*>(o));
    }

    void CollectLeaves(set<string> &list)
    {
        CollectLeaves(list, this);
    }

    Formula(const char* name, const char* formula, TTree* tree)
        : TTreeFormula(name, formula, tree)
    {
    }
};

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
    const vector<string> files   = conf.Vec<string>("file");
    const string out             = conf.Get<string>("out");
    const string tree            = conf.Get<string>("tree");

    const bool force             = conf.Get<bool>("force");
    const bool append            = conf.Get<bool>("append");
    const bool dryrun            = conf.Get<bool>("dry-run");
    const bool skip              = conf.Get<bool>("skip");

    const uint16_t verbose       = conf.Get<uint16_t>("verbose");
    const int64_t  first         = conf.Get<int64_t>("first");
    const int64_t  max           = conf.Get<int64_t>("max");
    const uint16_t header        = conf.Get<uint16_t>("header");

    const bool print_ls          = conf.Get<bool>("print-ls");
    const bool print_branches    = conf.Get<bool>("print-branches");
    const bool print_leaves      = conf.Get<bool>("print-leaves");

    const auto  _ignore   = conf.Vec<string>("ignore");
    const auto  autoalias = conf.Vec<Configuration::Map>("auto-alias");

    if (max && first>=max)
        cerr << "WARNING: Resource `first` (" << first << ") exceeds `max` (" << max << ")" << endl;

    // -------------------------------------------------------------------------

    /*const*/ Tools::Splitting split(conf);

    if (verbose>0)
    {
        cout << "\n-------------------------- Evaluating input ------------------------\n";
        cout << "Start Time: " << Time::sql << Time(Time::local) << endl;
    }

    if (verbose>0)
        cout << "Processing Tree: " << tree << endl;

    TChain c(tree.c_str());

    uint64_t cnt = 0;
    for (const auto &file : files)
    {
        const auto add = c.Add(file.c_str(), 0);
        if (verbose>0)
            cout << file << ": " << add << " file(s) added." << endl;
        cnt += add;
    }

    if (cnt==0)
    {
        cerr << "No files found." << endl;
        return 1;
    }

    if (verbose>0)
        cout << cnt << " file(s) found." << endl;

    if (print_ls)
    {
        cout << '\n';
        c.ls();
        cout << '\n';
    }

    c.SetMakeClass(1);

    TObjArray *branches = c.GetListOfBranches();
    TObjArray *leaves   = c.GetListOfLeaves();

    if (print_branches)
    {
        cout << '\n';
        branches->Print();
    }

    const auto entries = c.GetEntriesFast();

    if (verbose>0)
        cout << branches->GetEntries() << " branches found." << endl;

    if (print_leaves)
    {
        cout << '\n';
        leaves->Print();
    }
    if (verbose>0)
    {
        cout << leaves->GetEntries() << " leaves found." << endl;
        cout << entries << " events found." << endl;
    }

    // ----------------------------------------------------------------------

    if (verbose>0)
        cout << "\n-------------------------- Evaluating output -----------------------" << endl;

    vector<FileEntry::Container> vec;

/*
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
    */

    set<string> leaflist;

    // ------------------------- Setup all branches in tree -------------------

    TIter Next(leaves);
    TObject *o = 0;
    while ((o=Next()))
    {
        TLeaf *L = dynamic_cast<TLeaf*>(o);//c.GetLeaf(o->GetName());
        if (!L)
            continue;

        string name = L->GetName();
        if (verbose>2)
            cout << '\n' << L->GetTitle() << " {" << L->GetTypeName() << "}";

        const string tn = L->GetTypeName();

        // Check if this is a basic type, otherwise skip
        const auto it = FileEntry::LUT.root(tn);
        if (it==FileEntry::LUT.cend())
        {
            if (verbose>2)
                cout << " (-n/a-)";
            continue;
        }

        // Check if this is an array, otherwise skip
        if (L->GetLenStatic()!=L->GetLen())
        {
            if (verbose>2)
                cout << " (-skipped-)";
            continue;
        }

        // Check for renaming via auto-alias
        for (auto m=autoalias.cbegin(); m!=autoalias.cend(); m++)
            name = boost::regex_replace(name, boost::regex(m->first), m->second);

        // if renaming has changed name, define alias
        if (name!=L->GetName())
        {
            if (!c.SetAlias(name.c_str(), L->GetName()))
            {
                cerr << "\nERROR - Alias could not be established!\n";
                cerr << "   " << name << " = " << L->GetName() << endl;
                return 1;
            }
            if (verbose==1 || verbose==2)
                cout << "\nAuto-alias: " << name << " = " << L->GetName();
            if (verbose>2)
                cout << " <alias:" << name << ">";
        }

        // Check whether the un-aliased column is in the ignore list
        bool found = skip;
        for (auto b=_ignore.cbegin(); b!=_ignore.cend(); b++)
        {
            if (boost::regex_match(L->GetName(), boost::regex(*b)))
            {
                found = true;
                break;
            }
        }

        // Check whether the aliased column is in the ignore list
        for (auto b=_ignore.cbegin(); b!=_ignore.cend(); b++)
        {
            if (boost::regex_match(name.c_str(), boost::regex(*b)))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            if (verbose>2)
                cout << " (-ignored-)";
            continue;
        }

        vec.emplace_back(L->GetName(), name, it->type, L->GetLenStatic());
        c.SetBranchAddress(L->GetName(), vec.back().ptr);
    }

    if (verbose>0)
    {
        cout << '\n';
        if (skip)
            cout << "Default columns skipped: ";
        cout << vec.size() << " default leaf/leaves setup for reading." << endl;
    }

    // ------------------- Configure manual aliases ----------------------------

    const auto valiases = conf.GetWildcardOptions("alias.*");
    if (verbose>0 && valiases.size()>0)
        cout << '\n';
    for (auto it=valiases.cbegin(); it!=valiases.cend(); it++)
    {
        const string name = it->substr(6);
        const string val  = conf.Get<string>(*it);

#if ROOT_VERSION_CODE < ROOT_VERSION(6,18,00)
        if (val.find_first_of(',')!=string::npos)
        {
            cerr << "\nERROR - Alias contains comma: Functions with two arguments";
            cerr << "\ndo not work as alias with root versions less than v6.18!\n";
            cerr << "   " << name << " = " << val << endl;
            return 2;
        }
#endif

        if (verbose>0)
            cout << "Alias: " << name << " = " << val << endl;

        if (!c.SetAlias(name.c_str(), val.c_str()))
        {
            cerr << "\nERROR - Alias could not be established!\n";
            cerr << "   " << name << " = " << val << endl;
            return 3;
        }
    }

    // -------------------------- Configure Selector --------------------------

    TTreeFormulaManager *manager = new TTreeFormulaManager;

    if (verbose>0)
        cout << "\nSelector: " <<  conf.Get<string>("selector") << endl;

    Formula selector("Selector", conf.Get<string>("selector").c_str(), &c);
    if (selector.GetNdim()==0)
    {
        cerr << "Compilation of Selector failed!" << endl;
        return 4;
    }
    selector.SetQuickLoad(kTRUE);
    selector.CollectLeaves(leaflist);
    manager->Add(&selector);

    // -------------------- Configure additional columns ----------------------

    vector<TTreeFormula*> formulas;

    const auto vform = conf.GetWildcardOptions("add.*");
    if (verbose>0 && vform.size()>0)
        cout << '\n';
    for (auto it=vform.cbegin(); it!=vform.cend(); it++)
    {
        const string name = it->substr(4);
        const string val  = conf.Get<string>(*it);

        if (verbose>0)
            cout << "Adding column: " << name << " = " << val << endl;

        Formula *form = new Formula(name.c_str(), val.c_str(), &c);
        if (form->GetNdim()==0)
        {
            cerr << "Compilation of Column failed!" << endl;
            return 5;
        }
        form->SetQuickLoad(kTRUE);
        form->CollectLeaves(leaflist);
        formulas.emplace_back(form);
        manager->Add(form);
    }
    manager->Sync();

    // ------- Set a branch address for all leaves referenced by formulas -------

    for (auto ileaf=leaflist.cbegin(); ileaf!=leaflist.cend(); ileaf++)
    {
        // Get Leaf
        TLeaf *L = c.GetLeaf(ileaf->c_str());
        if (!L)
            continue;

        // Adress already set
        if (L->GetBranch()->GetAddress())
            continue;

        if (verbose>2)
            cout << '\n' << L->GetName() << " {" << L->GetTypeName() << "}";

        const string tn = L->GetTypeName();

        // Check if this is a basic type, otherwise skip
        const auto it = FileEntry::LUT.root(tn);
        if (it==FileEntry::LUT.cend())
        {
            cerr << "Failed to enable branch '" << L->GetName() << "' (type unknown)" << endl;
            return 6;
        }

        // Check if this is an array, otherwise skip
        if (L->GetLenStatic()!=L->GetLen())
        {
            cerr << "Failed to enable branch '" << L->GetName() << "' (strange array size)" << endl;
            return 7;
        }

        vec.emplace_back(L->GetName(), L->GetName(), it->type, L->GetLenStatic());
        c.SetBranchAddress(L->GetName(), vec.back().ptr);
    }

    if (verbose>0)
        cout << '\n' << formulas.size() << " additional columns setup for writing." << endl;

    // ------------------------- Enable branch reading ------------------------

    UInt_t datatype = 0;
    const bool has_datatype = c.SetBranchAddress("DataType.fVal", &datatype) >= 0;
    if (has_datatype && verbose>0)
        cout << "\nRows with DataType.fVal!=1 will be skipped." << endl;


    // Simply switch on all bracnhs for which an address was set
    c.SetBranchStatus("*", 0);

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

        c.SetBranchStatus(B->GetName(), 1);
        if (verbose>2)
            cout << "\nEnable Branch: " << B->GetName();
    }
    if (verbose>2)
        cout << endl;

    // -------------------------------------------------------------------------

    if (verbose>0)
    {
        cout << '\n';
        split.print();
    }

    if (dryrun)
    {
        cout << "\nDry run: file output skipped!" << endl;
        return 0;
    }

    if (verbose>0)
        cout << "\n-------------------------- Converting file -------------------------" << endl;

    vector<ofstream> outfiles;

    if (split.empty())
    {
        TString path(out.c_str());
        const int rc = CheckFile(path, force, verbose);
        if (rc>0)
            return rc;

        outfiles.emplace_back(path.Data(), append ? ios::app : ios::trunc);
        if (rc==-1 || (force && rc==0 && !append))
            WriteHeader(outfiles.back(), vec, formulas, skip, header);
    }
    else
    {
        for (size_t i=0; i<split.size(); i++)
        {
            TString path(out.c_str());
            path += "-";
            path += i;

            const int rc = CheckFile(path, force, verbose);
            if (rc>0)
                return rc;
            outfiles.emplace_back(path.Data(), append ? ios::app : ios::trunc);
            if (rc==-1 || (force && rc==0 && !append))
                WriteHeader(outfiles.back(), vec, formulas, skip, header);
        }
    }

    // ---------------------------- Write Body --------------------------------
    size_t count = 0;
    vector<size_t> ncount(split.empty()?1:split.size());

    auto itree = c.GetTreeNumber();

    const size_t num = max>0 && (max-first)<entries ? (max-first) : entries;
    for (size_t j=first; j<num; j++)
    {
        c.GetEntry(j);
        if (has_datatype && datatype!=1)
            continue;

        if (itree != c.GetTreeNumber())
        {
            manager->UpdateFormulaLeaves();
            itree = c.GetTreeNumber();
        }

        if (selector.GetNdim() && selector.EvalInstance(0)<=0)
            continue;

        const size_t index = split.index(count++);
        ncount[index]++;

        vector<string> join;

        if (!skip)
        {
            for (auto v=vec.cbegin(); v!=vec.cend(); v++)
            {
                const size_t N = v->num;
                for (size_t i=0; i<N; i++)
                    join.emplace_back(v->fmt(i));
            }
        }

        for (auto v=formulas.cbegin(); v!=formulas.cend(); v++)
            join.emplace_back(to_string((*v)->EvalInstance(0)));

        outfiles[index] << boost::join(join, " ") << "\n";
    }

    if (verbose>0)
    {
        cout << "\nTotal: N=" << count << " out of " << num << " row(s) written [N=" << first << ".." << num-1 << "]." << endl;
        for (int i=0; i<split.size(); i++)
            cout << "File " << i << ": nrows=" << ncount[i] << '\n';
        cout << '\n';
    }

    if (verbose>0)
    {
        cout << "Total execution time: " << Time().UnixTime()-start.UnixTime() << "s.\n";
        cout << "Success!\n" << endl;
    }
    return 0;
}
