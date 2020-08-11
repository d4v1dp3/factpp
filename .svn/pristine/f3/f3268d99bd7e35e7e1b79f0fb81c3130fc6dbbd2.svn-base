// **************************************************************************
/** @class MessageImp

@brief The base implementation of a distributed messaging system


Overwriting the Write() member function allows to change the look and
feel and also the target of the messages issued through a MessageImp

**/
// **************************************************************************
#include "MessageImp.h"

#include <stdarg.h>

#include <mutex>

#include "tools.h"
#include "Time.h"
#include "WindowLog.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Stores a reference to the given ostream in fOut. This is the stream to
//! which all messaged issued are redirected by default if Write() has
//! not been overwritten doing something else
//!
//! The stored reference can be accessed by either
//!    operator()() or Out()
//!
//! Note, that you have to ensure the stream which is references doesn't
//! go out of scope while in use by MessageImp or one of its derivatives.
//!
//! @param out
//!    ostream to which the output should be redirected
//
MessageImp::MessageImp(ostream &out) : fOut(out), fLastMjd(0)
{
}

// --------------------------------------------------------------------------
//
//! This is a special write function formatting a string when the
//! state of a state machine has changed.
//! 
//! If the state is <-1 nothing is done.
//!
//! Calls the virtual function IndicateStateChange
//!
//! @param time
//!    The time assigned to the message
//!
//! @param server
//!    The server name which is emitting the state change
//!
//! @param msg
//!    The message text
//!
//! @param state
//!    The new state of the system
//
void MessageImp::StateChanged(const Time &time, const string &server, const string &msg, int state)
{
    if (state<-1)
        return;

    ostringstream out;
    out << server << ": Changed state to " << state << " '" << msg << "' received.";

    Write(time, out.str(), MessageImp::kInfo);

    IndicateStateChange(time, server);
}

// --------------------------------------------------------------------------
//
//! The basic implementation of the output of a message to the output
//! stream. This can overwritten by inheriting classes. The default is
//! to redirect the message to the stream fOut. In addition colors
//! from WindowLog are used depending on the severity. The color are ignored
//! if the stream is not of type WindowLog.
//!
//! The default message has the form:
//!     ## 2011-02-22 11:13:32.000754 - Text message.
//!
//! while ## is a placeholder depending on the severity of the message, e.g.
//!
//!     kMessage:  default    ->
//!     kInfo:     green      I>
//!     kWarn:     yellow     W>
//!     kError:    red        E>
//!     kAlarm     red        E>
//!     kFatal:    red-blink  !>
//!     kDebug:    blue
//!     default:   bold       >>
//!
//! @param time
//!    The time assigned to the message
//!
//! @param txt
//!    The message text
//!
//! @param severity
//!    The severity of the message
//
int MessageImp::WriteImp(const Time &time, const string &txt, int severity)
{
    if (severity==kAlarm && txt.length()==0)
        return 0;

    static mutex mtx;
    const lock_guard<mutex> guard(mtx);

    switch (severity)
    {
    case kMessage: fOut << kDefault       << " -> "; break;
    case kComment: fOut << kDefault       << " #> "; break;
    case kInfo:    fOut << kGreen         << " I> "; break;
    case kWarn:    fOut << kYellow        << " W> "; break;
    case kError:
    case kAlarm:   fOut << kRed           << " E> "; break;
    case kFatal:   fOut << kRed << kBlink << " !> "; break;
    case kDebug:   fOut << kBlue          << "    "; break;
    default:       fOut << kBold          << " >> "; break;
    }
    fOut << time.GetAsStr("%H:%M:%S.%f") << " - " << txt << endl;

    return 0;
}

int MessageImp::Write(const Time &time, const string &txt, int severity)
{
    const uint32_t mjd = time.Mjd();

    if (fLastMjd != mjd)
        WriteImp(time, "=================== "+time.GetAsStr("%Y-%m-%d")+" ["+to_string(mjd)+"] ==================");

    fLastMjd = mjd;

    WriteImp(time, txt, severity);
    return 0;
}

// --------------------------------------------------------------------------
//
//! Calls Write with the current time the message text and the severity.
//!
//! @param txt
//!    The message text to be passed to Write
//!
//! @param severity
//!    The severity of the message to be passed to Write
//
int MessageImp::Update(const string &txt, int severity)
{
    Write(Time(), txt, severity);
    return 0;
}

// --------------------------------------------------------------------------
//
//! Just a helper to format the message according to the user input.
//! See the documentation of printf for details.
//!
//! @param severity
//!    The severity of the message to be passed to Write
//!
//! @param fmt
//!    Format string according to which the text is formatted
//
/*
int MessageImp::Update(int severity, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string str = Tools::Format(fmt, ap);
    va_end(ap);
    return Update(str, severity);
}
*/
