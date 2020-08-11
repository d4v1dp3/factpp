#include <functional>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Console.h"
#include "Converter.h"
#include "HeadersFAD.h"

#include "tools.h"

#include "DimDescriptionService.h"
#include "EventBuilderWrapper.h"

#include "zofits.h"

#ifdef HAVE_NOVA
#include "externals/nova.h"
#endif

namespace ba = boost::asio;
namespace bs = boost::system;

using ba::ip::tcp;

using namespace std;

// ------------------------------------------------------------------------

class ConnectionFAD : public Connection
{
    vector<uint16_t> fBuffer;

protected:
    FAD::EventHeader   fEventHeader;
    FAD::ChannelHeader fChannelHeader[FAD::kNumChannels];

private:
    bool fIsVerbose;
    bool fIsHexOutput;
    bool fIsDataOutput;
    bool fBlockTransmission;

    uint64_t fCounter;

    FAD::EventHeader fBufEventHeader;
    vector<uint16_t> fTargetRoi;

protected:
    void PrintEventHeader()
    {
        Out() << endl << kBold << "Header received (N=" << dec << fCounter << "):" << endl;
        Out() << fEventHeader;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fEventHeader, 16) << endl;
    }

    void PrintChannelHeaders()
    {
        Out() << dec << endl;

        for (unsigned int c=0; c<FAD::kNumChips; c++)
        {
            Out() << "ROI|" << fEventHeader.Crate() << ":" << fEventHeader.Board() << ":" << c << ":";
            for (unsigned int ch=0; ch<FAD::kNumChannelsPerChip; ch++)
		Out() << " " << setw(4) << fChannelHeader[c+ch*FAD::kNumChips].fRegionOfInterest;
            Out() << endl;
        }

        Out() << "CEL|" << fEventHeader.Crate() << ":" <<fEventHeader.Board() << ": ";
        for (unsigned int c=0; c<FAD::kNumChips; c++)
        {
            if (0)//fIsFullChannelHeader)
            {
                for (unsigned int ch=0; ch<FAD::kNumChannelsPerChip; ch++)
                    Out() << " " << setw(4) << fChannelHeader[c+ch*FAD::kNumChips].fStartCell;
                Out() << endl;
            }
            else
            {
                Out() << " ";
                const uint16_t cel = fChannelHeader[c*FAD::kNumChannelsPerChip].fStartCell;
                for (unsigned int ch=1; ch<FAD::kNumChannelsPerChip; ch++)
                    if (cel!=fChannelHeader[c+ch*FAD::kNumChips].fStartCell)
                    {
                        Out() << "!";
                        break;
                    }
                Out() << cel;
            }
        }
        Out() << endl;

        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fChannelHeader, 16) << endl;

    }

    virtual void UpdateFirstHeader()
    {
    }

    virtual void UpdateEventHeader()
    {
        // emit service with trigger counter from header
        if (fIsVerbose)
            PrintEventHeader();
    }

    virtual void UpdateChannelHeaders()
    {
        // emit service with trigger counter from header
        if (fIsVerbose)
            PrintChannelHeaders();

    }

    virtual void UpdateData(const uint16_t *data, size_t sz)
    {
        // emit service with trigger counter from header
        if (fIsVerbose && fIsDataOutput)
            Out() << Converter::GetHex<uint16_t>(data, sz, 16, true) << endl;
    }

private:
    enum
    {
        kReadHeader = 1,
        kReadData   = 2,
    };

    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int type)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection to "+URL()+" closed by remote host (FAD).");

            // 107: Transport endpoint is not connected (bs::error_code(107, bs::system_category))
            // 125: Operation canceled
            if (err && err!=ba::error::eof &&                     // Connection closed by remote host
                err!=ba::error::basic_errors::not_connected &&    // Connection closed by remote host
                err!=ba::error::basic_errors::operation_aborted)  // Connection closed by us
            {
                ostringstream str;
                str << "Reading from " << URL() << ": " << err.message() << " (" << err << ")";// << endl;
                Error(str);
            }
            //PostClose(err!=ba::error::basic_errors::operation_aborted);
            PostClose(false);
            return;
        }

        if (type==kReadHeader)
        {
            if (bytes_received!=sizeof(FAD::EventHeader))
            {
                ostringstream str;
                str << "Bytes received (" << bytes_received << " don't match header size " << sizeof(FAD::EventHeader);
                Error(str);
                PostClose(false);
                return;
            }

            fEventHeader = fBuffer;

            if (fEventHeader.fStartDelimiter!=FAD::kDelimiterStart)
            {
                ostringstream str;
                str << "Invalid header received: start delimiter wrong, received ";
                str << hex << fEventHeader.fStartDelimiter << ", expected " << FAD::kDelimiterStart << ".";
                Error(str);
                PostClose(false);
                return;
            }

            if (fCounter==0)
                UpdateFirstHeader();

            UpdateEventHeader();

            EventBuilderWrapper::This->debugHead(fEventHeader);

            fBuffer.resize(fEventHeader.fPackageLength-sizeof(FAD::EventHeader)/2);
            AsyncRead(ba::buffer(fBuffer), kReadData);
            AsyncWait(fInTimeout, 2000, &Connection::HandleReadTimeout);

            return;
        }

        fInTimeout.cancel();

        if (ntohs(fBuffer.back())!=FAD::kDelimiterEnd)
        {
            ostringstream str;
            str << "Invalid data received: end delimiter wrong, received ";
            str << hex << ntohs(fBuffer.back()) << ", expected " << FAD::kDelimiterEnd << ".";
            Error(str);
            PostClose(false);
            return;
        }

        uint8_t *ptr = reinterpret_cast<uint8_t*>(fBuffer.data());
        uint8_t *end = ptr + fBuffer.size()*2;
        for (unsigned int i=0; i<FAD::kNumChannels; i++)
        {
            if (ptr+sizeof(FAD::ChannelHeader) > end)
            {
                Error("Channel header exceeds buffer size.");
                PostClose(false);
                return;
            }

            fChannelHeader[i] = vector<uint16_t>(reinterpret_cast<uint16_t*>(ptr),
                                                 reinterpret_cast<uint16_t*>(ptr)+sizeof(FAD::ChannelHeader)/2);
            ptr += sizeof(FAD::ChannelHeader);

            //UpdateChannelHeader(i);

            if (ptr+fChannelHeader[i].fRegionOfInterest*2 > end)
            {
                Error("Data block exceeds buffer size.");
                PostClose(false);
                return;
            }

            const uint16_t *data = reinterpret_cast<uint16_t*>(ptr);
            UpdateData(data, fChannelHeader[i].fRegionOfInterest*2);
            ptr += fChannelHeader[i].fRegionOfInterest*2;
        }

        if (fIsVerbose)
            UpdateChannelHeaders();

        fCounter++;

        fBuffer.resize(sizeof(FAD::EventHeader)/2);
        AsyncRead(ba::buffer(fBuffer), kReadHeader);
    }

    void HandleReadTimeout(const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
            return;

        if (error)
        {
            ostringstream str;
            str << "Read timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
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
        if (fInTimeout.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        Error("Timeout reading data from "+URL());
        PostClose(false);
    }

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        fBufEventHeader.clear();
        fBufEventHeader.fEventCounter = 1;
        fBufEventHeader.fStatus = 0xf000|
            FAD::EventHeader::kDenable|
            FAD::EventHeader::kDwrite|
            FAD::EventHeader::kDcmLocked|
            FAD::EventHeader::kDcmReady|
            FAD::EventHeader::kSpiSclk;

        fEventHeader.clear();
        for (unsigned int i=0; i<FAD::kNumChannels; i++)
            fChannelHeader[i].clear();

        fCounter = 0;

        fBuffer.resize(sizeof(FAD::EventHeader)/2);
        AsyncRead(ba::buffer(fBuffer), kReadHeader);

//        for (int i=0; i<36; i++)
//            CmdSetRoi(i, 100);

//        Cmd(FAD::kCmdTriggerLine, true);
//        Cmd(FAD::kCmdSingleTrigger);
    }

public:
    void PostCmd(std::vector<uint16_t> cmd)
    {
        if (fBlockTransmission || !IsConnected())
            return;

#ifdef DEBUG_TX
        ostringstream msg;
        msg << "Sending command:" << hex;
        msg << " 0x" << setw(4) << setfill('0') << cmd[0];
        msg << " (+ " << cmd.size()-1 << " bytes data)";
        Message(msg);
#endif
        transform(cmd.begin(), cmd.end(), cmd.begin(), htons);

        PostMessage(cmd);
    }

    void PostCmd(uint16_t cmd)
    {
        if (fBlockTransmission || !IsConnected())
            return;

#ifdef DEBUG_TX
        ostringstream msg;
        msg << "Sending command:" << hex;
        msg << " 0x" << setw(4) << setfill('0') << cmd;
        Message(msg);
#endif
        cmd = htons(cmd);
        PostMessage(&cmd, sizeof(uint16_t));
    }

    void PostCmd(uint16_t cmd, uint16_t data)
    {
        if (fBlockTransmission || !IsConnected())
            return;

#ifdef DEBUG_TX
        ostringstream msg;
        msg << "Sending command:" << hex;
        msg << " 0x" << setw(4) << setfill('0') << cmd;
        msg << " 0x" << setw(4) << setfill('0') << data;
        Message(msg);
#endif
        const uint16_t d[2] = { htons(cmd), htons(data) };
        PostMessage(d, sizeof(d));
    }

public:
    ConnectionFAD(ba::io_service& ioservice, MessageImp &imp, uint16_t/* slot*/) :
        Connection(ioservice, imp()),
        fIsVerbose(false), fIsHexOutput(false), fIsDataOutput(false),
        fBlockTransmission(false), fCounter(0),
        fTargetRoi(FAD::kNumChannels)
    {
        // Maximum possible needed space:
        // The full header, all channels with all DRS bins
        // Two trailing shorts
        fBuffer.reserve(sizeof(FAD::EventHeader) + FAD::kNumChannels*(sizeof(FAD::ChannelHeader) + FAD::kMaxBins*sizeof(uint16_t)) + 2*sizeof(uint16_t));

        SetLogStream(&imp);
    }

    void Cmd(FAD::Enable cmd, bool on=true)
    {
        switch (cmd)
        {
        case FAD::kCmdDrsEnable:   fBufEventHeader.Enable(FAD::EventHeader::kDenable,     on);  break;
        case FAD::kCmdDwrite:      fBufEventHeader.Enable(FAD::EventHeader::kDwrite,      on);  break;
        case FAD::kCmdTriggerLine: fBufEventHeader.Enable(FAD::EventHeader::kTriggerLine, on);  break;
        case FAD::kCmdBusyOn:      fBufEventHeader.Enable(FAD::EventHeader::kBusyOn,      on);  break;
        case FAD::kCmdBusyOff:     fBufEventHeader.Enable(FAD::EventHeader::kBusyOff,     on);  break;
        case FAD::kCmdContTrigger: fBufEventHeader.Enable(FAD::EventHeader::kContTrigger, on);  break;
        case FAD::kCmdSocket:      fBufEventHeader.Enable(FAD::EventHeader::kSock17,      !on); break;
        default:
            break;
        }

        PostCmd(cmd + (on ? 0 : 0x100));
    }

    // ------------------------------

    // IMPLEMENT: Abs/Rel
    void CmdPhaseShift(int16_t val)
    {
        vector<uint16_t> cmd(abs(val)+2, FAD::kCmdPhaseApply);
        cmd[0] = FAD::kCmdPhaseReset;
        cmd[1] = val<0 ? FAD::kCmdPhaseDecrease : FAD::kCmdPhaseIncrease;
        PostCmd(cmd);
    }

    bool CmdSetTriggerRate(int32_t val)
    {
        if (val<0 || val>0xffff)
            return false;

        fBufEventHeader.fTriggerGeneratorPrescaler = val;
        PostCmd(FAD::kCmdWriteRate, val);//uint8_t(1000./val/12.5));
        //PostCmd(FAD::kCmdWriteExecute);

        return true;
    }

    void CmdSetRunNumber(uint32_t num)
    {
        fBufEventHeader.fRunNumber = num;

        PostCmd(FAD::kCmdWriteRunNumberLSW, num&0xffff);
        PostCmd(FAD::kCmdWriteRunNumberMSW, num>>16);
        PostCmd(FAD::kCmdWriteExecute);
    }

    void CmdSetRegister(uint8_t addr, uint16_t val)
    {
        // Allowed addr:  [0, MAX_ADDR]
        // Allowed value: [0, MAX_VAL]
        PostCmd(FAD::kCmdWrite + addr, val);
        PostCmd(FAD::kCmdWriteExecute);
    }

    bool CmdSetDacValue(int8_t addr, uint16_t val)
    {
        if (addr<0)
        {
            for (unsigned int i=0; i<=FAD::kMaxDacAddr; i++)
            {
                fBufEventHeader.fDac[i] = val;
                PostCmd(FAD::kCmdWriteDac + i, val);
            }
            PostCmd(FAD::kCmdWriteExecute);
            return true;
        }

        if (uint8_t(addr)>FAD::kMaxDacAddr) // NDAC
            return false;

        fBufEventHeader.fDac[addr] = val;

        PostCmd(FAD::kCmdWriteDac + addr, val);
        PostCmd(FAD::kCmdWriteExecute);
        return true;
    }

    bool CmdSetRoi(int8_t addr, uint16_t val)
    {
        if (val>FAD::kMaxRoiValue)
            return false;

        if (addr<0)
        {
            for (unsigned int i=0; i<=FAD::kMaxRoiAddr; i++)
            {
                fTargetRoi[i] = val;
                PostCmd(FAD::kCmdWriteRoi + i, val);
            }
            PostCmd(FAD::kCmdWriteExecute);
            return true;
        }

        if (uint8_t(addr)>FAD::kMaxRoiAddr)
            return false;

        fTargetRoi[addr] = val;

        PostCmd(FAD::kCmdWriteRoi + addr, val);
        PostCmd(FAD::kCmdWriteExecute);
        return true;
    }

    bool CmdSetRoi(uint16_t val) { return CmdSetRoi(-1, val); }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetHexOutput(bool b)
    {
        fIsHexOutput = b;
    }

    void SetDataOutput(bool b)
    {
        fIsDataOutput = b;
    }

    void SetBlockTransmission(bool b)
    {
        fBlockTransmission = b;
    }

    bool IsTransmissionBlocked() const
    {
        return fBlockTransmission;
    }

    void PrintEvent()
    {
        if (fCounter>0)
        {
            PrintEventHeader();
            PrintChannelHeaders();
        }
        else
            Out() << "No event received yet." << endl;
    }

    bool HasCorrectRoi() const
    {
        for (int i=0; i<FAD::kNumChannels; i++)
            if (fTargetRoi[i]!=fChannelHeader[i].fRegionOfInterest)
                return false;

        return true;
    }

    bool HasCorrectHeader() const
    {
        return fEventHeader==fBufEventHeader;
    }

    bool IsConfigured() const
    {
        return HasCorrectRoi() && HasCorrectHeader();
    }

    void PrintCheckHeader()
    {
        Out() << "================================================================================" << endl;
        fEventHeader.print(Out());
        Out() << "--------------------------------------------------------------------------------" << endl;
        fBufEventHeader.print(Out());
        Out() << "================================================================================" << endl;
    }

    const FAD::EventHeader &GetConfiguration() const { return fBufEventHeader; }
};

// ------------------------------------------------------------------------

template <class T>
class StateMachineFAD : public StateMachineAsio<T>, public EventBuilderWrapper
{
private:
    typedef map<uint8_t, ConnectionFAD*> BoardList;

    BoardList fBoards;

    bool fIsVerbose;
    bool fIsHexOutput;
    bool fIsDataOutput;
    bool fDebugTx;

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int Cmd(FAD::Enable command)
    {
        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->Cmd(command);

        return T::GetCurrentState();
    }

    int SendCmd(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SendCmd", 4))
            return T::kSM_FatalError;

        if (evt.GetUInt()>0xffff)
        {
            T::Warn("Command value out of range (0-65535).");
            return T::GetCurrentState();
        }

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->PostCmd(evt.GetUInt());

        return T::GetCurrentState();
    }

    int SendCmdData(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SendCmdData", 8))
            return T::kSM_FatalError;

        const uint32_t *ptr = evt.Ptr<uint32_t>();

        if (ptr[0]>0xffff)
        {
            T::Warn("Command value out of range (0-65535).");
            return T::GetCurrentState();
        }

        if (ptr[1]>0xffff)
        {
            T::Warn("Data value out of range (0-65535).");
            return T::GetCurrentState();
        }

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->PostCmd(ptr[0], ptr[1]);

        return T::GetCurrentState();
    }

    int CmdEnable(const EventImp &evt, FAD::Enable command)
    {
        if (!CheckEventSize(evt.GetSize(), "CmdEnable", 1))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->Cmd(command, evt.GetBool());

        return T::GetCurrentState();
    }

    bool Check(const uint32_t *dat, uint32_t maxaddr, uint32_t maxval)
    {
        if (dat[0]>maxaddr)
        {
            ostringstream msg;
            msg << hex << "Address " << dat[0] << " out of range, max=" << maxaddr << ".";
            T::Error(msg);
            return false;
        }

        if (dat[1]>maxval)
        {
            ostringstream msg;
            msg << hex << "Value " << dat[1] << " out of range, max=" << maxval << ".";
            T::Error(msg);
            return false;
        }

        return true;
    }

    int SetRegister(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetRegister", 8))
            return T::kSM_FatalError;

        const uint32_t *dat = evt.Ptr<uint32_t>();

        if (!Check(dat, FAD::kMaxRegAddr, FAD::kMaxRegValue))
            return T::GetCurrentState();

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->CmdSetRegister(dat[0], dat[1]);

        return T::GetCurrentState();
    }

    int SetRoi(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetRoi", 8))
            return T::kSM_FatalError;

        const int32_t *dat = evt.Ptr<int32_t>();

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            if (!i->second->CmdSetRoi(dat[0], dat[1]))
            {
                ostringstream msg;
                msg << hex << "Channel " << dat[0] << " or Value " << dat[1] << " out of range.";
                T::Error(msg);
                return T::GetCurrentState();
            }


        return T::GetCurrentState();
    }

    int SetDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDac", 8))
            return T::kSM_FatalError;

        const int32_t *dat = evt.Ptr<int32_t>();

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            if (!i->second->CmdSetDacValue(dat[0], dat[1]))
            {
                ostringstream msg;
                msg << hex << "Channel " << dat[0] << " or Value " << dat[1] << " out of range.";
                T::Error(msg);
                return T::GetCurrentState();
            }

        return T::GetCurrentState();
    }

    int Trigger(int n)
    {
        for (int nn=0; nn<n; nn++)
            for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
                i->second->Cmd(FAD::kCmdSingleTrigger);

        return T::GetCurrentState();
    }

    int SendTriggers(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SendTriggers", 4))
            return T::kSM_FatalError;

        Trigger(evt.GetUInt());

        return T::GetCurrentState();
    }
/*
    int StartRun(const EventImp &evt, bool start)
    {
        if (!CheckEventSize(evt.GetSize(), "StartRun", 0))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->Cmd(FAD::kCmdRun, start);

        return T::GetCurrentState();
    }
*/
    int PhaseShift(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "PhaseShift", 2))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->CmdPhaseShift(evt.GetShort());

        return T::GetCurrentState();
    }

    int SetTriggerRate(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerRate", 4))
            return T::kSM_FatalError;

        if (evt.GetUInt()>0xffff)
        {
            ostringstream msg;
            msg << hex << "Value " << evt.GetUShort() << " out of range, max=" << 0xffff << "(?)";
            T::Error(msg);
            return T::GetCurrentState();
        }

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->CmdSetTriggerRate(evt.GetUInt());

        return T::GetCurrentState();
    }

    int SetRunNumber(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetRunNumber", 8))
            return T::kSM_FatalError;

        const uint64_t num = evt.GetUXtra();

        if (num<=0 || num>FAD::kMaxRunNumber)
        {
            ostringstream msg;
            msg << "Run number " << num << " out of range [1;" << FAD::kMaxRunNumber << "]";
            T::Error(msg);
            return T::GetCurrentState();
        }

        if (!IncreaseRunNumber(num))
            return T::GetCurrentState();
 
        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->CmdSetRunNumber(GetRunNumber());

        return T::GetCurrentState();
    }

    int SetMaxMemoryBuffer(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetMaxMemoryBuffer", 2))
            return T::kSM_FatalError;

        const int16_t mem = evt.GetShort();

        if (mem<=0)
        {
            ostringstream msg;
            msg << hex << "Value " << mem << " out of range.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        SetMaxMemory(mem);

        return T::GetCurrentState();
    }

    int SetEventTimeoutSec(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetEventTimeoutSec", 2))
            return T::kSM_FatalError;

        const int16_t sec = evt.GetShort();

        if (sec<=0)
        {
            ostringstream msg;
            msg << hex << "Value " << sec << " out of range.";
            T::Error(msg);
            return T::GetCurrentState();
        }

        SetEventTimeout(sec);

        return T::GetCurrentState();
    }

    int SetFileFormat(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetFileFormat", 2))
            return T::kSM_FatalError;

        const uint16_t fmt = evt.GetUShort();

        // A simple way to make sure that no invalid file format
        // is passed to the event builder
	switch (fmt)
	{
        case FAD::kNone:
        case FAD::kDebug:
        case FAD::kFits:
        case FAD::kZFits:
        case FAD::kCfitsio:
        case FAD::kRaw:
        case FAD::kCalib:
            SetOutputFormat(FAD::FileFormat_t(fmt));
            break;
	default:
            T::Error("File format unknonw.");
            return T::GetCurrentState();
        }

        return T::GetCurrentState();
    }

    int StartDrsCalibration()
    {
        SetOutputFormat(FAD::kCalib);
        return T::GetCurrentState();
    }

    int ResetSecondaryDrsBaseline()
    {
        EventBuilderWrapper::ResetSecondaryDrsBaseline();
        return T::GetCurrentState();
    }

    int LoadDrsCalibration(const EventImp &evt)
    {
        EventBuilderWrapper::LoadDrsCalibration(evt.GetText());
        return T::GetCurrentState();
    }
/*
    int Test(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "Test", 2))
            return T::kSM_FatalError;


        SetMode(evt.GetShort());

        return T::GetCurrentState();
    }*/


    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetHexOutput(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetHexOutput", 1))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->SetHexOutput(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDataOutput(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDataOutput", 1))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->SetDataOutput(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDebugTx(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDebugTx", 1))
            return T::kSM_FatalError;

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            i->second->SetDebugTx(evt.GetBool());

        return T::GetCurrentState();
    }

    const BoardList::iterator GetSlot(uint16_t slot)
    {
        const BoardList::iterator it=fBoards.find(slot);
        if (it==fBoards.end())
        {
            ostringstream str;
            str << "Slot " << slot << " not found.";
            T::Warn(str);
        }

        return it;
    }

    int PrintEvent(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "PrintEvent", 2))
            return T::kSM_FatalError;

        const int16_t slot = evt.Get<int16_t>();

        if (slot<0)
        {
            for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
                i->second->PrintEvent();
        }
        else
        {
            const BoardList::iterator it=GetSlot(slot);
            if (it!=fBoards.end())
                it->second->PrintEvent();
        }

        return T::GetCurrentState();
    }

    int SetBlockTransmission(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetBlockTransmission", 3))
            return T::kSM_FatalError;

        const int16_t slot = evt.Get<int32_t>();

        const BoardList::iterator it=GetSlot(slot);
        if (it!=fBoards.end())
            it->second->SetBlockTransmission(evt.Get<uint8_t>(2));

        return T::GetCurrentState();
    }

    int SetBlockTransmissionRange(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetBlockTransmissionRange", 5))
            return T::kSM_FatalError;

        const int16_t *slot  = evt.Ptr<int16_t>();
        const bool     block = evt.Get<uint8_t>(4);

        for (int i=slot[0]; i<=slot[1]; i++)
        {
            const BoardList::iterator it=GetSlot(i);
            if (it!=fBoards.end())
                it->second->SetBlockTransmission(block);
        }

        return T::GetCurrentState();
    }

    int SetIgnoreSlot(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetIgnoreSlot", 3))
            return T::kSM_FatalError;

        const uint16_t slot = evt.Get<uint16_t>();

        if (slot>39)
        {
            T::Warn("Slot out of range (0-39).");
            return T::GetCurrentState();
        }

        SetIgnore(slot, evt.Get<uint8_t>(2));

        return T::GetCurrentState();
    }

    int SetIgnoreSlots(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetIgnoreSlots", 5))
            return T::kSM_FatalError;

        const int16_t *slot  = evt.Ptr<int16_t>();
        const bool     block = evt.Get<uint8_t>(4);

        if (slot[0]<0 || slot[1]>39 || slot[0]>slot[1])
        {
            T::Warn("Slot out of range.");
            return T::GetCurrentState();
        }

        for (int i=slot[0]; i<=slot[1]; i++)
            SetIgnore(i, block);

        return T::GetCurrentState();
    }

    int StartConfigure(const EventImp &evt)
    {
        const string name = evt.Ptr<char>(16);

        fTargetConfig = fConfigs.find(name);
        if (fTargetConfig==fConfigs.end())
        {
            T::Error("StartConfigure - Run-type '"+name+"' not found.");
            return T::GetCurrentState();
        }

        // FIXME: What about an error state?
        const uint32_t runno = StartNewRun(evt.Get<int64_t>(), evt.Get<int64_t>(8), *fTargetConfig);
        if (runno==0)
            return FAD::State::kConnected;

        ostringstream str;
        str << "Starting configuration for run " << runno << " (" << name << ")";
        T::Message(str.str());

        if (runno>=1000)
            T::Warn("Run number exceeds logical maximum of 999 - this is no problem for writing but might give raise to problems in the analysis.");

        const FAD::Configuration &conf = fTargetConfig->second;

        for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
        {
            ConnectionFAD &fad  = *it->second;

            fad.Cmd(FAD::kCmdBusyOn,      true);  // continously on
            fad.Cmd(FAD::kCmdTriggerLine, false);
            fad.Cmd(FAD::kCmdContTrigger, false);
            fad.Cmd(FAD::kCmdSocket,      true);
            fad.Cmd(FAD::kCmdBusyOff,     false);  // normal when BusyOn==0

            fad.Cmd(FAD::kCmdDwrite,      conf.fDwrite);
            fad.Cmd(FAD::kCmdDrsEnable,   conf.fDenable);

            for (int i=0; i<FAD::kNumDac; i++)
                fad.CmdSetDacValue(i, conf.fDac[i]);

            for (int i=0; i<FAD::kNumChips; i++)
                for (int j=0; j<FAD::kNumChannelsPerChip; j++)
                    fad.CmdSetRoi(i*FAD::kNumChannelsPerChip+j, conf.fRoi[j]);

            fad.CmdSetTriggerRate(conf.fTriggerRate);
            fad.CmdSetRunNumber(runno);
            fad.Cmd(FAD::kCmdResetEventCounter);
            fad.Cmd(FAD::kCmdTriggerLine, true);
            //fad.Cmd(FAD::kCmdSingleTrigger);
            //fad.Cmd(FAD::kCmdTriggerLine, true);
        }

        // Now the old run is stopped already. So all other servers can start a new run
        // (Note that we might need another step which only checks if the continous trigger
        //  is wwitched off, too)
        const int64_t runs[2] = { runno, runno+1 };
        fDimStartRun.Update(runs);

        return FAD::State::kConfiguring1;
    }

    int ResetConfig()
    {
        const int64_t runs[2] = { -1, GetRunNumber() };
        fDimStartRun.Update(runs);

        return FAD::State::kConnected;
    }

    void CloseRun(uint32_t runid)
    {
        if (runid==GetRunNumber()-1)
            ResetConfig();
    }

    int AddAddress(const EventImp &evt)
    {
        const string addr = Tools::Trim(evt.GetText());

        const tcp::endpoint endpoint = GetEndpoint(addr);
        if (endpoint==tcp::endpoint())
            return T::GetCurrentState();

        for (BoardList::const_iterator i=fBoards.begin(); i!=fBoards.end(); i++)
        {
            if (i->second->GetEndpoint()==endpoint)
            {
               T::Warn("Address "+addr+" already known.... ignored.");
               return T::GetCurrentState();
            }
        }

        AddEndpoint(endpoint);

        return T::GetCurrentState();
    }

    int RemoveSlot(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "RemoveSlot", 2))
            return T::kSM_FatalError;

        const int16_t slot = evt.GetShort();

        const BoardList::iterator it = GetSlot(slot);

        if (it==fBoards.end())
            return T::GetCurrentState();

        ConnectSlot(slot, tcp::endpoint());

        delete it->second;
        fBoards.erase(it);

        return T::GetCurrentState();
    }

    int ListSlots()
    {
        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
        {
            const int           &idx = i->first;
            const ConnectionFAD *fad = i->second;

            ostringstream str;
            str << "Slot " << setw(2) << idx << ": " << fad->GetEndpoint();

            if (fad->IsConnecting())
                str << " (0:connecting, ";
            else
            {
                if (fad->IsClosed())
                    str << " (0:disconnected, ";
                if (fad->IsConnected())
                    str << " (0:connected, ";
            }

            switch (fStatus2[idx])
            {
            case 0:  str << "1:disconnected)"; break;
            case 8:  str << "1:connected)";     break;
            default: str << "1:connecting)";    break;
            }

            if (fad->IsTransmissionBlocked())
                str << " [cmd_blocked]";

            if (fStatus2[idx]==8 && IsIgnored(idx))
                str << " [data_ignored]";

            if (fad->IsConnected() && fStatus2[idx]==8 && fad->IsConfigured())
                str << " [configured]";

            T::Out() << str.str() << endl;
        }

        T::Out() << "Event builder thread:";
        if (!IsThreadRunning())
            T::Out() << " not";
        T::Out() << " running" << endl;

        // FIXME: Output state

        return T::GetCurrentState();
    }

    void EnableConnection(ConnectionFAD *ptr, bool enable=true)
    {
        if (!enable)
        {
            ptr->PostClose(false);
            return;
        }

        if (!ptr->IsDisconnected())
        {
            ostringstream str;
            str << ptr->GetEndpoint();

            T::Warn("Connection to "+str.str()+" already in progress.");
            return;
        }

	ptr->StartConnect();
    }

    void EnableAll(bool enable=true)
    {
        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            EnableConnection(i->second, enable);
    }

    int CloseOpenFiles()
    {
        EventBuilderWrapper::CloseOpenFiles();
        return T::GetCurrentState();
    }

    int EnableSlot(const EventImp &evt, bool enable)
    {
        if (!CheckEventSize(evt.GetSize(), "EnableSlot", 2))
            return T::kSM_FatalError;

        const int16_t slot = evt.GetShort();

        const BoardList::iterator it = GetSlot(slot);
        if (it==fBoards.end())
            return T::GetCurrentState();

        EnableConnection(it->second, enable);
        ConnectSlot(it->first, enable ? it->second->GetEndpoint() : tcp::endpoint());

        return T::GetCurrentState();
    }

    int ToggleSlot(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ToggleSlot", 2))
            return T::kSM_FatalError;

        const int16_t slot = evt.GetShort();

        const BoardList::iterator it = GetSlot(slot);
        if (it==fBoards.end())
            return T::GetCurrentState();

        const bool enable = it->second->IsDisconnected();

        EnableConnection(it->second, enable);
        ConnectSlot(it->first, enable ? it->second->GetEndpoint() : tcp::endpoint());

        return T::GetCurrentState();
    }

    int StartConnection()
    {
        vector<tcp::endpoint> addr(40);

        for (BoardList::iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            addr[i->first] = i->second->GetEndpoint();

        StartThread(addr);
        EnableAll(true);

        return T::GetCurrentState();
    }

    int StopConnection()
    {
        Exit();
        EnableAll(false);
        return T::GetCurrentState();
    }

    int AbortConnection()
    {
        Abort();
        EnableAll(false);
        return T::GetCurrentState();
    }

    int Reset(bool soft)
    {
        ResetThread(soft);
        return T::GetCurrentState();
    }

    // ============================================================================

    int SetupZFits(const EventImp &evt, const std::function<void(int32_t)> &func)
    {
        if (!CheckEventSize(evt.GetSize(), "SetupZFits", 2))
            return T::kSM_FatalError;

        func(evt.GetShort());
        return T::GetCurrentState();
    }


    // ============================================================================
/*
    bool ProcessReconnection(const list<ReconnectionSlot>::iterator &it)
    {
        auto board = GetSlot(it->slot);
        if (board==fBoards.end())
            return false;

        ConnectionFAD *fad  = board->second;

        // ----------------------------------------------
        // Disconnect
        // ----------------------------------------------
        if (it->state==0)
        {
            if (!fad->IsConnected())
                return false;

            EnableConnection(fad, false);
            ConnectSlot(it->slot, tcp::endpoint());

            it->time  = Time();
            it->state = 1;

            return true;
        }

        // ----------------------------------------------
        // Wait for disconnect or timeout
        // ----------------------------------------------
        if (it->state==1)
        {
            if (!fad->IsDisconnected() && it->time+boost::posix_time::seconds(10)>Time())
                return true;

            it->time  = Time();
            it->state = 2;

            return true;
        }

        // ----------------------------------------------
        // Wait for timeout after disconnect / Re-connect
        // ----------------------------------------------
        if (it->state==2)
        {
            if (it->time+boost::posix_time::seconds(3)>Time())
                return true;

            EnableConnection(fad, true);
            ConnectSlot(it->slot, fad->GetEndpoint());

            it->time  = Time();
            it->state = 3;

            return true;
        }

        // ----------------------------------------------
        // Wait for connect or timeout / Re-start
        // ----------------------------------------------
        if (!fad->IsConnected() && it->time+boost::posix_time::seconds(10)>Time())
            return true;

        // 'Fix' the information which got lost during re-connection
        fad->Cmd(FAD::kCmdBusyOff,     false);
        fad->Cmd(FAD::kCmdSocket,      false);
        fad->Cmd(FAD::kCmdTriggerLine, true);

        return false;
    }
*/
    // ============================================================================

    vector<uint8_t> fStatus1;
    vector<uint8_t> fStatus2;
    bool            fStatusT;

    int Execute()
    {
        // ===== Evaluate connection status =====

        uint16_t nclosed1     = 0;
        uint16_t nconnecting1 = 0;
        uint16_t nconnecting2 = 0;
        uint16_t nconnected1  = 0;
        uint16_t nconnected2  = 0;
        uint16_t nconfigured  = 0;

        vector<uint8_t> stat1(40);
        vector<uint8_t> stat2(40);

        int cnt = 0; // counter for enabled board

        const bool runs = IsThreadRunning();

        for (int idx=0; idx<40; idx++)
        {
            // ----- Command socket -----
            const BoardList::const_iterator &slot = fBoards.find(idx);
            if (slot!=fBoards.end())
            {
                const ConnectionFAD *c = slot->second;
                if (c->IsDisconnected())
                {
                    stat1[idx] = 0;
                    nclosed1++;

                    //DisconnectSlot(idx);
                }
                if (c->IsConnecting())
                {
                    stat1[idx] = 1;
                    nconnecting1++;
                }
                if (c->IsConnected())
                {
                    stat1[idx] = 2;
                    nconnected1++;

                    if (c->IsConfigured())
                    {
                        stat1[idx] = 3;
                        nconfigured++;
                    }
                }

                cnt++;
            }

            // ----- Event builder -----

            stat2[idx] = 0; // disconnected
            if (!runs)
                continue;

            if (IsConnecting(idx))
            {
                nconnecting2++;
                stat2[idx] = 1; // connecting
            }

            if (IsConnected(idx))
            {
                nconnected2++;
                stat2[idx] = 8; // connected
            }
        }

        // ===== Send connection status via dim =====

        if (fStatus1!=stat1 || fStatus2!=stat2 || fStatusT!=runs)
        {
            fStatus1 = stat1;
            fStatus2 = stat2;
            fStatusT = runs;
            UpdateConnectionStatus(stat1, stat2, runs);
        }

        // ===== Return connection status =====

        // Keep the state during reconnection (theoretically, can only be WritingData)
/*        if (fReconnectionList.size()>0)
        {
            bool isnew = true;
            for (auto it=fReconnectionList.begin(); it!=fReconnectionList.end(); it++)
                if (it->state>0)
                {
                    isnew = false;
                    break;
                }

            if (isnew)
            {
                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                    it->second->Cmd(FAD::kCmdBusyOn, true);  // continously on
            }

            // Loop over all scheduled re-connections
            for (auto it=fReconnectionList.begin(); it!=fReconnectionList.end(); it++)
            {
                if (ProcessReconnection(it))
                    continue;

                const lock_guard<mutex> guard(fMutexReconnect);
                fReconnectionList.erase(it);
            }

            if (fReconnectionList.size()==0)
            {
                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                    it->second->Cmd(FAD::kCmdBusyOff, false);
            }

            return T::GetCurrentState();
        }
*/
        // fadctrl:       Always connecting if not disabled
        // event builder:
        if (nconnecting1==0 && nconnected1>0 && nconnected2==nconnected1)
        {
            if (T::GetCurrentState()==FAD::State::kConfiguring1)
            {
                // Wait until the configuration commands to all boards
                // have been sent and achknowledged
                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                    if (!it->second->IsTxQueueEmpty())
                        return FAD::State::kConfiguring1;

                // Note that if there are less than 40 boards, this
                // can be so fast that the single trigger still
                // comes to early, and a short watiting is necessary :(
                usleep(250000);

                // Now we can sent the trigger
                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                    it->second->Cmd(FAD::kCmdSingleTrigger);

                return FAD::State::kConfiguring2;
            }

            // If all boards are configured and we are configuring
            // go on and start the FADs
            if (T::GetCurrentState()==FAD::State::kConfiguring2)
            {
                // If not all boards have yet received the proper
                // configuration
                if (nconfigured!=nconnected1)
                    return FAD::State::kConfiguring2;

                // FIXME: Distinguish between not all boards have received
                // the configuration and the configuration is not consistent

                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                {
                    ConnectionFAD &fad  = *it->second;

                    // Make sure that after switching on the trigger line
                    // there needs to be some waiting before all boards
                    // can be assumed to be listening
                    fad.Cmd(FAD::kCmdResetEventCounter);
                    fad.Cmd(FAD::kCmdSocket,      false);
                    //fad.Cmd(FAD::kCmdTriggerLine, true);
                    if (fTargetConfig->second.fContinousTrigger)
                        fad.Cmd(FAD::kCmdContTrigger, true);
                    fad.Cmd(FAD::kCmdBusyOn,      false);  // continously on

                    // FIXME: How do we find out when the FADs
                    //        successfully enabled the trigger lines?
                }

//                const lock_guard<mutex> guard(fMutexReconnect);
//                fReconnectionList.clear();

                return FAD::State::kConfiguring3;
            }

            if (T::GetCurrentState()==FAD::State::kConfiguring3)
            {
                // Wait until the configuration commands to all boards
                // have been sent and achknowledged
                for (BoardList::iterator it=fBoards.begin(); it!=fBoards.end(); it++)
                    if (!it->second->IsTxQueueEmpty())
                        return FAD::State::kConfiguring3;

                usleep(250000);

                return FAD::State::kConfigured;
            }

            if (T::GetCurrentState()==FAD::State::kConfigured)
            {
                // Stay in Configured as long as we have a valid
                // configuration and the run has not yet been started
                // (means the the event builder has received its
                // first event)
                if (IsRunWaiting() && nconfigured==nconnected1)
                    return FAD::State::kConfigured;

                if (!IsRunWaiting())
                    T::Message("Run successfully started... first data received.");
                if (nconfigured!=nconnected1)
                    T::Message("Configuration of some boards changed.");
            }

            // FIXME: Rename WritingData to TakingData
            return IsRunInProgress() ? FAD::State::kRunInProgress : FAD::State::kConnected;
        }

        if (nconnecting1>0 || nconnecting2>0 || nconnected1!=nconnected2)
            return FAD::State::kConnecting;

        // nconnected1 == nconnected2 == 0
        return runs ? FAD::State::kDisconnected : FAD::State::kOffline;
    }

    void AddEndpoint(const tcp::endpoint &addr)
    {
        int i=0;
        while (i<40)
        {
            if (fBoards.find(i)==fBoards.end())
                break;
            i++;
        }

        if (i==40)
        {
            T::Warn("Not more than 40 slots allowed.");
            return;
        }

        ConnectionFAD *fad = new ConnectionFAD(*this, *this, i);

        fad->SetEndpoint(addr);
        fad->SetVerbose(fIsVerbose);
        fad->SetHexOutput(fIsHexOutput);
        fad->SetDataOutput(fIsDataOutput);
        fad->SetDebugTx(fDebugTx);

        fBoards[i] = fad;
    }


    DimDescribedService fDimStartRun;
    DimDescribedService fDimConnection;

    void UpdateConnectionStatus(const vector<uint8_t> &stat1, const vector<uint8_t> &stat2, bool thread)
    {
        vector<uint8_t> stat(41);

        for (int i=0; i<40; i++)
            stat[i] = stat1[i]|(stat2[i]<<3);

        stat[40] = thread;

        fDimConnection.Update(stat);
    }

public:
    StateMachineFAD(ostream &out=cout) :
        StateMachineAsio<T>(out, "FAD_CONTROL"),
        EventBuilderWrapper(*static_cast<MessageImp*>(this)),
        fStatus1(40), fStatus2(40), fStatusT(false),
        fDimStartRun("FAD_CONTROL/START_RUN", "X:1;X:1",
                                              "Run numbers"
                                              "|run[idx]:Run no of last conf'd run (-1 if reset or none config'd yet)"
                                              "|next[idx]:Run number which will be assigned to next configuration"),
        fDimConnection("FAD_CONTROL/CONNECTIONS", "C:40;C:1",
                                                  "Connection status of FAD boards"
                                                  "|status[bitpattern]:lower bits stat1, upper bits stat2, for every board. 40=thread"
                                                  "|thread[bool]:true or false whether the event builder threads are running")
    {
        ResetConfig();
        SetOutputFormat(FAD::kNone);

        // State names
        T::AddStateName(FAD::State::kOffline, "Disengaged",
                        "All enabled FAD boards are disconnected and the event-builer thread is not running.");

        T::AddStateName(FAD::State::kDisconnected, "Disconnected",
                        "All enabled FAD boards are disconnected, but the event-builder thread is running.");

        T::AddStateName(FAD::State::kConnecting, "Connecting",
                        "Only some enabled FAD boards are connected.");

        T::AddStateName(FAD::State::kConnected, "Connected",
                        "All enabled FAD boards are connected..");

        T::AddStateName(FAD::State::kConfiguring1, "Configuring1",
                        "Waiting 3 seconds for all FADs to be configured before requesting configuration.");

        T::AddStateName(FAD::State::kConfiguring2, "Configuring2",
                        "Waiting until all boards returned their configuration and they are valid.");

        T::AddStateName(FAD::State::kConfiguring3, "Configuring3",
                        "Waiting until 'enable trigger line' was sent to all boards.");

        T::AddStateName(FAD::State::kConfigured, "Configured",
                        "The configuration of all boards was successfully cross checked. Waiting for events with a new run number to receive.");

        T::AddStateName(FAD::State::kRunInProgress, "RunInProgress",
                        "Events currently received by the event builder will be flagged to be written, no end-of-run event occured yet.");

        // FAD Commands
        T::AddEvent("SEND_CMD", "I:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SendCmd, this, placeholders::_1))
            ("Send a command to the FADs. Values between 0 and 0xffff are allowed."
             "|command[uint16]:Command to be transmittted.");
        T::AddEvent("SEND_DATA", "I:2", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SendCmdData, this, placeholders::_1))
            ("Send a command with data to the FADs. Values between 0 and 0xffff are allowed."
             "|command[uint16]:Command to be transmittted."
             "|data[uint16]:Data to be sent with the command.");

        T::AddEvent("ENABLE_SRCLK", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdSrclk))
            ("Set SRCLK");
        T::AddEvent("ENABLE_BUSY_OFF", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdBusyOff))
            ("Set BUSY continously low");
        T::AddEvent("ENABLE_BUSY_ON", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdBusyOn))
            ("Set BUSY constantly high (has priority over BUSY_OFF)");
        T::AddEvent("ENABLE_SCLK", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdSclk))
            ("Set SCLK");
        T::AddEvent("ENABLE_DRS", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdDrsEnable))
            ("Switch Domino wave");
        T::AddEvent("ENABLE_DWRITE", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdDwrite))
            ("Set Dwrite (possibly high / always low)");
        T::AddEvent("ENABLE_CONTINOUS_TRIGGER", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdContTrigger))
            ("Enable continous (internal) trigger.");
        T::AddEvent("ENABLE_TRIGGER_LINE", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdTriggerLine))
            ("Incoming triggers can be accepted/will not be accepted");
        T::AddEvent("ENABLE_COMMAND_SOCKET_MODE", "B:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CmdEnable, this, placeholders::_1, FAD::kCmdSocket))
            ("Set debug mode (yes: dump events through command socket, no=dump events through other sockets)");

        T::AddEvent("SET_TRIGGER_RATE", "I:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SetTriggerRate, this, placeholders::_1))
            ("Enable continous trigger");
        T::AddEvent("SEND_SINGLE_TRIGGER")
            (bind(&StateMachineFAD::Trigger, this, 1))
            ("Issue software triggers");
        T::AddEvent("SEND_N_TRIGGERS", "I", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SendTriggers, this, placeholders::_1))
            ("Issue N software triggers (note that these are the triggers sent, not the triggers executed)"
             "|N[int]: Number of triggers to be sent to the board.");
        /*
        T::AddEvent("START_RUN", "", FAD::kConnecting, FAD::kConnected, FAD::kRunInProgress)
            (bind(&StateMachineFAD::StartRun, this, placeholders::_1, true))
            ("Set FAD DAQ mode. when started, no configurations must be send.");
        T::AddEvent("STOP_RUN", FAD::kConnecting, FAD::kConnected, FAD::kRunInProgress)
            (bind(&StateMachineFAD::StartRun, this, placeholders::_1, false))
            ("");
            */
        T::AddEvent("PHASE_SHIFT", "S:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::PhaseShift, this, placeholders::_1))
            ("Adjust ADC phase (in 'steps')"
             "|phase[short]");

        T::AddEvent("RESET_EVENT_COUNTER", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::Cmd, this, FAD::kCmdResetEventCounter))
            ("Reset the FAD boards' event counter to 0.");

        T::AddEvent("SET_RUN_NUMBER", "X:1", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SetRunNumber, this, placeholders::_1))
            ("Sent a new run-number to the boards"
             "|num[int]:Run number");

        T::AddEvent("SET_MAX_MEMORY", "S:1")
            (bind(&StateMachineFAD::SetMaxMemoryBuffer, this, placeholders::_1))
            ("Set maximum memory buffer size allowed to be consumed by the EventBuilder to buffer events."
             "|memory[short]:Buffer size in Mega-bytes.");

        T::AddEvent("SET_EVENT_TIMEOUT", "S:1")
            (bind(&StateMachineFAD::SetEventTimeoutSec, this, placeholders::_1))
            ("Set the timeout after which an event expires which was not completely received yet."
             "|timeout[sec]:Timeout in seconds [1;32767]");

        T::AddEvent("SET_REGISTER", "I:2", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SetRegister, this, placeholders::_1))
            ("set register to value"
            "|addr[short]:Address of register"
            "|val[short]:Value to be set");

        // FIXME:  Maybe add a mask which channels should be set?
        T::AddEvent("SET_REGION_OF_INTEREST", "I:2", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SetRoi, this, placeholders::_1))
            ("Set region-of-interest to value"
            "|channel[short]:Channel on each chip for which the ROI is set (0-8), -1 for all"
            "|val[short]:Value to be set");

        // FIXME:  Maybe add a mask which channels should be set?
        T::AddEvent("SET_DAC_VALUE", "I:2", FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::SetDac, this, placeholders::_1))
            ("Set DAC numbers in range to value"
            "|addr[short]:Address of register (-1 for all)"
            "|val[short]:Value to be set");

        T::AddEvent("CONFIGURE", "X:2;C", FAD::State::kConnected, FAD::State::kConfigured, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::StartConfigure, this, placeholders::_1))
            ("Configure a new run. If the internla trigger is enabled this might even start a new run."
             "|time_max[s]:Maximum time before the run is closed in seconds (0: unlimited)"
             "|num_max[int]:Maximum number of events before the run is closed in seconds (0: unlimited)"
             "|run_type[string]:Run type which describes the runs");

        T::AddEvent("RESET_CONFIGURE", FAD::State::kConfiguring1, FAD::State::kConfiguring2, FAD::State::kConfiguring3, FAD::State::kConfigured)
            (bind(&StateMachineFAD::ResetConfig, this))
            ("If configuration failed and the fadctrl is waiting for something, use this to reset the state.");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineFAD::SetVerbosity, this, placeholders::_1))
            ("Set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("SET_HEX_OUTPUT", "B:1")
            (bind(&StateMachineFAD::SetHexOutput, this, placeholders::_1))
            ("Enable or disable hex output for received data"
             "|hexout[bool]:disable or enable hex output for received data (yes/no)");

        T::AddEvent("SET_DATA_OUTPUT", "B:1")
            (bind(&StateMachineFAD::SetDataOutput, this, placeholders::_1))
            ("Enable or disable printing of the received adc data to the console"
             "|dataout[bool]:disable or enable data output for received data (yes/no)");

        T::AddEvent("SET_DEBUG_TX", "B:1")
            (bind(&StateMachineFAD::SetDebugTx, this, placeholders::_1))
	    ("Enable or disable the output of messages in case of successfull data transmission to the boards."
	     "|debug[bool]:disable or enable debug output for transmitted data (yes/no)");

        T::AddEvent("PRINT_EVENT", "S:1")
            (bind(&StateMachineFAD::PrintEvent, this, placeholders::_1))
            ("Print (last) event"
             "|board[short]:slot from which the event should be printed (-1 for all)");

        T::AddEvent("BLOCK_TRANSMISSION", "S:1;B:1")
            (bind(&StateMachineFAD::SetBlockTransmission, this, placeholders::_1))
            ("Blocks the transmission of commands to the given slot. Use with care! For debugging pupose only!"
             "|slot[short]:Slot to which the command transmission should be blocked (0-39)"
             "|enable[bool]:Whether the command transmission should be blockes (yes) or allowed (no)");

        T::AddEvent("BLOCK_TRANSMISSION_RANGE", "S:2;B:1")
            (bind(&StateMachineFAD::SetBlockTransmissionRange, this, placeholders::_1))
            ("Blocks the transmission of commands to the given range of slots. Use with care! For debugging pupose only!"
             "|first[short]:First slot to which the command transmission should be blocked (0-39)"
             "|last[short]:Last slot to which the command transmission should be blocked (0-39)"
             "|enable[bool]:Whether the command transmission should be blockes (yes) or allowed (no)");

        T::AddEvent("IGNORE_EVENTS", "S:1;B:1")
            (bind(&StateMachineFAD::SetIgnoreSlot, this, placeholders::_1))
            ("Instructs the event-builder to ignore events from the given slot but still read the data from the socket."
             "|slot[short]:Slot from which the data should be ignored when building events"
             "|enable[bool]:Whether the event builder should ignore data from this slot (yes) or allowed (no)");

        T::AddEvent("IGNORE_EVENTS_RANGE", "S:2;B:1")
            (bind(&StateMachineFAD::SetIgnoreSlots, this, placeholders::_1))
            ("Instructs the event-builder to ignore events from the given slot but still read the data from the socket."
             "|first[short]:First slot from which the data should be ignored when building events"
             "|last[short]:Last slot from which the data should be ignored when building events"
             "|enable[bool]:Whether the event builder should ignore data from this slot (yes) or allowed (no)");

        T::AddEvent("CLOSE_OPEN_FILES", FAD::State::kDisconnected, FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::CloseOpenFiles, this))
            ("Close all run files opened by the EventBuilder.");

        //T::AddEvent("TEST", "S:1")
        //   (bind(&StateMachineFAD::Test, this, placeholders::_1))
        //    ("");



        // Conenction commands
        T::AddEvent("START", FAD::State::kOffline)
            (bind(&StateMachineFAD::StartConnection, this))
            ("Start EventBuilder thread and connect all valid slots.");

        T::AddEvent("STOP",  FAD::State::kDisconnected, FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::StopConnection, this))
            ("Stop EventBuilder thread (still write buffered events) and disconnect all slots.");

        T::AddEvent("ABORT", FAD::State::kDisconnected, FAD::State::kConnecting, FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::AbortConnection, this))
            ("Immediately abort EventBuilder thread and disconnect all slots.");

        T::AddEvent("SOFT_RESET", FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::Reset, this, true))
            ("Wait for buffers to drain, close all files and reinitialize event builder thread.");

        T::AddEvent("HARD_RESET", FAD::State::kConnected, FAD::State::kRunInProgress)
            (bind(&StateMachineFAD::Reset, this, false))
            ("Free all buffers, close all files and reinitialize event builder thread.");

        T::AddEvent("CONNECT", "S:1", FAD::State::kDisconnected, FAD::State::kConnecting, FAD::State::kConnected)
            (bind(&StateMachineFAD::EnableSlot, this, placeholders::_1, true))
            ("Connect a disconnected slot.");

        T::AddEvent("DISCONNECT", "S:1", FAD::State::kConnecting, FAD::State::kConnected)
            (bind(&StateMachineFAD::EnableSlot, this, placeholders::_1, false))
            ("Disconnect a connected slot.");

        T::AddEvent("TOGGLE", "S:1", FAD::State::kDisconnected, FAD::State::kConnecting, FAD::State::kConnected)
            (bind(&StateMachineFAD::ToggleSlot, this, placeholders::_1))
            ("Toggle the status of a slot.");

        T::AddEvent("SET_FILE_FORMAT", "S:1")
            (bind(&StateMachineFAD::SetFileFormat, this, placeholders::_1))
            ("Set the output file format (see FAD::FileFormat_t)");

        T::AddEvent("START_DRS_CALIBRATION")
            (bind(&StateMachineFAD::StartDrsCalibration, this))
            ("Start a drs calibration (shortcut for SET_FILEFORMAT 4)");

        T::AddEvent("RESET_SECONDARY_DRS_BASELINE")
            (bind(&StateMachineFAD::ResetSecondaryDrsBaseline, this))
            ("Reset the secondary drs baseline (e.g. to change roi)");

        T::AddEvent("LOAD_DRS_CALIBRATION", "C")
            (bind(&StateMachineFAD::LoadDrsCalibration, this, placeholders::_1))
            ("Load a DRS calibration file"
             "|absolute path");


        // --------- Setup compression of FITS files -----------
        T::AddEvent("SET_ZFITS_DEFAULT_NUM_THREADS", "S")
            (bind(&StateMachineFAD::SetupZFits, this, placeholders::_1, zofits::DefaultNumThreads))
            ("Set the number of compression threads to use (+1 for writing)"
             "|num[int]:Number of threads");
        T::AddEvent("SET_ZFITS_DEFAULT_MAX_MEMORY", "S")
            (bind(&StateMachineFAD::SetupZFits, this, placeholders::_1, zofits::DefaultMaxMemory))
            ("Set the maximum amount of memory zfits will use for compression"
             "|mem[int]:Memory in MB");
        T::AddEvent("SET_ZFITS_DEFAULT_NUM_TILES", "S")
            (bind(&StateMachineFAD::SetupZFits, this, placeholders::_1, zofits::DefaultMaxNumTiles))
            ("Set the number of tiles with which the catalog is initialized"
             "|num[int]:Number of tiles");
        T::AddEvent("SET_ZFITS_DEFAULT_ROWS_PER_TILE", "S")
            (bind(&StateMachineFAD::SetupZFits, this, placeholders::_1, zofits::DefaultNumRowsPerTile))
            ("Set the number of rows which are compressed into one tile"
             "|num[int]:Number of rows per tile");


        T::AddEvent("ADD_ADDRESS", "C", FAD::State::kOffline)
            (bind(&StateMachineFAD::AddAddress, this, placeholders::_1))
            ("Add the address of a DRS4 board to the first free slot"
             "|IP[string]:address in the format <address:port>");
        T::AddEvent("REMOVE_SLOT", "S:1", FAD::State::kOffline)
            (bind(&StateMachineFAD::RemoveSlot, this, placeholders::_1))
            ("Remove the Iaddress in slot n. For a list see LIST"
             "|slot[short]:Remove the address in slot n from the list");
        T::AddEvent("LIST_SLOTS")
            (bind(&StateMachineFAD::ListSlots, this))
            ("Print a list of all available board addressesa and whether they are enabled");
    }

    ~StateMachineFAD()
    {
        for (BoardList::const_iterator i=fBoards.begin(); i!=fBoards.end(); i++)
            delete i->second;
        fBoards.clear();
    }

    tcp::endpoint GetEndpoint(const string &base)
    {
        const size_t p0 = base.find_first_of(':');
        const size_t p1 = base.find_last_of(':');

        if (p0==string::npos || p0!=p1)
        {
           T::Out() << kRed << "GetEndpoint - Wrong format ('host:port' expected)" << endl;
           return tcp::endpoint();
        }

        tcp::resolver resolver(StateMachineAsio<T>::get_io_service());

        boost::system::error_code ec;

        const tcp::resolver::query query(base.substr(0, p0), base.substr(p0+1));
        const tcp::resolver::iterator iterator = resolver.resolve(query, ec);

        if (ec)
        {
           T::Out() << kRed << "GetEndpoint - Couldn't resolve endpoint '" << base << "': " << ec.message();
           return tcp::endpoint();
        }

        return *iterator;
    }

    typedef map<string, FAD::Configuration> Configs;
    Configs fConfigs;
    Configs::const_iterator fTargetConfig;


    template<class V>
    bool CheckConfigVal(Configuration &conf, V max, const string &name, const string &sub)
    {
        if (!conf.HasDef(name, sub))
        {
            T::Error("Neither "+name+"default nor "+name+sub+" found.");
            return false;
        }

        const V val = conf.GetDef<V>(name, sub);

        if (val<=max)
            return true;

        ostringstream str;
        str << name << sub << "=" << val << " exceeds allowed maximum of " << max << "!";
        T::Error(str);

        return false;
    }

    int EvalOptions(Configuration &conf)
    {
        // ---------- General setup ---------
        fIsVerbose = !conf.Get<bool>("quiet");
        fIsHexOutput = conf.Get<bool>("hex-out");
        fIsDataOutput = conf.Get<bool>("data-out");
        fDebugTx = conf.Get<bool>("debug-tx");

#ifdef HAVE_NOVA
        T::Info("Preset observatory: "+Nova::LnLatPosn::preset()+" [PRESET_OBSERVATORY]");
#endif

        // --------- Setup compression of FITS files -----------
        if (conf.Has("zfits.num-threads"))
            zofits::DefaultNumThreads(conf.Get<int32_t>("zfits.num-threads"));
        if (conf.Has("zfits.max-mem"))
            zofits::DefaultMaxMemory(conf.Get<uint32_t>("zfits.max-mem")*1000);
        if (conf.Has("zfits.num-tiles"))
            zofits::DefaultMaxNumTiles(conf.Get<uint32_t>("zfits.num-tiles"));
        if (conf.Has("zfits.num-rows"))
            zofits::DefaultNumRowsPerTile(conf.Get<uint32_t>("zfits.num-rows"));

        // ---------- Setup event builder ---------
        SetMaxMemory(conf.Get<unsigned int>("max-mem"));
        SetEventTimeout(conf.Get<uint16_t>("event-timeout"));

        if (!InitRunNumber(conf.Get<string>("destination-folder")))
            return 1;

        // ---------- Setup run types ---------
        const vector<string> types = conf.Vec<string>("run-type");
        if (types.size()==0)
            T::Warn("No run-types defined.");
        else
            T::Message("Defining run-types");
        for (vector<string>::const_iterator it=types.begin();
             it!=types.end(); it++)
        {
            T::Message(" -> "+ *it);

            if (fConfigs.count(*it)>0)
            {
                T::Error("Run-type "+*it+" defined twice.");
                return 2;
            }

            FAD::Configuration target;

            if (!CheckConfigVal<bool>(conf, true, "enable-drs.",               *it) ||
                !CheckConfigVal<bool>(conf, true, "enable-dwrite.",            *it) ||
                !CheckConfigVal<bool>(conf, true, "enable-continous-trigger.", *it))
                return 3;

            target.fDenable          = conf.GetDef<bool>("enable-drs.", *it);
            target.fDwrite           = conf.GetDef<bool>("enable-dwrite.", *it);
            target.fContinousTrigger = conf.GetDef<bool>("enable-continous-trigger.", *it);

            target.fTriggerRate = 0;
            //if (target.fContinousTrigger)
            {
                if (!CheckConfigVal<uint16_t>(conf, 0xffff, "trigger-rate.", *it))
                    return 4;

                target.fTriggerRate = conf.GetDef<uint16_t>("trigger-rate.", *it);
            }

            for (int i=0; i<FAD::kNumChannelsPerChip; i++)
            {
                ostringstream str;
                str << "roi-ch" << i << '.';

                if (!CheckConfigVal<uint16_t>(conf, FAD::kMaxRoiValue, "roi.",    *it) &&
                    !CheckConfigVal<uint16_t>(conf, FAD::kMaxRoiValue, str.str(), *it))
                    return 5;

                target.fRoi[i] = conf.HasDef(str.str(), *it) ?
                    conf.GetDef<uint16_t>(str.str(), *it) :
                    conf.GetDef<uint16_t>("roi.",    *it);
            }

            for (int i=0; i<FAD::kNumDac; i++)
            {
                ostringstream str;
                str << "dac-" << i << '.';

                if (!CheckConfigVal<uint16_t>(conf, FAD::kMaxDacValue, "dac.",    *it) &&
                    !CheckConfigVal<uint16_t>(conf, FAD::kMaxDacValue, str.str(), *it))
                    return 6;

                target.fDac[i] = conf.HasDef(str.str(), *it) ?
                    conf.GetDef<uint16_t>(str.str(), *it) :
                    conf.GetDef<uint16_t>("dac.",    *it);
            }

            fConfigs[*it] = target;
        }

        // FIXME: Add a check about unsused configurations

        // ---------- Setup board addresses for fake-fad ---------

        if (conf.Has("debug-addr"))
        {
            const string addr = conf.Get<string>("debug-addr");
            const int    num  = conf.Get<unsigned int>("debug-num");

            const tcp::endpoint endpoint = GetEndpoint(addr);
            if (endpoint==tcp::endpoint())
                return 7;

            for (int i=0; i<num; i++)
                AddEndpoint(tcp::endpoint(endpoint.address(), endpoint.port()+8*i));

            if (conf.Get<bool>("start"))
                StartConnection();
            return -1;
        }

        // ---------- Setup board addresses for the real camera ---------

        if (conf.Has("base-addr"))
        {
            string base = conf.Get<string>("base-addr");

            if (base=="def" || base =="default")
                base = "10.0.128.128:31919";

            const tcp::endpoint endpoint = GetEndpoint(base);
            if (endpoint==tcp::endpoint())
                return 8;

            const ba::ip::address_v4::bytes_type ip = endpoint.address().to_v4().to_bytes();

            if (ip[2]>250 || ip[3]>244)
            {
                T::Out() << kRed << "EvalConfiguration - IP address given by --base-addr out-of-range." << endl;
                return 9;
            }

            for (int crate=0; crate<4; crate++)
                for (int board=0; board<10; board++)
                {
                    ba::ip::address_v4::bytes_type target = endpoint.address().to_v4().to_bytes();
                    target[2] += crate;
                    target[3] += board;

                    AddEndpoint(tcp::endpoint(ba::ip::address_v4(target), endpoint.port()));
                }

            if (conf.Get<bool>("start"))
                StartConnection();
            return -1;

        }

        // ---------- Setup board addresses one by one ---------

        if (conf.Has("addr"))
        {
            const vector<string> addrs = conf.Vec<string>("addr");
            for (vector<string>::const_iterator i=addrs.begin(); i<addrs.end(); i++)
            {
                const tcp::endpoint endpoint = GetEndpoint(*i);
                if (endpoint==tcp::endpoint())
                    return 10;

                AddEndpoint(endpoint);
            }

            if (conf.Get<bool>("start"))
                StartConnection();
            return -1;
        }
        return -1;
    }

};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineFAD<S>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("FAD control options");
    control.add_options()
        ("quiet,q",  po_bool(true), "Disable printing contents of all received messages in clear text.")
        ("hex-out",  po_bool(), "Enable printing contents of all printed messages also as hex data.")
        ("data-out", po_bool(), "Enable printing received event data.")
        ("debug-tx", po_bool(), "Enable debugging of ethernet transmission.")
        ;

    po::options_description connect("FAD connection options");
    connect.add_options()
        ("addr",        vars<string>(), "Network address of FAD")
        ("base-addr",   var<string>(),  "Base address of all FAD")
        ("debug-num,n", var<unsigned int>(40),  "Sets the number of fake boards to be connected locally")
        ("debug-addr",  var<string>(),  "")
        ("start",       po_bool(false), "Start the connction immediately after boot")
        ;

    po::options_description builder("Event builder options");
    builder.add_options()
        ("max-mem",            var<unsigned int>(100), "Maximum memory the event builder thread is allowed to consume for its event buffer")
        ("event-timeout",      var<uint16_t>(30),      "After how many seconds is an event considered to be timed out? (<=0: disabled)")
        ("destination-folder", var<string>(""),        "Destination folder (base folder) for the event builder binary data files.")
        ;

    po::options_description zfits("FITS compression options");
    zfits.add_options()
        ("zfits.num-threads", var<int32_t>(),  "Number of threads to spawn writing compressed FITS files")
        ("zfits.max-mem",     var<uint32_t>(), "Maximum amount of memory to be allocated by FITS compression in MB")
        ("zfits.num-tiles",   var<uint32_t>(), "Maximum number of tiles in the catalog")
        ("zfits.num-rows",    var<uint32_t>(), "Maximum number of rows per tile")
        ;

    po::options_description runtype("Run type configuration");
    runtype.add_options()
        ("run-type",                     vars<string>(),        "Run type, e.g. data, pedestal, drs-calibration, light-pulser")
        ("enable-dwrite.*",              var<bool>(),           "")
        ("enable-drs.*",                 var<bool>(),           "")
        ("enable-continous-trigger.*",   var<bool>(),           "")
        ("trigger-rate.*",               var<uint16_t>(),       "")
        ("dac.*",                        var<uint16_t>(),       "")
        ("dac-0.*",                      var<uint16_t>(),       "")
        ("dac-1.*",                      var<uint16_t>(),       "")
        ("dac-2.*",                      var<uint16_t>(),       "")
        ("dac-3.*",                      var<uint16_t>(),       "")
        ("dac-4.*",                      var<uint16_t>(),       "")
        ("dac-5.*",                      var<uint16_t>(),       "")
        ("dac-6.*",                      var<uint16_t>(),       "")
        ("dac-7.*",                      var<uint16_t>(),       "")
        ("roi.*",                        var<uint16_t>(),       "")
        ("roi-ch0.*",                    var<uint16_t>(),       "")
        ("roi-ch1.*",                    var<uint16_t>(),       "")
        ("roi-ch2.*",                    var<uint16_t>(),       "")
        ("roi-ch3.*",                    var<uint16_t>(),       "")
        ("roi-ch4.*",                    var<uint16_t>(),       "")
        ("roi-ch5.*",                    var<uint16_t>(),       "")
        ("roi-ch6.*",                    var<uint16_t>(),       "")
        ("roi-ch7.*",                    var<uint16_t>(),       "")
        ("roi-ch8.*",                    var<uint16_t>(),       "")
        ;

    conf.AddEnv("dns",  "DIM_DNS_NODE");
    conf.AddEnv("host", "DIM_HOST_NODE");

    conf.AddOptions(control);
    conf.AddOptions(connect);
    conf.AddOptions(builder);
    conf.AddOptions(zfits);
    conf.AddOptions(runtype);
}

void PrintUsage()
{
    cout <<
        "The fadctrl controls the FAD boards.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: fadctrl [-c type] [OPTIONS]\n"
        "  or:  fadctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineFAD<StateMachine>>();

    /* Additional help text which is printed after the configuration
     options goes here */
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

//    try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
//            if (conf.Get<bool>("no-dim"))
//                return RunShell<LocalStream, StateMachine>(conf);
//            else
                return RunShell<LocalStream, StateMachineDim>(conf);
        }

        // Cosole access w/ and w/o Dim
/*        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine>(conf);
            else
                return RunShell<LocalConsole, StateMachine>(conf);
        }
        else
*/        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim>(conf);
        }
    }
/*    catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
