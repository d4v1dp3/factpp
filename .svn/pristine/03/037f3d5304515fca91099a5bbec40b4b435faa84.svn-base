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

#include "HeadersFAD.h"

#include "dis.hxx"
#include "Dim.h"

using namespace std;
using namespace FAD;

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using boost::lexical_cast;
using ba::ip::tcp;

class tcp_connection;

class Trigger : public DimCommandHandler
{
    DimCommand fCmd;

    vector<tcp_connection*> vec;

public:
    Trigger() : fCmd("FAD/TRIGGER", "I:1", this)
    {
    }

    void Add(tcp_connection *ptr)
    {
        vec.push_back(ptr);
    }

    void Remove(tcp_connection *ptr)
    {
        vec.erase(find(vec.begin(), vec.end(), ptr));
    }

    void commandHandler();
};

// ------------------------------------------------------------------------

class tcp_connection : public ba::ip::tcp::socket, public boost::enable_shared_from_this<tcp_connection>
{
public:
    static Trigger fTrigger;

    const int fBoardId;

    double   fStartTime;

    void AsyncRead(ba::mutable_buffers_1 buffers)
    {
        ba::async_read(*this, buffers,
                       boost::bind(&tcp_connection::HandleReceivedData, shared_from_this(),
                                   dummy::error, dummy::bytes_transferred));
    }

    void AsyncWrite(ba::ip::tcp::socket *socket, const ba::const_buffers_1 &buffers)
    {
        ba::async_write(*socket, buffers,
                        boost::bind(&tcp_connection::HandleSentData, shared_from_this(),
                                    dummy::error, dummy::bytes_transferred));
    }
    void AsyncWait(ba::deadline_timer &timer, int seconds,
                               void (tcp_connection::*handler)(const bs::error_code&))// const
    {
        timer.expires_from_now(boost::posix_time::milliseconds(seconds));
        timer.async_wait(boost::bind(handler, shared_from_this(), dummy::error));
    }

    // The constructor is prvate to force the obtained pointer to be shared
    tcp_connection(ba::io_service& ioservice, int boardid) : ba::ip::tcp::socket(ioservice),
        fBoardId(boardid), fRamRoi(kNumChannels), fTriggerSendData(ioservice),
        fTriggerEnabled(false)
    {
        fTrigger.Add(this);
    }
    void PostTrigger(uint32_t triggerid)
    {
        if (fTriggerEnabled)
            get_io_service().post(boost::bind(&tcp_connection::SendData, this, triggerid));
    }

    // Callback when writing was successfull or failed
#ifdef DEBUG_TX
    void HandleSentData(const boost::system::error_code& error, size_t bytes_transferred)
    {
        cout << "Data sent[" << fBoardId << "]: (transmitted=" << bytes_transferred << ") rc=" << error.message() << " (" << error << ")" << endl;
        fOutQueue.pop_front();
    }
#else
    void HandleSentData(const boost::system::error_code&, size_t)
    {
        fOutQueue.pop_front();
    }
#endif

    vector<uint16_t> fBufCommand;

    vector<uint16_t> fCommand;

    FAD::EventHeader   fHeader;
    FAD::EventHeader   fRam;
    FAD::ChannelHeader fChHeader[kNumChannels];

    vector<uint16_t> fRamRoi;

    ba::deadline_timer fTriggerSendData;

    bool fTriggerEnabled;
    bool fCommandSocket;

    int fSocket;

    deque<vector<uint16_t>> fOutQueue;

    void SendData(uint32_t triggerid)
    {
        if (fOutQueue.size()>3)
            return;

        fHeader.fPackageLength = sizeof(EventHeader)/2+1;
        fHeader.fEventCounter++;
        fHeader.fTriggerCounter = triggerid;
        fHeader.fTimeStamp = uint32_t((Time(Time::utc).UnixTime()-fStartTime)*10000);
        fHeader.fFreqRefClock = 997+rand()/(RAND_MAX/7);

        /* Trigger ID

        * Byte[4]: Bit 0:    ext1
        * Byte[4]: Bit 1:    ext2
        * Byte[4]: Bit 2-7:  n/40
        * Byte[5]: Bit 0: LP_1
        * Byte[5]: Bit 1: LP_2
        * Byte[5]: Bit 2: Pedestal
        * Byte[5]: Bit 3:
        * Byte[5]: Bit 4:
        * Byte[5]: Bit 5:
        * Byte[5]: Bit 6:
        * Byte[5]: Bit 7: TIM source

        */

        for (int i=0; i<FAD::kNumTemp; i++)
            fHeader.fTempDrs[i] = (42.+fBoardId/40.+float(rand())/RAND_MAX*5)*16;

        // Header, channel header, end delimiter
        size_t sz = sizeof(fHeader) + kNumChannels*sizeof(FAD::ChannelHeader) + 2;
        // Data
        for (int i=0; i<kNumChannels; i++)
            sz += fChHeader[i].fRegionOfInterest*2;

        vector<uint16_t> evtbuf;
        evtbuf.reserve(sz);

        for (int i=0; i<kNumChannels; i++)
        {
            fChHeader[i].fStartCell = int64_t(1023)*rand()/RAND_MAX;

             vector<int16_t> data(fChHeader[i].fRegionOfInterest, -1024+0x42+i/9+fHeader.fDac[1]/32);

            for (int ii=0; ii<fChHeader[i].fRegionOfInterest; ii++)
            {
                const int rel =  ii;
                const int abs = (ii+fChHeader[i].fStartCell)%fChHeader[i].fRegionOfInterest;

                data[rel] +=  6.*rand()/RAND_MAX +  5*exp(-rel/10); // sigma=10
                data[rel] += 15*sin(2*3.1415*abs/512); // sigma=10
            }

            if (triggerid>0)
            {
                int    p    =   5.*rand()/RAND_MAX+ 20;
                double rndm = 500.*rand()/RAND_MAX+500;
                for (int ii=0; ii<fChHeader[i].fRegionOfInterest; ii++)
                    data[ii] += rndm*exp(-0.5*(ii-p)*(ii-p)/25); // sigma=10
            }

            const vector<uint16_t> buf = fChHeader[i].HtoN();

            evtbuf.insert(evtbuf.end(), buf.begin(), buf.end());
            evtbuf.insert(evtbuf.end(), data.begin(), data.end());

            fHeader.fPackageLength += sizeof(ChannelHeader)/2;
            fHeader.fPackageLength += fChHeader[i].fRegionOfInterest;
        }

        evtbuf.push_back(htons(FAD::kDelimiterEnd));

        const vector<uint16_t> h = fHeader.HtoN();

        evtbuf.insert(evtbuf.begin(), h.begin(), h.end());

        fOutQueue.push_back(evtbuf);

        if (fCommandSocket)
            AsyncWrite(this, ba::buffer(ba::const_buffer(fOutQueue.back().data(), fOutQueue.back().size()*2)));
        else
        {
            if (fSockets.size()==0)
                return;

            fSocket++;
            fSocket %= fSockets.size();

            AsyncWrite(fSockets[fSocket].get(), ba::buffer(ba::const_buffer(fOutQueue.back().data(), fOutQueue.back().size()*2)));
        }
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
        if (fTriggerEnabled)
            SendData(0);

        AsyncWait(fTriggerSendData, fHeader.fTriggerGeneratorPrescaler, &tcp_connection::TriggerSendData);
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

            switch (fBufCommand[0])
            {
            case kCmdDrsEnable:
            case kCmdDrsEnable+0x100:
                fHeader.Enable(FAD::EventHeader::kDenable, fBufCommand[0]==kCmdDrsEnable);
                cout << "-> DrsEnable " << fBoardId << " " << (fBufCommand[0]==kCmdDrsEnable) << endl;
                break;

            case kCmdDwrite:
            case kCmdDwrite+0x100:
                fHeader.Enable(FAD::EventHeader::kDwrite, fBufCommand[0]==kCmdDwrite);
                cout << "-> Dwrite " << fBoardId << " " << (fBufCommand[0]==kCmdDwrite) << endl;
                break;

            case kCmdTriggerLine:
            case kCmdTriggerLine+0x100:
                cout << "-> Trigger line " << fBoardId << " " << (fBufCommand[0]==kCmdTriggerLine) << endl;
                fTriggerEnabled = fBufCommand[0]==kCmdTriggerLine;
                fHeader.Enable(FAD::EventHeader::kTriggerLine, fTriggerEnabled);
                break;

            case kCmdSclk:
            case kCmdSclk+0x100:
                cout << "-> Sclk " << fBoardId << endl;
                fHeader.Enable(FAD::EventHeader::kSpiSclk, fBufCommand[0]==kCmdSclk);
                break;

            case kCmdSrclk:
            case kCmdSrclk+0x100:
                cout << "-> Srclk " << fBoardId << endl;
                break;

            case kCmdRun:
            case kCmdRun+0x100:
                fStartTime = Time(Time::utc).UnixTime();
                cout << "-> Run " << fBoardId << endl;
                break;

            case kCmdBusyOff:
            case kCmdBusyOff+0x100:
                cout << "-> BusyOff " << fBoardId << " " << (fBufCommand[0]==kCmdBusyOff) << endl;
                fHeader.Enable(FAD::EventHeader::kBusyOff, fBufCommand[0]==kCmdBusyOff);
                break;

            case kCmdBusyOn:
            case kCmdBusyOn+0x100:
                cout << "-> BusyOn " << fBoardId << " " << (fBufCommand[0]==kCmdBusyOn) << endl;
                fHeader.Enable(FAD::EventHeader::kBusyOn, fBufCommand[0]==kCmdBusyOn);
                break;

            case kCmdSocket:
            case kCmdSocket+0x100:
                cout << "-> Socket " << fBoardId << " " << (fBufCommand[0]==kCmdSocket) << endl;
                fCommandSocket = fBufCommand[0]==kCmdSocket;
                fHeader.Enable(FAD::EventHeader::kSock17, !fCommandSocket);
                break;

            case kCmdContTrigger:
            case kCmdContTrigger+0x100:
                if (fBufCommand[0]==kCmdContTrigger)
                    AsyncWait(fTriggerSendData, 0, &tcp_connection::TriggerSendData);
                else
                    fTriggerSendData.cancel();
                fHeader.Enable(FAD::EventHeader::kContTrigger, fBufCommand[0]==kCmdContTrigger);
                cout << "-> ContTrig " << fBoardId << " " << (fBufCommand[0]==kCmdContTrigger) << endl;
                break;

            case kCmdResetEventCounter:
                cout << "-> ResetId " << fBoardId << endl;
                fHeader.fEventCounter = 0;
                break;

            case kCmdSingleTrigger:
                cout << "-> Trigger " << fBoardId << endl;
                SendData(0);
                break;

            case kCmdWriteExecute:
                cout << "-> Execute " << fBoardId << endl;
                memcpy(fHeader.fDac, fRam.fDac, sizeof(fHeader.fDac));
                for (int i=0; i<kNumChannels; i++)
                    fChHeader[i].fRegionOfInterest = fRamRoi[i];
                fHeader.fRunNumber = fRam.fRunNumber;
                break;

            case kCmdWriteRunNumberMSW:
                fCommand = fBufCommand;
                break;

            case kCmdWriteRunNumberLSW:
                fCommand = fBufCommand;
                break;

            default:
                if (fBufCommand[0]>=kCmdWriteRoi && fBufCommand[0]<kCmdWriteRoi+kNumChannels)
                {
                    fCommand.resize(2);
                    fCommand[0] = kCmdWriteRoi;
                    fCommand[1] = fBufCommand[0]-kCmdWriteRoi;
                    break;
                }
                if (fBufCommand[0]>= kCmdWriteDac && fBufCommand[0]<kCmdWriteDac+kNumDac)
                {
                    fCommand.resize(2);
                    fCommand[0] = kCmdWriteDac;
                    fCommand[1] = fBufCommand[0]-kCmdWriteDac;
                    break;
                }
                if (fBufCommand[0]==kCmdWriteRate)
                {
                    fCommand.resize(1);
                    fCommand[0] = kCmdWriteRate;
                    break;
                }

                cout << "Received b=" << bytes_received << ": " << error.message() << " (" << error << ")" << endl;
                cout << "Hex:" << Converter::GetHex<uint16_t>(&fBufCommand[0], bytes_received) << endl;
                return;
            }

            fBufCommand.resize(1);
            AsyncRead(ba::buffer(fBufCommand));
            return;
        }

        transform(fBufCommand.begin(), fBufCommand.begin()+bytes_received/2,
                  fBufCommand.begin(), ntohs);

        switch (fCommand[0])
        {
        case kCmdWriteRunNumberMSW:
            fRam.fRunNumber &= 0xffff;
            fRam.fRunNumber |= fBufCommand[0]<<16;
            cout << "-> Set RunNumber " << fBoardId << " MSW" << endl;
            break;
        case kCmdWriteRunNumberLSW:
            fRam.fRunNumber &= 0xffff0000;
            fRam.fRunNumber |= fBufCommand[0];
            cout << "-> Set RunNumber " << fBoardId << " LSW" << endl;
            break;
        case kCmdWriteRoi:
            cout << "-> Set " << fBoardId << " Roi[" << fCommand[1] << "]=" << fBufCommand[0] << endl;
            //fChHeader[fCommand[1]].fRegionOfInterest = fBufCommand[0];
            fRamRoi[fCommand[1]] = fBufCommand[0];
            break;

        case kCmdWriteDac:
            cout << "-> Set " << fBoardId << " Dac[" << fCommand[1] << "]=" << fBufCommand[0] << endl;
            fRam.fDac[fCommand[1]] = fBufCommand[0];
            break;

        case kCmdWriteRate:
            cout << "-> Set " << fBoardId << " Rate =" << fBufCommand[0] << endl;
            fHeader.fTriggerGeneratorPrescaler = fBufCommand[0];
            break;
        }

        fCommand.resize(0);

        fBufCommand.resize(1);
        AsyncRead(ba::buffer(fBufCommand));
    }

public:
    typedef boost::shared_ptr<tcp_connection> shared_ptr;

    static shared_ptr create(ba::io_service& io_service, int boardid)
    {
        return shared_ptr(new tcp_connection(io_service, boardid));
    }

    void start()
    {
        // Ownership of buffer must be valid until Handler is called.

        fTriggerEnabled=false;
        fCommandSocket=true;

        fHeader.fStartDelimiter = FAD::kDelimiterStart;
        fHeader.fVersion = 0x104;
        fHeader.fBoardId = (fBoardId%10) | ((fBoardId/10)<<8);
        fHeader.fRunNumber = 0;
        fHeader.fDNA = reinterpret_cast<uint64_t>(this);
        fHeader.fTriggerGeneratorPrescaler = 100;
        fHeader.fStatus = 0xf<<12 |
            FAD::EventHeader::kDenable    |
            FAD::EventHeader::kDwrite     |
            FAD::EventHeader::kDcmLocked  |
            FAD::EventHeader::kDcmReady   |
            FAD::EventHeader::kSpiSclk;


        fStartTime = Time(Time::utc).UnixTime();

        for (int i=0; i<kNumChannels; i++)
        {
            fChHeader[i].fId = (i%9) | ((i/9)<<4);
            fChHeader[i].fRegionOfInterest = 0;
        }

        // Emit something to be written to the socket
        fBufCommand.resize(1);
        AsyncRead(ba::buffer(fBufCommand));

//        AsyncWait(fTriggerDynData, 1, &tcp_connection::SendDynData);

//        AsyncWrite(ba::buffer(ba::const_buffer(&fHeader, sizeof(FTM::Header))));
//        AsyncWait(deadline_, 3, &tcp_connection::check_deadline);

    }

    vector<boost::shared_ptr<ba::ip::tcp::socket>> fSockets;

    ~tcp_connection()
    {
        fTrigger.Remove(this);
        fSockets.clear();
    }

    void handle_accept(boost::shared_ptr<ba::ip::tcp::socket> socket, int port, const boost::system::error_code&/* error*/)
    {
        cout << this << " Added one socket[" << fBoardId << "] " << socket->remote_endpoint().address().to_v4().to_string();
        cout << ":"<< port << endl;
        fSockets.push_back(socket);
    }
};

Trigger tcp_connection::fTrigger;

void Trigger::commandHandler()
{
    if (!getCommand())
        return;

    for (vector<tcp_connection*>::iterator it=vec.begin();
         it!=vec.end(); it++)
        (*it)->PostTrigger(getCommand()->getInt());
}


class tcp_server
{
    tcp::acceptor acc0;
    tcp::acceptor acc1;
    tcp::acceptor acc2;
    tcp::acceptor acc3;
    tcp::acceptor acc4;
    tcp::acceptor acc5;
    tcp::acceptor acc6;
    tcp::acceptor acc7;

    int fBoardId;

public:
    tcp_server(ba::io_service& ioservice, int port, int board) :
        acc0(ioservice, tcp::endpoint(tcp::v4(), port)),
        acc1(ioservice, tcp::endpoint(tcp::v4(), port+1)),
        acc2(ioservice, tcp::endpoint(tcp::v4(), port+2)),
        acc3(ioservice, tcp::endpoint(tcp::v4(), port+3)),
        acc4(ioservice, tcp::endpoint(tcp::v4(), port+4)),
        acc5(ioservice, tcp::endpoint(tcp::v4(), port+5)),
        acc6(ioservice, tcp::endpoint(tcp::v4(), port+6)),
        acc7(ioservice, tcp::endpoint(tcp::v4(), port+7)),
        fBoardId(board)
    {
        // We could start listening for more than one connection
        // here, but since there is only one handler executed each time
        // it would not make sense. Before one handle_accept is not
        // finished no new handle_accept will be called.
        // Workround: Start a new thread in handle_accept
        start_accept();
    }

private:
    void start_accept(tcp_connection::shared_ptr dest, tcp::acceptor &acc)
    {
        boost::shared_ptr<ba::ip::tcp::socket> connection =
            boost::shared_ptr<ba::ip::tcp::socket>(new ba::ip::tcp::socket(acc.get_io_service()));

        acc.async_accept(*connection,
                          boost::bind(&tcp_connection::handle_accept,
                                      dest, connection,
                                      acc.local_endpoint().port(),
                                      ba::placeholders::error));
    }

    void start_accept()
    {
        cout << "Start accept[" << fBoardId << "] " << acc0.local_endpoint().port() << "..." << flush;
        tcp_connection::shared_ptr new_connection = tcp_connection::create(/*acceptor_.*/acc0.get_io_service(), fBoardId);

        cout << new_connection.get() << " ";

        // This will accept a connection without blocking
        acc0.async_accept(*new_connection,
                          boost::bind(&tcp_server::handle_accept,
                                      this,
                                      new_connection,
                                      ba::placeholders::error));

        start_accept(new_connection, acc1);
        start_accept(new_connection, acc2);
        start_accept(new_connection, acc3);
        start_accept(new_connection, acc4);
        start_accept(new_connection, acc5);
        start_accept(new_connection, acc6);
        start_accept(new_connection, acc7);

        cout << "start-done." << endl;
    }

    void handle_accept(tcp_connection::shared_ptr new_connection, const boost::system::error_code& error)
    {
        // The connection has been accepted and is now ready to use

        // not installing a new handler will stop run()
        cout << new_connection.get() << " Handle accept[" << fBoardId << "]["<<new_connection->fBoardId<<"]..." << flush;
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
        ("port,p",    var<uint16_t>(4000), "")
        ("num,n",     var<uint16_t>(40),   "")
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

    DimServer::start("FAD");

    //try
    {
        ba::io_service io_service;

        const uint16_t n = conf.Get<uint16_t>("num");
        uint16_t port = conf.Get<uint16_t>("port");

        vector<shared_ptr<tcp_server>> servers;

        for (int i=0; i<n; i++)
        {
            shared_ptr<tcp_server> server(new tcp_server(io_service, port, i));
            servers.push_back(server);

            port += 8;
        }

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


