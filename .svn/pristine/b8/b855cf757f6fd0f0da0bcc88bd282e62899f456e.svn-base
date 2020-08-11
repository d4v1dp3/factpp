// **************************************************************************
/** @class StateMachine

 @brief Class for a state machine implementation just on the console

This class implements a StateMachine which is to be controlled from the
console. It redirects all output posted via MessageImp to the console
and, if the stream is a WindowLog, adds colors.

When constructing the Dim network is started and while dstruction it is
stopped.

@todo
   Do we really need to create Event-objects? Shouldn't we add a
   ProcessEvent function which takes an event as argument instead?
   Or something else which easily allows to add data to the events?

*/
// **************************************************************************
#include "StateMachine.h"

#include "WindowLog.h"

#include "Event.h"
#include "Time.h"
#include "Shell.h"

#include "tools.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Overwrite StateMachineImp::AddTransition to create an Event object
//! instead of an EventImp object. The event name propagated is name.
//!
//! For parameter description see StateMachineImp.
//!
EventImp *StateMachine::CreateEvent(const string &name, const string &fmt)
{
    return new Event(GetName()+'/'+name, fmt);
}

// --------------------------------------------------------------------------
//
//! This is (analog to StateMachineDim::commandHandler()) the function which
//! is called if an event from the console is received. It then is
//! supposed to send the event to the messge queue or handle it directly
//! if the machine was not yet started.
//!
//! If fCurrentState is smaller than 0 or we are in kSM_FatalError state,
//! all incoming commands are ignored.
//!
//! The commandHandler will go through the list of available commands
//! (fListOfEventss). If the received command was recognized, it is added
//! via PostCommand into the fifo.
//!
//! @todo
//!    - Fix the exit when cmd is not of type EventImp
//!    - Do we need a possibility to suppress a call to "HandleEvent"
//!      or is a state<0 enough?
//
bool StateMachine::ProcessCommand(const std::string &str, const char *ptr, size_t siz)
{
    EventImp *evt = FindEvent(str);
    if (!evt)
        return false;

    PostEvent(*evt, ptr, siz);
    return true;
}

