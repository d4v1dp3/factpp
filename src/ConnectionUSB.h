#ifndef FACT_Connection
#define FACT_Connection

#include <list>
#include <array>
#include <string>

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "MessageImp.h"

class ConnectionUSB : public MessageImp, public boost::asio::serial_port
{
private:
    MessageImp *fLog;

    std::string fAddress;

    enum ConnectionStatus_t
    {
        kDisconnected = 0,
        kConnecting   = 1,
        kConnected    = 2,
    };

protected:
    boost::asio::deadline_timer   fInTimeout;

private:
    boost::asio::deadline_timer   fOutTimeout;
    boost::asio::deadline_timer   fConnectTimeout;

    size_t fQueueSize;

    ConnectionStatus_t fConnectionStatus;

public:
    void SetLogStream(MessageImp *log) { fLog = log; }
    std::ostream &Out() { return fLog ? fLog->Out() : Out(); }

    // -------- Abbreviations for starting async tasks ---------

    void AsyncRead(const boost::asio::mutable_buffers_1 buffers, int type=0, int counter=0);
    void AsyncWrite(const boost::asio::const_buffers_1 &buffers);
    void AsyncWait(boost::asio::deadline_timer &timer, int millisec,
                   void (ConnectionUSB::*handler)(const boost::system::error_code&));

protected:
    void CloseImp(int64_t delay=0);

private:
    void ConnectImp(const boost::system::error_code& error,
                    boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

    void HandleWriteTimeout(const boost::system::error_code &error);
    void HandleSentData(const boost::system::error_code& error, size_t);
    void HandleReconnectTimeout(const boost::system::error_code &error);

    int Write(const Time &t, const std::string &txt, int qos=kInfo);

    virtual void ConnectionEstablished() { }

public:
    ConnectionUSB(boost::asio::io_service& io_service, std::ostream &out);

    // ------------------------ connect --------------------------

    void SetEndpoint(const std::string &addr);

    virtual void Connect(int baud_rate=115200,
                 int character_size=8,
                 boost::asio::serial_port_base::parity::type parity=boost::asio::serial_port_base::parity::none,
                 boost::asio::serial_port_base::stop_bits::type stop_bits=boost::asio::serial_port_base::stop_bits::one,
                 boost::asio::serial_port_base::flow_control::type flow_control=boost::asio::serial_port_base::flow_control::none);

    void ConfigureConnection();

    // ------------------------ close --------------------------
    void PostClose(int64_t delay=0);

    // ------------------------ write --------------------------
    void PostMessage(const void *msg, size_t s=0);
    void PostMessage(const std::string &cmd, size_t s=-1);

    template<typename T, size_t N>
    void PostMessage(const std::array<T, N> &msg)
    {
        PostMessage(msg.begin(), msg.size()*sizeof(T));
    }

    template<typename T>
        void PostMessage(const std::vector<T> &msg)
    {
        PostMessage(&msg[0], msg.size()*sizeof(T));
    }

    // ------------------------ others --------------------------

    virtual void HandleReceivedData(const boost::system::error_code&, size_t, int = 0, int = 0) { }
    virtual void HandleTransmittedData(size_t) { }
    virtual void HandleReadTimeout(const boost::system::error_code&) { }

    int IsClosed() const { return !is_open(); }

    bool IsConnected()  const { return fConnectionStatus==kConnected;  }
    bool IsConnecting() const { return fConnectionStatus==kConnecting; }

    std::string URL() const { return fAddress; }
};

#endif
