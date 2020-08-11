// **************************************************************************
/** @class StateMachineDim

This class implements a StateMachine within a Dim network. It redirects
all output posted via MessageImp to a service called "NAME/MESSAGE"
while name is the name of the machine given in the constructor. In
addition two services are offered: NAME/STATE and NAME/VERSION.
NAME/STATE propagates any state change to the netork.

When constructing the Dim network is started and while dstruction it is
stopped.

@todo
    Proper support for versioning

*/
// **************************************************************************
#include "StateMachineDim.h"

#include "tools.h"

#include "EventDim.h"
#include "ServiceDim.h"

using namespace std;

const int StateMachineDim::fVersion = 42;

// --------------------------------------------------------------------------
//
//! The constrcutor first initialized DimStart with the given machine name.
//! DimStart is just a wrapper which constructor calls DimServer::start()
//! to ensure that initializing the Dim sub-system, is the first what is
//! done.
//!
//! The second objet instantiated is the MessageDim class which offers
//! the MESSAGE service used to broadcast logging messages to other
//! Dim clients.
//!
//! After this the services STATE and VERSION are setup. STATE will
//! be used to broadcast the state of the machine. Version broadcasts
//! the global version number of the StateMachineDim implementation
//!
//! After redirecting the handler which handels dim's EXIT command
//! to ourself (it will then call StateMachineDim::exitHandler) and
//! adding human readable state names for the default states
//! implemented by StateMachingImp the state is set to kSM_Initializing.
//! Warning: The EXIT handler is global!
//!
//! @param name
//!    The name with which the dim-services should be prefixed, e.g.
//!    "DRIVE" will lead to "DRIVE/SERVICE". It is also propagated
//!    to DimServer::start()
//!
//! @param out
//!    A refrence to an ostream which allows to redirect the log-output
//!    to something else than cout. The default is cout. The reference
//!    is propagated to fLog
//!
//! @todo
//!    - Shell the VERSION be set from the derived class?
//
StateMachineDim::StateMachineDim(ostream &out, const std::string &name)
    : DimLog(out, name), DimStart(name, DimLog::fLog), StateMachineImp(out, name),
    fDescriptionStates(name+"/STATE_LIST", "C",
                       "Provides a list with descriptions for each service."
                       "|StateList[string]:A \\n separated list of the form id:name=description"),
    fSrvState(name+"/STATE", "C",
              "Provides the state of the state machine as quality of service."
              "|Text[string]:A human readable string sent by the last state change.")
    //    fSrvVersion((name+"/VERSION").c_str(), const_cast<int&>(fVersion)),
{
    SetDefaultStateNames();
}

// --------------------------------------------------------------------------
//
//! Overwrite StateMachineImp::AddTransition to create a EventDim
//! instead of an Event object. The event name propagated to the EventDim
//! is fName+"/"+name.
//!
//! For parameter description see StateMachineImp.
//!
EventImp *StateMachineDim::CreateEvent(const string &name, const string &fmt)
{
    return new EventDim(GetName()+"/"+name, fmt, this);
}

EventImp *StateMachineDim::CreateService(const string &name)
{
    return new ServiceDim(name, this);
}

// --------------------------------------------------------------------------
//
//! Overwrite StateMachineImp::AddStateName. In addition to storing the
//! state locally it is also propagated through Dim in the STATE_LIST
//! service. 
//!
//! @param state
//!    Number of the state to which a name should be assigned
//!
//! @param name
//!    A name which should be assigned to the state, e.g. "Tracking"
//!
//! @param doc
//!    A explanatory text describing the state
//!
bool StateMachineDim::AddStateName(const int state, const std::string &name, const std::string &doc)
{
    if (name.empty())
        return false;

    const bool rc = HasState(state) || GetStateIndex(name)!=kSM_NotAvailable;

    StateMachineImp::AddStateName(state, name, doc);

    string str;
    for (auto it=fStateNames.begin(); it!=fStateNames.end(); it++)
        str += to_string(it->first)+':'+it->second.first+'='+it->second.second+'\n';

    fDescriptionStates.Update(str);

    return rc;
}

// --------------------------------------------------------------------------
//
//! Overwrite StateMachineImp::SetCurrentState. In addition to
//! calling StateMachineImo::SetCurrentState the new state is also
//! distributed via the DimService STATE.
//!
//! For parameter description see StateMachineImp.
//!
string StateMachineDim::SetCurrentState(int state, const char *txt, const std::string &cmd)
{
    const string msg = StateMachineImp::SetCurrentState(state, txt, cmd);
    if (msg.empty())
        return "";

    fSrvState.setQuality(state);
    fSrvState.Update(msg);

    return msg;
}

// --------------------------------------------------------------------------
//
//! In the case of dim this secures HandleEvent against dim's commandHandler
//!
void StateMachineDim::Lock()
{
    dim_lock();
}

// --------------------------------------------------------------------------
//
//! In the case of dim this secures HandleEvent against dim's commandHandler
//!
void StateMachineDim::UnLock()
{
    dim_unlock();
}

void StateMachineDim::infoHandler()
{
    DimInfo *inf = getInfo();
    if (!inf)
        return;

    const EventImp *evt = dynamic_cast<EventImp*>(inf);

    if (HasEvent(evt))
        PostEvent(*evt);
}

// --------------------------------------------------------------------------
//
//! Overwritten DimCommand::commandHandler()
//!
//! If fCurrentState is smaller than 0 or we are in kSM_FatalError state,
//! all incoming commands are ignored.
//!
//! The commandHandler will go through the list of available commands
//! (fListOfEventss). If the received command was recognized, it is added
//! via PushCommand into the fifo.
//!
//! @todo
//!    - Fix the exit when cmd is not of type EventImp
//!    - Fix docu
//!    - Do we need a possibility to suppress a call to "HandleEvent"
//!      or is a state<0 enough?
//
void StateMachineDim::commandHandler()
{
    DimCommand *cmd = getCommand();
    if (!cmd)
        return;

    const EventImp *evt = dynamic_cast<EventImp*>(cmd);

    if (HasEvent(evt))
        PostEvent(*evt);
}

// --------------------------------------------------------------------------
//
//! Overwrites MessageImp::Update. This redirects output issued via
//! MessageImp to MessageDim object.
//
int StateMachineDim::Write(const Time &time, const string &txt, int qos)
{
    return DimLog::fLog.Write(time, txt, qos);
}

// --------------------------------------------------------------------------
//
//! exitHandler of the DimServer. The EXIT command is implemented by each
//! DimServer automatically. exitHandler calls Stop(code) and exit(-1)
//! in case the received exit-value is a special number (42). abort()
//! is called if 0x42 is received.
//!
//! @param code
//!    value which is passed to Stop(code)
//
void StateMachineDim::exitHandler(int code)
{
    Out() << " -- " << Time().GetAsStr() << " - EXIT(" << code << ") command received." << endl;
    if (code<0) // negative values reserved for internal use
    {
        Out() << " -- " << Time().GetAsStr() << ": ignored." << endl;
        return;
    }

    Stop(code);
    if (code==42)
        exit(128);
    if (code==0x42)
        abort();
}
