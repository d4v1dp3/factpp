// **************************************************************************
/** @class Connection

@brief Maintains an ansynchronous TCP/IP client connection

*/
// **************************************************************************
#include "ConnectionUSB.h"

#include <boost/bind.hpp>

using namespace std;

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using ba::serial_port_base;

//#define DEBUG_TX
//#define DEBUG

#ifdef DEBUG
#include <fstream>
#include <iomanip>
#include "Time.h"
#endif

// -------- Abbreviations for starting async tasks ---------

int ConnectionUSB::Write(const Time &t, const string &txt, int qos)
{
    if (fLog)
        return fLog->Write(t, txt, qos);

    return MessageImp::Write(t, txt, qos);
}

void ConnectionUSB::AsyncRead(const ba::mutable_buffers_1 buffers, int type, int counter)
{
    ba::async_read(*this, buffers,
                   boost::bind(&ConnectionUSB::HandleReceivedData, this,
                               dummy::error, dummy::bytes_transferred, type, counter));
}

void ConnectionUSB::AsyncWrite(const ba::const_buffers_1 &buffers)
{
    ba::async_write(*this, buffers,
                    boost::bind(&ConnectionUSB::HandleSentData, this,
                                dummy::error, dummy::bytes_transferred));
}

void ConnectionUSB::AsyncWait(ba::deadline_timer &timer, int millisec,
                           void (ConnectionUSB::*handler)(const bs::error_code&))
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

// ------------------------ close --------------------------
// close from another thread
void ConnectionUSB::CloseImp(int64_t delay)
{
    if (IsConnected())
        Info("Closing connection to "+URL()+".");

    // Close possible open connections
    bs::error_code ec;
    cancel(ec);
    if (ec && ec!=ba::error::basic_errors::bad_descriptor)
    {
        ostringstream msg;
        msg << "Cancel async requests on " << URL() << ": " << ec.message() << " (" << ec << ")";
        Error(msg);
    }

    if (IsConnected())
    {
        close(ec);
        if (ec)
        {
            ostringstream msg;
            msg << "Closing " << URL() << ": " << ec.message() << " (" << ec << ")";
            Error(msg);
        }
        else
            Info("Closed connection to "+URL()+" succesfully.");
    }

    // Stop deadline counters
    fInTimeout.cancel();
    fOutTimeout.cancel();
    fConnectTimeout.cancel();

    // Reset the connection status
    fQueueSize = 0;
    fConnectionStatus = kDisconnected;

#ifdef DEBUG
    ofstream fout1("transmitted.txt", ios::app);
    ofstream fout2("received.txt", ios::app);
    ofstream fout3("send.txt", ios::app);
    fout1 << Time() << ": ---" << endl;
    fout2 << Time() << ": ---" << endl;
    fout3 << Time() << ": ---" << endl;
#endif

    if (delay<0 || IsConnecting())
        return;

    // We need some timeout before reconnecting!
    // And we have to check if we are alreayd trying to connect
    // We should wait until all operations in progress were canceled
    fConnectTimeout.expires_from_now(boost::posix_time::seconds(delay));
    fConnectTimeout.async_wait(boost::bind(&ConnectionUSB::HandleReconnectTimeout, this, dummy::error));
}

void ConnectionUSB::PostClose(int64_t delay)
{
    get_io_service().post(boost::bind(&ConnectionUSB::CloseImp, this, delay));
}

void ConnectionUSB::HandleReconnectTimeout(const bs::error_code &error)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    // 125: Operation canceled (bs::error_code(125, bs::system_category))
    if (error)
    {
        ostringstream str;
        str << "Reconnect timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);

        CloseImp(-1);
        return;
    }


    if (is_open())
    {
        Error("HandleReconnectTimeout - "+URL()+" is already open.");
        return;
    }

    // Check whether the deadline has passed. We compare the deadline
    // against the current time since a new asynchronous operation
    // may have moved the deadline before this actor had a chance
    // to run.
    if (fConnectTimeout.expires_at() > ba::deadline_timer::traits_type::now())
        return;

    // Start trying to reconnect
    Connect();
}


// ------------------------ write --------------------------
void ConnectionUSB::HandleWriteTimeout(const bs::error_code &error)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    // 125: Operation canceled (bs::error_code(125, bs::system_category))
    if (error)
    {
        ostringstream str;
        str << "Write timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);

        CloseImp(-1);
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

    CloseImp(-1);
}

void ConnectionUSB::HandleSentData(const bs::error_code& error, size_t n)
{
    if (error==ba::error::basic_errors::operation_aborted)
        return;

    if (error && error != ba::error::not_connected)
    {
        ostringstream str;
        str << "Writing to " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
        Error(str);

        CloseImp(-1);
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

#ifdef DEBUG_TX
    ostringstream msg;
    msg << n << " bytes successfully sent to " << URL();
    Message(msg);
#endif

#ifdef DEBUG
    ofstream fout("transmitted.txt", ios::app);
    fout << Time() << ": ";
    for (unsigned int i=0; i<fOutQueue.front().size(); i++)
        fout << hex << setfill('0') << setw(2) << (uint32_t)fOutQueue.front()[i];
    fout << endl;
#endif

    HandleTransmittedData(n);
}

void ConnectionUSB::PostMessage(const void *ptr, size_t sz)
{
    // This function can be called from a different thread...
    if (!is_open())
        return;

    // ... this is why we have to increase fQueueSize first
    fQueueSize++;

    // ... and shift the deadline timer
    // This is not ideal, because if we are continously
    // filling the buffer, it will never timeout
    AsyncWait(fOutTimeout, 5000, &ConnectionUSB::HandleWriteTimeout);

    // Now we can schedule the buffer to be sent
    AsyncWrite(ba::const_buffers_1(ptr, sz));
}

void ConnectionUSB::PostMessage(const string &cmd, size_t max)
{
    if (max==size_t(-1))
        max = cmd.length()+1;

    PostMessage(cmd.c_str(), min(cmd.length()+1, max));
}

void ConnectionUSB::Connect(int _baud_rate, int _character_size,
                            parity::type _parity, stop_bits::type _stop_bits,
                            flow_control::type _flow_control)
{
    fConnectionStatus = kConnecting;

    Info("Connecting to "+URL()+".");

    bs::error_code ec;
    open(URL(), ec);

    if (ec)
    {
        ostringstream msg;
        msg << "Error opening " << URL() << "... " << ec.message() << " (" << ec << ")";
        Error(msg);
        fConnectionStatus = kDisconnected;
        return;
    }

    Info("Connection established.");

    try
    {
        Debug("Setting Baud Rate");
        set_option(baud_rate(_baud_rate));

        Debug("Setting Character Size");
        set_option(character_size(_character_size));

        Debug("Setting Parity");
        set_option(parity(_parity));

        Debug("Setting Sop Bits");
        set_option(stop_bits(_stop_bits));

        Debug("Setting Flow control");
        set_option(flow_control(_flow_control));
    }
    catch (const bs::system_error &erc)
    {
        Error(string("Setting connection options: ")+erc.what());
        close();
        return;
    }

    fQueueSize = 0;
    fConnectionStatus = kConnected;

    ConnectionEstablished();
}

void ConnectionUSB::SetEndpoint(const string &addr)
{
    if (fConnectionStatus>=1)
        Warn("Connection or connection attempt in progress. New endpoint only valid for next connection.");

    fAddress = "/dev/"+addr;
}


ConnectionUSB::ConnectionUSB(ba::io_service& ioservice, ostream &out) :
MessageImp(out), ba::serial_port(ioservice), fLog(0),
/*
 fBaudRate(115200),
fCharacterSize(8), fParity(parity::none), fStopBits(stop_bits::one),
fFlowControl(flow_control::none),
*/
fInTimeout(ioservice), fOutTimeout(ioservice), fConnectTimeout(ioservice),
fQueueSize(0), fConnectionStatus(kDisconnected)
{
}
