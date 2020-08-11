// **************************************************************************
/** @class EventDim

@brief Implementation of an EventImp as a DimStampedInfo

This is the implementation of an event which can be posted to a state
machine via the DIM network.

@todo
- Add reference to DIM docu
- improve docu

*/
// **************************************************************************
#ifndef FACT_ServiceDim
#define FACT_ServiceDim

#include "EventImp.h"
#include "dic.hxx"

class ServiceDim : public EventImp, public DimStampedInfo
{
public:
    ServiceDim(const std::string &name, DimInfoHandler *handler)
        : EventImp(), DimStampedInfo(name.c_str(), (void*)NULL, 0, handler)
    {
    }
    std::string GetName() const   { return const_cast<ServiceDim*>(this)->getName(); }
    std::string GetFormat() const { return const_cast<ServiceDim*>(this)->getFormat(); }

    const void *GetData() const   { return const_cast<ServiceDim*>(this)->getData(); }
    size_t      GetSize() const   { return const_cast<ServiceDim*>(this)->getSize(); }

    Time GetTime() const
    {
        // Must be in exactly this order!
        const int tsec = const_cast<ServiceDim*>(this)->getTimestamp();
        const int tms  = const_cast<ServiceDim*>(this)->getTimestampMillisecs();

        return Time(tsec, tms*1000);
    }

    int GetQoS() const { return const_cast<ServiceDim*>(this)->getQuality(); }
};

#endif
