#ifndef FACT_DimNetwork
#define FACT_DimNetwork

// **************************************************************************
/** @class StateClient

@brief A simple Dim client diriving from MessageDimRX subscribing to the STATE service

This is a simple dim client which subscribes to the MESSAGE and STATE
service of a server. It stores the last state and its time as well as
the last message sent.

**/
// **************************************************************************
#include "MessageDim.h"
#include "Time.h"

class StateClient : public MessageDimRX
{
private:
    Time fStateTime;           /// Combine into one MTime (together with value)
    int  fState;               /// -2 not initialized, -1 not connected, 0>= state of client

    DimStampedInfo fInfoState; /// The dim service subscription

protected:
    void infoHandler();

public:
    StateClient(const std::string &name, MessageImp &imp);

    bool IsConnected() const { return fState>=0; }
    int  GetState() const    { return fState;    }

    const char *GetName() const { return const_cast<DimStampedInfo&>(fInfoState).getName(); }
};



// **************************************************************************
/** @class DimNetwork

@brief Implements automatic subscription to MESSAGE and STATE services

This class derives from DimServiceInfoList, so that it has a full
overview of all commands and services existing in the current Dim
network. In addition it automatically subscribes to all available
MESSAGE and STATE services and redirects them to its MessageImp base class.

@todo
- maybe the StateClient can be abondoned, this way it would be possible
  to subscribe to only available MESSAGE and STATE services

**/
// **************************************************************************
#include "DimServiceInfoList.h"
#include "DimErrorRedirecter.h"

using namespace std;

class DimNetwork : public MessageImp, public DimErrorRedirecter, public DimServiceInfoListImp
{
private:
    void DeleteClientList();

protected:
    typedef std::map<const std::string, StateClient*> ClientList;

    ClientList fClientList; /// A list with all MESSAGE services to which we subscribed

    void AddServer(const std::string &s);
    void RemoveServer(std::string s);
    void RemoveAllServers();

public:
    DimNetwork(std::ostream &out=std::cout)
        : MessageImp(out), DimErrorRedirecter(static_cast<MessageImp&>(*this))
    {
    }
    ~DimNetwork()
    {
        DeleteClientList();
    }

    int GetCurrentState(const string &server) const;
};


#endif
