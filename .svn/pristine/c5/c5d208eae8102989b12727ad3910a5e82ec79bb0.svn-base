// **************************************************************************
/** @class Event

@brief Concerete implementation of an EventImp stroring name, format, data and time

This is the implementation of an event which can be posted to a state
machine, hosting all the data itself. In addition to the base class
it has storage for name, format, the data and a time stamp.

*/
// **************************************************************************
#include "Event.h"

#include <iostream>

#include "Time.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Store the name in fName and the format in fFormat. Initializes fTime
//! to an invalid time.
//!
//! @param name
//!    Name given to the event
//!
//! @param fmt
//!    If the event has data attached (like arguments of commands)
//!    the format can be given here. How the format string is defined
//!    is defined within the dim libarary. It is used in console
//!    and shell access to properly format the data sent with the event
//!
//! <B>From the dim manual:</B>
//!    The format parameter specifies the contents of the structure in
//!    the form T:N[;T:N]*[;T] where T is the item type: (I)nteger,
//!    (C)haracter, (L)ong, (S)hort, (F)loat, (D)ouble, and N is the
//!    number of such items. The type alone at the end means all
//!    following items are of the same type. Example: "I:3;F:2;C" means
//!    3 Integers, 2 Floats and Characters until the end. The format
//!    parameter is used for communicating between different platforms.
//!
//
Event::Event(const string &name, const string &fmt) :
    fName(name), fFormat(fmt), fTime(Time::none), fQoS(0), fEmpty(true)
{
}

// --------------------------------------------------------------------------
//
//! Copies the all contents from an EventImp. Note, that also the data
//! area is copied. If the name contains a slash ('/') everything before
//! the slash is removed.
//!
//! @param evt
//!    Reference to an object of type EventImp.
//
Event::Event(const EventImp &evt) : EventImp(evt),
fName(evt.GetName()), fFormat(evt.GetFormat()),
fData(evt.GetText(), evt.GetText()+evt.GetSize()), fTime(evt.GetTime()),
fQoS(evt.GetQoS()), fEmpty(evt.IsEmpty())
{
    const size_t pos = fName.find_first_of('/');
    if (pos!=string::npos)
        fName = fName.substr(pos+1);
}

// --------------------------------------------------------------------------
//
//! Copies the all contents from an EventImp. fData is initialized from the
//! given memory. If the name contains a slash ('/') everything before
//! the slash is removed.
//!
//! @param evt
//!    Reference to an object of type EventImp.
//!
//! @param ptr
//!    Pointer to the memory region to be copied.
//!
//! @param siz
//!    Size of the memory region to be copied.
//
Event::Event(const EventImp &evt, const char *ptr, size_t siz) : EventImp(evt),
fName(evt.GetName()), fFormat(evt.GetFormat()),
fData(ptr, ptr+siz), fTime(evt.GetTime()), fQoS(evt.GetQoS()), fEmpty(ptr==0)
{
    const size_t pos = fName.find_first_of('/');
    if (pos!=string::npos)
        fName = fName.substr(pos+1);
}
