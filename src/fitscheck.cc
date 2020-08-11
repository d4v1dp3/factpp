//****************************************************************
/** 

 */
 //****************************************************************
#include "Configuration.h"

#include "fits.h"

using namespace std;


void PrintUsage()
{
    cout <<
        "fitscheck is a tool to verify the checksums in a fits file.\n"
        "\n"
        "Usage: fitscheck [OPTIONS] fitsfile\n"
        //"  or:  fitscheck [OPTIONS]\n"
        "\n"
        "Return values:\n"
        " 0:  in case of success\n"
        " 1:  if the file could not be opened\n"
        " 2:  if the header checksum could not be varified and\n"
        " 3:  if the header checksum is ok but the data checksum could not be verified.\n"
        "\n";
    cout << endl;
}

void PrintHelp()
{
}

void SetupConfiguration(Configuration& conf)
{
    po::options_description configs("Fitscheck options");
    configs.add_options()
        ("fitsfile,f",  var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
                                     , "Name of FITS file")
        ;

    po::positional_options_description p;
    p.add("fitsfile", 1); // The first positional options

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

    const string fname = conf.Get<string>("fitsfile");

    cout << "Reading '" << fname << "'.." << flush;

    fits file(fname.c_str());
    if (!file)
    {
        cout << "fits::open() failed: " << strerror(errno) << " [errno=" << errno << "]";
        return 1;
    }

    if (!file.IsHeaderOk())
    {
        cout << " header checksum could not be verified." << endl;
        return 2;
    }

    const size_t n = file.GetNumRows()/10;

    while (file.GetNextRow())
        if (file.GetRow()<n && file.GetRow()%n==0)
            cout << '.' << flush;

    if (!file.IsFileOk())
    {
        cout << " data checksum could not be verified." << endl;
        return 3;
    }

    cout << " file ok." << endl;

    return 0;
}
