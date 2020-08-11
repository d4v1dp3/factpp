// **************************************************************************
/** @class DimServerList

@brief Maintains a list of all servers based on DIS_DNS/SERVER_LIST

This class describes to the service DIS_DNS/SERVER_LIST of the name server.
Thus it always contains an up-to-date list of all servers connected
to the dns server.

Whenever a server is added or removed, or all servers are removed from the
#list (the dns itself went offline), the follwoing virtual functions are
called:

- virtual void AddServer(const std::string &)
- virtual void RemoveServer(const std::string)
- virtual void RemoveAllServers()

If overwritten the implementations of the base class doesn't need to be
called, because it's empty.

@Bugs When a server silently disappears and reappears the service has
not correctly been deleted from the list and an exception is thrown.
It is not clear what a better handling here is. Maybe a server is
first removed from the list before it gets re-added if already in the list?

*/
// **************************************************************************
#include "DimServerList.h"

#include <sstream>
#include <algorithm>
#include <stdexcept>

using namespace std;



// --------------------------------------------------------------------------
//
//! Constructs the DimServerList. Subscribes a DimInfo to
//! DIS_DNS/SERVER_LIST.
//
DimServerList::DimServerList(DimServerListImp *list) : fList(list),
    fDimServers("DIS_DNS/SERVER_LIST", const_cast<char*>(""), this)
{
}

// --------------------------------------------------------------------------
//
//! Whenever the service with the server list is updated this functions
//! changes the contents of the list and calls RemoveAllServers(),
//! RemoveServer() and AddServer() where appropriate.
//
void DimServerList::infoHandler()
{
    if (getInfo()!=&fDimServers)
        return;

    // Get the received string from the handler
    const string str = fDimServers.getString();

    // Check if it starts with + or -
    if (str[0]!='-' && str[0]!='+')
    {
        fList->RemoveAllServers();
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
            const ServerList::iterator v = find(fServerList.begin(), fServerList.end(), trunc);
            if (v==fServerList.end())
            {
                //stringstream err;
                //err << "DimServerList: Server '" << trunc << "' not in list as it ought to be.";
                //throw runtime_error(err.str());
            }

            fList->RemoveServer(trunc);
            fServerList.erase(v);

            continue;
        }

        // If it starts with a + we have to add an entry
        if (server[0]=='+')
        {
            const string trunc = server.substr(1);

            // Check if this server is already in the list.
            // This should never happen if Dim works reliable
            const ServerList::iterator v = find(fServerList.begin(), fServerList.end(), trunc);
            if (v!=fServerList.end())
            {
                fList->RemoveServer(trunc);
                fServerList.erase(v);

                //stringstream err;
                //err << "DimServerList: Server '" << trunc << "' in list not as it ought to be.";
                //throw runtime_error(err.str());
            }

            fServerList.push_back(trunc);
            fList->AddServer(trunc);

            continue;
        }

        // In any other case we just add the entry to the list
        fServerList.push_back(server);
        fList->AddServer(server);
    }
}

// --------------------------------------------------------------------------
//
//! @param server
//!     server-name to check for
//!
//! @returns
//!     whether the server with the given name is online or not
//!
bool DimServerList::HasServer(const std::string &server) const
{
    return find(fServerList.begin(), fServerList.end(), server)!=fServerList.end();
}
