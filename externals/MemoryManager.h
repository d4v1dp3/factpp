#ifndef MARS_MemoryManager
#define MARS_MemoryManager

#ifndef __CINT__
#include <forward_list>
#else
namespace std
{
    class mutex;
    class condition_variable;
    template<class T> class shared_ptr<T>;
    template<class T> class forward_list<T>;
}
#endif

class MemoryStock
{
    friend class MemoryChunk;
    friend class MemoryManager;

    size_t fChunkSize;
    size_t fMaxMemory;

    size_t fInUse;
    size_t fAllocated;

    size_t fMaxInUse;

    std::mutex fMutexMem;
    std::mutex fMutexCond;
    std::condition_variable fCond;

    std::forward_list<std::shared_ptr<char>> fMemoryStock;

public:
    MemoryStock(size_t chunk, size_t max) : fChunkSize(chunk), fMaxMemory(max<chunk?chunk:max),
        fInUse(0), fAllocated(0), fMaxInUse(0)
    {
    }

private:
    std::shared_ptr<char> pop(bool block)
    {
        if (block)
        {
            // No free slot available, next alloc would exceed max memory:
            // block until a slot is available
            std::unique_lock<std::mutex> lock(fMutexCond);
            while (fMemoryStock.empty() && fAllocated+fChunkSize>fMaxMemory)
                fCond.wait(lock);
        }
        else
        {
            // No free slot available, next alloc would exceed max memory
            // return an empty pointer
            if (fMemoryStock.empty() && fAllocated+fChunkSize>fMaxMemory)
                return std::shared_ptr<char>();
        }

        // We will return this amount of memory
        // This is not 100% thread safe, but it is not a super accurate measure anyway
        fInUse += fChunkSize;
        if (fInUse>fMaxInUse)
            fMaxInUse = fInUse;

        if (fMemoryStock.empty())
        {
            // No free slot available, allocate a new one
            fAllocated += fChunkSize;
            return std::shared_ptr<char>(new char[fChunkSize]);
        }

        // Get the next free slot from the stack and return it
        const std::lock_guard<std::mutex> lock(fMutexMem);

        const auto mem = fMemoryStock.front();
        fMemoryStock.pop_front();
        return mem;
    };

    void push(const std::shared_ptr<char> &mem)
    {
        if (!mem)
            return;

        // Decrease the amont of memory in use accordingly
        fInUse -= fChunkSize;

        // If the maximum memory has changed, we might be over the limit.
        // In this case: free a slot
        if (fAllocated>fMaxMemory)
        {
            fAllocated -= fChunkSize;
            return;
        }

        {
            const std::lock_guard<std::mutex> lock(fMutexMem);
            fMemoryStock.emplace_front(mem);
        }

        {
            const std::lock_guard<std::mutex> lock(fMutexCond);
            fCond.notify_one();
        }
    }
};

class MemoryChunk
{
    friend class MemoryManager;

    std::shared_ptr<MemoryStock> fMemoryStock;
    std::shared_ptr<char>        fPointer;

    MemoryChunk(const std::shared_ptr<MemoryStock> &mem, bool block)
        : fMemoryStock(mem)
    {
        fPointer = fMemoryStock->pop(block);
    }

public:
    ~MemoryChunk()
    {
        fMemoryStock->push(fPointer);
    }
};

class MemoryManager
{
    std::shared_ptr<MemoryStock> fMemoryStock;

public:
    MemoryManager(size_t chunk, size_t max) : fMemoryStock(std::make_shared<MemoryStock>(chunk, max))
    {
    }

    std::shared_ptr<char> malloc(bool block=true)
    {
        const std::shared_ptr<MemoryChunk> chunk(new MemoryChunk(fMemoryStock, block));
        return std::shared_ptr<char>(chunk, chunk->fPointer.get());
    }

    size_t getChunkSize() const { return fMemoryStock->fChunkSize; }
    bool   setChunkSize(const size_t size)
    {
#ifdef __EXCEPTIONS
        if (getInUse())
            throw std::runtime_error("Cannot change the chunk size while there is memory in use");
        if (getMaxMemory()<size)
            throw std::runtime_error("Chunk size ("+std::to_string((long long int)size)+") larger than allowed memory ("+std::to_string((long long int)getMaxMemory())+")");
#else
        if (getInUse() || getMaxMemory()<size)
            return false;
#endif

        fMemoryStock->fChunkSize = size;
        return true;
    }

    size_t getMaxMemory() const { return fMemoryStock->fMaxMemory; }
    size_t getInUse() const { return fMemoryStock->fInUse; }
    size_t getAllocated() const { return fMemoryStock->fAllocated; }
    size_t getMaxInUse() const { return fMemoryStock->fMaxInUse; }
};

#endif
