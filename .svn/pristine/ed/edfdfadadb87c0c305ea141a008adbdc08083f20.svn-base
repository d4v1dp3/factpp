#ifndef FACT_ConnectionSSL
#define FACT_ConnectionSSL

#include <list>
#include <array>
#include <string>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "MessageImp.h"

class ConnectionSSL : public MessageImp, public boost::asio::ssl::context,
    public boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
{
private:
    MessageImp *fLog;

    std::string fAddress;
    std::string fPort;

    boost::asio::ip::tcp::endpoint fEndpoint;

    bool fVerbose;
    bool fDebugTx;

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
    boost::asio::deadline_timer   fConnectionTimer;

    size_t fQueueSize;

    ConnectionStatus_t fConnectionStatus;

    std::string fErrConnect;
    std::string fMsgConnect;

public:
    void SetLogStream(MessageImp *log) { fLog = log; }
    std::ostream &Out() { return fLog ? fLog->Out() : Out(); }

    // -------- Abbreviations for starting async tasks ---------

    void AsyncRead(const boost::asio::mutable_buffers_1 buffers, int type=0);
    void AsyncWrite(const boost::asio::const_buffers_1 &buffers);

    template<class T>
    void AsyncWaitImp(boost::asio::deadline_timer &timer, int millisec,
                      void (T::*handler)(const boost::system::error_code&))
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

        timer.async_wait(boost::bind(handler, this, boost::asio::placeholders::error));
    }

    void AsyncWait(boost::asio::deadline_timer &timer, int millisec,
                   void (ConnectionSSL::*handler)(const boost::system::error_code&))
    {
        AsyncWaitImp(timer, millisec, handler);
    }


private:
    void AsyncConnect(boost::asio::ip::tcp::resolver::iterator iterator);
    void AsyncConnect();

    void CloseImp(bool restart=true);

    bool ConnectImp(const boost::asio::ip::tcp::endpoint &endpoint,
                    const boost::system::error_code& error);
    void ConnectIter(boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                     const boost::system::error_code& error);
    void ConnectAddr(const boost::asio::ip::tcp::endpoint &endpoint,
                     const boost::system::error_code& error);

    void HandleConnectionTimer(const boost::system::error_code &error);
    void HandleWriteTimeout(const boost::system::error_code &error);
    void HandleSentData(const boost::system::error_code& error, size_t);
    void HandleHandshake(const boost::asio::ip::tcp::endpoint &endpoint, const boost::system::error_code& error);

    int Write(const Time &t, const std::string &txt, int qos=kInfo);

    virtual void ConnectionEstablished() { }
    virtual void ConnectionFailed() { }

public:
    ConnectionSSL(boost::asio::io_service& io_service, std::ostream &out);

    // ------------------------ connect --------------------------

    void SetEndpoint(const std::string &addr, int port);
    void SetEndpoint(const std::string &addr, const std::string &port);
    void SetEndpoint(const std::string &addr);
    void SetEndpoint(const boost::asio::ip::tcp::endpoint &ep);

    virtual void StartConnect();

    // ------------------------ close --------------------------
    void PostClose(bool restart=true);

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

    virtual void HandleReceivedData(const boost::system::error_code&, size_t, int = 0) { }
    virtual void HandleReadTimeout(const boost::system::error_code&) { }

    bool IsTxQueueEmpty() const { return fQueueSize==0; /*fOutQueue.empty();*/ }

    int IsClosed() const { return !lowest_layer().is_open(); }

    bool IsDisconnected() const { return fConnectionStatus==kDisconnected; }
    bool IsConnected()  const   { return fConnectionStatus==kConnected;    }
    bool IsConnecting() const   { return fConnectionStatus==kConnecting;   }

    void SetVerbose(bool b=true) { fVerbose=b; }
    void SetDebugTx(bool b=true) { fDebugTx=b; }

    bool GetVerbose() const { return fVerbose; }
    bool GetDebugTx() const { return fDebugTx; }

    std::string URL() const { return fAddress + ":" + fPort; }

    const boost::asio::ip::tcp::endpoint &GetEndpoint() const { return fEndpoint; }
};

#endif
