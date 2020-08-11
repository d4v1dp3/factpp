#include "DimState.h"

using namespace std;
using namespace boost;

void DimDnsServerList::HandlerServerImp(const EventImp &evt)
{
    if (evt.GetSize()==0)
        return;

    time = evt.GetTime();
    msg  = evt.GetString();

    typedef char_separator<char> separator;
    const tokenizer<separator> tok(msg, separator("|"));

    for (auto it=tok.begin(); it!=tok.end(); it++)
    {
        const size_t p = it->find_first_of('@');
        if (p==string::npos)
            continue;

        // The first part before the first @ is the server name
        string server = it->substr(0, p);
        if (server.empty())
            continue;

        // If it starts with a - we have to remove an entry
        if (server[0]=='-')
        {
            fServerList.erase(server.substr(1));
            CallbackServerRemove(server.substr(1));
            continue;
        }

        // If it starts with a + we have to add an entry
        if (server[0]=='+')
            server = server.substr(1);

        // Check if this server is already in the list.
        // This should never happen if Dim works reliable
        if (fServerList.insert(server).second)
            CallbackServerAdd(server);
    }
}

void DimDnsServiceList::HandlerServiceListImp(const EventImp &evt)
{
    if (evt.GetSize()==0)
        return;

    // Get the name of the service
    //const string svc = getInfo()->getName();

    // Get the server name from the service name
    //const string server  = svc.substr(0, svc.find_first_of('/'));
    //const string service = svc.substr(svc.find_first_of('/')+1);

    msg  = evt.GetString();
    time = evt.GetTime();

    // Initialize the entry with an empty list
    //if (msg[0]!='+' && msg[0]!='-')
    //    return;

    typedef char_separator<char> separator;
    const tokenizer<separator> tok(msg, separator("\n"));

    for (auto it=tok.begin(); it!=tok.end(); it++)
    {
        string str = *it;

        if (str[0]=='-')
            continue;

        if (str[0]=='+')
            str = str.substr(1);

        const size_t last_pipe = str.find_last_of('|');

        // Get the type and compare it with fType
        const string type = str.substr(last_pipe+1);
        if (type=="RPC")
            continue;

        //const bool iscmd = type=="CMD";
        //if (type!=fType && fType!="*")
        //    continue;

        const size_t first_pipe  = str.find_first_of('|');
        const size_t first_slash = str.find_first_of('/');

        // Get format, name and command name
        Service service;
        service.server  = str.substr(0, first_slash);
        service.name    = str.substr(0, first_pipe);
        service.service = str.substr(first_slash+1, first_pipe-first_slash-1);
        service.format  = str.substr(first_pipe +1, last_pipe -first_pipe -1);
        service.iscmd   = type=="CMD";

        const auto v = find(fServiceList.begin(), fServiceList.end(), service.name);
        if (v!=fServiceList.end())
            continue;

        CallbackServiceAdd(service);
    }
}
