#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::lexical_cast;

#include "Time.h"
#include "Converter.h"

#include "Dim.h"
#include "HeadersFTM.h"

using namespace std;
using namespace FTM;

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using boost::lexical_cast;
using ba::ip::tcp;

int Port = 0;

// ------------------------------------------------------------------------


// ------------------------------------------------------------------------

class tcp_connection : public ba::ip::tcp::socket, public boost::enable_shared_from_this<tcp_connection>
{
private:

    double fStartTime;

    void AsyncRead(ba::mutable_buffers_1 buffers)
    {
        ba::async_read(*this, buffers,
                       boost::bind(&tcp_connection::HandleReceivedData, shared_from_this(),
                                   dummy::error, dummy::bytes_transferred));
    }

    void AsyncWrite(const ba::const_buffers_1 &buffers)
    {
        ba::async_write(*this, buffers,
                        boost::bind(&tcp_connection::HandleSentData, shared_from_this(),
                                    dummy::error, dummy::bytes_transferred));
    }
    void AsyncWait(ba::deadline_timer &timer, int seconds,
                               void (tcp_connection::*handler)(const bs::error_code&))// const
    {
        timer.expires_from_now(boost::posix_time::milliseconds(seconds));
        timer.async_wait(boost::bind(handler, shared_from_this(), dummy::error));
    }

    ba::deadline_timer fTriggerDynData;

    // The constructor is prvate to force the obtained pointer to be shared
    tcp_connection(ba::io_service& ioservice) : ba::ip::tcp::socket(ioservice),
        fTriggerDynData(ioservice), fTriggerSendData(ioservice)
    {
        //deadline_.expires_at(boost::posix_time::pos_infin);

        fHeader.fDelimiter=kDelimiterStart;
        fHeader.fState=FTM::kFtmIdle|FTM::kFtmLocked;
        fHeader.fBoardId=0xaffe;
        fHeader.fFirmwareId=0x42;

        fDelimiter = htons(kDelimiterEnd);

        fStaticData.clear();

        fStaticData.fMultiplicityPhysics = 1;
        fStaticData.fMultiplicityCalib   = 40;
        fStaticData.fWindowCalib         = 1;
        fStaticData.fWindowPhysics       = 0;
        fStaticData.fDelayTrigger        = 21;
        fStaticData.fDelayTimeMarker     = 42;
        fStaticData.fDeadTime            = 84;

        fStaticData.fClockConditioner[0] = 100;
        fStaticData.fClockConditioner[1] = 1;
        fStaticData.fClockConditioner[2] = 8;
        fStaticData.fClockConditioner[3] = 9;
        fStaticData.fClockConditioner[4] = 11;
        fStaticData.fClockConditioner[5] = 13;
        fStaticData.fClockConditioner[6] = 14;
        fStaticData.fClockConditioner[7] = 15;

        fStaticData.fTriggerSequence = 1 | (2<<5) | (3<<10);

        fStaticData.fGeneralSettings =
            FTM::StaticData::kTrigger |
            FTM::StaticData::kLPext   |
            FTM::StaticData::kPedestal;

        fStaticData.fActiveFTU[0] = 0x3ff;
        fStaticData.fActiveFTU[3] = 0x3ff;

        for (int i=0; i<40; i++)
        {
            for (int p=0; p<4; p++)
                fStaticData[i].fEnable[p] = 0x1ff;

            for (int p=0; p<5; p++)
                fStaticData[i].fDAC[p]    = (p+1)*10;

            fStaticData[i].fPrescaling    = 42;
        }

        for (unsigned long long i=0; i<40; i++)
        {
            fFtuList[i].fDNA      = (i<<48)|(i<<32)|(i<<16)|i;
            fFtuList[i].fPingAddr = (1<<8) | i;
        }

        fFtuList[1].fPingAddr = (1<<9) | 1;
        fFtuList[0].fPingAddr = 0;

        fFtuList.fNumBoards = 19;
        fFtuList.fNumBoardsCrate[0] = 9;
        fFtuList.fNumBoardsCrate[1] = 0;
        fFtuList.fNumBoardsCrate[2] = 0;
        fFtuList.fNumBoardsCrate[3] = 10;
    }

    // Callback when writing was successfull or failed
    void HandleSentData(const boost::system::error_code& error, size_t bytes_transferred)
    {
        cout << "Data sent: (transmitted=" << bytes_transferred << ") rc=" << error.message() << " (" << error << ")" << endl;
    }

    vector<uint16_t> fBufCommand;
    vector<uint16_t> fBufHeader;
    vector<uint16_t> fBufFtuList;
    vector<uint16_t> fBufStaticData;
    vector<uint16_t> fBufDynamicData;

    vector<uint16_t> fCommand;
    FTM::Header      fHeader;
    FTM::FtuList     fFtuList;
    FTM::StaticData  fStaticData;
    FTM::DynamicData fDynamicData;

    //vector<uint16_t> fStaticData;

    uint16_t fDelimiter;
    uint16_t fBufRegister;

    uint16_t fCounter;
    uint16_t fTimeStamp;

    bool fReportsDisabled;

    ba::deadline_timer fTriggerSendData;

    void SendDynamicData()
    {
        if (fReportsDisabled)
            return;

        //if (fHeader.fState == FTM::kFtmRunning)
        //    fDynamicData.fOnTimeCounter = lrint(Time().UnixTime()-fStartTime);

        fDynamicData.fTempSensor[0] = (23. + (6.*rand()/RAND_MAX-3))*10;
        fDynamicData.fTempSensor[1] = (55. + (6.*rand()/RAND_MAX-3))*10;
        fDynamicData.fTempSensor[2] = (39. + (6.*rand()/RAND_MAX-3))*10;
        fDynamicData.fTempSensor[3] = (42. + (6.*rand()/RAND_MAX-3))*10;

        for (int i=0; i<40; i++)
            for (int p=0; p<4; p++)
                fDynamicData[i].fRatePatch[p] = (1000 + (float(rand())/RAND_MAX-0.5)*25*p);

        fHeader.fType=kDynamicData;     // FtuList
        fHeader.fDataSize=sizeof(FTM::DynamicData)/2+1;
        fHeader.fTriggerCounter = fCounter;
        fHeader.fTimeStamp = fTimeStamp++*1000000;//lrint(Time().UnixTime());

        fBufHeader      = fHeader.HtoN();
        fBufDynamicData = fDynamicData.HtoN();

        AsyncWrite(ba::buffer(ba::const_buffer(&fBufHeader[0],      fBufHeader.size()*2)));
        AsyncWrite(ba::buffer(ba::const_buffer(&fBufDynamicData[0], sizeof(FTM::DynamicData))));
        AsyncWrite(ba::buffer(ba::const_buffer(&fDelimiter, 2)));
    }

    void SendStaticData()
    {
        fHeader.fType=kStaticData;     // FtuList
        fHeader.fDataSize=sizeof(FTM::StaticData)/2+1;
        fHeader.fTriggerCounter = fCounter;
        fHeader.fTimeStamp = fTimeStamp*1000000;//lrint(Time().UnixTime());

        for (int i=0; i<4; i++)
            fFtuList.fActiveFTU[i] = fStaticData.fActiveFTU[i];

        fBufHeader     = fHeader.HtoN();
        fBufStaticData = fStaticData.HtoN();

        AsyncWrite(ba::buffer(ba::const_buffer(&fBufHeader[0],     fBufHeader.size()*2)));
        AsyncWrite(ba::buffer(ba::const_buffer(&fBufStaticData[0], fBufStaticData.size()*2)));
        AsyncWrite(ba::buffer(ba::const_buffer(&fDelimiter, 2)));
    }

    void HandleReceivedData(const boost::system::error_code& error, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0)
        {
            // Close the connection
            close();
            return;
        }

        // No command received yet
        if (fCommand.size()==0)
        {
            transform(fBufCommand.begin(), fBufCommand.begin()+bytes_received/2,
                      fBufCommand.begin(), ntohs);

            if (fBufCommand[0]!='@')
            {
                cout << "Inavlid command: 0x" << hex << fBufCommand[0] << dec << endl;
                cout << "Received b=" << bytes_received << ": " << error.message() << " (" << error << ")" << endl;
                cout << "Hex:" << Converter::GetHex<uint16_t>(&fBufCommand[0], bytes_received) << endl;
                return;
            }

            switch (fBufCommand[1])
            {
            case kCmdToggleLed:
                cout << "-> TOGGLE_LED" << endl;

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));
                return;

            case kCmdPing:
                cout << "-> PING" << endl;

                fHeader.fType=kFtuList;     // FtuList
                fHeader.fDataSize=sizeof(FTM::FtuList)/2+1;
                fHeader.fTriggerCounter = fCounter;
                fHeader.fTimeStamp = fTimeStamp*1000000;//lrint(Time().UnixTime());

                fFtuList[1].fPingAddr = ((rand()&1)<<9) | 1;
                fFtuList[0].fPingAddr = ((rand()&1)<<8);

                fBufHeader  = fHeader.HtoN();
                fBufFtuList = fFtuList.HtoN();

                AsyncWrite(ba::buffer(ba::const_buffer(&fBufHeader[0],  fBufHeader.size()*2)));
                AsyncWrite(ba::buffer(ba::const_buffer(&fBufFtuList[0], fBufFtuList.size()*2)));
                AsyncWrite(ba::buffer(ba::const_buffer(&fDelimiter, 2)));

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));
                return;

            case kCmdRead: // kCmdRead
                cout << "-> READ" << endl;
                switch (fBufCommand[2])
                {
                case kCmdStaticData:
                    cout << "-> STATIC" << endl;

                    SendStaticData();

                    fBufCommand.resize(5);
                    AsyncRead(ba::buffer(fBufCommand));

                    return;

                case kCmdDynamicData:
                    cout << "-> DYNAMIC" << endl;

                    SendDynamicData();

                    fBufCommand.resize(5);
                    AsyncRead(ba::buffer(fBufCommand));

                    return;

                case kCmdRegister:
                    fCommand = fBufCommand;
                    cout << "-> REGISTER" << endl;

                    fBufCommand.resize(1);
                    AsyncRead(ba::buffer(fBufCommand));
                    return;
                }
                break;


            case kCmdWrite:
                switch (fBufCommand[2])
                {
                case kCmdRegister:
                    fCommand = fBufCommand;
                    cout << "-> REGISTER" << endl;

                    fBufCommand.resize(2);
                    AsyncRead(ba::buffer(fBufCommand));
                    return;

                case kCmdStaticData:
                    fCommand = fBufCommand;
                    cout << "-> STATIC DATA" << endl;

                    fBufCommand.resize(sizeof(StaticData)/2);
                    AsyncRead(ba::buffer(fBufCommand));
                    return;
                }
                break;

            case kCmdDisableReports:
                cout << "-> DISABLE REPORTS " << !fBufCommand[2] << endl;
                fReportsDisabled = !fBufCommand[2];

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));
                return;

            case kCmdConfigFTU:
                cout << "-> Configure FTU " << (fBufCommand[2]&0xff) << " " << (fBufCommand[2]>>8) << endl;

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));
                return;

            case kCmdStartRun:
                fHeader.fState = FTM::kFtmRunning|FTM::kFtmLocked;

                fStartTime = Time().UnixTime();

                fCounter = 0;
                fTimeStamp = 0;
                fHeader.fTriggerCounter = fCounter;

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));

                AsyncWait(fTriggerSendData, 0, &tcp_connection::TriggerSendData);
                return;

            case kCmdStopRun:
                fHeader.fState = FTM::kFtmIdle|FTM::kFtmLocked;

                fTriggerSendData.cancel();

                fCounter = 0;
                fTimeStamp = 0;

                fBufCommand.resize(5);
                AsyncRead(ba::buffer(fBufCommand));
                return;
            }

            cout << "Received b=" << bytes_received << ": " << error.message() << " (" << error << ")" << endl;
            cout << "Hex:" << Converter::GetHex<uint16_t>(&fBufCommand[0], bytes_received) << endl;
            return;
        }

        // Command data received

        // Prepare reception of next command
        switch (fCommand[1])
        {
        case kCmdRead: // kCmdRead
            {
                const uint16_t addr = ntohs(fBufCommand[0]);
                const uint16_t val  = reinterpret_cast<uint16_t*>(&fStaticData)[addr];

                cout << "-> GET REGISTER[" << addr << "]=" << val << endl;

                fHeader.fType=kRegister;     // FtuList
                fHeader.fDataSize=2;
                fHeader.fTimeStamp = fTimeStamp*1000000;//lrint(Time().UnixTime());

                fBufHeader = fHeader.HtoN();
                fBufStaticData[addr] = htons(val);

                AsyncWrite(ba::buffer(ba::const_buffer(&fBufHeader[0], fBufHeader.size()*2)));
                AsyncWrite(ba::buffer(ba::const_buffer(&fBufStaticData[addr], 2)));
                AsyncWrite(ba::buffer(ba::const_buffer(&fDelimiter, 2)));
                break;
            }

        case kCmdWrite:
            switch (fCommand[2])
            {
            case kCmdRegister:
                {
                    const uint16_t addr = ntohs(fBufCommand[0]);
                    const uint16_t val  = ntohs(fBufCommand[1]);

                    cout << "-> SET REGISTER[" << addr << "]=" << val << endl;

                    reinterpret_cast<uint16_t*>(&fStaticData)[addr] = val;
                }
                break;

            case kCmdStaticData:
                {
                    cout << "-> SET STATIC DATA" << endl;
                    fStaticData = fBufCommand;
                }
                break;
            }
            break;
        }

        fCommand.resize(0);

        fBufCommand.resize(5);
        AsyncRead(ba::buffer(fBufCommand));
    }

    void SendDynData(const boost::system::error_code &ec)
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

        if (fTriggerDynData.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        // The deadline has passed.
        SendDynamicData();

        AsyncWait(fTriggerDynData, 1000, &tcp_connection::SendDynData);
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


        if (fStaticData.IsEnabled(StaticData::kTrigger))
            Dim::SendCommand("FAD/TRIGGER", fCounter++);

        const uint16_t time = 100*float(rand())/RAND_MAX+50;

        AsyncWait(fTriggerSendData, time, &tcp_connection::TriggerSendData);
    }

public:
    typedef boost::shared_ptr<tcp_connection> shared_ptr;

    static shared_ptr create(ba::io_service& io_service)
    {
        return shared_ptr(new tcp_connection(io_service));
    }

    void start()
    {
        // Ownership of buffer must be valid until Handler is called.

        // Emit something to be written to the socket
        fBufCommand.resize(5);
        AsyncRead(ba::buffer(fBufCommand));

        AsyncWait(fTriggerDynData, 1000, &tcp_connection::SendDynData);

//        AsyncWrite(ba::buffer(ba::const_buffer(&fHeader, sizeof(FTM::Header))));
//        AsyncWait(deadline_, 3, &tcp_connection::check_deadline);

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

#include "Configuration.h"

void SetupConfiguration(::Configuration &conf)
{
    const string n = conf.GetName()+".log";

    po::options_description config("Program options");
    config.add_options()
        ("dns",       var<string>("localhost"), "Dim nameserver host name (Overwites DIM_DNS_NODE environment variable)")
        ("port,p",    var<uint16_t>(5000), "")
        ;

    po::positional_options_description p;
    p.add("port", 1); // The first positional options
    p.add("num",  1); // The second positional options

    conf.AddEnv("dns", "DIM_DNS_NODE");

    conf.AddOptions(config);
    conf.SetArgumentPositions(p);
}

int main(int argc, const char **argv)
{
    ::Configuration conf(argv[0]);

    SetupConfiguration(conf);

    po::variables_map vm;
    try
    {
        vm = conf.Parse(argc, argv);
    }
#if BOOST_VERSION > 104000
    catch (po::multiple_occurrences &e)
    {
        cerr << "Program options invalid due to: " << e.what() << " of '" << e.get_option_name() << "'." << endl;
        return -1;
    }
#endif
    catch (exception& e)
    {
        cerr << "Program options invalid due to: " << e.what() << endl;
        return -1;
    }

    if (conf.HasVersion() || conf.HasPrint() || conf.HasHelp())
        return -1;

    Dim::Setup(conf.Get<string>("dns"));

    //try
    {
        ba::io_service io_service;

        Port = conf.Get<uint16_t>("port");

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


