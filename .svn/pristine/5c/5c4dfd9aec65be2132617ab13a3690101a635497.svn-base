#include "DataWriteFits.h"

#include "HeadersFAD.h"
#include "EventBuilder.h"
#include "Converter.h"


using namespace std;

DataWriteFits::~DataWriteFits()
{
    if (fFile.IsOpen())
    {
        WriteFooter();
        fFile.Close();
    }

    delete fConv;
}

template <typename T>
    void DataWriteFits::WriteKey(const string &name, const int idx, const T &value, const string &comment)
{
    ostringstream str;
    str << name << idx;

    ostringstream com;
    com << "Board " << setw(2) << idx << ": " << comment;

    fFile.WriteKey(str.str(), value, com.str());
}

// --------------------------------------------------------------------------
//
//! DataWriteFits constructor. This is the one that should be used, not the default one (parameter-less)
//! @param runid This parameter should probably be removed. I first thought it was the run number, but apparently it is not
//! @param h a pointer to the RUN_HEAD structure that contains the informations relative to this run
//
bool DataWriteFits::Open(const RUN_HEAD &h, const FAD::RunDescription &d)
{
    if (fConv)
    {
        Error("DataWriteFits::Open called twice.");
        return false;
    }

    const int16_t realRoiTM = (h.NroiTM >= 2*h.Nroi && h.Nroi<=512) ? h.Nroi : 0;

    fFile.AddColumn('I', "EventNum");
    fFile.AddColumn('I', "TriggerNum");
    fFile.AddColumn('S', "TriggerType");
    fFile.AddColumn('I', "NumBoards");
    fFile.AddColumn('C', "Errors",              4);
    fFile.AddColumn('I', "SoftTrig");
    fFile.AddColumn('I', "UnixTimeUTC",         2);
    fFile.AddColumn('I', "BoardTime",           NBOARDS);
    fFile.AddColumn('S', "StartCellData",       NPIX);
    fFile.AddColumn('S', "StartCellTimeMarker", NTMARK);
    fFile.AddColumn('S', "Data",                h.NPix*h.Nroi);	
    fFile.AddColumn('S', "TimeMarker",          h.NTm*realRoiTM);

    // Write length of physical pipeline (1024)
    fConv = new Converter(Converter::ToFormat(fFile.GetColumnTypes()));

    const size_t sz = (h.NPix*h.Nroi + h.NTm*realRoiTM)*2;
    if (fConv->GetSize()-sz+4!=sizeof(EVENT))
    {
        ostringstream str;
        str << "The EVENT structure size (" << sizeof(EVENT) << ") doesn't match the described FITS row (";
        str << fConv->GetSize()-sz+4 << ")";
        Error(str);
        return false;
    }

    //Form filename, based on runid and run-type
    fFileName = FormFileName("fits");

    if (!fFile.OpenFile(fFileName))
        return false;

    if (!fFile.OpenTable("Events"))
        return false;

    if (!fFile.WriteDefaultKeys("fadctrl"))
        return false;

    Info("==> TODO: Write sampling frequency...");

    //write header data
    //first the "standard" keys
    try
    {
        fFile.WriteKey("BLDVER",   h.Version,  "Builder version");
        fFile.WriteKey("RUNID",    GetRunId(),  "Run number");
//        fFile.WriteKey("RUNTYPE",  h.RunType,  "Type of run");
        fFile.WriteKey("NBOARD",   h.NBoard,   "Number of acquisition boards");
        fFile.WriteKey("NPIX",     h.NPix,     "Number of pixels");
        fFile.WriteKey("NTMARK",   h.NTm,      "Number of time marker channels");
        fFile.WriteKey("NCELLS",   1024,        "Maximum number of slices per pixels");
        fFile.WriteKey("NROI",     h.Nroi,     "Number of slices per pixels");
        fFile.WriteKey("NROITM",   realRoiTM,   "Number of slices per time-marker");

        const uint16_t realOffset = (h.NroiTM > h.Nroi) ?  h.NroiTM - 2*h.Nroi : 0;
        fFile.WriteKey("TMSHIFT",  realOffset,  "Shift of the start of the time marker readout wrt to data");

        //FIXME should we also put the start and stop time of the received data ?
        //now the events header related variables
        fFile.WriteKey("CAMERA",   "MGeomCamFACT", "");
        fFile.WriteKey("DAQ",      "DRS4",         "");
        fFile.WriteKey("ADCRANGE", 2000,        "Dynamic range in mV");
        fFile.WriteKey("ADC",      12,          "Resolution in bits");
        fFile.WriteKey("RUNTYPE",  d.name,      "File type according to FAD configuration");

        // Write a single key for:
        // -----------------------
        // Start package flag
        // package length
        // version number
        // status
        // Prescaler

        // Write 40 kays for (?)
        // Phaseshift
        // DAC

        for (int i=0; i<h.NBoard; i++)
        {
            const PEVNT_HEADER &hh = h.FADhead[i];

            // Header values whihc won't change during the run
            WriteKey("ID",    i, hh.board_id,   "Board ID");
            WriteKey("FWVER", i, hh.version_no, "Firmware Version");

            ostringstream dna;
            dna << "0x" << hex << hh.DNA;
            WriteKey("DNA", i, dna.str(), "Unique FPGA device identifier (DNA)");
        }

        // FIXME: Calculate average ref clock frequency
        for (int i=0; i<h.NBoard; i++)
        {
            const PEVNT_HEADER &hh = h.FADhead[i];

            if (hh.start_package_flag==0)
                continue;

            fFile.WriteKey("BOARD", i, "Board number for RUN, PRESC, PHASE and DAC");
            // fFile.WriteKey("RUN",   hh.runnumber, "Run number");
            fFile.WriteKey("PRESC", hh.trigger_generator_prescaler, "Trigger generator prescaler");
            fFile.WriteKey("PHASE", (int16_t)hh.adc_clock_phase_shift, "ADC clock phase shift");

            for (int j=0; j<8; j++)
            {
                ostringstream dac, cmt;
                dac << "DAC" << j;
                cmt << "Command value for " << dac.str();
                fFile.WriteKey(dac.str(), hh.dac[j], cmt.str());
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
        fFile.WriteKey("REFCLK", avg/cnt*2.048, "Average reference clock frequency in Hz");

        fFile.WriteKey("DRSCALIB", GetDrsStep()>=0, "This file belongs to a DRS calibration");
        if (GetDrsStep()>=0)
            fFile.WriteKey("DRSSTEP", GetDrsStep(), "Step of the DRS calibration");

    }
    catch (const CCfits::FitsException &e)
    {
        Error("CCfits::Table::addKey failed in '"+fFileName+"': "+e.message());
        return false;
    }

    fTstart[0] = h.RunTime;
    fTstart[1] = h.RunUsec;

    fTstop[0] = 0;
    fTstop[1] = 0;

    fTriggerCounter.fill(0);

    //Last but not least, add header keys that will be updated when closing the file
    return WriteFooter();
}

// --------------------------------------------------------------------------
//
//! This writes one event to the file
//! @param e the pointer to the EVENT
//
bool DataWriteFits::WriteEvt(const EVT_CTRL2 &evt)
{
    if (!fFile.AddRow())
        return false;

    // Remember the counter of the last written event
    fTriggerCounter = evt.triggerCounter;

    // Remember the time of the last event
    fTstop[0] = evt.time.tv_sec;
    fTstop[1] = evt.time.tv_usec;

    const EVENT &e = *evt.fEvent;

    const int realRoiTM = (e.RoiTM > e.Roi) ? e.Roi : 0;
    const size_t sz = sizeof(EVENT) + sizeof(e.StartPix)*e.Roi+sizeof(e.StartTM)*realRoiTM; //ETIENNE from RoiTm to Roi

    const vector<char> data = fConv->ToFits(reinterpret_cast<const char*>(&e)+4, sz-4);

    return fFile.WriteData(data.data(), data.size());
}

bool DataWriteFits::WriteFooter()
{
    try
    {
        /*
        fFile.WriteKey("NBEVTOK",  rt ? rt->nEventsOk  : uint32_t(0),
                       "How many events were written");

        fFile.WriteKey("NBEVTREJ", rt ? rt->nEventsRej : uint32_t(0),
                       "How many events were rejected by SW-trig");

        fFile.WriteKey("NBEVTBAD", rt ? rt->nEventsBad : uint32_t(0),
                       "How many events were rejected by Error");
        */

        //FIXME shouldn't we convert start and stop time to MjD first ?
        //FIXME shouldn't we also add an MjD reference ?

        const Time start(fTstart[0], fTstart[1]);
        const Time stop (fTstop[0],  fTstop[1]);

        fFile.WriteKey("TSTARTI",  uint32_t(floor(start.UnixDate())),
                       "Time when first event received (integral part)");
        fFile.WriteKey("TSTARTF",  fmod(start.UnixDate(), 1),
                       "Time when first event received (fractional part)");
        fFile.WriteKey("TSTOPI",   uint32_t(floor(stop.UnixDate())),
                       "Time when last event received (integral part)");
        fFile.WriteKey("TSTOPF",   fmod(stop.UnixDate(), 1),
                       "Time when last event received (fractional part)");
        fFile.WriteKey("DATE-OBS", start.Iso(),
                       "Time when first event received");
        fFile.WriteKey("DATE-END", stop.Iso(),
                       "Time when last event received");

        fFile.WriteKey("NTRG",     fTriggerCounter[0], "No of physics triggered events");
        fFile.WriteKey("NTRGPED",  fTriggerCounter[1], "No of pure pedestal triggered events");
        fFile.WriteKey("NTRGLPE",  fTriggerCounter[2], "No of external light pulser triggered events");
        fFile.WriteKey("NTRGTIM",  fTriggerCounter[3], "No of time calibration triggered events");
        fFile.WriteKey("NTRGLPI",  fTriggerCounter[4], "No of internal light pulser triggered events");
        fFile.WriteKey("NTRGEXT1", fTriggerCounter[5], "No of triggers from ext1 triggered events");
        fFile.WriteKey("NTRGEXT2", fTriggerCounter[6], "No of triggers from ext2 triggered events");
        fFile.WriteKey("NTRGMISC", fTriggerCounter[7], "No of all other triggered events");
    }
    catch (const CCfits::FitsException &e)
    {
        Error("CCfits::Table::addKey failed in '"+fFile.GetName()+"': "+e.message());
        return false;
    }
    return true;
}

// --------------------------------------------------------------------------
//
//! Closes the file, and before this it write the TAIL data
//! @param rt the pointer to the RUN_TAIL data structure
//
bool DataWriteFits::Close(const EVT_CTRL2 &)
{
    if (!fFile.IsOpen())
        return false;

    WriteFooter();

    fFile.Close();

    return true;
}
