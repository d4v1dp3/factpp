// **************************************************************************
/** @namespace ReadlineColor

@brief A fewer helper functions to apply color attributes and redirect the output

 */
// **************************************************************************
#include "ReadlineColor.h"

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

#include "Time.h"
#include "Readline.h"
#include "WindowLog.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! @returns
//!    always true
//
bool ReadlineColor::PrintBootMsg(ostream &out, const string &name, bool interactive)
{
#if BOOST_VERSION < 104600
    const string n = boost::filesystem::path(name).stem();
#else
    const string n = boost::filesystem::path(name).stem().string();
#endif

    out << kBlue << kBold << kUnderline << "\n Master Control Program (compiled " __DATE__ " " __TIME__ << ") " << endl;
    out << kBlue <<
        "\n"
        "  ENCOM MX 16-923 USER # 0" << int(Time().Mjd()) << Time::fmt(" %H:%M:%S") << Time() << Time::reset << " INFORMATION\n"
        "\n"
        "  TELESCOPE CONTROL PROGRAM: " << n << "\n"
        "  ANNEXED BY FACT COLLABORATION\n"
        "  ORIGINAL PROGRAM WRITTEN BY T.BRETZ\n"
        "  THIS INFORMATION " << kUnderline << "PRIORITY ONE"
        << endl;
    out << kBlue << "  END OF LINE\n" << endl;

    if (!interactive)
        return true;

    out << "Enter 'h' for help." << endl;
    out << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! @returns
//!    always true
//
bool ReadlineColor::PrintCommands(ostream &out)
{
    out << endl;
    out << " " << kUnderline << " Commands:" << endl;
    out << "   No application specific commands defined." << endl;
    out << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! Displays the available ncurses attributes, like color.
//!
//! @returns
//!    always true
//
bool ReadlineColor::PrintAttributes(ostream &out)
{
    out << endl;
    out << " Attributes:" << endl;
    out << "   " << kReset      << "kReset" << endl;
    out << "   " << kNormal     << "kNormal" << endl;
    out << "   " << kHighlight  << "kHighlight" << endl;
    out << "   " << kReverse    << "kReverse" << endl;
    out << "   " << kUnderline  << "kUnderline" << endl;
    out << "   " << kBlink      << "kBlink" << endl;
    out << "   " << kDim        << "kDim" << endl;
    out << "   " << kBold       << "kBold" << endl;
    out << "   " << kProtect    << "kProtect" << endl;
    out << "   " << kInvisible  << "kInvisible" << endl;
    out << "   " << kAltCharset << "kAltCharset" << kReset << "  (kAltCharset)" << endl;
    out << endl;
    out << " Colors:" << endl;
    out << "   " << kDefault << "kDefault  " << kBold << "+  kBold" << endl;
    out << "   " << kRed     << "kRed      " << kBold << "+  kBold" << endl;
    out << "   " << kGreen   << "kGreen    " << kBold << "+  kBold" << endl;
    out << "   " << kYellow  << "kYellow   " << kBold << "+  kBold" << endl;
    out << "   " << kBlue    << "kBlue     " << kBold << "+  kBold" << endl;
    out << "   " << kMagenta << "kMagenta  " << kBold << "+  kBold" << endl;
    out << "   " << kCyan    << "kCyan     " << kBold << "+  kBold" << endl;
    out << "   " << kWhite   << "kWhite    " << kBold << "+  kBold" << endl;
    out << "   " << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! Displays the keybindings available due to the Shell class
//!
//! @returns
//!    always true
//!
//! @todo
//!    Update output
//
bool ReadlineColor::PrintKeyBindings(ostream &out)
{
    out << endl;
    out << " " << kUnderline << "Key bindings:" << endl << endl;;
    out << "  Default key-bindings are identical with your bash." << endl;
    out << endl;
    out << kBold << "   Page-up         " << kReset << "Search backward in history" << endl;
    out << kBold << "   Page-dn         " << kReset << "Search forward in history" << endl;
    out << kBold << "   Ctrl-left       " << kReset << "One word backward" << endl;
    out << kBold << "   Ctrl-right      " << kReset << "One word forward" << endl;
    out << kBold << "   Home            " << kReset << "Beginning of line" << endl;
    out << kBold << "   End             " << kReset << "End of line" << endl;
    out << kBold << "   Ctrl-d          " << kReset << "Quit" << endl;
    out << kBold << "   Ctrl-y          " << kReset << "Delete line" << endl;
    out << kBold << "   Alt-end/Ctrl-k  " << kReset << "Delete until the end of the line" << endl;
    out << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! Print a general help text which also includes the commands pre-defined
//! by the Shell class.
//!
//! @returns
//!    always true
//!
//! @todo
//!    Get it up-to-date
//
bool ReadlineColor::PrintGeneralHelp(ostream &out, const string &name)
{
    out << endl;
    out << " " << kUnderline << "General help:" << endl << endl;
    out << "  The command history is automatically loaded and saves to" << endl;
    out << "  and from " << name << endl;
    out << endl;
    out << kBold << "   h,help       " << kReset << "Print this help message\n";
    out << kBold << "   clear        " << kReset << "Clear history buffer\n";
    out << kBold << "   lh,history   " << kReset << "Dump the history buffer to the screen\n";
    out << kBold << "   v,variable   " << kReset << "Dump readline variables\n";
    out << kBold << "   f,function   " << kReset << "Dump readline functions\n";
    out << kBold << "   m,funmap     " << kReset << "Dump readline funmap\n";
    out << kBold << "   c,command    " << kReset << "Dump available commands\n";
    out << kBold << "   k,keylist    " << kReset << "Dump key bindings\n";
    out << kBold << "   a,attrs      " << kReset << "Dump available stream attributes\n";
    out << kBold << "   .! command   " << kReset << "Execute a shell command\n";
    out << kBold << "   .w n         " << kReset << "Sleep n milliseconds\n";
    out << kBold << "   .x filename  " << kReset << "Execute a script of commands (+optional arguments)\n";
    out << kBold << "   .x file:N    " << kReset << "Execute a script of commands, start at label N\n";
    out << kBold << "   .j N         " << kReset << "Forward jump to label N\n";
    out << kBold << "   .lt f0 f1 N  " << kReset << "If float f0 lower than float f1, jump to label N\n";
    out << kBold << "   .gt f0 f1 N  " << kReset << "If float f0 greater than float f1, jump to label N\n";
    out << kBold << "   .eq i0 i1 N  " << kReset << "If int i0 equal int i1, jump to label N\n";
    out << kBold << "   : N          " << kReset << "Defines a label (N=number)\n";
    out << kBold << "   # comment    " << kReset << "Ignored\n";
    out << kBold << "   .q,quit      " << kReset << "Quit" << endl;
    out << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! Execute a shell command through a pipe. Write stdout to rl_outstream
//!
//! @param cmd
//!     Command to be executed
//!
//! @returns
//!     always true
//
bool ReadlineColor::ExecuteShellCommand(ostream &out, const string &cmd)
{
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        out << kRed << "ERROR - Could not create pipe '" << cmd << "': " << strerror(errno) << " [" << errno << "]" << endl;
        return true;
    }

    while (1)
    {
        char buf[1024];

        const size_t sz = fread(buf, 1, 1024, pipe);
        out.write(buf, sz);

        if (feof(pipe) || ferror(pipe))
            break;
    }

    out << endl;

    if (ferror(pipe))
        out << kRed << "ERROR - Reading from pipe '" << cmd << "': " << strerror(errno) << " [" << errno << "]" << endl;

    pclose(pipe);

    return true;
}


bool ReadlineColor::Process(ostream &out, const string &str)
{
    // ----------- Readline -----------

    if (str.substr(0, 2)==".!")
         return ExecuteShellCommand(out, str.substr(2));

    if (str=="lh" || str=="history")
    {
        out << endl << kBold << "History:" << endl;
        return Readline::RedirectionWrapper(out, Readline::DumpHistory);
    }

    if (str=="v" || str=="variable")
    {
        out << endl << kBold << "Variables:" << endl;
        return Readline::RedirectionWrapper(out, Readline::DumpVariables);
    }

    if (str=="f" || str=="function")
    {
        out << endl << kBold << "Functions:" << endl;
        return Readline::RedirectionWrapper(out, Readline::DumpFunctions);
    }

    if (str=="m" || str=="funmap")
    {
        out << endl << kBold << "Funmap:" << endl;
        return Readline::RedirectionWrapper(out, Readline::DumpFunmap);
    }

    // ------------ ReadlineWindow -------------

    if (str=="a" || str=="attrs")
        return PrintAttributes(out);

    return false;
}
