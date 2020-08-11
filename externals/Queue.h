#ifndef FACT_Queue
#define FACT_Queue

#include <list>
#include <functional>

#ifndef __CINT__
#include <thread>
#include <condition_variable>
#else
namespace std
{
    class mutex;
    class thread;
    class condition_variable;
    template<class T> class function<T>;
}
#endif


// The second template argument must support:
//    iterator it = begin();   // get the next element to be processed
//    erase(it);               // erase the processed element from the queue
//    push_back();             // add a new element to the queue
//    emplace_back();          // emplace a new element to the queue
//    splice();                // used to efficiently implement post with mutex

template<class T, class List=std::list<T>>
class Queue
{
    size_t fSize;                 // Only necessary for before C++11

    List fList;

    std::mutex fMutex;             // Mutex needed for the conditional
    std::condition_variable fCond; // Conditional

    enum state_t
    {
        kIdle,
        kRun,
        kStop,
        kAbort,
        kTrigger,
        kPrompt

    };

    state_t fState;               // Stop signal for the thread

    typedef std::function<bool(const T &)> callback;
    callback fCallback;       // Callback function called by the thread

    std::thread fThread;      // Handle to the thread

    void Thread()
    {
        std::unique_lock<std::mutex> lock(fMutex);

        // No filling allowed by default (the queue is
        // always processed until it is empty)
        size_t allowed = 0;

        while (1)
        {
            while (fSize==allowed && fState==kRun)
                fCond.wait(lock);

            // Check if the State flag has been changed
            if (fState==kAbort)
                break;

            if (fState==kStop && fList.empty())
                break;

            // If thread got just woken up, move back the state to kRun
            if (fState==kTrigger)
                fState = kRun;

            // Could have been a fState==kTrigger case
            if (fList.empty())
                continue;

            // During the unlocked state, fSize might change.
            // The current size of the queue needs to be saved.
            allowed = fSize;

            // get the first entry from the (sorted) list
            const auto it = fList.begin();

            // Theoretically, we can loose a signal here, but this is
            // not a problem, because then we detect a non-empty queue
            lock.unlock();

            // If the first event in the queue could not be processed,
            // no further processing makes sense until a new event has
            // been posted (or the number of events in the queue has
            // changed)  [allowed>0], in the case processing was
            // successfull [allowed==0], the next event will be processed
            // immediately.

            if (fCallback && fCallback(*it))
                allowed = 0;

            lock.lock();

            // Whenever an event was successfully processed, allowed
            // is equal to zero and thus the event will be popped
            if (allowed>0)
                continue;

            fList.erase(it);
            fSize--;
        }

        fList.clear();
        fSize = 0;

        fState = kIdle;
    }

public:
    Queue(const callback &f, bool startup=true) : fSize(0), fState(kIdle), fCallback(f)
    {
        if (startup)
            start();
    }

    Queue(const Queue<T,List>& q) : fSize(0), fState(kIdle), fCallback(q.fCallback)
    {
    }

    Queue<T,List>& operator = (const Queue<T,List>& q)
    {
        fSize     = 0;
        fState    = kIdle;
        fCallback = q.fCallback;
        return *this;
    }

#ifdef __MARS__ // Needed for the compilatio of the dictionary
    Queue() : fSize(0), fState(kIdle)
    {
    }
#endif

    ~Queue()
    {
        wait(true);
    }

    bool start()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState!=kIdle)
            return false;

        fState = kRun;
        fThread = std::thread(std::bind(&Queue::Thread, this));
        return true;
    }

    bool stop()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kIdle)
            return false;

        fState = kStop;
        fCond.notify_one();

        return true;
    }

    bool abort()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kIdle)
            return false;

        fState = kAbort;
        fCond.notify_one();

        return true;
    }

    bool wait(bool abrt=false)
    {
        {
            const std::lock_guard<std::mutex> lock(fMutex);
            if (fState==kIdle || fState==kPrompt)
                return false;

            if (fState==kRun)
            {
                fState = abrt ? kAbort : kStop;
                fCond.notify_one();
            }
        }

        fThread.join();
        return true;
    }

    bool enablePromptExecution()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState!=kIdle || fSize>0)
            return false;

        fState = kPrompt;
        return true;
    }

    bool disablePromptExecution()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState!=kPrompt)
            return false;

        fState = kIdle;
        return true;
    }

    bool setPromptExecution(bool state)
    {
        return state ? enablePromptExecution() : disablePromptExecution();
    }


    bool post(const T &val)
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kPrompt)
            return fCallback(val);

        if (fState==kIdle)
            return false;

        fList.push_back(val);
        fSize++;

        fCond.notify_one();

        return true;
    }

    bool notify()
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState!=kRun)
            return false;

        fState = kTrigger;
        fCond.notify_one();

        return true;
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    template<typename... _Args>
        bool emplace(_Args&&... __args)
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kPrompt)
            return fCallback(T(__args...));

        if (fState==kIdle)
            return false;

        fList.emplace_back(__args...);
        fSize++;

        fCond.notify_one();

        return true;
    }

    bool post(T &&val) { return emplace(std::move(val)); }
#endif

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    bool move(List&& x, typename List::iterator i)
#else
    bool move(List& x, typename List::iterator i)
#endif
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kIdle)
            return false;

        fList.splice(fList.end(), x, i);
        fSize++;

        fCond.notify_one();

        return true;
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    bool move(List& x, typename List::iterator i) { return move(std::move(x), i); }
#endif

    size_t size() const
    {
        return fSize;
    }

    bool empty() const
    {
        return fSize==0;
    }

    bool operator<(const Queue& other) const
    {
        return fSize < other.fSize;
    }

};

#endif
