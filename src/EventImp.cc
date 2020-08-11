// **************************************************************************
/** @class EventImp

@brief A general base-class describing events issues in a state machine

@section General purpose

The general purpose of this class is to describe an event which can
occur in one of our StateMachines. It provides pointers to data
associated with the event, a target state and stores the states in which
issuing this event is allowed. The target state might be negative to
describe that no transition of the state is requested.

Such an event canjust be a description of an event, but can also be
an issued event which already contain data.

The format can, but need not, contain the format of the data area.
As a rule, it should follow the format also used in the DIM network.


@section Assigning functions to an event

To any event a function call can be assigned. Thanks to boost::bind there
are various and different very powerful ways to do that. 

The function assigned with AssignFunction must return int. When it is
executed it is given a const reference of the current event as an argument,
i.e. if you want to get such a reference in your function, you can reference
it using the placeholder _1. (Remark: it is allowe to omit the _1 placeholder
if no reference to the EventImp object is needed)

A few examples:

\code
   int function(const EventImp &evt, int i, const char *txt) { return i; }

   EventImp evt;
   evt.AssignFunction(boost::bind(function, _1, 7, "hallo"));
   cout << evt.Exec() << endl;
   // 7
\endcode

When the function is executed later via ExecFunc() in will get a reference
to the executing EventImp as its first argument (indicated by '_1'), it will
get 7 and "hallo" as second and third argument.

\code
   int function(int i, const char *txt, const EventImp &evt) { return i; }

   EventImp evt;
   evt.AssignFunction(boost::bind(function, 7, "hallo", _1));
   cout << evt.Exec() << endl;
   // 7
\endcode

Is the same example than the one above, but the arguments are in a different
order.

\code
   class A
   {
      int function(const EventImp &evt, int i, const char *txt)
      {
         cout << this << endl; return i;
      }
   };

   A a;

   EventImp evt;
   evt.AssignFunction(boost::bind(&A::function, &a, _1, 7, "hallo"));
   cout << evt.Exec() << endl;
   // &a
   // 7
\endcode

The advanatge of boost::bind is that it also works for member functions
of classes. In this case the first argument after the function-pointer
\b must be a pointer to a valid class-object. This can also be \em this
if called from within a class object.

Also note that everything (as usual) which is not a reference is copied
when the bind function is invoked. If you want to distribute a reference
instead use ref(something), like

\code
   int function(int &i)  { return i; }

   int j = 5;
   EventImp evt;
   evt.AssignFunction(bind(function, ref(j));
   j = 7;
   cout << evt.Exec() << endl;
   // 7
\endcode

Note, that you are responsible for the validity, that means: Do not
destroy your object (eg. reference to j) while bind might still be called
later, or a pointer to \em this.

@section References
   - <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/bind/bind.html">boost::bind (V1.45.0)</A>

@todo
   Add link to DIM format

*/
// **************************************************************************
#include "EventImp.h"

#include <sstream>

#include "Time.h"
#include "WindowLog.h"
#include "Description.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Copy the contents of an EventImp (fTargetState, fAllowedStates and
//!  fFunction)
//
EventImp::EventImp(const EventImp &cmd) : fAllowedStates(cmd.fAllowedStates),
    fFunction(cmd.fFunction)
{
}

// --------------------------------------------------------------------------
//
//! If the state is 0 or positive add it to fAllowedStates
//!
//! @param state
//!     The state which should be added
//
void EventImp::AddAllowedState(int state)
{
    if (state>=0)
        fAllowedStates.push_back(state);
}

// --------------------------------------------------------------------------
//
//! Add all states in the string to fAllowedStates.
//! 
//! @param states
//!    A string containing the states. They can either be separated by
//!    whitespaces or commas, e.g. "1 2 3 4" or "1, 7, 9, 10". Note that
//!    no real consistency check is done.
//
void EventImp::AddAllowedStates(const string &states)
{
    stringstream stream(states);

    const bool sep = stream.str().find(',')==string::npos;

    string buffer;
    while (getline(stream, buffer, sep ? ' ' : ','))
        AddAllowedState(stoi(buffer));
}

// --------------------------------------------------------------------------
//
//! Return whether the given state is in the list of allowed states.
//! 
//! @param state
//!    The state to look for in fAllowedStates
//!
//! @returns
//!    If the given state is negative returns false. If the list of allowed
//!    states is empty return true. Otherwise return whether the state
//!    is found in fAllowedList or not.
//
bool EventImp::IsStateAllowed(int state) const
{
    // States with negative values are internal states and are
    // never allowed
    // if (state<0)
    //    return false;

    // In case no allowed state is explicitly set
    // all positive states are allowed
    if (fAllowedStates.size()==0)
        return true;

    return find(fAllowedStates.begin(), fAllowedStates.end(), state)!=fAllowedStates.end();
}

// --------------------------------------------------------------------------
//
//! @returns the event data converted to a std::string. Trailing redundant
//!          \0's are removed.
//!
string EventImp::GetString() const
{
    if (GetSize()==0)
        return std::string();

    size_t s = GetSize()-1;
    while (s>0 && GetText()[s]==0)
        s--;

    return std::string(GetText(), s+1);
}

// --------------------------------------------------------------------------
//
//! Print the contents of the event to the given stream.
//!
//! @param out
//!    An ostream to which the output should be redirected.
//!
//! @param strip
//!    defines whether a possible SERVER name in the event name
//!    should be stripped or not.
//!
void EventImp::Print(ostream &out, bool strip) const
{
    if (GetDescription().empty())
        return;

    out << " -";

    const string str = GetName();
    if (!str.empty())
        out << kBold << str.substr(strip?str.find_first_of('/')+1:0) << kReset << "-";

    const string fmt = GetFormat();

    if (!str.empty() && !fmt.empty())
        out << " ";

    if (!fmt.empty())
        out << "[" << fmt << "]";

    vector<Description> v = Description::SplitDescription(GetDescription());

    if (!GetDescription().empty())
    {
        out << kBold;
        for (vector<Description>::const_iterator j=v.begin()+1;
             j!=v.end(); j++)
            out << " <" << j->name << ">";
        out << kReset;
    }

    for (unsigned int i=0; i<fAllowedStates.size(); i++)
        out << " " << fAllowedStates[i];

    const Time tm = GetTime();

    const bool t = tm!=Time::None && tm!=Time(1970,1,1);
    const bool s = GetSize()>0;

    if (s || t)
        out << "(";
    if (t)
        out << tm.GetAsStr("%H:%M:%S.%f");
    if (s && t)
        out << "/";
    if (s)
        out << "size=" << GetSize();
    if (s || t)
        out << ")";
    out << endl;

    if (GetDescription().empty())
    {
        out << endl;
        return;
    }

    out << "     " << v[0].comment << endl;

    for (vector<Description>::const_iterator j=v.begin()+1;
         j!=v.end(); j++)
    {
        out << "      ||" << kGreen << j->name;
        if (!j->comment.empty())
            out << kReset << ": " << kBlue << j->comment;
        if (!j->unit.empty())
            out << kYellow << " [" << j->unit << "]";
        out << endl;
    }
    out << endl;
}

// --------------------------------------------------------------------------
//
//! Calls Print(std::cout)
//!
//! @param strip
//!    defines whether a possible SERVER name in the event name
//!    should be stripped or not.
//
void EventImp::Print(bool strip) const
{
    Print(cout, strip);
}

string EventImp::GetTimeAsStr(const char *fmt) const
{
    return GetTime().GetAsStr(fmt);
}

uint64_t EventImp::GetJavaDate() const
{
    return GetTime().JavaDate();
}
