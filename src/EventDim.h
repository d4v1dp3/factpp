// **************************************************************************
/** @class EventDim

@brief Concerete implementation of an EventImp as a DimCommand

This is the implementation of an event which can be posted to a state
machine via the DIM network.

@todo
- Add reference to DIM docu
- improve docu

*/
// **************************************************************************
#ifndef FACT_EventDim
#define FACT_EventDim

#include "EventImp.h"

#include "dis.hxx" // DimCommand

#include "DimDescriptionService.h"

class EventDim : public EventImp, public DimCommand
{
    DimDescriptionService *fDescription;

public:
    EventDim(const std::string &name, const std::string &format, DimCommandHandler *handler)
        : EventImp(), DimCommand(name.c_str(), format.c_str(), handler), fDescription(0)
    {
        // Initialize these values from DimCommand, because DimCommand
        // does not yet do it.
        itsData   = 0;
        itsSize   = 0;

        secs      = 0;
        millisecs = 0;
    }
    ~EventDim()
    {
        delete fDescription;
    }
    void SetDescription(const std::string &str)
    {
        if (fDescription)
            delete fDescription;
        fDescription = new DimDescriptionService(GetName(), str);
    }
    std::string GetDescription() const { return fDescription ? fDescription->GetDescription() : ""; }

    std::string GetName() const   { return const_cast<EventDim*>(this)->getName(); }
    std::string GetFormat() const { return const_cast<EventDim*>(this)->getFormat(); }

    const void *GetData() const   { return const_cast<EventDim*>(this)->getData(); }
    size_t      GetSize() const   { return const_cast<EventDim*>(this)->getSize(); }

    Time GetTime() const
    {
        // Must be in exactly this order!
        const int tsec = const_cast<EventDim*>(this)->getTimestamp();
        const int tms  = const_cast<EventDim*>(this)->getTimestampMillisecs();

        return Time(tsec, tms*1000);
    }
};

#endif
