#include "DataWriteFits2.h"

#include <boost/filesystem.hpp>

#include "HeadersFAD.h"
#include "EventBuilder.h"

#include "externals/factofits.h"
#include "externals/DrsCalib.h"

#ifdef HAVE_NOVA
#include "externals/nova.h"
#endif

using namespace std;

DataWriteFits2::DataWriteFits2(const std::string &path, uint64_t night, uint32_t runid, MessageImp &imp)
    : DataProcessorImp(path, night, runid, imp)
{
    fFile = std::make_shared<ofits>();
}

DataWriteFits2::DataWriteFits2(const std::string &path, uint64_t night, uint32_t runid, const DrsCalibration &cal, MessageImp &imp)
    : DataProcessorImp(path, night, runid, imp)
{
    factofits *file = new factofits;
    file->SetDrsCalibration(cal);
    fFile = std::shared_ptr<ofits>(file);
}

void DataWriteFits2::WriteHeader(const RUN_HEAD &h, const FAD::RunDescription &d)
{
    const int16_t realRoiTM = (h.NroiTM >= 2*h.Nroi && h.Nroi<=512) ? h.Nroi : 0;

    fFile->AddColumnInt("EventNum", "uint32", "FAD board event counter");
    fFile->AddColumnInt("TriggerNum", "uint32", "FTM board trigger counter");
    fFile->AddColumnShort("TriggerType", "uint16", "FTM board trigger type");
    fFile->AddColumnInt("NumBoards", "uint32", "Number of connected boards");
    fFile->AddColumnInt(2, "UnixTimeUTC", "uint32", "Unix time seconds and microseconds");
    fFile->AddColumnInt(NBOARDS, "BoardTime", "uint32", "Board internal time counter");
    fFile->AddColumnShort(NPIX, "StartCellData", "uint16", "DRS4 start cell of readout");
    fFile->AddColumnShort(NTMARK, "StartCellTimeMarker", "uint16", "DRS4 start cell of readout time marker");

    vector<uint16_t> processing(2);
    processing[0] = FITS::kFactSmoothing;
    processing[1] = FITS::kFactHuffman16;

    const FITS::Compression comp(processing, FITS::kOrderByRow);

    fFile->AddColumnShort(comp, h.NPix*h.Nroi,   "Data",       "int16", "Digitized data");
    fFile->AddColumnShort(comp, h.NTm*realRoiTM, "TimeMarker", "int16", "Digitized time marker - if available");

    const size_t sz = (h.NPix*h.Nroi + h.NTm*realRoiTM)*2;
    if (fFile->GetBytesPerRow()-sz+4!=sizeof(EVENT))
    {
        ostringstream str;
        str << "The EVENT structure size (" << sizeof(EVENT) << ") doesn't match the described FITS row (";
        str << fFile->GetBytesPerRow()-sz+4 << ")";
        throw runtime_error(str.str());
    }

    // =============== Default keys for all files ================
    fFile->SetDefaultKeys();
    fFile->SetInt("NIGHT", GetNight(), "Night as int");
#ifdef HAVE_NOVA
    fFile->SetStr("OBSERVAT", Nova::LnLatPosn::preset(), "Observatory name (see nova.h)");
#endif

    // ================ Header keys for raw-data =================
    fFile->SetInt("BLDVER",   h.Version,  "Builder version");
    fFile->SetInt("RUNID",    GetRunId(),  "Run number");
    fFile->SetInt("NBOARD",   h.NBoard,   "Number of acquisition boards");
    fFile->SetInt("NPIX",     h.NPix,     "Number of pixels");
    fFile->SetInt("NTMARK",   h.NTm,      "Number of time marker channels");
    fFile->SetInt("NCELLS",   1024,        "Maximum number of slices per pixels");
    fFile->SetInt("NROI",     h.Nroi,     "Number of slices per pixels");
    fFile->SetInt("NROITM",   realRoiTM,   "Number of slices per time-marker");

    const uint16_t realOffset = (h.NroiTM > h.Nroi) ?  h.NroiTM - 2*h.Nroi : 0;
    fFile->SetInt("TMSHIFT",  realOffset,  "Shift of marker readout w.r.t. to data");

    //FIXME should we also put the start and stop time of the received data ?
    //now the events header related variables
    fFile->SetStr("CAMERA",   "MGeomCamFACT", "MARS camera geometry class");
    fFile->SetStr("DAQ",      "DRS4",         "Data acquisition type");
    fFile->SetInt("ADCRANGE", 2000,        "Dynamic range in mV");
    fFile->SetInt("ADC",      12,          "Resolution in bits");
    fFile->SetStr("RUNTYPE",  d.name,      "File type according to FAD configuration");

    // Write a single key for:
    // -----------------------
    // Start package flag
    // package length
    // version number
    // status
    // Prescaler

    // Write 40 keys for (?)
    // Phaseshift
    // DAC

    for (int i=0; i<h.NBoard; i++)
    {
        const PEVNT_HEADER &hh = h.FADhead[i];

        ostringstream sout;
        sout << "Board " << setw(2) << i<< ": ";

        const string num = to_string(i);

        // Header values whihc won't change during the run
        fFile->SetInt("ID"+num,    hh.board_id,   sout.str()+"Board ID");
        fFile->SetInt("FWVER"+num, hh.version_no, sout.str()+"Firmware Version");
        fFile->SetHex("DNA"+num,   hh.DNA,        sout.str()+"Unique FPGA device identifier (DNA)");
    }

    // FIXME: Calculate average ref clock frequency
    for (int i=0; i<h.NBoard; i++)
    {
        const PEVNT_HEADER &hh = h.FADhead[i];
        if (hh.start_package_flag==0)
            continue;

        fFile->SetInt("BOARD", i, "Board number for RUN, PRESC, PHASE and DAC");
        fFile->SetInt("PRESC", hh.trigger_generator_prescaler, "Trigger generator prescaler");
        fFile->SetInt("PHASE", (int16_t)hh.adc_clock_phase_shift, "ADC clock phase shift");

        for (int j=0; j<8; j++)
        {
            ostringstream dac, cmt;
            dac << "DAC" << j;
            cmt << "Command value for " << dac.str();
            fFile->SetInt(dac.str(), hh.dac[j], cmt.str());
        }

        break;
    }

    double avg = 0;
    int    cnt = 0;
    for (int i=0; i<h.NBoard; i++)
    {
        const PEVNT_HEADER &hh = h.FADhead[i];

        if (hh.start_package_flag==0)
            continue;

        avg += hh.REFCLK_frequency;
        cnt ++;
    }

    // FIXME: I cannot write a double! WHY?
    fFile->SetFloat("REFCLK", avg/cnt*2.048, "Average reference clock frequency in Hz");

    fFile->SetBool("DRSCALIB", GetDrsStep()>=0, "This file belongs to a DRS calibration");
    if (GetDrsStep()>=0)
        fFile->SetInt("DRSSTEP", GetDrsStep(), "Step of the DRS calibration");

    fTstart[0] = h.RunTime;
    fTstart[1] = h.RunUsec;

    fTstop[0] = 0;
    fTstop[1] = 0;

    fTriggerCounter.fill(0);

    WriteFooter();

    fFile->WriteTableHeader("Events");
};

// --------------------------------------------------------------------------
//
//! DataWriteFits constructor. This is the one that should be used, not the default one (parameter-less)
//! @param runid This parameter should probably be removed. I first thought it was the run number, but apparently it is not
//! @param h a pointer to the RUN_HEAD structure that contains the informations relative to this run
//
bool DataWriteFits2::Open(const RUN_HEAD &h, const FAD::RunDescription &d)
{
    //Form filename, based on runid and run-type
    fFileName = FormFileName(dynamic_pointer_cast<factofits>(fFile)?"fits.fz":"fits");

    if (boost::filesystem::exists(fFileName))
    {
        Error("ofits - file '"+fFileName+"' already exists.");
        return false;
    }

    zofits *fits = dynamic_cast<zofits*>(fFile.get());
    if (fits)
    {
        const uint32_t nrpt = zofits::DefaultNumRowsPerTile();

        // Maximum number of events if taken with 100Hz
        // (If no limit requested, maxtime is 24*60*60)
        const uint32_t ntime = d.maxtime*100/nrpt;

        // Maximum number of events if taken as number
        // (If no limit requested, maxevts is INT32_MAX)
        const uint32_t nevts = d.maxevt/nrpt+1;

        // get the minimum of all three
        uint32_t num = zofits::DefaultMaxNumTiles();
        if (ntime<num)
            num = ntime;
        if (nevts<num)
            num = nevts;

        fits->SetNumTiles(num);
    }

    try
    {
        fFile->open(fFileName.c_str());
    }
    catch (const exception &e)
    {
        Error("ofits::open() failed for '"+fFileName+"': "+e.what());
        return false;
    }

    if (!(*fFile))
    {
        ostringstream str;
        str << "ofstream::open() failed for '" << fFileName << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);
        return false;
    }

    try
    {
        WriteHeader(h, d);
    }
    catch (const exception &e)
    {
        Error("ofits - Writing header failed for '"+fFileName+"': "+e.what());
        return false;
    }

    if (!(*fFile))
    {
        ostringstream str;
        str << "ofstream::write() failed for '" << fFileName << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);
        return false;
    }

    return true;
}

// --------------------------------------------------------------------------
//
//! This writes one event to the file
//! @param e the pointer to the EVENT
//
bool DataWriteFits2::WriteEvt(const EVT_CTRL2 &evt)
{
    // Remember the counter of the last written event
    fTriggerCounter = evt.triggerCounter;

    // Remember the time of the last event
    fTstop[0] = evt.time.tv_sec;
    fTstop[1] = evt.time.tv_usec;

    const EVENT &e = *evt.fEvent;

    const int realRoiTM = (e.RoiTM > e.Roi) ? e.Roi : 0;
    const size_t sz = sizeof(EVENT) + sizeof(e.StartPix)*e.Roi+sizeof(e.StartTM)*realRoiTM; //ETIENNE from RoiTm to Roi

    try
    {
        fFile->WriteRow(reinterpret_cast<const char*>(&e)+4, sz-4);
    }
    catch (const exception &ex)
    {
        Error("ofits::WriteRow failed for '"+fFileName+"': "+ex.what());
        return false;
    }

    if (!(*fFile))
    {
        ostringstream str;
        str << "fstream::write() failed for '" << fFileName << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);
        return false;
    }

    return true;
}

void DataWriteFits2::WriteFooter()
{
    //FIXME shouldn't we convert start and stop time to MjD first ?
    //FIXME shouldn't we also add an MjD reference ?

    const Time start(fTstart[0], fTstart[1]);
    const Time stop (fTstop[0],  fTstop[1]);

    fFile->SetInt("TSTARTI",  uint32_t(floor(start.UnixDate())),
                  "Time when first evt received (integral part)");
    fFile->SetFloat("TSTARTF",  fmod(start.UnixDate(), 1),
                    "Time when first evt received (fractional part)");
    fFile->SetInt("TSTOPI",   uint32_t(floor(stop.UnixDate())),
                  "Time when last evt received (integral part)");
    fFile->SetFloat("TSTOPF",   fmod(stop.UnixDate(), 1),
                    "Time when last evt received (fractional part)");
    fFile->SetStr("DATE-OBS", start.Iso(),
                  "Time when first event received");
    fFile->SetStr("DATE-END", stop.Iso(),
                  "Time when last event received");

    fFile->SetInt("NTRG",     fTriggerCounter[0], "No of physics triggered events");
    fFile->SetInt("NTRGPED",  fTriggerCounter[1], "No of pure pedestal triggered events");
    fFile->SetInt("NTRGLPE",  fTriggerCounter[2], "No of external light pulser triggered events");
    fFile->SetInt("NTRGTIM",  fTriggerCounter[3], "No of time calibration triggered events");
    fFile->SetInt("NTRGLPI",  fTriggerCounter[4], "No of internal light pulser triggered events");
    fFile->SetInt("NTRGEXT1", fTriggerCounter[5], "No of triggers from ext1 triggered events");
    fFile->SetInt("NTRGEXT2", fTriggerCounter[6], "No of triggers from ext2 triggered events");
    fFile->SetInt("NTRGMISC", fTriggerCounter[7], "No of all other triggered events");
}

// --------------------------------------------------------------------------
//
//! Closes the file, and before this it write the TAIL data
//! @param rt the pointer to the RUN_TAIL data structure
//
bool DataWriteFits2::Close(const EVT_CTRL2 &)
{
    if (!fFile->is_open())
    {
        Error("DataWriteFits2::Close() called but file '"+fFileName+"' not open.");
        return false;
    }

    try
    {
        WriteFooter();
    }
    catch (const exception &e)
    {
        Error("ofits - Setting footer key values failed for '"+fFileName+"': "+e.what());
        return false;
    }

    try
    {
        fFile->close();
    }
    catch (const exception &e)
    {
        Error("ofits::close() failed for '"+fFileName+"': "+e.what());
        return false;
    }

    if (!(*fFile))
    {
        ostringstream str;
        str << "ofstream::close() failed for '" << fFileName << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);
        return false;
    }

    return true;
}
