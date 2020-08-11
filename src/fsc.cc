#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::lexical_cast;

#include "Time.h"

#include "HeadersFTM.h"

using namespace std;
using namespace FTM;

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using boost::lexical_cast;
using ba::ip::tcp;

// ------------------------------------------------------------------------


// ------------------------------------------------------------------------

class tcp_connection : public ba::ip::tcp::socket, public boost::enable_shared_from_this<tcp_connection>
{
private:

    /*
    void AsyncRead(ba::mutable_buffers_1 buffers)
    {
        ba::async_read(*this, buffers,
                       boost::bind(&tcp_connection::HandleReceivedData, shared_from_this(),
                                   dummy::error, dummy::bytes_transferred));
    }*/

    void AsyncWrite(const ba::const_buffers_1 &buffers)
    {
        ba::async_write(*this, buffers,
                        boost::bind(&tcp_connection::HandleSentData, shared_from_this(),
                                    dummy::error, dummy::bytes_transferred));
    }
    void AsyncWait(ba::deadline_timer &timer, int seconds,
                               void (tcp_connection::*handler)(const bs::error_code&))// const
    {
        timer.expires_from_now(boost::posix_time::seconds(seconds));
        timer.async_wait(boost::bind(handler, shared_from_this(), dummy::error));
    }

    ba::deadline_timer fTriggerSendData;

    // The constructor is prvate to force the obtained pointer to be shared
    tcp_connection(ba::io_service& ioservice) : ba::ip::tcp::socket(ioservice),
        fTriggerSendData(ioservice)
    {
    }

    // Callback when writing was successfull or failed
    void HandleSentData(const boost::system::error_code& error, size_t bytes_transferred)
    {
        cout << "Data sent: (transmitted=" << bytes_transferred << ") rc=" << error.message() << " (" << error << ")" << endl;
    }

    stringstream fBuffer;

    void SendData()
    {
        fBuffer.str("");
        fBuffer <<
            "status: 00000538 \n"
            "time_s: 764.755 \n"
            "VOLTAGES \n"
            " \n"
            "enable:11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111  00001111 \n"
            "  done:11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111  00001111 \n"
            "values:0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 00 0 0 0 0 0 0 0 0 \n"
            "RESISTANCES \n"
            " \n"
            "enable:11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 \n"
            "  done:11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 \n"
            "values: \n"
            "1000.16 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "1197.07 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "558.59 677.92 817.26 989.39 1200.35 1503.06 1799.90 2204.18 \n"
            "3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 3199.99 \n"
            "end.\n";

        AsyncWrite(ba::buffer(ba::const_buffer(fBuffer.str().c_str(), fBuffer.str().length())));
    }

    void TriggerSendData(const boost::system::error_code &ec)
    {
        if (!is_open())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            return;
        }

        if (ec==ba::error::basic_errors::operation_aborted)
            return;

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fTriggerSendData.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        // The deadline has passed.
        SendData();

        AsyncWait(fTriggerSendData, 1, &tcp_connection::TriggerSendData);
    }


public:
    typedef boost::shared_ptr<tcp_connection> shared_ptr;

    static shared_ptr create(ba::io_service& io_service)
    {
        return shared_ptr(new tcp_connection(io_service));
    }

    void start()
    {
        AsyncWait(fTriggerSendData, 1, &tcp_connection::TriggerSendData);
    }
};


class tcp_server : public tcp::acceptor
{
public:
    tcp_server(ba::io_service& ioservice, int port) :
        tcp::acceptor(ioservice, tcp::endpoint(tcp::v4(), port))

    {
        // We could start listening for more than one connection
        // here, but since there is only one handler executed each time
        // it would not make sense. Before one handle_accept is not
        // finished no new handle_accept will be called.
        // Workround: Start a new thread in handle_accept
        start_accept();
    }

private:
    void start_accept()
    {
        cout << "Start accept..." << flush;
        tcp_connection::shared_ptr new_connection = tcp_connection::create(/*acceptor_.*/get_io_service());

        // This will accept a connection without blocking
        async_accept(*new_connection,
                     boost::bind(&tcp_server::handle_accept,
                                 this,
                                 new_connection,
                                 ba::placeholders::error));

        cout << "start-done." << endl;
    }

    void handle_accept(tcp_connection::shared_ptr new_connection, const boost::system::error_code& error)
    {
        // The connection has been accepted and is now ready to use

        // not installing a new handler will stop run()
        cout << "Handle accept..." << flush;
        if (!error)
        {
            new_connection->start();

            // The is now an open connection/server (tcp_connection)
            // we immediatly schedule another connection
            // This allowed two client-connection at the same time
            start_accept();
        }
        cout << "handle-done." << endl;
    }
};

int main(int argc, const char **argv)
{
    //try
    {
        ba::io_service io_service;

        int Port = argc==2 ? lexical_cast<int>(argv[1]) : 5000;

        tcp_server server(io_service, Port);
        //  ba::add_service(io_service, &server);
        //  server.add_service(...);
        //cout << "Run..." << flush;

        // Calling run() from a single thread ensures no concurrent access
        // of the handler which are called!!!
        io_service.run();

        //cout << "end." << endl;
    }
    /*catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }*/

    return 0;
}
/*  ====================== Buffers ===========================

char d1[128]; ba::buffer(d1));
std::vector<char> d2(128); ba::buffer(d2);
boost::array<char, 128> d3; by::buffer(d3);

// --------------------------------
char d1[128];
std::vector<char> d2(128);
boost::array<char, 128> d3;

boost::array<mutable_buffer, 3> bufs1 = {
   ba::buffer(d1),
   ba::buffer(d2),
   ba::buffer(d3) };
sock.read(bufs1);

std::vector<const_buffer> bufs2;
bufs2.push_back(boost::asio::buffer(d1));
bufs2.push_back(boost::asio::buffer(d2));
bufs2.push_back(boost::asio::buffer(d3));
sock.write(bufs2);


// ======================= Read functions =========================

ba::async_read_until --> delimiter

streambuf buf; // Ensure validity until handler!
by::async_read(s, buf, ....);

ba::async_read(s, ba:buffer(data, size), handler);
 // Single buffer
 boost::asio::async_read(s,
                         ba::buffer(data, size),
 compl-func -->          ba::transfer_at_least(32),
                         handler);

 // Multiple buffers
boost::asio::async_read(s, buffers,
 compl-func -->         boost::asio::transfer_all(),
                        handler);
                        */

// ================= Others ===============================

        /*
        strand   Provides serialised handler execution.
        work     Class to inform the io_service when it has work to do.


io_service::
dispatch   Request the io_service to invoke the given handler.
poll       Run the io_service's event processing loop to execute ready
           handlers.
poll_one   Run the io_service's event processing loop to execute one ready
           handler.
post       Request the io_service to invoke the given handler and return
           immediately.
reset      Reset the io_service in preparation for a subsequent run()
           invocation.
run        Run the io_service's event processing loop.
run_one    Run the io_service's event processing loop to execute at most
           one handler.
stop       Stop the io_service's event processing loop.
wrap       Create a new handler that automatically dispatches the wrapped
           handler on the io_service.

strand::         The io_service::strand class provides the ability to
                 post and dispatch handlers with the guarantee that none
                 of those handlers will execute concurrently.

dispatch         Request the strand to invoke the given handler.
get_io_service   Get the io_service associated with the strand.
post             Request the strand to invoke the given handler and return
                 immediately.
wrap             Create a new handler that automatically dispatches the
                 wrapped handler on the strand.

work::           The work class is used to inform the io_service when
                 work starts and finishes. This ensures that the io_service's run() function will not exit while work is underway, and that it does exit when there is no unfinished work remaining.
get_io_service   Get the io_service associated with the work.
work             Constructor notifies the io_service that work is starting.

*/


