#include "DataProcessorImp.h"

#include <boost/filesystem.hpp>

#include "HeadersFAD.h"
#include "EventBuilder.h"
#include "tools.h"

using namespace std;


// --------------------------------------------------------------------------
//
//! This creates an appropriate file name for a particular run number and type
//! @param runNumberq the run number for which a filename is to be created
//! @param runType an int describing the kind of run. 0=Data, 1=Pedestal, 2=Calibration, 3=Calibrated data
//! @param extension a string containing the extension to be appened to the file name
//
string DataProcessorImp::FormFileName(const string &path, uint64_t night, uint32_t runid, const string &extension)
{
    ostringstream name;

    if (!path.empty())
    {
        name << path;
        if (path[path.length()-1] != '/')
            name << '/';
    }

    name << Tools::Form("%04d/%02d/%02d/", night/10000, (night/100)%100, night%100);

    try
    {
        boost::filesystem::create_directories(name.str());
    }
    catch (const runtime_error &)
    {
        // File creation will fail anyway
        //Error(e.what());
    }

    name << night << '_' << setfill('0') << setw(3) << runid << '.' << extension;
    return name.str();
}

// =======================================================================

bool DataDump::Open(const RUN_HEAD &h, const FAD::RunDescription &d)
{
    fFileName = "/dev/null";

    ostringstream str;
    str << this << " - "
        << "OPEN_FILE #" << GetRunId() << ":"
        << " Ver=" << h.Version
        << " Nb="  << h.NBoard
        << " Np="  << h.NPix
        << " NTm=" << h.NTm
        << " roi=" << h.Nroi
        << " Typ=" << d.name;

    Debug(str);

    fTime = Time();

    return true;
}

bool DataDump::WriteEvt(const EVT_CTRL2 &e)
{
    const Time now;
    if (now-fTime<boost::posix_time::seconds(5))
        return true;

    fTime = now;

    ostringstream str;
    str << this << " - EVENT #" << e.evNum << " / " << e.trgNum;
    Debug(str);

    return true;
}

bool DataDump::Close(const EVT_CTRL2 &)
{
    ostringstream str;
    str << this << " - CLOSE FILE #" << GetRunId();

    Debug(str);

    return true;
}

// =======================================================================

bool DataDebug::WriteEvt(const EVT_CTRL2 &e)
{
    cout << "WRITE_EVENT #" << GetRunId() << " (" << e.evNum << ")" << endl;
    cout << " Typ=" << e.trgTyp << endl;
    cout << " roi=" << e.nRoi << endl;
    cout << " tim=" << e.time.tv_sec << endl;

    return true;
}

