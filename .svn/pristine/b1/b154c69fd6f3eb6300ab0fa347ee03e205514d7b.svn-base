#include <valarray>

#include <boost/filesystem.hpp>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "Connection.h"
#include "Configuration.h"
#include "Console.h"

#include "tools.h"

#include "zfits.h"

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace std;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"
#include "DimState.h"

// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Program options");
    control.add_options()
        ("target,t", var<string>()->required(),  "")
        ("event,e",  var<uint32_t>(), "")
        ;

    po::positional_options_description p;
    p.add("target", 1); // The first positional options
    p.add("event",  2); // The second positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

/*
 Extract usage clause(s) [if any] for SYNOPSIS.
 Translators: "Usage" and "or" here are patterns (regular expressions) which
 are used to match the usage synopsis in program output.  An example from cp
 (GNU coreutils) which contains both strings:
  Usage: cp [OPTION]... [-T] SOURCE DEST
    or:  cp [OPTION]... SOURCE... DIRECTORY
    or:  cp [OPTION]... -t DIRECTORY SOURCE...
 */
void PrintUsage()
{
    cout <<
        "Retrieve an event from a file in binary representation.\n"
        "\n"
        "Usage: getevent [-c type] [OPTIONS]\n"
        "  or:  getevent [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    //Main::PrintHelp<StateMachineEventServer>();

    /* Additional help text which is printed after the configuration
     options goes here */

    /*
     cout << "bla bla bla" << endl << endl;
     cout << endl;
     cout << "Environment:" << endl;
     cout << "environment" << endl;
     cout << endl;
     cout << "Examples:" << endl;
     cout << "test exam" << endl;
     cout << endl;
     cout << "Files:" << endl;
     cout << "files" << endl;
     cout << endl;
     */
}

/*
boost::filesystem::recursive_directory_iterator createRIterator(boost::filesystem::path path)
{
    try
    {
        return boost::filesystem::recursive_directory_iterator(path);
    }
    catch(boost::filesystem::filesystem_error& fex)
    {
        std::cout << fex.what() << std::endl;
        return boost::filesystem::recursive_directory_iterator();
    }
}

void dump(boost::filesystem::path path, int level)
{
    try
    {
        std::cout << (boost::filesystem::is_directory(path) ? 'D' : ' ') << ' ';
        std::cout << (boost::filesystem::is_symlink(path) ? 'L' : ' ') << ' ';

        for(int i = 0; i < level; ++i)
            std::cout << ' ';

        std::cout << path.filename() << std::endl;
    }
    catch(boost::filesystem::filesystem_error& fex)
    {
        std::cout << fex.what() << std::endl;
    }
}

void plainListTree(boost::filesystem::path path) // 1.
{
    dump(path, 0);
 
    boost::filesystem::recursive_directory_iterator it = createRIterator(path);
    boost::filesystem::recursive_directory_iterator end;
 
    while(it != end) // 2.
    {
        dump(*it, it.level()); // 3.
 
        if (boost::filesystem::is_directory(*it) && boost::filesystem::is_symlink(*it)) // 4.
            it.no_push();
 
        try
        {
            ++it; // 5.
        }
        catch(std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            it.no_push(); // 6.
            try { ++it; } catch(...) { std::cout << "!!" << std::endl; return; } // 7.
        }
    }
}
*/

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    const string name = conf.Get<string>("target");
/*
    if (!conf.Has("event"))
    {
        plainListTree(name);
        return 0;
    }
*/
    const uint32_t event = conf.Has("event") ? conf.Get<uint32_t>("event") : 0;

    zfits file(name);
    if (!file)
    {
        cerr << name << ": " <<  strerror(errno) << endl;
        return 1;
    }

    // Php can only read 32bit ints
    const uint32_t nRows = file.GetNumRows();

    const uint32_t nRoi = file.GetUInt("NROI");
    const uint32_t nPix = file.GetUInt("NPIX");

    const bool     isMC = file.HasKey("ISMC") && file.GetStr("ISMC")=="T";

    // Is drs calibration file?
    const int8_t step = file.HasKey("STEP") ? file.GetUInt("STEP") : -1;

    const string strbeg = isMC ? "DATE" : "RUN"+to_string(step)+"-BEG";
    const string strend = isMC ? "DATE" : "RUN"+to_string(step)+"-END";

    const double start = step==-1 && !isMC ? file.GetUInt("TSTARTI")+file.GetFloat("TSTARTF") : Time(file.GetStr(strbeg)).UnixDate();
    const double stop  = step==-1 && !isMC ? file.GetUInt("TSTOPI")+file.GetFloat("TSTOPF")   : Time(file.GetStr(strend)).UnixDate();

    const bool     isDrsCalib = file.HasKey("DRSCALIB") &&  file.GetStr("DRSCALIB")=="T";
    const int16_t  drsStep    = isDrsCalib ? file.GetUInt("DRSSTEP") : step;
    const string   runType    = step==-1 ? file.GetStr("RUNTYPE") : "";
    const uint16_t scale      = file.HasKey("SCALE") ? file.GetUInt("SCALE") : (step==-1?0:10);

    vector<char>     run(80);
    vector<int16_t>  data(nRoi*nPix);
    vector<float>    mean(nRoi*nPix);
    vector<uint32_t> unixTimeUTC(2);

    //uint32_t boardTime[40];
    uint32_t eventNum;
    //uint32_t numBoards;
    //uint16_t startCellData[1440];
    //uint16_t startCellTimeMarker[160];
    //uint32_t triggerNum;
    uint16_t triggerType;

    if (step==-1)
    {
        file.SetRefAddress("EventNum", eventNum);
        //file.SetRefAddress("TriggerNum", triggerNum);
        file.SetRefAddress("TriggerType", triggerType);
        if (!isMC)
            file.SetVecAddress("UnixTimeUTC", unixTimeUTC);
    }

    float energy, impact, phi, theta;
    if (isMC)
    {
        file.SetRefAddress("MMcEvtBasic.fEnergy", energy);
        file.SetRefAddress("MMcEvtBasic.fImpact", impact);
        file.SetRefAddress("MMcEvtBasic.fTelescopeTheta", theta);
        file.SetRefAddress("MMcEvtBasic.fTelescopePhi", phi);
    }

    switch (step)
    {
    case 0:  file.SetVecAddress("BaselineMean",      mean); strcpy( run.data(), "DRS (pedestal 1024)"); break;
    case 1:  file.SetVecAddress("GainMean",          mean); strcpy( run.data(), "DRS (gain)");          break;
    case 2:  file.SetVecAddress("TriggerOffsetMean", mean); strcpy( run.data(), "DRS (pedestal roi)");  break;
    default: file.SetVecAddress("Data",              data); strncpy(run.data(), runType.c_str(), 79);   break;
    }

    if (!file.GetRow(step==-1 ? event : 0))
        return 2;

    if (step!=-1)
        for (uint32_t i=0; i<nRoi*nPix; i++)
            data[i] = round(mean[i]*10);

    cout.write(run.data(),                80);
    cout.write((char*)&start,             sizeof(double));
    cout.write((char*)&stop,              sizeof(double));
    cout.write((char*)&drsStep,           sizeof(drsStep));
    cout.write((char*)&nRows,             sizeof(nRows));
    cout.write((char*)&scale,             sizeof(scale));

    cout.write((char*)&nRoi,              sizeof(nRoi));
    cout.write((char*)&nPix,              sizeof(nPix));
    cout.write((char*)&eventNum,          sizeof(eventNum));
    //cout.write((char*)&triggerNum,        sizeof(triggerNum));
    cout.write((char*)&triggerType,       sizeof(triggerType));
    cout.write((char*)unixTimeUTC.data(), sizeof(uint32_t)*2);
    cout.write((char*)data.data(),        sizeof(int16_t)*nRoi*nPix);

    return 0;
}
