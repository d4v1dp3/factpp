#include "StateMachineDimControl.h"

#include <boost/filesystem.hpp>

#include "Dim.h"
#include "Event.h"
#include "Readline.h"
#include "InterpreterV8.h"
#include "Configuration.h"
#include "Converter.h"

#include "tools.h"

#ifdef HAVE_NOVA
#include "nova.h"
#endif

using namespace std;

// ------------------------------------------------------------------------

bool StateMachineDimControl::fIsServer = false;

string StateMachineDimControl::Line(const string &txt, char fill)
{
    const int n = (55-txt.length())/2;

    ostringstream out;
    out << setfill(fill);
    out << setw(n) << fill << ' ';
    out << txt;
    out << ' ' << setw(n) << fill;

    if (2*n+txt.length()+2 != 57)
        out << fill;

    return out.str();
}

int StateMachineDimControl::ChangeState(int qos, const Time &, int scriptdepth, string scriptfile, string user)
{
    string msg;
    /*
    switch (qos)
    {
    case -4: msg = "End";       break;
    case -3: msg = "Loading";   break;
    case -2: msg = "Compiling"; break;
    case -1: msg = "Running";   break;
    default:
        {
            ostringstream out;
            out << "Label " << qos;
            msg = out.str();
        }
    }
    */

    //if (qos<0)
        msg += to_string(scriptdepth);

    msg += ":"+scriptfile+"["+user+":"+to_string(getpid())+"]";

    //if (fDebug)
    //Write(time, Line(msg, qos<-1 ? '=' :'-'), MessageImp::kInternal);

    if (qos==-4)
        fScriptUser = fUser;

    SetCurrentState(qos+4, msg.c_str());
    //SetCurrentState(qos+4, Line(msg, qos<-1 ? '=' :'-').c_str());
    return GetCurrentState();

    //return qos+4;
}

int StateMachineDimControl::ChangeState(int state)
{
    return ChangeState(state, Time(), Readline::GetScriptDepth(), Readline::GetScript(), fScriptUser);
        /*
         === This might be necessary for thread safety,
         === but it break that the signal for the start of a new
         === script arrives synchronously before the first output
         === from the script

        // Post an anonymous event to the event loop
        Event evt("");
        evt.AssignFunction(bind(&StateMachineDimControl::ChangeState, this,
                                qos, time, Readline::GetScriptDepth(),
                                Readline::GetScript(), fScriptUser));
        return PostEvent(evt);
        */
}

int StateMachineDimControl::StartScript(const EventImp &imp, const string &cmd)
{
    string opt(imp.GetString());

    map<string,string> data;
    try
    {
        data = Tools::Split(opt, true);
    }
    catch (const exception &e)
    {
        Warn("DIM_CONTROL/START: Parsing failed: "+opt+" ["+string(e.what())+"]");
        return GetCurrentState();
    }

    if (imp.GetSize()==0 || opt.size()==0 || opt[0]==0)
    {
        Error("DIM_CONTROL/START: File name missing.");
        return GetCurrentState();
    }

    if (fDebug)
        Debug("Start '"+opt+"' received.");

    if (fDebug)
        Debug("Received data: "+imp.GetString());

    const auto user = data.find("user");
    fScriptUser = user==data.end() ? fUser : user->second;

    if (fDebug)
    {
        for (auto it=data.begin(); it!=data.end(); it++)
            Debug("   Arg: "+it->first+" = "+it->second);
    }

    string emit = cmd+imp.GetString();
    if (cmd==".js ")
        emit += fArgumentsJS;

    Readline::SetExternalInput(emit);
    return GetCurrentState();
}

int StateMachineDimControl::StopScript(const EventImp &imp)
{
    const string str(imp.GetString());

    string msg("Stop received");
    msg += str.empty() ? "." : " ["+str+"]";

    Info(msg);

    Readline::StopScript();
    InterpreterV8::JsStop();
    return GetCurrentState();
}

void StateMachineDimControl::Stop(int code)
{
    InterpreterV8::JsStop();
    StateMachineDim::Stop(code);
}

int StateMachineDimControl::InterruptScript(const EventImp &evt)
{
    if (!fInterruptHandler)
        return GetCurrentState();

    string str = evt.GetString();

    const size_t p = str.find_last_of('\n');
    if (p!=string::npos)
        str[p] = ':';

    if (GetCurrentState()<3)
    {
        Warn("Interrupt request received ["+str+"]... but no running script.");
        return GetCurrentState();
    }

    Info("Interrupt request received ["+str+"]");
    return fInterruptHandler(evt);
}

bool StateMachineDimControl::SendDimCommand(const string &server, string str, ostream &lout)
{
    const lock_guard<mutex> guard(fMutex);

    if (fServerList.find(server)==fServerList.end())
        throw runtime_error("SendDimCommand - Server '"+server+"' not online.");

    str = Tools::Trim(str);

    // Find the delimiter between the command name and the data
    size_t p0 = str.find_first_of(' ');
    if (p0==string::npos)
        p0 = str.length();

    // Get just the command name separated from the data
    const string name = str.substr(0, p0);

    // Compile the command which will be sent to the state-machine
    for (auto is=fServiceList.begin(); is!=fServiceList.end(); is++)
    {
        if (str.empty() && is->server==server)
            return true;

        if (is->server!=server || is->service!=name)
            continue;

        if (!is->iscmd)
            throw runtime_error("'"+server+"/"+name+" not a command.");

        // Avoid compiler warning of unused parameter
        lout << flush;

        // Convert the user entered data according to the format string
        // into a data block which will be attached to the event
#ifndef DEBUG
        ostringstream sout;
        const Converter conv(sout, is->format, false);
#else
        const Converter conv(lout, is->format, false);
#endif
        if (!conv)
            throw runtime_error("Couldn't properly parse the format... ignored.");

#ifdef DEBUG
        lout << kBlue << server << '/' << name;
#endif
        const vector<char> v = conv.GetVector(str.substr(p0));
#ifdef DEBUG
        lout << kBlue << " [" << v.size() << "]" << endl;
#endif
        const string cmd = server + '/' + name;
        const int rc = DimClient::sendCommand(cmd.c_str(), (void*)v.data(), v.size());
        if (!rc)
            throw runtime_error("ERROR - Sending command "+cmd+" failed.");

        return true;
    }

    if (!str.empty())
        throw runtime_error("SendDimCommand - Format information for "+server+"/"+name+" not yet available.");

    return false;
}

int StateMachineDimControl::PrintStates(std::ostream &out, const std::string &serv)
{
    const lock_guard<mutex> guard(fMutex);

    int rc = 0;
    for (auto it=fServerList.begin(); it!=fServerList.end(); it++)
    {
        if (!serv.empty() && *it!=serv)
            continue;

        out << kRed << "----- " << *it << " -----" << endl;

        int cnt = 0;
        for (auto is=fStateDescriptionList.begin(); is!=fStateDescriptionList.end(); is++)
        {
            const string &server = is->first.first;

            if (server!=*it)
                continue;

            const int32_t &state   = is->first.second;
            const string  &name    = is->second.first;
            const string  &comment = is->second.second;

            out << kBold   << setw(5) << state << kReset << ": ";
            out << kYellow << name;
            if (!comment.empty())
                out << kBlue   << " (" << comment << ")";
            out << endl;

            cnt++;
        }

        if (cnt==0)
            out << "   <no states>" << endl;
        else
            rc++;

        out << endl;
    }

    return rc;
}

int StateMachineDimControl::PrintDescription(std::ostream &out, bool iscmd, const std::string &serv, const std::string &service)
{
    const lock_guard<mutex> guard(fMutex);

    int rc = 0;
    for (auto it=fServerList.begin(); it!=fServerList.end(); it++)
    {
        if (!serv.empty() && *it!=serv)
            continue;

        out << kRed << "----- " << *it << " -----" << endl << endl;

        for (auto is=fServiceList.begin(); is!=fServiceList.end(); is++)
        {
            if (is->server!=*it)
                continue;

            if (!service.empty() && is->service!=service)
                continue;

            if (is->iscmd!=iscmd)
                continue;

            rc++;

            out << " " << is->service;
            if (!is->format.empty())
                out << '[' << is->format << ']';

            const auto id = fServiceDescriptionList.find(*it+"/"+is->service);
            if (id!=fServiceDescriptionList.end())
            {
                const vector<Description> &v = id->second;

                for (auto j=v.begin()+1; j!=v.end(); j++)
                    out << " <" << j->name << ">";
                out << endl;

                if (!v[0].comment.empty())
                    out << "    " << v[0].comment << endl;

                for (auto j=v.begin()+1; j!=v.end(); j++)
                {
                    out << "    " << kGreen << j->name;
                    if (!j->comment.empty())
                        out << kReset << ": " << kBlue << j->comment;
                    if (!j->unit.empty())
                        out << kYellow << " [" << j->unit << "]";
                    out << endl;
                }
            }
            out << endl;
        }
        out << endl;
    }

    return rc;
}

int StateMachineDimControl::HandleStateChange(const string &server, DimDescriptions *dim)
{
    fMutex.lock();
    const State descr = dim->description();
    const State state = State(dim->state(), descr.index==DimState::kNotAvailable?"":descr.name, descr.comment, dim->cur.first);
    fCurrentStateList[server] = state;
    fMutex.unlock();

    fStateCallback(server, state);

    return GetCurrentState();
}

State StateMachineDimControl::GetServerState(const std::string &server)
{
    const lock_guard<mutex> guard(fMutex);

    const auto it = fCurrentStateList.find(server);
    return it==fCurrentStateList.end() ? State() : it->second;
}

int StateMachineDimControl::HandleStates(const string &server, DimDescriptions *dim)
{
    const lock_guard<mutex> guard(fMutex);

    const auto is = fCurrentStateList.find(server);
    for (auto it=dim->states.begin(); it!=dim->states.end(); it++)
    {
        fStateDescriptionList[make_pair(server, it->index)] = make_pair(it->name, it->comment);
        if (is==fCurrentStateList.end())
            continue;

        State &s = is->second;
        if (s.index==it->index)
        {
            s.name    = it->name;
            s.comment = it->comment;
        }
    }

    return GetCurrentState();
}

int StateMachineDimControl::HandleDescriptions(DimDescriptions *dim)
{
    const lock_guard<mutex> guard(fMutex);

    for (auto it=dim->descriptions.begin(); it!=dim->descriptions.end(); it++)
        fServiceDescriptionList[it->front().name].assign(it->begin(), it->end());

    return GetCurrentState();
}

std::vector<Description> StateMachineDimControl::GetDescription(const std::string &service)
{
    const lock_guard<mutex> guard(fMutex);

    const auto it = fServiceDescriptionList.find(service);
    return it==fServiceDescriptionList.end() ? vector<Description>() : it->second;
}

int StateMachineDimControl::HandleServerAdd(const string &server)
{
    if (fIsServer && server=="DIM_CONTROL")
        return GetCurrentState();

    if (server!="DIS_DNS")
    {
        struct Find : string
        {
            Find(const string &ref) : string(ref) { }
            bool operator()(const DimDescriptions *dim) { return *this==dim->server; }
        };

        if (find_if(fDimDescriptionsList.begin(), fDimDescriptionsList.end(),
                    Find(server))==fDimDescriptionsList.end())
        {
            DimDescriptions *d = new DimDescriptions(server);

            fDimDescriptionsList.push_back(d);
            d->SetCallback(bind(&StateMachineDimControl::HandleStateChange, this, server, d));
            d->SetCallbackStates(bind(&StateMachineDimControl::HandleStates, this, server, d));
            d->SetCallbackDescriptions(bind(&StateMachineDimControl::HandleDescriptions, this, d));
            d->Subscribe(*this);
        }
    }

    // Make a copy of the list to be able to
    // lock the access to the list

    const lock_guard<mutex> guard(fMutex);
    fServerList.insert(server);

    return GetCurrentState();
}

int StateMachineDimControl::HandleServerRemove(const string &server)
{
    const lock_guard<mutex> guard(fMutex);
    fServerList.erase(server);

    return GetCurrentState();
}

vector<string> StateMachineDimControl::GetServerList()
{
    vector<string> rc;

    const lock_guard<mutex> guard(fMutex);

    rc.reserve(fServerList.size());
    for (auto it=fServerList.begin(); it!=fServerList.end(); it++)
        rc.push_back(*it);

    return rc;
}

vector<string> StateMachineDimControl::GetCommandList(const string &server)
{
    const lock_guard<mutex> guard(fMutex);

    const string  s = server.substr(0, server.length()-1);

    if (fServerList.find(s)==fServerList.end())
        return vector<string>();

    vector<string> rc;

    for (auto it=fServiceList.begin(); it!=fServiceList.end(); it++)
        if (it->iscmd && it->server==s)
            rc.push_back(server+it->service);

    return rc;
}

vector<string> StateMachineDimControl::GetCommandList()
{
    vector<string> rc;

    const lock_guard<mutex> guard(fMutex);

    for (auto it=fServiceList.begin(); it!=fServiceList.end(); it++)
        if (it->iscmd)
            rc.push_back(it->server+"/"+it->service);

    return rc;
}

set<Service> StateMachineDimControl::GetServiceList()
{
    const lock_guard<mutex> guard(fMutex);
    return fServiceList;
}

vector<State> StateMachineDimControl::GetStates(const string &server)
{
    const lock_guard<mutex> guard(fMutex);

    vector<State> rc;

    for (auto it=fStateDescriptionList.begin(); it!=fStateDescriptionList.end(); it++)
    {
        if (it->first.first!=server)
            continue;

        rc.emplace_back(it->first.second, it->second.first, it->second.second);
    }

    return rc;
}


int StateMachineDimControl::HandleAddService(const Service &svc)
{
    // Make a copy of the list to be able to
    // lock the access to the list
    const lock_guard<mutex> guard(fMutex);
    fServiceList.insert(svc);

    return GetCurrentState();
}

bool StateMachineDimControl::HasServer(const std::string &server)
{
    const lock_guard<mutex> guard(fMutex);
    return fServerList.find(server)!=fServerList.end();
}

StateMachineDimControl::StateMachineDimControl(ostream &out) : StateMachineDim(out, fIsServer?"DIM_CONTROL":"")
{
    fDim.Subscribe(*this);
    fDimList.Subscribe(*this);

    fDimList.SetCallbackServerAdd   (bind(&StateMachineDimControl::HandleServerAdd,    this, placeholders::_1));
    fDimList.SetCallbackServerRemove(bind(&StateMachineDimControl::HandleServerRemove, this, placeholders::_1));
    fDimList.SetCallbackServiceAdd  (bind(&StateMachineDimControl::HandleAddService,   this, placeholders::_1));

    // State names
    AddStateName(0, "Idle",      "No script currently in processing.");
    AddStateName(1, "Loading",   "Script is loading.");
    AddStateName(2, "Compiling", "JavaScript is compiling.");
    AddStateName(3, "Running",   "Script is running.");

    AddEvent("START", "C", 0)
        (bind(&StateMachineDimControl::StartScript, this, placeholders::_1, ".js "))
        ("Start a JavaScript");

    AddEvent("EXECUTE", "C", 0)
        (bind(&StateMachineDimControl::StartScript, this, placeholders::_1, ".x "))
        ("Execute a batch script");

    AddEvent("STOP", "C")
        (bind(&StateMachineDimControl::StopScript, this, placeholders::_1))
        ("Stop a runnning batch script or JavaScript");

    AddEvent("INTERRUPT", "C")
        (bind(&StateMachineDimControl::InterruptScript, this, placeholders::_1))
        ("Send an interrupt request (IRQ) to a running JavaScript");
}

StateMachineDimControl::~StateMachineDimControl()
{
    for (auto it=fDimDescriptionsList.begin(); it!=fDimDescriptionsList.end(); it++)
        delete *it;
}

int StateMachineDimControl::EvalOptions(Configuration &conf)
{
    fDebug = conf.Get<bool>("debug");
    fUser  = conf.Get<string>("user");
    fScriptUser = fUser;

#ifdef HAVE_NOVA
    Info("Preset observatory: "+Nova::LnLatPosn::preset()+" [PRESET_OBSERVATORY]");
#endif

    // FIXME: Check fUser for quotes!

    const map<string, string> &js = conf.GetOptions<string>("JavaScript.");
    for (auto it=js.begin(); it!=js.end(); it++)
    {
        string key = it->first;
        string val = it->second;

        // Escape key
        boost::replace_all(key, "\\", "\\\\");
        boost::replace_all(key, "'", "\\'");
        boost::replace_all(key, "\"", "\\\"");

        // Escape value
        boost::replace_all(val, "\\", "\\\\");
        boost::replace_all(val, "'", "\\'");
        boost::replace_all(val, "\"", "\\\"");

        fArgumentsJS += " '"+key +"'='"+val+"'";
    }

#if BOOST_VERSION < 104600
    const string fname = boost::filesystem::path(conf.GetName()).filename();
#else
    const string fname = boost::filesystem::path(conf.GetName()).filename().string();
#endif

    if (fname=="dimserver")
        return -1;

    if (conf.Get<bool>("stop"))
        return !Dim::SendCommand("DIM_CONTROL/STOP", fUser);

    if (conf.Has("interrupt"))
        return !Dim::SendCommand("DIM_CONTROL/INTERRUPT", conf.Get<string>("interrupt")+"\n"+fUser);

    if (conf.Has("start"))
        return !Dim::SendCommand("DIM_CONTROL/START", conf.Get<string>("start")+" user='"+fUser+"'"+fArgumentsJS);

    if (conf.Has("batch"))
        return !Dim::SendCommand("DIM_CONTROL/EXECUTE", conf.Get<string>("batch")+" user='"+fUser+"'");

    if (conf.Has("msg"))
        return !Dim::SendCommand("CHAT/MSG", fUser+": "+conf.Get<string>("msg"));

    if (conf.Has("restart"))
        return !Dim::SendCommand(conf.Get<string>("restart")+"/EXIT", uint32_t(126));

    if (conf.Get<bool>("shutdown"))
        return !Dim::SendCommand("DIS_DNS/KILL_SERVERS", uint32_t(125));

    return -1;
}
