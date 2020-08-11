// **************************************************************************
/** @class ServiceList

@brief Maintains a list of all servers and services available in the Dim nbetwork

The idea of this class is to maintain a list of servers and services
available in the Dim network. The servers are retrieved by subscribing to
DIS_DNS/SERVER_LIST. The services are retrieved by subscribing to all
servers SERVICE_LIST service.

The server names and the corresponidng DimInfo ovjects for their service
lists are stored in fServerList.

The services of all servers are stored in the fServiceList.

From the services a lookup table fFormatList is created storing the
received formats of all services/commands. The format list is only
updated. So it might happen that formats for commands not available
anymore are still in the list.

Whether commands or services are stored can be selected in the constructor
by the type argument. Use "CMD" for commands and "" for services.


@todo
- Resolve the dependancy on WindowLog, maybe we can issue the log-messages
  via MessageImp or we provide more general modfiers (loike in MLogManip)
- Maybe we also get updates (+/-) on the SERVCIE_LIST?
- check if we really need our own logging stream
- Implement fType=="*"

*/
// **************************************************************************
#include "ServiceList.h"

#include <sstream>

#include "WindowLog.h"
#include "Converter.h"

#include "tools.h"
#include "Time.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Instantiates the default output-stream, subscribes to the Dim service
//! DIS_DNS/SERVER_LIST, which is supposed to contain all servers available
//! in the network, sets fType to type and inistialises fHandler with 0
//!
//! @param type
//!    The type of rows which is filtered out of the retrieved server list.
//!    Use "CMD" for commands and "" for services.
//!
//! @param out
//!    A log-stream to which errors are sent. This however is something
//!    which should not happen anyway.
//
ServiceList::ServiceList(const char *type, ostream &out) :
    wout(out), fDimServers("DIS_DNS/SERVER_LIST", const_cast<char*>(""), this),
    fType(type), fHandler(0)
{
}

// --------------------------------------------------------------------------
//
//! Instantiates the default output-stream, subscribes to the Dim service
//! DIS_DNS/SERVER_LIST, which is supposed to contain all servers available
//! in the network, sets fType to "CMD" and inistialises fHandler with 0
//!
//! @param out
//!    A log-stream to which errors are sent. This however is something
//!    which should not happen anyway.
//
ServiceList::ServiceList(ostream &out) :
    wout(out), fDimServers("DIS_DNS/SERVER_LIST", const_cast<char*>(""), this),
    fType(""), fHandler(0)
{
}

// --------------------------------------------------------------------------
//
//! Delete the allocated memory from fServerList
//
ServiceList::~ServiceList()
{
    for (ServerMap::iterator i=fServerList.begin(); i!=fServerList.end(); i++)
    {
            delete i->second.first;
            delete i->second.second;
    }
}

// --------------------------------------------------------------------------
//
//! The infoHandler which is called when an update to one of our subscribed
//! services is available. If it is the service list, it calls
//! ProcessServerList() and ProcessServiceList() otherwise.
//!
//! After the update has been processed the infoHandler() of fHandler
//! is called if fHandler is available to signal an update to a parent
//! class.
//
void ServiceList::infoHandler()
{
    if (getInfo()==&fDimServers)
        ProcessServerList();
    else
        ProcessServiceList(*getInfo());

    if (fHandler)
    {
        fHandler->itsService = 0;
        fHandler->infoHandler();
    }
}

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
DimInfo *ServiceList::CreateDimInfo(const string &str, const string &svc) const
{
    return new DimInfo((str+'/'+svc).c_str(),
                       const_cast<char*>(""),
                       const_cast<ServiceList*>(this));
}

// --------------------------------------------------------------------------
//
//! This function processes the update of the DIS_DNS/SERVER_LIST update.
//! After this function the server list should be up-to-date again.
//!
//! For each new server a SERVER/SERVICE_LIST service subscription is
//! created and stored in the fServerList. For each removed server
//! the corresponding object are deleted, as well as the corresponding
//! entries from the fServiceList.
//!
void ServiceList::ProcessServerList()
{
    // Get the received string from the handler
    const string str = fDimServers.getString();

    // Check if it starts with + or -
    if (str[0]!='-' && str[0]!='+')
    {
        // If it doesn't start with + or - remove all existing servers
        // we have received a full server list
        for (ServerMap::iterator i=fServerList.begin(); i!=fServerList.end(); i++)
        {
            delete i->second.first;
            delete i->second.second;

            ServiceMap::iterator x = fServiceList.find(i->first);
            fServiceList.erase(x);
            //wout << "Delete: " << i->first << endl;
        }

        fServerList.clear();
    }

    // Create a stringstream to tokenize the received string
    stringstream stream(str);

    // Loop over the seperating tokens
    string buffer;
    while (getline(stream, buffer, '|'))
    {
        // The first part before the first @ is the server name
        const string server = buffer.substr(0, buffer.find_first_of('@'));
        if (server.empty())
            continue;

        // If it starts with a - we have to remove an entry
        if (server[0]=='-')
        {
            const string trunc = server.substr(1);

            // Check if this server is not found in the list.
            // This should never happen if Dim works reliable
            const ServerMap::iterator v = fServerList.find(trunc);
            if (v==fServerList.end())
            {
                wout << kRed << "Server '" << trunc << "' not in list as it ought to be." << endl;
                continue;
            }

            // Remove the server from the server list
            delete v->second.first;
            delete v->second.second;
            fServerList.erase(v);

            // Remove the server from the command list
            ServiceMap::iterator w = fServiceList.find(trunc);
            fServiceList.erase(w);

            wout << " -> " << Time().GetAsStr() << " - " << trunc << "/SERVICE_LIST: Disconnected." << endl;
            //wout << "Remove: " << server << endl;
            continue;
        }

        // If it starts with a + we have to add an entry
        if (server[0]=='+')
        {
            const string trunc = server.substr(1);

            // Check if this server is already in the list.
            // This should never happen if Dim works reliable
            const ServerMap::iterator v = fServerList.find(trunc);
            if (v!=fServerList.end())
            {
                wout << kRed << "Server '" << trunc << "' in list not as it ought to be." << endl;
                continue;
            }

            // Add the new server to the server list
            fServerList[trunc] = make_pair(CreateSL(trunc), CreateFMT(trunc));

            wout << " -> " << Time().GetAsStr() << " - " << trunc << "/SERVICE_LIST: Connected." << endl;
            //wout << "Add   : " << server << endl;
            continue;
        }

        // In any other case we just add the entry to the list
        fServerList[server] = make_pair(CreateSL(server), CreateFMT(server));
        //wout << "Add  0: " << server << endl;
    }
}

// --------------------------------------------------------------------------
//
//! Process an update of the SERVICE_LIST service of the given DimInfo
//!
//! All services found are stored in the fServiceList map to be accessible
//! through the server name. Their format is format is stored in the
//! fFormatList. Note, that the list if only updated. So it will also
//! contain services which are not available anymore. For an up-to-date
//! list of service use fServiceList
//!
//! Only entries matching the fType data member are stored.
//!
//! @todo
//!    Make sure that we do not receive +/- updates on the SERVICE_LIST
//!    like on the SERVER_LIST
//!
void ServiceList::ProcessServiceList(DimInfo &info)
{
    const string str = info.getString();
    if (str.empty())
        return;

    // Get the name of the service
    string buffer = info.getName();

    if (buffer.find("SERVICE_DESC")!=buffer.length()-12)
    {
        // Get the server name from the service name
        const string server = buffer.substr(0, buffer.find_first_of('/'));

        // Initialize the entry with an empty list
        if (str[0]!='+')
            fServiceList[server] = vector<string>();

        // For easy and fast access get the corresponding reference
        vector<string> &list = fServiceList[server];

        // Tokenize the stream into lines
        stringstream stream(str);
        while (getline(stream, buffer, '\n'))
        {
            if (buffer.empty())
                continue;

            // Get the type and compare it with fType
            const string type = buffer.substr(buffer.find_last_of('|')+1);
            if (type!=fType)
                continue;

            // Get format, name and command name
            const string fmt  = buffer.substr(buffer.find_first_of('|')+1, buffer.find_last_of('|')-buffer.find_first_of('|')-1);
            const string name = buffer.substr(buffer.find_first_of('/')+1, buffer.find_first_of('|')-buffer.find_first_of('/')-1);
            const string cmd  = buffer.substr(0, buffer.find_first_of('|'));

            // Add name the the list
            list.push_back(name);

            // Add format to the list
            fFormatList[cmd] = fmt;
        }
    }
    else
    {
        fDescriptionMap.clear();

        stringstream stream(str);
        while (getline(stream, buffer, '\n'))
        {
            if (buffer.empty())
                continue;

            const vector<Description> v = Description::SplitDescription(buffer);

            const string svc = v[0].name;

            fDescriptionMap[svc]  = v[0].comment;
            fDescriptionList[svc] = vector<Description>(v.begin()+1, v.end());
        }
    }
}

// --------------------------------------------------------------------------
//
//! @returns
//!    the list of servers as a vector of strings.
//
vector<string> ServiceList::GetServerList() const
{
    vector<string> v;
    for (ServerMap::const_iterator i=fServerList.begin(); i!=fServerList.end(); i++)
        v.push_back(i->first);

    return v;
}

vector<string> ServiceList::GetServiceList(const std::string &server) const
{
    const ServiceMap::const_iterator m = fServiceList.find(server);
    return m==end() ? vector<string>() : m->second;
}

vector<string> ServiceList::GetServiceList() const
{
    vector<string> vec;
    for (ServerMap::const_iterator i=fServerList.begin(); i!=fServerList.end(); i++)
    {
        const string server = i->first;

        const vector<string> v = GetServiceList(server);

        for (vector<string>::const_iterator s=v.begin(); s<v.end(); s++)
            vec.push_back(server+"/"+*s);
    }

    return vec;
}

// --------------------------------------------------------------------------
//
//! Get the format of a command or service
//!
//! @param service
//!    full qualified service name, e.g. SERVER/EXIT
//!
//! @returns
//!    the format corresponding to the given service. If the service is not
//!    found an empty string is returned.
//
string ServiceList::GetFormat(const string &service) const
{
    const StringMap::const_iterator i = fFormatList.find(service);
    return i==fFormatList.end() ? "" : i->second;
}

// --------------------------------------------------------------------------
//
//! Get the format of a command or service
//!
//! @param server
//!     the server name, e.g. SERVER
//!
//! @param name
//!     the service name, e.g. EXIT
//!
//! @returns
//!    the format corresponding to the given service. If the service is not
//!    found an empty string is returned.
//
string ServiceList::GetFormat(const string &server, const string &name) const
{
    return GetFormat(server+'/'+name);
}

// --------------------------------------------------------------------------
//
//! Get the Description vector of a command or service
//!
//! @param service
//!    full qualified service name, e.g. SERVER/EXIT
//!
//! @returns
//!    a vector of Description objects corresponding to the arguments
//!
//
vector<Description> ServiceList::GetDescriptions(const string &service) const
{
    const DescriptionMap::const_iterator i = fDescriptionList.find(service);
    return i==fDescriptionList.end() ? vector<Description>() : i->second;
}

// --------------------------------------------------------------------------
//
//! Get the Description vector of a command or service
//!
//! @param server
//!     the server name, e.g. SERVER
//!
//! @param name
//!     the service name, e.g. EXIT
//!
//! @returns
//!    a vector of Description objects corresponding to the arguments
//
vector<Description> ServiceList::GetDescriptions(const string &server, const string &name) const
{
    return GetDescriptions(server+'/'+name);
}

// --------------------------------------------------------------------------
//
//! Get a description describing the given command or service if available.
//!
//! @param service
//!    full qualified service name, e.g. SERVER/EXIT
//!
//! @returns
//!    string with the stored comment
//!
//
string ServiceList::GetComment(const string &service) const
{
    const StringMap::const_iterator i = fDescriptionMap.find(service);
    return i==fDescriptionMap.end() ? "" : i->second;
}

// --------------------------------------------------------------------------
//
//! Get a description describing the given command or service if available.
//!
//! @param server
//!     the server name, e.g. SERVER
//!
//! @param name
//!     the service name, e.g. EXIT
//!
//! @returns
//!    string with the stored comment
//
string ServiceList::GetComment(const string &server, const string &name) const
{
    return GetComment(server+"/"+name);
}

// --------------------------------------------------------------------------
//
//! Checks if a server is existing.
//!
//! @param name
//!     Name of a server, e.g. DIS_DNS
//!
//! @returns
//!    true if the server is found in fServiceList, false otherwise
//
bool ServiceList::HasServer(const string &name) const
{
    return fServiceList.find(name)!=end();
}

// --------------------------------------------------------------------------
//
//! Checks if a given service is existing.
//!
//! @param server
//!     Name of a server, e.g. DIS_DNS
//!
//! @param service
//!     Name of a service, e.g. EXIT
//!
//! @returns
//!    true if the service is found in fServiceList, false otherwise.
//
bool ServiceList::HasService(const string &server, const string &service) const
{
    ServiceMap::const_iterator v = fServiceList.find(server);
    if (v==end())
        return false;

    const vector<string> &w = v->second;
    return find(w.begin(), w.end(), service)!=w.end();
}

bool ServiceList::HasService(const string &svc) const
{
    const size_t p = svc.find_first_of('/');
    if (p==string::npos)
        return false;

    return HasService(svc.substr(0, p), svc.substr(p+1));
}

// --------------------------------------------------------------------------
//
//! Returns an iterator to the begin of ta vector<string> which will
//! contain the service names of the given server.
//!
//! @param server
//!     Name of a server, e.g. DIS_DNS
//!
//! @returns
//!    an iterator to the vector of strings with the service names
//!    for the given server. If none is found it returns
//!    vector<string>().end()
//
vector<string>::const_iterator ServiceList::begin(const string &server) const
{
    ServiceMap::const_iterator i = fServiceList.find(server);
    if (i==end())
        return vector<string>().end();

    return i->second.begin();
}

// --------------------------------------------------------------------------
//
//! Returns an iterator to the end of ta vector<string> which will
//! contain the service names of the given server.
//!
//! @param server
//!     Name of a server, e.g. DIS_DNS
//!
//! @returns
//!     an iterator to the vector of strings with the service names
//!    for the given server. If none is found it returns
//!    vector<string>().end()
//
vector<string>::const_iterator ServiceList::end(const string &server) const
{
    ServiceMap::const_iterator i = fServiceList.find(server);
    if (i==end())
        return vector<string>().end();

    return i->second.end();
}


// --------------------------------------------------------------------------
//
//! Print the stored list of servers to the given stream.
//!
//! @param out
//!    ostream to which the server names stored in fServerList are dumped
//!
void ServiceList::PrintServerList(ostream &out) const
{
    out << endl << kBold << "ServerList:" << endl;

    for (ServerMap::const_iterator i=fServerList.begin(); i!=fServerList.end(); i++)
    {
        const string &server = i->first;
        DimInfo *ptr1 = i->second.first;
        DimInfo *ptr2 = i->second.second;

        out << " " << server << " " << ptr1->getName() << "|" << ptr2->getName() << endl;
    }
    out << endl;
}

// --------------------------------------------------------------------------
//
//! Print the stored list of services to the given stream.
//!
//! @param out
//!    ostream to which the services names stored in fServiceList are dumped
//!
void ServiceList::PrintServiceList(ostream &out) const
{
    out << endl << kBold << "ServiceList:" << endl;

    for (ServiceMap::const_iterator i=fServiceList.begin(); i!=fServiceList.end(); i++)
    {
        const string &server = i->first;
        const vector<string> &lst = i->second;

        out << " " << server << endl;

        for (vector<string>::const_iterator j=lst.begin(); j!=lst.end(); j++)
            out << "  " << *j << " [" << GetFormat(server, *j) << "]" << endl;
    }
    out << endl;
}

// --------------------------------------------------------------------------
//
//! Print the full available documentation (description) of all available
//! services or comments to the the given stream.
//!
//! @param out
//!    ostream to which the output is send.
//!
//! @param serv
//!    if a server is given, only the information for this server is printed
//!
//! @param service
//!    if a service is given, only information for this service is printed
//
int ServiceList::PrintDescription(std::ostream &out, const string &serv, const string &service) const
{
    int rc = 0;
    for (ServiceMap::const_iterator i=fServiceList.begin(); i!=fServiceList.end(); i++)
    {
        const string &server = i->first;

        if (!serv.empty() && server!=serv)
            continue;

        out << kRed << "----- " << server << " -----" << endl;

        const vector<string> &lst = i->second;
        for (vector<string>::const_iterator s=lst.begin(); s!=lst.end(); s++)
        {
            if (!service.empty() && *s!=service)
                continue;

            rc++;

            out << " " << *s;

            const string fmt = GetFormat(server, *s);
            if (!fmt.empty())
                out << '[' << fmt << ']';

            const string svc = server + '/' + *s;

            const DescriptionMap::const_iterator v = fDescriptionList.find(svc);
            if (v==fDescriptionList.end())
            {
                out << endl;
                continue;
            }

            for (vector<Description>::const_iterator j=v->second.begin();
                 j!=v->second.end(); j++)
                out << " <" << j->name << ">";
            out << endl;

            const StringMap::const_iterator d = fDescriptionMap.find(svc);
            if (d!=fDescriptionMap.end() && !d->second.empty())
                out << "    " << d->second << endl;

            for (vector<Description>::const_iterator j=v->second.begin();
                 j!=v->second.end(); j++)
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
//! Request a SERVER_LIST from the name server and a SERVICE_LIST from all
//! servers in the list. Dumps the result to the given ostream.
//!
//! @param out
//!    ostream to which the received info is redirected
//!
void ServiceList::DumpServiceList(ostream &out)
{
    DimCurrentInfo info1("DIS_DNS/SERVER_LIST", const_cast<char*>(""));

    stringstream stream(info1.getString());

    string buffer;
    while (getline(stream, buffer, '|'))
    {
        const string server = buffer.substr(0, buffer.find_first_of('@'));
        if (server.empty())
            continue;

        out << kBold << " " << server << endl;

        DimCurrentInfo info2((server+"/SERVICE_LIST").c_str(), const_cast<char*>(""));

        string buffer2;

        stringstream stream2(info2.getString());
        while (getline(stream2, buffer2, '\n'))
        {
            if (buffer2.empty())
                continue;

            out << "  " << buffer2 << endl;
        }
    }
}

// --------------------------------------------------------------------------
//
//! Tries to send a dim command according to the arguments.
//! The command given is evaluated according to the available format string.
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
//!    If parsing the string was successfull and the command exists in the
//!    network true is returned, false otherwise.
//!
bool ServiceList::SendDimCommand(ostream &lout, const string &server, const string &str) const
{
    // Find the delimiter between the command name and the data
    size_t p0 = str.find_first_of(' ');
    if (p0==string::npos)
        p0 = str.length();

    // Get just the command name separated from the data
    const string name = str.substr(0, p0);

    // Compile the command which will be sent to the state-machine
    const string cmd = server + '/' + name;

    if (!HasService(server, name))
    {
        lout << kRed << "Unkown command '" << cmd << "'" << endl;
        return false;
    }

    // Get the format of the event data
    const string fmt = GetFormat(cmd);

    // Convert the user entered data according to the format string
    // into a data block which will be attached to the event
    const Converter conv(lout, fmt, false);
    if (!conv)
    {
        lout << kRed << "Couldn't properly parse the format... ignored." << endl;
        return false;
    }

    try
    {
        lout << kBlue << cmd;
        const vector<char> v = conv.GetVector(str.substr(p0));
        lout << endl;

        const int rc = DimClient::sendCommand(cmd.c_str(), (void*)v.data(), v.size());
        if (rc)
            lout << kGreen << "Command " << cmd << " emitted successfully to DimClient." << endl;
        else
            lout << kRed << "ERROR - Sending command " << cmd << " failed." << endl;
    }
    catch (const std::runtime_error &e)
    {
        lout << endl << kRed << e.what() << endl;
        return false;
    }

    return true;
}
