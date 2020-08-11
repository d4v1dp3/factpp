#ifndef FACT_DataWriteFits2
#define FACT_DataWriteFits2

#include "DataProcessorImp.h"

#include <array>

class ofits;

struct DrsCalibration;

class DataWriteFits2 : public DataProcessorImp
{
    std::shared_ptr<ofits> fFile;

    std::array<uint32_t, 8> fTriggerCounter;

    uint32_t fTstart[2];
    uint32_t fTstop[2];

    void WriteHeader(const RUN_HEAD &h, const FAD::RunDescription &d);
    void WriteFooter();

    virtual int GetDrsStep() const { return -1; }

public:
    DataWriteFits2(const std::string &path, uint64_t night, uint32_t runid, MessageImp &imp);
    DataWriteFits2(const std::string &path, uint64_t night, uint32_t runid, const DrsCalibration &cal, MessageImp &imp);

    bool Open(const RUN_HEAD &h, const FAD::RunDescription &d);
    bool WriteEvt(const EVT_CTRL2 &e);
    bool Close(const EVT_CTRL2 &);

    Time GetTstart() const { return Time(fTstart[0], fTstart[1]); }
    Time GetTstop() const  { return Time(fTstop[0],  fTstop[1]);  }
};

#endif
