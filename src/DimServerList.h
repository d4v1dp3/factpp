#ifndef FACT_DimServerList
#define FACT_DimServerList

#include <vector>
#include <string>

#include "dic.hxx"

class DimServerListImp;

class DimServerList : public DimInfoHandler
{
public:
    typedef std::vector<std::string> ServerList;

private:
    DimServerListImp *fList;
    ServerList fServerList;  /// A list with the available servers
    DimInfo    fDimServers;  /// A DimInfo to retrieve the SERVER_LIST from teh DNS server

protected:
    void infoHandler();

public:
    DimServerList(DimServerListImp *list);

    /// @returns a reference to the list of servers
    const ServerList &GetServerList() const { return fServerList; }

    /// @returns whether the given server is in the list or not
    /// @param server string with the server which availability should be checked
    bool HasServer(const std::string &server) const;
};

class DimServerListImp
{
    DimServerList fList;

public:
    DimServerListImp() : fList(this) { }
    virtual ~DimServerListImp() { }

    virtual void AddServer(const std::string &) { };
    virtual void RemoveServer(std::string) { };
    virtual void RemoveAllServers() { };

    /// @returns a reference to the list of servers
    const DimServerList::ServerList &GetServerList() const { return fList.GetServerList(); }

    /// @returns whether the given server is in the list or not
    /// @param server string with the server which availability should be checked
    bool HasServer(const std::string &server) const { return fList.HasServer(server); }
};

#endif

// ***************************************************************************
/** @fn DimServerList::AddServer(const std::string &server)

This virtual function is called as a callback whenever a new server appears
to be available on the dns.

The default is to do nothing.

@param server
   Server name of the server which was added

**/
// ***************************************************************************
/** @fn DimServerList::RemoveServer(const std::string &server)

This virtual function is called as a callback whenever a server disappears
from the dns.

The default is to do nothing.

@param server
   Server name of the server which was removed


**/
// ***************************************************************************
/** @fn DimServerList::RemoveAllServers()

This virtual function is called as a callback whenever a all servers
disappear, e.g. the dns has vanished.

The default is to do nothing.


**/
// ***************************************************************************
