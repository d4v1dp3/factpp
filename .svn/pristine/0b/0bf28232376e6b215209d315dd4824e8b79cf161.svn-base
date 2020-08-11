#ifndef FACT_DimData

struct DimData
{
    const int               qos;
    const std::string       name;
    const std::string       format;
    const std::vector<char> data;
    const Time              time;

    Time extract(DimInfo *inf) const
    {
        // Must be called in exactly this order!
        const int tsec = inf->getTimestamp();
        const int tms  = inf->getTimestampMillisecs();

        return Time(tsec, tms*1000);
    }

    DimData(DimInfo *inf) : qos(inf->getQuality()),
        name(inf->getName()),
        format(inf->getFormat()),
        data(inf->getString(), inf->getString()+inf->getSize()),
        time(extract(inf))
    {
    }

    template<typename T>
        T get(uint32_t offset=0) const { return *reinterpret_cast<const T*>(data.data()+offset); }

    template<typename T>
        const T *ptr(uint32_t offset=0) const { return reinterpret_cast<const T*>(data.data()+offset); }

    template<typename T>
        const T &ref(uint32_t offset=0) const { return *reinterpret_cast<const T*>(data.data()+offset); }

    const char *c_str() const { return (char*)data.data(); }

    size_t size() const { return data.size(); }
};

#endif
