#ifndef FACT_StateMachineDim
#define FACT_StateMachineDim

// ***************************************************************************
/**
 @class DimLog

 @brief Ensures that the MessageDimTX is initialized before errors could be redirected to it

**/
// ***************************************************************************
#include "MessageDim.h"       // MessageDimTX

class DimLog
{
    friend class StateMachineDim;

    MessageDimTX fLog;
    DimLog(std::ostream &out, const std::string &name) : fLog(name, out) { }
};

// ***************************************************************************
/**
 @class DimStart

 @brief Ensures calling DimServer::start() in its constructor and DimServer::stop() in its destructor

**/
// ***************************************************************************
#include "DimErrorRedirecter.h"

class DimStart : public DimErrorRedirecter
{
    const bool fIsValidServer;

protected:
    DimStart(const std::string &name, MessageImp &imp) : DimErrorRedirecter(imp), fIsValidServer(!name.empty())
    {
        DimClient::setNoDataCopy();
        if (fIsValidServer)
        {
            DimServer::start(name.c_str());

            // Give some time to come up before
            // the first log messages are sent
            sleep(1);
        }
    }
    ~DimStart()
    {
        if (!fIsValidServer)
            return;

        // Give some time for pending log messages to be
        // transmitted before the network is stopped
        sleep(1);
        DimServer::stop();
    }
};

// ***************************************************************************
/**
 @class StateMachineDim

 @brief Class for a state machine implementation within a DIM network

**/
// ***************************************************************************
#include "StateMachine.h"     // StateMachien

class StateMachineDim : public DimCommandHandler, public DimInfoHandler, public DimLog, public DimStart, public StateMachineImp
{
private:
    static const int fVersion;   /// Version number

    DimDescribedService fDescriptionStates; /// DimService propagating the state descriptions
    DimDescribedService fSrvState;          /// DimService offering fCurrentState
//    DimService fSrvVersion;        /// DimService offering fVersion

    void exitHandler(int code);  /// Overwritten DimCommand::exitHandler.
    void commandHandler();       /// Overwritten DimCommand::commandHandler 
    void infoHandler();          /// Overwritten DimInfo::infoHandler

    EventImp *CreateEvent(const std::string &name, const std::string &fmt);
    EventImp *CreateService(const std::string &name);

protected:
    /// This is an internal function to do some action in case of
    /// a state change, like updating the corresponding service.
    std::string SetCurrentState(int state, const char *txt="", const std::string &cmd="");

    void Lock();
    void UnLock();

public:
    StateMachineDim(std::ostream &out=std::cout, const std::string &name="DEFAULT");

    /// Redirect our own logging to fLog
    int Write(const Time &time, const std::string &txt, int qos=kMessage);

    bool AddStateName(const int state, const std::string &name, const std::string &doc="");

    bool MessageQueueEmpty() const { return DimLog::fLog.MessageQueueEmpty(); }
};

#endif
