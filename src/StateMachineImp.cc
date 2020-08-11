// **************************************************************************
/** @class StateMachineImp

 @brief Base class for a state machine implementation

 \dot
  digraph example {
      node [shape=record, fontname=Helvetica, fontsize=10];
      s [ label="Constructor" style="rounded"  color="red"   URL="\ref StateMachineImp::StateMachineImp"];
      a [ label="State -3 (kSM_NotReady)"      color="red"   URL="\ref StateMachineImp::StateMachineImp"];
      b [ label="State -2 (kSM_Initializing)"  color="red"   URL="\ref StateMachineImp::StateMachineImp"];
      c [ label="State -1 (kSM_Configuring)"   color="red"   URL="\ref StateMachineImp::StateMachineImp"];
      y [ label="State 0 (kSM_Ready)"                        URL="\ref StateMachineImp::Run"];
      r [ label="User states (Running)" ];
      e [ label="State 256 (kSM_Error)" ];
      f [ label="State 65535 (kSM_FatalError)" color="red"   URL="\ref StateMachineImp::Run"];

      // ---- manual means: command or program introduced ----

      // Startup from Run() to Ready
      s -> a [ arrowhead="open" color="red"  style="solid"  ]; // automatic (mandatory)
      a -> b [ arrowhead="open" color="red"  style="solid"  ]; // automatic (mandatory)
      b -> c [ arrowhead="open" color="red"  style="solid"  ]; // automatic (mandatory)

      c -> y [ arrowhead="open" color="red"  style="solid" URL="\ref StateMachineImp::Run" ]; // prg: Run()

      y -> c [ arrowhead="open" style="dashed" URL="\ref StateMachineDim::exitHandler" ]; // CMD: EXIT
      r -> c [ arrowhead="open" style="dashed" URL="\ref StateMachineDim::exitHandler" ]; // CMD: EXIT
      e -> c [ arrowhead="open" style="dashed" URL="\ref StateMachineDim::exitHandler" ]; // CMD: EXIT

      e -> y [ arrowhead="open" color="red"  style="dashed" ]; // CMD: RESET (e.g.)

      y -> e [ arrowhead="open" color="blue" style="solid"  ]; // prg
      r -> e [ arrowhead="open" color="blue" style="solid"  ]; // prg

      y -> r [ arrowhead="open" color="blue" style="dashed" ]; // CMD/PRG
      r -> y [ arrowhead="open" color="blue" style="dashed" ]; // CMD/PRG

      y -> f [ arrowhead="open" color="blue" style="solid"  ]; // prg
      r -> f [ arrowhead="open" color="blue" style="solid"  ]; // prg
      e -> f [ arrowhead="open" color="blue" style="solid"  ]; // prg
  }
  \enddot

  - <B>Red box</B>: Internal states. Events which are received are
    discarded.
  - <B>Black box</B>: State machine running. Events are accepted and
    processed according to the implemented functions Transition(),
    Configuration() and Execute(). Events are accepted accoding to the
    lookup table of allowed transitions.
  - <B>Red solid arrow</B>: A transition initiated by the program itself.
  - <b>Dashed arrows in general</b>: Transitions which can be initiated
    by a dim-command or get inistiated by the program.
  - <b>Solid arrows in general</b>: These transitions are always initiated by
    the program.
  - <B>Red dashed</B>: Suggested RESET event (should be implemented by
    the derived class)
  - <B>Black dashed arrow</B>: Exit from the main loop. This can either
    happen by the Dim-provided EXIT-command or a call to StateMachineDim::Stop.
  - <B>Black arrows</B>: Other events or transitions which can be
    implemented by the derived class.
  - <B>Dotted black arrow</B>: Exit from the main-loop which is initiated
    by the program itself through StateMachineDim::Stop() and not by the
    state machine itself (Execute(), Configure() and Transition())
  - <b>Blue dashed arrows</b>: Transitions which happen either by receiving
    a event or are initiated from the state machine itself
    (by return values of (Execute(), Configure() and Transition())
  - <b>Blue solid</b>: Transitions which cannot be initiated by dim
    event but only by the state machine itself.
  - From the program point of view the fatal error is identical with
    the kSM_Configuring state, i.e. it is returned from the main-loop.
    Usually this will result in program termination. However, depending
    on the state the program might decide to use different cleaning
    routines.

@todo
   - A proper and correct cleanup after an EXIT or Stop() is missing.
     maybe we have to force a state 0 first?
*/
// **************************************************************************
#include "StateMachineImp.h"

#include "Time.h"
#include "Event.h"

#include "WindowLog.h"
#include "Converter.h"

#include "tools.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! The state of the state machine (fCurrentState) is initialized with
//! kSM_NotReady
//!
//! Default state names for kSM_NotReady, kSM_Ready, kSM_Error and
//! kSM_FatalError are set via AddStateName.
//!
//! fExitRequested is set to 0, fRunning to false.
//!
//! Furthermore, the ostream is propagated to MessageImp, as well as
//! stored in fOut.
//!
//! MessageImp is used for messages which are distributed (e.g. via DIM),
//! fOut is used for messages which are only displayed on the local console.
//!
//! Subsequent, i.e. derived classes should setup all allowed state
//! transitions as well as all allowed configuration event by
//! AddEvent and AddStateName.
//!
//! @param out
//!    A refrence to an ostream which allows to redirect the log-output
//!    to something else than cout. The default is cout. The reference
//!    is propagated to fLog
//!
//! @param name
//!    The server name stored in fName
//!
//
StateMachineImp::StateMachineImp(ostream &out, const std::string &name)
    : MessageImp(out), fName(name), fCurrentState(kSM_NotReady),
    fBufferEvents(true), fRunning(false), fExitRequested(0)
{
    SetDefaultStateNames();
}

// --------------------------------------------------------------------------
//
//! delete all object stored in fListOfEvent and in fEventQueue
//
StateMachineImp::~StateMachineImp()
{
    // For this to work EventImp must be the first class from which
    // the object inherits
    for (vector<EventImp*>::iterator cmd=fListOfEvents.begin(); cmd!=fListOfEvents.end(); cmd++)
        delete *cmd;
}

// --------------------------------------------------------------------------
//
//! Sets the default state names. This function should be called in
//! derived classes again if they overwrite SetStateName().
//
void StateMachineImp::SetDefaultStateNames()
{
    AddStateName(kSM_NotReady,   "NotReady", "State machine not ready, events are ignored.");
    AddStateName(kSM_Ready,      "Ready",    "State machine ready to receive events.");
    AddStateName(kSM_Error,      "ERROR",    "Common error state.");
    AddStateName(kSM_FatalError, "FATAL",    "A fatal error occured, the eventloop is stopped.");
}

// --------------------------------------------------------------------------
//
//! Puts the given event into the fifo. The fifo will take over ownership.
//! Access to fEventQueue is encapsulated by fMutex.
//!
//! @param cmd
//!    Pointer to an object of type Event to be stored in the fifo
//!
//! @todo
//!    Can we also allow EventImp?
//
void StateMachineImp::PushEvent(Event *cmd)
{
    const lock_guard<mutex> guard(fMutex);
    fEventQueue.emplace_back(cmd);
    fCond.notify_one();
}

// --------------------------------------------------------------------------
//
//! Get an event from the fifo. We will take over the owenership of the
//! object. The pointer is deleted from the fifo. Access of fEventQueue
//! is encapsulated by fMutex.
//!
//! @returns
//!    A pointer to an Event object
//
shared_ptr<Event> StateMachineImp::PopEvent()
{
    const lock_guard<mutex> guard(fMutex);

    // Get the next event from the stack
    // and remove event from the stack
    const shared_ptr<Event> cmd = fEventQueue.front();
    fEventQueue.pop_front();
    return cmd;
}

// --------------------------------------------------------------------------
//
//! With this function commands are posted to the event queue. The data
//! is not given as binary data but as a string instead. It is converted
//! according to the format of the corresponding event and an event
//! is posted to the queue if successfull.
//!
//! @param lout
//!    Stream to which output should be redirected
//!    event should be for.
//!
//! @param str
//!    Command with data, e.g. "COMMAND 1 2 3 4 5 test"
//!
//! @returns
//!    false if no event was posted to the queue. If
//!    PostEvent(EventImp&,const char*, size_t) was called return its
//!    return value
//
bool StateMachineImp::PostEvent(ostream &lout, const string &str)
{
    // Find the delimiter between the command name and the data
    size_t p0 = str.find_first_of(' ');
    if (p0==string::npos)
        p0 = str.length();

    // Compile the command which will be sent to the state-machine
    const string name = fName + "/" + str.substr(0, p0);

    // Check if this command is existing at all
    EventImp *evt = FindEvent(name);
    if (!evt)
    {
        lout << kRed << "Unknown command '" << name << "'" << endl;
        return false;
    }

    // Get the format of the event data
    const string fmt = evt->GetFormat();

    // Convert the user entered data according to the format string
    // into a data block which will be attached to the event
#ifndef DEBUG
    ostringstream sout;
    const Converter conv(sout, fmt, false);
#else
    const Converter conv(lout, fmt, false);
#endif
    if (!conv)
    {
        lout << kRed << "Couldn't properly parse the format... ignored." << endl;
        return false;
    }

    try
    {
#ifdef DEBUG
        lout << kBlue << name;
#endif
        const vector<char> v = conv.GetVector(str.substr(p0));
#ifdef DEBUG
        lout << endl;
#endif

        return PostEvent(*evt, v.data(), v.size());
    }
    catch (const std::runtime_error &e)
    {
        lout << endl << kRed << e.what() << endl;
    }

    return false;
}

// --------------------------------------------------------------------------
//
//! With this function commands are posted to the event queue. If the
//! event loop has not yet been started with Run() the command is directly
//! handled by HandleEvent.
//!
//! Events posted when the state machine is in a negative state or
//! kSM_FatalError are ignored.
//!
//! A new event is created and its data contents initialized with the
//! specified memory.
//!
//! @param evt
//!    The event to be posted. The precise contents depend on what the
//!    event should be for.
//!
//! @param ptr
//!    pointer to the memory which should be attached to the event
//!
//! @param siz
//!    size of the memory which should be attached to the event
//!
//! @returns
//!    false if the event is ignored, true otherwise.
//!
//! @todo
//!    - Shell we check for the validity of a command at the current state, too?
//!    - should we also get the output stream as an argument here?
//
bool StateMachineImp::PostEvent(const EventImp &evt, const char *ptr, size_t siz)
{
    if (/*GetCurrentState()<0 ||*/ GetCurrentState()==kSM_FatalError)
    {
        Out() << kYellow << "State<0 or FatalError: Event ignored." << endl;
        return false;
    }

    if (IsRunning() || fBufferEvents)
    {
        Event *event = new Event(evt, ptr, siz);
        //Debug("Posted: "+event->GetName());
        PushEvent(event);
    }
    else
    {
        // FIXME: Is this thread safe? (Yes, because the data is copied)
        // But two handlers could be called at the same time. Do we
        // need to lock the handlers? (Dim + console)
        // FIXME: Is copying of the data necessary?
        const Event event(evt, ptr, siz);
        Lock();
        HandleEvent(event);
        UnLock();
    }
    return true;
}

// --------------------------------------------------------------------------
//
//! With this function commands are posted to the event queue. If the
//! event loop has not yet been started with Run() the command is directly
//! handled by HandleEvent.
//!
//! Events posted when the state machine is in a negative state or
//! kSM_FatalError are ignored.
//!
//! @param evt
//!    The event to be posted. The precise contents depend on what the
//!    event should be for.
//!
//! @returns
//!    false if the event is ignored, true otherwise.
//!
//! @todo
//!    - Shell we check for the validity of a command at the current state, too?
//!    - should we also get the output stream as an argument here?
//
bool StateMachineImp::PostEvent(const EventImp &evt)
{
    if (/*GetCurrentState()<0 ||*/ GetCurrentState()==kSM_FatalError)
    {
        Out() << kYellow << "State<0 or FatalError: Event ignored." << endl;
        return false;
    }

    if (IsRunning() || fBufferEvents)
        PushEvent(new Event(evt));
    else
    {
        // FIXME: Is this thread safe? (Yes, because it is only used
        // by Dim and this is thread safe) But two handlers could
        // be called at the same time. Do we need to lock the handlers?
        HandleEvent(evt);
    }
    return true;
}

// --------------------------------------------------------------------------
//
//! Return all event names of the StateMachine
//!
//! @returns
//!    A vector of strings with all event names of the state machine.
//!    The event names all have the SERVER/ pre-fix removed.
//
const vector<string> StateMachineImp::GetEventNames()
{
    vector<string> v;

    const string &name = fName + "/";
    const int     len  = name.length();

    const lock_guard<mutex> guard(fMutexEvt);

    for (vector<EventImp*>::const_iterator i=fListOfEvents.begin();
         i!=fListOfEvents.end(); i++)
    {
        const string evt = (*i)->GetName();

        v.push_back(evt.substr(0, len)==name ? evt.substr(len) : evt);
    }

    return v;
}

// --------------------------------------------------------------------------
//
//! Call for each event in fListEvents its Print function with the given
//! stream.
//!
//! @param out
//!    ostream to which the output should be redirected
//!
//! @param evt
//!    if given only the given event is selected
//
void StateMachineImp::PrintListOfEvents(ostream &out, const string &evt)
{
    const lock_guard<mutex> guard(fMutexEvt);

    for (vector<EventImp*>::const_iterator c=fListOfEvents.begin(); c!=fListOfEvents.end(); c++)
        if (evt.empty() || GetName()+'/'+evt==(*c)->GetName())
            (*c)->Print(out, true);
}

// --------------------------------------------------------------------------
//
//! Call for each event in fListEvents its Print function with the given
//! stream if it is an allowed event in the current state.
//!
//! @param out
//!    ostream to which the output should be redirected
//!
//
void StateMachineImp::PrintListOfAllowedEvents(ostream &out)
{
    const lock_guard<mutex> guard(fMutexEvt);

    for (vector<EventImp*>::const_iterator c=fListOfEvents.begin(); c!=fListOfEvents.end(); c++)
        if ((*c)->IsStateAllowed(fCurrentState))
            (*c)->Print(out, true);
}

// --------------------------------------------------------------------------
//
//! Call PrintListOfEvents with fOut as the output stream
//!
//! @param str
//!    if given only the given event is selected
//
//
void StateMachineImp::PrintListOfEvents(const string &str)
{
    PrintListOfEvents(Out(), str);
}

// --------------------------------------------------------------------------
//
//! Print a list of all states with descriptions.
//!
//! @param out
//!    ostream to which the output should be redirected
//
void StateMachineImp::PrintListOfStates(std::ostream &out) const
{
    out << endl;
    out << kBold << "List of available states:" << endl;
    for (StateNames::const_iterator i=fStateNames.begin(); i!=fStateNames.end(); i++)
    {
        ostringstream state;
        state << i->first;
        out << " -[" << kBold << state.str() << kReset << "]:" << setfill(' ') << setw(6-state.str().length()) << ' ' << kYellow << i->second.first << kBlue << " (" << i->second.second << ")" << endl;
    }
    out << endl;
}

// --------------------------------------------------------------------------
//
//! Print a list of all states with descriptions.
//
void StateMachineImp::PrintListOfStates() const
{
    PrintListOfStates(Out());
}

// --------------------------------------------------------------------------
//
//! Check whether an event (same pointer!) is in fListOfEvents
//!
//! @returns
//!    true if the event was found, false otherwise
//
bool StateMachineImp::HasEvent(const EventImp *cmd)
{
    // Find the event from the list of commands and queue it
    const lock_guard<mutex> guard(fMutexEvt);
    return find(fListOfEvents.begin(), fListOfEvents.end(), cmd)!=fListOfEvents.end();
}

// --------------------------------------------------------------------------
//
//! Check whether an event with the given name is found in fListOfEvents.
//! Note that currently there is no mechanism which ensures that not two
//! events have the same name.
//!
//! @returns
//!    true if the event was found, false otherwise
//
EventImp *StateMachineImp::FindEvent(const string &evt)
{
    // Find the command from the list of commands and queue it
    const lock_guard<mutex> guard(fMutexEvt);
    for (vector<EventImp*>::const_iterator c=fListOfEvents.begin(); c!=fListOfEvents.end(); c++)
        if (evt == (*c)->GetName())
            return *c;

    return 0;
}

// --------------------------------------------------------------------------
//
//! Calling this function, a new (named) event is added to the state
//! machine. Via a call to CreateEvent a new event is created with the
//! given targetstate, name and format. 
//!
//! The allowed states are passed to the new event and a message
//! is written to the output-stream.
//!
//! @param name
//!    The command name which should initiate the transition. The DimCommand
//!    will be constructed with the name given to the constructor and this
//!    name, e.g. "DRIVE/CHANGE_STATE_TO_NEW_STATE"
//!
//! @param states
//!    A comma sepeareted list of ints, e.g. "1, 4, 5, 9" with states
//!    in which this new state transition is allowed and will be accepted.
//!
//! @param fmt
//!    A format as defined by the dim system can be given for the command.
//!    However, it has no real meaning except that it is stored within the
//!    DimCommand object. However, the user must make sure that the data of
//!    received commands is properly extracted. No check is done.
//
EventImp &StateMachineImp::AddEvent(const string &name, const string &states, const string &fmt)
{
    EventImp *evt = CreateEvent(name, fmt);

    evt->AddAllowedStates(states);

#ifdef DEBUG
    Out() << ":   " << Time().GetAsStr("%H:%M:%S.%f");
    Out() << " - Adding command " << evt->GetName();
    Out() << endl;
#endif

    const lock_guard<mutex> guard(fMutexEvt);
    fListOfEvents.push_back(evt);
    return *evt;
}

// --------------------------------------------------------------------------
//
//! Calling this function, a new (named) event is added to the state
//! machine. Therefore an instance of type DimEvent is created and added
//! to the list of available commands fListOfEvents.
//!
//! @param name
//!    The command name which should initiate the transition. The DimCommand
//!    will be constructed with the name given to the constructor and this
//!    name, e.g. "DRIVE/CHANGE_STATE_TO_NEW_STATE"
//!
//! @param s1, s2, s3, s4, s5
//!    A list of states from which a transition to targetstate is allowed
//!    by this command.
//
EventImp &StateMachineImp::AddEvent(const string &name, int s1, int s2, int s3, int s4, int s5)
{
    ostringstream str;
    str << s1 << ' '  << s2 << ' ' << s3 << ' ' << s4 << ' ' << s5;
    return AddEvent(name, str.str(), "");
}

// --------------------------------------------------------------------------
//
//! Calling this function, a new (named) event is added to the state
//! machine. Therefore an instance of type DimEvent is created and added
//! to the list of available commands fListOfEvents.
//!
//! @param name
//!    The command name which should initiate the transition. The DimCommand
//!    will be constructed with the name given to the constructor and this
//!    name, e.g. "DRIVE/CHANGE_STATE_TO_NEW_STATE"
//!
//! @param fmt
//!    A format as defined by the dim system can be given for the command.
//!    However, it has no real meaning except that it is stored within the
//!    DimCommand object. However, the user must make sure that the data of
//!    received commands is properly extracted. No check is done.
//!
//! @param s1, s2, s3, s4, s5
//!    A list of states from which a transition to targetstate is allowed
//!    by this command.
//
EventImp &StateMachineImp::AddEvent(const string &name, const string &fmt, int s1, int s2, int s3, int s4, int s5)
{
    ostringstream str;
    str << s1 << ' '  << s2 << ' ' << s3 << ' ' << s4 << ' ' << s5;
    return AddEvent(name, str.str(), fmt);
}

EventImp *StateMachineImp::CreateService(const string &)
{
    return new EventImp();
}

// --------------------------------------------------------------------------
//
EventImp &StateMachineImp::Subscribe(const string &name)
{
    EventImp *evt = CreateService(name);

    const lock_guard<mutex> guard(fMutexEvt);
    fListOfEvents.push_back(evt);
    return *evt;
}

void StateMachineImp::Unsubscribe(EventImp *evt)
{
    {
        const lock_guard<mutex> guard(fMutexEvt);

        auto it = find(fListOfEvents.begin(), fListOfEvents.end(), evt);
        if (it==fListOfEvents.end())
            return;

        fListOfEvents.erase(it);
    }
    delete evt;
}

// --------------------------------------------------------------------------
//
//! To be able to name states, i.e. present the current state in human
//! readable for to the user, a string can be assigned to each state.
//! For each state this function can be called only once, i.e. state name
//! cannot be overwritten.
//!
//! Be aware that two states should not have the same name!
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
bool StateMachineImp::AddStateName(const int state, const std::string &name, const std::string &doc)
{
    //auto it = fStateNames.find(state);

    //if (/*it!=fStateNames.end() &&*/ !it->second.first.empty())
    //    return false;

    fStateNames[state] = make_pair(name, doc);
    return true;
}

// --------------------------------------------------------------------------
//
//! Get a state's index by its name.
//!
//! @param name
//!    Name of the state to search for
//!
//! @returns
//!    Index of the state if found, kSM_NotAvailable otherwise
//!
int StateMachineImp::GetStateIndex(const string &name) const
{
    for (auto it=fStateNames.begin(); it!=fStateNames.end(); it++)
        if (it->second.first==name)
            return it->first;

    return kSM_NotAvailable;
}

// --------------------------------------------------------------------------
//
//! @param state
//!    The state for which the name should be returned.
//!
//! @returns
//!    The state name as stored in fStateNames is returned, corresponding
//!    to the state given. If no name exists the number is returned
//!    as string.
//!
const string StateMachineImp::GetStateName(int state) const
{
    const StateNames::const_iterator i = fStateNames.find(state);
    return i==fStateNames.end() || i->second.first.empty() ? to_string(state) : i->second.first;
}

// --------------------------------------------------------------------------
//
//! @param state
//!    The state for which should be checked
//!
//! @returns
//!    true if a nam for this state already exists, false otherwise
//!
bool StateMachineImp::HasState(int state) const
{
    return fStateNames.find(state) != fStateNames.end();
}

// --------------------------------------------------------------------------
//
//! @param state
//!    The state for which the name should be returned.
//!
//! @returns
//!    The description of a state name as stored in fStateNames is returned,
//!    corresponding to the state given. If no name exists an empty string is
//!    returned.
//!
const string StateMachineImp::GetStateDesc(int state) const
{
    const StateNames::const_iterator i = fStateNames.find(state);
    return i==fStateNames.end() ? "" : i->second.second;
}

// --------------------------------------------------------------------------
//
//! This functions works in analogy to GetStateName, but the state number
//! is added in []-parenthesis after the state name if it is available.
//!
//! @param state
//!    The state for which the name should be returned.
//!
//! @returns
//!    The state name as stored in fStateName is returned corresponding
//!    to the state given plus the state number added in []-parenthesis.
//!    If no name exists the number is returned as string.
//!
//
const string StateMachineImp::GetStateDescription(int state) const
{
    const string &str = GetStateName(state);

    ostringstream s;
    s << state;
    if (str==s.str())
        return str;

    return str.empty() ? s.str() : (str+'['+s.str()+']');
}

// --------------------------------------------------------------------------
//
//! This function is a helpter function to do all the corresponding action
//! if the state machine decides to change its state.
//!
//! If state is equal to the current state (fCurrentState) nothing is done.
//! Then the service STATE (fSrcState) is updated with the new state
//! and the text message and updateService() is called to distribute
//! the update to all clients.
//!
//! In addition a log message is created and set via UpdateMsg.
//!
//! @param state
//!    The new state which should be applied
//!
//! @param txt
//!    A text corresponding to the state change which is distributed
//!    together with the state itself for convinience.
//!
//! @param cmd
//!    This argument can be used to give an additional name of the function
//!    which is reponsible for the state change. It will be included in the
//!    message
//!
//! @return
//!    return the new state which was set or -1 in case of no change
//
string StateMachineImp::SetCurrentState(int state, const char *txt, const std::string &cmd)
{
    if (state==fCurrentState)
    {
        Out() << " -- " << Time().GetAsStr("%H:%M:%S.%f") << " - State " << GetStateDescription(state) << " already set... ";
        if (!cmd.empty())
            Out() << "'" << cmd << "' ignored.";
        Out() << endl;
        return "";
    }

    const int old = fCurrentState;

    const string nold = GetStateDescription(old);
    const string nnew = GetStateDescription(state);

    string msg = nnew + " " + txt;
    if (!cmd.empty())
        msg += " (" + cmd + ")";

    fCurrentState = state;

    // State might have changed already again...
    // Not very likely, but possible. That's why state is used
    // instead of fCurrentState.

    ostringstream str;
    str << "State Transition from " << nold << " to " << nnew << " (" << txt;
    if (!cmd.empty())
        str << ": " << cmd;
    str << ")";
    Message(str);

    return msg;
}

// --------------------------------------------------------------------------
//
//! This function handles a new state issued by one of the event handlers.
//!
//! @param newstate
//!    A possible new state
//!
//! @param evt
//!    A pointer to the event which was responsible for the state change,
//!    NULL if no event was responsible.
//!
//! @param txt
//!    Text which is issued if the current state has changed and the new
//!    state is identical to the target state as stored in the event
//!    reference, or when no alternative text was given, or the pointer to
//!    evt is NULL.
//!
//! @param alt
//!    An alternative text which is issues when the newstate of a state change
//!    doesn't match the expected target state.
//!
//! @returns
//!    false if newstate is kSM_FatalError, true otherwise
//
bool StateMachineImp::HandleNewState(int newstate, const EventImp *evt,
                                     const char *txt)
{
    if (newstate==kSM_FatalError)
        return false;

    if (newstate==fCurrentState || newstate==kSM_KeepState)
        return true;

    SetCurrentState(newstate, txt, evt ? evt->GetName() : "");

    return true;
}

// --------------------------------------------------------------------------
//
//! This is the event handler. Depending on the type of event it calles
//! the function associated with the event, the Transition() or
//! Configure() function.
//!
//! It first checks if the given even is valid in the current state. If
//! it is not valid the function returns with true.
//!
//! If it is valid, it is checked whether a function is associated with
//! the event. If this is the case, evt.Exec() is called and HandleNewState
//! called with its return value. 
//!
//! If the event's target state is negative (unnamed Event) the Configure()
//! function is called with the event as argument and HandleNewState with
//! its returned new state.
//!
//! If the event's target state is 0 or positive (named Event) the
//! Transition() function is called with the event as argument and
//! HandleNewState with its returned new state.
//!
//! In all three cases the return value of HandleNewState is returned.
//!
//! Any of the three commands should usually return the current state
//! or (in case of the Transition() command) return the new state. However,
//! all three command can issue a state change by returning a new state.
//! However, this will just change the internal state. Any action which
//! is connected with the state change must have been executed already.
//!
//! @param evt
//!    a reference to the event which should be handled
//!
//! @returns
//!    false in case one of the commands changed the state to kSM_FataError,
//!    true otherwise
//
bool StateMachineImp::HandleEvent(const EventImp &evt)
{
    if (!evt.HasFunc())
    {
        Warn(evt.GetName()+": No function assigned... ignored.");
        return true;

    }

#ifdef DEBUG
    ostringstream out;
    out << "Handle: " << evt.GetName() << "[" << evt.GetSize() << "]";
    Debug(out);
#endif

    // Check if the received command is allow in the current state
    if (!evt.IsStateAllowed(fCurrentState))
    {
        Warn(evt.GetName()+": Not allowed in state "+GetStateDescription()+"... ignored.");
        return true;
    }

    return HandleNewState(evt.ExecFunc(), &evt,
                          "by ExecFunc function-call");
}

// --------------------------------------------------------------------------
//
//! This is the main loop, or what could be called the running state
//! machine. The flow diagram below shows what the loop is actually doing.
//! It's main purpose is to serialize command excecution and the main
//! loop in the state machine (e.g. the tracking loop)
//!
//! Leaving the loop can be forced by setting fExitRequested to another
//! value than zero. This is done automatically if dim's EXIT command
//! is received or can be forced by calling Stop().
//!
//! As long as no new command arrives the Execute() command is called
//! continously. This should implement the current action which
//! should be performed in the current state, e.g. calculating a
//! new command value and sending it to the hardware.
//!
//! If a command is received it is put into the fifo by the commandHandler().
//! The main loop now checks the fifo. If commands are in the fifo, it is
//! checked whether the command is valid ithin this state or not. If it is
//! not valid it is ignored. If it is valid the corresponding action
//! is performed. This can either be a call to Configure() (when no state
//! change is connected to the command) or Transition() (if the command
//! involves a state change).
//! In both cases areference to the received command (Command) is
//! passed to the function. Note that after the functions have finished
//! the command will go out of scope and be deleted.
//!
//! None of the commands should take to long for execution. Otherwise the
//! response time of the main loop will become too slow.
//!
//! Any of the three commands should usually return the current state
//! or (in case of the Transition() command) return the new state. However,
//! all three command can issue a state change by returning a new state.
//! However, this will just change the internal state. Any action which
//! is connected with the state change must have been executed already.
//!
//!
//!
//!  \dot
//!   digraph Run {
//!       node  [ shape=record, fontname=Helvetica, fontsize=10 ];
//!       edge  [ labelfontname=Helvetica, labelfontsize=8 ];
//!       start0 [ label="Run()" style="rounded"];
//!       start1 [ label="fExitRequested=0\nfRunning=true\nSetCurrentState(kSM_Ready)"];
//!       cond1  [ label="Is fExitRequested==0?"];
//!       exec   [ label="HandleNewState(Execute())"];
//!       fifo   [ label="Any event in FIFO?"];
//!       get    [ label="Get event from FIFO\n Is event allowed within the current state?" ];
//!       handle [ label="HandleEvent()" ];
//!       exit1  [ label="fRunning=false\nSetCurrentState(kSM_FatalError)\n return -1" style="rounded"];
//!       exit2  [ label="fRunning=false\nSetCurrentState(kSM_NotReady)\n return fExitRequested-1" style="rounded"];
//!
//!       start0   -> start1   [ weight=8 ];
//!       start1   -> cond1    [ weight=8 ];
//!
//!       cond1:e  -> exit2:n  [ taillabel="true"  ];
//!       cond1    -> exec     [ taillabel="false"  weight=8 ];
//!
//!       exec     -> fifo     [ taillabel="true"   weight=8 ];
//!       exec:e   -> exit1:e  [ taillabel="false" ];
//!
//!       fifo     -> cond1    [ taillabel="false" ];
//!       fifo     -> get      [ taillabel="true"   weight=8 ];
//!
//!       get      -> handle   [ taillabel="true"  ];
//!
//!       handle:s -> exit1:n  [ taillabel="false"  weight=8 ];
//!       handle   -> cond1    [ taillabel="true"  ];
//!   }
//!   \enddot
//!
//! @param dummy
//!    If this parameter is set to treu then no action is executed
//!    and now events are dispatched from the event list. It is usefull
//!    if functions are assigned directly to any event to simulate
//!    a running loop (e.g. block until Stop() was called or fExitRequested
//!    was set by an EXIT command.  If dummy==true, fRunning is not set
//!    to true to allow handling events directly from the event handler.
//!
//! @returns
//!    In the case of a a fatal error -1 is returned, fExitRequested-1 in all
//!    other cases (This corresponds to the exit code either received by the
//!    EXIT event or given to the Stop() function)
//!
//! @todo  Fix docu (kSM_SetReady, HandleEvent)
//
int StateMachineImp::Run(bool dummy)
{
    if (fCurrentState>=kSM_Ready)
    {
        Error("Run() can only be called in the NotReady state.");
        return -1;
    }

    if (!fExitRequested)
    {
        fRunning = !dummy;

        SetCurrentState(kSM_Ready, "by Run()");

        std::unique_lock<std::mutex> lock(fMutex);
        fMutex.unlock();

        while (1)
        {
            fMutex.lock();
            if (IsQueueEmpty())
                fCond.wait_for(lock, chrono::microseconds(10000));
            fMutex.unlock();

            if (fExitRequested)
                break;

            if (dummy)
                continue;

            // If the command stack is empty go on with processing in the
            // current state
            if (!IsQueueEmpty())
            {
                // Pop the next command which arrived from the stack
                const shared_ptr<Event> cmd(PopEvent());
                if (!HandleEvent(*cmd))
                    break;
            }

            // Execute a step in the current state of the state machine
            if (!HandleNewState(Execute(), 0, "by Execute-command"))
                break;
        }

        fRunning = false;

        if (!fExitRequested)
        {
            Fatal("Fatal Error occured... shutting down.");
            return -1;
        }

        SetCurrentState(kSM_NotReady, "due to return from Run().");
    }

    const int exitcode = fExitRequested-1;

    // Prepare for next call
    fExitRequested = 0;

    return exitcode;
}

// --------------------------------------------------------------------------
//
//! This function can be called to stop the loop of a running state machine.
//! Run() will then return with a return value corresponding to the value
//! given as argument.
//!
//! Note that this is a dangerous operation, because as soon as one of the
//! three state machine commands returns (Execute(), Configure() and
//! Transition()) the loop will be left and Run(9 will return. The program
//! is then responsible of correctly cleaning up the mess which might be left
//! behind.
//!
//! @param code
//!    int with which Run() should return when returning.
//
void StateMachineImp::Stop(int code)
{
    fExitRequested = code+1;
}
