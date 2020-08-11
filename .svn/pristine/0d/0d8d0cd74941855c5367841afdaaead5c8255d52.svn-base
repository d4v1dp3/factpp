#include "InterpreterV8.h"

#ifdef HAVE_V8

#include <fstream>
#include <sstream>
#include <iomanip>

#include <sys/stat.h>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#ifdef HAVE_NOVA
#include "nova.h"
#endif

#ifdef HAVE_SQL
#include "Database.h"
#endif

#include <v8.h>

#include "dim.h"
#include "tools.h"
#include "Readline.h"
#include "izstream.h"

#include "WindowLog.h"

using namespace std;
using namespace v8;

v8::Handle<v8::FunctionTemplate> InterpreterV8::fTemplateLocal;
v8::Handle<v8::FunctionTemplate> InterpreterV8::fTemplateSky;
v8::Handle<v8::FunctionTemplate> InterpreterV8::fTemplateEvent;
v8::Handle<v8::FunctionTemplate> InterpreterV8::fTemplateDescription;
//v8::Handle<v8::FunctionTemplate> InterpreterV8::fTemplateDatabase;


// ==========================================================================
//                           Some documentation
// ==========================================================================
//
// Threads:
// --------
// In most cases Js* and other calls to native C++ code could be wrapped
// with an Unlocker to allow possible other JavaScipt 'threads' to run
// during that time. However, all of these calls should take much less than
// the preemption time of 10ms, so it would just be a waste of tim.
//
// Termination:
// ------------
// Each thread running V8 code needs to be signalled individually for
// termination. Therefor a list of V8 thread ids is created.
//
// If termination has already be signalled, no thread should start running
// anymore (thy could, e.g., wait for their locking). So after locking
// it has to be checked if the thread was terminated already. Note
// that all calls to Terminate() must be locked to ensure that fThreadId
// is correct when it is checked.
//
// The current thread id must be added to fThreadIds _before_ any
// function is called after Locking and before execution is given
// back to JavaScript, e.g. in script->Run(). So until the thread
// is added to the list Terminate will not be executed. If Terminate
// is then executed, it is ensured that the current thread is
// already in the list. If terminate has been called before
// the Locking, the check for the validiy of fThreadId ensures that
// nothing is executed.
//
// Empty handles:
// --------------
// If exceution is terminated, V8 calls might return with empty handles,
// e.g. Date::New(). Therefore, the returned handles of these calls have to
// be checked in all placed to avoid that V8 will core dump.
//
// HandleScope:
// ------------
// A handle scope is a garbage collector and collects all handles created
// until it goes out of scope. Handles which are not needed anymore are
// then deleted. To return a handle from a HandleScope you need to use
// Close(). E.g., String::AsciiValue does not create a new handle and
// hence does not need a HandleScope. Any ::New will need a handle scope.
// Forgetting the HandleScope could in principle fill your memory,
// but everything is properly deleted by the global HandleScope at
// script termination.
//
// Here is another good reference for v8, also containing some
// good explanations for the meaning of handles, persistent handles
// and weak handles: http://create.tpsitulsa.com/wiki/V8_Cookbook
//
// ==========================================================================
//                            Simple interface
// ==========================================================================

Handle<Value> InterpreterV8::FuncExit(const Arguments &)
{
    V8::TerminateExecution(fThreadId);

    // we have to throw an excption to make sure that the
    // calling thread does not go on executing until it
    // has realized that it should terminate
    return ThrowException(Null());
}

Handle<Value> InterpreterV8::FuncSleep(const Arguments& args)
{
    if (args.Length()==0)
    {
        // Theoretically, the CPU usage can be reduced by maybe a factor
        // of four using a larger value, but this also means that the
        // JavaScript is locked for a longer time.
        const Unlocker unlock;
        usleep(1000);
        return Undefined();
    }

    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be exactly 1."));

    if (!args[0]->IsUint32())
        return ThrowException(String::New("Argument 1 must be an uint32."));

    // Using a Javascript function has the advantage that it is fully
    // interruptable without the need of C++ code
    const string code =
        "(function(){"
        "var t=new Date();"
        "while ((new Date()-t)<"+to_string(args[0]->Int32Value())+") v8.sleep();"
        "})();";

    return ExecuteInternal(code);
}

Handle<Value> InterpreterV8::FuncTimeout(const Arguments &args)
{
    if (args.Length()<2)
        return ThrowException(String::New("Number of arguments must be at least two."));

    if (!args[0]->IsNull() && !args[0]->IsInt32())
        return ThrowException(String::New("Argument 0 not null and not an int32."));

    if (!args[1]->IsFunction())
        return ThrowException(String::New("Argument 1 not a function."));

    if (args.Length()>2 && !args[2]->IsObject())
        return ThrowException(String::New("Argument 2 not an object."));

    const int32_t timeout = args[0]->IsNull() ? 0 : args[0]->Int32Value();
    const bool    null    = args[0]->IsNull();

    HandleScope handle_scope;

    Handle<Function> func = Handle<Function>::Cast(args[1]);

    const int nn = args.Length()==2 ? 0 : args.Length()-3;

    vector<Handle<Value>> argv(nn);
    for (int i=0; i<nn; i++)
        argv[i] = args[i+3];

    Time t;
    while (1)
    {
        const Handle<Value> rc = args.Length()<3 ?
            func->Call(func, nn, argv.data()) :
            func->Call(args[2]->ToObject(), nn, argv.data());

        if (rc.IsEmpty())
            return Undefined();

        if (!rc->IsUndefined())
            return handle_scope.Close(rc);

        if (!null && Time()-t>=boost::posix_time::milliseconds(abs(timeout)))
            break;

        // Theoretically, the CPU usage can be reduced by maybe a factor
        // of four using a larger value, but this also means that the
        // JavaScript is locked for a longer time.
        const Unlocker unlock;
        usleep(1000);
    }

    if (timeout<0)
        return Undefined();

    const string str = "Waiting for func to return a defined value timed out.";
    return ThrowException(String::New(str.c_str()));
}

void InterpreterV8::Thread(int &id, Persistent<Object> _this, Persistent<Function> func, uint32_t ms)
{
    const Locker lock;

    if (fThreadId<0)
    {
        id = -1;
        return;
    }

    // Warning: As soon as id is set, the parent of this thread might terminate
    //          and hance the reference to id does not exist anymore. So, id
    //          is just a kind of return value and must not be used at all
    //          otherwise.

    const int id_local = V8::GetCurrentThreadId();
    id = id_local;
    fThreadIds.insert(id_local);

    const HandleScope handle_scope;

    func->CreationContext()->Enter();

    TryCatch exception;

    const bool rc = ms==0 || !ExecuteInternal("v8.sleep("+to_string(ms)+");").IsEmpty();
    if (rc)
    {
        if (_this.IsEmpty())
            func->Call(func, 0, NULL);
        else
            func->Call(_this, 0, NULL);
    }

    func.Dispose();
    _this.Dispose();

    fThreadIds.erase(id_local);

    if (!HandleException(exception, "thread"))
        V8::TerminateExecution(fThreadId);

    func->CreationContext()->Exit();
}

Handle<Value> InterpreterV8::FuncThread(const Arguments& args)
{
    if (!args.IsConstructCall())
        return ThrowException(String::New("Thread must be called as constructor."));

    if (args.Length()!=2 && args.Length()!=3)
        return ThrowException(String::New("Number of arguments must be two or three."));

    if (!args[0]->IsUint32())
        return ThrowException(String::New("Argument 0 not an uint32."));

    if (!args[1]->IsFunction())
        return ThrowException(String::New("Argument 1 not a function."));

    if (args.Length()==3 && !args[2]->IsObject())
        return ThrowException(String::New("Argument 2 not an object."));

    //if (!args.IsConstructCall())
    //    return Constructor(args);

    const HandleScope handle_scope;

    Handle<Function> handle = Handle<Function>::Cast(args[1]);

    Persistent<Function> func =  Persistent<Function>::New(handle);
    Persistent<Object> _this;
    if (args.Length()==3)
        _this = Persistent<Object>::New(args[2]->ToObject());

    const uint32_t ms = args[0]->Uint32Value();

    int id=-2;
    fThreads.push_back(thread(bind(&InterpreterV8::Thread, this, ref(id), _this, func, ms)));
    {
        // Allow the thread to lock, so we can get the thread id.
        const Unlocker unlock;
        while (id==-2)
            usleep(1);
    }

    Handle<Object> self = args.This();

    self->Set(String::New("id"), Integer::NewFromUnsigned(id), ReadOnly);
    self->Set(String::New("kill"), FunctionTemplate::New(WrapKill)->GetFunction(), ReadOnly);

    return Undefined();
}

Handle<Value> InterpreterV8::FuncKill(const Arguments& args)
{
    const uint32_t id = args.This()->Get(String::New("id"))->Uint32Value();

    V8::TerminateExecution(id);

    return Boolean::New(fThreadIds.erase(id));
}

Handle<Value> InterpreterV8::FuncSend(const Arguments& args)
{
    if (args.Length()==0)
        return ThrowException(String::New("Number of arguments must be at least 1."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument 1 must be a string."));

    const String::AsciiValue str(args[0]);

    string command = *str;

    if (command.length()==0)
        return ThrowException(String::New("Server name empty."));

    if (args.Length()==0)
    {
        if (command.find_first_of('/')==string::npos)
            command += "/";
    }

    // Escape all string arguments. All others can be kept as they are.
    for (int i=1; i<args.Length(); i++)
    {
        string arg = *String::AsciiValue(args[i]);

        // Escape string
        if (args[i]->IsString())
        {
            boost::replace_all(arg, "\\", "\\\\");
            boost::replace_all(arg, "'", "\\'");
            boost::replace_all(arg, "\"", "\\\"");
        }

        command += " "+arg;
    }

    try
    {
        return Boolean::New(JsSend(command));
    }
    catch (const runtime_error &e)
    {
        return ThrowException(String::New(e.what()));
    }
}

// ==========================================================================
//                               State control
// ==========================================================================

Handle<Value> InterpreterV8::FuncWait(const Arguments& args)
{
    if (args.Length()!=2 && args.Length()!=3)
        return ThrowException(String::New("Number of arguments must be 2 or 3."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument 1 not a string."));

    if (!args[1]->IsInt32() && !args[1]->IsString())
        return ThrowException(String::New("Argument 2 not an int32 and not a string."));

    if (args.Length()==3 && !args[2]->IsInt32() && !args[2]->IsUndefined())
        return ThrowException(String::New("Argument 3 not an int32 and not undefined."));

    // Using a Javascript function has the advantage that it is fully
    // interruptable without the need of C++ code

    const string index   = args[1]->IsInt32() ? "s.index" : "s.name";
    const bool   timeout = args.Length()==3 && !args[2]->IsUndefined();
    const string arg0    = *String::AsciiValue(args[0]);
    const string state   = args[1]->IsString() ? *String::AsciiValue(args[1]) : "";
    const string arg1    = args[1]->IsString() ? ("\""+state+"\"") : to_string(args[1]->Int32Value());
    const bool   isNot   = arg0[0]=='!';
    const string name    = isNot ? arg0.substr(1) : arg0;

    if (arg0.find_first_of("\"'")!=string::npos)
        return ThrowException(String::New("Server name must not contain quotation marks."));

    if (args[1]->IsString())
        if (state.find_first_of("\"'")!=string::npos)
            return ThrowException(String::New("State name must not contain quotation marks."));

    string code =  "(function(name,state,ms)"
                   "{";
    if (timeout)
        code +=       "var t = new Date();";
    code +=           "var s;"
                      "while (1)"
                      "{"
                         "s = dim.state(name);"
                         "if(!s)throw new Error('Waiting for state "+arg1+" of server "+arg0+" failed.');";
    if (isNot)
        code +=
                         "if(state!="+index+")return true;";
    else
        code +=
                         "if(state=="+index+")return true;";
    if (timeout)
        code +=          "if((new Date()-t)>Math.abs(ms))break;";

    code +=              "v8.sleep();"
                      "}";
    if (timeout)
        code +=       "if(ms>0)throw new Error('Waiting for state "+arg1+" of server "+arg0+" ['+"+index+"+'] timed out.');";
    code +=           "return false;"
                   "})('"+name+"',"+arg1;
    if (timeout)
        code +=    "," + (args[2]->IsUndefined()?"undefined":to_string(args[2]->Int32Value()));
    code +=        ");";

    return ExecuteInternal(code);
}

Handle<Value> InterpreterV8::FuncState(const Arguments& args)
{
    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be exactly 1."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument 1 must be a string."));

    // Return state.name/state.index

    const String::AsciiValue str(args[0]);

    const State rc = JsState(*str);
    if (rc.index<=-256)
        return Undefined();

    HandleScope handle_scope;

    Handle<Object> obj = Object::New();

    obj->Set(String::New("server"), String::New(*str),            ReadOnly);
    obj->Set(String::New("index"),  Integer::New(rc.index),       ReadOnly);
    obj->Set(String::New("name"),   String::New(rc.name.c_str()), ReadOnly);

    const Local<Value> date = Date::New(rc.time.JavaDate());
    if (rc.index>-256 && !date.IsEmpty())
        obj->Set(String::New("time"),  date);

    return handle_scope.Close(obj);
}

Handle<Value> InterpreterV8::FuncNewState(const Arguments& args)
{
    if (args.Length()<1 || args.Length()>3)
        return ThrowException(String::New("Number of arguments must be 1, 2 or 3."));

    if (!args[0]->IsUint32())
        return ThrowException(String::New("Argument 1 must be an uint32."));
    if (args.Length()>1 && !args[1]->IsString())
        return ThrowException(String::New("Argument 2 must be a string."));
    if (args.Length()>2 && !args[2]->IsString())
        return ThrowException(String::New("Argument 3 must be a string."));

    const uint32_t index   = args[0]->Int32Value();
    const string   name    = *String::AsciiValue(args[1]);
    const string   comment = *String::AsciiValue(args[2]);

    if (index<10 || index>255)
        return ThrowException(String::New("State must be in the range [10, 255]."));

    if (name.empty())
        return ThrowException(String::New("State name must not be empty."));

    if (name.find_first_of(':')!=string::npos || name.find_first_of('=')!=string::npos)
        return ThrowException(String::New("State name must not contain : or =."));

    struct Find : State
    {
        Find(int idx, const string &n) : State(idx, n) { }
        bool operator()(const pair<int, string> &p) { return index==p.first || name==p.second; }
    };

    if (find_if(fStates.begin(), fStates.end(), Find(index, name))!=fStates.end())
    {
        const string what =
            "State index ["+to_string(index)+"] or name ["+name+"] already defined.";

        return ThrowException(String::New(what.c_str()));
    }

    return Boolean::New(JsNewState(index, name, comment));
}

Handle<Value> InterpreterV8::FuncSetState(const Arguments& args)
{
    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be exactly 1."));

    if (!args[0]->IsUint32() && !args[0]->IsString())
        return ThrowException(String::New("Argument must be an uint32 or a string."));

    int index = -2;
    if (args[0]->IsUint32())
    {
        index = args[0]->Int32Value();
    }
    else
    {
        const string name = *String::AsciiValue(args[0]);
        index = JsGetState(name);
        if (index==-2)
            return ThrowException(String::New(("State '"+name+"' not found.").c_str()));
    }

    if (index<10 || index>255)
        return ThrowException(String::New("State must be in the range [10, 255]."));

    return Boolean::New(JsSetState(index));
}

Handle<Value> InterpreterV8::FuncGetState(const Arguments& args)
{
    if (args.Length()>0)
        return ThrowException(String::New("getState must not take arguments."));

    const State state = JsGetCurrentState();

    HandleScope handle_scope;

    Handle<Object> rc = Object::New();
    if (rc.IsEmpty())
        return Undefined();

    rc->Set(String::New("index"), Integer::New(state.index), ReadOnly);
    rc->Set(String::New("name"), String::New(state.name.c_str()), ReadOnly);
    rc->Set(String::New("description"), String::New(state.comment.c_str()), ReadOnly);

    return handle_scope.Close(rc);
}

Handle<Value> InterpreterV8::FuncGetStates(const Arguments& args)
{
    if (args.Length()>1)
        return ThrowException(String::New("getStates must not take more than one arguments."));

    if (args.Length()==1 && !args[0]->IsString())
        return ThrowException(String::New("Argument must be a string."));

    const string server = args.Length()==1 ? *String::AsciiValue(args[0]) : "DIM_CONTROL";

    const vector<State> states = JsGetStates(server);

    HandleScope handle_scope;

    Handle<Object> list = Object::New();
    if (list.IsEmpty())
        return Undefined();

    for (auto it=states.begin(); it!=states.end(); it++)
    {
        Handle<Value> entry = StringObject::New(String::New(it->name.c_str()));
        if (entry.IsEmpty())
            return Undefined();

        StringObject::Cast(*entry)->Set(String::New("description"), String::New(it->comment.c_str()), ReadOnly);
        list->Set(Integer::New(it->index), entry, ReadOnly);
    }

    return handle_scope.Close(list);
}

Handle<Value> InterpreterV8::FuncGetDescription(const Arguments& args)
{
    if (args.Length()!=1)
        return ThrowException(String::New("getDescription must take exactly one argument."));

    if (args.Length()==1 && !args[0]->IsString())
        return ThrowException(String::New("Argument must be a string."));

    const string service = *String::AsciiValue(args[0]);

    const vector<Description> descriptions = JsGetDescription(service);
    const set<Service> services = JsGetServices();

    auto is=services.begin();
    for (; is!=services.end(); is++)
        if (is->name==service)
            break;

    if (is==services.end())
        return Undefined();

    HandleScope handle_scope;

    Handle<Object> arr = fTemplateDescription->GetFunction()->NewInstance();//Object::New();
    if (arr.IsEmpty())
        return Undefined();

    auto it=descriptions.begin();
    arr->Set(String::New("name"), String::New(it->name.c_str()), ReadOnly);
    if (!it->comment.empty())
        arr->Set(String::New("description"), String::New(it->comment.c_str()), ReadOnly);
    if (is!=services.end())
    {
        arr->Set(String::New("server"), String::New(is->server.c_str()), ReadOnly);
        arr->Set(String::New("service"), String::New(is->service.c_str()), ReadOnly);
        arr->Set(String::New("isCommand"), Boolean::New(is->iscmd), ReadOnly);
        if (!is->format.empty())
            arr->Set(String::New("format"), String::New(is->format.c_str()), ReadOnly);
    }

    uint32_t i=0;
    for (it++; it!=descriptions.end(); it++)
    {
        Handle<Object> obj = Object::New();
        if (obj.IsEmpty())
            return Undefined();

        if (!it->name.empty())
            obj->Set(String::New("name"), String::New(it->name.c_str()), ReadOnly);
        if (!it->comment.empty())
            obj->Set(String::New("description"), String::New(it->comment.c_str()), ReadOnly);
        if (!it->unit.empty())
            obj->Set(String::New("unit"), String::New(it->unit.c_str()), ReadOnly);

        arr->Set(i++, obj);
    }

    return handle_scope.Close(arr);
}

Handle<Value> InterpreterV8::FuncGetServices(const Arguments& args)
{
    if (args.Length()>2)
        return ThrowException(String::New("getServices must not take more than two argument."));

    if (args.Length()>=1 && !args[0]->IsString())
        return ThrowException(String::New("First argument must be a string."));

    if (args.Length()==2 && !args[1]->IsBoolean())
        return ThrowException(String::New("Second argument must be a boolean."));

    string arg0 = args.Length() ? *String::AsciiValue(args[0]) : "";
    if (arg0=="*")
        arg0="";

    const set<Service> services = JsGetServices();

    HandleScope handle_scope;

    Handle<Array> arr = Array::New();
    if (arr.IsEmpty())
        return Undefined();

    uint32_t i=0;
    for (auto is=services.begin(); is!=services.end(); is++)
    {
        if (!arg0.empty() && is->name.find(arg0)!=0)
            continue;

        if (args.Length()==2 && args[1]->BooleanValue()!=is->iscmd)
            continue;

        Handle<Object> obj = Object::New();
        if (obj.IsEmpty())
            return Undefined();

        obj->Set(String::New("name"), String::New(is->name.c_str()), ReadOnly);
        obj->Set(String::New("server"), String::New(is->server.c_str()), ReadOnly);
        obj->Set(String::New("service"), String::New(is->service.c_str()), ReadOnly);
        obj->Set(String::New("isCommand"), Boolean::New(is->iscmd), ReadOnly);
        if (!is->format.empty())
            obj->Set(String::New("format"), String::New(is->format.c_str()), ReadOnly);

        arr->Set(i++, obj);
    }

    return handle_scope.Close(arr);
}

// ==========================================================================
//                             Internal functions
// ==========================================================================


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
Handle<Value> InterpreterV8::FuncLog(const Arguments& args)
{
    for (int i=0; i<args.Length(); i++)
    {
        const String::AsciiValue str(args[i]);
        if (*str)
            JsPrint(*str);
    }

    if (args.Length()==0)
        JsPrint();

    return Undefined();
}

Handle<Value> InterpreterV8::FuncAlarm(const Arguments& args)
{
    for (int i=0; i<args.Length(); i++)
    {
        const String::AsciiValue str(args[i]);
        if (*str)
            JsAlarm(*str);
    }

    if (args.Length()==0)
        JsAlarm();

    return Undefined();
}

Handle<Value> InterpreterV8::FuncOut(const Arguments& args)
{
    for (int i=0; i<args.Length(); i++)
    {
        const String::AsciiValue str(args[i]);
        if (*str)
            JsOut(*str);
    }
    return Undefined();
}

Handle<Value> InterpreterV8::FuncWarn(const Arguments& args)
{
    for (int i=0; i<args.Length(); i++)
    {
        const String::AsciiValue str(args[i]);
        if (*str)
            JsWarn(*str);
    }
    return Undefined();
}

// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
Handle<Value> InterpreterV8::FuncInclude(const Arguments& args)
{
    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be one."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument must be a string."));

    const String::AsciiValue file(args[0]);
    if (*file == NULL)
        return ThrowException(String::New("File name missing."));

    if (strlen(*file)==0)
        return ThrowException(String::New("File name empty."));

    const auto path = boost::filesystem::path(*file);

    const auto f = path.is_absolute() ? path : boost::filesystem::path(fIncludePath)/path;

    izstream fin(*file);//f.string().c_str());
    if (!fin)
        return ThrowException(String::New(errno!=0?strerror(errno):"Insufficient memory for decompression"));

    string buffer;
    getline(fin, buffer, '\0');

    if ((fin.fail() && !fin.eof()) || fin.bad())
        return ThrowException(String::New(strerror(errno)));

    if (buffer.length()>1 && buffer[0]=='#' && buffer[1]=='!')
        buffer.insert(0, "//");

    return ExecuteCode(buffer, *file);
}

Handle<Value> InterpreterV8::FuncFile(const Arguments& args)
{
    if (args.Length()!=1 && args.Length()!=2)
        return ThrowException(String::New("Number of arguments must be one or two."));

    const String::AsciiValue file(args[0]);
    if (*file == NULL)
        return ThrowException(String::New("File name missing"));

    if (args.Length()==2 && !args[1]->IsString())
        return ThrowException(String::New("Second argument must be a string."));

    const string delim = args.Length()==2 ? *String::AsciiValue(args[1]) : "";

    if (args.Length()==2 && delim.size()!=1)
        return ThrowException(String::New("Second argument must be a string of length 1."));

    HandleScope handle_scope;

    const auto path = boost::filesystem::path(*file);

    const auto f = path.is_absolute() ? path : boost::filesystem::path(fIncludePath)/path;

    izstream fin(*file);//f.string().c_str());
    if (!fin)
        return ThrowException(String::New(errno!=0?strerror(errno):"Insufficient memory for decompression"));

    if (args.Length()==1)
    {
        string buffer;
        getline(fin, buffer, '\0');
        if ((fin.fail() && !fin.eof()) || fin.bad())
            return ThrowException(String::New(strerror(errno)));

        Handle<Value> str = StringObject::New(String::New(buffer.c_str()));
        StringObject::Cast(*str)->Set(String::New("name"), String::New(*file));
        return handle_scope.Close(str);
    }

    Handle<Array> arr = Array::New();
    if (arr.IsEmpty())
        return Undefined();

    int i=0;
    string buffer;
    while (getline(fin, buffer, delim[0]))
        arr->Set(i++, String::New(buffer.c_str()));

    if ((fin.fail() && !fin.eof()) || fin.bad())
        return ThrowException(String::New(strerror(errno)));

    arr->Set(String::New("name"),  String::New(*file));
    arr->Set(String::New("delim"), String::New(delim.c_str(), 1));

    return handle_scope.Close(arr);
}

// ==========================================================================
//                                 Mail
// ==========================================================================

Handle<Value> InterpreterV8::ConstructorMail(const Arguments &args)
{
    if (!args.IsConstructCall())
        return ThrowException(String::New("Mail must be called as constructor"));

    if (args.Length()!=1 || !args[0]->IsString())
        return ThrowException(String::New("Constructor must be called with a single string as argument"));

    HandleScope handle_scope;

    Handle<Array> rec = Array::New();
    Handle<Array> att = Array::New();
    Handle<Array> bcc = Array::New();
    Handle<Array> cc  = Array::New();
    Handle<Array> txt = Array::New();
    if (rec.IsEmpty() || att.IsEmpty() || bcc.IsEmpty() || cc.IsEmpty() || txt.IsEmpty())
        return Undefined();

    Handle<Object> self = args.This();

    self->Set(String::New("subject"),     args[0]->ToString(), ReadOnly);
    self->Set(String::New("recipients"),  rec, ReadOnly);
    self->Set(String::New("attachments"), att, ReadOnly);
    self->Set(String::New("bcc"),         bcc, ReadOnly);
    self->Set(String::New("cc"),          cc,  ReadOnly);
    self->Set(String::New("text"),        txt, ReadOnly);

    self->Set(String::New("send"), FunctionTemplate::New(WrapSendMail)->GetFunction(), ReadOnly);

    return handle_scope.Close(self);
}

vector<string> InterpreterV8::ValueToArray(const Handle<Value> &val, bool only)
{
    vector<string> rc;

    Handle<Array> arr = Handle<Array>::Cast(val);
    for (uint32_t i=0; i<arr->Length(); i++)
    {
        Handle<Value> obj = arr->Get(i);
        if (obj.IsEmpty())
            continue;

        if (obj->IsNull() || obj->IsUndefined())
            continue;

        if (only && !obj->IsString())
            continue;

        rc.push_back(*String::AsciiValue(obj->ToString()));
    }

    return rc;
}

Handle<Value> InterpreterV8::FuncSendMail(const Arguments& args)
{
    HandleScope handle_scope;

    if (args.Length()>1)
        return ThrowException(String::New("Only one argument allowed."));

    if (args.Length()==1 && !args[0]->IsBoolean())
        return ThrowException(String::New("Argument must be a boolean."));

    const bool block = args.Length()==0 || args[0]->BooleanValue();

    const Handle<Value> sub = args.This()->Get(String::New("subject"));
    const Handle<Value> rec = args.This()->Get(String::New("recipients"));
    const Handle<Value> txt = args.This()->Get(String::New("text"));
    const Handle<Value> att = args.This()->Get(String::New("attachments"));
    const Handle<Value> bcc = args.This()->Get(String::New("bcc"));
    const Handle<Value> cc  = args.This()->Get(String::New("cc"));

    const vector<string> vrec = ValueToArray(rec);
    const vector<string> vtxt = ValueToArray(txt, false);
    const vector<string> vatt = ValueToArray(att);
    const vector<string> vbcc = ValueToArray(bcc);
    const vector<string> vcc  = ValueToArray(cc);

    if (vrec.size()==0)
        return ThrowException(String::New("At least one valid string is required in 'recipients'."));
    if (vtxt.size()==0)
        return ThrowException(String::New("At least one valid string is required in 'text'."));

    const string subject = *String::AsciiValue(sub->ToString());

    FILE *pipe = popen(("from=no-reply@fact-project.org mailx -~ "+vrec[0]).c_str(), "w");
    if (!pipe)
        return ThrowException(String::New(strerror(errno)));

    fprintf(pipe, "%s", ("~s"+subject+"\n").c_str());
    for (auto it=vrec.begin()+1; it<vrec.end(); it++)
        fprintf(pipe, "%s", ("~t"+*it+"\n").c_str());
    for (auto it=vbcc.begin(); it<vbcc.end(); it++)
        fprintf(pipe, "%s", ("~b"+*it+"\n").c_str());
    for (auto it=vcc.begin(); it<vcc.end(); it++)
        fprintf(pipe, "%s", ("~c"+*it+"\n").c_str());
    for (auto it=vatt.begin(); it<vatt.end(); it++)
        fprintf(pipe, "%s", ("~@"+*it+"\n").c_str());  // Must not contain white spaces

    for (auto it=vtxt.begin(); it<vtxt.end(); it++)
        fwrite((*it+"\n").c_str(), it->length()+1, 1, pipe);

    fprintf(pipe, "\n---\nsent by dimctrl");

    if (!block)
        return Undefined();

    const int rc = pclose(pipe);

    const Locker lock;
    return handle_scope.Close(Integer::New(WEXITSTATUS(rc)));
}

// ==========================================================================
//                                 Curl
// ==========================================================================

Handle<Value> InterpreterV8::ConstructorCurl(const Arguments &args)
{
    if (!args.IsConstructCall())
        return ThrowException(String::New("Curl must be called as constructor"));

    if (args.Length()!=1 || !args[0]->IsString())
        return ThrowException(String::New("Constructor must be called with a single string as argument"));

    HandleScope handle_scope;

    Handle<Array> data = Array::New();
    if (data.IsEmpty())
        return Undefined();

    Handle<Object> self = args.This();

    self->Set(String::New("url"),  args[0]->ToString(), ReadOnly);
    self->Set(String::New("data"), data, ReadOnly);

    self->Set(String::New("send"), FunctionTemplate::New(WrapSendCurl)->GetFunction(), ReadOnly);

    return handle_scope.Close(self);
}

Handle<Value> InterpreterV8::FuncSendCurl(const Arguments& args)
{
    HandleScope handle_scope;

    if (args.Length()>1)
        return ThrowException(String::New("Only one argument allowed."));

    if (args.Length()==1 && !args[0]->IsBoolean())
        return ThrowException(String::New("Argument must be a boolean."));

    const bool block = args.Length()==0 || args[0]->BooleanValue();

    const Handle<Value> url  = args.This()->Get(String::New("url"));
    const Handle<Value> data = args.This()->Get(String::New("data"));

    const vector<string> vdata = ValueToArray(data);
    const string sdata = boost::algorithm::join(vdata, "&");

    const string surl = *String::AsciiValue(url->ToString());

    string cmd = "curl -sSf ";
    if (!sdata.empty())
        cmd += "--data '"+sdata+"' ";
    cmd += "'http://"+surl+"' 2>&1 ";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return ThrowException(String::New(strerror(errno)));

    if (!block)
        return Undefined();

    string txt;

    while (!feof(pipe))
    {
        char buf[1025];
        if (fgets(buf, 1024, pipe)==NULL)
            break;
        txt += buf;
    }

    const int rc = pclose(pipe);

    Handle<Object> obj = Object::New();

    obj->Set(String::New("cmd"), String::New(cmd.c_str()));
    obj->Set(String::New("data"), String::New(txt.c_str()));
    obj->Set(String::New("rc"), Integer::NewFromUnsigned(WEXITSTATUS(rc)));

    const Locker lock;
    return handle_scope.Close(obj);
}

// ==========================================================================
//                                 Database
// ==========================================================================

Handle<Value> InterpreterV8::FuncDbClose(const Arguments &args)
{
    void *ptr = External::Unwrap(args.This()->GetInternalField(0));
    if (!ptr)
        return Boolean::New(false);

#ifdef HAVE_SQL
    Database *db = reinterpret_cast<Database*>(ptr);
    auto it = find(fDatabases.begin(), fDatabases.end(), db);
    fDatabases.erase(it);
    delete db;
#endif

    HandleScope handle_scope;

    args.This()->SetInternalField(0, External::New(0));

    return handle_scope.Close(Boolean::New(true));
}

Handle<Value> InterpreterV8::FuncDbQuery(const Arguments &args)
{
    if (args.Length()==0)
        return ThrowException(String::New("Arguments expected."));

    void *ptr = External::Unwrap(args.This()->GetInternalField(0));
    if (!ptr)
        return Undefined();

    string query;
    for (int i=0; i<args.Length(); i++)
        query += string(" ") + *String::AsciiValue(args[i]);
    query.erase(0, 1);

#ifdef HAVE_SQL
    try
    {
        HandleScope handle_scope;

        Database *db = reinterpret_cast<Database*>(ptr);

        const mysqlpp::StoreQueryResult res = db->query(query).store();

        Handle<Array> ret = Array::New();
        if (ret.IsEmpty())
            return Undefined();

        ret->Set(String::New("table"), String::New(res.table()),   ReadOnly);
        ret->Set(String::New("query"), String::New(query.c_str()), ReadOnly);

        Handle<Array> cols = Array::New();
        if (cols.IsEmpty())
            return Undefined();

        int irow=0;
        for (vector<mysqlpp::Row>::const_iterator it=res.begin(); it<res.end(); it++)
        {
            Handle<Object> row = Object::New();
            if (row.IsEmpty())
                return Undefined();

            const mysqlpp::FieldNames *list = it->field_list().list;

            for (size_t i=0; i<it->size(); i++)
            {
                const Handle<Value> name = String::New((*list)[i].c_str());
                if (irow==0)
                    cols->Set(i, name);

                if ((*it)[i].is_null())
                {
                    row->Set(name, Undefined(), ReadOnly);
                    continue;
                }

                const string sql_type = (*it)[i].type().sql_name();

                const bool uns = sql_type.find("UNSIGNED")==string::npos;

                if (sql_type.find("BIGINT")!=string::npos)
                {
                    if (uns)
                    {
                        const uint64_t val = (uint64_t)(*it)[i];
                        if (val>UINT32_MAX)
                            row->Set(name, Number::New(val), ReadOnly);
                        else
                            row->Set(name, Integer::NewFromUnsigned(val), ReadOnly);
                    }
                    else
                    {
                        const int64_t val = (int64_t)(*it)[i];
                        if (val<INT32_MIN || val>INT32_MAX)
                            row->Set(name, Number::New(val), ReadOnly);
                        else
                            row->Set(name, Integer::NewFromUnsigned(val), ReadOnly);
                    }
                    continue;
                }

                // 32 bit
                if (sql_type.find("INT")!=string::npos)
                {
                    if (uns)
                        row->Set(name, Integer::NewFromUnsigned((uint32_t)(*it)[i]), ReadOnly);
                    else
                        row->Set(name, Integer::New((int32_t)(*it)[i]), ReadOnly);
                    continue;
                }

                if (sql_type.find("BOOL")!=string::npos )
                {
                    row->Set(name, Boolean::New((bool)(*it)[i]), ReadOnly);
                    continue;
                }

                if (sql_type.find("FLOAT")!=string::npos)
                {
                    ostringstream val;
                    val << setprecision(7) << (float)(*it)[i];
                    row->Set(name, Number::New(stod(val.str())), ReadOnly);
                    continue;

                }
                if (sql_type.find("DOUBLE")!=string::npos)
                {
                    row->Set(name, Number::New((double)(*it)[i]), ReadOnly);
                    continue;
                }

                if (sql_type.find("CHAR")!=string::npos ||
                    sql_type.find("TEXT")!=string::npos)
                {
                    row->Set(name, String::New((const char*)(*it)[i]), ReadOnly);
                    continue;
                }

                time_t date = 0;
                if (sql_type.find("TIMESTAMP")!=string::npos)
                    date = mysqlpp::Time((*it)[i]);

                if (sql_type.find("DATETIME")!=string::npos)
                    date = mysqlpp::DateTime((*it)[i]);

                if (sql_type.find(" DATE ")!=string::npos)
                    date = mysqlpp::Date((*it)[i]);

                if (date>0)
                {
                    // It is important to catch the exception thrown
                    // by Date::New in case of thread termination!
                    const Local<Value> val = Date::New(date*1000);
                    if (val.IsEmpty())
                        return Undefined();

                    row->Set(name, val, ReadOnly);
                }
            }

            ret->Set(irow++, row);
        }

        if (irow>0)
            ret->Set(String::New("cols"), cols, ReadOnly);

        return handle_scope.Close(ret);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }
#else
    return Undefined();
#endif
}

Handle<Value> InterpreterV8::FuncDatabase(const Arguments &args)
{
    if (!args.IsConstructCall())
        return ThrowException(String::New("Database must be called as constructor."));

    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be 1."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument 1 not a string."));

#ifdef HAVE_SQL
    try
    {
        HandleScope handle_scope;

        //if (!args.IsConstructCall())
        //    return Constructor(fTemplateDatabase, args);

        Database *db = new Database(*String::AsciiValue(args[0]));
        fDatabases.push_back(db);

        Handle<Object> self = args.This();
        self->Set(String::New("user"),     String::New(db->user.c_str()), ReadOnly);
        self->Set(String::New("server"),   String::New(db->server.c_str()), ReadOnly);
        self->Set(String::New("database"), String::New(db->db.c_str()), ReadOnly);
        self->Set(String::New("port"),     db->port==0?Undefined():Integer::NewFromUnsigned(db->port), ReadOnly);
        self->Set(String::New("query"),    FunctionTemplate::New(WrapDbQuery)->GetFunction(), ReadOnly);
        self->Set(String::New("close"),    FunctionTemplate::New(WrapDbClose)->GetFunction(),   ReadOnly);
        self->SetInternalField(0, External::New(db));

        return handle_scope.Close(self);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }
#else
    return Undefined();
#endif
}

// ==========================================================================
//                                 Services
// ==========================================================================

Handle<Value> InterpreterV8::Convert(char type, const char* &ptr)
{
    // Dim values are always unsigned per (FACT++) definition
    switch (type)
    {
    case 'F':
        {
            // Remove the "imprecision" effect coming from casting a float to
            // a double and then showing it with double precision
            ostringstream val;
            val << setprecision(7) << *reinterpret_cast<const float*>(ptr);
            ptr += 4;
            return Number::New(stod(val.str()));
        }
    case 'D':  { Handle<Value> v=Number::New(*reinterpret_cast<const double*>(ptr)); ptr+=8; return v; }
    case 'I':
    case 'L':  { Handle<Value> v=Integer::NewFromUnsigned(*reinterpret_cast<const uint32_t*>(ptr)); ptr += 4; return v; }
    case 'X':
        {
            const int64_t val = *reinterpret_cast<const int64_t*>(ptr);
            ptr += 8;
            if (val>=0 && val<=UINT32_MAX)
                return Integer::NewFromUnsigned(val);
            if (val>=INT32_MIN && val<0)
                return Integer::New(val);
            return Number::New(val);
        }
    case 'S':  { Handle<Value> v=Integer::NewFromUnsigned(*reinterpret_cast<const uint16_t*>(ptr)); ptr += 2; return v; }
    case 'C':  { Handle<Value> v=Integer::NewFromUnsigned((uint16_t)*reinterpret_cast<const uint8_t*>(ptr));  ptr += 1; return v; }
    }
    return Undefined();
}

Handle<Value> InterpreterV8::FuncClose(const Arguments &args)
{
    HandleScope handle_scope;

    //const void *ptr = Local<External>::Cast(args.Holder()->GetInternalField(0))->Value();

    const String::AsciiValue str(args.This()->Get(String::New("name")));

    const auto it = fReverseMap.find(*str);
    if (it!=fReverseMap.end())
    {
        it->second.Dispose();
        fReverseMap.erase(it);
    }

    args.This()->Set(String::New("isOpen"), Boolean::New(false), ReadOnly);

    return handle_scope.Close(Boolean::New(JsUnsubscribe(*str)));
}

Handle<Value> InterpreterV8::ConvertEvent(const EventImp *evt, uint64_t counter, const char *str)
{
    const vector<Description> vec = JsDescription(str);

    Handle<Object> ret = fTemplateEvent->GetFunction()->NewInstance();//Object::New();
    if (ret.IsEmpty())
        return Undefined();

    const Local<Value> date = Date::New(evt->GetJavaDate());
    if (date.IsEmpty())
        return Undefined();

    ret->Set(String::New("name"),    String::New(str),              ReadOnly);
    ret->Set(String::New("format"),  String::New(evt->GetFormat().c_str()), ReadOnly);
    ret->Set(String::New("qos"),     Integer::New(evt->GetQoS()),   ReadOnly);
    ret->Set(String::New("size"),    Integer::New(evt->GetSize()),  ReadOnly);
    ret->Set(String::New("counter"), Integer::New(counter),         ReadOnly);
    if (evt->GetJavaDate()>0)
        ret->Set(String::New("time"), date, ReadOnly);

    // If names are available data will also be provided as an
    // object. If an empty event was received, but names are available,
    // the object will be empty. Otherwise 'obj' will be undefined.
    // obj===undefined:               no data received
    // obj!==undefined, length==0:    names for event available
    // obj!==undefined, obj.length>0: names available, data received
    Handle<Object> named = Object::New();
    if (vec.size()>0)
        ret->Set(String::New("obj"), named, ReadOnly);

    // If no event was received (usually a disconnection event in
    // the context of FACT++), no data is returned
    if (evt->IsEmpty())
        return ret;

    // If valid data was received, but the size was zero, then
    // null is returned as data
    // data===undefined: no data received
    // data===null:      event received, but no data
    // data.length>0:    event received, contains data
    if (evt->GetSize()==0 || evt->GetFormat().empty())
    {
        ret->Set(String::New("data"), Null(), ReadOnly);
        return ret;
    }

    // It seems a copy is required either in the boost which comes with
    // Ubuntu 16.04 or in gcc5 ?!
    const string fmt = evt->GetFormat();

    typedef boost::char_separator<char> separator;
    const boost::tokenizer<separator> tokenizer(fmt, separator(";:"));

    const vector<string> tok(tokenizer.begin(), tokenizer.end());

    Handle<Object> arr = tok.size()>1 ? Array::New() : ret;
    if (arr.IsEmpty())
        return Undefined();

    const char *ptr = evt->GetText();
    const char *end = evt->GetText()+evt->GetSize();

    try
    {
        size_t pos = 1;
        for (auto it=tok.begin(); it<tok.end() && ptr<end; it++, pos++)
        {
            char type = (*it)[0];
            it++;

            string name = pos<vec.size() ? vec[pos].name : "";
            if (tok.size()==1)
                name = "data";

            // Get element size
            uint32_t sz = 1;
            switch (type)
            {
            case 'X':
            case 'D': sz = 8; break;
            case 'F':
            case 'I':
            case 'L': sz = 4; break;
            case 'S': sz = 2; break;
            case 'C': sz = 1; break;
            }

            // Check if no number is attached if the size of the
            // received data is consistent with the format string
            if (it==tok.end() && (end-ptr)%sz>0)
                return Exception::Error(String::New(("Number of received bytes ["+to_string(evt->GetSize())+"] does not match format ["+evt->GetFormat()+"]").c_str()));

            // Check if format has a number attached.
            // If no number is attached calculate number of elements
            const uint32_t cnt = it==tok.end() ? (end-ptr)/sz : stoi(it->c_str());

            // is_str: Array of type C but unknown size (String)
            // is_one: Array of known size, but size is 1 (I:1)
            const bool is_str = type=='C' && it==tok.end();
            const bool is_one = cnt==1    && it!=tok.end();

            Handle<Value> v;

            if (is_str)
                v = String::New(ptr);
            if (is_one)
                v = Convert(type, ptr);

            // Array of known (I:5) or unknown size (I), but no string
            if (!is_str && !is_one)
            {
                Handle<Object> a = Array::New(cnt);
                if (a.IsEmpty())
                    return Undefined();

                for (uint32_t i=0; i<cnt; i++)
                    a->Set(i, Convert(type, ptr));

                v = a;
            }

            if (tok.size()>1)
                arr->Set(pos-1, v);
            else
                ret->Set(String::New("data"), v, ReadOnly);

            if (!name.empty())
            {
                const Handle<String> n = String::New(name.c_str());
                named->Set(n, v);
            }
        }

        if (tok.size()>1)
            ret->Set(String::New("data"), arr, ReadOnly);

        return ret;
    }
    catch (...)
    {
        return Exception::Error(String::New(("Format string conversion '"+evt->GetFormat()+"' failed.").c_str()));
    }
}
/*
Handle<Value> InterpreterV8::FuncGetData(const Arguments &args)
{
    HandleScope handle_scope;

    const String::AsciiValue str(args.Holder()->Get(String::New("name")));

    const pair<uint64_t, EventImp *> p = JsGetEvent(*str);

    const EventImp *evt = p.second;
    if (!evt)
        return Undefined();

    //if (counter==cnt)
    //    return info.Holder();//Holder()->Get(String::New("data"));

    Handle<Value> ret = ConvertEvent(evt, p.first, *str);
    return ret->IsNativeError() ? ThrowException(ret) : handle_scope.Close(ret);
}
*/
Handle<Value> InterpreterV8::FuncGetData(const Arguments &args)
{
    if (args.Length()>2)
        return ThrowException(String::New("Number of arguments must not be greater than 2."));

    if (args.Length()>=1 && !args[0]->IsInt32() && !args[0]->IsNull())
        return ThrowException(String::New("Argument 1 not an uint32."));

    if (args.Length()==2 && !args[1]->IsBoolean())
        return ThrowException(String::New("Argument 2 not a boolean."));

    // Using a Javascript function has the advantage that it is fully
    // interruptable without the need of C++ code
    const bool    null    = args.Length()>=1 && args[0]->IsNull();
    const int32_t timeout = args.Length()>=1 ? args[0]->Int32Value() : 0;
    const bool    named   = args.Length()<2 || args[1]->BooleanValue();

    HandleScope handle_scope;

    const Handle<String> data   = String::New("data");
    const Handle<String> object = String::New("obj");

    const String::AsciiValue name(args.Holder()->Get(String::New("name")));

    TryCatch exception;

    Time t;
    while (!exception.HasCaught())
    {
        const pair<uint64_t, EventImp *> p = JsGetEvent(*name);

        const EventImp *evt = p.second;
        if (evt)
        {
            const Handle<Value> val = ConvertEvent(evt, p.first, *name);
            if (val->IsNativeError())
                return ThrowException(val);

            // Protect against the return of an exception
            if (val->IsObject())
            {
                const Handle<Object> event = val->ToObject();
                const Handle<Value>  obj   = event->Get(named?object:data);
                if (!obj.IsEmpty())
                {
                    if (!named)
                    {
                        // No names (no 'obj'), but 'data'
                        if (!obj->IsUndefined())
                            return handle_scope.Close(val);
                    }
                    else
                    {
                        // Has names and data was received?
                        if (obj->IsObject() && obj->ToObject()->GetOwnPropertyNames()->Length()>0)
                            return handle_scope.Close(val);
                    }
                }
            }
        }

        if (args.Length()==0)
            break;

        if (!null && Time()-t>=boost::posix_time::milliseconds(abs(timeout)))
            break;

        // Theoretically, the CPU usage can be reduced by maybe a factor
        // of four using a larger value, but this also means that the
        // JavaScript is locked for a longer time.
        const Unlocker unlock;
        usleep(1000);
    }

    // This hides the location of the exception, which is wanted.
    if (exception.HasCaught())
        return exception.ReThrow();

    if (timeout<0)
        return Undefined();

    const string str = "Waiting for a valid event of "+string(*name)+" timed out.";
    return ThrowException(String::New(str.c_str()));
}


// This is a callback from the RemoteControl piping event handling
// to the java script ---> in test phase!
void InterpreterV8::JsHandleEvent(const EventImp &evt, uint64_t cnt, const string &service)
{
    // FIXME: This blocks service updates, we have to run this
    //        in a dedicated thread.
    const Locker locker;

    if (fThreadId<0)
        return;

    const auto it = fReverseMap.find(service);
    if (it==fReverseMap.end())
        return;

    const HandleScope handle_scope;

    Handle<Object> obj = it->second;

    const Handle<String> onchange = String::New("onchange");
    if (!obj->Has(onchange))
        return;

    const Handle<Value> val = obj->Get(onchange);
    if (!val->IsFunction())
        return;

    obj->CreationContext()->Enter();

    // -------------------------------------------------------------------

    TryCatch exception;

    const int id = V8::GetCurrentThreadId();
    fThreadIds.insert(id);

    Handle<Value> ret = ConvertEvent(&evt, cnt, service.c_str());
    if (ret->IsObject())
        Handle<Function>::Cast(val)->Call(obj, 1, &ret);

    fThreadIds.erase(id);

    if (!HandleException(exception, "Service.onchange"))
        V8::TerminateExecution(fThreadId);

    if (ret->IsNativeError())
    {
        JsException(service+".onchange callback - "+*String::AsciiValue(ret));
        V8::TerminateExecution(fThreadId);
    }

    obj->CreationContext()->Exit();
}

Handle<Value> InterpreterV8::OnChangeSet(Local<String> prop, Local<Value> value, const AccessorInfo &)
{
    // Returns the value if the setter intercepts the request. Otherwise, returns an empty handle.
    const string server = *String::AsciiValue(prop);
    auto it = fStateCallbacks.find(server);

    if (it!=fStateCallbacks.end())
    {
        it->second.Dispose();
        fStateCallbacks.erase(it);
    }

    if (value->IsFunction())
        fStateCallbacks[server] = Persistent<Object>::New(value->ToObject());

    return Handle<Value>();
}

void InterpreterV8::JsHandleState(const std::string &server, const State &state)
{
    // FIXME: This blocks service updates, we have to run this
    //        in a dedicated thread.
    const Locker locker;

    if (fThreadId<0)
        return;

    auto it = fStateCallbacks.find(server);
    if (it==fStateCallbacks.end())
    {
        it = fStateCallbacks.find("*");
        if (it==fStateCallbacks.end())
            return;
    }

    const HandleScope handle_scope;

    it->second->CreationContext()->Enter();

    // -------------------------------------------------------------------

    Handle<ObjectTemplate> obj = ObjectTemplate::New();
    obj->Set(String::New("server"),  String::New(server.c_str()), ReadOnly);

    if (state.index>-256)
    {
        obj->Set(String::New("index"),   Integer::New(state.index),          ReadOnly);
        obj->Set(String::New("name"),    String::New(state.name.c_str()),    ReadOnly);
        obj->Set(String::New("comment"), String::New(state.comment.c_str()), ReadOnly);
        const Local<Value> date = Date::New(state.time.JavaDate());
        if (!date.IsEmpty())
            obj->Set(String::New("time"), date);
    }

    // -------------------------------------------------------------------

    TryCatch exception;

    const int id = V8::GetCurrentThreadId();
    fThreadIds.insert(id);

    Handle<Value> args[] = { obj->NewInstance() };
    Handle<Function> fun = Handle<Function>(Function::Cast(*it->second));
    fun->Call(fun, 1, args);

    fThreadIds.erase(id);

    if (!HandleException(exception, "dim.onchange"))
        V8::TerminateExecution(fThreadId);

    it->second->CreationContext()->Exit();
}

// ==========================================================================
//                           Interrupt handling
// ==========================================================================

Handle<Value> InterpreterV8::FuncSetInterrupt(const Arguments &args)
{
    if (args.Length()!=1)
        return ThrowException(String::New("Number of arguments must be 1."));

    if (!args[0]->IsNull() && !args[0]->IsUndefined() && !args[0]->IsFunction())
        return ThrowException(String::New("Argument not a function, null or undefined."));

    if (args[0]->IsNull() || args[0]->IsUndefined())
    {
        fInterruptCallback.Dispose();
        fInterruptCallback.Clear();
        return Undefined();
    }

    // Returns the value if the setter intercepts the request. Otherwise, returns an empty handle.
    fInterruptCallback = Persistent<Object>::New(args[0]->ToObject());
    return Undefined();
}

Handle<Value> InterpreterV8::HandleInterruptImp(string str, uint64_t time)
{
    if (fInterruptCallback.IsEmpty())
        return Handle<Value>();

    const size_t p = str.find_last_of('\n');

    const string usr = p==string::npos?"":str.substr(p+1);

    string irq = p==string::npos?str:str.substr(0, p);

    map<string,string> data;
    try
    {
        data = Tools::Split(irq, true);
    }
    catch (const exception &e)
    {
        irq = "ERROR";
        data["0"] = e.what();
        JsWarn("Couldn't parse interrupt: "+irq+" ["+string(e.what())+"]");
    }

    Local<Value>  irq_str = String::New(irq.c_str());
    Local<Value>  usr_str = String::New(usr.c_str());
    Local<Value>  date    = Date::New(time);
    Handle<Object> arr    = Array::New(data.size());

    if (date.IsEmpty() || arr.IsEmpty())
        return Handle<Value>();

    for (auto it=data.begin(); it!=data.end(); it++)
        arr->Set(String::New(it->first.c_str()), String::New(it->second.c_str()));

    Handle<Value> args[] = { irq_str, arr, date, usr_str };
    Handle<Function> fun = Handle<Function>(Function::Cast(*fInterruptCallback));

    return fun->Call(fun, 4, args);
}

int InterpreterV8::JsHandleInterrupt(const EventImp &evt)
{
    // FIXME: This blocks service updates, we have to run this
    //        in a dedicated thread.
    const Locker locker;

    if (fThreadId<0 || fInterruptCallback.IsEmpty())
        return -42;

    const HandleScope handle_scope;

    fInterruptCallback->CreationContext()->Enter();

    // -------------------------------------------------------------------

    TryCatch exception;

    const int id = V8::GetCurrentThreadId();
    fThreadIds.insert(id);

    const Handle<Value> val = HandleInterruptImp(evt.GetString(), evt.GetJavaDate());

    fThreadIds.erase(id);

    const int rc = !val.IsEmpty() && val->IsInt32() ? val->Int32Value() : 0;

    if (!HandleException(exception, "interrupt"))
        V8::TerminateExecution(fThreadId);

    fInterruptCallback->CreationContext()->Exit();

    return rc<10 || rc>255 ? -42 : rc;
}

Handle<Value> InterpreterV8::FuncTriggerInterrupt(const Arguments &args)
{
    string data;
    for (int i=0; i<args.Length(); i++)
    {
        const String::AsciiValue str(args[i]);

        if (string(*str).find_first_of('\n')!=string::npos)
            return ThrowException(String::New("No argument must contain line breaks."));

        if (!*str)
            continue;

        data += *str;
        data += ' ';
    }

    HandleScope handle_scope;

    const Handle<Value> rc = HandleInterruptImp(Tools::Trim(data), Time().JavaDate());
    return handle_scope.Close(rc);
}

// ==========================================================================
//                           Class 'Subscription'
// ==========================================================================

Handle<Value> InterpreterV8::FuncSubscription(const Arguments &args)
{
    if (args.Length()!=1 && args.Length()!=2)
        return ThrowException(String::New("Number of arguments must be one or two."));

    if (!args[0]->IsString())
        return ThrowException(String::New("Argument 1 must be a string."));

    if (args.Length()==2 && !args[1]->IsFunction())
        return ThrowException(String::New("Argument 2 must be a function."));

    const String::AsciiValue str(args[0]);

    if (!args.IsConstructCall())
    {
        const auto it = fReverseMap.find(*str);
        if (it!=fReverseMap.end())
            return it->second;

        return Undefined();
    }

    const HandleScope handle_scope;

    Handle<Object> self = args.This();
    self->Set(String::New("get"),    FunctionTemplate::New(WrapGetData)->GetFunction(),  ReadOnly);
    self->Set(String::New("close"),  FunctionTemplate::New(WrapClose)->GetFunction(),    ReadOnly);
    self->Set(String::New("name"),   String::New(*str), ReadOnly);
    self->Set(String::New("isOpen"), Boolean::New(true));

    if (args.Length()==2)
        self->Set(String::New("onchange"), args[1]);

    fReverseMap[*str] = Persistent<Object>::New(self);

    void *ptr = JsSubscribe(*str);
    if (ptr==0)
        return ThrowException(String::New(("Subscription to '"+string(*str)+"' already exists.").c_str()));

    self->SetInternalField(0, External::New(ptr));

    return Undefined();

    // Persistent<Object> p = Persistent<Object>::New(obj->NewInstance());
    // obj.MakeWeak((void*)1, Cleanup);
    // return obj;
}

// ==========================================================================
//                            Astrometry
// ==========================================================================
#ifdef HAVE_NOVA

double InterpreterV8::GetDataMember(const Arguments &args, const char *name)
{
    return args.This()->Get(String::New(name))->NumberValue();
}

Handle<Value> InterpreterV8::CalcDist(const Arguments &args, const bool local)
{
    if (args.Length()!=2)
        return ThrowException(String::New("dist must be called with exactly two arguments."));

    if (!args[0]->IsObject() || !args[1]->IsObject())
        return ThrowException(String::New("At least one argument not an object."));

    // FiXME: Add a check for the argument type

    HandleScope handle_scope;

    Handle<Object> obj[2] =
    {
        Handle<Object>::Cast(args[0]),
        Handle<Object>::Cast(args[1])
    };

    const Handle<String> s_theta = String::New(local?"zd":"dec"); // was: zd
    const Handle<String> s_phi   = String::New(local?"az":"ra");  // was: az

    const double conv_t = M_PI/180;
    const double conv_p = local ? -M_PI/180 : M_PI/12;
    const double offset = local ? 0 : M_PI;

    const double theta0 = offset - obj[0]->Get(s_theta)->NumberValue() * conv_t;
    const double phi0   =          obj[0]->Get(s_phi  )->NumberValue() * conv_p;
    const double theta1 = offset - obj[1]->Get(s_theta)->NumberValue() * conv_t;
    const double phi1   =          obj[1]->Get(s_phi  )->NumberValue() * conv_p;

    if (!finite(theta0) || !finite(theta1) || !finite(phi0) || !finite(phi1))
        return ThrowException(String::New("some values not valid or not finite."));

    /*
    const double x0 = sin(zd0) * cos(az0);   // az0 -= az0
    const double y0 = sin(zd0) * sin(az0);   // az0 -= az0
    const double z0 = cos(zd0);

    const double x1 = sin(zd1) * cos(az1);   // az1 -= az0
    const double y1 = sin(zd1) * sin(az1);   // az1 -= az0
    const double z1 = cos(zd1);

    const double res = acos(x0*x1 + y0*y1 + z0*z1) * 180/M_PI;
    */

    // cos(az1-az0) = cos(az1)*cos(az0) + sin(az1)*sin(az0)

    const double x = sin(theta0) * sin(theta1) * cos(phi1-phi0);
    const double y = cos(theta0) * cos(theta1);

    const double res = acos(x + y) * 180/M_PI;

    return handle_scope.Close(Number::New(res));
}

Handle<Value> InterpreterV8::LocalDist(const Arguments &args)
{
    return CalcDist(args, true);
}

Handle<Value> InterpreterV8::SkyDist(const Arguments &args)
{
    return CalcDist(args, false);
}

Handle<Value> InterpreterV8::MoonDisk(const Arguments &args)
{
    if (args.Length()>1)
        return ThrowException(String::New("disk must not be called with more than one argument."));

    const uint64_t v = uint64_t(args[0]->NumberValue());
    const Time utc = args.Length()==0 ? Time() : Time(v/1000, v%1000);

    return Number::New(Nova::GetLunarDisk(utc.JD()));
}

struct AstroArgs
{
    string obs;
    Nova::LnLatPosn posn;
    double jd;
    uint64_t jsdate;

    AstroArgs() : jsdate(0) { }
};

AstroArgs EvalAstroArgs(int offset, const Arguments &args, int8_t type=2)
{
    const uint8_t max = abs(type);

    if (args.Length()>offset+max)
        throw runtime_error("Number of arguments must not exceed "+to_string(offset+max)+".");

    if (type==1  && args.Length()==offset+1 && !args[offset]->IsString())
        throw runtime_error("Argument "+to_string(offset+1)+" must be a string.");
    if (type==-1 && args.Length()==offset+1 && !args[offset]->IsDate())
        throw runtime_error("Argument "+to_string(offset+1)+" must be a date.");

    if (args.Length()==offset+1 && !(args[offset]->IsDate() || args[offset]->IsString()))
        throw runtime_error("Argument "+to_string(offset+1)+" must be a string or Date.");

    if (args.Length()==offset+2 &&
        !(args[offset+0]->IsDate() && args[offset+1]->IsString()) &&
        !(args[offset+1]->IsDate() && args[offset+0]->IsString()))
        throw runtime_error("Arguments "+to_string(offset+1)+" and "+to_string(offset+2)+" must be a string/Date or Date/string.");

    HandleScope handle_scope;

    Local<Value> obs = args.This()->Get(String::New("observatory"));
    if (args.Length()>offset && args[offset]->IsString())
        obs = args[offset];
    if (args.Length()>offset+1 && args[offset+1]->IsString())
        obs = args[offset+1];

    AstroArgs rc;

    // For constructors, observatory can stay empty if not explicitly given
    if (offset<2)
        rc.obs = Nova::LnLatPosn::preset();

    if (!obs.IsEmpty() && !obs->IsUndefined())
        rc.obs = *String::AsciiValue(obs);

    rc.posn = rc.obs;

    if ((!rc.obs.empty() || offset==0) && !rc.posn.isValid())
        throw runtime_error("Observatory "+rc.obs+" unknown.");

    Local<Value> date = args.This()->Get(String::New("time"));
    if (args.Length()>offset && args[offset]->IsDate())
        date = args[offset];
    if (args.Length()>offset+1 && args[offset+1]->IsDate())
        date = args[offset+1];

    // For constructors, time can stay empty if not explicitly given
    if (offset<2)
        rc.jsdate = Time().JavaDate();

    if (!date.IsEmpty() && !date->IsUndefined())
        rc.jsdate = uint64_t(date->NumberValue());

    rc.jd = Time(rc.jsdate/1000, rc.jsdate%1000).JD();

    return rc;
}

Handle<Value> InterpreterV8::LocalToSky(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(0, args, 2);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    Nova::ZdAzPosn hrz;
    hrz.zd = GetDataMember(args, "zd");
    hrz.az = GetDataMember(args, "az");

    if (!finite(hrz.zd) || !finite(hrz.az))
        return ThrowException(String::New("Zd and az must be finite."));

    const Nova::EquPosn equ = Nova::GetEquFromHrz(hrz, local.posn, local.jd);

    HandleScope handle_scope;

    Handle<Value> arg_loc[] = { Number::New(hrz.zd), Number::New(hrz.az), String::New(local.obs.c_str()), Date::New(local.jsdate) };
    Handle<Object> loc = fTemplateLocal->GetFunction()->NewInstance(4, arg_loc);

    Handle<Value> arg_sky[] = { Number::New(equ.ra/15), Number::New(equ.dec), loc };
    return handle_scope.Close(fTemplateSky->GetFunction()->NewInstance(3, arg_sky));
}

Handle<Value> InterpreterV8::SkyToLocal(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(0, args, 2);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    Nova::EquPosn equ;
    equ.ra  = GetDataMember(args, "ra")*15;
    equ.dec = GetDataMember(args, "dec");

    if (!finite(equ.ra) || !finite(equ.dec))
        return ThrowException(String::New("Ra and dec must be finite."));

    HandleScope handle_scope;

    const Nova::ZdAzPosn hrz = Nova::GetHrzFromEqu(equ, local.posn, local.jd);

    Handle<Value> arg[] = { Number::New(hrz.zd), Number::New(hrz.az), String::New(local.obs.c_str()), Date::New(local.jsdate) };
    return handle_scope.Close(fTemplateLocal->GetFunction()->NewInstance(4, arg));
}

Handle<Value> InterpreterV8::MoonToLocal(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(0, args, 1);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    Nova::EquPosn equ;
    equ.ra  = GetDataMember(args, "ra")*15;
    equ.dec = GetDataMember(args, "dec");

    if (!finite(equ.ra) || !finite(equ.dec))
        return ThrowException(String::New("Ra and dec must be finite."));

    HandleScope handle_scope;

    const Nova::ZdAzPosn hrz = Nova::GetHrzFromEqu(equ, local.posn, local.jd);

    Handle<Value> arg[] = { Number::New(hrz.zd), Number::New(hrz.az), String::New(local.obs.c_str()), Date::New(local.jsdate) };
    return handle_scope.Close(fTemplateLocal->GetFunction()->NewInstance(4, arg));
}

Handle<Value> InterpreterV8::ConstructorMoon(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(0, args, -1);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    const Nova::EquPosn equ = Nova::GetLunarEquCoords(local.jd, 0.01);

    HandleScope handle_scope;

    // ----------------------------

    if (!args.IsConstructCall())
        return handle_scope.Close(Constructor(args));

    Handle<Function> function =
        FunctionTemplate::New(MoonToLocal)->GetFunction();
    if (function.IsEmpty())
        return Undefined();

    Handle<Object> self = args.This();
    self->Set(String::New("ra"),      Number::New(equ.ra/15),  ReadOnly);
    self->Set(String::New("dec"),     Number::New(equ.dec),    ReadOnly);
    self->Set(String::New("toLocal"), function,                ReadOnly);
    self->Set(String::New("time"),    Date::New(local.jsdate), ReadOnly);

    return handle_scope.Close(self);
}

Handle<Value> InterpreterV8::ConstructorSky(const Arguments &args)
{
    if (args.Length()<2 || args.Length()>3)
        return ThrowException(String::New("Sky constructor takes two or three arguments."));

    if (args.Length()>2 && !args[2]->IsObject())
    {
        const string n = *String::AsciiValue(args[2]->ToObject()->GetConstructorName());
        if (n!="Local")
            return ThrowException(String::New("Third argument must be of type Local."));
    }

    const double ra  = args[0]->NumberValue();
    const double dec = args[1]->NumberValue();

    if (!finite(ra) || !finite(dec))
        return ThrowException(String::New("The first two arguments to Sky must be valid numbers."));

    // ----------------------------

    HandleScope handle_scope;

    if (!args.IsConstructCall())
        return handle_scope.Close(Constructor(args));

    Handle<Function> function =
        FunctionTemplate::New(SkyToLocal)->GetFunction();
    if (function.IsEmpty())
        return Undefined();

    Handle<Object> self = args.This();
    self->Set(String::New("ra"),      Number::New(ra),  ReadOnly);
    self->Set(String::New("dec"),     Number::New(dec), ReadOnly);
    self->Set(String::New("toLocal"), function,         ReadOnly);
    if (args.Length()==3)
        self->Set(String::New("local"), args[2], ReadOnly);

    return handle_scope.Close(self);
}

Handle<Value> InterpreterV8::ConstructorLocal(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(2, args, 2);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    const double zd = args[0]->NumberValue();
    const double az = args[1]->NumberValue();

    if (!finite(zd) || !finite(az))
        return ThrowException(String::New("The first two arguments to Local must be valid numbers."));

    // --------------------

    HandleScope handle_scope;

    if (!args.IsConstructCall())
        return handle_scope.Close(Constructor(args));

    Handle<Function> function =
        FunctionTemplate::New(LocalToSky)->GetFunction();
    if (function.IsEmpty())
        return Undefined();

    Handle<Object> self = args.This();
    self->Set(String::New("zd"),    Number::New(zd), ReadOnly);
    self->Set(String::New("az"),    Number::New(az), ReadOnly);
    self->Set(String::New("toSky"), function,        ReadOnly);
    if (!local.obs.empty())
        self->Set(String::New("observatory"), String::New(local.obs.c_str()), ReadOnly);
    if (local.jsdate>0)
        self->Set(String::New("time"),  Date::New(local.jsdate), ReadOnly);

    return handle_scope.Close(self);
}

Handle<Object> ConstructRiseSet(const AstroArgs &args, const Nova::RstTime &rst, const bool &rc)
{
    Handle<Object> obj = Object::New();
    obj->Set(String::New("time"), Date::New(args.jsdate), ReadOnly);
    obj->Set(String::New("observatory"), String::New(args.obs.c_str()), ReadOnly);

    const bool isUp = 
        (rst.rise<rst.set && (args.jd>rst.rise && args.jd<rst.set)) ||
        (rst.rise>rst.set && (args.jd<rst.set  || args.jd>rst.rise));

    obj->Set(String::New("isUp"), Boolean::New(isUp), ReadOnly);

    if (!rc) // circumpolar
        return obj;

    Handle<Value> rise  = Date::New(Time(rst.rise).JavaDate());
    Handle<Value> set   = Date::New(Time(rst.set).JavaDate());
    Handle<Value> trans = Date::New(Time(rst.transit).JavaDate());
    if (rise.IsEmpty() || set.IsEmpty() || trans.IsEmpty())
        return Handle<Object>();

    obj->Set(String::New("rise"), rise, ReadOnly);
    obj->Set(String::New("set"), set, ReadOnly);
    obj->Set(String::New("transit"), trans, ReadOnly);

    return obj;
}

Handle<Value> InterpreterV8::SunHorizon(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(1, args, 2);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    HandleScope handle_scope;

    double hrz = NAN;
    if (args.Length()==0 || args[0]->IsNull())
        hrz = LN_SOLAR_STANDART_HORIZON;
    if (args.Length()>0 && args[0]->IsNumber())
        hrz = args[0]->NumberValue();
    if (args.Length()>0 && args[0]->IsString())
    {
        string arg(Tools::Trim(*String::AsciiValue(args[0])));
        transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

        if (arg==string("horizon").substr(0, arg.length()))
            hrz = LN_SOLAR_STANDART_HORIZON;
        if (arg==string("civil").substr(0, arg.length()))
            hrz = LN_SOLAR_CIVIL_HORIZON;
        if (arg==string("nautical").substr(0, arg.length()))
            hrz = LN_SOLAR_NAUTIC_HORIZON;
        if (arg==string("fact").substr(0, arg.length()))
            hrz = -13;
        if (arg==string("astronomical").substr(0, arg.length()))
            hrz = LN_SOLAR_ASTRONOMICAL_HORIZON;
    }

    if (!finite(hrz))
        return ThrowException(String::New("First argument did not yield a valid number."));

    ln_rst_time sun;
    const bool rc = ln_get_solar_rst_horizon(local.jd-0.5, &local.posn, hrz, &sun)==0;
    Handle<Object> rst = ConstructRiseSet(local, sun, rc);
    rst->Set(String::New("horizon"), Number::New(hrz));
    return handle_scope.Close(rst);
};

Handle<Value> InterpreterV8::MoonHorizon(const Arguments &args)
{
    AstroArgs local;
    try
    {
        local = EvalAstroArgs(0, args, 2);
    }
    catch (const exception &e)
    {
        return ThrowException(String::New(e.what()));
    }

    HandleScope handle_scope;

    ln_rst_time moon;
    const bool rc = ln_get_lunar_rst(local.jd-0.5, &local.posn, &moon)==0;
    Handle<Object> rst = ConstructRiseSet(local, moon, rc);
    return handle_scope.Close(rst);
};
#endif

// ==========================================================================
//                            Process control
// ==========================================================================

bool InterpreterV8::HandleException(TryCatch& try_catch, const char *where)
{
    if (!try_catch.HasCaught() || !try_catch.CanContinue())
        return true;

    const HandleScope handle_scope;

    Handle<Value> except = try_catch.Exception();
    if (except.IsEmpty() || except->IsNull())
        return true;

    const String::AsciiValue exception(except);

    const Handle<Message> message = try_catch.Message();
    if (message.IsEmpty())
        return false;

    ostringstream out;

    if (!message->GetScriptResourceName()->IsUndefined())
    {
        // Print (filename):(line number): (message).
        const String::AsciiValue filename(message->GetScriptResourceName());
        if (filename.length()>0)
        {
            out << *filename;
            if (message->GetLineNumber()>0)
                out << ": l." << message->GetLineNumber();
            if (*exception)
                out << ": ";
        }
    }

    if (*exception)
        out << *exception;

    out << " [" << where << "]";

    JsException(out.str());

    // Print line of source code.
    const String::AsciiValue sourceline(message->GetSourceLine());
    if (*sourceline)
        JsException(*sourceline);

    // Print wavy underline (GetUnderline is deprecated).
    const int start = message->GetStartColumn();
    const int end   = message->GetEndColumn();

    out.str("");
    if (start>0)
        out << setfill(' ') << setw(start) << ' ';
    out << setfill('^') << setw(end-start) << '^';

    JsException(out.str());

    const String::AsciiValue stack_trace(try_catch.StackTrace());
    if (stack_trace.length()<=0)
        return false;

    if (!*stack_trace)
        return false;

    const string trace(*stack_trace);

    typedef boost::char_separator<char> separator;
    const boost::tokenizer<separator> tokenizer(trace, separator("\n"));

    // maybe skip: "    at internal:"
    // maybe skip: "    at unknown source:"

    auto it = tokenizer.begin();
    JsException("");
    while (it!=tokenizer.end())
        JsException(*it++);

    return false;
}

Handle<Value> InterpreterV8::ExecuteInternal(const string &code)
{
    // Try/catch and re-throw hides our internal code from
    // the displayed exception showing the origin and shows
    // the user function instead.
    TryCatch exception;

    const Handle<Value> result = ExecuteCode(code);

    // This hides the location of the exception in the internal code,
    // which is wanted.
    if (exception.HasCaught())
        exception.ReThrow();

    return result;
}

Handle<Value> InterpreterV8::ExecuteCode(const string &code, const string &file)
{
    HandleScope handle_scope;

    const Handle<String> source = String::New(code.c_str(), code.size());
    const Handle<String> origin = String::New(file.c_str());
    if (source.IsEmpty())
        return Undefined();

    const Handle<Script> script = Script::Compile(source, origin);
    if (script.IsEmpty())
        return Undefined();

    const Handle<String> __date__ = String::New("__DATE__");
    const Handle<String> __file__ = String::New("__FILE__");

    Handle<Value> save_date;
    Handle<Value> save_file;

    Handle<Object> global = Context::GetCurrent()->Global();
    if (!global.IsEmpty())
    {
        struct stat attrib;
        if (stat(file.c_str(), &attrib)==0)
        {
            save_date = global->Get(__date__);
            save_file = global->Get(__file__);

            global->Set(__file__, String::New(file.c_str()));

            const Local<Value> date = Date::New(attrib.st_mtime*1000);
            if (!date.IsEmpty())
                global->Set(__date__, date);
        }
    }

    const Handle<Value> rc = script->Run();
    if (rc.IsEmpty())
        return Undefined();

    if (!global.IsEmpty() && !save_date.IsEmpty())
    {
        global->ForceSet(__date__, save_date);
        global->ForceSet(__file__, save_file);
    }

    return handle_scope.Close(rc);
}

void InterpreterV8::ExecuteConsole()
{
    JsSetState(3);

    WindowLog lout;
    lout << "\n " << kUnderline << " JavaScript interpreter " << kReset << " (enter '.q' to quit)\n" << endl;

    Readline::StaticPushHistory("java.his");

    string command;
    while (1)
    {
        // Create a local handle scope so that left-overs from single
        // console inputs will not fill up the memory
        const HandleScope handle_scope;

        // Unlocking is necessary for the preemption to work
        const Unlocker global_unlock;

        const string buffer = Tools::Trim(Readline::StaticPrompt(command.empty() ? "JS> " : " \\> "));
        if (buffer==".q")
            break;

        // buffer empty, do nothing
        if (buffer.empty())
            continue;

        // Compose command
        if (!command.empty())
            command += ' ';
        command += buffer;

        // If line ends with a backslash, allow addition of next line
        auto back = command.rbegin();
        if (*back=='\\')
        {
            *back = ' ';
            command = Tools::Trim(command);
            continue;
        }

        // Locking is necessary to be able to execute java script code
        const Locker lock;

        // Catch exceptions during code compilation
        TryCatch exception;

        // Execute code which was entered
        const Handle<Value> rc = ExecuteCode(command, "console");

        // If all went well and the result wasn't undefined then print
        // the returned value.
        if (!rc->IsUndefined() && !rc->IsFunction())
            JsResult(*String::AsciiValue(rc));

        if (!HandleException(exception, "console"))
            lout << endl;

        // Stop all other threads
        for (auto it=fThreadIds.begin(); it!=fThreadIds.end(); it++)
            V8::TerminateExecution(*it);

        // Allow the java scripts (threads) to run and hence to terminate
        const Unlocker unlock;

        // Wait until all threads are terminated
        while (!fThreadIds.empty())
            usleep(1000);

        // command has been executed, collect new command
        command = "";
    }

    lout << endl;

    Readline::StaticPopHistory("java.his");
}

// ==========================================================================
//                                  CORE
// ==========================================================================

InterpreterV8::InterpreterV8() : fThreadId(-1)
{
    const string ver(V8::GetVersion());

    typedef boost::char_separator<char> separator;
    const boost::tokenizer<separator> tokenizer(ver, separator("."));

    const vector<string> tok(tokenizer.begin(), tokenizer.end());

    const int major = tok.size()>0 ? stol(tok[0]) : -1;
    const int minor = tok.size()>1 ? stol(tok[1]) : -1;
    const int build = tok.size()>2 ? stol(tok[2]) : -1;

    if (major>3 || (major==3 && minor>9) || (major==3 && minor==9 && build>10))
    {
        const string argv = "--use_strict";
        V8::SetFlagsFromString(argv.c_str(), argv.size());
    }

    /*
     const string argv1 = "--prof";
     const string argv2 = "--noprof-lazy";

     V8::SetFlagsFromString(argv1.c_str(), argv1.size());
     V8::SetFlagsFromString(argv2.c_str(), argv2.size());
     */

    This = this;
}

Handle<Value> InterpreterV8::Constructor(/*Handle<FunctionTemplate> T,*/ const Arguments &args)
{
    vector<Handle<Value>> argv(args.Length());

    for (int i=0; i<args.Length(); i++)
        argv[i] = args[i];

    return args.Callee()->NewInstance(args.Length(), argv.data());
}


void InterpreterV8::AddFormatToGlobal()// const
{
    const string code =
        "String.form = function(str, arr)"
        "{"
            "var i = -1;"
            "function callback(exp, p0, p1, p2, p3, p4/*, pos, str*/)"
            "{"
                "if (exp=='%%')"
                    "return '%';"
                ""
                "if (arr[++i]===undefined)"
                    "return undefined;"
                ""
                "var exp  = p2 ? parseInt(p2.substr(1)) : undefined;"
                "var base = p3 ? parseInt(p3.substr(1)) : undefined;"
                ""
                "var val;"
                "switch (p4)"
                "{"
                "case 's': val = arr[i]; break;"
                "case 'c': val = arr[i][0]; break;"
                "case 'f': val = parseFloat(arr[i]).toFixed(exp); break;"
                "case 'p': val = parseFloat(arr[i]).toPrecision(exp); break;"
                "case 'e': val = parseFloat(arr[i]).toExponential(exp); break;"
                "case 'x': val = parseInt(arr[i]).toString(base?base:16); break;"
                "case 'd': val = parseFloat(parseInt(arr[i], base?base:10).toPrecision(exp)).toFixed(0); break;"
                //"default:\n"
                //"    throw new SyntaxError('Conversion specifier '+p4+' unknown.');\n"
                "}"
                ""
                "val = val==undefined || typeof(val)=='object' ? JSON.stringify(val) : val.toString(base);"
                ""
                "var sz = parseInt(p1); /* padding size */"
                "var ch = p1 && p1[0]=='0' ? '0' : ' '; /* isnull? */"
                "while (val.length<sz)"
                    "val = p0 !== undefined ? val+ch : ch+val; /* isminus? */"
                ""
                "return val;"
            "}"
            ""
            "var regex = /%(-)?(0?[0-9]+)?([.][0-9]+)?([#][0-9]+)?([scfpexd])/g;"
            "return str.replace(regex, callback);"
        "}"
        "\n"
        "String.prototype.$ = function()"
        "{"
            "return String.form(this, Array.prototype.slice.call(arguments));"
        "}"
        "\n"
        "String.prototype.count = function(c,i)"
        "{"
            "return (this.match(new RegExp(c,i?'gi':'g'))||[]).length;"
        "}"/*
        "\n"
        "var format = function()"
        "{"
            "return dim.format(arguments[0], Array.prototype.slice.call(arguments,1));"
        "}"*/;

    // ExcuteInternal does not work properly here...
    // If suring compilation an exception is thrown, it will not work
    Handle<Script> script = Script::New(String::New(code.c_str()), String::New("internal"));
    if (!script.IsEmpty())
        script->Run();
}

void InterpreterV8::JsLoad(const std::string &)
{
    Readline::SetScriptDepth(1);
}

void InterpreterV8::JsEnd(const std::string &)
{
    Readline::SetScriptDepth(0);
}

bool InterpreterV8::JsRun(const string &filename, const map<string, string> &map)
{
    const Locker locker;
    fThreadId = V8::GetCurrentThreadId();

    JsPrint(string("JavaScript Engine V8 ")+V8::GetVersion());

    JsLoad(filename);

    const HandleScope handle_scope;

    // Create a template for the global object.
    Handle<ObjectTemplate> dim = ObjectTemplate::New();
    dim->Set(String::New("log"),       FunctionTemplate::New(WrapLog),       ReadOnly);
    dim->Set(String::New("alarm"),     FunctionTemplate::New(WrapAlarm),     ReadOnly);
    dim->Set(String::New("wait"),      FunctionTemplate::New(WrapWait),      ReadOnly);
    dim->Set(String::New("send"),      FunctionTemplate::New(WrapSend),      ReadOnly);
    dim->Set(String::New("state"),     FunctionTemplate::New(WrapState),     ReadOnly);
    dim->Set(String::New("version"),   Integer::New(DIM_VERSION_NUMBER),     ReadOnly);
    dim->Set(String::New("getStates"), FunctionTemplate::New(WrapGetStates), ReadOnly);
    dim->Set(String::New("getDescription"), FunctionTemplate::New(WrapGetDescription), ReadOnly);
    dim->Set(String::New("getServices"), FunctionTemplate::New(WrapGetServices), ReadOnly);

    Handle<ObjectTemplate> dimctrl = ObjectTemplate::New();
    dimctrl->Set(String::New("defineState"), FunctionTemplate::New(WrapNewState),  ReadOnly);
    dimctrl->Set(String::New("setState"),    FunctionTemplate::New(WrapSetState),  ReadOnly);
    dimctrl->Set(String::New("getState"),    FunctionTemplate::New(WrapGetState),  ReadOnly);
    dimctrl->Set(String::New("setInterruptHandler"), FunctionTemplate::New(WrapSetInterrupt), ReadOnly);
    dimctrl->Set(String::New("triggerInterrupt"), FunctionTemplate::New(WrapTriggerInterrupt), ReadOnly);

    Handle<ObjectTemplate> v8 = ObjectTemplate::New();
    v8->Set(String::New("sleep"),   FunctionTemplate::New(WrapSleep), ReadOnly);
    v8->Set(String::New("timeout"), FunctionTemplate::New(WrapTimeout), ReadOnly);
    v8->Set(String::New("version"), String::New(V8::GetVersion()),    ReadOnly);

    Handle<ObjectTemplate> console = ObjectTemplate::New();
    console->Set(String::New("out"), FunctionTemplate::New(WrapOut), ReadOnly);
    console->Set(String::New("warn"), FunctionTemplate::New(WrapWarn), ReadOnly);

    Handle<ObjectTemplate> onchange = ObjectTemplate::New();
    onchange->SetNamedPropertyHandler(OnChangeGet, WrapOnChangeSet);
    dim->Set(String::New("onchange"), onchange);

    Handle<ObjectTemplate> global = ObjectTemplate::New();
    global->Set(String::New("v8"),      v8,      ReadOnly);
    global->Set(String::New("dim"),     dim,     ReadOnly);
    global->Set(String::New("dimctrl"), dimctrl, ReadOnly);
    global->Set(String::New("console"), console, ReadOnly);
    global->Set(String::New("include"), FunctionTemplate::New(WrapInclude),                ReadOnly);
    global->Set(String::New("exit"),    FunctionTemplate::New(WrapExit),                   ReadOnly);

    Handle<FunctionTemplate> sub = FunctionTemplate::New(WrapSubscription);
    sub->SetClassName(String::New("Subscription"));
    sub->InstanceTemplate()->SetInternalFieldCount(1);
    global->Set(String::New("Subscription"), sub, ReadOnly);

#ifdef HAVE_SQL
    Handle<FunctionTemplate> db = FunctionTemplate::New(WrapDatabase);
    db->SetClassName(String::New("Database"));
    db->InstanceTemplate()->SetInternalFieldCount(1);
    global->Set(String::New("Database"), db, ReadOnly);
#endif

    Handle<FunctionTemplate> thread = FunctionTemplate::New(WrapThread);
    thread->SetClassName(String::New("Thread"));
    global->Set(String::New("Thread"), thread, ReadOnly);

    Handle<FunctionTemplate> file = FunctionTemplate::New(WrapFile);
    file->SetClassName(String::New("File"));
    global->Set(String::New("File"), file, ReadOnly);

    Handle<FunctionTemplate> evt = FunctionTemplate::New();
    evt->SetClassName(String::New("Event"));
    global->Set(String::New("Event"), evt, ReadOnly);

    Handle<FunctionTemplate> desc = FunctionTemplate::New();
    desc->SetClassName(String::New("Description"));
    global->Set(String::New("Description"), desc, ReadOnly);

    fTemplateEvent = evt;
    fTemplateDescription = desc;

#ifdef HAVE_MAILX
    Handle<FunctionTemplate> mail = FunctionTemplate::New(ConstructorMail);
    mail->SetClassName(String::New("Mail"));
    global->Set(String::New("Mail"), mail, ReadOnly);
#endif

#ifdef HAVE_CURL
    Handle<FunctionTemplate> curl = FunctionTemplate::New(ConstructorCurl);
    curl->SetClassName(String::New("Curl"));
    global->Set(String::New("Curl"), curl, ReadOnly);
#endif

#ifdef HAVE_NOVA
    Handle<FunctionTemplate> sky = FunctionTemplate::New(ConstructorSky);
    sky->SetClassName(String::New("Sky"));
    sky->Set(String::New("dist"),  FunctionTemplate::New(SkyDist), ReadOnly);
    global->Set(String::New("Sky"), sky, ReadOnly);

    Handle<FunctionTemplate> loc = FunctionTemplate::New(ConstructorLocal);
    loc->SetClassName(String::New("Local"));
    loc->Set(String::New("dist"),  FunctionTemplate::New(LocalDist), ReadOnly);
    global->Set(String::New("Local"), loc, ReadOnly);

    Handle<FunctionTemplate> moon = FunctionTemplate::New(ConstructorMoon);
    moon->SetClassName(String::New("Moon"));
    moon->Set(String::New("disk"), FunctionTemplate::New(MoonDisk), ReadOnly);
    moon->Set(String::New("horizon"), FunctionTemplate::New(MoonHorizon), ReadOnly);
    global->Set(String::New("Moon"), moon, ReadOnly);

    Handle<FunctionTemplate> sun = FunctionTemplate::New();
    sun->SetClassName(String::New("Sun"));
    sun->Set(String::New("horizon"), FunctionTemplate::New(SunHorizon), ReadOnly);
    global->Set(String::New("Sun"), sun, ReadOnly);

    fTemplateLocal = loc;
    fTemplateSky   = sky;
#endif

    // Persistent
    Persistent<Context> context = Context::New(NULL, global);
    if (context.IsEmpty())
    {
        JsException("Creation of global context failed...");
        JsEnd(filename);
        return false;
    }

    // Switch off eval(). It is not possible to track it's exceptions.
    context->AllowCodeGenerationFromStrings(false);

    Context::Scope scope(context);

    Handle<Array> args = Array::New(map.size());
    for (auto it=map.begin(); it!=map.end(); it++)
        args->Set(String::New(it->first.c_str()), String::New(it->second.c_str()));
    context->Global()->Set(String::New("$"),   args, ReadOnly);
    context->Global()->Set(String::New("arg"), args, ReadOnly);

    const Local<Value> starttime = Date::New(Time().JavaDate());
    if (!starttime.IsEmpty())
        context->Global()->Set(String::New("__START__"), starttime, ReadOnly);

    //V8::ResumeProfiler();

    TryCatch exception;

    AddFormatToGlobal();

    if (!exception.HasCaught())
    {
        JsStart(filename);

        Locker::StartPreemption(10);

        if (filename.empty())
            ExecuteConsole();
        else
        {
            // We call script->Run because it is the only way to
            // catch exceptions.
            const Handle<String> source = String::New(("include('"+filename+"');").c_str());
            const Handle<String> origin = String::New("main");
            const Handle<Script> script = Script::Compile(source, origin);
            if (!script.IsEmpty())
            {
                JsSetState(3);
                script->Run();
            }
        }

        Locker::StopPreemption();

        // Stop all other threads
        for (auto it=fThreadIds.begin(); it!=fThreadIds.end(); it++)
            V8::TerminateExecution(*it);
        fThreadIds.clear();
    }

    // Handle an exception
    /*const bool rc =*/ HandleException(exception, "main");

    // IsProfilerPaused()
    // V8::PauseProfiler();

    // -----
    // This is how an exit handler could look like, but there is no way to interrupt it
    // -----
    // Handle<Object> obj = Handle<Object>::Cast(context->Global()->Get(String::New("dim")));
    // if (!obj.IsEmpty())
    // {
    //     Handle<Value> onexit = obj->Get(String::New("onexit"));
    //     if (!onexit->IsUndefined())
    //         Handle<Function>::Cast(onexit)->NewInstance(0, NULL); // argc, argv
    //     // Handle<Object> result = Handle<Function>::Cast(onexit)->NewInstance(0, NULL); // argc, argv
    // }

    //context->Exit();

    // The threads are started already and wait to get the lock
    // So we have to unlock (manual preemtion) so that they get
    // the signal to terminate.
    {
        const Unlocker unlock;

        for (auto it=fThreads.begin(); it!=fThreads.end(); it++)
            it->join();
        fThreads.clear();
    }

    // Now we can dispose all persistent handles from state callbacks
    for (auto it=fStateCallbacks.begin(); it!=fStateCallbacks.end(); it++)
        it->second.Dispose();
    fStateCallbacks.clear();

    // Now we can dispose the persistent interrupt handler
    fInterruptCallback.Dispose();
    fInterruptCallback.Clear();

    // Now we can dispose all persistent handles from reverse maps
    for (auto it=fReverseMap.begin(); it!=fReverseMap.end(); it++)
        it->second.Dispose();
    fReverseMap.clear();

#ifdef HAVE_SQL
    // ...and close all database handles
    for (auto it=fDatabases.begin(); it!=fDatabases.end(); it++)
        delete *it;
    fDatabases.clear();
#endif

    fStates.clear();

    context.Dispose();

    JsEnd(filename);

    return true;
}

void InterpreterV8::JsStop()
{
    Locker locker;
    V8::TerminateExecution(This->fThreadId);
}

vector<string> InterpreterV8::JsGetCommandList(const char *, int) const
{
    vector<string> rc;

    rc.emplace_back("for (");
    rc.emplace_back("while (");
    rc.emplace_back("if (");
    rc.emplace_back("switch (");
    rc.emplace_back("case ");
    rc.emplace_back("var ");
    rc.emplace_back("function ");
    rc.emplace_back("Date(");
    rc.emplace_back("new Date(");
    rc.emplace_back("'use strict';");
    rc.emplace_back("undefined");
    rc.emplace_back("null");
    rc.emplace_back("delete ");
    rc.emplace_back("JSON.stringify(");
    rc.emplace_back("JSON.parse(");

    rc.emplace_back("dim.log(");
    rc.emplace_back("dim.alarm(");
    rc.emplace_back("dim.wait(");
    rc.emplace_back("dim.send(");
    rc.emplace_back("dim.state(");
    rc.emplace_back("dim.version");
    rc.emplace_back("dim.getStates(");
    rc.emplace_back("dim.getDescription(");
    rc.emplace_back("dim.getServices(");

    rc.emplace_back("dimctrl.defineState(");
    rc.emplace_back("dimctrl.setState(");
    rc.emplace_back("dimctrl.getState(");
    rc.emplace_back("dimctrl.setInterruptHandler(");
    rc.emplace_back("dimctrl.triggerInterrupt(");

    rc.emplace_back("v8.sleep(");
    rc.emplace_back("v8.timeout(");
    rc.emplace_back("v8.version()");

    rc.emplace_back("console.out(");
    rc.emplace_back("console.warn(");

    rc.emplace_back("include(");
    rc.emplace_back("exit()");

#ifdef HAVE_SQL
    rc.emplace_back("Database(");
    rc.emplace_back("new Database(");

    rc.emplace_back(".table");
    rc.emplace_back(".user");
    rc.emplace_back(".database");
    rc.emplace_back(".port");
    rc.emplace_back(".query");
#endif

    rc.emplace_back("Subscription(");
    rc.emplace_back("new Subscription(");

    rc.emplace_back("Thread(");
    rc.emplace_back("new Thread(");

    rc.emplace_back("File(");
    rc.emplace_back("new File(");

    rc.emplace_back("Event(");
    rc.emplace_back("new Event(");

    rc.emplace_back("Description(");
    rc.emplace_back("new Description(");

#ifdef HAVE_MAILX
    rc.emplace_back("Mail(");
    rc.emplace_back("new Mail(");

    rc.emplace_back(".subject");
    rc.emplace_back(".receipients");
    rc.emplace_back(".attachments");
    rc.emplace_back(".bcc");
    rc.emplace_back(".cc");
    rc.emplace_back(".text");
    rc.emplace_back(".send(");
#endif

#ifdef HAVE_CURL
    rc.emplace_back("Curl(");
    rc.emplace_back("new Curl(");

    rc.emplace_back(".url");
    rc.emplace_back(".user");
    rc.emplace_back(".data");
//    rc.emplace_back(".send("); -> MAILX
#endif

#ifdef HAVE_NOVA
    rc.emplace_back("Sky(");
    rc.emplace_back("new Sky(");

    rc.emplace_back("Sky.dist");
    rc.emplace_back("Local(");

    rc.emplace_back("new Local(");
    rc.emplace_back("Local.dist");

    rc.emplace_back("Moon(");
    rc.emplace_back("new Moon(");
    rc.emplace_back("Moon.disk(");
    rc.emplace_back("Moon.horizon(");

    rc.emplace_back("Sun.horizon(");

    rc.emplace_back(".zd");
    rc.emplace_back(".az");
    rc.emplace_back(".ra");
    rc.emplace_back(".dec");

    rc.emplace_back(".toLocal(");
    rc.emplace_back(".toSky(");
    rc.emplace_back(".rise");
    rc.emplace_back(".set");
    rc.emplace_back(".transit");
    rc.emplace_back(".isUp");

    rc.emplace_back("horizon");
    rc.emplace_back("civil");
    rc.emplace_back("nautical");
    rc.emplace_back("astronomical");
#endif

    rc.emplace_back(".server");
    rc.emplace_back(".service");
    rc.emplace_back(".name");
    rc.emplace_back(".isCommand");
    rc.emplace_back(".format");
    rc.emplace_back(".description");
    rc.emplace_back(".unit");
    rc.emplace_back(".delim");
    rc.emplace_back(".isOpen");

    rc.emplace_back(".qos");
    rc.emplace_back(".size");
    rc.emplace_back(".counter");
    rc.emplace_back(".type");
    rc.emplace_back(".obj");
    rc.emplace_back(".data");
    rc.emplace_back(".comment");
    rc.emplace_back(".index");
    rc.emplace_back(".time");
    rc.emplace_back(".close()");
    rc.emplace_back(".onchange");
    rc.emplace_back(".get(");


    rc.emplace_back("__DATE__");
    rc.emplace_back("__FILE__");

    return rc;
}

#endif

InterpreterV8 *InterpreterV8::This = 0;
