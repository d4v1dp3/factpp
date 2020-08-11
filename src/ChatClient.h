#ifndef FACT_ChatClient
#define FACT_ChatClient

// **************************************************************************
/** @class ChatClientImp

@brief The base implementation for a chat client

**/
// **************************************************************************
#include "Time.h"
#include "MessageDim.h"
#include "DimErrorRedirecter.h"

using namespace std;

class ChatClientImp : public MessageImp, public DimErrorRedirecter, public MessageDimRX
{
protected:
    std::ostream &lout;          /// Output stream for local synchrounous output

    int Write(const Time &t, const string &txt, int)
    {
        Out() << t << ": " << txt.substr(6) << endl;
        return 0;
    }

protected:
    // Redirect asynchronous output to the output window
    ChatClientImp(std::ostream &out, std::ostream &in) :
        MessageImp(out),
        DimErrorRedirecter(static_cast<MessageImp&>(*this)),
        MessageDimRX("CHAT", static_cast<MessageImp&>(*this)),
        lout(in)
    {
        Out() << Time::fmt("%H:%M:%S");
    }
};

// **************************************************************************
/** @class ChatClient

@brief Implements a remote control based on a Readline class for the Chat client

@tparam T
   The base class for ChatClient. Either Readline or a class
    deriving from it. This is usually either Console or Shell.

**/
// **************************************************************************
#include "WindowLog.h"
#include "ReadlineColor.h"
#include "tools.h"

template <class T>
class ChatClient : public T, public ChatClientImp
{
public:
    // Redirect asynchronous output to the output window
    ChatClient(const char *name) : T(name),
        ChatClientImp(T::GetStreamOut(), T::GetStreamIn())
    {
    }

    // returns whether a command should be put into the history
    bool Process(const std::string &str)
    {
        if (ReadlineColor::Process(lout, str))
            return true;

        if (T::Process(str))
            return true;

        const int rc = DimClient::sendCommand("CHAT/MSG", str.c_str());
        if (!rc)
            lout << kRed << "ERROR - Sending message failed." << endl;

        return false;
    }

    std::string GetUpdatePrompt() const
    {
        // If we have not cd'ed to a server show only the line start
        return T::GetLinePrompt() + " " + (IsConnected() ? "" : "dis") + "connected> ";
    }
};



// **************************************************************************
/** @class ChatConsole

@brief Derives the ChatClient from Control and adds a proper prompt

*/
// **************************************************************************
#include "Console.h"

class ChatConsole : public ChatClient<Console>
{
public:
    ChatConsole(const char *name, bool continous=false) :
        ChatClient<Console>(name)
    {
        SetContinous(continous);
    }
};

// **************************************************************************
/** @class ChatShell

@brief Derives the ChatClient from Shell and adds colored prompt

 */
// **************************************************************************
#include "Shell.h"

class ChatShell : public ChatClient<Shell>
{
public:
    ChatShell(const char *name, bool = false) :
        ChatClient<Shell>(name)
    {
    }
};

#endif
