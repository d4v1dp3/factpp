// **************************************************************************
/** @class DimDescriptionService

@brief A DimService which broadcasts descriptions for services and commands

The DimDescriptionService creates a service with the name of the server like
SERVER/SERVICE_DESC. This is meant in addition to the SERVICE_LIST service
of each node to contain a description of the service and its arguments.

Assume you have created a service (or command) with the format I:2;F:1
a valid description string would look like

   Description|int[addr]:Address range (from - to)|val[byte]:Value to be set

Description is a general description of the command or service itself,
int and val are the names of the arguments (e.g. names of FITS columns),
addr and byte have the meaning of a unit (e.g. unit of FITS column)
and the text after the colon is a description of the arguments
(e.g. comment of a FITS column). The description must not contain a
line-break character \n.

You can omit either the name, the unit or the comment or any combination of them.
The descriptions of the individual format strings are separated by a vertical line.

The description should contain as many descriptions as format chunks, e.g.

 - I:1          should contain one description chunks
 - I:1;F:1      should contain two description chunks
 - I:2;F:1      should contain two description chunks
 - I:2;I:1;F:1  should contain three description chunks

*/
// **************************************************************************
#include "DimDescriptionService.h"

#include <stdexcept>

#include "dis.hxx"
#include "Time.h"

using namespace std;

DimService *DimDescriptionService::fService = 0;
int         DimDescriptionService::fCount   = 0;
std::string DimDescriptionService::fData    = "";

set<string> DimDescribedService::fServices;

// --------------------------------------------------------------------------
//
//! When the constructor is first called, a service with the name
//! SERVER/SERVICE_DESC is created. The server name SERVER is retrieved
//! from DimServer::itsName. If DimServer::itsName is empty, the
//! server name is extracted from the given name as the part before the
//! first '/'. A string "name=format\n" is added to fData and stored
//! in fDescription.
//!
//! A counter fCount for the number of instantiations is increased.
//!
//! @param name
//!     The name of the service or command to be described, e.g. SERVER/COMMAND
//!
//! @param desc
//!     A description string. For details see class reference
//!
//! @throws
//!     If a server name couldn't be reliably determined a logic_error
//!     exception is thrown; if the given description contains a '\n'
//!     also a logic_error is thrown.
//
DimDescriptionService::DimDescriptionService(const std::string &name, const std::string &desc)
{
    string server = DimServer::itsName ? DimServer::itsName : "";
    if (server.empty())
    {
        const size_t p = name.find_first_of('/');
        if (p==string::npos)
            throw logic_error("Could not determine server name");

        server = name.substr(0, p);
    }

    if (desc.find_first_of('\n')!=string::npos)
            throw logic_error("Description for "+name+" contains '\\n'");

    if (!fService)
    {
        fService = new DimService((server+"/SERVICE_DESC").c_str(), const_cast<char*>(""));
        fData =
            server + "/SERVICE_DESC"
            "=Descriptions of services or commands and there arguments"
            "|Description[string]:For a detailed "
            "explanation of the descriptive string see the class reference "
            "of DimDescriptionService.\n" +
            server + "/CLIENT_LIST"
            "=Native Dim service: A list of all connected clients\n" +
            server + "/VERSION_NUMBER"
            "=Native Dim service: Version number of Dim in use"
            "|DimVer[int]:Version*100+Release (e.g. V19r17 = 1917)\n" +
            server + "/EXIT"
            "=This is a native Dim command: Exit program"
            "remotely. FACT++ programs use the given number as return code."
            "|Rc[int]:Return code, under normal circumstances this should be 0 or 1 (42 will call exit() directly, 0x42 will call abort() directly.\n" +
            server + "/SERVICE_LIST"
            "=Native Dim service: List of services, commands and formats"
            "|ServiceList[string]:For details see the Dim manual.\n";
    }


    fCount++;

    fDescription = name + '=' + desc;

    if (fData.find(fDescription+'\n')!=std::string::npos)
        return;

    fData += fDescription + '\n';

    const Time t;
    fService->setTimestamp(t.Time_t(), t.ms());
    fService->setData(const_cast<char*>(fData.c_str()));
    fService->updateService();
}


// --------------------------------------------------------------------------
//
//! If fDescription is found in fData it is removed from fData.
//! The counter fCount is decreased and fService deleted if the counter
//! reached 0.
//
DimDescriptionService::~DimDescriptionService()
{
    const size_t pos = fData.find(fDescription+'\n');
    if (pos!=std::string::npos)
        fData.replace(pos, fDescription.size()+1, "");

    if (--fCount>0)
        return;

    delete fService;
    fService=0;
}

void DimDescribedService::setTime(const Time &t)
{
    setTimestamp(t.Time_t(), t.ms());
}

void DimDescribedService::setTime()
{
    setTime(Time());
}

int DimDescribedService::Update(const Time &t)
{
    setTime(t);
    return updateService();
}

int DimDescribedService::Update()
{
    return Update(Time());
}

int DimDescribedService::Update(const string &data)
{
    return Update(data.data());
}

int DimDescribedService::Update(const char *data)
{
    setData(data);
    return Update();
}
