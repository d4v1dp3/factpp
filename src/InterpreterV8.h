#ifndef FACT_InterpreterV8
#define FACT_InterpreterV8

#include <map>
#include <set>
#include <list>
#include <string>
#include <thread>

#ifdef HAVE_V8
#include <v8.h>
#endif

#include "State.h"
#include "Service.h"
#include "Description.h"
#include "EventImp.h"

class Database;

#ifdef HAVE_NOVA
struct ln_rst_time;
#endif

class InterpreterV8
{
    static InterpreterV8 *This;

    // The main thread id, needed to be able to terminate
    // the thread forcefully from 'the outside'
    int fThreadId;
    std::set<int> fThreadIds;

    // A loookup table which allows to indentify the
    // the JavaScript object corrsponding to the
    // service name (for checking of an .onchange
    // subscription exists for that object)
    std::map<std::string, v8::Persistent<v8::Object>> fReverseMap;

    // Lookup table for the callbacks in cases of state changes
    std::map<std::string, v8::Persistent<v8::Object>> fStateCallbacks;

    // List of all threads
    std::vector<std::thread> fThreads;

    // List of all states already set
    std::vector<std::pair<int, std::string>> fStates;

    // Interrupt handler
    v8::Persistent<v8::Object> fInterruptCallback;

    static v8::Handle<v8::FunctionTemplate> fTemplateLocal;
    static v8::Handle<v8::FunctionTemplate> fTemplateSky;
    static v8::Handle<v8::FunctionTemplate> fTemplateEvent;
    static v8::Handle<v8::FunctionTemplate> fTemplateDescription;

#ifdef HAVE_SQL
    std::list<Database*> fDatabases;
#endif

    std::string fIncludePath;

#ifdef HAVE_V8
    bool HandleException(v8::TryCatch &try_catch, const char *where);
    void ExecuteConsole();
    v8::Handle<v8::Value> ExecuteCode(const std::string &code, const std::string &file="internal");
    v8::Handle<v8::Value> ExecuteInternal(const std::string &code);

    void Thread(int &id, v8::Persistent<v8::Object> _this, v8::Persistent<v8::Function> func, uint32_t ms);

    std::vector<std::string> ValueToArray(const v8::Handle<v8::Value> &val, bool only=true);

    v8::Handle<v8::Value> FuncWait(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSend(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSleep(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncTimeout(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncThread(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncKill(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncLog(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncAlarm(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncOut(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncWarn(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncFile(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSendMail(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSendCurl(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncInclude(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncExit(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncState(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSetState(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncGetState(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncGetStates(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncGetDescription(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncGetServices(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncNewState(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSetInterrupt(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncTriggerInterrupt(const v8::Arguments& args);
    //v8::Handle<v8::Value> FuncOpen(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncSubscription(const v8::Arguments& args);
    v8::Handle<v8::Value> FuncGetData(const v8::Arguments &args);
    v8::Handle<v8::Value> FuncClose(const v8::Arguments &args);
    v8::Handle<v8::Value> FuncQuery(const v8::Arguments &args);
    v8::Handle<v8::Value> FuncDatabase(const v8::Arguments &args);
    v8::Handle<v8::Value> FuncDbQuery(const v8::Arguments &args);
    v8::Handle<v8::Value> FuncDbClose(const v8::Arguments &args);
    v8::Handle<v8::Value> OnChangeSet(v8::Local<v8::String>, v8::Local<v8::Value>, const v8::AccessorInfo &);

    static v8::Handle<v8::Value> Constructor(const v8::Arguments &args);

    static v8::Handle<v8::Value> ConstructorMail(const v8::Arguments &args);
    static v8::Handle<v8::Value> ConstructorCurl(const v8::Arguments &args);

#ifdef HAVE_NOVA
    static double GetDataMember(const v8::Arguments &args, const char *name);

    static v8::Handle<v8::Value> CalcDist(const v8::Arguments &args, const bool);

    static v8::Handle<v8::Value> LocalToString(const v8::Arguments &args);
    static v8::Handle<v8::Value> SkyToString(const v8::Arguments &args);
    static v8::Handle<v8::Value> MoonToString(const v8::Arguments &args);
    static v8::Handle<v8::Value> LocalDist(const v8::Arguments &args);
    static v8::Handle<v8::Value> SkyDist(const v8::Arguments &args);
    static v8::Handle<v8::Value> MoonDisk(const v8::Arguments &args);
    static v8::Handle<v8::Value> LocalToSky(const v8::Arguments &args);
    static v8::Handle<v8::Value> SkyToLocal(const v8::Arguments &args);
    static v8::Handle<v8::Value> MoonToLocal(const v8::Arguments &args);
    static v8::Handle<v8::Value> ConstructorMoon(const v8::Arguments &args);
    static v8::Handle<v8::Value> ConstructorSky(const v8::Arguments &args);
    static v8::Handle<v8::Value> ConstructorLocal(const v8::Arguments &args);
    static v8::Handle<v8::Value> MoonHorizon(const v8::Arguments &args);
    static v8::Handle<v8::Value> SunHorizon(const v8::Arguments &args);
#endif

    static v8::Handle<v8::Value> WrapInclude(const v8::Arguments &args)  { if (This) return This->FuncInclude(args);  else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapFile(const v8::Arguments &args)     { if (This) return This->FuncFile(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSendMail(const v8::Arguments &args) { if (This) return This->FuncSendMail(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSendCurl(const v8::Arguments &args) { if (This) return This->FuncSendCurl(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapLog(const v8::Arguments &args)      { if (This) return This->FuncLog(args);      else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapAlarm(const v8::Arguments &args)    { if (This) return This->FuncAlarm(args);    else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapOut(const v8::Arguments &args)      { if (This) return This->FuncOut(args);      else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapWarn(const v8::Arguments &args)     { if (This) return This->FuncWarn(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapWait(const v8::Arguments &args)     { if (This) return This->FuncWait(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSend(const v8::Arguments &args)     { if (This) return This->FuncSend(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSleep(const v8::Arguments &args)    { if (This) return This->FuncSleep(args);    else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapTimeout(const v8::Arguments &args)  { if (This) return This->FuncTimeout(args);  else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapThread(const v8::Arguments &args)   { if (This) return This->FuncThread(args);   else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapKill(const v8::Arguments &args)     { if (This) return This->FuncKill(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapExit(const v8::Arguments &args)     { if (This) return This->FuncExit(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapState(const v8::Arguments &args)    { if (This) return This->FuncState(args);    else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapNewState(const v8::Arguments &args) { if (This) return This->FuncNewState(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSetState(const v8::Arguments &args) { if (This) return This->FuncSetState(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapGetState(const v8::Arguments &args) { if (This) return This->FuncGetState(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapGetStates(const v8::Arguments &args){ if (This) return This->FuncGetStates(args);else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapGetDescription(const v8::Arguments &args){ if (This) return This->FuncGetDescription(args);else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapGetServices(const v8::Arguments &args){ if (This) return This->FuncGetServices(args);else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSetInterrupt(const v8::Arguments &args){ if (This) return This->FuncSetInterrupt(args);else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapTriggerInterrupt(const v8::Arguments &args){ if (This) return This->FuncTriggerInterrupt(args);else return v8::Undefined(); }
    //static v8::Handle<v8::Value> WrapOpen(const v8::Arguments &args)     { if (This) return This->FuncOpen(args);     else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapSubscription(const v8::Arguments &args){ if (This) return This->FuncSubscription(args);else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapGetData(const v8::Arguments &args)  { if (This) return This->FuncGetData(args);  else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapClose(const v8::Arguments &args)    { if (This) return This->FuncClose(args);    else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapQuery(const v8::Arguments &args)    { if (This) return This->FuncQuery(args);    else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapDatabase(const v8::Arguments &args) { if (This) return This->FuncDatabase(args); else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapDbQuery(const v8::Arguments &args)  { if (This) return This->FuncDbQuery(args);  else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapDbClose(const v8::Arguments &args)  { if (This) return This->FuncDbClose(args);  else return v8::Undefined(); }
    static v8::Handle<v8::Value> WrapOnChangeSet(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
    {
        if (This) return This->OnChangeSet(prop, value, info);  else return v8::Undefined();
    }

    static v8::Handle<v8::Value> OnChangeGet(v8::Local<v8::String>, const v8::AccessorInfo &)
    {
        return v8::Handle<v8::Value>();
    }

    static v8::Handle<v8::Value> Convert(char type, const char* &ptr);
    v8::Handle<v8::Value> ConvertEvent(const EventImp *evt, uint64_t, const char *str);
#endif

    v8::Handle<v8::Value> HandleInterruptImp(std::string, uint64_t);

public:
    InterpreterV8();
    virtual ~InterpreterV8()
    {
        This = 0;

#ifdef HAVE_V8
        //v8::Locker locker;
        v8::V8::Dispose();
#endif
    }

    std::vector<std::string> JsGetCommandList(const char *, int) const;

    virtual void  JsLoad(const std::string & = "");
    virtual void  JsStart(const std::string &) { }
    virtual void  JsEnd(const std::string & = "");
    virtual void  JsPrint(const std::string & = "") { }
    virtual void  JsAlarm(const std::string & = "") { }
    virtual void  JsOut(const std::string &) { }
    virtual void  JsWarn(const std::string &) { }
    virtual void  JsResult(const std::string &) { }
    virtual void  JsException(const std::string &) { }
    virtual bool  JsSend(const std::string &) { return true; }
    //virtual void  JsSleep(uint32_t) { }
    //virtual int   JsWait(const std::string &, int32_t, uint32_t) { return -1; };
    virtual State JsState(const std::string &) { return State(); };
    virtual void *JsSubscribe(const std::string &) { return 0; };
    virtual bool  JsUnsubscribe(const std::string &) { return false; };

    virtual bool  JsNewState(int, const std::string&, const std::string&) { return false; }
    virtual bool  JsSetState(int) { return false; }
    virtual bool  JsHasState(int) const { return false; }
    virtual bool  JsHasState(const std::string &) const { return false; }
    virtual int   JsGetState(const std::string &) const { return -2; }
    virtual State JsGetCurrentState() const { return State(); }
    virtual std::vector<State> JsGetStates(const std::string &) { return std::vector<State>(); }
    virtual std::set<Service> JsGetServices() { return std::set<Service>(); }
    virtual std::vector<Description> JsGetDescription(const std::string &) { return std::vector<Description>(); }

    virtual std::vector<Description> JsDescription(const std::string &) { return std::vector<Description>(); };
    virtual std::pair<uint64_t, EventImp *> JsGetEvent(const std::string &) { return std::make_pair(0, (EventImp*)0); };

    int JsHandleInterrupt(const EventImp &);
    void JsHandleEvent(const EventImp &, uint64_t, const std::string &);
    void JsHandleState(const std::string &, const State &);

    void AddFormatToGlobal();

    bool JsRun(const std::string &, const std::map<std::string,std::string> & = std::map<std::string,std::string>());
    static void JsStop();
};

#ifndef HAVE_V8
inline bool InterpreterV8::JsRun(const std::string &, const std::map<std::string,std::string> &) { return false; }
inline void InterpreterV8::JsStop() { }
#endif

#endif
