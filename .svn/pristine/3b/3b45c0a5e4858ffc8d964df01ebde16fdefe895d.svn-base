#ifndef FACT_ServiceList
#define FACT_ServiceList

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "dic.hxx"
#include "dis.hxx"

#include "Description.h"

class ServiceList : public DimClient
{
public:
    typedef std::map<const std::string, std::pair<DimInfo*, DimInfo*>> ServerMap;
    typedef std::map<const std::string, std::vector<std::string>>      ServiceMap;
    typedef std::map<const std::string, std::string>                   StringMap;
    typedef std::map<const std::string, std::vector<Description>>      DescriptionMap;

private:
    std::ostream &wout;        /// stream for redirection of the output

    DimInfo fDimServers;       /// A DimInfo to retrieve the SERVER_LIST from teh DNS server

    const std::string fType;   /// A filter for the type of the services to be collected, e.g. CMD

    ServerMap      fServerList;      /// A map storing server names and a DimInfo for their SERVICE_LIST
    ServiceMap     fServiceList;     /// A map storing server names and vector with all their available commands
    StringMap      fFormatList;      /// A map storing all commands and their format strings
    StringMap      fDescriptionMap;  /// A map storing all descriptions for commands and services
    DescriptionMap fDescriptionList; /// A map storing all descriptions for arguments of command and services

    DimInfoHandler *fHandler;  /// A callback to signal updates

    DimInfo *CreateDimInfo(const std::string &str, const std::string &svc) const;

    DimInfo *CreateSL(const std::string &str) const { return CreateDimInfo(str, "SERVICE_LIST"); }
    DimInfo *CreateFMT(const std::string &str) const { return CreateDimInfo(str, "SERVICE_DESC"); }

    void ProcessServerList();
    void ProcessServiceList(DimInfo &info);

    void infoHandler();

public:
    ServiceList(const char *type, std::ostream &out=std::cout);
    ServiceList(std::ostream &out=std::cout);
    ~ServiceList();

    void SetHandler(DimInfoHandler *handler=0) { fHandler=handler; }

    std::vector<std::string> GetServerList() const;
    std::vector<std::string> GetServiceList() const;
    std::vector<std::string> GetServiceList(const std::string &server) const;

    bool HasServer(const std::string &name) const;
    bool HasService(const std::string &server, const std::string &service) const;
    bool HasService(const std::string &service) const;

    ServiceMap::const_iterator begin() const { return fServiceList.begin(); }
    ServiceMap::const_iterator end() const  { return fServiceList.end(); }

    std::vector<std::string>::const_iterator begin(const std::string &server) const;
    std::vector<std::string>::const_iterator end(const std::string &server) const;

    std::string GetFormat(const std::string &server, const std::string &name) const;
    std::string GetFormat(const std::string &service) const;

    std::string GetComment(const std::string &server, const std::string &name) const;
    std::string GetComment(const std::string &service) const;

    std::vector<Description> GetDescriptions(const std::string &server, const std::string &name) const;
    std::vector<Description> GetDescriptions(const std::string &service) const;

    void PrintServerList(std::ostream &out) const;
    void PrintServiceList(std::ostream &out) const;
    int  PrintDescription(std::ostream &out, const std::string &serv="", const std::string &svc="") const;
    void PrintServerList() const { PrintServerList(wout); }
    void PrintServiceList() const { PrintServiceList(wout); }
    int  PrintDescription(const std::string &serv="", const std::string &svc="") const { return PrintDescription(wout, serv, svc); }

    static void DumpServiceList(std::ostream &out);
    void DumpServiceList() const { DumpServiceList(wout); }

    bool SendDimCommand(std::ostream &lout, const std::string &server, const std::string &str) const;
    bool SendDimCommand(const std::string &server, const std::string &str) const
    {
        return SendDimCommand(std::cout, server, str);
    }
};

#endif
