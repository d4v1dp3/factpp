// **************************************************************************
/** @class DimServiceInfoList

@brief Maintains a list of all services available in the Dim network

The idea of this class is to maintain a list of services available
in the Dim network as well as state descriptions if available.

Therefore, it subscribes to the SERVICE_LIST, SERVICE_DESC and STATE_LIST
services of all servers.

To maintain the list it derives from DimServerList which maintains
a list of all servers.

To maintain the subscriptions it overwrites:

- void DimServerList::AddServer(const std::string &s)
- void DimServerList::RemoveServer(const std::string &s)
- void DimServerList::RemoveAllServers()

If a derived class also overwrites these functions it must be ensured that
the member functions of DimServiceInfoList are still called properly.

Whenever a service is added or removed, or all services of one server
is removed the following virtual functions are called:

- virtual void AddService(const std::string &server, const std::string &service, const std::string &fmt, bool iscmd)
- virtual void RemoveService(const std::string &server, const std::string &service, bool iscmd)
- virtual void RemoveAllServices(const std::string &server)

Note, that these functions are not called from the RemoveServer() and
RemoveAllServer() functions. It might be a difference whether all services
were removed but the server is still online or the server went offline.

If a description or a state was added, this is signaled though:

- virtual void AddDescription(const std::string &server, const std::string &service, const std::vector<Description> &vec)
- virtual void AddStates(const std::string &server, const std::vector<State> &vec)

Note, that Descriptions and States are never removed except a service or
server goes offline. It is expected that if a service comes online also
the list of descritions is sent again.

*/
// **************************************************************************
#include "DimServiceInfoList.h"

#include <sstream>

#include "WindowLog.h"
#include "Converter.h"

#include "tools.h"
#include "Time.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! A helper to shorten the call to create a DimInfo.
//!
//! @param str
//!    name of the server to which we want to subscribe
//!
//! @param svc
//!    name of the servic on the server to which we want to subscribe
//!
//! @returns
//!    a pointer to the newly created DimInfo
//!
DimInfo *DimServiceInfoList::CreateDimInfo(const string &str, const string &svc) const
{
    return new DimInfo((str+'/'+svc).c_str(),
                       const_cast<char*>(""),
                       const_cast<DimServiceInfoList*>(this));
}

// --------------------------------------------------------------------------
//
//! Adds the service subscription for SERVICE_LIST, SERVICE_DESC and
//! STATE_LIST for the given server. Don't forget to call this function
//! if it is overwritten in a derived class.
//!
//! @param s
//!     server which should be added
//!
//! @throws
//!     a runtime_error is the server is already in the list
//
void DimServiceInfoList::AddServer(const string &s)
{
    // Check if this server is already in the list.
    // This should never happen if Dim works reliable
    const ServiceInfoList::iterator v = fServiceInfoList.find(s);
    if (v!=fServiceInfoList.end())
    {
        stringstream err;
        err << "DimServiceInfoList: Server '" << s << "' in list not as it ought to be.";
        throw runtime_error(err.str());
    }

    fServiceInfoList[s].push_back(CreateSL(s));
    fServiceInfoList[s].push_back(CreateFMT(s));
    fServiceInfoList[s].push_back(CreateDS(s));
}

// --------------------------------------------------------------------------
//
//! Removes the service subscription for SERVICE_LIST, SERVICE_DESC and
//! STATE_LIST for the given server, as well as the stored informations.
//! Don't forget to call this function if it is overwritten in a derived
//! class.
//!
//! @param s
//!     server which should be removed. We need to make a copy, otherwise
//!     RemoveServer will destroy the staring the reference is pointing to
//!
//! @throws
//!     a runtime_error is the server to be removed is not in the list
//
void DimServiceInfoList::RemoveServer(const string s)
{
    fList->RemoveAllServices(s);

    const ServiceInfoList::iterator v = fServiceInfoList.find(s);
    if (v==fServiceInfoList.end())
        return;
    /*
    {
        stringstream err;
        err << "DimServiceInfoList: Server '" << s << "' not in list as it ought to be.";
        throw runtime_error(err.str());
    }*/

    // Remove the server from the server list
    delete v->second[0];
    delete v->second[1];
    delete v->second[2];

    fServiceInfoList.erase(v);
    fServiceList.erase(s);
}

// --------------------------------------------------------------------------
//
//! Removes the service subscription for SERVICE_LIST, SERVICE_DESC and
//! STATE_LIST for all servers, as well as all stored informations.
//! Don't forget to call this function if it is overwritten in a derived
//! class.
//!
void DimServiceInfoList::RemoveAllServers()
{
    while (!fServiceInfoList.empty())
        RemoveServer(fServiceInfoList.begin()->first);
}


// --------------------------------------------------------------------------
//
//! This function processes the update of the SERVICE_LIST, SERVICE_DESC,
//! and STATE_LIST updates.
//!
//! Whenever a service is added or removed or all services of a server are
//! removed (the list is newly sent completely) the virtual functions
//! AddService(), RemoveService() and RemoveAllServices() aee called.
//!
//! If a new description or a new state is added, the virtual functions
//! AddDescription() and AddStates() respectively are called.
//
void DimServiceInfoList::infoHandler()
{
    // Get the name of the service
    const string svc = getInfo()->getName();

    // Get the server name from the service name
    const string server  = svc.substr(0, svc.find_first_of('/'));
    const string service = svc.substr(svc.find_first_of('/')+1);

    if (service=="SERVICE_LIST")
    {
        // For easy and fast access get the corresponding reference
        TypeList &list = fServiceList[server].first;

        const string str = getInfo()->getString();

        // WHAT's THIS???
        if (str.length()==0)
            return;

        // Initialize the entry with an empty list
        if (str[0]!='+' && str[0]!='-')
        {
            fList->RemoveAllServices(server);
            list.clear();
        }

        string buffer;

        // Tokenize the stream into lines
        stringstream stream(str);
        while (getline(stream, buffer, '\n'))
        {
            if (buffer.empty())
                continue;

            // Get the type and compare it with fType
            const string type = buffer.substr(buffer.find_last_of('|')+1);
            if (type=="RPC")
                continue;

            /*
            const bool iscmd = type=="CMD";
            if (type!=fType && fType!="*")
                continue;
                */

            // Get format, name and command name
            const string fmt  = buffer.substr(buffer.find_first_of('|')+1, buffer.find_last_of('|')-buffer.find_first_of('|')-1);
            const string name = buffer.substr(buffer.find_first_of('/')+1, buffer.find_first_of('|')-buffer.find_first_of('/')-1);
            //const string cmd  = buffer.substr(0, buffer.find_first_of('|'));

            const bool iscmd = type=="CMD";

            // FIXME: Do we need to check that the buffer starts with SERVER ?

            if (buffer[0]=='-')
            {
                // Check if this server is not found in the list.
                // This should never happen if Dim works reliable
                const TypeList::iterator v = list.find(name);
                /*
                if (v==list.end())
                {
                    stringstream err;
                    err << "DimServiceInfoList: Service '" << server << "/" << name << "' not in list as it ought to be.";
                    // Seems to happen why more than one client is subscribed
                    // and e.g. the datalogger is immediately quit
                    throw runtime_error(err.str());
                }*/

                fList->RemoveService(server, name, iscmd);
                if (v!=list.end())
                    list.erase(v);

                continue;
            }

            if (buffer[0]=='+')
            {
                // Check if this server is not found in the list.
                // This should never happen if Dim works reliable
                const TypeList::iterator v = list.find(name);
                if (v!=list.end())
                {
                    stringstream err;
                    err << "DimServiceInfoList: Service '" << server << "/" << name << "' already in list not as it ought to be.";
                    throw runtime_error(err.str());
                }

                list[name] = make_pair(fmt, iscmd);
                fList->AddService(server, name, fmt, iscmd);

                continue;
            }

            // Add name the the list
            list[name] = make_pair(fmt, iscmd);
            fList->AddService(server, name, fmt, iscmd);
        }

        return;
    }

    if (service=="SERVICE_DESC")
    {
        // For easy and fast access get the corresponding reference
        DescriptionList &list = fServiceList[server].second;

        list.clear();

        string buffer;

        stringstream stream(getInfo()->getString());
        while (getline(stream, buffer, '\n'))
        {
            if (buffer.empty())
                continue;

            const vector<Description> v = Description::SplitDescription(buffer);

            const string name    = v[0].name.substr(v[0].name.find_first_of('/')+1);
            const string comment = v[0].comment;

            list[name] = make_pair(comment, vector<Description>(v.begin()+1, v.end()));

            fList->AddDescription(server, name, v);
        }

        return;
    }

    if (service=="STATE_LIST")
    {
        vector<State> &vec = fServiceList[server].third;
        vec = State::SplitStates(getInfo()->getString());
        fList->AddStates(server, vec);

        return;
    }
}

// --------------------------------------------------------------------------
//
//! Returns a list of all services available for the given server.
//! Depending on iscmd either only services or only commands are returned.
//!
//! @param server
//!     server for which the list should be returned
//! 
//! @param iscmd
//!     true if only commands should be returned, false for services
//!
//! @returns
//!     a vector<string> which contains all the service or command names for
//!     the given server. The names returned are always SERVER/SERVICE
//!     If the server was not fund an empty vector is returned.
//
vector<string> DimServiceInfoList::GetServiceList(const std::string &server, bool iscmd) const
{
    const ServiceList::const_iterator m = fServiceList.find(server);
    if (m==fServiceList.end())
        return vector<string>();

    const TypeList &list = m->second.first;

    vector<string> vec;
    for (TypeList::const_iterator i=list.begin(); i!=list.end(); i++)
        if (i->second.second==iscmd)
            vec.push_back(server+'/'+i->first);

    return vec;
}

// --------------------------------------------------------------------------
//
//! Returns a list of all services available in the network.
//! Depending on iscmd either only services or only commands are returned.
//!
//! @param iscmd
//!     true if only commands should be returned, false for services
//!
//! @returns
//!     a vector<string> which contains all the service or command names in
//!     the network. The names returned are always SERVER/SERVICE
//
vector<string> DimServiceInfoList::GetServiceList(bool iscmd) const
{
    vector<string> vec;
    for (ServiceList::const_iterator m=fServiceList.begin(); m!=fServiceList.end(); m++)
    {
        const TypeList &list = m->second.first;

        for (TypeList::const_iterator i=list.begin(); i!=list.end(); i++)
            if (i->second.second==iscmd)
                vec.push_back(m->first+'/'+i->first);
    }

    return vec;
}

// --------------------------------------------------------------------------
//
//! Returns a list of all descriptions for the given service on the
//! given server. Service in this context can also be a command.
//!
//! @param server
//!     Server name to look for
//!
//! @param service
//!     Service/command name to look for
//!
//! @returns
//!     a vector<Description> which contains all argument descriptions for
//!     the given service or command. The first entry contains the name
//!     and the general description for the given service. If the server
//!     or service was not found an empty vector is returned.
//
std::vector<Description> DimServiceInfoList::GetDescription(const std::string &server, const std::string &service) const
{
    const ServiceList::const_iterator s = fServiceList.find(server);
    if (s==fServiceList.end())
        return vector<Description>();

    const DescriptionList &descs = s->second.second;

    const DescriptionList::const_iterator d = descs.find(service);
    if (d==descs.end())
        return vector<Description>();

    vector<Description> vec;
    vec.push_back(Description(service, d->second.first));
    vec.insert(vec.end(), d->second.second.begin(), d->second.second.end());

    return vec;
}

// --------------------------------------------------------------------------
//
//! Returns a list of all states associated with the given server.
//!
//! @param server
//!     Server name to look for
//!
//! @returns
//!     a vector<State> which contains all state descriptions for
//!     the given server. If the server or service was not found an
//!     empty vector is returned.
//
vector<State> DimServiceInfoList::GetStates(const std::string &server) const
{
    const ServiceList::const_iterator s = fServiceList.find(server);
    if (s==fServiceList.end())
        return vector<State>();

    return s->second.third;
}

// --------------------------------------------------------------------------
//
//! Returns the Description of the state as defined by the arguments.
//! given server. Service in this context can also be a command.
//!
//! @param server
//!     Server name to look for
//!
//! @param state
//!     The state index to look for (e.g. 1)
//!
//! @returns
//!     The State object containing the description. If the server was
//!     not found the State object will contain the index -3, if the
//!     state was not found -2.
//
State DimServiceInfoList::GetState(const std::string &server, int state) const
{
    const ServiceList::const_iterator s = fServiceList.find(server);
    if (s==fServiceList.end())
    {
        stringstream str;
        str << "DimServiceInfoList::GetState: Searching for state #" << state << " server " << server << " not found.";
        return State(-3, "Server not found", str.str());
    }

    const std::vector<State> &v = s->second.third;

    for (vector<State>::const_iterator i=v.begin(); i!=v.end(); i++)
        if (i->index==state)
            return *i;

    stringstream str;
    str << "DimServiceInfoList::GetState: State #" << state << " not found on server " << server << ".";
    return State(-2, "State not found", str.str());
}

// --------------------------------------------------------------------------
//
//! Returns whether the given service on the given server is a command
//! or not.
//!
//! @param server
//!     Server name to look for
//!
//! @param service
//!     The service name to look for
//!
//! @returns
//!     1 if it is a command, 0 if it is a service, -1 if the service
//!     was not found on the server, -2 if the server was not found.
//
int DimServiceInfoList::IsCommand(const std::string &server, const std::string &service) const
{
    const ServiceList::const_iterator s = fServiceList.find(server);
    if (s==fServiceList.end())
        return -2;

    const TypeList &list = s->second.first;

    const TypeList::const_iterator t = list.find(service);
    if (t==list.end())
        return -1;

    return t->second.second;
}


// --------------------------------------------------------------------------
//
//! Print the full available documentation (description) of all available
//! services or comments to the the given stream.
//!
//! @param out
//!    ostream to which the output is send.
//!
//! @param iscmd
//!   true if all commands should be printed, false for services.
//!
//! @param serv
//!    if a server is given, only the information for this server is printed
//!
//! @param service
//!    if a service is given, only information for this service is printed
//!
//! @returns
//!    the number of descriptions found
//
int DimServiceInfoList::PrintDescription(std::ostream &out, bool iscmd, const string &serv, const string &service) const
{
    int rc = 0;
    for (ServiceList::const_iterator i=fServiceList.begin(); i!=fServiceList.end(); i++)
    {
        const string &server = i->first;

        if (!serv.empty() && server!=serv)
            continue;

        out << kRed << "----- " << server << " -----" << endl;

        const TypeList        &types = i->second.first;
        const DescriptionList &descs = i->second.second;

        for (TypeList::const_iterator t=types.begin(); t!=types.end(); t++)
        {
            if (!service.empty() && t->first!=service)
                continue;

            if (t->second.second!=iscmd)
                continue;

            rc++;

            out << " " << t->first;

            // Check t->second->first for command or service
            const string fmt = t->second.first;
            if (!fmt.empty())
                out << '[' << fmt << ']';

            const DescriptionList::const_iterator d = descs.find(t->first);
            if (d==descs.end())
            {
                out << endl;
                continue;
            }

            const string comment         = d->second.first;
            const vector<Description> &v = d->second.second;

            for (vector<Description>::const_iterator j=v.begin(); j!=v.end(); j++)
                out << " <" << j->name << ">";
            out << endl;

            if (!comment.empty())
                out << "    " << comment << endl;

            for (vector<Description>::const_iterator j=v.begin(); j!=v.end(); j++)
            {
                out << "    " << kGreen << j->name;
                if (!j->comment.empty())
                    out << kReset << ": " << kBlue << j->comment;
                if (!j->unit.empty())
                    out << kYellow << " [" << j->unit << "]";
                out << endl;
            }
        }
        out << endl;
    }

    return rc;
}

// --------------------------------------------------------------------------
//
//! Print the full list of stated for the given server.
//!
//! @param out
//!    ostream to which the output is send.
//!
//! @param serv
//!    if a server is given, only the information for this server is printed
//!
//! @returns
//!    the number of states found
//
int DimServiceInfoList::PrintStates(std::ostream &out, const string &serv) const
{
    int rc = 0;
    for (ServiceList::const_iterator i=fServiceList.begin(); i!=fServiceList.end(); i++)
    {
        const string &server = i->first;

        if (!serv.empty() && server!=serv)
            continue;

        out << kRed << "----- " << server << " -----" << endl;

        const vector<State> &v = i->second.third;

        if (v.empty())
            out << "   <no states>" << endl;
        else
            rc++;

        for (vector<State>::const_iterator s=v.begin(); s!=v.end(); s++)
        {
            out << kBold   << setw(5) << s->index << kReset << ": ";
            out << kYellow << s->name;
            out << kBlue   << " (" << s->comment << ")" << endl;
        }
        out << endl;
    }

    return rc;
}


// --------------------------------------------------------------------------
//
//! Tries to send a dim command according to the arguments.
//! The command given is evaluated according to the available format string.
//!
//! @param server
//!    The name of the server to which the command should be send, e.g. DRIVE
//!
//! @param str
//!    Command and data, eg "TRACK 12.5 13.8"
//!
//! @param lout
//!    the ostream to which errors and debug output is redirected
//!
//! @throws
//!    runtime_error if the server or command was not found, or if the
//!    format associated with the command could not be properly parsed,
//!    or if the command could not successfully be emitted.
//!
void DimServiceInfoList::SendDimCommand(const string &server, string str, ostream &lout) const
{
    str = Tools::Trim(str);

    // Find the delimiter between the command name and the data
    size_t p0 = str.find_first_of(' ');
    if (p0==string::npos)
        p0 = str.length();

    // Get just the command name separated from the data
    const string name = str.substr(0, p0);

    // Compile the command which will be sent to the state-machine
    const string cmd = server + '/' + name;

    const ServiceList::const_iterator m = fServiceList.find(server);
    if (m==fServiceList.end())
        throw runtime_error("Unkown server '"+server+"'");

    const TypeList &services = m->second.first;

    const TypeList::const_iterator t = services.find(name);
    if (t==services.end())
        throw runtime_error("Command '"+name+"' not known on server '"+server+"'");

    if (!t->second.second)
        throw runtime_error("'"+server+"/"+name+" not a command.");

    // Get the format of the event data
    const string fmt = t->second.first;

    // Avoid compiler warning of unused parameter
    lout << flush;

    // Convert the user entered data according to the format string
    // into a data block which will be attached to the event
#ifndef DEBUG
    ostringstream sout;
    const Converter conv(sout, fmt, false);
#else
    const Converter conv(lout, fmt, false);
#endif
    if (!conv)
        throw runtime_error("Couldn't properly parse the format... ignored.");

#ifdef DEBUG
    lout << kBlue << cmd;
#endif
    const vector<char> v = conv.GetVector(str.substr(p0));
#ifdef DEBUG
    lout << kBlue << " [" << v.size() << "]" << endl;
#endif

    DimClient::sendCommandNB(cmd.c_str(), (void*)v.data(), v.size());
}

// --------------------------------------------------------------------------
//
//! Catches the runtime_erros thrown by
//!    SendDimCommand(const string &, string, ostream &)
//! and redirects the error message to the output stream.
//!
//! @param lout
//!    the ostream to which errors and debug output is redirected
//!
//! @param server
//!    The name of the server to which the command should be send, e.g. DRIVE
//!
//! @param str
//!    Command and data, eg "TRACK 12.5 13.8"
//!
//! @returns
//!    true if SendDimComment didn't throw an exception, false otherwise
//!
bool DimServiceInfoList::SendDimCommand(ostream &lout, const string &server, const string &str) const
{
    try
    {
        SendDimCommand(server, str, lout);
        //lout << kGreen << "Command emitted successfully to " << server << "." << endl;
        return true;
    }
    catch (const runtime_error &e)
    {
        lout << kRed << e.what() << endl;
        return false;
    }
}

// --------------------------------------------------------------------------
//
//! Calls SendDimCommand(const string &, string, ostream &) and dumps
//! the output.
//!
//! @param server
//!    The name of the server to which the command should be send, e.g. DRIVE
//!
//! @param str
//!    Command and data, eg "TRACK 12.5 13.8"
//!
//! @throws
//!    see SendDimCommand(const string &, string, ostream &)
//
void DimServiceInfoList::SendDimCommand(const std::string &server, const std::string &str) const
{
    ostringstream dummy;
    SendDimCommand(server, str, dummy);
}

DimServiceInfoList::DimServiceInfoList(DimServiceInfoListImp *list) : DimServerList(list), fList(list) { }
