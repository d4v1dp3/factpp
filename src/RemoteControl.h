#ifndef FACT_RemoteControl
#define FACT_RemoteControl

// **************************************************************************
/** @class RemoteControlImp

@brief This implements the basic functions of a remote control via dim

Through a ServiceList object this object subscribes to all available
SERVICE_LISTs  in the dim network. This allows to keep an up-to-date
list of all servers and services. Its ProcessCommand member function
allows to emit commands according to the services found in the network.
Its infoHandler() is called as an update notifier from the ClientList
object.

**/
// **************************************************************************
#include <string>

using namespace std;

class RemoteControlImp
{
protected:
    std::ostream &lin;           /// Output stream for local synchrounous output
    std::ostream &lout;          /// Output stream for local synchrounous output

    std::string fCurrentServer;  /// The server to which we currently cd'ed

protected:
    // Redirect asynchronous output to the output window
    RemoteControlImp(std::ostream &out, std::ostream &in) : lin(out), lout(in)
    {
    }
    virtual ~RemoteControlImp() { }
    bool ProcessCommand(const std::string &str, bool change=true);

    virtual bool HasServer(const std::string &) { return false; }
    virtual bool SendDimCommand(ostream &, const std::string &, const std::string &, bool = false) { return false; }
};

// **************************************************************************
/** @class RemoteControl

@brief Implements a remote control based on a Readline class for the dim network

This template implements all functions which overwrite any function from the
Readline class. Since several derivatives of the Readline class implement
different kind of Readline access, this class can be derived by any of them
due to its template argument. However, the normal case will be
deriving it from either Console or Shell.

@tparam T
   The base class for RemoteControl. Either Readlien or a class
    deriving from it. This is usually either Console or Shell.

**/
// **************************************************************************
#include "StateMachineDimControl.h"

#include "InterpreterV8.h"
#include "ReadlineColor.h"
#include "Event.h"
#include "tools.h"

template <class T>
class RemoteControl : public T, public RemoteControlImp, public InterpreterV8
{
protected:
    StateMachineDimControl *fImp;

    void SetSection(int s) { if (fImp) fImp->ChangeState(s); }

    int Write(const Time &time, const std::string &txt, int qos=MessageImp::kMessage)
    {
        if (!fImp)
            return 0;
        return fImp ? fImp->Write(time, txt, qos) : MessageImp::Write(time, txt, qos);
    }

    void exitHandler(int code) { if (dynamic_cast<MainImp*>(fImp)) dynamic_cast<MainImp*>(fImp)->Stop(code); else exit(code); }

    // ==================== Readline tab-completion =====================

    static void append(std::string &str)
    {
        str.append("/");
    }
    static void chop(std::string &str)
    {
        const size_t p = str.find_first_of('/');
        if (p!=string::npos)
            str = str.substr(p+1);
    }

    // This funtion defines which generator should be called.
    // If it returns 0 the standard readline generator are called.
    // Otherwise set the right generator with rl_completion_matches.
    char **Completion(const char *text, int start, int)
    {
        if (T::fScript=="java")
        {
            return T::Complete(JsGetCommandList(text, start), text);
        }

        // Get the whole buffer before the tab-position
        const string b = string(T::GetBuffer());
        const string s = b.substr(0, start);
        const string l = Tools::Trim(s.c_str());
        if (l.empty())
        {
            if (fCurrentServer.empty())
            {
                const size_t p1 = b.find_first_of(' ');
                const size_t p2 = b.find_first_of('/');

                if (p1==string::npos && p2!=string::npos)
                    return T::Complete(GetCommandList(), text);

                std::vector<std::string> v = GetServerList();
                for_each(v.begin(), v.end(), RemoteControl::append);
                return T::Complete(v, text);
            }
            else
            {
                std::vector<std::string> v = GetCommandList(fCurrentServer);
                for_each(v.begin(), v.end(), RemoteControl::chop);
                return T::Complete(v, text);
            }
        }
        return T::Complete(GetCommandList(l), text);
    }

    void EventHook(bool newline)
    {
        if (fImp && !fImp->HasServer(fCurrentServer))
            fCurrentServer = "";

        T::EventHook(newline);
    }

    // ===== Interface to access the DIM network through the StateMachine ====

    bool HasServer(const std::string &server) { return fImp ? fImp->HasServer(server) : false; }
    vector<string> GetServerList() const { return fImp ? fImp->GetServerList() : vector<string>(); }
    vector<string> GetCommandList(const string &server) const { return fImp ? fImp->GetCommandList(server) : vector<string>(); }
    vector<string> GetCommandList() const { return fImp ? fImp->GetCommandList() : vector<string>(); }
    int PrintStates(std::ostream &out, const std::string &serv="") const { return fImp ? fImp->PrintStates(out, serv) : 0; }
    int PrintDescription(std::ostream &out, bool iscmd, const std::string &serv="", const std::string &service="") const
    { return fImp ? fImp->PrintDescription(out, iscmd, serv, service) : 0; }
    bool SendDimCommand(ostream &out, const std::string &server, const std::string &str, bool do_throw=false)
    {
        if (do_throw)
            return fImp ? fImp->SendDimCommand(server, str, out) : false;

        try
        {
            return fImp ? fImp->SendDimCommand(server, str, out) : false;
        }
        catch (const runtime_error &e)
        {
            lout << kRed << e.what() << endl;
            return false;
        }
    }

    // ============ Pseudo-callback interface for the JavaScrip engine =======

    void  JsLoad(const std::string &)         { SetSection(-3); InterpreterV8::JsLoad(); }
    void  JsStart(const std::string &)        { SetSection(-2); }
    void  JsEnd(const std::string &)          { UnsubscribeAll(); InterpreterV8::JsEnd(); SetSection(-4); }
    bool  JsSend(const std::string &str)      { return ProcessCommand(str, false); }
    void  JsOut(const std::string &msg)       { lin << kDefault << msg << endl; }
    void  JsWarn(const std::string &msg)      { lin << kYellow << msg << endl; }
    void  JsResult(const std::string &msg)    { lin << kBlue << " = " << msg << '\n' << endl; }
    void  JsPrint(const std::string &msg)     { if (fImp) fImp->Comment(msg); }
    void  JsAlarm(const std::string &msg)     { if (fImp) fImp->Alarm(msg); }
    void  JsException(const std::string &str) { if (fImp) fImp->Error(str.empty()?"|":("| "+str)); }
    bool  JsHasState(int s) const             { return fImp && fImp->HasState(s); }
    bool  JsHasState(const string &n) const   { return fImp && (fImp->GetStateIndex(n)!=StateMachineImp::kSM_NotAvailable); }
    bool  JsSetState(int s)                   { if (!fImp || fImp->GetCurrentState()<2) return false; SetSection(s-4); return true; }
    int   JsGetState(const string &n) const   { return fImp ? fImp->GetStateIndex(n) : StateMachineImp::kSM_NotAvailable; }
    vector<State> JsGetStates(const string &server) { return fImp ? fImp->GetStates(server) : vector<State>(); }
    set<Service> JsGetServices() { return fImp ? fImp->GetServiceList() : set<Service>(); }
    vector<Description> JsGetDescription(const string &server) { return fImp ? fImp->GetDescription(server) : vector<Description>(); }
    State JsGetCurrentState() const
    {
        if (!fImp)
            return State();
        const int idx = fImp->GetCurrentState();
        return State(idx, fImp->GetStateName(idx), fImp->GetStateDescription(idx));
    }
    State JsState(const std::string &server)  { return fImp ? fImp->GetServerState(server) : State(-256, string()); }
    bool  JsNewState(int s, const string &n, const string &c)
    {
        return fImp && fImp->AddStateName(s, n, c);
    }

    /*
    void JsSleep(uint32_t ms)
    {
        const Time timeout = Time()+boost::posix_time::millisec(ms==0?1:ms);

        T::Lock();

        while (timeout>Time() && !T::IsScriptStopped())
            usleep(1);

        T::Unlock();
    }*/

    int JsWait(const string &server, int32_t state, uint32_t ms)
    {
        if (!fImp)
        {
            lout << kRed << "RemoteControl class not fully initialized." << endl;
            T::StopScript();
            return -1;
        }

        if (!HasServer(server))
        {
            lout << kRed << "Server '" << server << "' not found." << endl;
            T::StopScript();
            return -1;
        }

        T::Lock();

        const Time timeout = ms<=0 ? Time(Time::none) : Time()+boost::posix_time::millisec(ms);

        int rc = 0;
        do
        {
            State st = fImp->GetServerState(server);
            if (st.index==-256)
            {
                lout << kRed << "Server '" << server << "' disconnected." << endl;
                T::StopScript();
                return -1;
            }
            if (st.index==state)
            {
                rc = 1;
                break;
            }

            usleep(1);
        }
        while (timeout>Time() && !T::IsScriptStopped());

        T::Unlock();

        return rc;
    }

    vector<Description> JsDescription(const string &service)
    {
        return fImp ? fImp->GetDescription(service) :  vector<Description>();
    }

    struct EventInfo
    {
        EventImp *ptr;
        uint64_t counter;
        Event data;
        EventInfo(EventImp *p) : ptr(p), counter(0) { }
    };

    // Keep a copy of the data for access by V8
    map<string, EventInfo> fInfo;
    std::mutex fMutex;

    pair<uint64_t, EventImp *> JsGetEvent(const std::string &service)
    {
        // This function is called from JavaScript
        const lock_guard<mutex> lock(fMutex);

        const auto it = fInfo.find(service);

        // No subscription for this event available
        if (it==fInfo.end())
            return make_pair(0, static_cast<EventImp*>(NULL));

        EventInfo &info = it->second;

        // No event was received yet
        if (info.counter==0)
            return make_pair(0, static_cast<EventImp*>(NULL));

        return make_pair(info.counter-1, (EventImp*)&info.data);
    }

    int Handle(const EventImp &evt, const string &service)
    {
        // This function is called from the StateMachine
        fMutex.lock();

        const auto it = fInfo.find(service);

        // This should never happen... but just in case.
        if (it==fInfo.end())
        {
            fMutex.unlock();
            return StateMachineImp::kSM_KeepState;
        }

        EventInfo &info = it->second;

        const uint64_t cnt = ++info.counter;
        info.data = static_cast<Event>(evt);

        fMutex.unlock();

        JsHandleEvent(evt, cnt, service);

        return StateMachineImp::kSM_KeepState;
    }

    void *JsSubscribe(const std::string &service)
    {
        if (!fImp)
            return 0;

        // This function is called from JavaScript
        const lock_guard<mutex> lock(fMutex);

        // Do not subscribe twice
        if (fInfo.find(service)!=fInfo.end())
            return 0;

        EventImp *ptr = &fImp->Subscribe(service)(fImp->Wrap(bind(&RemoteControl<T>::Handle, this, placeholders::_1, service)));
        fInfo.insert(make_pair(service, EventInfo(ptr)));
        return ptr;
    }

    bool JsUnsubscribe(const std::string &service)
    {
        if (!fImp)
            return false;

        // This function is called from JavaScript
        const lock_guard<mutex> lock(fMutex);

        const auto it = fInfo.find(service);
        if (it==fInfo.end())
            return false;

        fImp->Unsubscribe(it->second.ptr);
        fInfo.erase(it);

        return true;
    }

    void UnsubscribeAll()
    {
        // This function is called from JavaScript
        const lock_guard<mutex> lock(fMutex);

        for (auto it=fInfo.begin(); it!=fInfo.end(); it++)
            fImp->Unsubscribe(it->second.ptr);

        fInfo.clear();
    }

    // ===========================================================================


public:
    // Redirect asynchronous output to the output window
    RemoteControl(const char *name) : T(name),
        RemoteControlImp(T::GetStreamOut(), T::GetStreamIn()), fImp(0)
    {
    }

    bool PrintGeneralHelp()
    {
        T::PrintGeneralHelp();
        lout << " " << kUnderline << "Specific commands:\n";
        lout << kBold << "   h,help <arg> " << kReset << "List help text for given server or command.\n";
        lout << kBold << "   svc,services " << kReset << "List all services in the network.\n";
        lout << kBold << "   st,states    " << kReset << "List all states in the network.\n";
        lout << kBold << "   > <text>     " << kReset << "Echo <text> to the output stream\n";
        lout << kBold << "   .s           " << kReset << "Wait for the state-machine to change to the given state.\n";
        lout <<          "                "              "     .s <server> [<state> [<timeout> [<label>]]]\n";
        lout <<          "                "              "<server>  The server for which state to wait (e.g. FTM_CONTROL)\n";
        lout <<          "                "              "<state>   The state id (see 'states') for which to wait (e.g. 3)\n";
        lout <<          "                "              "<imeout>  A timeout in millisenconds how long to wait (e.g. 500)\n";
        lout <<          "                "              "<label>   A label (number) until which everything is skipped in case of timeout\n";
        lout << kBold << "   .js file     " << kReset << "Execute a JavaScript\n";
        if (!StateMachineDimControl::fIsServer)
            lout << kBold << "   .java        " << kReset << "Start JavaScript interpreter\n";
        lout << endl;
        return true;
    }

    bool PrintCommands()
    {
        lout << endl << kBold << "List of commands:" << endl;
        PrintDescription(lout, true);
        return true;
    }

    // returns whether a command should be put into the history
    bool Process(const std::string &str)
    {
        if (str.substr(0, 2)=="h " || str.substr(0, 5)=="help ")
        {
            const size_t p1 = str.find_first_of(' ');
            const string svc = str.substr(p1+1);

            const size_t p3 = svc.find_first_of('/');
            const string s = svc.substr(0, p3);
            const string c = p3==string::npos?"":svc.substr(p3+1);

            lout << endl;
            if (!fCurrentServer.empty())
            {
                if (PrintDescription(lout, true, fCurrentServer, svc)==0)
                    lout << "   " << svc << ": <not found>" << endl;
            }
            else
            {
                if (PrintDescription(lout, true, s, c)==0)
                    lout << "   <no matches found>" <<endl;
            }

            return true;
        }

        if (str.substr(0, 4)==".js ")
        {
            string opt(str.substr(4)+" "+fImp->GetArguments());

            const map<string,string> data = Tools::Split(opt, true); // allow positional arguments
            if (opt.size()==0 && data.size()==0)
            {
                lout << kRed << "JavaScript filename missing." << endl;
                return true;
            }

            T::fScript = opt;

            T::Lock();
            JsRun(opt, data);
            T::Unlock();

            return true;
        }

        if ((str==".java" || str.substr(0,6)==".java ") && !StateMachineDimControl::fIsServer)
        {
            string opt(str+" "+fImp->GetArguments());

            const map<string,string> data = Tools::Split(opt, true); // allow positional arguments

            T::fScript = "java";

            T::Lock();
            JsRun("", data);
            T::Unlock();

            T::fScript = "";

            return true;
        }

        if (str.substr(0, 3)==".s ")
        {
            istringstream in(str.substr(3));

            int state=-100, ms=0;
            string server;

            in >> server >> state >> ms;
            if (state==-100)
            {
                lout << kRed << "Couldn't parse state id in '" << str.substr(3) << "'" << endl;
                return true;
            }

            T::Lock();
            const int rc = JsWait(server, state, ms);
            T::Unlock();

            if (rc<0 || rc==1)
                return true;

            int label = -1;
            in >> label;
            if (in.fail() && !in.eof())
            {
                lout << kRed << "Invalid label in '" << str.substr(3) << "'" << endl;
                T::StopScript();
                return true;
            }
            T::SetLabel(label);

            return true;
        }

        if (str[0]=='>')
        {
            fImp->Comment(Tools::Trim(str.substr(1)));
            return true;
        }

        if (ReadlineColor::Process(lout, str))
            return true;

        if (T::Process(str))
            return true;

        if (str=="services" || str=="svc")
        {
            PrintDescription(lout, false);
            return true;
        }

        if (str=="states" || str=="st")
        {
            PrintStates(lout);
            return true;
        }

        return !ProcessCommand(str);
    }

    void SetReceiver(StateMachineDimControl &imp)
    {
        fImp = &imp;
        fImp->SetStateCallback(bind(&InterpreterV8::JsHandleState, this, placeholders::_1, placeholders::_2));
        fImp->SetInterruptHandler(bind(&InterpreterV8::JsHandleInterrupt, this, placeholders::_1));
    }
};



// **************************************************************************
/** @class RemoteStream

 */
// **************************************************************************
#include "Console.h"

class RemoteStream : public RemoteControl<ConsoleStream>
{
public:
    RemoteStream(const char *name, bool null = false)
        : RemoteControl<ConsoleStream>(name) { SetNullOutput(null); }
};

// **************************************************************************
/** @class RemoteConsole

@brief Derives the RemoteControl from Control and adds a proper prompt

This is basically a RemoteControl, which derives through the template
argument from the Console class. It enhances the functionality of
the remote control with a proper updated prompt.

 */
// **************************************************************************

class RemoteConsole : public RemoteControl<Console>
{
public:
    RemoteConsole(const char *name, bool continous=false) :
        RemoteControl<Console>(name)
    {
        SetContinous(continous);
    }
    string GetUpdatePrompt() const;
};

// **************************************************************************
/** @class RemoteShell

@brief Derives the RemoteControl from Shell and adds colored prompt

This is basically a RemoteControl, which derives through the template
argument from the Shell class. It enhances the functionality of
the local control with a proper updated prompt.

 */
// **************************************************************************
#include "Shell.h"

class RemoteShell : public RemoteControl<Shell>
{
public:
    RemoteShell(const char *name, bool = false) :
        RemoteControl<Shell>(name)
    {
    }
    string GetUpdatePrompt() const;
};

#endif
