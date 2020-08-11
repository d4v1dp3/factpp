#ifndef FACT_EventBuilderWrapper
#define FACT_EventBuilderWrapper

#include <sstream>

#if BOOST_VERSION < 104400
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#undef BOOST_HAS_RVALUE_REFS
#endif
#endif
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "DimWriteStatistics.h"

#include "DataCalib.h"
#include "DataWriteRaw.h"

#ifdef HAVE_FITS
#include "DataWriteFits.h"
#else
#define DataWriteFits DataWriteFits2
#endif

#include "DataWriteFits2.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace fs = boost::filesystem;

using ba::ip::tcp;

using namespace std;

// ========================================================================

#include "EventBuilder.h"

void StartEvtBuild();
void CloseRunFile();

// ========================================================================

class EventBuilderWrapper
{
public:
    // FIXME
    static EventBuilderWrapper *This;

    MessageImp &fMsg;

private:
    boost::thread fThreadMain;

    enum
    {
        kCurrent   = 0,
        kTotal     = 1,
        kEventId   = 2,
        kTriggerId = 3,
    };

    FAD::FileFormat_t fFileFormat;

    //uint32_t fMaxRun;
    uint32_t fLastOpened;
    uint32_t fLastClosed;
    array<uint32_t,4> fNumEvts;

    DimWriteStatistics  fDimWriteStats;
    DimDescribedService fDimRuns;
    DimDescribedService fDimEvents;
    DimDescribedService fDimTrigger;
    DimDescribedService fDimRawData;
    DimDescribedService fDimEventData;
    DimDescribedService fDimFeedbackData;
    DimDescribedService fDimFwVersion;
    DimDescribedService fDimRunNumber;
    DimDescribedService fDimStatus;
    DimDescribedService fDimDNA;
    DimDescribedService fDimTemperature;
    DimDescribedService fDimPrescaler;
    DimDescribedService fDimRefClock;
    DimDescribedService fDimRoi;
    DimDescribedService fDimDac;
    DimDescribedService fDimDrsRuns;
    DimDescribedService fDimDrsCalibration;
    DimDescribedService fDimStatistics1;
    //DimDescribedService fDimStatistics2;
    DimDescribedService fDimFileFormat;
    DimDescribedService fDimIncomplete;

    struct EventData
    {
        uint32_t runNum;
        uint32_t evNum;
        float data[4*1440];
    }  __attribute__((__packed__));

    Queue<pair<Time,GUI_STAT>>                      fQueueStatistics1;
    Queue<tuple<Time,bool,FAD::EventHeader>>        fQueueProcHeader;
    Queue<pair<Time,array<uint32_t,4>>>             fQueueEvents;
    Queue<tuple<Time,char,array<uint32_t,8>>>       fQueueTrigger;
    Queue<pair<Time,array<uint16_t,2>>>             fQueueRoi;
    Queue<vector<char>>                             fQueueRawData;
    Queue<tuple<Time,uint32_t,EventData/*array<float,1440*4>*/>> fQueueEventData;
    Queue<tuple<Time, array<uint32_t,40>, array<int16_t,160>>> fQueueTempRefClk;

    string   fPath;
    uint32_t fNightAsInt;
    uint32_t fRunNumber;
    int64_t  fRunInProgress;

    array<uint16_t,2> fVecRoi;
    pair<float,EventData/*array<float, 1440*4>*/> fMaxEvent; // Maximum event from applyCalib

protected:
    bool InitRunNumber(const string &path="")
    {
        if (!path.empty())
        {
            if (!DimWriteStatistics::DoesPathExist(path, fMsg))
            {
                fMsg.Error("Data path "+path+" does not exist!");
                return false;
            }

            fPath = path;
            fDimWriteStats.SetCurrentFolder(fPath);

            fMsg.Info("Data path set to "+path+".");
        }

        // Get current night
        const Time now;

        const uint32_t night = now.NightAsInt();
        if (night==fNightAsInt)
            return true;

        const string crosscheck = now.GetPrevSunRise().GetAsStr("%Y%m%d");
        if (crosscheck!=to_string(night))
        {
            fMsg.Warn("The crosscheck for the night failed. "+crosscheck+" is not equal to "+to_string(night)+"... keeping old one.");
            fMsg.Warn("This is a severe error. Please restart fadctrl.");
            return true;
        }

        // In some circumstances, I do not understand yet (but I guess it can happen
        // when the shared objects are re-compiled while the program is not
        // re-started), it can happen that the returned value is wrong by one day.
        // So this is just another check to avoid problems.
        const uint32_t night_test = Time(now-boost::posix_time::seconds(1)).NightAsInt();
        if (night_test != night)
            return true;

        // And another check. Let's read the clock again.
        // In both cases a false condition is no error and can happen. But if it happens,
        // the bahaviour will change a fraction of a second later and the conditon
        // will be true. No run should be taken just around that moment and if one
        // is taken, then the date doesn't matter.
        if (Time().NightAsInt() != night)
            return true;

        if (night<fNightAsInt)
        {
            fMsg.Warn("New night "+to_string(night)+" ["+now.GetAsStr()+"] before current night "+to_string(night)+"... keeping old one.");
            fMsg.Warn("Please check the system clock.");
            return true;
        }

        // Check for run numbers
        fRunNumber = 1000;

        while (--fRunNumber>0)
        {
            const string name = DataProcessorImp::FormFileName(fPath, night, fRunNumber, "");

            if (access((name+"bin").c_str(), F_OK) == 0)
                break;
            if (access((name+"fits").c_str(), F_OK) == 0)
                break;
            if (access((name+"fits.fz").c_str(), F_OK) == 0)
                break;
            if (access((name+"fits.gz").c_str(), F_OK) == 0)
                break;
            if (access((name+"drs.fits").c_str(), F_OK) == 0)
                break;
        }

        // This is now the first file which does not exist
        fRunNumber++;
        fLastOpened = 0;

        // Check if we have exceeded the maximum
        if (fRunNumber==1000)
        {
            fMsg.Error("You have a file with run-number 1000 in "+fPath+" ["+to_string(night)+"]");
            return false;
        }

        // WARNING: This fails if the sun-rise is around midnight UTC!
        // It would be more appropriate to relate the "NightAsInt" to
        // the LOCAL time of sun-rise!
        ostringstream str;
        if (fNightAsInt==0)
            str << "First night...";
        else
            str << "Night has changed from " << fNightAsInt << " [" << now << "]... ";
        str << " next run-number is " << night << "-" << setfill('0') << setw(3) << fRunNumber << " [" << (fPath.empty()?".":fPath) << "]";
        fMsg.Message(str);

        fNightAsInt = night;

        return true;
    }

public:
    EventBuilderWrapper(MessageImp &imp) : fMsg(imp),
        fFileFormat(FAD::kNone), /*fMaxRun(0),*/ fLastOpened(0), fLastClosed(0),
        fDimWriteStats  ("FAD_CONTROL", imp),
        fDimRuns        ("FAD_CONTROL/RUNS",               "I:2;C",
                                                           "Run files statistics"
                                                           "|stats[int]:last opened or closed run"
                                                           "|file[string]:filename of last opened file"),
        fDimEvents      ("FAD_CONTROL/EVENTS",             "I:4",
                                                           "Event counts"
                                                           "|evtsCount[int]:Num evts cur. run, total (all run), evt ID, trig. Num"),
        fDimTrigger     ("FAD_CONTROL/TRIGGER_COUNTER",    "I:1;I:1;I:1;I:1;I:1;I:1;I:1;I:1",
                                                           "Trigger counter"
                                                           "|N_trg[uint32]:Number of physics triggers"
                                                           "|N_ped[uint32]:Number of pure pedestal triggers"
                                                           "|N_lpe[uint32]:Number of external light pulser triggers"
                                                           "|N_tim[uint32]:Number of time calibration triggers"
                                                           "|N_lpi[uint32]:Number of internal light pulser triggers"
                                                           "|N_ext1[uint32]:Number of external triggers at input ext1"
                                                           "|N_ext2[uint32]:Number of external triggers at input ext2"
                                                           "|N_misc[uint32]:Number of all other triggers"),
        fDimRawData     ("FAD_CONTROL/RAW_DATA",           "S:1;S:1;I:1;I:1;S:1;I:1;I:2;I:40;S:1440;S:160;F",
                                                           "|roi[uint16]:number of samples per pixel"
                                                           "|roi_tm[uint16]:number of samples per time-marker channel"
                                                           "|num_fad[uint32]:event number from FADs"
                                                           "|num_ftm[uint32]:trigger number from FTM"
                                                           "|type[uint16]:trigger type from FTM"
                                                           "|num_boards[uint32]:number of active boards"
                                                           "|time[uint32]:PC time as unix time stamp"
                                                           "|time_board[uint32]:Time stamp of FAD boards"
                                                           "|start_pix[int16]:start sample of pixels"
                                                           "|start_tm[int16]:start sample of time marker channels"
                                                           "|adc[int16]:adc data"),
        fDimEventData   ("FAD_CONTROL/EVENT_DATA",         "I:1;I:1;F:1440;F:1440;F:1440;F:1440", "|run:|evt:|avg:|rms:|max:|pos"),
        fDimFeedbackData("FAD_CONTROL/FEEDBACK_DATA",      "F:1440", ""),
        fDimFwVersion   ("FAD_CONTROL/FIRMWARE_VERSION",   "F:42",
                                                           "Firmware version number of fad boards"
                                                           "|firmware[float]:Version number of firmware, for each board. 40=min, 41=max"),
        fDimRunNumber   ("FAD_CONTROL/RUN_NUMBER",         "I:42",
                                                           "Run numbers coming from FAD boards"
                                                           "|runNumbers[int]:current run number of each FAD board. 40=min, 41=max"),
        fDimStatus      ("FAD_CONTROL/STATUS",             "S:42",
                                                           "Status of FAD boards"
                                                           "|status[bitpattern]:Status of each FAD board. Maybe buggy"),
        fDimDNA         ("FAD_CONTROL/DNA",                "X:40",
                                                           "DNA of FAD boards"
                                                           "|DNA[hex]:Hex identifier of each FAD board"),
        fDimTemperature ("FAD_CONTROL/TEMPERATURE",        "S:1;F:160",
                                                           "DRS temperatures"
                                                           "|cnt[uint16]:Counter of averaged values"
                                                           "|temp[deg C]:average temp of all DRS chips"),
        fDimPrescaler   ("FAD_CONTROL/PRESCALER",          "S:42",
                                                           "Trigger generator prescaler of fad boards"
                                                           "|prescaler[int]:Trigger generator prescaler value, for each board"),
        fDimRefClock    ("FAD_CONTROL/REFERENCE_CLOCK",    "S:1;F:40",
                                                           "Reference clock of FAD boards"
                                                           "|cnt[uint16]:Counter of averaged values"
                                                           "|clk[Hz]:Averaged clock of ref clocks of FAD boards"),
        fDimRoi         ("FAD_CONTROL/REGION_OF_INTEREST", "S:2",  "roi:|roi_rm:"),
        fDimDac         ("FAD_CONTROL/DAC",                "S:336",
                                                           "DAC settings of each FAD board"
                                                           "|DAC[int]:DAC counts, sequentially DAC 0 board 0, 0/1, 0/2... (plus min max)"),
        fDimDrsRuns     ("FAD_CONTROL/DRS_RUNS",           "I:1;I:3;I:1",
                                                           "|roi:Region of interest of secondary baseline"
                                                           "|run:Run numbers of DRS runs (0=none)"
                                                           "|night:Night as int of the first run (0 if none)"),
        fDimDrsCalibration("FAD_CONTROL/DRS_CALIBRATION",  "I:1;I:3;F:1474560;F:1474560;F:1474560;F:1474560;F:1474560;F:1474560;F:163840;F:163840",
                                                           "|roi:Region of interest of secondary baseline"
                                                           "|run:Run numbers of DRS runs (0=none)"),
        fDimStatistics1 ("FAD_CONTROL/STATISTICS1",        "I:5;X:3;I:1;I:2;C:40;I:40;I:40",
                                                           "Event Builder status for GUI display"
                                                           "|bufferInfo[int]:Events in buffer, incomp., comp., write, proc., tot."
                                                           "|memInfo[int]:total mem allocated, used mem, max memory"
                                                           "|deltaT[ms]:Time in ms for rates"
                                                           "|rateNew[int]:Number of new start events received"
                                                           "|numConn[int]:Number of connections per board"
                                                           "|rateBytes[int]:Bytes read during last cylce"
                                                           "|relBytes[int]:Relative number of total bytes received (received - released)"),
        fDimFileFormat("FAD_CONTROL/FILE_FORMAT",          "S:1", "|format[int]:Current file format"),
        fDimIncomplete("FAD_CONTROL/INCOMPLETE",           "X:1", "|incomplete[bits]:bit_index=c*10+b. board b(0..3) in crate c(0..9)"),
        // It is important to instantiate them after the DimServices
        fQueueStatistics1(std::bind(&EventBuilderWrapper::UpdateDimStatistics1, this, placeholders::_1)),
        fQueueProcHeader( std::bind(&EventBuilderWrapper::procHeader,           this, placeholders::_1)),
        fQueueEvents(     std::bind(&EventBuilderWrapper::UpdateDimEvents,      this, placeholders::_1)),
        fQueueTrigger(    std::bind(&EventBuilderWrapper::UpdateDimTrigger,     this, placeholders::_1)),
        fQueueRoi(        std::bind(&EventBuilderWrapper::UpdateDimRoi,         this, placeholders::_1)),
        fQueueRawData(    std::bind(&EventBuilderWrapper::UpdateDimRawData,     this, placeholders::_1)),
        fQueueEventData(  std::bind(&EventBuilderWrapper::UpdateDimEventData,   this, placeholders::_1)),
        fQueueTempRefClk( std::bind(&EventBuilderWrapper::UpdateDimTempRefClk,  this, placeholders::_1)),
        fNightAsInt(0), fRunInProgress(-1),
        fMaxEvent(make_pair(-FLT_MAX, EventData()/*array<float,1440*4>()*/))
    {
        if (This)
            throw logic_error("EventBuilderWrapper cannot be instantiated twice.");

        This = this;

        fVecRoi.fill(0);

        memset(fNumEvts.data(), 0, sizeof(fNumEvts));
        fDimEvents.Update(fNumEvts);

        for (size_t i=0; i<40; i++)
            ConnectSlot(i, tcp::endpoint());
    }

    virtual ~EventBuilderWrapper()
    {
        Abort();

        // FIXME: Used timed_join and abort afterwards
        //        What's the maximum time the eb need to abort?
        fThreadMain.join();
    }

    map<uint32_t, FAD::RunDescription> fExpectedRuns;

    mutex mtx_newrun;

    uint32_t StartNewRun(int64_t maxtime, int64_t maxevt, const pair<string, FAD::Configuration> &ref)
    {
        if (maxtime<=0 || maxtime>24*60*60)
            maxtime = 24*60*60;
        if (maxevt<=0 || maxevt>INT32_MAX)
            maxevt  = INT32_MAX;

        if (!InitRunNumber())
            return 0;

        const FAD::RunDescription descr =
        {
            uint32_t(maxtime),
            uint32_t(maxevt),
            fNightAsInt,
            ref.first,
            ref.second,
        };

        const lock_guard<mutex> lock(mtx_newrun);
        fExpectedRuns[fRunNumber] = descr;
        return fRunNumber++;
    }

    bool IsThreadRunning()
    {
        return fThreadMain.joinable();
    }

    void SetMaxMemory(unsigned int mb) const
    {
        g_maxMem = size_t(mb)*1000000;
    }
    void SetEventTimeout(uint16_t to) const
    {
        g_evtTimeout = to;
    }

    void StartThread(const vector<tcp::endpoint> &addr)
    {
        if (IsThreadRunning())
        {
            fMsg.Warn("Start - EventBuilder still running");
            return;
        }

        //fLastMessage.clear();

        for (size_t i=0; i<40; i++)
            ConnectSlot(i, addr[i]);

        fMsg.Message("Starting EventBuilder thread");

        fThreadMain = boost::thread(StartEvtBuild);

        // Run a detached thread which ensures that our thread
        // is joined so that it is not joinable anymore once
        // it is finished (I think this is similar to
        // boost::thread_guard, but I could not figure out
        // how it works)
        std::thread([this] { this->fThreadMain.join(); }).detach();
    }

    void ConnectSlot(unsigned int i, const tcp::endpoint &addr)
    {
        if (i>39)
            return;

        fRunInProgress = -1;

        if (addr==tcp::endpoint())
        {
            // In this order
            g_port[i].sockDef = 0;

            fDimIncomplete.setQuality(0);
            fDimIncomplete.Update(uint64_t(0));
            return;
        }

        struct sockaddr_in sockaddr; //IP for each socket
        sockaddr.sin_family      = AF_INET;
        sockaddr.sin_addr.s_addr = htonl(addr.address().to_v4().to_ulong());
        sockaddr.sin_port        = htons(addr.port());
        memcpy(&g_port[i].sockAddr, &sockaddr, sizeof(struct sockaddr_in));

        // In this order
        g_port[i].sockDef = 1;

        fDimIncomplete.setQuality(0);
        fDimIncomplete.Update(uint64_t(0));
    }

    void IgnoreSlot(unsigned int i)
    {
        if (i>39)
            return;

        if (g_port[i].sockAddr.sin_port==0)
            return;

        g_port[i].sockDef = -1;
    }


    void Abort()
    {
        fMsg.Message("Signal abort to EventBuilder thread...");
        g_reset = 2;
    }

    void ResetThread(bool soft)
    {
        fMsg.Message("Signal reset to EventBuilder thread...");
        g_reset = soft ? 101 : 102;
    }

    void Exit()
    {
        fMsg.Message("Signal exit to EventBuilder thread...");
        g_reset = 1;
    }

    bool IsConnected(int i) const     { return gi_NumConnect[i]==1; }
    bool IsConnecting(int i) const    { return gi_NumConnect[i]==0 && g_port[i].sockDef!=0; }
    bool IsDisconnected(int i) const  { return gi_NumConnect[i]==0 && g_port[i].sockDef==0; }
    bool IsRunInProgress() const { return fRunInProgress>=0; }

    void SetIgnore(int i, bool b) const { if (g_port[i].sockDef!=0) g_port[i].sockDef=b?-1:1; }
    bool IsIgnored(int i) const { return g_port[i].sockDef==-1; }

    void SetOutputFormat(FAD::FileFormat_t f)
    {
        const bool changed = f!=fFileFormat;

        fFileFormat = f;
        fDimFileFormat.Update(uint16_t(f));

        string msg = "File format set to: ";
        switch (f)
	{
        case FAD::kNone:    msg += "kNone.";   break;
        case FAD::kDebug:   msg += "kDebug.";  break;
        case FAD::kFits:    msg += "kFits.";   break;
        case FAD::kZFits:   msg += "kZFits.";  break;
        case FAD::kCfitsio: msg += "kCfitsio"; break;
        case FAD::kRaw:     msg += "kRaw";     break;
        case FAD::kCalib:
            DataCalib::Restart();
            DataCalib::Update(fDimDrsCalibration, fDimDrsRuns);
            fMsg.Message("Resetted DRS calibration.");
            return;
        }

        if (changed)
            fMsg.Message(msg);
    }

    virtual int ResetSecondaryDrsBaseline()
    {
        if (DataCalib::ResetTrgOff(fDimDrsCalibration, fDimDrsRuns))
        {
            fFileFormat = FAD::kCalib;
            fDimFileFormat.Update(uint16_t(fFileFormat));
            fMsg.Message("Resetted DRS calibration for secondary baseline.");
        }
        else
            fMsg.Warn("Could not reset DRS calibration of secondary baseline.");

        return 0;
    }

    void LoadDrsCalibration(const char *fname)
    {
        if (!DataCalib::ReadFits(fname, fMsg))
            return;

        fMsg.Info("Successfully loaded DRS calibration from "+string(fname));
        DataCalib::Update(fDimDrsCalibration, fDimDrsRuns);
    }

    virtual int CloseOpenFiles() { CloseRunFile(); fRunInProgress = -1; return 0; }


    // -------------- Mapped event builder callbacks ------------------

    void UpdateRuns(const string &fname="")
    {
        uint32_t values[2] =
        {
            fLastOpened,
            fLastClosed
        };

        vector<char> data(sizeof(values)+fname.size()+1);
        memcpy(data.data(), values, sizeof(values));
        strcpy(data.data()+sizeof(values), fname.c_str());
        fDimRuns.setQuality((bool)fFile);
        fDimRuns.Update(data);

        if (!fname.empty())
            fDimWriteStats.FileOpened(fname);
    }

    shared_ptr<DataProcessorImp> fFile;

    bool UpdateDimEvents(const pair<Time,array<uint32_t,4>> &stat)
    {
        fDimEvents.setData(stat.second.data(), sizeof(uint32_t)*4);
        fDimEvents.Update(stat.first);
        return true;
    }

    bool UpdateDimTrigger(const tuple<Time,char,array<uint32_t,8>> &stat)
    {
        fDimTrigger.setQuality(get<1>(stat));
        fDimTrigger.setData(get<2>(stat).data(), sizeof(uint32_t)*8);
        fDimTrigger.Update(get<0>(stat));
        return true;
    }

    bool runOpen(const EVT_CTRL2 &evt)
    {
        const uint32_t night = evt.runCtrl->night;
        const uint32_t runid = evt.runNum>0 ? evt.runNum : time(NULL);

        // If there is still an open file: close it
        if (fFile)
            runClose(evt);

        // Keep a copy of the currently valid drs calibration
        // and associate it to the run control structure
        evt.runCtrl->calib = make_shared<DrsCalibration>(DataCalib::GetCalibration());

        // Crate the file
        DataProcessorImp *file = 0;
        switch (fFileFormat)
        {
        case FAD::kNone:    file = new DataDump(fPath, night, runid,  fMsg); break;
        case FAD::kDebug:   file = new DataDebug(fPath, night, runid, fMsg); break;
        case FAD::kCfitsio: file = new DataWriteFits(fPath, night, runid,  fMsg); break;
        case FAD::kFits:    file = new DataWriteFits2(fPath, night, runid, fMsg); break;
        case FAD::kZFits:   file = new DataWriteFits2(fPath, night, runid, *evt.runCtrl->calib, fMsg); break;
	case FAD::kRaw:     file = new DataWriteRaw(fPath, night, runid, fMsg); break;
	case FAD::kCalib:   file = new DataCalib(fPath, night, runid, *evt.runCtrl->calib, fDimDrsCalibration, fDimDrsRuns, fMsg); break;
        }

        try
        {
            // Try to open the file
            FAD::RunDescription desc;
            desc.maxevt  = evt.runCtrl->maxEvt;
            desc.maxtime = evt.runCtrl->closeTime - evt.runCtrl->openTime;
            desc.name    = evt.runCtrl->runType;

            if (!file->Open(evt, desc))
                return false;
        }
        catch (const exception &e)
        {
            fMsg.Error("Exception trying to open file: "+string(e.what()));
            return false;
        }

        fLastOpened = runid;

        // Signal that a file is open
        fFile = shared_ptr<DataProcessorImp>(file);

        // Now do all the calls which potentially block (dim)

        // Time for update runs before time for update events
        UpdateRuns(file->GetFileName());
        fNumEvts[kEventId]   = 0;
        fNumEvts[kTriggerId] = 0;
        fNumEvts[kCurrent]   = 0;

        const Time time;

        fQueueEvents.emplace(time, fNumEvts);
        fQueueTrigger.emplace(time, 'o', evt.triggerCounter);

        ostringstream str;
        str << "Opened: " << file->GetFileName() << " (" << file->GetRunId() << ")";
        fMsg.Info(str);

        return true;
    }

    bool runWrite(const EVT_CTRL2 &e)
    {
        /*
        const size_t size = sizeof(EVENT)+1440*(evt.Roi+evt.RoiTM)*2;
        vector evt(e.fEvent, e.fEvent+size);

        const EVENT &evt = *reinterpret_cast<EVENT*>(evt.data());

        int16_t *val = evt.Adc_Data;
        const int16_t *off = e.runCtrl->zcalib.data();
        for (const int16_t *start=evt.StartPix; start<evt.StartPix+1440; val+=1024, off+=1024, start++)
        {
            if (*start<0)
                continue;

            for (size_t i=0; i<roi; i++)
                val[i] -= offset[(*start+i)%1024];
        }*/

        if (!fFile->WriteEvt(e))
            return false;

        //const EVENT &evt = *e.fEvent;

        fNumEvts[kCurrent]++;
        fNumEvts[kEventId]   = e.evNum;//evt.EventNum;
        fNumEvts[kTriggerId] = e.trgNum;//evt.TriggerNum;
        fNumEvts[kTotal]++;

        static Time oldt(boost::date_time::neg_infin);
        Time newt;
        if (newt>oldt+boost::posix_time::seconds(1))
        {
            fQueueEvents.emplace(newt, fNumEvts);
            fQueueTrigger.emplace(newt, 'w', e.triggerCounter);
            oldt = newt;
        }

        return true;
    }

    void runClose(const EVT_CTRL2 &evt)
    {
        if (!fFile)
            return;

        // It can happen that runFinished was never called
        // (e.g. runWrite failed)
        if (fRunInProgress==fFile->GetRunId())
            fRunInProgress = -1;

        // Close the file
        const bool rc = fFile->Close(evt);

        fLastClosed = fFile->GetRunId();

        ostringstream str;
        str << "Closed: " << fFile->GetFileName() << " (" << fFile->GetRunId() << ")";
        if (!rc)
            str << "... failed!";

        // Signal that the file is closed

        fFile.reset();

        // Now do all the calls which can potentially block (dim)

        CloseRun(fLastClosed); 

        // Time for update events before time for update runs
        const Time time;

        fQueueEvents.emplace(time, fNumEvts);
        fQueueTrigger.emplace(time, 'c', evt.triggerCounter);

        UpdateRuns();

        // Do the potentially blocking call after all others
        rc ? fMsg.Info(str) : fMsg.Error(str);

        // If a Drs Calibration has just been finished, all following events
        // should also be processed with this calibration.
        // Note that this is a generally dangerous operation. Here, the previous
        // DRS calibration shared_ptr gets freed and if it is the last in use,
        // the memory will vanish. If another thread accesses that pointer,
        // it _must_ make a copy of the shared_ptr first to ensure that
        // the memory will stay in scope until the end of its operation.
        const DrsCalibration &cal = DataCalib::GetCalibration();

        RUN_CTRL2 &run = *evt.runCtrl;
        if (!run.calib || run.calib->fStep != cal.fStep || run.calib->fRoi!=cal.fRoi)
            run.calib = make_shared<DrsCalibration>(cal);
    }

    virtual void CloseRun(uint32_t /*runid*/) { }

    bool UpdateDimRoi(const pair<Time, array<uint16_t,2>> &roi)
    {
        fDimRoi.setData(roi.second.data(), sizeof(uint16_t)*2);
        fDimRoi.Update(roi.first);
        return true;
    }

    bool UpdateDimTempRefClk(const tuple<Time, array<uint32_t,40>, array<int16_t,160>> &dat)
    {
        const auto delay = boost::posix_time::seconds(5);

        const Time &tm = get<0>(dat);

        const array<uint32_t,40> &clk = get<1>(dat);
        const array<int16_t,160> &tmp = get<2>(dat);

        // --------------- RefClock ---------------

        // history, add current data to history
        static list<pair<Time,array<uint32_t,40>>> listclk;
        listclk.emplace_back(tm, clk);

        // --------------- Temperatures ---------------

        // history, add current data to history
        static list<pair<Time,array<int16_t,160>>> listtmp;
        listtmp.emplace_back(tm, tmp);

        // ========== Update dim services once a second =========

        static Time oldt(boost::date_time::neg_infin);
        Time newt;

        if (newt<oldt+delay)
            return true;

        oldt = newt;

        // --------------- RefClock ---------------

        // remove expired data from history
        while (1)
        {
            auto it=listclk.begin();
            if (it==listclk.end() || it->first+delay>tm)
                break;
            listclk.pop_front();
        }

        // Structure for dim service
        struct Clock
        {
            uint16_t num;
            float val[40];
            Clock() { memset(this, 0, sizeof(Clock)); }
        } __attribute__((__packed__));

        // Calculate average and fll structure
        vector<uint16_t> clknum(40);

        Clock avgclk;
        avgclk.num = listclk.size();
        for (auto it=listclk.begin(); it!=listclk.end(); it++)
            for (int i=0; i<40; i++)
                if (it->second[i]!=UINT32_MAX)
                {
                    avgclk.val[i] += it->second[i];
                    clknum[i]++;
                }
        for (int i=0; i<40; i++)
            avgclk.val[i] *= 2.048/clknum[i];

        // Update dim service
        fDimRefClock.setData(avgclk);
        fDimRefClock.Update(tm);

        listclk.clear();

        // --------------- Temperatures ---------------

        // remove expired data from history
        while (1)
        {
            auto it=listtmp.begin();
            if (it==listtmp.end() || it->first+delay>tm)
                break;
            listtmp.pop_front();
        }

        // Structure for dim service
        struct Temp
        {
            uint16_t num;
            float val[160];
            Temp() { memset(this, 0, sizeof(Temp)); }
        } __attribute__((__packed__));

        // Calculate average and fll structure
        vector<uint32_t> tmpnum(160);

        Temp avgtmp;
        avgtmp.num = listtmp.size();
        for (auto it=listtmp.begin(); it!=listtmp.end(); it++)
            for (int i=0; i<160; i++)
                if (it->second[i]!=INT16_MIN)
                {
                    avgtmp.val[i] += it->second[i];
                    tmpnum[i]++;
                }
        for (int i=0; i<160; i++)
            avgtmp.val[i] /= tmpnum[i]*16;

        // Update dim service
        fDimTemperature.setData(avgtmp);
        fDimTemperature.Update(tm);

        listtmp.clear();

        return true;
    }

    bool eventCheck(const EVT_CTRL2 &evt)
    {
        const EVENT *event = evt.fEvent;

        const Time tm(evt.time);

	const array<uint16_t,2> roi = {{ event->Roi, event->RoiTM }};

	if (roi!=fVecRoi)
        {
            fQueueRoi.emplace(tm, roi);
	    fVecRoi = roi;
	}

        const FAD::EventHeader *beg = reinterpret_cast<const FAD::EventHeader*>(evt.FADhead);
        const FAD::EventHeader *end = reinterpret_cast<const FAD::EventHeader*>(evt.FADhead)+40;

        // FIMXE: Compare with target configuration

        // Copy data to array
        array<uint32_t,40> clk;
        array<int16_t,160> tmp;

        for (int i=0; i<40; i++)
            clk[i] = UINT32_MAX;

        for (int i=0; i<160; i++)
            tmp[i] = INT16_MIN;

        //fill(clk.data(), clk.data()+ 40, UINT32_MAX);
        //fill(tmp.data(), tmp.data()+160,  INT16_MIN);

        for (const FAD::EventHeader *ptr=beg; ptr!=end; ptr++)
        {
            // FIXME: Compare with expectations!!!
            if (ptr->fStartDelimiter==0)
            {
                if (ptr==beg)
                    beg++;
                continue;
            }

            clk[ptr->Id()] = ptr->fFreqRefClock;
            for (int i=0; i<4; i++)
                tmp[ptr->Id()*4+i] = ptr->fTempDrs[i];

            if (beg->fStatus != ptr->fStatus)
            {
                fMsg.Error("Inconsistency in FAD status detected.... closing run.");
                return false;
            }

            if (beg->fRunNumber != ptr->fRunNumber)
            {
                fMsg.Error("Inconsistent run number detected.... closing run.");
                return false;
            }

            /*
            if (beg->fVersion != ptr->fVersion)
            {
                Error("Inconsist firmware version detected.... closing run.");
                CloseRunFile(runNr, 0, 0);
                break;
                }
                */
            if (beg->fEventCounter != ptr->fEventCounter)
            {
                fMsg.Error("Inconsistent FAD event number detected.... closing run.");
                return false;
            }

            if (beg->fTriggerCounter != ptr->fTriggerCounter)
            {
                fMsg.Error("Inconsistent FTM trigger number detected.... closing run.");
                return false;
            }

            // FIXME: Check with first event!
            if (beg->fAdcClockPhaseShift != ptr->fAdcClockPhaseShift)
            {
                fMsg.Error("Inconsistent phase shift detected.... closing run.");
                return false;
            }

            // FIXME: Check with first event!
            if (memcmp(beg->fDac, ptr->fDac, sizeof(beg->fDac)))
            {
                fMsg.Error("Inconsistent DAC values detected.... closing run.");
                return false;
            }

            if (beg->fTriggerType != ptr->fTriggerType)
            {
                fMsg.Error("Inconsistent trigger type detected.... closing run.");
                return false;
            }
        }

        // check REFCLK_frequency
        // check consistency with command configuration
        // how to log errors?
        // need gotNewRun/closedRun to know it is finished

        fQueueTempRefClk.emplace(tm, clk, tmp);

        if (evt.runCtrl->fileStat == kFileClosed)
        {
            static Time oldt(boost::date_time::neg_infin);
            if (tm>oldt+boost::posix_time::seconds(1))
            {
                fQueueTrigger.emplace(tm, 0, evt.runCtrl->triggerCounter);
                oldt = tm;
            }
        }

        return true;
    }

    Time fLastDimRawData;
    Time fLastDimEventData;

    bool UpdateDimRawData(const vector<char> &v)
    {
        const EVENT *evt = reinterpret_cast<const EVENT*>(v.data());

        fDimRawData.setData(v);
        fDimRawData.setQuality(evt->TriggerType);
        fDimRawData.Update(Time(evt->PCTime, evt->PCUsec));

        return true;
    }

    bool UpdateDimEventData(const tuple<Time,uint32_t,EventData/*array<float, 1440*4>*/> &tup)
    {
        fDimEventData.setQuality(get<1>(tup));
        fDimEventData.setData(get<2>(tup));
        fDimEventData.Update(get<0>(tup));

        return true;
    }

    void applyCalib(const EVT_CTRL2 &evt, const size_t &size)
    {
        const EVENT   *event = evt.fEvent;
        const int16_t *start = event->StartPix;

        // Get the reference to the run associated information
        RUN_CTRL2 &run = *evt.runCtrl;

        if (size==1) // If there is more than one event waiting (including this one), throw them away
        {
            Time now;

            // ------------------- Copy event data to new memory --------------------
            // (to make it thread safe; a static buffer might improve memory handling)
            const uint16_t roi = event->Roi;

            // ------------------- Apply full DRS calibration ------------------------
            // (Is that necessray, or would a simple offset correct do well already?)

            // This is a very important step. Making a copy of the shared pointer ensures
            // that another thread (here: runClose) can set a new shared_ptr with new
            // data without this thread being affected. If we just did run.calib->Apply
            // the shared_pointer in use here might vanash during the processing, the
            // memory is freed and we access invalid memory. It is not important
            // which memory we acces (the old or the new one) because it is just for
            // display purpose anyway.
            const shared_ptr<DrsCalibration> cal = run.calib;

            // There seems to be a problem using std::array... maybe the size is too big?
            // array<float, (1440+160)*1024> vec2;
            vector<float> vec((1440+160)*roi);
            cal->Apply(vec.data(), event->Adc_Data, start, roi);

            // ------------------- Appy DRS-step correction --------------------------
            for (auto it=run.prevStart.begin(); it!=run.prevStart.end(); it++)
            {
                DrsCalibrate::CorrectStep(vec.data(), 1440, roi, it->data(), start, roi+10);
                DrsCalibrate::CorrectStep(vec.data(), 1440, roi, it->data(), start, 3);
            }

            // ------------------------- Remove spikes --------------------------------
            DrsCalibrate::RemoveSpikes4(vec.data(), roi*1440);

            // -------------- Update raw data dim sevice (VERY SLOW) -----------------
            if (fQueueRawData.empty() && now>fLastDimRawData+boost::posix_time::seconds(5))
            {
                vector<char> data1(sizeof(EVENT)+vec.size()*sizeof(float));
                memcpy(data1.data(), event, sizeof(EVENT));
                memcpy(data1.data()+sizeof(EVENT), vec.data(), vec.size()*sizeof(float));
                fQueueRawData.emplace(data1);

                fLastDimRawData = now;
            }

            // ------------------------- Basic statistics -----------------------------
            DrsCalibrate::SlidingAverage(vec.data(), roi, 10);

            // If this is a cosmic event
            EventData edat;
            edat.runNum = evt.runNum;
            edat.evNum  = evt.evNum;
            //array<float, 1440*4> stats; // Mean, RMS, Max, Pos  // 60 to exclude time markers
            const float max = DrsCalibrate::GetPixelStats(edat.data, vec.data(), roi, 15, 60);
            if (evt.trgTyp==0 && max>fMaxEvent.first)
                fMaxEvent = make_pair(max, edat);

            // ------------------ Update dim service (statistics) ---------------------

            if (fQueueEventData.empty() && now>fLastDimEventData+boost::posix_time::milliseconds(4999))
            {
                edat.evNum  = evt.evNum;
                edat.runNum = evt.runNum;

                fQueueEventData.emplace(evt.time, evt.trgTyp, evt.trgTyp==0 ? fMaxEvent.second : edat);
                if (evt.trgTyp==0)
                    fMaxEvent.first = -FLT_MAX;

                fLastDimEventData = now;
            }

            // === SendFeedbackData(PEVNT_HEADER *fadhd, EVENT *event)
            //
            //    if (!ptr->HasTriggerLPext() && !ptr->HasTriggerLPint())
            //        return;
            //
            //    vector<float> data2(1440); // Mean, RMS, Max, Pos, first, last
            //    DrsCalibrate::GetPixelMax(data2.data(), data.data(), event->Roi, 0, event->Roi-1);
            //
            //    fDimFeedbackData.Update(data2);
        }

        // Keep the start cells of the last five events for further corrections
        // As a performance improvement we could also just store the
        // pointers to the last five events...
        // What if a new run is started? Do we mind?
        auto &l = run.prevStart; // History for start cells of previous events (for step calibration)

        if (l.size()<5)
            l.emplace_front();
        else
        {
            auto it = l.end();
            l.splice(l.begin(), l, --it);
        }

        memcpy(l.front().data(), start, 1440*sizeof(int16_t));
    }

    bool IsRunWaiting()
    {
        const lock_guard<mutex> lock(mtx_newrun);
        return fExpectedRuns.find(fRunNumber-1)!=fExpectedRuns.end();
    }

    uint32_t GetRunNumber() const
    {
        return fRunNumber;
    }

    bool IncreaseRunNumber(uint32_t run)
    {
        if (!InitRunNumber())
            return false;

        if (run<fRunNumber)
        {
            ostringstream msg;
            msg <<
                "Run number " << run << " smaller than next available "
                "run number " << fRunNumber << " in " << fPath << " [" << fNightAsInt << "]";
            fMsg.Error(msg);
            return false;
        }

        fRunNumber = run;

        return true;
    }

    void gotNewRun(RUN_CTRL2 &run)
    {
        // This is to secure iteration over fExpectedRuns
        const lock_guard<mutex> lock(mtx_newrun);

        map<uint32_t,FAD::RunDescription>::iterator it = fExpectedRuns.begin();
        while (it!=fExpectedRuns.end())
        {
            if (it->first<run.runId)
            {
                ostringstream str;
                str << "runOpen - Missed run " << it->first << ".";
                fMsg.Info(str);

                // Increase the iterator first, it becomes invalid with the next call
                const auto is = it++;
                fExpectedRuns.erase(is);
                continue;
            }

            if (it->first==run.runId)
                break;

            it++;
        }

        if (it==fExpectedRuns.end())
        {
            ostringstream str;
            str << "runOpen - Run " << run.runId << " wasn't expected (maybe manual triggers)";
            fMsg.Warn(str);

            // This is not ideal, but the best we can do
            run.night = fNightAsInt;

            return;
        }

        const FAD::RunDescription &conf = it->second;

        run.runType   = conf.name;
        run.maxEvt    = conf.maxevt;
        run.closeTime = conf.maxtime + run.openTime;
        run.night     = conf.night;

        fExpectedRuns.erase(it);

        // Now signal the fadctrl (configuration process that a run is in progress)
        // Maybe this could be done earlier, but we are talking about a
        // negligible time scale here.
        fRunInProgress = run.runId;
    }

    void runFinished()
    {
        // This is called when the last event of a run (run time exceeded or
        // max number of events exceeded) has been received.
        fRunInProgress = -1;
    }

    //map<boost::thread::id, string> fLastMessage;

    void factOut(int severity, const char *message)
    {
        ostringstream str;
        str << "EventBuilder: " << message;

        /*
        string &old = fLastMessage[boost::this_thread::get_id()];

        if (str.str()==old)
            return;
        old = str.str();
        */

        fMsg.Update(str, severity);
    }

/*
    void factStat(int64_t *stat, int len)
    {
        if (len!=7)
        {
            fMsg.Warn("factStat received unknown number of values.");
            return;
        }

        vector<int64_t> data(1, g_maxMem);
        data.insert(data.end(), stat, stat+len);

        static vector<int64_t> last(8);
        if (data==last)
            return;
        last = data;

        fDimStatistics.Update(data);

        //   len ist die Laenge des arrays.
        //   array[4] enthaelt wieviele bytes im Buffer aktuell belegt sind; daran
        //   kannst Du pruefen, ob die 100MB voll sind ....

        ostringstream str;
        str
            << "Wait=" << stat[0] << " "
            << "Skip=" << stat[1] << " "
            << "Del="  << stat[2] << " "
            << "Tot="  << stat[3] << " "
            << "Mem="  << stat[4] << "/" << g_maxMem << " "
            << "Read=" << stat[5] << " "
            << "Conn=" << stat[6];

        fMsg.Info(str);
    }
    */

    bool UpdateDimStatistics1(const pair<Time,GUI_STAT> &stat)
    {
        fDimStatistics1.setData(&stat.second, sizeof(GUI_STAT));
        fDimStatistics1.Update(stat.first);

        return true;
    }

    void factStat(const GUI_STAT &stat)
    {
        fQueueStatistics1.emplace(Time(), stat);
    }

    void factReportIncomplete(uint64_t rep)
    {
        fDimIncomplete.setQuality(1);
        fDimIncomplete.Update(rep);
    }

    array<FAD::EventHeader, 40> fVecHeader;

    template<typename T, class S>
    array<T, 42> Compare(const S *vec, const T *t)
    {
        const int offset = reinterpret_cast<const char *>(t) - reinterpret_cast<const char *>(vec);

        const T *min = NULL;
        const T *val = NULL;
        const T *max = NULL;

        array<T, 42> arr;

        // bool rc = true;
        for (int i=0; i<40; i++)
        {
            const char *base = reinterpret_cast<const char*>(vec+i);
            const T *ref = reinterpret_cast<const T*>(base+offset);

            arr[i] = *ref;

            if (gi_NumConnect[i]==0)
            {
                arr[i] = 0;
                continue;
            }

            if (!val)
            {
                min = ref;
                val = ref;
                max = ref;
            }

            if (*ref<*min)
                min = ref;

            if (*ref>*max)
                max = ref;

            // if (*val!=*ref)
            //     rc = false;
        }

        arr[40] = val ? *min : 1;
        arr[41] = val ? *max : 0;

        return arr;
    }

    template<typename T>
    array<T, 42> CompareBits(const FAD::EventHeader *h, const T *t)
    {
        const int offset = reinterpret_cast<const char *>(t) - reinterpret_cast<const char *>(h);

        T val = 0;
        T rc  = 0;

        array<T, 42> vec;

        bool first = true;

        for (int i=0; i<40; i++)
        {
            const char *base = reinterpret_cast<const char*>(&fVecHeader[i]);
            const T *ref = reinterpret_cast<const T*>(base+offset);

            vec[i+2] = *ref;

            if (gi_NumConnect[i]==0)
            {
                vec[i+2] = 0;
                continue;
            }

            if (first)
            {
                first = false;
                val = *ref;
                rc = 0;
            }

            rc |= val^*ref;
        }

        vec[0] = rc;
        vec[1] = val;

        return vec;
    }

    template<typename T, size_t N>
    void Update(DimDescribedService &svc, const array<T, N> &data, const Time &t=Time(), int n=N)
    {
        svc.setData(const_cast<T*>(data.data()), sizeof(T)*n);
        svc.Update(t);
    }

    template<typename T>
        void Print(const char *name, const pair<bool,array<T, 43>> &data)
    {
        cout << name << "|" << data.first << "|" << data.second[1] << "|" << data.second[0] << "<x<" << data.second[1] << ":";
        for (int i=0; i<40;i++)
            cout << " " << data.second[i+3];
        cout << endl;
    }

    vector<uint> fNumConnected;

    bool procHeader(const tuple<Time,bool,FAD::EventHeader> &dat)
    {
        const Time             &t = get<0>(dat);
        const bool        changed = get<1>(dat);
        const FAD::EventHeader &h = get<2>(dat);

        const FAD::EventHeader old = fVecHeader[h.Id()];
        fVecHeader[h.Id()] = h;

        if (old.fVersion != h.fVersion || changed)
        {
            const array<uint16_t,42> ver = Compare(&fVecHeader[0], &fVecHeader[0].fVersion);

            array<float,42> data;
            for (int i=0; i<42; i++)
            {
                ostringstream str;
                str << (ver[i]>>8) << '.' << (ver[i]&0xff);
                data[i] = stof(str.str());
            }
            Update(fDimFwVersion, data, t);
        }

        if (old.fRunNumber != h.fRunNumber || changed)
        {
            const array<uint32_t,42> run = Compare(&fVecHeader[0], &fVecHeader[0].fRunNumber);
            fDimRunNumber.setData(&run[0], 42*sizeof(uint32_t));
            fDimRunNumber.Update(t);
        }

        if (old.fTriggerGeneratorPrescaler != h.fTriggerGeneratorPrescaler || changed)
        {
            const array<uint16_t,42> pre = Compare(&fVecHeader[0], &fVecHeader[0].fTriggerGeneratorPrescaler);
            fDimPrescaler.setData(&pre[0], 42*sizeof(uint16_t));
            fDimPrescaler.Update(t);
        }

        if (old.fDNA != h.fDNA || changed)
        {
            const array<uint64_t,42> dna = Compare(&fVecHeader[0], &fVecHeader[0].fDNA);
            Update(fDimDNA, dna, t, 40);
        }

        if (old.fStatus != h.fStatus || changed)
        {
            const array<uint16_t,42> sts = CompareBits(&fVecHeader[0], &fVecHeader[0].fStatus);
            Update(fDimStatus, sts, t);
        }

        if (memcmp(old.fDac, h.fDac, sizeof(h.fDac)) || changed)
        {
            array<uint16_t, FAD::kNumDac*42> dacs;

            for (int i=0; i<FAD::kNumDac; i++)
            {
                const array<uint16_t, 42> dac = Compare(&fVecHeader[0], &fVecHeader[0].fDac[i]);
                memcpy(&dacs[i*42], &dac[0], sizeof(uint16_t)*42);
            }

            Update(fDimDac, dacs, t);
        }

        return true;
    }

    void debugHead(const FAD::EventHeader &h)
    {
        const uint16_t id = h.Id();
        if (id>39) 
            return;

        if (fNumConnected.size()!=40)
	    fNumConnected.resize(40);

	const vector<uint> con(gi_NumConnect, gi_NumConnect+40);

	const bool changed = con!=fNumConnected || !IsThreadRunning();

        fNumConnected = con;

        fQueueProcHeader.emplace(Time(), changed, h);
    }
};

EventBuilderWrapper *EventBuilderWrapper::This = 0;

// ----------- Event builder callbacks implementation ---------------
bool runOpen(const EVT_CTRL2 &evt)
{
    return EventBuilderWrapper::This->runOpen(evt);
}

bool runWrite(const EVT_CTRL2 &evt)
{
    return EventBuilderWrapper::This->runWrite(evt);
}

void runClose(const EVT_CTRL2 &evt)
{
    EventBuilderWrapper::This->runClose(evt);
}

bool eventCheck(const EVT_CTRL2 &evt)
{
    return EventBuilderWrapper::This->eventCheck(evt);
}

void gotNewRun(RUN_CTRL2 &run)
{
    EventBuilderWrapper::This->gotNewRun(run);
}

void runFinished()
{
    EventBuilderWrapper::This->runFinished();
}

void applyCalib(const EVT_CTRL2 &evt, const size_t &size)
{
    EventBuilderWrapper::This->applyCalib(evt, size);
}

void factOut(int severity, const char *message)
{
    EventBuilderWrapper::This->factOut(severity, message);
}

void factStat(const GUI_STAT &stat)
{
    EventBuilderWrapper::This->factStat(stat);
}

void factReportIncomplete(uint64_t rep)
{
    EventBuilderWrapper::This->factReportIncomplete(rep);
}

// ------

void debugHead(void *buf)
{
    const FAD::EventHeader &h = *reinterpret_cast<FAD::EventHeader*>(buf);
    EventBuilderWrapper::This->debugHead(h);
}

#endif
