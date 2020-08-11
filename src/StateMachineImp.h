#ifndef FACT_StateMachineImp
#define FACT_StateMachineImp

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <memory>
#include <functional> // std::function
#include <condition_variable>

#include "MainImp.h"
#include "MessageImp.h"

class Event;
class EventImp;

class StateMachineImp : public MainImp, public MessageImp
{
public:
    /// A list of default states available to any state machine.
    /// Derived classes must define different state-number for
    /// their purpose
    enum DefaultStates_t
    {
        kSM_KeepState    =    -42,  ///< 
        kSM_NotAvailable =     -2,  ///< Possible return value for GetStateIndex
        kSM_NotReady     =     -1,  ///< Mainloop not running, state machine stopped
        kSM_Ready        =      0,  ///< Mainloop running, state machine in operation
        kSM_UserMode     =      1,  ///< First user defined mode (to be used in derived classes' enums)
        kSM_Error        =  0x100,  ///< Error states should be between 0x100 and 0xffff
        kSM_FatalError   = 0xffff,  ///< Fatal error: stop program
    };

private:
    std::string fName;   /// Name of the state-machine / server (e.g. DRIVE)

    int fCurrentState;   /// Current state of the state machine

    typedef std::map<const int, std::pair<std::string, std::string>> StateNames;

protected:
    /// Human readable names associated with the states
    StateNames fStateNames;

private:
    std::vector<EventImp*> fListOfEvents; /// List of available commands as setup by user
    std::list<std::shared_ptr<Event>> fEventQueue;   /// Event queue (fifo) for the received commands

    std::mutex fMutex;    /// Mutex to ensure thread-safe access to the command fifo
    std::mutex fMutexEvt; /// Mutex to ensure thread-safe access to the command fifo

    std::condition_variable fCond; /// Conditional to signal run the an event is waiting

    bool fBufferEvents;  /// Flag if events should be buffered outside the event loop

protected:
    bool fRunning;       /// Machine is in main-loop
    int  fExitRequested; /// This is a flag which is set true if the main loop should stop

    /// Push a command into the fifo. The fifo takes over ownership
    virtual void PushEvent(Event *cmd);
    /// Pop a command from the fifo.
    std::shared_ptr<Event> PopEvent();

    bool HandleNewState(int newstate, const EventImp *evt, const char *txt);

protected:
    /// Is called continously to execute actions in the current state
    virtual int Execute() { return fCurrentState; }
    /// Is called when a configuration event is to be processed (no transition of state)
    //virtual int Configure(const Event &) { return kSM_FatalError; }
    /// Is called when a transition change event is to be processed (from one state to another) is received
    //virtual int Transition(const Event &) { return kSM_FatalError; }

private:
    virtual EventImp *CreateEvent(const std::string &name, const std::string &fmt) = 0;
    virtual EventImp *CreateService(const std::string &);

    virtual void Lock() { }
    virtual void UnLock() { }

    int Wrapper(const std::function<int(const EventImp &)> &f, const EventImp &imp)
    {
        const int rc = f(imp);
        return rc==kSM_KeepState ? GetCurrentState() : rc;
    }

protected:

    bool HandleEvent(const EventImp &evt);

    /// This is an internal function to do some action in case of
    /// a state change, like updating the corresponding service.
    virtual std::string SetCurrentState(int state, const char *txt="", const std::string &cmd="");

    EventImp &AddEvent(const std::string &name, const std::string &states, const std::string &fmt);
    EventImp &AddEvent(const std::string &name, int s1=-1, int s2=-1, int s3=-1, int s4=-1, int s5=-1);
    EventImp &AddEvent(const std::string &name, const std::string &fmt, int s1=-1, int s2=-1, int s3=-1, int s4=-1, int s5=-1);

    virtual bool AddStateName(const int state, const std::string &name, const std::string &doc="");

    void SetDefaultStateNames();

public:
    StateMachineImp(std::ostream &out=std::cout, const std::string &name="");
    ~StateMachineImp();

    std::function<int(const EventImp &)> Wrap(const std::function<int(const EventImp &)> &func)
    {
        return bind(&StateMachineImp::Wrapper, this, func, std::placeholders::_1);
    }

    const std::string &GetName() const { return fName; }

    EventImp &Subscribe(const std::string &name);
    void Unsubscribe(EventImp *evt);

    /// return the current state of the machine
    int GetCurrentState() const { return fCurrentState; }

    void SetReady()    { SetCurrentState(kSM_Ready, "set manually");    }
    void SetNotReady() { SetCurrentState(kSM_NotReady, "set manually"); }

    /// Start the mainloop
    virtual int Run(bool dummy);
    int Run() { return Run(false); }

    /// Request to stop the mainloop
    virtual void Stop(int code=0);

    /// Used to check if the main loop is already running or still running
    bool IsRunning() const { return fRunning; }

    /// Used to enable or disable buffering of events outside of the main loop
    void EnableBuffer(bool b=true) { fBufferEvents=b; }

    /// Post an event to the event queue
    bool PostEvent(std::ostream &lout, const std::string &str);
    bool PostEvent(const std::string &evt) { return PostEvent(std::cout, evt); }
    bool PostEvent(const EventImp &evt);
    bool PostEvent(const EventImp &evt, const char *ptr, size_t siz);

    // Event handling
    bool HasEvent(const EventImp *cmd);
    EventImp *FindEvent(const std::string &evt);

    bool IsQueueEmpty() const { return fEventQueue.empty(); }

    //const std::vector<EventImp*> &GetListOfEvents() const { return fListOfEvents; }
    const std::vector<std::string> GetEventNames();

    void PrintListOfEvents(std::ostream &out, const std::string &evt="");
    void PrintListOfEvents(const std::string &str="");

    void PrintListOfAllowedEvents(std::ostream &out);
    void PrintListOfAllowedEvents();

    void PrintListOfStates(std::ostream &out) const;
    void PrintListOfStates() const;


    int GetStateIndex(const std::string &name) const;
    bool HasState(int index) const;

    const std::string GetStateName(int state) const;
    const std::string GetStateName() const { return GetStateName(fCurrentState); }

    const std::string GetStateDesc(int state) const;
    const std::string GetStateDesc() const { return GetStateDesc(fCurrentState); }

    const std::string GetStateDescription(int state) const;
    const std::string GetStateDescription() const { return GetStateDescription(fCurrentState); }
};

#endif

// ***************************************************************************
/** @fn StateMachineImp::Execute()

This is what the state machine is doing in a certain state
continously. In an idle state this might just be doing nothing.

In the tracking state of the drive system this might be sending
new command values to the drive based on its current position.

The current state of the state machine can be accessed by GetCurrentState()

@returns
   Usually it should just return the current state. However, sometimes
   execution might lead to a new state, e.g. when a hardware error
   is detected. In this case a new state can be returned to put the state
   machine into a different state. Note, that the function is responsible
   of doing all actions connected with the state change itself.
   If not overwritten it returns the current status.

**/
// ***************************************************************************
/** @fn StateMachineImp::Configure(const Event &evt)

This function is called when a configuration event is to be processed.

The current state of the state machine is accessible via GetCurrentState().

The issued event and its corresponding data is accessible through
evn. (see Event and DimEvent for details) Usually such an event
will not change the state. In this case fCurrentState will be returned.
However, to allow the machine to go into an error state it is possible
to change the state by giving a different return value. When the
Configure function is called the validity of the state transition has
already been checked.

@param evt
   A reference to an Event object with the event which should
   be processed. Note that the cmd-object will get deleted after the
   function has returned.

@returns
   Usually it should just return the current state. However, sometimes
   a configuration command which was not intended to change the state
   has to change the state, e.g. to go to an error state. Return any
   other state than GetCurrentState() can put the state machine into
   a different state.  Note, that the function is responsible
   of doing all actions connected with the state change itself.
   If not overwritten it returns kSM_FatalError.

**/
// ***************************************************************************
/** @fn StateMachineImp::Transition(const Event &evt)

This function is called if a state transision was requested.

The current state of the state machine is accessible via GetCurrentState().

The new state is accessible via evt.GetTargetState().

The event and its corresponding data is accessible through evt.
(see DimCommand and DimEvent for details) If the transition was
successfull the new status should be returned. If it was unsuccessfull
either the old or any other new status will be returned.

When the Transition function is called the validity of the state
transition has already been checked.

@param evt
   A reference to an Event object with the event which should
   be processed. Note that the cmd-object will get deleted after the
   function has returned.

@returns
   Usually it should return the new state. However, sometimes
   a transition command might has to change the state to a different
   state than the one requested (e.g. an error has occured) In this
   case it is also allowed to return a different state.  Note, that the
   function is responsible of doing all actions connected with the
   state change itself.
   If not overwritten it returns kSM_FatalError.

**/
// ***************************************************************************
