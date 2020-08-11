#ifndef FACT_MessageDim
#define FACT_MessageDim

#include <functional> // std::function

#include "MessageImp.h"
#include "DimDescriptionService.h"
//#include <dis.hxx> // DimService

#include "../externals/Queue.h"

class MessageDimTX : public DimDescribedService, public MessageImp
{
private:
    bool fDebug;

    Queue<std::tuple<Time,std::string,int>> fMsgQueue;

    bool UpdateService(const std::tuple<Time,std::string,int> &data);

public:
    MessageDimTX(const std::string &name, std::ostream &out=std::cout);
    ~MessageDimTX();

    int Write(const Time &t, const std::string &txt, int qos=kInfo);

    void SetDebug(bool b=true) { fDebug=b; }

    bool MessageQueueEmpty() const { return fMsgQueue.empty(); }
};



#include <dic.hxx> // DimStampedInfo

class MessageDimRX : public DimInfoHandler
{
private:
    int  fMinLogLevel;
    bool fConnected;

protected:
    MessageImp &fMsg;

private:
    DimStampedInfo fDimMessage;

protected:
    void infoHandler();

public:
    MessageDimRX(const std::string &name, MessageImp &imp);

    void SetMinLogLevel(int min=0) { fMinLogLevel=min; }
    bool IsConnected() const { return fConnected; }
};

#endif
