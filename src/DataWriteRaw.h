#ifndef FACT_DataWriteRaw
#define FACT_DataWriteRaw

#include "DataProcessorImp.h"

class DataWriteRaw : public DataProcessorImp
{
    std::ofstream fOut;

    off_t fPosTail;

    uint32_t fCounter;


    // WRITE uint32_t 0xFAC77e1e  (FACT Tele)
    // ===
    // WRITE uint32_t TYPE(>0)          == 1
    // WRITE uint32_t ID(>0)            == 0
    // WRITE uint32_t VERSION(>0)       == 1
    // WRITE uint32_t LENGTH
    // -
    // WRITE uint32_t TELESCOPE ID
    // WRITE uint32_t RUNID
    // ===
    // WRITE uint32_t TYPE(>0)          == 2
    // WRITE uint32_t ID(>0)            == 0
    // WRITE uint32_t VERSION(>0)       == 1
    // WRITE uint32_t LENGTH
    // -
    // WRITE          HEADER
    // ===
    // [ 40 TIMES
    //    WRITE uint32_t TYPE(>0)       == 3
    //    WRITE uint32_t ID(>0)         == 0..39
    //    WRITE uint32_t VERSION(>0)    == 1
    //    WRITE uint32_t LENGTH
    //    -
    //    WRITE          BOARD-HEADER
    // ]
    // ===
    // WRITE uint32_t TYPE(>0)          == 4
    // WRITE uint32_t ID(>0)            == 0
    // WRITE uint32_t VERSION(>0)       == 1
    // WRITE uint32_t LENGTH
    // -
    // WRITE          FOOTER (empty)
    // ===
    // [ N times
    //    WRITE uint32_t TYPE(>0)       == 10
    //    WRITE uint32_t ID(>0)         == counter
    //    WRITE uint32_t VERSION(>0)    == 1
    //    WRITE uint32_t LENGTH HEADER
    //    -
    //    WRITE          HEADER+DATA
    // ]
    // ===
    // WRITE uint32_t TYPE   ==0
    // WRITE uint32_t VERSION==0
    // WRITE uint32_t LENGTH ==0
    // ===
    // Go back and write footer

    void WriteBlockHeader(uint32_t type, uint32_t ver, uint32_t cnt, uint32_t len);

    template<typename T>
        void WriteValue(const T &t);


public:
    DataWriteRaw(const std::string &path, uint64_t night, uint32_t id, MessageImp &imp) : DataProcessorImp(path, night, id, imp), fPosTail(0) { }
    ~DataWriteRaw() { if (fOut.is_open()) Close(); }

    enum
    {
        kEndOfFile = 0,
        kIdentifier = 1,
        kRunHeader,
        kBoardHeader,
        kRunSummary,
        kEvent,
    };

    bool Open(const RUN_HEAD &h, const FAD::RunDescription &d);
    bool WriteEvt(const EVT_CTRL2 &);
    bool Close(const EVT_CTRL2 &) { return Close(); }
    bool Close();
};

#endif
