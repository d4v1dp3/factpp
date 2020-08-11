#ifndef FACT_DimServiceInfoList
#define FACT_DimServiceInfoList

#include <map>
#include <vector>
#include <string>

#include "State.h"
#include "Description.h"
#include "DimServerList.h"

class DimInfo;
class DimServiceInfoListImp;

class DimServiceInfoList : public DimServerList
{
public:

    typedef std::map<const std::string, std::vector<DimInfo*>> ServiceInfoList;

    //                   Format   IsCmd
    typedef std::pair<std::string, bool> ServiceType;

    //                 name of service  Format/description
    typedef std::map<const std::string, ServiceType> TypeList;

    //                 ServiceName                 comment      list of descriptions
    typedef std::map<const std::string, std::pair<std::string, std::vector<Description>>> DescriptionList;

    struct ServerInfo
    {
        TypeList           first;   /// Format and description of the service
        DescriptionList    second;  /// Description of the arguments
        std::vector<State> third;   /// Available states of the server
    };

    //                   Server         ServerInfo
    typedef std::map<const std::string, ServerInfo> ServiceList;

private:
    DimServiceInfoListImp *fList;
    ServiceInfoList fServiceInfoList; /// A map storing the service description to retrieve all informations
    ServiceList     fServiceList;     /// A mal containing all the available informations

    DimInfo *CreateDimInfo(const std::string &str, const std::string &svc) const;
    DimInfo *CreateSL(const std::string &str) const { return CreateDimInfo(str, "SERVICE_LIST"); }
    DimInfo *CreateFMT(const std::string &str) const { return CreateDimInfo(str, "SERVICE_DESC"); }
    DimInfo *CreateDS(const std::string &str) const { return CreateDimInfo(str, "STATE_LIST"); }

public:
    void AddServer(const std::string &s);
    void RemoveServer(std::string);
    void RemoveAllServers();

protected:
    void infoHandler();

public:
    DimServiceInfoList(DimServiceInfoListImp *list);
    ~DimServiceInfoList() {  }

    std::vector<std::string> GetServiceList(bool iscmd=false) const;
    std::vector<std::string> GetServiceList(const std::string &server, bool iscmd=false) const;

    std::vector<Description> GetDescription(const std::string &server, const std::string &service) const;
    std::vector<State>       GetStates(const std::string &server) const;
    State                    GetState(const std::string &server, int state) const;

    int IsCommand(const std::string &server, const std::string &service) const;

    int PrintDescription(std::ostream &out, bool iscmd, const std::string &serv="", const std::string &service="") const;
    int PrintStates(std::ostream &out, const std::string &serv="") const;

    bool SendDimCommand(std::ostream &lout, const std::string &server, const std::string &str) const;
    void SendDimCommand(const std::string &server, std::string str, std::ostream &lout) const;
    void SendDimCommand(const std::string &server, const std::string &str) const;
};

class DimServiceInfoListImp : public DimServerListImp
{
public:
    DimServiceInfoList fInfo;

protected:
    virtual void AddServer(const std::string &s) { fInfo.AddServer(s); }
    virtual void RemoveServer(std::string s)     { fInfo.RemoveServer(s); }
    virtual void RemoveAllServers()              { fInfo.RemoveAllServers(); }

public:
    virtual void AddService(const std::string &, const std::string &, const std::string &, bool) { }
    virtual void RemoveService(std::string, std::string, bool) { }
    virtual void RemoveAllServices(const std::string &) { }
    virtual void AddDescription(const std::string &, const std::string &, const std::vector<Description> &) { }
    virtual void AddStates(const std::string &, const std::vector<State> &) { }

public:
    DimServiceInfoListImp() : fInfo(this) { }
    ~DimServiceInfoListImp() { fInfo.RemoveAllServers(); }

    std::vector<std::string> GetServiceList(bool iscmd=false) const
    { return fInfo.GetServiceList(iscmd); }
    std::vector<std::string> GetServiceList(const std::string &server, bool iscmd=false) const
    { return fInfo.GetServiceList(server, iscmd); }

    std::vector<std::string> GetCommandList() const { return GetServiceList(true); }
    std::vector<std::string> GetCommandList(const std::string &server) const { return GetServiceList(server, true); }

    std::vector<Description> GetDescription(const std::string &server, const std::string &service) const
    { return fInfo.GetDescription(server, service); }
    std::vector<State>       GetStates(const std::string &server) const
    { return fInfo.GetStates(server); }
    State                    GetState(const std::string &server, int state) const
    { return fInfo.GetState(server, state); }

    int IsCommand(const std::string &server, const std::string &service) const
    { return fInfo.IsCommand(server, service); }

    int PrintDescription(std::ostream &out, bool iscmd, const std::string &serv="", const std::string &service="") const
    { return fInfo.PrintDescription(out, iscmd, serv, service); }
    int PrintStates(std::ostream &out, const std::string &serv="") const
    { return fInfo.PrintStates(out, serv); }

    bool SendDimCommand(std::ostream &lout, const std::string &server, const std::string &str) const
    { return fInfo.SendDimCommand(lout, server, str); }
    void SendDimCommand(const std::string &server, std::string str, std::ostream &lout) const
    { return fInfo.SendDimCommand(server, str, lout); }
    void SendDimCommand(const std::string &server, const std::string &str) const
    { return fInfo.SendDimCommand(server, str); }
};



// ***************************************************************************
/** @fn DimServiceInfoList::AddService(const std::string &server, const std::string &service, const std::string &fmt, bool iscmd)

This virtual function is called as a callback whenever a new service appears.
The default is to do nothing.

@param server
   Server name of the server at which the new service appeared

@param service
   Service name which appeared

@param fmt
   Dim format string associated with the service

@param iscmd
   boolean which is true if it is a command, and false if it is a service


**/
// ***************************************************************************
/** @fn DimServiceInfoList::RemoveService(const std::string &server, const std::string &service, bool iscmd)

This virtual function is called as a callback whenever a service disappears.
The default is to do nothing.

@param server
   Server name of the server at which the new service appeared

@param service
   Service name which appeared

@param iscmd
   boolean which is true if it is a command, and false if it is a service


**/
// ***************************************************************************
/** @fn DimServiceInfoList::RemoveAllServices(const std::string &server)

This virtual function is called as a callback whenever a server disappears,
or the list must be cleared because a new list has been retrieved.
The default is to do nothing.

@param server
   Server name of the server at which the new service appeared


**/
// ***************************************************************************
/** @fn DimServiceInfoList::AddDescription(const std::string &server, const std::string &service, const std::vector<Description> &vec)

This virtual function is called as a callback whenever a new description
was received.
The default is to do nothing.

@param server
   Server name of the server for which something was received

@param service
   Service name for which the description weer received

@param vec
   vector<Description> associated with this service. The first entry in the
   list belongs to the service itself, each consecutive entry to its arguments


**/
// ***************************************************************************
/** @fn DimServiceInfoList::AddStates(const std::string &server, const std::vector<State> &vec)

This virtual function is called as a callback whenever a new list of states
was received.
The default is to do nothing.

@param server
   Server name for which the list was received

@param vec
   vector<State> associated with this server.

**/
// ***************************************************************************

#endif
