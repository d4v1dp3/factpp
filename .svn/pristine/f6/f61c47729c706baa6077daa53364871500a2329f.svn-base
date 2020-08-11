#include "DataCalib.h"

#include "EventBuilder.h"
#include "FitsFile.h"
#include "DimDescriptionService.h"

#include "externals/fits.h"

using namespace std;

DrsCalibration DataCalib::fData;
bool DataCalib::fProcessing = false;
vector<float> DataCalib::fStats(1440*1024*6+160*1024*2+4);

void DataCalib::Restart()
{
    fData.Clear();

    reinterpret_cast<uint32_t*>(fStats.data())[0] = 0;
    reinterpret_cast<uint32_t*>(fStats.data())[1] = 0;
    reinterpret_cast<uint32_t*>(fStats.data())[2] = 0;
    reinterpret_cast<uint32_t*>(fStats.data())[3] = 0;

    int i=0;
    while (i<1024*1440*2+4)  // Set mean and RMS to 0
        fStats[i++] = 0;
    while (i<1024*1440*3+4)
        fStats[i++] = 2000./4096; // Set mean to 0.5
    while (i<1440*1024*6+160*1024*2+4)
        fStats[i++] = 0;   // Set everything else to 0

    fProcessing = false;
}

bool DataCalib::ResetTrgOff(DimDescribedService &dim, DimDescribedService &runs)
{
    if (fData.fStep!=3)
        return false;

    for (int i=1024*1440*4+4; i<1440*1024*6+160*1024*2+4; i++)
        fStats[i] = 0;

    reinterpret_cast<uint32_t*>(fStats.data())[0] = 0;
    reinterpret_cast<uint32_t*>(fStats.data())[3] = 0;

    fData.fStep = 1;
    fData.fDateRunBeg[2] = "1970-01-01T00:00:00";
    fData.fDateRunEnd[2] = "1970-01-01T00:00:00";
    fData.fDateEnd = fData.fDateRunEnd[1];
    Update(dim, runs);
    fData.fStep = 2;

    return true;
}

void DataCalib::Update(DimDescribedService &dim, DimDescribedService &runs)
{
    const uint16_t roi = fData.fRoi;
    //const uint16_t ntm = fData.fNumTm;

    vector<float> buf(1440*1024*6+160*1024*2+4);

    memcpy(buf.data(), fStats.data(), (4*1024*1440+4)*sizeof(float));

    for (int i=0; i<1440; i++)
    {
        memcpy(buf.data()+4+1440*1024*4 + i*1024, fStats.data()+4 + 4*1024*1440 + roi*i,            roi*sizeof(float));
        memcpy(buf.data()+4+1440*1024*5 + i*1024, fStats.data()+4 + 4*1024*1440 + roi*1440 + roi*i, roi*sizeof(float));
    }

    /*
    for (int i=0; i<ntm; i++)
    {
        memcpy(buf.data()+4+1440*1024*6          + i*1024, fStats.data()+4 + 4*1024*1440 + 2*roi*1440,       roi*sizeof(float));
        memcpy(buf.data()+4+1440*1024*6+160*1024 + i*1024, fStats.data()+4 + 4*1024*1440 + 2*roi*1440+i*roi, roi*sizeof(float));
    }*/

#warning Time marker channels not sent

    const Time time(fData.fDateObs);
    const uint32_t night = time.NightAsInt();

    dim.setQuality(fData.fStep);
    dim.setData(buf);
    dim.Update(time);

    vector<uint32_t> data(5);
    memcpy(data.data(), buf.data(), 4*sizeof(uint32_t));
    data[4] = night<19700000 ? 0 : night;

    runs.setQuality(fData.fStep);
    runs.setData(data);
    runs.Update(time);
}

bool DataCalib::Open(const RUN_HEAD &h, const FAD::RunDescription &d)
{
    if (h.NPix != 1440)
    {
        Error("Number of pixels in header for run "+to_string(GetRunId())+" not 1440.");
        return false;
    }

    if (fProcessing)
    {
        Warn("Previous DRS calibration run not yet finished (current run "+to_string(GetRunId())+")");
        return false;
    }

    if (fData.fStep==3)
    {
        Warn("DRS Calibration already finished before current run "+to_string(GetRunId())+"... please restart!");
        return false;
    }

    if (fData.fStep!=2 && h.Nroi != 1024)
    {
        ostringstream msg;
        msg << "Region of interest of run " << GetRunId() << " not 1024, but " << h.Nroi << " in step " << fData.fStep <<  " ... as it ought to be.";
        Error(msg);
        return false;
    }

    vector<uint16_t> dac(8);
/*
    // We don't check consistency over several boards because this is done
    // by the eventCheck routine already
    for (int i=0; i<h.NBoard; i++)
    {
        const PEVNT_HEADER &hh = h.FADhead[i];

        if (hh.start_package_flag==0)
            continue;

        for (int j=0; j<8; j++)
            dac[j] = hh.dac[j];

        break;
    }

    for (int i=1; i<7; i++)
    {
        if (i==3 || dac[i]==dac[i+1])
            continue;

        ostringstream msg;
        msg << "Values of DAC" << i << " (" << dac[i] << ") and DAC" << i+1 <<" (" << dac[i+1] << ") do not match... cannot take DRS calibration!";
        fMsg.Error(msg);
        return false;
    }

    if (fData.fStep>0)
    {
        for (int j=0; j<8; j++)
        {
            if (fData.fDAC[j]==dac[j])
                continue;

            ostringstream msg;
            msg << "DAC value from previous run (DAC" << j << "=" << fData.fDAC[j] << ") and current run ";
            msg << "(DAC" << j << "=" << dac[j] << ") inconsistent... cannot take DRS calibration!";
            fMsg.Error(msg);
            return false;
        }
    }

    memcpy(fData.fDAC, dac.data(), 8*sizeof(uint16_t));
*/
    fProcessing = true;

    const bool hastm = h.Nroi<=512 && h.NroiTM>=2*h.Nroi;

    Reset();
    InitSize(hastm ? 1600 : 1440, h.Nroi);

    fData.fRoi   = fNumSamples;
    fData.fNumTm = hastm ? 160 : 0;

    return DataWriteFits2::Open(h, d);
}

bool DataCalib::WriteEvt(const EVT_CTRL2 &evt)
{
    // FIXME: SET StartPix to 0 if StartPix is -1

    const EVENT &e = *evt.fEvent;

    if (fData.fStep==0)
    {
        AddRel(e.Adc_Data, e.StartPix);
    }
    if (fData.fStep==1)
    {
        AddRel(e.Adc_Data, e.StartPix, fData.fOffset.data(), fData.fNumOffset);
    }
    if (fData.fStep==2)
    {
        AddAbs(e.Adc_Data, e.StartPix, fData.fOffset.data(), fData.fNumOffset);
    }

    return DataWriteFits2::WriteEvt(evt);
}

bool DataCalib::ReadFits(const string &str, MessageImp &msg)
{
    if (fProcessing)
    {
        msg.Error("Reading "+str+" failed: DRS calibration in process.");
        return false;
    }

    try
    {
        const string txt = fData.ReadFitsImp(str, fStats);
        if (txt.empty())
            return true;

        msg.Error(txt);
        return false;
    }
    catch (const runtime_error &e)
    {
        msg.Error("Exception reading "+str+": "+e.what());
        return false;
    }
}
/*
void DataCalib::WriteFitsImp(const string &filename, const vector<float> &vec) const
{
    const uint16_t roi = fData.fRoi;
    const uint16_t ntm = fData.fNumTm;

    const size_t n = 1440*1024*4 + 1440*roi*2 + ntm*roi*2 + 3;

    // The vector has a fixed size
    //if (vec.size()!=n+1)
    //    throw runtime_error("Size of vector does not match region-of-interest");

    ofits file(filename.c_str());

    file.AddColumnInt("RunNumberBaseline");
    file.AddColumnInt("RunNumberGain");
    file.AddColumnInt("RunNumberTriggerOffset");

    file.AddColumnFloat(1024*1440, "BaselineMean",        "mV");
    file.AddColumnFloat(1024*1440, "BaselineRms",         "mV");
    file.AddColumnFloat(1024*1440, "GainMean",            "mV");
    file.AddColumnFloat(1024*1440, "GainRms",             "mV");
    file.AddColumnFloat( roi*1440, "TriggerOffsetMean",   "mV");
    file.AddColumnFloat( roi*1440, "TriggerOffsetRms",    "mV");
    file.AddColumnFloat( roi*ntm,  "TriggerOffsetTMMean", "mV");
    file.AddColumnFloat( roi*ntm,  "TriggerOffsetTMRms",  "mV");

    DataWriteFits2::WriteDefaultKeys(file);

    file.SetInt("STEP",     fData.fStep, "");

    file.SetInt("ADCRANGE", 2000, "Dynamic range of the ADC in mV");
    file.SetInt("DACRANGE", 2500, "Dynamic range of the DAC in mV");
    file.SetInt("ADC",      12,   "Resolution of ADC in bits");
    file.SetInt("DAC",      16,   "Resolution of DAC in bits");
    file.SetInt("NPIX",     1440, "Number of channels in the camera");
    file.SetInt("NTM",      ntm,  "Number of time marker channels");
    file.SetInt("NROI",     roi,  "Region of interest");

    file.SetInt("NBOFFSET", fData.fNumOffset,       "Num of entries for offset calibration");
    file.SetInt("NBGAIN",   fData.fNumGain/1953125, "Num of entries for gain calibration");
    file.SetInt("NBTRGOFF", fData.fNumTrgOff,       "Num of entries for trigger offset calibration");

    // file.WriteKeyNT("DAC_A",    fData.fDAC[0],    "Level of DAC 0 in DAC counts")   ||
    // file.WriteKeyNT("DAC_B",    fData.fDAC[1],    "Leval of DAC 1-3 in DAC counts") ||
    // file.WriteKeyNT("DAC_C",    fData.fDAC[4],    "Leval of DAC 4-7 in DAC counts") ||

    file.WriteTableHeader("DrsCalibration");
    file.WriteRow(vec.data()+1, n*sizeof(float));
}
*/
bool DataCalib::Close(const EVT_CTRL2 &evt)
{
    if (fNumEntries==0)
    {
        ostringstream str;
        str << "DRS calibration run (run=" << GetRunId() << ", step=" << fData.fStep << ", roi=" << fData.fRoi << ") has 0 events.";
        Warn(str);
    }

    if (fData.fStep==0)
    {
        fData.fOffset.assign(fSum.begin(), fSum.end());
        fData.fNumOffset = fNumEntries;

        for (int i=0; i<1024*1440; i++)
            fData.fGain[i] = 4096*fNumEntries;

        // Scale ADC data from 12bit to 2000mV
        GetSampleStats(fStats.data()+4, 2000./4096);
        reinterpret_cast<uint32_t*>(fStats.data())[1] = GetRunId();;
    }
    if (fData.fStep==1)
    {
        fData.fGain.assign(fSum.begin(), fSum.end());
        fData.fNumGain = fNumEntries;

        // DAC:  0..2.5V == 0..65535            2500*50000   625*50000  625*3125
        // V-mV: 1000                           ----------   ---------  --------
        //fNumGain *= 2500*50000;                  65536       16384      1024
        //for (int i=0; i<1024*1440; i++)
        //    fGain[i] *= 65536;
        fData.fNumGain *= 1953125;
        for (int i=0; i<1024*1440; i++)
            fData.fGain[i] *= 1024;

        // Scale ADC data from 12bit to 2000mV
        GetSampleStats(fStats.data()+1024*1440*2+4, 2000./4096/fData.fNumOffset);//0.5);
        reinterpret_cast<uint32_t*>(fStats.data())[2] = GetRunId();;
    }
    if (fData.fStep==2)
    {
        fData.fTrgOff.assign(fSum.begin(), fSum.end());
        fData.fNumTrgOff = fNumEntries;

        // Scale ADC data from 12bit to 2000mV
        GetSampleStats(fStats.data()+1024*1440*4+4, 2000./4096/fData.fNumOffset);//0.5);
        reinterpret_cast<uint32_t*>(fStats.data())[0] = fNumSamples;
        reinterpret_cast<uint32_t*>(fStats.data())[3] = GetRunId();
    }

    const string beg = GetTstart().Iso();
    const string end = GetTstop().Iso();

    if (fData.fStep==0)
        fData.fDateObs = beg;
    fData.fDateEnd = end;

    fData.fDateRunBeg[fData.fStep] = beg;
    fData.fDateRunEnd[fData.fStep] = end;

    if (fData.fStep<=2)
    {
        const string filename = FormFileName("drs.fits");
        try
        {
            fData.WriteFitsImp(filename, fStats, GetNight());

            ostringstream str;
            str << "Wrote DRS calibration data (run=" << GetRunId() << ", step=" << fData.fStep << ", roi=" << fData.fRoi << ") to '" << filename << "'";
            Info(str);
        }
        catch (const exception &e)
        {
            Error("Exception writing run "+to_string(GetRunId())+" '"+filename+"': "+e.what());
        }
    }

    Update(fDim, fDimRuns);

    fData.fStep++;

    fProcessing = false;

    return DataWriteFits2::Close(evt);
}
