#ifndef FACT_EventImp
#define FACT_EventImp

#include <string>
#include <vector>

#include <functional>

#include "Time.h"

class EventImp
{
    std::vector<int> fAllowedStates; /// List of states in which this event is allowed

    /// http://www.boost.org/doc/libs/1_45_0/libs/bind/bind.html
    std::function<int(const EventImp &)> fFunction;

public:
    /// Constructor. Stores the target state given.
    EventImp() { }
    /// Copy constructor
    EventImp(const EventImp &cmd);
    virtual ~EventImp() {}

    // Description
    virtual void SetDescription(const std::string &) { }
    virtual std::string GetDescription() const { return ""; }

    // Function handling
    EventImp &AssignFunction(const std::function<int(const EventImp &)> &func=std::function<int(const EventImp &)>()) { fFunction = func; return *this; }
    bool HasFunc() const { return (bool)fFunction; }
    int ExecFunc() const { return fFunction ? fFunction(*this) : -1; }

    // Configuration helper
    EventImp &operator()(const std::function<int(const EventImp &)> &func) { return AssignFunction(func); }
    EventImp &operator()(const std::string &str) { SetDescription(str); return *this; }
    EventImp &operator()(const char *str) { SetDescription(str); return *this; }
    EventImp &operator()(int state) { fAllowedStates.push_back(state); return *this; }

    // Print contents
    virtual void Print(std::ostream &out, bool strip=false) const;
    virtual void Print(bool strip=false) const;

    // Handling of the states
    void AddAllowedState(int state);
    void AddAllowedStates(const std::string &states);

    bool IsStateAllowed(int state) const;

    // virtual function to return the data as stored in the derived classes
    virtual std::string GetName() const   { return ""; }
    virtual std::string GetFormat() const { return ""; }

    virtual const void *GetData() const { return 0; }
    virtual size_t      GetSize() const { return 0; }

    virtual Time GetTime() const { return Time::None; }
    virtual int  GetQoS() const  { return 0; }
    virtual bool IsEmpty() const { return GetData()==0; }

    std::string GetTimeAsStr(const char *fmt) const;
    uint64_t GetJavaDate() const;

    // Generalized access operators
    template<typename T>
        T Get(size_t offset=0) const
    {
        if (offset>=GetSize())
            throw std::logic_error("EventImp::Get - offset out of range.");
        return *reinterpret_cast<const T*>(GetText()+offset);
    }

    template<typename T>
        const T *Ptr(size_t offset=0) const
    {
        if (offset>=GetSize())
            throw std::logic_error("EventImp::Ptr - offset out of range.");
        return reinterpret_cast<const T*>(GetText()+offset);
    }

    template<typename T>
        const T &Ref(size_t offset=0) const
    {
        return *Ptr<T>(offset);
    }

    // Getter for all the data contained (name, format, data and time)
    const char *GetText() const { return reinterpret_cast<const char*>(GetData()); }

    bool     GetBool() const   { return Get<uint8_t>()!=0; }
    int16_t  GetShort() const  { return Get<int16_t>();    }
    uint16_t GetUShort() const { return Get<uint16_t>();   }
    int32_t  GetInt() const    { return Get<int32_t>();    }
    uint32_t GetUInt() const   { return Get<uint32_t>();   }
    int64_t  GetXtra() const   { return Get<int64_t>();    }
    uint64_t GetUXtra() const  { return Get<int64_t>();    }
    float    GetFloat() const  { return Get<float>();      }
    double   GetDouble() const { return Get<double>();     }

    std::vector<char> GetVector() const { return std::vector<char>(GetText(), GetText()+GetSize()); }
    std::string       GetString() const;
};

#endif
