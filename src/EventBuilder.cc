#include <poll.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

#include <cstring>
#include <cstdarg>
#include <list>
#include <queue>
#include <functional> // std::bind

#include <boost/algorithm/string/join.hpp>

#include "../externals/Queue.h"

#include "MessageImp.h"
#include "EventBuilder.h"
#include "HeadersFAD.h"

using namespace std;

#define MIN_LEN  32    // min #bytes needed to interpret FADheader
#define MAX_LEN  81920 // one max evt = 1024*2*36 + 8*36 + 72 + 4 = 74092  (data+boardheader+eventheader+endflag)

//#define COMPLETE_EVENTS
//#define USE_POLL
//#define USE_EPOLL
//#define USE_SELECT
//#define COMPLETE_EPOLL
//#define PRIORITY_QUEUE

// Reading only 1024: 13:  77Hz, 87%
// Reading only 1024: 12:  78Hz, 46%
// Reading only  300:  4: 250Hz, 92%
// Reading only  300:  3: 258Hz, 40%

// Reading only four threads 1024: 13:  77Hz, 60%
// Reading only four threads 1024: 12:  78Hz, 46%
// Reading only four threads  300:  4: 250Hz, 92%
// Reading only four threads  300:  3: 258Hz, 40%

// Default  300:  4: 249Hz, 92%
// Default  300:  3: 261Hz, 40%
// Default 1024: 13:  76Hz, 93%
// Default 1024: 12:  79Hz, 46%

// Poll [selected] 1024: 13:  63Hz, 45%
// Poll [selected] 1024: 14:  63Hz, 63%
// Poll [selected] 1024: 15:  64Hz, 80%
// Poll [selected]  300:  4: 230Hz, 47%
// Poll [selected]  300:  3: 200Hz, 94%

// Poll [all]      1024: 13:  65Hz, 47%
// Poll [all]      1024: 14:  64Hz, 59%
// Poll [all]      1024: 15:  62Hz, 67%
// Poll [all]       300:  4: 230Hz, 47%
// Poll [all]       300:  3: 230Hz, 35%

// ==========================================================================

bool runOpen(const EVT_CTRL2 &evt);
bool runWrite(const EVT_CTRL2 &evt);
void runClose(const EVT_CTRL2 &run);
void applyCalib(const EVT_CTRL2 &evt, const size_t &size);
void factOut(int severity, const char *message);
void factReportIncomplete (uint64_t rep);
void gotNewRun(RUN_CTRL2 &run);
void runFinished();
void factStat(const GUI_STAT &gj);
bool eventCheck(const EVT_CTRL2 &evt);
void debugHead(void *buf);

// ==========================================================================

int g_reset;

size_t g_maxMem;                //maximum memory allowed for buffer

uint16_t g_evtTimeout;           // timeout (sec) for one event

FACT_SOCK g_port[NBOARDS];      // .addr=string of IP-addr in dotted-decimal "ddd.ddd.ddd.ddd"

uint gi_NumConnect[NBOARDS];    //4 crates * 10 boards

GUI_STAT gj;

// ==========================================================================

namespace Memory
{
    uint64_t inuse     = 0;
    uint64_t allocated = 0;

    uint64_t max_inuse = 0;

    std::mutex mtx;

    std::forward_list<void*> memory;

    void *malloc()
    {
        // No free slot available, next alloc would exceed max memory
        if (memory.empty() && allocated+MAX_TOT_MEM>g_maxMem)
            return NULL;

        // We will return this amount of memory
        // This is not 100% thread safe, but it is not a super accurate measure anyway
        inuse += MAX_TOT_MEM;
        if (inuse>max_inuse)
            max_inuse = inuse;

        if (memory.empty())
        {
            // No free slot available, allocate a new one
            allocated += MAX_TOT_MEM;
            return  new char[MAX_TOT_MEM];
        }

        // Get the next free slot from the stack and return it
        const std::lock_guard<std::mutex> lock(mtx);

        void *mem = memory.front();
        memory.pop_front();
        return mem;
    };

    void free(void *mem)
    {
        if (!mem)
            return;

        // Decrease the amont of memory in use accordingly
        inuse -= MAX_TOT_MEM;

        // If the maximum memory has changed, we might be over the limit.
        // In this case: free a slot
        if (allocated>g_maxMem)
        {
            delete [] (char*)mem;
            allocated -= MAX_TOT_MEM;
            return;
        }

        const std::lock_guard<std::mutex> lock(mtx);
        memory.push_front(mem);
    }

};

// ==========================================================================

__attribute__((__format__ (__printf__, 2, 0)))
void factPrintf(int severity, const char *fmt, ...)
{
    char str[1000];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(str, 1000, fmt, ap);
    va_end(ap);

    factOut(severity, str);
}

// ==========================================================================

struct READ_STRUCT
{
    enum buftyp_t
    {
        kStream,
        kHeader,
        kData,
#ifdef COMPLETE_EVENTS
        kWait
#endif
    };

    // ---------- connection ----------

    static uint activeSockets;

    int  sockId;       // socket id (board number)
    int  socket;       // socket handle
    bool connected;    // is this socket connected?

    struct sockaddr_in SockAddr;  // Socket address copied from wrapper during socket creation

    // ------------ epoll -------------

    static int  fd_epoll;
    static epoll_event events[NBOARDS];

    static void init();
    static void close();
    static int  wait();
    static READ_STRUCT *get(int i) { return reinterpret_cast<READ_STRUCT*>(events[i].data.ptr); }

    // ------------ buffer ------------

    buftyp_t  bufTyp;  // what are we reading at the moment: 0=header 1=data -1=skip ...

    uint32_t  bufLen;  // number of bytes left to read
    uint8_t  *bufPos;  // next byte to read to the buffer next

    union
    {
        uint8_t  B[MAX_LEN];
        uint16_t S[MAX_LEN / 2];
        uint32_t I[MAX_LEN / 4];
        uint64_t L[MAX_LEN / 8];
        PEVNT_HEADER H;
    };

    timeval  time;
    uint64_t totBytes;  // total received bytes
    uint64_t relBytes;  // total released bytes
    uint32_t skip;      // number of bytes skipped before start of event

    uint32_t len() const { return uint32_t(H.package_length)*2; }

    void swapHeader();
    void swapData();

    // --------------------------------

    READ_STRUCT() : socket(-1), connected(false), totBytes(0), relBytes(0)
    {
        if (fd_epoll<0)
            init();
    }
    ~READ_STRUCT()
    {
        destroy();
    }

    void destroy();
    bool create(sockaddr_in addr);
    bool check(int, sockaddr_in addr);
    bool read();

};

#ifdef PRIORITY_QUEUE
struct READ_STRUCTcomp
{
    bool operator()(const READ_STRUCT *r1, const READ_STRUCT *r2)
    {
        const int64_t rel1 = r1->totBytes - r1->relBytes;
        const int64_t rel2 = r2->totBytes - r2->relBytes;
        return rel1 > rel2;
    }
};
#endif

int READ_STRUCT::wait()
{
    // wait for something to do...
    const int rc = epoll_wait(fd_epoll, events, NBOARDS, 100); // max, timeout[ms]
    if (rc>=0)
        return rc;

    if (errno==EINTR) // timout or signal interruption
        return 0;

    factPrintf(MessageImp::kError, "epoll_wait failed: %m (rc=%d)", errno);
    return -1;
}

uint READ_STRUCT::activeSockets = 0;
int READ_STRUCT::fd_epoll = -1;
epoll_event READ_STRUCT::events[NBOARDS];

void READ_STRUCT::init()
{
    if (fd_epoll>=0)
        return;

#ifdef USE_EPOLL
    fd_epoll = epoll_create(NBOARDS);
    if (fd_epoll<0)
    {
        factPrintf(MessageImp::kError, "Waiting for data failed: %d (epoll_create,rc=%d)", errno);
        return;
    }
#endif
}

void READ_STRUCT::close()
{
#ifdef USE_EPOLL
    if (fd_epoll>=0 && ::close(fd_epoll)>0)
        factPrintf(MessageImp::kFatal, "Closing epoll failed: %m (close,rc=%d)", errno);
#endif

    fd_epoll = -1;
}

bool READ_STRUCT::create(sockaddr_in sockAddr)
{
    if (socket>=0)
        return false;

    const int port = ntohs(sockAddr.sin_port) + 1;

    SockAddr.sin_family = sockAddr.sin_family;
    SockAddr.sin_addr   = sockAddr.sin_addr;
    SockAddr.sin_port   = htons(port);

    if ((socket = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) <= 0)
    {
        factPrintf(MessageImp::kFatal, "Generating socket %d failed: %m (socket,rc=%d)", sockId, errno);
        socket = -1;
        return false;
    }

    int optval = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)) < 0)
        factPrintf(MessageImp::kInfo, "Setting TCP_NODELAY for socket %d failed: %m (setsockopt,rc=%d)", sockId, errno);

    optval = 1;
    if (setsockopt (socket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0)
        factPrintf(MessageImp::kInfo, "Setting SO_KEEPALIVE for socket %d failed: %m (setsockopt,rc=%d)", sockId, errno);

    optval = 10;                 //start after 10 seconds
    if (setsockopt (socket, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(int)) < 0)
        factPrintf(MessageImp::kInfo, "Setting TCP_KEEPIDLE for socket %d failed: %m (setsockopt,rc=%d)", sockId, errno);

    optval = 10;                 //do every 10 seconds
    if (setsockopt (socket, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(int)) < 0)
        factPrintf(MessageImp::kInfo, "Setting TCP_KEEPINTVL for socket %d failed: %m (setsockopt,rc=%d)", sockId, errno);

    optval = 2;                  //close after 2 unsuccessful tries
    if (setsockopt (socket, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(int)) < 0)
        factPrintf(MessageImp::kInfo, "Setting TCP_KEEPCNT for socket %d failed: %m (setsockopt,rc=%d)", sockId, errno);

    factPrintf(MessageImp::kInfo, "Generated socket %d (%d)", sockId, socket);

    //connected = false;
    activeSockets++;

    return true;
}

void READ_STRUCT::destroy()
{
    if (socket<0)
        return;

#ifdef USE_EPOLL
    // strictly speaking this should not be necessary
    if (fd_epoll>=0 && connected && epoll_ctl(fd_epoll, EPOLL_CTL_DEL, socket, NULL)<0)
        factPrintf(MessageImp::kError, "epoll_ctrl failed: %m (EPOLL_CTL_DEL,rc=%d)", errno);
#endif

    if (::close(socket) > 0)
        factPrintf(MessageImp::kFatal, "Closing socket %d failed: %m (close,rc=%d)", sockId, errno);
    else
        factPrintf(MessageImp::kInfo, "Closed socket %d (%d)", sockId, socket);

    // Set the socket to "not connected"
    socket = -1;
    connected = false;
    activeSockets--;
    bufLen = 0;
}

bool READ_STRUCT::check(int sockDef, sockaddr_in addr)
{
    // Continue in the most most likely case (performance)
    //if (socket>=0 && sockDef!=0 && connected)
    //    return;
    const int old = socket;

    // socket open, but should not be open
    if (socket>=0 && sockDef==0)
        destroy();

    // Socket closed, but should be open
    if (socket<0 && sockDef!=0)
        create(addr); //generate address and socket

    const bool retval = old!=socket;

    // Socket closed
    if (socket<0)
        return retval;

    // Socket open and connected: Nothing to do
    if (connected)
        return retval;

    //try to connect if not yet done
    const int rc = connect(socket, (struct sockaddr *) &SockAddr, sizeof(SockAddr));
    if (rc == -1)
        return retval;

    connected = true;

    if (sockDef<0)
    {
        bufTyp = READ_STRUCT::kStream; // full data to be skipped
        bufLen = MAX_LEN;              // huge for skipping
    }
    else
    {
        bufTyp = READ_STRUCT::kHeader;  // expect a header
        bufLen = sizeof(PEVNT_HEADER);  // max size to read at begining
    }

    bufPos   = B;  // no byte read so far
    skip     = 0;  // start empty
    totBytes = 0;
    relBytes = 0;

    factPrintf(MessageImp::kInfo, "Connected socket %d (%d)", sockId, socket);

#ifdef USE_EPOLL
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = this;  // user data (union: ev.ptr)
    if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, socket, &ev)<0)
        factPrintf(MessageImp::kError, "epoll_ctl failed: %m (EPOLL_CTL_ADD,rc=%d)", errno);
#endif

    return retval;
}

bool READ_STRUCT::read()
{
    if (!connected)
        return false;

    if (bufLen==0)
        return true;

    const int32_t jrd = recv(socket, bufPos, bufLen, MSG_DONTWAIT);
    // recv failed
    if (jrd<0)
    {
        // There was just nothing waiting
        if (errno==EWOULDBLOCK || errno==EAGAIN)
            return false;

        factPrintf(MessageImp::kError, "Reading from socket %d failed: %m (recv,rc=%d)", sockId, errno);
        return false;
    }

    // connection was closed ...
    if (jrd==0)
    {
        factPrintf(MessageImp::kInfo, "Socket %d closed by FAD", sockId);

        destroy();//DestroySocket(rd[i]); //generate address and socket
        return false;
    }

    totBytes += jrd;

    // are we skipping this board ...
    if (bufTyp==kStream)
        return false;

    if (bufPos==B)
        gettimeofday(&time, NULL);

    bufPos += jrd;  //==> prepare for continuation
    bufLen -= jrd;

    // not yet all read
    return bufLen==0;
}

void READ_STRUCT::swapHeader()
{
    S[1]  = ntohs(S[1]);    // package_length (bytes not swapped!)
    S[2]  = ntohs(S[2]);    // version_no
    S[3]  = ntohs(S[3]);    // PLLLCK
    S[4]  = ntohs(S[4]);    // trigger_crc
    S[5]  = ntohs(S[5]);    // trigger_type

    I[3]  = ntohl(I[3]);    // trigger_id
    I[4]  = ntohl(I[4]);    // fad_evt_counter
    I[5]  = ntohl(I[5]);    // REFCLK_frequency

    S[12] = ntohs(S[12]);   // board id
    S[13] = ntohs(S[13]);   // adc_clock_phase_shift
    S[14] = ntohs(S[14]);   // number_of_triggers_to_generate
    S[15] = ntohs(S[15]);   // trigger_generator_prescaler

    I[10] = ntohl(I[10]);   // runnumber;
    I[11] = ntohl(I[11]);   // time;

    // Use back inserter??
    for (int s=24; s<24+NTemp+NDAC; s++)
        S[s] = ntohs(S[s]); // drs_temperature / dac
}

void READ_STRUCT::swapData()
{
    // swapEventHeaderBytes: End of the header. to channels now

    int i = 36;
    for (int ePatchesCount = 0; ePatchesCount<4*9; ePatchesCount++)
    {
        S[i+0] = ntohs(S[i+0]);//id
        S[i+1] = ntohs(S[i+1]);//start_cell
        S[i+2] = ntohs(S[i+2]);//roi
        S[i+3] = ntohs(S[i+3]);//filling

        i += 4+S[i+2];//skip the pixel data
    }
}

// ==========================================================================

bool checkRoiConsistency(const READ_STRUCT &rd, uint16_t roi[])
{
    int xjr = -1;
    int xkr = -1;

    //points to the very first roi
    int roiPtr = sizeof(PEVNT_HEADER)/2 + 2;

    roi[0] = ntohs(rd.S[roiPtr]);

    for (int jr = 0; jr < 9; jr++)
    {
        roi[jr] = ntohs(rd.S[roiPtr]);

        if (roi[jr]>1024)
        {
            factPrintf(MessageImp::kError, "Illegal roi in channel %d (allowed: roi<=1024)", jr, roi[jr]);
            return false;
        }

        // Check that the roi of pixels jr are compatible with the one of pixel 0
        if (jr!=8 && roi[jr]!=roi[0])
        {
            xjr = jr;
            break;
        }

        // Check that the roi of all other DRS chips on boards are compatible
        for (int kr = 1; kr < 4; kr++)
        {
            const int kroi = ntohs(rd.S[roiPtr]);
            if (kroi != roi[jr])
            {
                xjr = jr;
                xkr = kr;
                break;
            }
            roiPtr += kroi+4;
        }
    }

    if (xjr>=0)
    {
        if (xkr<0)
            factPrintf(MessageImp::kFatal, "Inconsistent Roi accross chips [DRS=%d], expected %d, got %d", xjr, roi[0], roi[xjr]);
        else
            factPrintf(MessageImp::kFatal, "Inconsistent Roi accross channels [DRS=%d Ch=%d], expected %d, got %d", xjr, xkr, roi[xjr], ntohs(rd.S[roiPtr]));

        return false;
    }

    if (roi[8] < roi[0])
    {
        factPrintf(MessageImp::kError, "Mismatch of roi (%d) in channel 8. Should be larger or equal than the roi (%d) in channel 0.", roi[8], roi[0]);
        return false;
    }

    return true;
}

list<shared_ptr<EVT_CTRL2>> evtCtrl;

shared_ptr<EVT_CTRL2> mBufEvt(const READ_STRUCT &rd, shared_ptr<RUN_CTRL2> &actrun)
{
    /*
     checkroi consistence
     find existing entry
     if no entry, try to allocate memory
     if entry and memory, init event structure
     */

    uint16_t nRoi[9];
    if (!checkRoiConsistency(rd, nRoi))
        return shared_ptr<EVT_CTRL2>();

    for (auto it=evtCtrl.rbegin(); it!=evtCtrl.rend(); it++)
    {
        // A reference is enough because the evtCtrl holds the shared_ptr anyway
        const shared_ptr<EVT_CTRL2> &evt = *it;

        // If the run is different, go on searching.
        // We cannot stop searching if a lower run-id is found as in
        // the case of the events, because theoretically, there
        // can be the same run on two different days.
        if (rd.H.runnumber != evt->runNum)
            continue;

        // If the ID of the new event if higher than the last one stored
        // in that run, we have to assign a new slot (leave the loop)
        if (rd.H.fad_evt_counter > evt->evNum/* && runID == evtCtrl[k].runNum*/)
            break;

        if (rd.H.fad_evt_counter != evt->evNum/* || runID != evtCtrl[k].runNum*/)
            continue;

        // We have found an entry with the same runID and evtID
        // Check if ROI is consistent
        if (evt->nRoi != nRoi[0] || evt->nRoiTM != nRoi[8])
        {
            factPrintf(MessageImp::kError, "Mismatch of roi within event. Expected roi=%d and roi_tm=%d, got %d and %d.",
                       evt->nRoi, evt->nRoiTM, nRoi[0], nRoi[8]);
            return shared_ptr<EVT_CTRL2>();
        }

        // It is maybe not likely, but the header of this board might have
        // arrived earlier. (We could also update the run-info, but
        // this should not make a difference here)
        if ((rd.time.tv_sec==evt->time.tv_sec && rd.time.tv_usec<evt->time.tv_usec) ||
            rd.time.tv_sec<evt->time.tv_sec)
            evt->time = rd.time;

        //everything seems fine so far ==> use this slot ....
        return evt;
    }

    if (actrun->runId==rd.H.runnumber && (actrun->roi0 != nRoi[0] || actrun->roi8 != nRoi[8]))
    {
        factPrintf(MessageImp::kError, "Mismatch of roi within run. Expected roi=%d and roi_tm=%d, got %d and %d (runID=%d, evID=%d)",
                   actrun->roi0, actrun->roi8, nRoi[0], nRoi[8], rd.H.runnumber, rd.H.fad_evt_counter);
        return shared_ptr<EVT_CTRL2>();
    }

    EVT_CTRL2 *evt = new EVT_CTRL2;

    evt->time   = rd.time;

    evt->runNum = rd.H.runnumber;
    evt->evNum  = rd.H.fad_evt_counter;

    evt->trgNum = rd.H.trigger_id;
    evt->trgTyp = rd.H.trigger_type;

    evt->nRoi   = nRoi[0];
    evt->nRoiTM = nRoi[8];

    //evt->firstBoard = rd.sockId;

    const bool newrun = actrun->runId != rd.H.runnumber;
    if (newrun)
    {
        factPrintf(MessageImp::kInfo, "New run %d (evt=%d) registered with roi=%d(%d), prev=%d",
                   rd.H.runnumber, rd.H.fad_evt_counter, nRoi[0], nRoi[8], actrun->runId);

        // The new run is the active run now
        actrun = make_shared<RUN_CTRL2>();

        const time_t &tsec = evt->time.tv_sec;

        actrun->openTime  = tsec;
        actrun->closeTime = tsec + 3600 * 24; // max time allowed
        actrun->runId     = rd.H.runnumber;
        actrun->roi0      = nRoi[0];  // FIXME: Make obsolete!
        actrun->roi8      = nRoi[8];  // FIXME: Make obsolete!

        // Signal the fadctrl that a new run has been started
        // Note this is the only place at which we can ensure that
        // gotnewRun is called only once
        gotNewRun(*actrun);
    }

    // Keep pointer to run of this event
    evt->runCtrl = actrun;

    // Increase the number of events we have started to receive in this run
    actrun->lastTime = evt->time.tv_sec;  // Time when the last event was received
    actrun->lastEvt++;

    // An event can be the first and the last, but not the last and the first.
    // Therefore gotNewRun is called before runFinished.
    // runFinished signals that the last event of a run was just received. Processing
    // might still be ongoing, but we can start a new run.
    const bool cond1 = actrun->lastEvt  < actrun->maxEvt;     // max number of events not reached
    const bool cond2 = actrun->lastTime < actrun->closeTime;  // max time not reached
    if (!cond1 || !cond2)
        runFinished();

    // We don't mind here that this is not common to all events,
    // because every coming event will fullfil the condition as well.
    if (!cond1)
        evt->closeRequest |= kRequestMaxEvtsReached;
    if (!cond2)
        evt->closeRequest |= kRequestMaxTimeReached;

    // Secure access to evtCtrl against access in CloseRunFile
    // This should be the last... otherwise we can run into threading issues
    // if the event is accessed before it is fully initialized.
    evtCtrl.emplace_back(evt);
    return evtCtrl.back();
}


void copyData(const READ_STRUCT &rBuf, EVT_CTRL2 *evt)
{
    const int i = rBuf.sockId;

    memcpy(evt->FADhead+i, &rBuf.H, sizeof(PEVNT_HEADER));

    int src = sizeof(PEVNT_HEADER) / 2;  // Header is 72 byte = 36 shorts

    // consistency of ROIs have been checked already (is it all correct?)
    const uint16_t &roi = rBuf.S[src+2];

    // different sort in FAD board.....
    EVENT *event = evt->fEvent;
    for (int px = 0; px < 9; px++)
    {
        for (int drs = 0; drs < 4; drs++)
        {
            const int16_t pixC = rBuf.S[src+1];    // start-cell
            const int16_t pixR = rBuf.S[src+2];    // roi
            //here we should check if pixH is correct ....

            const int pixS = i*36 + drs*9 + px;

            event->StartPix[pixS] = pixC;

            memcpy(event->Adc_Data + pixS*roi, &rBuf.S[src+4], roi * 2);

            src += 4+pixR;

            // Treatment for ch 9 (TM channel)
            if (px != 8)
                continue;

            const int tmS = i*4 + drs;

            //and we have additional TM info
            if (pixR > roi)
            {
                event->StartTM[tmS] = (pixC + pixR - roi) % 1024;

                memcpy(event->Adc_Data + tmS*roi + NPIX*roi, &rBuf.S[src - roi], roi * 2);
            }
            else
            {
                event->StartTM[tmS] = -1;
            }
        }
    }
}

// ==========================================================================

uint64_t reportIncomplete(const shared_ptr<EVT_CTRL2> &evt, const char *txt)
{
    factPrintf(MessageImp::kWarn, "skip incomplete evt (run=%d, evt=%d, n=%d, %s)",
               evt->runNum, evt->evNum, evtCtrl.size(), txt);

    uint64_t report = 0;

    char str[1000];

    int ik=0;
    for (int ib=0; ib<NBOARDS; ib++)
    {
        if (ib%10==0)
            str[ik++] = '|';

        const int jb = evt->board[ib];
        if (jb>=0) // data received from that board
        {
            str[ik++] = '0'+(jb%10);
            continue;
        }

        // FIXME: This is not synchronous... it reports
        // accoridng to the current connection status, not w.r.t. to the
        // one when the event was taken.
        if (gi_NumConnect[ib]==0) // board not connected
        {
            str[ik++] = 'x';
            continue;
        }

        // data from this board lost
        str[ik++] = '.';
        report |= ((uint64_t)1)<<ib;
    }

    str[ik++] = '|';
    str[ik]   = 0;

    factOut(MessageImp::kWarn, str);

    return report;
}

// ==========================================================================
// ==========================================================================

bool proc1(const shared_ptr<EVT_CTRL2> &);

Queue<shared_ptr<EVT_CTRL2>> processingQueue1(bind(&proc1, placeholders::_1));

bool proc1(const shared_ptr<EVT_CTRL2> &evt)
{
    applyCalib(*evt, processingQueue1.size());
    return true;
}

// If this is not convenient anymore, it could be replaced by
// a command queue, to which command+data is posted,
// (e.g. runOpen+runInfo, runClose+runInfo, evtWrite+evtInfo)
bool writeEvt(const shared_ptr<EVT_CTRL2> &evt)
{
    //const shared_ptr<RUN_CTRL2> &run = evt->runCtrl;
    RUN_CTRL2 &run = *evt->runCtrl;

    // Is this a valid event or just an empty event to trigger run close?
    // If this is not an empty event open the new run-file
    // Empty events are there to trigger run-closing conditions
    if (evt->valid())
    {
        // File not yet open
        if (run.fileStat==kFileNotYetOpen)
        {
            // runOpen will close a previous run, if still open
            if (!runOpen(*evt))
            {
                factPrintf(MessageImp::kError, "Could not open new file for run %d (evt=%d, runOpen failed)", evt->runNum, evt->evNum);
                run.fileStat = kFileClosed;
                return true;
            }

            factPrintf(MessageImp::kInfo, "Opened new file for data from run %d (evt=%d)", evt->runNum, evt->evNum);
            run.fileStat = kFileOpen;
        }

        // Here we have a valid calibration and can go on with that.
        // It is important that _all_ events are sent for calibration (except broken ones)
        processingQueue1.post(evt);
    }

    // File already closed
    if (run.fileStat==kFileClosed)
        return true;

    // If we will have a software trigger which prevents single events from writing,
    // the logic of writing the stop time and the trigger counters need to be adapted.
    // Currently it is just the values of the last valid event.
    bool rc1 = true;
    if (evt->valid())
    {
        rc1 = runWrite(*evt);
        if (!rc1)
            factPrintf(MessageImp::kError, "Writing event %d for run %d failed (runWrite)", evt->evNum, evt->runNum);
    }

    // File not open... no need to close or to check for close
    // ... this is the case if CloseRunFile was called before any file was opened.
    if (run.fileStat!=kFileOpen)
        return true;

    // File is not yet to be closed.
    if (rc1 && evt->closeRequest==kRequestNone)
        return true;

    runClose(*evt);
    run.fileStat = kFileClosed;

    vector<string> reason;
    if (evt->closeRequest&kRequestManual)
        reason.emplace_back("close was requested");
    if (evt->closeRequest&kRequestTimeout)
        reason.emplace_back("receive timed out");
    if (evt->closeRequest&kRequestConnectionChange)
        reason.emplace_back("connection changed");
    if (evt->closeRequest&kRequestEventCheckFailed)
        reason.emplace_back("event check failed");
    if (evt->closeRequest&kRequestMaxTimeReached)
        reason.push_back(to_string(run.closeTime-run.openTime)+"s had been reached");
    if (evt->closeRequest&kRequestMaxEvtsReached)
        reason.push_back(to_string(run.maxEvt)+" evts had been reached");
    if (!rc1)
        reason.emplace_back("runWrite failed");

    const string str = boost::algorithm::join(reason, ", ");
    factPrintf(MessageImp::kInfo, "File %d was closed because %s", run.runId,  str.c_str());

    return true;
}

Queue<shared_ptr<EVT_CTRL2>> secondaryQueue(bind(&writeEvt, placeholders::_1));

bool procEvt(const shared_ptr<EVT_CTRL2> &evt)
{
    RUN_CTRL2 &run = *evt->runCtrl;

    bool check = true;
    if (evt->valid())
    {
        EVENT *event = evt->fEvent;

        // This is already done in initMemory()
        //event->Roi         = evt->runCtrl->roi0;
        //event->RoiTM       = evt->runCtrl->roi8;
        //event->EventNum    = evt->evNum;
        //event->TriggerNum  = evt->trgNum;
        //event->TriggerType = evt->trgTyp;

        event->NumBoards = evt->nBoard;

        event->PCTime = evt->time.tv_sec;
        event->PCUsec = evt->time.tv_usec;

        for (int ib=0; ib<NBOARDS; ib++)
            event->BoardTime[ib] = evt->FADhead[ib].time;

        check = eventCheck(*evt);

        // If the event is valid, increase the trigger counter accordingly
        if (check)
        {
            // Physics trigger
            if (evt->trgTyp && !(evt->trgTyp & FAD::EventHeader::kAll))
                run.triggerCounter[0]++;
            // Pure pedestal trigger
            else  if ((evt->trgTyp&FAD::EventHeader::kPedestal) && !(evt->trgTyp&FAD::EventHeader::kTIM))
                run.triggerCounter[1]++;
            // external light pulser trigger
            else if (evt->trgTyp & FAD::EventHeader::kLPext)
                run.triggerCounter[2]++;
            // time calibration triggers
            else if (evt->trgTyp & (FAD::EventHeader::kTIM|FAD::EventHeader::kPedestal))
                run.triggerCounter[3]++;
            // internal light pulser trigger
            else if (evt->trgTyp & FAD::EventHeader::kLPint)
                run.triggerCounter[4]++;
            // external trigger input 1
            else if (evt->trgTyp & FAD::EventHeader::kExt1)
                run.triggerCounter[5]++;
            // external trigger input 2
            else if (evt->trgTyp & FAD::EventHeader::kExt2)
                run.triggerCounter[6]++;
            // other triggers
            else
                run.triggerCounter[7]++;
        }
    }

    // If this is an invalid event, the current triggerCounter needs to be copied
    // because runClose will use that one to update the TRIGGER_COUNTER.
    // When closing the file, the trigger counter of the last successfully
    // written event is used.
    evt->triggerCounter = run.triggerCounter;

    // If event check has failed, skip the event and post a close request instead.
    // Otherwise, if file is open post the event for being written
    if (!check)
        secondaryQueue.emplace(new EVT_CTRL2(kRequestEventCheckFailed, evt->runCtrl));
    else
        secondaryQueue.post(evt);

    return true;
}

// ==========================================================================
// ==========================================================================

/*
 task 1-4:

 lock1()-lock4();
 while (1)
 {
       wait for signal [lockN];  // unlocked

       while (n!=10)
         wait sockets;
         read;

       lockM();
       finished[n] = true;
       signal(mainloop);
       unlockM();
 }


 mainloop:

 while (1)
 {
       lockM();
       while (!finished[0] || !finished[1] ...)
          wait for signal [lockM];  // unlocked... signals can be sent
       finished[0-1] = false;
       unlockM()

       copy data to queue    // locked

       lockN[0-3];
       signalN[0-3];
       unlockN[0-3];
 }


 */

/*
    while (g_reset)
    {
        shared_ptr<EVT_CTRL2> evt = new shared_ptr<>;

        // Check that all sockets are connected

        for (int i=0; i<40; i++)
            if (rd[i].connected && epoll_ctl(fd_epoll, EPOLL_CTL_ADD, socket, NULL)<0)
               factPrintf(kError, "epoll_ctrl failed: %m (EPOLL_CTL_ADD,rc=%d)", errno);

        while (g_reset)
        {
           if (READ_STRUCT::wait()<0)
              break;

           if (rc_epoll==0)
              break;

           for (int jj=0; jj<rc_epoll; jj++)
           {
              READ_STRUCT *rs = READ_STRUCT::get(jj);
              if (!rs->connected)
                  continue;

              const bool rc_read = rs->read();
              if (!rc_read)
                  continue;

              if (rs->bufTyp==READ_STRUCT::kHeader)
              {
                  [...]
              }

              [...]

              if (epoll_ctl(fd_epoll, EPOLL_CTL_DEL, socket, NULL)<0)
                 factPrintf(kError, "epoll_ctrl failed: %m (EPOLL_CTL_DEL,rc=%d)", errno);
           }

           if (once_a_second)
           {
              if (evt==timeout)
                  break;
           }
        }

        if (evt.nBoards==actBoards)
            primaryQueue.post(evt);
    }
*/

Queue<shared_ptr<EVT_CTRL2>> primaryQueue(bind(&procEvt, placeholders::_1));

// This corresponds more or less to fFile... should we merge both?
shared_ptr<RUN_CTRL2> actrun;

void CloseRunFile()
{
    // Currently we need actrun here, to be able to set kFileClosed.
    // Apart from that we have to ensure that there is an open file at all
    // which we can close.
    // Submission to the primary queue ensures that the event
    // is placed at the right place in the processing chain.
    // (Corresponds to the correct run)
    primaryQueue.emplace(new EVT_CTRL2(kRequestManual, actrun));
}

bool mainloop(READ_STRUCT *rd)
{
    factPrintf(MessageImp::kInfo, "Starting EventBuilder main loop");

    primaryQueue.start();
    secondaryQueue.start();
    processingQueue1.start();;

    actrun = make_shared<RUN_CTRL2>();

    //time in seconds
    time_t gi_SecTime = time(NULL)-1;

    //loop until global variable g_runStat claims stop
    g_reset = 0;
    while (g_reset == 0)
    {
#ifdef USE_POLL
        int    pp[40];
        int    nn = 0;
        pollfd fds[40];
        for (int i=0; i<40; i++)
        {
            if (rd[i].socket>=0 && rd[i].connected && rd[i].bufLen>0)
            {
                fds[nn].fd = rd[i].socket;
                fds[nn].events = POLLIN;
                pp[nn] = i;
                nn++;
            }
        }

        const int rc_epoll = poll(fds, nn, 100);
        if (rc_epoll<0)
            break;
#endif

#ifdef USE_SELECT
        fd_set readfs;
        FD_ZERO(&readfs);
        int nfsd = 0;
        for (int i=0; i<NBOARDS; i++)
            if (rd[i].socket>=0 && rd[i].connected && rd[i].bufLen>0)
            {
                FD_SET(rd[i].socket, &readfs);
                if (rd[i].socket>nfsd)
                    nfsd = rd[i].socket;
            }

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        const int rc_select = select(nfsd+1, &readfs, NULL, NULL, &tv);
        // 0: timeout
        // -1: error
        if (rc_select<0)
        {
            factPrintf(MessageImp::kError, "Waiting for data failed: %d (select,rc=%d)", errno);
            continue;
        }
#endif

#ifdef USE_EPOLL
        const int rc_epoll = READ_STRUCT::wait();
        if (rc_epoll<0)
            break;
#endif

#ifdef PRIORITY_QUEUE
        priority_queue<READ_STRUCT*, vector<READ_STRUCT*>, READ_STRUCTcomp> prio;

        for (int i=0; i<NBOARDS; i++)
            if (rd[i].connected)
                prio.push(rd+i);

        if (!prio.empty()) do
#endif


#ifdef USE_POLL
        for (int jj=0; jj<nn; jj++)
#endif
#ifdef USE_EPOLL
        for (int jj=0; jj<rc_epoll; jj++)
#endif
#if !defined(USE_EPOLL) && !defined(USE_POLL) && !defined(PRIORITY_QUEUE)
        for (int jj=0; jj<NBOARDS; jj++)
#endif
        {
#ifdef PRIORITY_QUEUE
            READ_STRUCT *rs = prio.top();
#endif
#ifdef USE_SELECT
            if (!FD_ISSET(rs->socket, &readfs))
                continue;
#endif

#ifdef USE_POLL
            if ((fds[jj].revents&POLLIN)==0)
                continue;
#endif

#ifdef USE_EPOLL
            // FIXME: How to get i?
            READ_STRUCT *rs = READ_STRUCT::get(jj);
#endif

#ifdef USE_POLL
            // FIXME: How to get i?
            READ_STRUCT *rs = &rd[pp[jj]];
#endif

#if !defined(USE_POLL) && !defined(USE_EPOLL) && !defined(PRIORITY_QUEUE)
            const int i = (jj%4)*10 + (jj/4);
            READ_STRUCT *rs = &rd[i];
#endif

#ifdef COMPLETE_EVENTS
            if (rs->bufTyp==READ_STRUCT::kWait)
                continue;
#endif

            // ==================================================================

            const bool rc_read = rs->read();

            // Connect might have gotten closed during read
            gi_NumConnect[rs->sockId] = rs->connected;
            gj.numConn[rs->sockId]    = rs->connected;

            // Read either failed or disconnected, or the buffer is not yet full
            if (!rc_read)
                continue;

            // ==================================================================

            if (rs->bufTyp==READ_STRUCT::kHeader)
            {
                //check if startflag correct; else shift block ....
                // FIXME: This is not enough... this combination of
                //        bytes can be anywhere... at least the end bytes
                //        must be checked somewhere, too.
                uint k;
                for (k=0; k<sizeof(PEVNT_HEADER)-1; k++)
                {
                    if (rs->B[k]==0xfb && rs->B[k+1] == 0x01)
                        break;
                }
                rs->skip += k;

                //no start of header found
                if (k==sizeof(PEVNT_HEADER)-1)
                {
                    rs->B[0]   = rs->B[sizeof(PEVNT_HEADER)-1];
                    rs->bufPos = rs->B+1;
                    rs->bufLen = sizeof(PEVNT_HEADER)-1;
                    continue;
                }

                if (k > 0)
                {
                    memmove(rs->B, rs->B+k, sizeof(PEVNT_HEADER)-k);

                    rs->bufPos -= k;
                    rs->bufLen += k;

                    continue; // We need to read more (bufLen>0)
                }

                if (rs->skip>0)
                {
                    factPrintf(MessageImp::kInfo, "Skipped %d bytes on port %d", rs->skip, rs->sockId);
                    rs->skip = 0;
                }

                // Swap the header entries from network to host order
                rs->swapHeader();

                rs->bufTyp = READ_STRUCT::kData;
                rs->bufLen = rs->len() - sizeof(PEVNT_HEADER);

                debugHead(rs->B);  // i and fadBoard not used

                continue;
            }

            const uint16_t &end = *reinterpret_cast<uint16_t*>(rs->bufPos-2);
            if (end != 0xfe04)
            {
                factPrintf(MessageImp::kError, "End-of-event flag wrong on socket %2d for event %d (len=%d), got %04x",
                           rs->sockId, rs->H.fad_evt_counter, rs->len(), end);

                // ready to read next header
                rs->bufTyp = READ_STRUCT::kHeader;
                rs->bufLen = sizeof(PEVNT_HEADER);
                rs->bufPos = rs->B;
                // FIXME: What to do with the validity flag?
                continue;
            }

            // get index into mBuffer for this event (create if needed)
            const shared_ptr<EVT_CTRL2> evt = mBufEvt(*rs, actrun);

            // We have a valid entry, but no memory has yet been allocated
            if (evt && !evt->initMemory())
            {
                const time_t tm = time(NULL);
                if (evt->runCtrl->reportMem==tm)
                    continue;

                factPrintf(MessageImp::kError, "No free memory left for %d (run=%d)", evt->evNum, evt->runNum);
                evt->runCtrl->reportMem = tm;
                continue;
            }

            // ready to read next header
            rs->bufTyp = READ_STRUCT::kHeader;
            rs->bufLen = sizeof(PEVNT_HEADER);
            rs->bufPos = rs->B;

            // Fatal error occured. Event cannot be processed. Skip it. Start reading next header.
            if (!evt)
                continue;

            // This should never happen
            if (evt->board[rs->sockId] != -1)
            {
                factPrintf(MessageImp::kError, "Got event %5d from board %3d (i=%3d, len=%5d) twice.",
                           evt->evNum, rs->sockId, jj, rs->len());
                // FIXME: What to do with the validity flag?
                continue; // Continue reading next header
            }

            // Swap the data entries (board headers) from network to host order
            rs->swapData();

            // Copy data from rd[i] to mBuffer[evID]
            copyData(*rs, evt.get());

#ifdef COMPLETE_EVENTS
            // Do not read anmymore from this board until the whole event has been received
            rs->bufTyp = READ_STRUCT::kWait;
#endif
            // now we have stored a new board contents into Event structure
            evt->board[rs->sockId] = rs->sockId;
            evt->header = evt->FADhead+rs->sockId;
            evt->nBoard++;

#ifdef COMPLETE_EPOLL
            if (epoll_ctl(READ_STRUCT::fd_epoll, EPOLL_CTL_DEL, rs->socket, NULL)<0)
            {
                factPrintf(MessageImp::kError, "epoll_ctrl failed: %m (EPOLL_CTL_DEL,rc=%d)", errno);
                break;
            }
#endif
            // event not yet complete
            if (evt->nBoard < READ_STRUCT::activeSockets)
                continue;

            // All previous events are now flagged as incomplete ("expired")
            // and will be removed. (This is a bit tricky, because pop_front()
            // would invalidate the current iterator if not done _after_ the increment)
            for (auto it=evtCtrl.begin(); it!=evtCtrl.end(); )
            {
                const bool found = it->get()==evt.get();
                if (!found)
                    reportIncomplete(*it, "expired");
                else
                    primaryQueue.post(evt);

                // package_len is 0 if nothing was received.
                for (int ib=0; ib<40; ib++)
                    rd[ib].relBytes += uint32_t((*it)->FADhead[ib].package_length)*2;

                // The counter must be increased _before_ the pop_front,
                // otherwise the counter is invalidated by the pop_front!
                it++;
                evtCtrl.pop_front();

                // We reached the current event, so we are done
                if (found)
                    break;
            }

#ifdef COMPLETE_EPOLL
            for (int j=0; j<40; j++)
            {
                epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.ptr = &rd[j];  // user data (union: ev.ptr)
                if (epoll_ctl(READ_STRUCT::fd_epoll, EPOLL_CTL_ADD, rd[j].socket, &ev)<0)
                {
                    factPrintf(MessageImp::kError, "epoll_ctl failed: %m (EPOLL_CTL_ADD,rc=%d)", errno);
                    return;
                }
            }
#endif

#ifdef COMPLETE_EVENTS
            for (int j=0; j<40; j++)
            {
                //if (rs->bufTyp==READ_STRUCT::kWait)
                {
                    rs->bufTyp = READ_STRUCT::kHeader;
                    rs->bufLen = sizeof(PEVNT_HEADER);
                    rs->bufPos = rs->B;
                }
            }
#endif
        } // end for loop over all sockets
#ifdef PRIORITY_QUEUE
        while (0); // convert continue into break ;)
#endif

        // ==================================================================

        const time_t actTime = time(NULL);
        if (actTime == gi_SecTime)
        {
#if !defined(USE_SELECT) && !defined(USE_EPOLL) && !defined(USE_POLL)
            if (evtCtrl.empty())
                usleep(actTime-actrun->lastTime>300 ? 10000 : 1);
#endif
            continue;
        }
        gi_SecTime = actTime;

        // ==================================================================
        //loop over all active events and flag those older than read-timeout
        //delete those that are written to disk ....

        // This could be improved having the pointer which separates the queue with
        // the incomplete events from the queue with the complete events
        for (auto it=evtCtrl.begin(); it!=evtCtrl.end(); )
        {
            // A reference is enough because the shared_ptr is hold by the evtCtrl
            const shared_ptr<EVT_CTRL2> &evt = *it;

            // The first event is the oldest. If the first event within the
            // timeout window was received, we can stop searching further.
            if (evt->time.tv_sec+g_evtTimeout>=actTime)
                break;

            // The counter must be increased _before_ the pop_front,
            // otherwise the counter is invalidated by the pop_front!
            it++;

            // This timeout is caused because complete data from one or more
            // boards has been received, but the memory could not be allocated.
            // There is no reason why we should not go on waiting for
            // memory to become free. However, the FADs will disconnect
            // after 60s due to their keep-alive timeout, but the event builder
            // will still wait for memory to become available.
            // Currently, the only possibility to free the memory from the
            // evtCtrl to restart the event builder (STOP/START).
            if (!evt->valid())
                continue;

            // This will result in the emission of a dim service.
            // It doesn't matter if that takes comparably long,
            // because we have to stop the run anyway.
            const uint64_t rep = reportIncomplete(evt, "timeout");
            factReportIncomplete(rep);

            // At least the data from one boards is complete...
            // package_len is 0 when nothing was received from this board
            for (int ib=0; ib<40; ib++)
                rd[ib].relBytes += uint32_t(evt->FADhead[ib].package_length)*2;

            evtCtrl.pop_front();
        }

        // =================================================================

        gj.bufNew   = evtCtrl.size();            //# incomplete events in buffer
        gj.bufEvt   = primaryQueue.size();       //# complete events in buffer
        gj.bufWrite = secondaryQueue.size();     //# complete events in buffer
        gj.bufProc  = processingQueue1.size();   //# complete events in buffer
        gj.bufTot   = Memory::max_inuse/MAX_TOT_MEM;
        gj.usdMem   = Memory::max_inuse;
        gj.totMem   = Memory::allocated;
        gj.maxMem   = g_maxMem;

        gj.deltaT = 1000; // temporary, must be improved

        bool changed = false;

        static vector<uint64_t> store(NBOARDS);

        for (int ib=0; ib<NBOARDS; ib++)
        {
            gj.rateBytes[ib] = store[ib]>rd[ib].totBytes ? rd[ib].totBytes : rd[ib].totBytes-store[ib];
            gj.relBytes[ib]  = rd[ib].totBytes-rd[ib].relBytes;

            store[ib] = rd[ib].totBytes;

            if (rd[ib].check(g_port[ib].sockDef, g_port[ib].sockAddr))
                changed = true;

            gi_NumConnect[ib] = rd[ib].connected;
            gj.numConn[ib]    = rd[ib].connected;
        }

        factStat(gj);

        Memory::max_inuse = 0;

        // =================================================================

        // This is a fake event to trigger possible run-closing conditions once a second
        // FIXME: This is not yet ideal because a file would never be closed
        //        if a new file has been started and no events of the new file
        //        have been received yet
        int request = kRequestNone;

        // If nothing was received for more than 5min, close file
        if (actTime-actrun->lastTime>300)
            request |= kRequestTimeout;

        // If connection status has changed
        if (changed)
            request |= kRequestConnectionChange;

        if (request!=kRequestNone)
            runFinished();

        if (actrun->fileStat==kFileOpen)
            primaryQueue.emplace(new EVT_CTRL2(request, actrun));
    }

    //   1: Stop, wait for event to get processed
    //   2: Stop, finish immediately
    // 101: Restart, wait for events to get processed
    // 101: Restart, finish immediately
    //
    const int gi_reset = g_reset;

    const bool abort = gi_reset%100==2;

    factPrintf(MessageImp::kInfo, "Stop reading ... RESET=%d (%s threads)", gi_reset, abort?"abort":"join");

    primaryQueue.wait(abort);
    secondaryQueue.wait(abort);
    processingQueue1.wait(abort);

    // Here we also destroy all runCtrl structures and hence close all open files
    evtCtrl.clear();
    actrun.reset();

    factPrintf(MessageImp::kInfo, "Exit read Process...");
    factPrintf(MessageImp::kInfo, "%llu Bytes flagged as in-use.", Memory::inuse);

    factStat(gj);

    return gi_reset>=100;
}

// ==========================================================================
// ==========================================================================

void StartEvtBuild()
{
    factPrintf(MessageImp::kInfo, "Starting EventBuilder++");

    memset(gi_NumConnect, 0, NBOARDS*sizeof(*gi_NumConnect));

    memset(&gj, 0, sizeof(GUI_STAT));

    gj.usdMem   = Memory::inuse;
    gj.totMem   = Memory::allocated;
    gj.maxMem   = g_maxMem;


    READ_STRUCT rd[NBOARDS];

    // This is only that every socket knows its id (maybe we replace that by arrays instead of an array of sockets)
    for (int i=0; i<NBOARDS; i++)
        rd[i].sockId = i;

    while (mainloop(rd));

    //must close all open sockets ...
    factPrintf(MessageImp::kInfo, "Close all sockets...");

    READ_STRUCT::close();

    // Now all sockets get closed. This is not reflected in gi_NumConnect
    // The current workaround is to count all sockets as closed when the thread is not running
    factPrintf(MessageImp::kInfo, "EventBuilder++ closed");
}
