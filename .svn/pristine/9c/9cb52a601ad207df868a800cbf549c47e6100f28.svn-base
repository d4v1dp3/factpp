#ifndef FACT_Queue
#define FACT_Queue

#include <list>
#include <thread>
#include <condition_variable>

template<class T>
class Queue
{
    size_t fSize;                 // Only necessary for before C++11

    std::list<T> fList;

    std::mutex fMutex;        // Mutex needed for the conditional
    std::condition_variable fCond; // Conditional

    enum state_t
    {
        kIdle,
        kRun,
        kStop,
        kAbort,
    };

    state_t fState;               // Stop signal for the thread

    typedef std::function<void(const T &)> callback;
    callback fCallback;       // Callback function called by the thread

    std::thread fThread;      // Handle to the thread

    void Thread()
    {
        std::unique_lock<std::mutex> lock(fMutex);

        while (1)
        {
            while (fList.empty() && fState==kRun)
                fCond.wait(lock);

            if (fState==kAbort)
                break;

            if (fState==kStop && fList.empty())
                break;

            const T &val = fList.front();

            // Theoretically, we can loose a signal here, but this is
            // not a problem, because then we detect a non-empty queue
            lock.unlock();

            if (fCallback)
                fCallback(val);

            lock.lock();

            fList.pop_front();
            fSize--;
        }

        fList.clear();
        fSize = 0;

        fState = kIdle;
    }

public:
    Queue(const callback &f) : fSize(0), fState(kIdle), fCallback(f)
    {
        start();
    }
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
            if (fState==kIdle)
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

    bool post(const T &val)
    {
        const std::lock_guard<std::mutex> lock(fMutex);
        if (fState==kIdle)
            return false;

        fList.push_back(val);
        fSize++;

        fCond.notify_one();

        return true;
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    template<typename... _Args>
        bool emplace(_Args&&... __args)
    {
        const std::lock_guard<std::mutex> lock(fMutex);
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
    bool move(std::list<T>&& x, typename std::list<T>::iterator i)
#else
    bool move(std::list<T>& x, typename std::list<T>::iterator i)
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
    bool move(std::list<T>& x, typename std::list<T>::iterator i) { return move(std::move(x), i); }
#endif

    size_t size() const
    {
        return fSize;
    }

    bool empty() const
    {
        return fSize==0;
    }
};

#endif
