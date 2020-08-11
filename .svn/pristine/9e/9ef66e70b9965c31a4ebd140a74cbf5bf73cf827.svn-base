// **************************************************************************
/** @class Console

@brief This is an extension to the Readline class provding buffered output

This in an extension to the Readline class. It's purpose is to keep a
buffered output stream and flush the stream either between readline entries
(non continous mode) or continously, keeping the readline prompt as
intact as possible.

 */
// **************************************************************************
#include "Console.h"

#include <unistd.h>

#include <sstream>
#include <iostream>

#include "tools.h"

#include "ReadlineColor.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Instantiate a console stream. It will create a WindowLog object
//! and immediatel switch off its output to the console. The default more
//! is non-continous.
//!
//! @param name
//!     The name of the program passed to the Readline constructor
//!
Console::Console(const char *name) : Readline(name), fContinous(false)
{
    fLogO.SetNullOutput();
    fLogI.SetNullOutput();
    fLogO.SetBacklog(true);
    fLogI.SetBacklog(true);
}

// --------------------------------------------------------------------------
//
//! Flush the contents of the buffer before it is destroyed.
//
Console::~Console()
{
    // flush buffer to display before it is destroyed in its destructor
    fLogO.Display();
    fLogI.Display();
}

void Console::PrintReadlineError(const std::string &str)
{
    fLogI << kRed << str << endl;
}

// --------------------------------------------------------------------------
//
//! Wrapper to call the correspnding function from ReadlineColor
//
bool Console::PrintGeneralHelp()
{
    return ReadlineColor::PrintGeneralHelp(fLogI, GetName());
}

// --------------------------------------------------------------------------
//
//! Wrapper to call the correspnding function from ReadlineColor
//
bool Console::PrintCommands()
{
    return ReadlineColor::PrintCommands(fLogI);
}

// --------------------------------------------------------------------------
//
//! Wrapper to call the correspnding function from ReadlineColor
//
bool Console::PrintKeyBindings()
{
    return ReadlineColor::PrintKeyBindings(fLogI);
}

void Console::Lock()
{
    // FIXME: Check missing
    fLogO.Display(true);
    fLogO.SetBacklog(false);
    fLogO.SetNullOutput(false);
}

void Console::Unlock()
{
    // FIXME: Check missing
    fLogO.SetNullOutput(true);
    fLogO.SetBacklog(true);
}

// --------------------------------------------------------------------------
//
//! Processes the command provided by the Shell-class.
//!
//! @returns
//!    whether a command was successfully processed or could not be found
//
bool Console::Process(const string &str)
{
    if (ReadlineColor::Process(fLogI, str))
        return true;

    if (str.substr(0, 3)==".w ")
    {
        Lock();
        usleep(stoul(str.substr(3))*1000);
        Unlock();
        return true;
    }

    if (Readline::Process(str))
        return true;

    return false;
}

// --------------------------------------------------------------------------
//
//! Before readline starts flush the buffer to display all stuff which was
//! buffered since the last readline call returned.
//
void Console::Startup()
{
    // Call readline's startup (just in case, it is empty)
    Readline::Startup();

    // First flush the buffer of the stream which is synchronous
    // with the prompt
    fLogI.Display(true);

    // Now flush the stream which is asychronous
    fLogO.Display(true);

    // The order has the advantage that output initiated by the prompt
    // is not interrupter by the synchronous stream
}

// --------------------------------------------------------------------------
//
//! Flush the buffer if we are in continous mode, and call Readline's
//! EventHook to update the prompt.
//
void Console::EventHook(bool)
{
    // If the output is continous and we are going to output something
    // first jump back to the beginning of the line (well, that
    // doesn't work well if the input line is already two lines)
    // and then flush the buffer.
    const bool newline = fContinous && fLogO.GetSizeBacklog()>0;
    if (newline)
    {
        // Clear the line we are going to overwrite
        std::cout << "\r\033[0K";
        fLogO.Display(true);
    }

    // Call Readline's EventHook to update the prompt
    // and signal readline so that a new prompt is displayed
    Readline::EventHook(newline);
}

string Console::GetLinePrompt() const
{
    const string siz = fLogO.GetSizeStr();

    ostringstream str;
    str << '[' << GetLine();
    return fContinous ? str.str()+']' : str.str()+':'+siz+']';
}

// --------------------------------------------------------------------------
//
//! Before Readline::Run() is called the buffer is flushed as well as
//! after the Run() loop has exited.
//! command processing. This keeps things as seperated as possible,
//! although there is no gurantee.
//
void Console::Run(const char *)
{
    // Flush the buffer before we print the boot message
    fLogO.Display(true);

    ReadlineColor::PrintBootMsg(fLogI, GetName());

    // Flush the buffer before we start out readline loop
    fLogI.Display(true);
    fLogO.Display(true);

    // Now run readlines main loop
    Readline::Run();

    // flush buffer to display
    fLogI.Display(true);
    fLogO.Display(true);
}

// **************************************************************************
/** @class ConsoleStream

@brief This is an extension to the Readline class provding a colored output

This in an extension to the Readline class. It's purpose is just to have a
colored output stream available. It's main idea is that it is used in
environments without user interaction as a replacement the Console class.
This is interesting to be able to do everything identical as if Console
would be used, but Run() does not prompt but just wait until Stop()
was called. The advantage is that some functions of Readline can be used
like Execute and the history (just for Execute() commands of course)

 */
// **************************************************************************

ConsoleStream::~ConsoleStream()
{
    fLogO.Display();
}

// --------------------------------------------------------------------------
//
//! Instantiate a console stream. It will create a single WindowLog object
//! which is returned as input and output stream.
//!
//! @param name
//!     The name of the program passed to the Readline constructor
//!
ConsoleStream::ConsoleStream(const char *name) : Readline(name)
{
    fLogO.SetBacklog(false);
    fLogO.SetNullOutput(false);
    ReadlineColor::PrintBootMsg(fLogO, GetName(), false);
}

void ConsoleStream::PrintReadlineError(const std::string &str)
{
    fLogO << kRed << str << endl;
}

// --------------------------------------------------------------------------
//
//! Just usleep until Stop() was called.
//
void ConsoleStream::Run(const char *)
{
    while (!IsStopped())
    {
        const string buf = GetExternalInput();
        SetExternalInput("");
        if (!buf.empty())
            ProcessLine(buf);

        usleep(100000);
    }
}
