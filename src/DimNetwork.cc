#include "DimNetwork.h"

StateClient::StateClient(const std::string &name, MessageImp &imp) :
    MessageDimRX(name, imp), fState(-2),
    fInfoState((name + "/STATE").c_str(), (void*)NULL, 0, this)
{
}

// --------------------------------------------------------------------------
//
//! Extract the information about the state and its message. Store the
//! state and redirect the message to fMsg.
//
void StateClient::infoHandler()
{
    DimInfo *curr = getInfo(); // get current DimInfo address
    if (!curr)
        return;

    if (curr==&fInfoState)
    {
        const bool disconnected = fInfoState.getSize()==0;

        // Make sure getTimestamp is called _before_ getTimestampMillisecs
        const int tsec = fInfoState.getTimestamp();
        const int tms  = fInfoState.getTimestampMillisecs();

        fState     = disconnected ? -2 : fInfoState.getQuality();
        fStateTime = Time(tsec, tms*1000);

        const string name = fInfoState.getName();

        fMsg.StateChanged(fStateTime, name.substr(0, name.length()-6),
                          disconnected ? "" : fInfoState.getString(), fState);

        return;
    }

    MessageDimRX::infoHandler();
}

// ==========================================================================

// --------------------------------------------------------------------------
//
//! Delete all StateClient objects from teh list and clear the list.
//
void DimNetwork::DeleteClientList()
{
    for (ClientList::iterator i=fClientList.begin();
         i!=fClientList.end(); i++)
        delete i->second;

    fClientList.clear();
}

// --------------------------------------------------------------------------
//
//! Adds the StateClient for the given server. Don't forget to
//! call this function if it is overwritten in a derived class.
//!
//! @param s
//!     server which should be added
//!
//! @throws
//!     a runtime_error is the server is already in the list
//
void DimNetwork::AddServer(const string &s)
{
    DimServiceInfoListImp::AddServer(s);
    if (s=="DIM_DNS")
        return;

    // Check if this server is already in the list.
    // This should never happen if Dim works reliable
    const ClientList::iterator v = fClientList.find(s);
    if (v!=fClientList.end())
    {
        stringstream err;
        err << "Server '" << s << "' in list not as it ought to be.";
        throw runtime_error(err.str());
    }

    // Add the new server to the server list
    fClientList[s] = new StateClient(s, *this);
}

// --------------------------------------------------------------------------
//
//! Removes the StateClient for the given server. Don't forget to
//! call this function if it is overwritten in a derived class.
//!
//! @param s
//!     server which should be removed
//!
//! @throws
//!     a runtime_error is the server to be removed is not in the list
//
void DimNetwork::RemoveServer(string s)
{
    DimServiceInfoListImp::RemoveServer(s);
    if (s=="DIM_DNS")
        return;

    const ClientList::iterator v = fClientList.find(s);
    if (v==fClientList.end())
    {
        stringstream err;
        err << "Server '" << s << "' not in list as it ought to be.";
        throw runtime_error(err.str());
    }

    // Remove the server from the server list
    delete v->second;

    fClientList.erase(v);
}

// --------------------------------------------------------------------------
//
//! RemovesAll StateClients. Don't forget to call this function if it
//! is overwritten in a derived class.
//!
void DimNetwork::RemoveAllServers()
{
    DimServiceInfoListImp::RemoveAllServers();
    DeleteClientList();
}

// --------------------------------------------------------------------------
//
//! @param server
//!    server for which the current state should be returned
//!
//! @returns
//!    the current state of the given server, -2 if the server was not found
//!
int DimNetwork::GetCurrentState(const string &server) const
{
    const ClientList::const_iterator v = fClientList.find(server);
    return v==fClientList.end() ? -2 : v->second->GetState();
}
