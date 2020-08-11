#include "RemoteControl.h"

// ==========================================================================

bool RemoteControlImp::ProcessCommand(const std::string &str, bool change)
{
    if (fCurrentServer.empty())
    {
        const size_t p1 = str.find_first_of(' ');
        const size_t p2 = str.find_first_of('/');

        const bool is_cmd = p2!=string::npos && p1>p2;

        string s = str;
        if (is_cmd)
            s = str.substr(0, p2);

        if (p2<p1 && p2!=str.length()-1)
        {
            const string c = str.substr(p2+1);
            return SendDimCommand(lout, s, c, !change);
        }

        if (HasServer(s))
        {
            if (!change)
                return SendDimCommand(lout, str, "", !change);

            fCurrentServer = s;
            return true;
        }

        if (!change && is_cmd)
            throw runtime_error("Unkown server '"+s+"'");

        if (change)
            lout << kRed << "Unkown server '" << s << "'" << endl;

        return false;
    }

    if (!fCurrentServer.empty() && str=="..")
    {
        fCurrentServer = "";
        return true;
    }
    return SendDimCommand(lout, fCurrentServer, str, !change);
}

// ==========================================================================

#include "tools.h"

string RemoteConsole::GetUpdatePrompt() const
{
    if (fImp->GetCurrentState()>=3)
        return "";

    // If we are continously flushing the buffer omit the buffer size
    // If we are buffering show the buffer size
    const string beg = GetLinePrompt();

    // If we have not cd'ed to a server show only the line start
    if (fCurrentServer.empty() || !fImp)
        return beg + "> ";

    // Check if we have cd'ed to a valid server
    const State state = fImp->GetServerState(fCurrentServer);
    if (state.index==-256)
        return beg + "> ";

    // The server
    const string serv = "\033[34m\033[1m"+fCurrentServer+"\033[0m";

    // If no match found or something wrong found just output the server
    if (state.index<-1)
        return beg + " " + serv + "> ";

    // If everything found add the state to the server
    return beg + " " + serv + ":\033[32m\033[1m" + state.name + "\033[0m> ";
}

string RemoteShell::GetUpdatePrompt() const
{
    // If we are continously flushing the buffer omit the buffer size
    // If we are buffering show the buffer size
    const string beg = GetLinePrompt();

    // If we have not cd'ed to a server show only the line start
    if (fCurrentServer.empty() || !fImp)
        return beg + "> ";

    const State state = fImp->GetServerState(fCurrentServer);
    if (state.index==-256)
        return beg + "> ";//Form("\n[%d] \033[34m\033[1m%s\033[0m> ", GetLine(), fCurrentServer.c_str());

    // If no match found or something wrong found just output the server
    if (state.index<-1)
        return beg + " " + fCurrentServer + "> ";

    // If everything found add the state to the server
    return beg + " " + fCurrentServer + ":" + state.name + "> ";
}
