#ifndef FACT_DimDescriptionService
#define FACT_DimDescriptionService

#include <set>
#include <array>
#include <string>
#include <vector>

class Time;
class DimService;

class DimDescriptionService
{
    static int         fCount;     /// Counter to count the number of instatiations
    static DimService *fService;   /// Pointer to the DimService distributing the desscriptions
    static std::string fData;      /// Data to be distributed with the service

    std::string fDescription;      /// Local storage for the applied description

public:
    DimDescriptionService(const std::string &name, const std::string &format);
    virtual ~DimDescriptionService();

    std::string GetDescription() const { return fDescription; }
};

#include "dis.hxx"

class DimDescribedService : public DimDescriptionService, public DimService
{
    static std::set<std::string> fServices;

public:
    template<typename T>
    DimDescribedService(const std::string &name, const T &val, const char *desc)
        : DimDescriptionService(name.c_str(), desc), DimService(name.c_str(), const_cast<T&>(val))
    {
        fServices.insert(getName());
        setQuality(0);
    }

    template<typename T>
    DimDescribedService(const std::string &name, const char *format, const T &val, const char *desc)
        : DimDescriptionService(name.c_str(), desc), DimService(name.c_str(), format, const_cast<T*>(&val), sizeof(T))
    {
        fServices.insert(getName());
        setQuality(0);
    }

    DimDescribedService(const std::string &name, const char *format, const char *desc)
       : DimDescriptionService(name.c_str(), desc), DimService(name.c_str(), format, (void*)NULL, 0)
    {
        fServices.insert(getName());
        setQuality(0);
        // FIXME: compare number of ; with number of |
    }

    ~DimDescribedService()
    {
        fServices.erase(getName());
    }

    static const std::set<std::string> &GetServices() { return fServices; }

    void setData(const void *ptr, size_t sz)
    {
        DimService::setData(const_cast<void*>(ptr), sz);
    }

    void setData(const char *str)
    {
        DimService::setData(const_cast<char*>(str));
    }

    void setData(const std::string &str)
    {
        setData(str.data());
    }

    template<class T>
    void setData(const T &data)
    {
        setData(&data, sizeof(T));
    }

    template<typename T>
    void setData(const std::vector<T> &data)
    {
        setData(data.data(), data.size()*sizeof(T));
    }

    template<class T, size_t N>
    void setData(const std::array<T, N> &data)
    {
        setData(data.data(), N*sizeof(T));
    }

    void setTime(const Time &t);
    void setTime();

    int Update();
    int Update(const Time &t);
    int Update(const std::string &data);
    int Update(const char *data);

    template<class T>
    int Update(const T &data)
    {
        setData(&data, sizeof(T));
        return Update();
    }

    template<typename T>
    int Update(const std::vector<T> &data)
    {
        setData(data);
        return Update();
    }

    template<class T, size_t N>
    int Update(const std::array<T, N> &data)
    {
        setData(data);
        return Update();
    }

    // FIXME: Implement callback with boost::function instead of Pointer to this
};

#endif
