// **************************************************************************
/** @class DimErrorRedirecter

@brief A base class taking care of padding, exit handler and error handlers

This class first switches off padding for the DimServer and the DimClient
(dis and dic). Furthermore, it redirects both error handlers to the
DimErrorRedirecter. Redirect the exit handler.

Only one instance of this class is allowed, since all Dim handlers are
global.

In the destructor of the class the handlers are correctly restored.
The padding setup is kept.

For FACT++ all Dim data is transmitted without padding!

To catch the error messages overwrite the errorHandler. The errorHandler
of the DimErrorRedirecter redirects the error messages to the logging
stream given in the constructor.

To catch the exit requests overwrite the exitHandler.

*/
// **************************************************************************
#include "DimErrorRedirecter.h"

#include <dic.hxx>

#include "WindowLog.h"
#include "MessageImp.h"

using namespace std;

int DimErrorRedirecter::fDimErrorRedireterCnt = 0;

// --------------------------------------------------------------------------
//
//! - disable padding for dim server and dim client
//! - redirect DimClient error handler
//! - redirect DimServer error handler
//! - set exit handler of DimServer
//
DimErrorRedirecter::DimErrorRedirecter(MessageImp &imp) : fMsg(imp)
{
    if (fDimErrorRedireterCnt++)
        throw logic_error("ERROR - More than one instance of MyHandlers.");

    dic_disable_padding();
    dis_disable_padding();

    DimServer::addExitHandler(this);
    DimServer::addErrorHandler(this);
    DimClient::addErrorHandler(this);
}

// --------------------------------------------------------------------------
//
//! - reset DimClient error handler
//! - reset DimServer error handler
//! - reset exit handler of DimServer
//
DimErrorRedirecter::~DimErrorRedirecter()
{
    DimClient::addErrorHandler(0);
    DimServer::addErrorHandler(0);
    DimServer::addExitHandler(0);
}

void DimErrorRedirecter::errorHandler(int severity, int code, char *msg)
{
    static const string id = "<DIM> ";

    switch (severity)
    {
    case DIM_FATAL:   fMsg.Fatal(id+msg); break;
    case DIM_ERROR:   fMsg.Error(id+msg); break;
    case DIM_WARNING: fMsg.Warn(id+msg);  break;
    case DIM_INFO:    fMsg.Info(id+msg);  break;
    default:
        ostringstream str;
        str << "DIM message with unknown severity (" << severity << "): ";
        str << msg << " (" << code << ")";
        fMsg.Warn(str);
        break;
    }

    /*
     DIMDNSUNDEF	DIM_FATAL	DIM_DNS_NODE undefined
     DIMDNSREFUS	DIM_FATAL	DIM_DNS refuses connection
     DIMDNSDUPLC	DIM_FATAL	Service already exists in DNS
     DIMDNSEXIT	        DIM_FATAL	DNS requests server to EXIT
     DIMDNSTMOUT	DIM_WARNING	Server failed sending Watchdog

     DIMDNSCNERR	DIM_ERROR	Connection to DNS failed
     DIMDNSCNEST	DIM_INFO	Connection to DNS established

     DIMSVCDUPLC	DIM_ERROR	Service already exists in Server
     DIMSVCFORMT	DIM_ERROR	Bad format string for service
     DIMSVCINVAL	DIM_ERROR	Invalid Service ID

     DIMTCPRDERR	DIM_ERROR	TCP/IP read error
     DIMTCPWRRTY	DIM_WARNING	TCP/IP write error - Retrying
     DIMTCPWRTMO	DIM_ERROR	TCP/IP write error - Disconnected
     DIMTCPLNERR	DIM_ERROR	TCP/IP listen error
     DIMTCPOPERR	DIM_ERROR	TCP/IP open server error
     DIMTCPCNERR	DIM_ERROR	TCP/IP connection error
     DIMTCPCNEST	DIM_INFO	TCP/IP connection established
     */
}

