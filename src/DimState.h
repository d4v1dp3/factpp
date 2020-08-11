#ifndef FACT_DimState
#define FACT_DimState

#include <set>
#include <string>
#include <functional>

#include "State.h"
#include "Service.h"
#include "EventImp.h"
#include "WindowLog.h"
#include "Description.h"
#include "StateMachineImp.h"

class DimState
{
public:
    enum
    {
        kOffline      = -256,
        kNotAvailable = -257,
    };

protected:
    typedef std::function<int(const EventImp &)> callback;

    callback fCallback;

    void HandlerImp(const EventImp &evt)
    {
        const bool disconnected = evt.GetSize()==0;

        last = cur;
        cur  = std::make_pair(evt.GetTime(), disconnected ? kOffline : evt.GetQoS());

        msg = disconnected ? "" : evt.GetString();
    }

    int Callback(const EventImp &evt)
    {
        return fCallback ? fCallback(evt) : StateMachineImp::kSM_KeepState;
    }

    virtual int Handler(const EventImp &evt)
    {
        HandlerImp(evt);
        return Callback(evt);
    }

public:
    DimState() { }
    DimState(const std::string &n, const std::string s="STATE") : server(n),
        service(n+"/"+s),
        last(std::make_pair(Time(), kOffline)), cur(std::make_pair(Time(), kOffline))
    {
    }
    virtual ~DimState()
    {
    }

    /*const*/ std::string server;
    /*const*/ std::string service;

    std::pair<Time, int32_t> last;
    std::pair<Time, int32_t> cur;
    std::string msg;

    virtual void Subscribe(StateMachineImp &imp)
    {
        imp.Subscribe(service)
            (imp.Wrap(std::bind(&DimState::Handler, this, std::placeholders::_1)));
    }

    void SetCallback(const callback &cb)
    {
        fCallback = cb;
    }

    const Time    &time() const  { return cur.first; }
    const int32_t &state() const { return cur.second; }

    bool online() const { return state()>kOffline; }

    virtual State description() const { return State(kNotAvailable, ""); }
};

inline std::ostream &operator<<(std::ostream& out, const DimState &s)
{
    const State rc = s.description();

    out << s.time().GetAsStr("%H:%M:%S.%f").substr(0, 12) << " - ";
    out << kBold << s.server;

    if (s.state()==DimState::kOffline)
        return out << ": Offline";

    if (rc.index==DimState::kNotAvailable)
        return out;

    out << ": ";

//    if (rc.index==-2)
//        out << s.state();
//    else
        out << rc.name << "[" << rc.index << "]";

    if (!rc.comment.empty())
        out << " - " << kBlue << rc.comment;

    return out;
}


class DimDescribedState : public DimState
{
    typedef std::function<void()> callback_desc;

    callback_desc fCallbackStates;

    virtual void CallbackStates()
    {
        if (fCallbackStates)
            fCallbackStates();
    }


public:
    DimDescribedState(const std::string &n) : DimState(n)
    {
    }

    std::vector<State> states;

    virtual void Subscribe(StateMachineImp &imp)
    {
        imp.Subscribe(server+"/STATE_LIST")
            (imp.Wrap(std::bind(&DimDescribedState::HandleDesc, this, std::placeholders::_1)));

        DimState::Subscribe(imp);
    }

    void SetCallbackStates(const callback_desc &cb)
    {
        fCallbackStates = cb;
    }

    int HandleDesc(const EventImp &evt)
    {
        if (evt.GetSize()>0)
        {
            states = State::SplitStates(evt.GetString());
            states.emplace_back(kOffline, "Offline");

            CallbackStates();
        }

        return StateMachineImp::kSM_KeepState;
    }

    State description() const
    {
        for (auto it=states.begin(); it!=states.end(); it++)
            if (it->index==state())
                return State(it->index, it->name, it->comment, time());

        return State(kNotAvailable, "n/a");
    }
};

class DimDescriptions : public DimDescribedState
{
    typedef std::function<void()> callback_desc;

    callback_desc fCallbackDescriptions;

    virtual void CallbackDescriptions()
    {
        if (fCallbackDescriptions)
            fCallbackDescriptions();
    }

public:
    DimDescriptions(const std::string &n) : DimDescribedState(n)
    {
    }

    std::vector<std::vector<Description>> descriptions;

    virtual void Subscribe(StateMachineImp &imp)
    {
        imp.Subscribe(server+"/SERVICE_DESC")
            (imp.Wrap(std::bind(&DimDescriptions::HandleServiceDesc, this, std::placeholders::_1)));

        DimDescribedState::Subscribe(imp);
    }

    void SetCallbackDescriptions(const callback_desc &cb)
    {
        fCallbackDescriptions = cb;
    }


    int HandleServiceDesc(const EventImp &evt)
    {
        descriptions.clear();
        if (evt.GetSize()>0)
        {
            std::string buf;
            std::stringstream stream(evt.GetString());
            while (getline(stream, buf, '\n'))
                descriptions.push_back(Description::SplitDescription(buf));
        }

        CallbackDescriptions();

        return StateMachineImp::kSM_KeepState;
    }
};

class DimVersion : public DimState
{
    int Handler(const EventImp &evt)
    {
        HandlerImp(evt);

        cur.second = evt.GetSize()==4 ? evt.GetInt() : kOffline;
        if (cur.second==0)
            cur.second=kOffline;

        return Callback(evt);
    }

public:
    DimVersion() : DimState("DIS_DNS", "VERSION_NUMBER") { }

    std::string version() const
    {
        if (!online())
            return "Offline";

        std::ostringstream out;
        out << "V" << state()/100 << 'r' << state()%100;
        return out.str();
    }

    State description() const
    {
        return State(state(), version(), "", time());
    }
};

class DimControl : public DimState
{
    std::map<std::string, callback> fCallbacks;

    int Handler(const EventImp &evt)
    {
        HandlerImp(evt);

        shortmsg    = msg;
        file        = "";
        scriptdepth = -1;

        // Find begining of descriptor
        const size_t p0 = msg.find_first_of(' ');
        if (p0==std::string::npos)
            return StateMachineImp::kSM_KeepState;

        // Find begining of filename
        const size_t p1 = msg.find_first_of(':');
        if (p1==std::string::npos)
            return StateMachineImp::kSM_KeepState;

        // Find end of filename
        const size_t p2 = msg.find_last_of('[');
        if (p2==std::string::npos)
            return StateMachineImp::kSM_KeepState;

        scriptdepth = atoi(msg.c_str()+p0+1);
        file = msg.substr(p1+1, p2-p1-1);

        shortmsg.insert(0, msg.substr(p0+1, p1-p0));
        shortmsg.erase(p1+1,p2-p0-1);

        const int rc = Callback(evt);

        const auto func = fCallbacks.find(file);
        if (func==fCallbacks.end())
            return rc;

        // Call callback
        return func->second(evt);
    }


public:
    DimControl() : DimState("DIM_CONTROL") { }

    std::string file;
    std::string shortmsg;
    int scriptdepth;

    void AddCallback(const std::string &script, const callback &cb)
    {
        fCallbacks[script] = cb;
    }

    State description() const
    {
        return State(state(), "Current label", "", time());
    }
};

class DimDnsServerList
{
protected:
    typedef std::function<void(const std::string &)> callback_srv;
    typedef std::function<void(const EventImp &)>    callback_evt;

    callback_srv fCallbackServerAdd;
    callback_srv fCallbackServerRemove;
    callback_evt fCallbackServerEvent;

    std::set<std::string> fServerList;

    void HandlerServerImp(const EventImp &evt);

    virtual void CallbackServerAdd(const std::string &str)
    {
        if (fCallbackServerAdd)
            fCallbackServerAdd(str);
    }
    virtual void CallbackServerRemove(const std::string &str)
    {
        if (fCallbackServerRemove)
            fCallbackServerRemove(str);
    }
    virtual void CallbackServerEvent(const EventImp &evt)
    {
        if (fCallbackServerEvent)
            fCallbackServerEvent(evt);
    }
    virtual int HandlerServer(const EventImp &evt)
    {
        HandlerServerImp(evt);
        CallbackServerEvent(evt);

        return StateMachineImp::kSM_KeepState;
    }

public:
    DimDnsServerList() 
    {
    }
    virtual ~DimDnsServerList()
    {
    }

    Time  time;
    std::string msg;

    virtual void Subscribe(StateMachineImp &imp)
    {
        imp.Subscribe("DIS_DNS/SERVER_LIST")
            (imp.Wrap(std::bind(&DimDnsServerList::HandlerServer, this, std::placeholders::_1)));
    }

    void SetCallbackServerAdd(const callback_srv &cb)
    {
        fCallbackServerAdd = cb;
    }

    void SetCallbackServerRemove(const callback_srv &cb)
    {
        fCallbackServerRemove = cb;
    }

    void SetCallbackServerEvent(const callback_evt &cb)
    {
        fCallbackServerEvent = cb;
    }

    //const Time &time() const { return time; }
    //const std::string &msg() const { return msg; }
};

class DimDnsServiceList : public DimDnsServerList
{
    StateMachineImp *fStateMachine;

    typedef std::function<void(const Service &)> callback_svc;

    callback_svc fCallbackServiceAdd;
    //callback_evt fCallbackServiceEvt;

    std::vector<std::string> fServiceList;

    std::set<std::string> fServers;

    void CallbackServerAdd(const std::string &server)
    {
        DimDnsServerList::CallbackServerAdd(server);

        if (fServers.find(server)!=fServers.end())
            return;

        fStateMachine->Subscribe(server+"/SERVICE_LIST")
            (fStateMachine->Wrap(std::bind(&DimDnsServiceList::HandlerServiceList, this, std::placeholders::_1)));

        fServers.insert(server);
    }

    void HandlerServiceListImp(const EventImp &evt);

/*
    virtual void CallbackServiceEvt(const EventImp &evt)
    {
        if (fCallbackServiceEvt)
            fCallbackServiceEvt(evt);
    }
*/
    virtual int HandlerServiceList(const EventImp &evt)
    {
        HandlerServiceListImp(evt);
        //CallbackServiceEvent(evt);

        return StateMachineImp::kSM_KeepState;
    }


    virtual void CallbackServiceAdd(const Service &service)
    {
        if (fCallbackServiceAdd)
            fCallbackServiceAdd(service);
    }

public:
    DimDnsServiceList() : fStateMachine(0)
    {
    }

    void Subscribe(StateMachineImp &imp)
    {
        fStateMachine = &imp;
        DimDnsServerList::Subscribe(imp);
    }

    void SetCallbackServiceAdd(const callback_svc &cb)
    {
        fCallbackServiceAdd = cb;
    }
/*
    void SetCallbackServiceEvt(const callback_svc &cb)
    {
        fCallbackServiceEvt = cb;
    }
*/
};

#endif
