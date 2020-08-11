#include "Configuration.h"

#include "factfits.h"
#include "factofits.h"

using namespace std;

void PrintUsage()
{
    cout <<
        "fitsselect is....\n"
        "\n"
        "Usage: fitsselect rawfile eventlist outfile\n"
        "\n"
        ;
    cout << endl;
}

void SetupConfiguration(Configuration& conf)
{
    po::options_description configs("Fitsdump options");
    configs.add_options()
        ("infile",    var<string>()->required(), "")
        ("outfile",   var<string>()->required(), "")
        ("eventlist", var<string>()->required(), "")
        ;

    po::positional_options_description p;
    p.add("infile",    1); // The first positional options
    p.add("eventlist", 1); // The second positional options
    p.add("outfile",  -1); // All others

    conf.AddOptions(configs);
    conf.SetArgumentPositions(p);
}

int main(int argc, const char** argv)
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))//, PrintHelp))
        return -1;

    const string infile = conf.Get<string>("infile");

    const string outfile   = conf.Get<string>("outfile");
    const string eventlist = conf.Get<string>("eventlist");

    set<uint32_t> list;
    ifstream fin(eventlist.c_str());
    while (1)
    {
        uint32_t evt;
        fin >> evt;
        if (!fin)
            break;

        list.insert(evt);
    }

    factfits  inf(infile.c_str(), "Events", false);
    factofits outf(outfile.c_str());

    outf.SetDrsCalibration(inf.GetOffsetCalibration());

    uint32_t rowWidth = inf.HasKey("ZNAXIS1") ? inf.GetUInt("ZNAXIS1") : inf.GetUInt("ZNAXIS2");

    vector<char> buffer(rowWidth);

    outf.CopyKeys(inf);

    vector<pair<void*, char*>> pointers;

    unsigned int count = 0;

    uint32_t *evtNum = 0;

    const fits::Table::Columns& columns = inf.GetColumns();
    for (fits::Table::Columns::const_iterator it=columns.begin(); it!=columns.end(); it++)
    {
        const fits::Table::Column &col = it->second;

        if (it->first=="Data" || it->first=="TimeMarker")
        {
            vector<uint16_t> processing(2);
            processing[0] = FITS::kFactSmoothing;
            processing[1] = FITS::kFactHuffman16;

            const FITS::Compression comp(processing, FITS::kOrderByRow);

            outf.AddColumn(comp, col.num, col.type, it->first, col.unit, "");
        }
        else
            outf.AddColumn(col.num, col.type, it->first, col.unit, "");

        void *ptr = inf.SetPtrAddress(it->first);
        pointers.emplace_back(ptr, buffer.data()+count);
        count += col.num*col.size;

        if (it->first=="EventNum")
            evtNum = reinterpret_cast<uint32_t*>(ptr);
    }

    if (evtNum==0)
        throw runtime_error("Colum EventNum not found.");

    if (count!=rowWidth)
        throw runtime_error("Size mismatch.");

    inf.PrintColumns();

    outf.WriteTableHeader(inf.GetStr("EXTNAME").c_str());

    while (inf.GetNextRow())
    {
        if (list.find(*evtNum)==list.end())
            continue;

        int i=0;
        for (fits::Table::Columns::const_iterator it=columns.begin(); it!= columns.end(); it++, i++)
            memcpy(pointers[i].second, pointers[i].first, it->second.num*it->second.size);

        outf.WriteRow(buffer.data(), rowWidth);
        if (!outf)
            throw runtime_error("Write stream failure.");
    }

    if (!inf.good())
        throw runtime_error("Read stream failure.");

    if (!outf.close())
        throw runtime_error("Write stream failure.");

    return 0;
}
