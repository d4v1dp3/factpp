// **************************************************************************
/** @class Connection

@brief Maintains an ansynchronous TCP/IP client connection

@todo
   Unify with ConnectionUSB

*/
// **************************************************************************
#include "Connection.h"

using namespace std;

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using ba::ip::tcp;

    // -------- Abbreviations for starting async tasks ---------

int Connection::Write(const Time &t, const string &txt, int qos)
{
    if (fLog)
        return fLog->Write(t, txt, qos);

    return MessageImp::Write(t, txt, qos);
}

void Connection::AsyncRead(const ba::mutable_buffers_1 buffers, int type)
{
    ba::async_read(*this, buffers,
                   boost::bind(&Connection::HandleReceivedData, this,
                               dummy::error, dummy::bytes_transferred, type));
}

void Connection::AsyncWrite(const ba::const_buffers_1 &buffers)
{
    ba::async_write(*this, buffers,
                    boost::bind(&Connection::HandleSentData, this,
                                dummy::error, dummy::bytes_transferred));
}

/*
void Connection::AsyncWait(ba::deadline_timer &timer, int millisec,
                           void (Connection::*handler)(const bs::error_code&))
{
    // - The boost::asio::basic_deadline_timer::expires_from_now()
    //   function cancels any pending asynchronous waits, and returns
    //   the number of asynchronous waits that were cancelled. If it
    //   returns 0 then you were too late and the wait handler has
    //   already been executed, or will soon be executed. If it
    //   returns 1 then the wait handler was successfully cancelled.
    // - If a wait handler is cancelled, the bs::error_code passed to
    //   it contains the value bs::error::operation_aborted.
    timer.expires_from_now(boost::posix_time::milliseconds(millisec));

    timer.async_wait(boost::bind(handler, this, dummy::error));
}
*/

void Connection::AsyncConnect(tcp::resolver::iterator iterator)
{
    tcp::endpoint endpoint = *iterator;

    // AsyncConnect + Deadline
     async_connect(endpoint,
                  boost::bind(&Connection::ConnectIter,
                              this, iterator, ba::placeholders::error));

    // We will get a "Connection timeout anyway"
    //AsyncWait(fConnectTimeout, 5, &Connection::HandleConnectTimeout);
}

void Connection::AsyncConnect()
{
    // AsyncConnect + Deadline
     async_connect(fEndpoint,
                  boost::bind(&Connection::ConnectAddr,
                              this, fEndpoint, ba::placeholders::error));

    // We will get a "Connection timeout anyway"
    //AsyncWait(fConnectTimeout, 5, &Connection::HandleConnectTimeout);
}

// ------------------------ close --------------------------
// close from another thread
void Connection::CloseImp(bool restart)
{
    if (IsConnected() && fVerbose)
    {
        ostringstream str;
        str << "Connection closed to " << URL() << ".";
        Info(str);
    }

    // Stop any pending connection attempt
    fConnectionTimer.cancel();

    // Close possible open connections
    close();

    // Reset the connection status
    fQueueSize = 0;
    fConnectionStatus = kDisconnected;

    // Stop deadline counters
    fInTimeout.cancel();
    fOutTimeout.cancel();

    if (!restart || IsConnecting())
        return;

    // We need some timeout before reconnecting!
    // And we have to check if we are alreayd trying to connect
    // We shoudl wait until all operations in progress were canceled

    // Start trying to reconnect
    fMsgConnect = "";
    fErrConnect = "";
    StartConnect();
}

void Connection::PostClose(bool restart)
{
    get_io_service().post(boost::bind(&Connection::CloseImp, this, restart));
}

// ------------------------ write --------------------------
void Connection::HandleWriteTimeout(const bs::error_code &error)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    // 125: Operation canceled (bs::error_code(125, bs::system_category))
    if (error)
    {
        ostringstream str;
        str << "Write timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);

        CloseImp();
        return;
    }

    if (!is_open())
    {
        // For example: Here we could schedule a new accept if we
        // would not want to allow two connections at the same time.
        return;
    }

    // Check whether the deadline has passed. We compare the deadline
    // against the current time since a new asynchronous operation
    // may have moved the deadline before this actor had a chance
    // to run.
    if (fOutTimeout.expires_at() > ba::deadline_timer::traits_type::now())
        return;

    Error("fOutTimeout has expired, writing data to "+URL());

    CloseImp();
}

void Connection::HandleSentData(const bs::error_code& error, size_t n)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    if (error && error != ba::error::not_connected)
    {
        ostringstream str;
        str << "Writing to " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);

        CloseImp();
        return;
    }

    if (error == ba::error::not_connected)
    {
        ostringstream msg;
        msg << n << " bytes could not be sent to " << URL() << " due to missing connection.";
        Warn(msg);

        return;
    }

    if (--fQueueSize==0)
        fOutTimeout.cancel();

    if (fDebugTx)
    {
        ostringstream msg;
        msg << n << " bytes successfully sent to " << URL();
        Debug(msg);
    }
}

void Connection::PostMessage(const void *ptr, size_t sz)
{
    // This function can be called from a different thread...
    if (!is_open())
        return;

    // ... this is why we have to increase fQueueSize first
    fQueueSize++;

    // ... and shift the deadline timer
    // This is not ideal, because if we are continously
    // filling the buffer, it will never timeout
    AsyncWait(fOutTimeout, 5000, &Connection::HandleWriteTimeout);

    // Now we can schedule the buffer to be sent
    AsyncWrite(ba::const_buffers_1(ptr, sz));

    // If a socket is closed, all pending asynchronous
    // operation will be aborted.
}

void Connection::PostMessage(const string &cmd, size_t max)
{
    if (max==size_t(-1))
        max = cmd.length()+1;

    PostMessage(cmd.c_str(), min(cmd.length()+1, max));
}

void Connection::HandleConnectionTimer(const bs::error_code &error)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    if (error)
    {
        ostringstream str;
        str << "Connetion timer of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);
    }

    if (is_open())
    {
        // For example: Here we could schedule a new accept if we
        // would not want to allow two connections at the same time.
        return;
    }

    // Check whether the deadline has passed. We compare the deadline
    // against the current time since a new asynchronous operation
    // may have moved the deadline before this actor had a chance
    // to run.
    if (fConnectionTimer.expires_at() < ba::deadline_timer::traits_type::now())
        StartConnect();
}

bool Connection::ConnectImp(const tcp::endpoint &endpoint, const bs::error_code& error)
{
    const string host = endpoint.port()==0 ? "" :
        endpoint.address().to_string()+':'+to_string((long long unsigned int)endpoint.port());

    // Connection established
    if (!error)
    {
	set_option(socket_base::keep_alive(true));

	const int optval = 30;
        // First keep alive after 30s
	setsockopt(native_handle(), SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval));
        // New keep alive after 30s
	setsockopt(native_handle(), SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval));

        if (fVerbose)
            Info("Connection established to "+host+"...");

        fQueueSize = 0;
        fConnectionStatus = kConnected;

        ConnectionEstablished();
        return true;
    }

    // If returning from run will lead to deletion of this
    // instance, close() is not needed (maybe implicitly called).
    // If run is called again, close() is needed here. Otherwise:
    // Software caused connection abort when we try to resolve
    // the endpoint again.
    CloseImp(false);

    ostringstream msg;
    if (!host.empty())
        msg << "Connecting to " << host << ": " << error.message() << " (" << error << ")";

    if (fErrConnect!=msg.str())
    {
        if (error!=ba::error::basic_errors::connection_refused)
            fMsgConnect = "";
        fErrConnect = msg.str();
        Warn(fErrConnect);
    }

    if (error==ba::error::basic_errors::operation_aborted)
        return true;

    fConnectionStatus = kConnecting;

    return false;
/*
    // Go on with the next
    if (++iterator != tcp::resolver::iterator())
    {
        AsyncConnect(iterator);
        return;
    }
*/
    // No more entries to try, if we would not put anything else
    // into the queue anymore it would now return (run() would return)

    // Since we don't want to block the main loop, we wait using an
    // asnychronous timer

    // FIXME: Should we move this before AsyncConnect() ?
//    AsyncWait(fConnectionTimer, 250, &Connection::HandleConnectionTimer);
}

void Connection::ConnectIter(tcp::resolver::iterator iterator, const bs::error_code& error)
{
    if (ConnectImp(*iterator, error))
        return;

    // Go on with the next
    if (++iterator != tcp::resolver::iterator())
    {
        AsyncConnect(iterator);
        return;
    }

    // No more entries to try, if we would not put anything else
    // into the queue anymore it would now return (run() would return)
    AsyncWait(fConnectionTimer, 250, &Connection::HandleConnectionTimer);
}

void Connection::ConnectAddr(const tcp::endpoint &endpoint, const bs::error_code& error)
{
    if (ConnectImp(endpoint, error))
        return;

    AsyncWait(fConnectionTimer, 250, &Connection::HandleConnectionTimer);
}

// FIXME: Async connect should get address and port as an argument
void Connection::StartConnect()
{
    fConnectionStatus = kConnecting;

    if (fEndpoint!=tcp::endpoint())
    {
        ostringstream msg;
        msg << "Trying to connect to " << fEndpoint << "...";
        if (fMsgConnect!=msg.str())
        {
            fMsgConnect = msg.str();
            Info(msg);
        }

        AsyncConnect();
        return;
    }

    const bool valid = !fAddress.empty() || !fPort.empty();

    boost::system::error_code ec;

    ostringstream msg;
    if (!valid)
        msg << "No target address... connection attempt postponed.";
    else
    {
        tcp::resolver resolver(get_io_service());

        tcp::resolver::query query(fAddress, fPort);
        tcp::resolver::iterator iterator = resolver.resolve(query, ec);

        msg << "Trying to connect to " << URL() << "...";

        // Start connection attempts (will also reset deadline counter)
        if (!ec)
            AsyncConnect(iterator);
        else
            msg << " " << ec.message() << " (" << ec << ")";
    }

    // Only output message if it has changed
    if (fMsgConnect!=msg.str())
    {
        fMsgConnect = msg.str();
        if (ec)
            Error(msg);
        if (!ec && fVerbose)
            Info(msg);
    }

    if (!valid || ec)
        AsyncWait(fConnectionTimer, 250, &Connection::HandleConnectionTimer);
}

void Connection::SetEndpoint(const string &addr, int port)
{
    if (fConnectionStatus>=1)
        Warn("Connection or connection attempt in progress. New endpoint only valid for next connection.");

    fAddress = addr;
    fPort    = to_string((long long)port);
}

void Connection::SetEndpoint(const string &addr, const string &port)
{
    if (fConnectionStatus>=1 && URL()!=":")
        Warn("Connection or connection attempt in progress. New endpoint only valid for next connection.");

    fAddress = addr;
    fPort    = port;
}

void Connection::SetEndpoint(const string &addr)
{
    const size_t p0 = addr.find_first_of(':');
    const size_t p1 = addr.find_last_of(':');

    if (p0==string::npos || p0!=p1)
    {
        Error("Connection::SetEndpoint - Wrong format of argument '"+addr+"' ('host:port' expected)");
        return;
    }

    SetEndpoint(addr.substr(0, p0), addr.substr(p0+1));
}

void Connection::SetEndpoint(const tcp::endpoint &ep)
{
    const ba::ip::address addr = ep.address();

    const ba::ip::address use =
        addr.is_v6() && addr.to_v6().is_loopback() ?
        ba::ip::address(ba::ip::address_v4::loopback()) :
        addr;

    SetEndpoint(use.to_string(), ep.port());

    fEndpoint = tcp::endpoint(use, ep.port());
}


Connection::Connection(ba::io_service& ioservice, ostream &out) :
MessageImp(out), tcp::socket(ioservice),
fLog(0), fVerbose(true), fDebugTx(false),
fInTimeout(ioservice), fOutTimeout(ioservice), fConnectionTimer(ioservice),
fQueueSize(0), fConnectionStatus(kDisconnected)
{
}
