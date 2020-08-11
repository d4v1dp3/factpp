#ifndef FACT_Event
#define FACT_Event

#include "EventImp.h"

class Event : public EventImp
{
private:
    std::string fName;        /// A name associated with the event
    std::string fFormat;      /// A string describing the format of the data
    std::string fDescription; /// A human readable description of the event

    std::vector<char> fData;  /// Data associated with this event

    Time  fTime;              /// Time stamp
    int   fQoS;               /// Quality of service
    bool  fEmpty;             /// Empty is true if received event was a NULL pointer

public:
    Event() : fQoS(0), fEmpty(true) { }
    /// Constructs an event as a combination of an EventImp and a DimCommand
    Event(const std::string &name, const std::string &fmt="");
    /// Copy constructor
    Event(const EventImp &imp);
    Event(const EventImp &imp, const char *ptr, size_t siz);

    void SetDescription(const std::string &str) { fDescription=str; }
    std::string GetDescription() const { return fDescription; }

    /// Return the stored name of the event
    std::string GetName() const { return fName; }
    /// Return the stored format of the data
    std::string GetFormat() const { return fFormat; }

    /// Return a pointer to the data region
    const void *GetData() const { return &*fData.begin(); }
    /// Return the size of the data
    size_t      GetSize() const { return fData.size(); }

    /// Return reference to a time stamp
    Time GetTime() const { return fTime; }
    /// Return Quality of Service
    int  GetQoS() const  { return fQoS; }
    /// Return if event is not just zero size but empty
    bool IsEmpty() const { return fEmpty; }

    void SetTime() { fTime = Time(); }
    void SetData(const std::vector<char> &data) { fData = data; }
    void SetData(const void *ptr, size_t siz) {
        const char *c = reinterpret_cast<const char*>(ptr);
        fData = std::vector<char>(c, c+siz); }

    void SetInt(int i) { SetData(&i, sizeof(i)); }
    void SetFloat(float f) { SetData(&f, sizeof(f)); }
    void SetDouble(float d) { SetData(&d, sizeof(d)); }
    void SetShort(short s) { SetData(&s, sizeof(s)); }
    void SetText(const char *txt) { SetData(txt, strlen(txt)+1); }
    void SetString(const std::string &str) { SetData(str.c_str(), str.length()+1); }
};

#endif
