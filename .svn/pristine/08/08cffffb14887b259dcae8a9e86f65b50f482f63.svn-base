#ifndef FACT_DataProcessorImp
#define FACT_DataProcessorImp

#include "MessageImp.h"

struct RUN_HEAD;
struct EVT_CTRL2;
struct RUN_CTRL2;

namespace FAD
{
    struct RunDescription;
};

class DataProcessorImp : public MessageImp
{
    std::string fPath;
    uint32_t    fNight;
    uint32_t    fRunId;

    int Write(const Time &time, const std::string &txt, int qos=kMessage)
    {
        return fMsg.Write(time, txt, qos);
    }

protected:
    MessageImp &fMsg;
    std::string fFileName;

public:
    DataProcessorImp(const std::string &path, uint64_t night, uint32_t id, MessageImp &imp) : fPath(path), fNight(night), fRunId(id), fMsg(imp) { }
    virtual ~DataProcessorImp() { }

    virtual bool Open(const RUN_HEAD &h, const FAD::RunDescription &desc) = 0;
    virtual bool WriteEvt(const EVT_CTRL2 &) = 0;
    virtual bool Close(const EVT_CTRL2 &) = 0;

    const std::string &GetFileName() const { return fFileName; }

    std::string GetPath() const { return fPath; }
    uint32_t    GetNight() const { return fNight; }
    uint32_t    GetRunId() const { return fRunId; }

    static std::string FormFileName(const std::string &path, uint64_t night, uint32_t runid, const std::string &extension);
    std::string FormFileName(const std::string &extension)
    {
        return FormFileName(fPath, fNight, fRunId, extension);
    }
};

#include "Time.h"

class DataDump : public DataProcessorImp
{
    Time fTime;

public:
    DataDump(const std::string &path, uint64_t night, uint32_t id, MessageImp &imp) : DataProcessorImp(path, night, id, imp) { }

    bool Open(const RUN_HEAD &h, const FAD::RunDescription &d);
    bool WriteEvt(const EVT_CTRL2 &);
    bool Close(const EVT_CTRL2 &);
};

class DataDebug : public DataDump
{
public:
    DataDebug(const std::string &path, uint64_t night, uint32_t id, MessageImp &imp) : DataDump(path, night, id, imp) { }

    bool WriteEvt(const EVT_CTRL2 &);
};

#endif
