#include "MessageDim.h"

#include "tools.h"
#include "Time.h"

using namespace std;

// **************************************************************************
/** @class MessageDimTX

@brief Based on MessageImp, redirects log-output to a Dim service MESSAGE

This is a special DimService which offers SERVER/MESSAGE to the DimNetwork
and redirects output issued via its base-class MessageImp to the Dim
service. The severity of the message is send as qualiy of service of
the service message.

@section Examples

 - A simple and usefull example can be found in \ref log.cc and \ref logtime.cc

**/
// **************************************************************************

// --------------------------------------------------------------------------
//
//! Constructs a DimService with the name SERVER/MESSAGE. And passes the
//! given ostream down to the MessageImp base.
//!
//! @param name
//!    Name of the message server to which we want to subscribe, e.g. DRIVE
//!
//! @param out
//!    ostream passed to MessageImp. It is used to redirect the output to.
//
MessageDimTX::MessageDimTX(const std::string &name, std::ostream &out)
    : DimDescribedService(name + "/MESSAGE", const_cast<char*>("C"),
                          "A general logging service providing a quality of service (severity)"
                          "|Message[string]:The message"),
    MessageImp(out), fDebug(false),
    fMsgQueue(std::bind(&MessageDimTX::UpdateService, this, placeholders::_1))
{
    // This is a message which will never arrive because
    // the time to establish a client-sever connection is
    // too short.
    Message("MessageDimTX started.");
}

// --------------------------------------------------------------------------
//
//!
//
MessageDimTX::~MessageDimTX()
{
    // Everything here will never be sent by dim because the
    // dim services have been stopped already. This is necessary,
    // to have them available already during startup
    Message("MessageDimTX shutting down ["+to_string(fMsgQueue.size())+"]");
    fMsgQueue.wait();
}

bool MessageDimTX::UpdateService(const tuple<Time,string,int> &data)
{
    setData(get<1>(data));
    setQuality(get<2>(data));

    const int rc = DimDescribedService::Update(get<0>(data));
    if (rc==0 && fDebug)
        Out() << " !! " << get<0>(data).GetAsStr() << " - Sending failed!" << endl;

    return true;
}

// --------------------------------------------------------------------------
//
//! First calls MessageImp::Write to output the message tobe transmitted
//! also to a local logging stream. Then the Dim service is updated.
//! If sending of the message failed a message is written to the
//! logging stream stored in MessageImp. It is intentionally not
//! output through Update to make it look different than usual
//! transmitted messages.
//
int MessageDimTX::Write(const Time &t, const string &txt, int qos)
{
    MessageImp::Write(t, txt, qos);
    fMsgQueue.emplace(t, txt, qos);
    return 1;
}

// **************************************************************************
/** @class MessageDimRX

@brief Based on MessageImp, subscribes to a MESSAGE service in the Dim network

This is a special DimInfoHandler. It subscribes to a service SERVER/MESSAGE
on the DimNetwork and redirects all received output to its base class
MessageImp view MessageImp::Write. the quality of service received with
each service update is passed as severity.

@section Examples

 - A simple and usefull example can be found in \ref log.cc and \ref logtime.cc

 @todo Maybe it is not a good idea that MessageImp is a base class,
 maybe it should be a reference given in the constructor

**/
// **************************************************************************

// --------------------------------------------------------------------------
//
//! Setup a DimStamedInfo service subscription for SERVER/MESSAGE
//!
//! @param name
//!    the name of the SERVER
//!
//! @param imp
//!    A reference to MessageImo to which messages will be redirected
//
MessageDimRX::MessageDimRX(const std::string &name, MessageImp &imp)
: fMinLogLevel(0), fConnected(false), fMsg(imp),
fDimMessage((name+"/MESSAGE").c_str(), (void*)NULL, 0, this)
{
}

// --------------------------------------------------------------------------
//
//! If the server has been disconnected write a simple log-message.
//! Skip all received messages which have a severity smaller than
//! fMinLogLevel. Write any other message with MessageImp::Write.
//
void MessageDimRX::infoHandler()
{
    if (getInfo()!=&fDimMessage)
        return;

    const string name   = fDimMessage.getName();
    const string server = name.substr(0, name.find_first_of('/'));

    fConnected = fDimMessage.getSize()!=0;

    // The server is diconnected. Do nothing
    if (!fConnected)
    {
        // We cannot print this message because it is produced by
        // every server which doesn't have the MESSAGE service, too.
        //fMsg.Message(server+": Disconnected.");
        return;
    }

    // skip all messages with a severity smaller than the minimum log level
    if (fDimMessage.getQuality()<fMinLogLevel)
        return;

    const string msg = server+": "+fDimMessage.getString();

    // Make sure getTimestamp is called _before_ getTimestampMillisecs
    // Must be in exactly this order!
    const int tsec = fDimMessage.getTimestamp();
    const int tms  = fDimMessage.getTimestampMillisecs();

    // Write the received message to the output
    fMsg.Write(Time(tsec, tms*1000), msg, fDimMessage.getQuality());
}
