#ifndef FACT_DataWriteFits
#define FACT_DataWriteFits

#include "DataProcessorImp.h"
#include "FitsFile.h"

#include <array>

class Converter;

class DataWriteFits : public DataProcessorImp
{
    Converter *fConv;

    FitsFile fFile;

    std::array<uint32_t, 8> fTriggerCounter;

    uint32_t fTstart[2];
    uint32_t fTstop[2];

    template <typename T>
        void WriteKey(const string &name, const int idx, const T &value, const string &comment);

    bool WriteFooter();

    virtual int GetDrsStep() const { return -1; }

public:
    DataWriteFits(const std::string &path, uint64_t night,  uint32_t runid, MessageImp &imp) :
        DataProcessorImp(path, night, runid, imp), fConv(0), fFile(imp)
    {
    }

    ~DataWriteFits();

    bool Open(const RUN_HEAD &h, const FAD::RunDescription &d);
    bool WriteEvt(const EVT_CTRL2 &);
    bool Close(const EVT_CTRL2 &);
};

#endif
