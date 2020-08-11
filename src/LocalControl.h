#ifndef FACT_LocalControl
#define FACT_LocalControl

#include <ostream>

// **************************************************************************
/** @class LocalControl

@brief Implements a local control for a StateMachine based on a Readline class

This template implements all functions which overwrite any function from the
Readline class needed for a local control of a state machien. Since
several derivatives of the Readline class implement different kind of
Readline access, this class can be derived by any of them due to its
template argument. However, the normal case will be deriving it from
either Console or Shell.

@tparam T
   The base class for RemoteControl. Either Readlien or a class
   deriving from it. This is usually either Console or Shell.

**/
// **************************************************************************
#include <boost/version.hpp>
#include <boost/filesystem.hpp>

#include "tools.h"

#include "WindowLog.h"
#include "StateMachineImp.h"

using namespace std;

template <class T>
class LocalControl : public T
{
private:
    char **Completion(const char *text, int pos, int)
    {
        return pos>0 ? 0 : T::Complete(fStateMachine->GetEventNames(), text);
    }

protected:
    StateMachineImp *fStateMachine;

    std::ostream &lout;

    std::string fName;

    LocalControl(const char *name) : T(name), 
        fStateMachine(0), lout(T::GetStreamIn()),
#if BOOST_VERSION < 104600
        fName(boost::filesystem::path(name).filename())
#else
        fName(boost::filesystem::path(name).filename().string())
#endif
    { }

    bool PrintGeneralHelp()
    {
        T::PrintGeneralHelp();
        lout << " " << kUnderline << "Specific commands:" << endl;
        lout << kBold << "   ac,allowed   " << kReset << "Display a list of all currently allowed commands." << endl;
        lout << kBold << "   st,states    " << kReset << "Display a list of the available states with description." << endl;
        lout << kBold << "   > <text>     " << kReset << "Echo <text> to the output stream" << endl;
        lout << kBold << "   .s           " << kReset << "Wait for the state-machine to change to the given state.\n";
        lout <<          "                "              "     .s <server> [<state> [<timeout> [<label>]]]\n";
        lout <<          "                "              "<server>  The server for which state to wait (e.g. FTM_CONTROL)\n";
        lout <<          "                "              "<state>   The state id (see 'states') for which to wait (e.g. 3)\n";
        lout <<          "                "              "<imeout>  A timeout in millisenconds how long to wait (e.g. 500)\n";
        lout <<          "                "              "<label>   A label until which everything is skipped in case of timeout\n";
        lout << endl;
        return true;
    }
    bool PrintCommands()
    {
        lout << endl << kBold << "List of commands:" << endl;
        fStateMachine->PrintListOfEvents(lout);
        lout << endl;

        return true;
    }

    bool Process(const std::string &str)
    {
        if (str.substr(0, 2)=="h " || str.substr(0, 5)=="help ")
        {
            lout << endl;
            fStateMachine->PrintListOfEvents(lout, str.substr(str.find_first_of(' ')+1));
            lout << endl;

            return true;
        }
        if (str=="states" || str=="st")
        {
            fStateMachine->PrintListOfStates(lout);
            return true;
        }
        if (str=="allowed" || str=="ac")
        {
            lout << endl << kBold << "List of commands allowed in current state:" << endl;
            fStateMachine->PrintListOfAllowedEvents(lout);
            lout << endl;
            return true;
        }

        if (str.substr(0, 3)==".s ")
        {
            istringstream in(str.substr(3));

            int state=-100, ms=0;
            in >> state >> ms;

            if (state==-100)
            {
                lout << kRed << "Couldn't parse state id in '" << str.substr(3) << "'" << endl;
                return true;
            }

            const Time timeout = ms<=0 ? Time(Time::none) : Time()+boost::posix_time::millisec(ms);

            while (fStateMachine->GetCurrentState()!=state && timeout>Time() && !T::IsScriptStopped())
                usleep(1);

            if (fStateMachine->GetCurrentState()==state)
                return true;

            int label = -1;
            in >> label;
            if (in.fail() && !in.eof())
            {
                lout << kRed << "Invalid label in '" << str.substr(3) << "'" << endl;
                T::StopScript();
                return true;
            }
            T::SetLabel(label);

            return true;
        }

        if (str[0]=='>')
        {
            fStateMachine->Comment(Tools::Trim(str.substr(1)));
            return true;
        }

        if (T::Process(str))
            return true;

        return !fStateMachine->PostEvent(lout, str);
    }

public:

    void SetReceiver(StateMachineImp &imp) { fStateMachine = &imp; }
};

// **************************************************************************
/** @class LocalStream

@brief Derives the LocalControl from ConsoleStream

This is basically a LocalControl, which derives through the template
argument from the ConsoleStream class. 

 */
// **************************************************************************
#include "Console.h"

class LocalStream : public LocalControl<ConsoleStream>
{
public:
    LocalStream(const char *name, bool null = false)
        : LocalControl<ConsoleStream>(name) { SetNullOutput(null); }
};

// **************************************************************************
/** @class LocalConsole

@brief Derives the LocalControl from Control and adds prompt

This is basically a LocalControl, which derives through the template
argument from the Console class. It enhances the functionality of
the local control with a proper updated prompt.

 */
// **************************************************************************
#include "tools.h"

class LocalConsole : public LocalControl<Console>
{
public:
    LocalConsole(const char *name, bool continous=false)
        : LocalControl<Console>(name)
    {
        SetContinous(continous);
    }

    string GetUpdatePrompt() const
    {
        return GetLinePrompt()+" "
            "\033[34m\033[1m"+fName+"\033[0m:"
            "\033[32m\033[1m"+fStateMachine->GetStateName()+"\033[0m> ";
    }
};

// **************************************************************************
/** @class LocalShell

@brief Derives the LocalControl from Shell and adds a colored prompt

This is basically a LocalControl, which derives through the template
argument from the Shell class. It enhances the functionality of
the local control with a proper updated prompt.

 */
// **************************************************************************
#include "Shell.h"

class LocalShell : public LocalControl<Shell>
{
public:
    LocalShell(const char *name, bool = false)
        : LocalControl<Shell>(name) { }

    string GetUpdatePrompt() const
    {
        return GetLinePrompt()+' '+fName+':'+fStateMachine->GetStateName()+"> ";
    }
};

#endif
