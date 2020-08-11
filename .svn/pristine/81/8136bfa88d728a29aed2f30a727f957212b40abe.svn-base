#ifndef FACT_EventBuilder
#define FACT_EventBuilder

#include "FAD.h"

#include <list>
#include <array>
#include <forward_list>

namespace std
{
    class mutex;
}


/* global variables; 
   to avoid race canoditions, only one thread is allowed to write 
   the name of the variable defines which process shall write it:

   g_XXX : main control thread
   gi_XX : input thread (reading from camera)
   gw_XX : write thread (writing to disk)
   qp_XX : processing thread(s) (processing data, eg. soft-trig)

*/
extern int  g_reset     ;  //>0 = reset different levels of eventbuilder
extern size_t g_maxMem  ;  //maximum memory allowed for buffer
extern uint16_t g_evtTimeout;  //timeout (sec) for one event

extern FACT_SOCK g_port[NBOARDS] ;  // .port = baseport, .addr=string of IP-addr in dotted-decimal "ddd.ddd.ddd.ddd"

extern uint gi_NumConnect[NBOARDS];   //4 crates * 10 boards

struct DrsCalibration;

enum FileStatus_t
{
    kFileNotYetOpen,
    kFileOpen,
    kFileClosed
};

enum CloseRequest_t
{
    kRequestNone             =    0,
    kRequestManual           = 1<<1,
    kRequestTimeout          = 1<<2,
    kRequestConnectionChange = 1<<3,
    kRequestEventCheckFailed = 1<<4,
    kRequestMaxEvtsReached   = 1<<5,
    kRequestMaxTimeReached   = 1<<6
};


struct RUN_CTRL2
{
    int64_t runId ;      // Run number

    time_t reportMem;        // initMemory has reported no memory once (set outside of class)

    time_t openTime;     // Time when first event (first board) was received
    time_t lastTime;     // Time when last event was received (set when first board data received)
    time_t closeTime;    // Time when run should be closed
    uint32_t night;      // night as int as determined for this run

    uint32_t lastEvt;    // number of events received (counted when the first board was received)
    uint32_t maxEvt;     // maximum number which should be written to file

    uint16_t roi0;       // roi for normal pixels
    uint16_t roi8;       // roi for pixels8

    std::string runType;

    FileStatus_t fileStat;

    std::array<uint32_t, 8> triggerCounter;  // triggerCounter must only be manipulated in procEvt to keep it thread safe

    std::shared_ptr<DrsCalibration> calib;
    std::list<std::array<int16_t,1440>> prevStart; // History for start cells of previous events (for step calibration)

    RUN_CTRL2() : runId(-1), reportMem(0), lastTime(0), lastEvt(0), maxEvt(1<<31), fileStat(kFileNotYetOpen)
    {
        triggerCounter.fill(0);

        // runId   = -1;
        // fileId  = kFileNotYetOpen;
        // lastEvt =  0;    // Number of events partially started to read
        // actEvt  =  0;    // Number of written events
        // maxEvt  = 1<<31; // max number events allowed (~2400min @ 250Hz)

    }
    //~RUN_CTRL2();
};

#define MAX_HEAD_MEM (NBOARDS * sizeof(PEVNT_HEADER))
#define MAX_TOT_MEM (sizeof(EVENT) + (NPIX+NTMARK)*1024*2 + MAX_HEAD_MEM)

namespace Memory
{
    extern uint64_t inuse;
    extern uint64_t allocated;

    extern uint64_t max_inuse;

    extern std::mutex mtx;

    extern std::forward_list<void*> memory;

    extern void *malloc();
    extern void  free(void *mem);
};

struct EVT_CTRL2
{
    uint32_t  runNum;  // header->runnumber;
    uint32_t  evNum;   // header->fad_evt_counter

    uint32_t  trgNum;  // header->trigger_id
    uint32_t  trgTyp;  // header->trigger_type
    uint32_t  fadLen;

    //uint16_t  firstBoard; // first board from which data was received
    uint16_t  nBoard;
    int16_t   board[NBOARDS];

    uint16_t  nRoi;
    uint16_t  nRoiTM;

    timeval   time;

    PEVNT_HEADER *FADhead; // Pointer to the whole allocated memory
    EVENT        *fEvent;  // Pointer to the event data itself
    PEVNT_HEADER *header;  // Pointer to a valid header within FADhead

    int closeRequest;

    std::array<uint32_t, 8> triggerCounter;  // triggerCounter must only be manipulated in procEvt to keep it thread safe

    std::shared_ptr<RUN_CTRL2> runCtrl;

    // Be carefull with this constructor... writeEvt can seg fault
    // it gets an empty runCtrl
    EVT_CTRL2() : nBoard(0), FADhead(0), header(0), closeRequest(kRequestNone)
    {
        //flag all boards as unused
        std::fill(board,  board+NBOARDS, -1);
    }
    /*
    EVT_CTRL2(CloseRequest_t req) : nBoard(0), FADhead(0), header(0), reportMem(false), closeRequest(req), runCtrl(new RUN_CTRL2)
    {
        //flag all boards as unused
        std::fill(board, board+NBOARDS, -1);
        }*/

    EVT_CTRL2(int req, const std::shared_ptr<RUN_CTRL2> &run) : nBoard(0), FADhead(0), header(0), closeRequest(req), runCtrl(run)
    {
        //flag all boards as unused
        std::fill(board, board+NBOARDS, -1);
    }
    ~EVT_CTRL2()
    {
        Memory::free(FADhead);
    }

    operator RUN_HEAD() const
    {
        RUN_HEAD rh;

        rh.Nroi    = nRoi;
        rh.NroiTM  = nRoiTM;
        rh.RunTime = time.tv_sec;
        rh.RunUsec = time.tv_usec;

        memcpy(rh.FADhead, FADhead, NBOARDS*sizeof(PEVNT_HEADER));

        return rh;
    }

    bool valid() const { return header; }

    bool initMemory()
    {
        // We have a valid entry, but no memory has yet been allocated
        if (FADhead)
            return true;

        FADhead = (PEVNT_HEADER*)Memory::malloc();
        if (!FADhead)
            return false;

        fEvent = reinterpret_cast<EVENT*>(FADhead+NBOARDS);

        memset(FADhead, 0, (NPIX+NTMARK)*2*nRoi+NBOARDS*sizeof(PEVNT_HEADER)+sizeof(EVENT));

        //flag all pixels as unused, flag all TMark as unused
        std::fill(fEvent->StartPix, fEvent->StartPix+NPIX,   -1);
        std::fill(fEvent->StartTM,  fEvent->StartTM +NTMARK, -1);

        fEvent->Roi         = nRoi;
        fEvent->RoiTM       = nRoiTM;
        fEvent->EventNum    = evNum;
        fEvent->TriggerNum  = trgNum;
        fEvent->TriggerType = trgTyp;

        return true;
    }
};

#endif
