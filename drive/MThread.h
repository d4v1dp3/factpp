#ifndef MARS_MThread
#define MARS_MThread

#ifndef ROOT_TThread
#include <TThread.h>
#endif

class MThread // We don't want MThread to be derived from TObject
{
private:
    TThread fThread;

    Int_t fNumCleanups;

    virtual void CleanUp() { }
    static void MapCleanUp(void *arg)
    {
        MThread *th = (MThread*)arg;
        th->CleanUp();
    }

    virtual Int_t Thread() = 0;
    static void *MapThread(void *arg)
    {
        // GetPriority();     High: -1 - -20, Norm: 0, Low: 1-20
        // pthread_setschedprio(SelfId(), priority);
        // 0: ok,

        TThread::CleanUpPush((void*)&MapCleanUp, arg);

        MThread *th = (MThread*)arg;
        return reinterpret_cast<void*>(th->Thread());
    }

public:
    MThread(TThread::EPriority pri = TThread::kNormalPriority) :
        fThread(MapThread, this, pri), fNumCleanups(0) { }
    MThread(const char *thname, TThread::EPriority pri = TThread::kNormalPriority) :
        fThread(thname, MapThread, this, pri), fNumCleanups(0) { }
    virtual ~MThread() { }

    // Setter: Thread control
    Int_t RunThread(void *arg = 0) { return fThread.Run(arg); }

    // Send cancel request and wait for cancellation
    // 13 is returned if thread is not running,
    // the return code of Join otherwise
    Int_t CancelThread(void **ret = 0) {
        const Int_t rc = fThread.Kill();
        if (rc==13) // Thread not running
            return rc;
        return fThread.Join(ret);
    }

    // Int_t            Kill() { return fThread.Kill(); }
    // Long_t           Join(void **ret = 0) { return fThread.Join(ret); }

    // void             SetPriority(EPriority pri)
    // void             Delete(Option_t *option="") { TObject::Delete(option); }

    // Getter
    TThread::EState  GetThreadState() const { return fThread.GetState(); }
    TString          GetThreadStateStr() const;
    Long_t           GetThreadId() const { return fThread.GetId(); }
    // EPriority        GetPriority() const { return fPriority; }

    Bool_t IsThreadRunning()  const { return fThread.GetState()==TThread::kRunningState; }
    Bool_t IsThreadCanceled() const { return fThread.GetState()==TThread::kCancelingState; }

    // This is a version of usleep which is a cancel point
    static void Sleep(UInt_t us)
    {
        TThread::SetCancelOn();
        usleep(us);
        TThread::SetCancelOff();
    }
};

#endif
