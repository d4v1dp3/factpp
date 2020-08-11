#include <array>

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

#include "tools.h"

#include "HeadersFTM.h"


namespace ba = boost::asio;
namespace bs = boost::system;

using namespace std;

// ------------------------------------------------------------------------

class ConnectionFTM : public Connection
{
public:
    enum States
    {
        // State Machine states
        kDisconnected = StateMachineImp::kSM_UserMode,
        kConnected,
        kIdle,
        kConfigured,  // Returned if idle and fBufStaticData==fStaticData
        kTriggerOn,
    };

private:
    vector<uint16_t> fBuffer;

    bool fHasHeader;

    bool fIsVerbose;
    bool fIsDynamicOut;
    bool fIsHexOutput;

protected:
    map<uint16_t, uint32_t> fCounter;

    FTM::Header      fHeader;
    FTM::FtuList     fFtuList;
    FTM::StaticData  fStaticData;    // fStaticBufferTx
    FTM::DynamicData fDynamicData;
    FTM::Error       fError;

    FTM::StaticData  fBufStaticData; // fStaticBufferRx

    virtual void UpdateFirstHeader()
    {
        // FIXME: Message() ?
        Out() << endl << kBold << "First header received:" << endl;
        Out() << fHeader;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fHeader, 16) << endl;
    }

    virtual void UpdateHeader()
    {
        // emit service with trigger counter from header
        if (!fIsVerbose)
            return;

        if (fHeader.fType==FTM::kDynamicData && !fIsDynamicOut)
            return;

        Out() << endl << kBold << "Header received:" << endl;
        Out() << fHeader;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fHeader, 16) << endl;
    }

    virtual void UpdateFtuList()
    {
        if (!fIsVerbose)
            return;

        Out() << endl << kBold << "FtuList received:" << endl;
        Out() << fFtuList;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fFtuList, 16) << endl;
    }

    virtual void UpdateStaticData()
    {
        if (!fIsVerbose)
            return;

        Out() << endl << kBold << "Static data received:" << endl;
        Out() << fStaticData;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fStaticData, 16) << endl;
    }

    virtual void UpdateDynamicData()
    {
        if (!fIsDynamicOut)
            return;

        Out() << endl << kBold << "Dynamic data received:" << endl;
        Out() << fDynamicData;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fDynamicData, 16) << endl;
    }

    virtual void UpdateError()
    {
        if (!fIsVerbose)
            return;

        Out() << endl << kRed << "Error received:" << endl;
        Out() << fError;
        if (fIsHexOutput)
            Out() << Converter::GetHex<uint16_t>(fError, 16) << endl;
    }

    virtual void UpdateCounter()
    {
        if (!fIsVerbose)
            return;

        if (!fIsDynamicOut)
            return;

        Out() << "Received: ";
        Out() << "H=" << fCounter[FTM::kHeader] << "  ";
        Out() << "S=" << fCounter[FTM::kStaticData] << "  ";
        Out() << "D=" << fCounter[FTM::kDynamicData] << "  ";
        Out() << "F=" << fCounter[FTM::kFtuList] << "  ";
        Out() << "E=" << fCounter[FTM::kErrorList] << "  ";
        Out() << "R=" << fCounter[FTM::kRegister] << endl;
    }

    bool CheckConsistency(FTM::StaticData &data)
    {
        bool warn1 = false;
        if (data.IsEnabled(FTM::StaticData::kPedestal) != (data.GetSequencePed()  >0) ||
            data.IsEnabled(FTM::StaticData::kLPint)    != (data.GetSequenceLPint()>0) ||
            data.IsEnabled(FTM::StaticData::kLPext)    != (data.GetSequenceLPext()>0))
        {
            warn1 = true;
            data.Enable(FTM::StaticData::kPedestal, data.GetSequencePed()>0);
            data.Enable(FTM::StaticData::kLPint,    data.GetSequenceLPint()>0);
            data.Enable(FTM::StaticData::kLPext,    data.GetSequenceLPext()>0);
        }

        bool warn2 = false;
        const uint16_t ref = data[0].fPrescaling;
        for (int i=1; i<40; i++)
        {
            if (data[i].fPrescaling != ref)
            {
                warn2 = true;
                data[i].fPrescaling = ref;
            }
        }

        bool warn3 = false;
        for (int i=0; i<4; i++)
            if (data.fActiveFTU[i]!=0x3ff)
            {
                warn3 = true;
                data.fActiveFTU[i]=0x3ff;
            }



        if (warn1)
            Warn("GeneralSettings not consistent with trigger sequence.");
        if (warn2)
            Warn("Prescaling not consistent for all boards.");
        if (warn3)
            Warn("Not all FTUs are enabled - enable all FTUs.");

        return !warn1 && !warn2 && !warn3;
    }

private:
    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int /*type*/)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host (FTM).");

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
            PostClose(err!=ba::error::basic_errors::operation_aborted);
            return;
        }

        // If we have not yet received a header we expect one now
        // This could be moved to a HandleReceivedHeader function
        if (!fHasHeader)
        {
            if (bytes_received!=sizeof(FTM::Header))
            {
                ostringstream str;
                str << "Excepted " << sizeof(FTM::Header) << " bytes (FTM::Header) but received " << bytes_received << ".";
                Error(str);
                PostClose(false);
                return;
            }

            fHeader = fBuffer;

            // Check the data integrity
            if (fHeader.fDelimiter!=FTM::kDelimiterStart)
            {
                ostringstream str;
                str << "Invalid header received: start delimiter wrong, received ";
                str << hex << fHeader.fDelimiter << ", expected " << FTM::kDelimiterStart << ".";
                Error(str);
                PostClose(false);
                return;
            }

            fHasHeader = true;

            // Convert FTM state into FtmCtrl state
            if (++fCounter[FTM::kHeader]==1)
                UpdateFirstHeader();

            UpdateCounter();
            UpdateHeader();

            // Start reading of data
            switch (fHeader.fType)
            {
            case FTM::kStaticData:
            case FTM::kDynamicData:
            case FTM::kFtuList:
            case FTM::kRegister:
            case FTM::kErrorList:
                // This is not very efficient because the space is reallocated
                // maybe we can check if the capacity of the std::vector
                // is ever decreased. If not, everythign is fine.
                fBuffer.resize(fHeader.fDataSize);
                AsyncRead(ba::buffer(fBuffer));
                AsyncWait(fInTimeout, 1000, &Connection::HandleReadTimeout);
                return;

            default:
                ostringstream str;
                str << "Unknonw type " << fHeader.fType << " in received header." << endl;
                Error(str);
                PostClose(false);
                return;
            }

            return;
        }

        // Check the data integrity (check end delimiter)
        if (ntohs(fBuffer.back())!=FTM::kDelimiterEnd)
        {
            ostringstream str;
            str << "Invalid data received: end delimiter wrong, received ";
            str << hex << ntohs(fBuffer.back()) << ", expected " << FTM::kDelimiterEnd << ".";
            Error(str);
            PostClose(false);
            return;
        }

        // Remove end delimiter
        fBuffer.pop_back();

        try
        {
            // If we have already received a header this is the data now
            // This could be moved to a HandleReceivedData function

            fCounter[fHeader.fType]++;
            UpdateCounter();

            switch (fHeader.fType)
            {
            case FTM::kFtuList:
                fFtuList = fBuffer;
                UpdateFtuList();
                break;

            case FTM::kStaticData:
                if (fCounter[FTM::kStaticData]==1)
                {
                    // This check is only done at startup
                    FTM::StaticData data(fBuffer);

                    if (fIsVerbose)
                    {
                        Out() << endl << kBold << "Static data received:" << endl;
                        Out() << data;
                        if (fIsHexOutput)
                            Out() << Converter::GetHex<uint16_t>(data, 16) << endl;
                    }

                    if (!CheckConsistency(data))
                    {
                        CmdSendStatDat(data);
                        CmdPing(); // FIXME: Only needed in case of warn3
                        break;
                    }
                }

                fStaticData = fBuffer;

                // is this the first received static data block?
                if (!fBufStaticData.valid())
                    fBufStaticData = fStaticData;

                UpdateStaticData();
                break;

            case FTM::kDynamicData:
                fDynamicData = fBuffer;
                UpdateDynamicData();
                break;

            case FTM::kRegister:
                if (fIsVerbose)
                {
                    Out() << endl << kBold << "Register received: " << endl;
                    Out() << "Addr:  " << ntohs(fBuffer[0]) << endl;
                    Out() << "Value: " << ntohs(fBuffer[1]) << endl;
                }
                break;

            case FTM::kErrorList:
                fError = fBuffer;
                UpdateError();
                break;

            default:
                ostringstream str;
                str << "Unknonw type " << fHeader.fType << " in header." << endl;
                Error(str);
                PostClose(false);
                return;
            }
        }
        catch (const logic_error &e)
        {
            ostringstream str;
            str << "Exception converting buffer into data structure: " << e.what();
            Error(str);
            PostClose(false);
            return;
        }

        fInTimeout.cancel();

        //fHeader.clear();
        fHasHeader = false;
        fBuffer.resize(sizeof(FTM::Header)/2);
        AsyncRead(ba::buffer(fBuffer));
    }

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        fCounter.clear();
        fBufStaticData.clear();

        fHeader.clear();
        fHasHeader = false;
        fBuffer.resize(sizeof(FTM::Header)/2);
        AsyncRead(ba::buffer(fBuffer));

//        if (!fDefaultSetup.empty())
//            LoadStaticData(fDefaultSetup);

        // Get a header and configdata!
        CmdReqStatDat();

        // get the DNA of the FTUs
        CmdPing();
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

            PostClose();
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

        Error("Timeout ("+to_simple_string(fInTimeout.expires_from_now())+") reading data from "+URL());

        PostClose();
    }

    template<size_t N>
    void PostCmd(array<uint16_t, N> dat, uint16_t u1=0, uint16_t u2=0, uint16_t u3=0, uint16_t u4=0)
    {
        array<uint16_t, 5> cmd = {{ '@', u1, u2, u3, u4 }};

        if (fIsVerbose)
        {
            ostringstream msg;
            msg << "Sending command:" << hex;
            msg << " 0x" << setw(4) << setfill('0') << cmd[0];
            msg << " 0x" << setw(4) << setfill('0') << u1;
            msg << " 0x" << setw(4) << setfill('0') << u2;
            msg << " 0x" << setw(4) << setfill('0') << u3;
            msg << " 0x" << setw(4) << setfill('0') << u4;
            msg << " (+" << dec << dat.size() << " words)";
            Message(msg);
        }

        vector<uint16_t> out(cmd.size()+dat.size());

        transform(cmd.begin(), cmd.end(), out.begin(), htons);
        transform(dat.begin(), dat.end(), out.begin()+cmd.size(), htons);

        PostMessage(out);
    }

    void PostCmd(vector<uint16_t> dat, uint16_t u1=0, uint16_t u2=0, uint16_t u3=0, uint16_t u4=0)
    {
        array<uint16_t, 5> cmd = {{ '@', u1, u2, u3, u4 }};

        if (fIsVerbose)
        {
            ostringstream msg;
            msg << "Sending command:" << hex;
            msg << " 0x" << setw(4) << setfill('0') << cmd[0];
            msg << " 0x" << setw(4) << setfill('0') << u1;
            msg << " 0x" << setw(4) << setfill('0') << u2;
            msg << " 0x" << setw(4) << setfill('0') << u3;
            msg << " 0x" << setw(4) << setfill('0') << u4;
            msg << " (+" << dec << dat.size() << " words)";
            Message(msg);
        }

        vector<uint16_t> out(cmd.size()+dat.size());

        transform(cmd.begin(), cmd.end(), out.begin(), htons);
        copy(dat.begin(), dat.end(), out.begin()+cmd.size());

        PostMessage(out);
    }

    void PostCmd(uint16_t u1=0, uint16_t u2=0, uint16_t u3=0, uint16_t u4=0)
    {
        PostCmd(array<uint16_t, 0>(), u1, u2, u3, u4);
    }
public:

//    static const uint16_t kMaxAddr;

public:
    ConnectionFTM(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fIsDynamicOut(true), fIsHexOutput(true)
    {
        SetLogStream(&imp);
    }

    void CmdToggleLed()
    {
        PostCmd(FTM::kCmdToggleLed);
    }

    void CmdPing()
    {
        PostCmd(FTM::kCmdPing);
    }

    void CmdReqDynDat()
    {
        PostCmd(FTM::kCmdRead, FTM::kCmdDynamicData);
    }

    void CmdReqStatDat()
    {
        PostCmd(FTM::kCmdRead, FTM::kCmdStaticData);
    }

    void CmdSendStatDat(const FTM::StaticData &data)
    {
        fBufStaticData = data;

        PostCmd(data.HtoN(), FTM::kCmdWrite, FTM::kCmdStaticData);

        // Request the changed configuration to ensure the
        // change is distributed in the network
        CmdReqStatDat();
    }

    void CmdStartRun(bool log=true)
    {
        PostCmd(FTM::kCmdStartRun, FTM::kStartRun);
        CmdGetRegister(0);

        if (log)
            Info("Sending start trigger.");
    }

    void CmdStopRun()
    {
        PostCmd(FTM::kCmdStopRun);
        CmdGetRegister(0);

        Info("Sending stop trigger.");
    }

    void CmdTakeNevents(uint32_t n)
    {
        const array<uint16_t, 2> data = {{ uint16_t(n>>16), uint16_t(n&0xffff) }};
        PostCmd(data, FTM::kCmdStartRun, FTM::kTakeNevents);

        // Update state information by requesting a new header
        CmdGetRegister(0);
    }

    bool CmdSetRegister(uint16_t addr, uint16_t val)
    {
        if (addr>FTM::StaticData::kMaxAddr)
            return false;

        const array<uint16_t, 2> data = {{ addr, val }};
        PostCmd(data, FTM::kCmdWrite, FTM::kCmdRegister);

        reinterpret_cast<uint16_t*>(&fBufStaticData)[addr] = val;

        // Request the changed configuration to ensure the
        // change is distributed in the network
        CmdReqStatDat();

        return true;
    }

    bool CmdGetRegister(uint16_t addr)
    {
        if (addr>FTM::StaticData::kMaxAddr)
            return false;

        const array<uint16_t, 1> data = {{ addr }};
        PostCmd(data, FTM::kCmdRead, FTM::kCmdRegister);

        return true;
    }

    bool CmdResetCrate(uint16_t addr)
    {
        if (addr>3)
            return false;

        PostCmd(FTM::kCmdCrateReset, 1<<addr);
        Info("Sending crate reset for crate "+to_string(addr));

        return true;
    }

    bool CmdResetCamera()
    {
        PostCmd(FTM::kCmdCrateReset, FTM::kResetCrate0);
        PostCmd(FTM::kCmdCrateReset, FTM::kResetCrate1);
        PostCmd(FTM::kCmdCrateReset, FTM::kResetCrate2);
        PostCmd(FTM::kCmdCrateReset, FTM::kResetCrate3);

        Info("Sending camera reset");

        return true;
    }

    bool CmdDisableReports(bool b)
    {
        PostCmd(FTM::kCmdDisableReports, b ? uint16_t(0) : uint16_t(1));
        return true;
    }


    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetHexOutput(bool b)
    {
        fIsHexOutput = b;
    }

    void SetDynamicOut(bool b)
    {
        fIsDynamicOut = b;
    }
/*
    void SetDefaultSetup(const string &file)
    {
        fDefaultSetup = file;
    }
*/

    bool LoadStaticData(string name)
    {
        if (name.rfind(".bin")!=name.length()-4)
            name += ".bin";

        ifstream fin(name);
        if (!fin)
            return false;

        FTM::StaticData data;

        fin.read(reinterpret_cast<char*>(&data), sizeof(FTM::StaticData));

        if (fin.gcount()<streamsize(sizeof(FTM::StaticData)))
            return false;

        if (fin.fail() || fin.eof())
            return false;

        if (fin.peek()!=-1)
            return false;

        CmdSendStatDat(data);

        return true;
    }

    bool SaveStaticData(string name) const
    {
        if (name.rfind(".bin")!=name.length()-4)
            name += ".bin";

        ofstream fout(name);
        if (!fout)
            return false;

        fout.write(reinterpret_cast<const char*>(&fStaticData), sizeof(FTM::StaticData));

        return !fout.bad();
    }

    bool SetThreshold(int32_t patch, int32_t value)
    {
        if (patch>FTM::StaticData::kMaxPatchIdx)
            return false;

        if (value<0 || value>FTM::StaticData::kMaxDAC)
            return false;

        if (patch<0)
        {
            FTM::StaticData data(fBufStaticData);

            bool ident = true;
            for (int i=0; i<=FTM::StaticData::kMaxPatchIdx; i++)
                if (data[i/4].fDAC[i%4] != value)
                {
                    ident = false;
                    break;
                }

            if (ident)
                return true;

            for (int i=0; i<=FTM::StaticData::kMaxPatchIdx; i++)
                data[i/4].fDAC[i%4] = value;

            // Maybe move to a "COMMIT" command?
            CmdSendStatDat(data);

            return true;
        }

        /*
          if (data[patch/4].fDAC[patch%4] == value)
             return true;
          */
 
        // Calculate offset in static data block
        const uint16_t addr = (uintptr_t(&fStaticData[patch/4].fDAC[patch%4])-uintptr_t(&fStaticData))/2;

        // From CmdSetRegister
        const array<uint16_t, 2> data = {{ addr, uint16_t(value) }};
        PostCmd(data, FTM::kCmdWrite, FTM::kCmdRegister);

        reinterpret_cast<uint16_t*>(&fBufStaticData)[addr] = value;

        // Now execute change before the static data is requested back
        PostCmd(FTM::kCmdConfigFTU, (patch/40) | (((patch/4)%10)<<8));

        //CmdGetRegister(addr);
        CmdReqStatDat();

        return true;
    }

    bool SetSelectedThresholds(const int32_t *th)
    {
        for (int i=0; i<FTM::StaticData::kMaxPatchIdx; i++)
            if (th[i]>FTM::StaticData::kMaxDAC)
                return false;

        // FTM::StaticData data(fBufStaticData);

        for (int i=0; i<=FTM::StaticData::kMaxPatchIdx; i++)
        {
            if (th[i]<0 || fBufStaticData[i/4].fDAC[i%4]==th[i])
                continue;

            // Calculate offset in static data block
            const uint16_t addr = (uintptr_t(&fStaticData[i/4].fDAC[i%4])-uintptr_t(&fStaticData))/2;

            reinterpret_cast<uint16_t*>(&fBufStaticData)[addr] = th[i];

            // From CmdSetRegister
            const array<uint16_t, 2> arr = {{ addr, uint16_t(th[i]) }};
            PostCmd(arr, FTM::kCmdWrite, FTM::kCmdRegister);
            PostCmd(FTM::kCmdConfigFTU, (i/40) | (((i/4)%10)<<8));
        }

        //CmdGetRegister(addr);
        CmdReqStatDat();

        return true;
    }

    bool SetAllThresholds(const int32_t *th)
    {
        for (int i=0; i<FTM::StaticData::kMaxPatchIdx; i++)
            if (th[i]<0 || th[i]>FTM::StaticData::kMaxDAC)
                return false;

        FTM::StaticData data(fBufStaticData);

        for (int i=0; i<=FTM::StaticData::kMaxPatchIdx; i++)
            data[i/4].fDAC[i%4] = th[i];

        CmdSendStatDat(data);

        return true;
    }

    bool SetNoutof4(int32_t patch, int32_t value)
    {
        if (patch>=FTM::StaticData::kMaxMultiplicity)
            return false;

        if (value<0 || value>FTM::StaticData::kMaxDAC)
            return false;

        if (patch<0)
        {
            FTM::StaticData data(fBufStaticData);

            bool ident = true;
            for (int i=0; i<FTM::StaticData::kMaxMultiplicity; i++)
                if (data[i].fDAC[4] != value)
                {
                    ident = false;
                    break;
                }

            if (ident)
                return true;

            for (int i=0; i<=FTM::StaticData::kMaxMultiplicity; i++)
                data[i].fDAC[4] = value;

            // Maybe move to a "COMMIT" command?
            CmdSendStatDat(data);

            return true;
        }

        /*
         if (data[patch/4].fDAC[patch%4] == value)
            return true;

         data[patch/4].fDAC[patch%4] = value;

         CmdSendStatDat(data);
         return true;
         */

        // Calculate offset in static data block
        const uint16_t addr = (uintptr_t(&fStaticData[patch].fDAC[4])-uintptr_t(&fStaticData))/2;

        // From CmdSetRegister
        const array<uint16_t, 2> data = {{ addr, uint16_t(value) }};
        PostCmd(data, FTM::kCmdWrite, FTM::kCmdRegister);

        reinterpret_cast<uint16_t*>(&fBufStaticData)[addr] = value;

        // Now execute change before the static data is requested back
        PostCmd(FTM::kCmdConfigFTU, (patch/40) | (((patch/4)%10)<<8));

        //CmdGetRegister(addr);
        CmdReqStatDat();

        return true;
    }

    bool SetPrescaling(uint32_t value)
    {
        if (value>0xffff)
            return false;

        FTM::StaticData data(fBufStaticData);

        bool ident = true;
        for (int i=0; i<40; i++)
            if (data[i].fPrescaling != value)
            {
                ident = false;
                break;
            }

        if (ident)
            return true;

        data.SetPrescaling(value);

        // Maybe move to a "COMMIT" command?
        CmdSendStatDat(data);

        return true;
    }

    bool EnableFTU(int32_t board, bool enable)
    {
        if (board>39)
            return false;

        FTM::StaticData data(fBufStaticData);

        if (board<0)
        {
            if (enable)
                data.EnableAllFTU();
            else
                data.DisableAllFTU();
        }
        else
        {
            if (enable)
                data.EnableFTU(board);
            else
                data.DisableFTU(board);

        }

        // Maybe move to a "COMMIT" command?
        CmdSendStatDat(data);

        return true;
    }

    bool ToggleFTU(uint32_t board)
    {
        if (board>39)
            return false;

        FTM::StaticData data(fBufStaticData);

        data.ToggleFTU(board);

        // Maybe move to a "COMMIT" command?
        CmdSendStatDat(data);

        return true;
    }

    bool SetVal(uint16_t *dest, uint32_t val, uint32_t max)
    {
        if (val>max)
            return false;

        if (*dest==val)
            return true;

        FTM::StaticData data(fBufStaticData);

        dest = reinterpret_cast<uint16_t*>(&data) + (dest - reinterpret_cast<uint16_t*>(&fStaticData));

        *dest = val;

        CmdSendStatDat(data);

        return true;
    }

    bool SetTriggerInterval(uint32_t val)
    {
        return SetVal(&fStaticData.fTriggerInterval, val,
                      FTM::StaticData::kMaxTriggerInterval);
    }

    bool SetTriggerDelay(uint32_t val)
    {
        return SetVal(&fStaticData.fDelayTrigger, val,
                      FTM::StaticData::kMaxDelayTrigger);
    }

    bool SetTimeMarkerDelay(uint32_t val)
    {
        return SetVal(&fStaticData.fDelayTimeMarker, val,
                      FTM::StaticData::kMaxDelayTimeMarker);
    }

    bool SetDeadTime(uint32_t val)
    {
        return SetVal(&fStaticData.fDeadTime, val,
                      FTM::StaticData::kMaxDeadTime);
    }

    void Enable(FTM::StaticData::GeneralSettings type, bool enable)
    {
        //if (fStaticData.IsEnabled(type)==enable)
        //    return;

        FTM::StaticData data(fBufStaticData);
        data.Enable(type, enable);
        CmdSendStatDat(data);
    }

    bool SetTriggerSeq(const uint16_t d[3])
    {
	if (d[0]>FTM::StaticData::kMaxSequence ||
            d[1]>FTM::StaticData::kMaxSequence ||
            d[2]>FTM::StaticData::kMaxSequence)
            return false;

        FTM::StaticData data(fBufStaticData);

        /*
         data.Enable(FTM::StaticData::kPedestal, d[0]>0);
         data.Enable(FTM::StaticData::kLPext,    d[1]>0);
         data.Enable(FTM::StaticData::kLPint,    d[2]>0);
         */

        data.SetSequence(d[0], d[2], d[1]);

        //if (fStaticData.fTriggerSeq     !=data.fTriggerSequence ||
        //    fStaticData.fGeneralSettings!=data.fGeneralSettings)
        //    CmdSendStatDat(data);

        CmdSendStatDat(data);

        return true;
    }

    bool SetTriggerMultiplicity(uint16_t n)
    {
        if (n==0 || n>FTM::StaticData::kMaxMultiplicity)
            return false;

        //if (n==fBufStaticData.fMultiplicityPhysics)
        //    return true;

        FTM::StaticData data(fBufStaticData);

        data.fMultiplicityPhysics = n;

        CmdSendStatDat(data);

        return true;
    }

    bool SetTriggerWindow(uint16_t win)
    {
        if (win>FTM::StaticData::kMaxWindow)
            return false;

        //if (win==fStaticData.fWindowPhysics)
        //    return true;

        FTM::StaticData data(fBufStaticData);

        data.fWindowPhysics = win;

        CmdSendStatDat(data);

        return true;
    }

    bool SetCalibMultiplicity(uint16_t n)
    {
        if (n==0 || n>FTM::StaticData::kMaxMultiplicity)
            return false;

        //if (n==fStaticData.fMultiplicityCalib)
        //    return true;

        FTM::StaticData data(fBufStaticData);

        data.fMultiplicityCalib = n;

        CmdSendStatDat(data);

        return true;
    }

    bool SetCalibWindow(uint16_t win)
    {
        if (win>FTM::StaticData::kMaxWindow)
            return false;

        //if (win==fStaticData.fWindowCalib)
        //    return true;

        FTM::StaticData data(fBufStaticData);

        data.fWindowCalib = win;

        CmdSendStatDat(data);

        return true;
    }

    bool SetClockRegister(const uint64_t reg[])
    {
        FTM::StaticData data(fBufStaticData);

        for (int i=0; i<8; i++)
            if (reg[i]>0xffffffff)
                return false;

        data.SetClockRegister(reg);

        CmdSendStatDat(data);

        return true;
    }

    bool EnableLP(FTM::StaticData::GeneralSettings lp, FTM::StaticData::LightPulserEnable group, bool enable)
    {
        if (lp!=FTM::StaticData::kLPint && lp!=FTM::StaticData::kLPext)
            return false;

        FTM::StaticData data(fBufStaticData);

        if (lp==FTM::StaticData::kLPint)
            data.EnableLPint(group, enable);

        if (lp==FTM::StaticData::kLPext)
            data.EnableLPext(group, enable);

        CmdSendStatDat(data);

        return true;
    }

    bool SetIntensity(FTM::StaticData::GeneralSettings lp, uint16_t intensity)
    {
        if (intensity>FTM::StaticData::kMaxIntensity)
            return false;

        if (lp!=FTM::StaticData::kLPint && lp!=FTM::StaticData::kLPext)
            return false;

        FTM::StaticData data(fBufStaticData);

        if (lp==FTM::StaticData::kLPint)
            data.fIntensityLPint = intensity;

        if (lp==FTM::StaticData::kLPext)
            data.fIntensityLPext = intensity;

        CmdSendStatDat(data);

        return true;
    }

    bool EnablePixel(int16_t idx, bool enable)
    {
        if (idx<-1 || idx>FTM::StaticData::kMaxPixelIdx)
            return false;

        if (idx==-1)
        {
            FTM::StaticData data(fBufStaticData);

            for (int i=0; i<=FTM::StaticData::kMaxPixelIdx; i++)
                data.EnablePixel(i, enable);

            CmdSendStatDat(data);

            return true;
        }

        /*
         data.EnablePixel(idx, enable);
         CmdSendStatDat(data);
         return true;
         */

        FTM::StaticData data(fBufStaticData);

        const uintptr_t base = uintptr_t(&data);
        const uint16_t *mem  = data.EnablePixel(idx, enable);

        // Calculate offset in static data block
        const uint16_t addr = (uintptr_t(mem)-base)/2;

        // From CmdSetRegister
        const array<uint16_t, 2> cmd = {{ addr, *mem }};
        PostCmd(cmd, FTM::kCmdWrite, FTM::kCmdRegister);

        reinterpret_cast<uint16_t*>(&fBufStaticData)[addr] = *mem;

        // Now execute change before the static data is requested back
        PostCmd(FTM::kCmdConfigFTU, (idx/360) | (((idx/36)%10)<<8));

        // Now request the register back to ensure consistency
        //CmdGetRegister(addr);
        CmdReqStatDat();

        return true;
    }

    bool DisableAllPixelsExcept(uint16_t idx)
    {
        if (idx>FTM::StaticData::kMaxPixelIdx)
            return false;

        FTM::StaticData data(fBufStaticData);

        for (int i=0; i<=FTM::StaticData::kMaxPixelIdx; i++)
            data.EnablePixel(i, i==idx);

        CmdSendStatDat(data);

        return true;
    }

    bool DisableAllPatchesExcept(int16_t idx)
    {
        if (idx>FTM::StaticData::kMaxPatchIdx)
            return false;

        FTM::StaticData data(fBufStaticData);

        for (int i=0; i<=FTM::StaticData::kMaxPixelIdx; i++)
            data.EnablePixel(i, i/9==idx);

        CmdSendStatDat(data);

        return true;
    }

    bool EnablePatch(int16_t idx, bool enable)
    {
        if (idx>FTM::StaticData::kMaxPatchIdx)
            return false;

        FTM::StaticData data(fBufStaticData);

        for (int i=0; i<=FTM::StaticData::kMaxPixelIdx; i++)
            if (i/9==idx)
                data.EnablePixel(i, enable);

        CmdSendStatDat(data);

        return true;
    }

    bool TogglePixel(uint16_t idx)
    {
        if (idx>FTM::StaticData::kMaxPixelIdx)
            return false;

        FTM::StaticData data(fBufStaticData);

        data.EnablePixel(idx, !fBufStaticData.Enabled(idx));

        CmdSendStatDat(data);

        return true;
    }

    States GetState() const
    {
        if (!IsConnected())
            return kDisconnected; // rc=1

        switch (fHeader.fState&FTM::kFtmStates)
        {
        case FTM::kFtmUndefined:  // 0
            return fBufStaticData.valid() ? kConnected :  kDisconnected;    // rc=2

        case FTM::kFtmRunning:    // 3
        case FTM::kFtmCalib:      // 4
            return kTriggerOn;    // rc=4

        case FTM::kFtmIdle:      // 1
        case FTM::kFtmConfig:    // 2          //  rc=7          // rc=3
            return fStaticData == fBufStaticData ? kConfigured : kIdle;
        }

        throw runtime_error("ConnectionFTM::GetState - Impossible code reached.");
    }

    // If fState==2, the clock conditioner will always be reported as unlocked
    //bool IsLocked() const { return fHeader.fState&FTM::kFtmLocked; }

    uint32_t GetCounter(FTM::Types type) { return fCounter[type]; }

    const FTM::StaticData &GetStaticData() const { return fStaticData; }
};

//const uint16_t ConnectionFTM::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimFTM : public ConnectionFTM
{
private:

    DimDescribedService fDimPassport;
    DimDescribedService fDimTriggerRates;
    DimDescribedService fDimError;
    DimDescribedService fDimFtuList;
    DimDescribedService fDimStaticData;
    DimDescribedService fDimDynamicData;
    DimDescribedService fDimCounter;

    uint64_t fTimeStamp;
    uint64_t fTimeStampOn;
    uint32_t fTriggerCounter;
    uint64_t fPrevState;

    void UpdateFirstHeader()
    {
        ConnectionFTM::UpdateFirstHeader();

        const FTM::DimPassport data(fHeader);
        fDimPassport.Update(data);
    }

    /*
    void UpdateHeader()
    {
        ConnectionFTM::UpdateHeader();

        if (fHeader.fType!=FTM::kDynamicData)
            return;

        const FTM::DimTriggerCounter data(fHeader);
        fDimTriggerCounter.Update(data);
    }*/

    void UpdateFtuList()
    {
        ConnectionFTM::UpdateFtuList();

        const FTM::DimFtuList data(fHeader, fFtuList);
        fDimFtuList.Update(data);
    }

    void UpdateStaticData()
    {
        ConnectionFTM::UpdateStaticData();

        const FTM::DimStaticData data(fHeader, fStaticData);
        fDimStaticData.setQuality(fHeader.fState);
        fDimStaticData.Update(data);
    }

    void UpdateDynamicData()
    {
        ConnectionFTM::UpdateDynamicData();

        const FTM::DimDynamicData data(fHeader, fDynamicData, fStaticData);
        fDimDynamicData.setQuality(fHeader.fState);
        fDimDynamicData.Update(data);

        uint64_t odiff = fDynamicData.fOnTimeCounter;
        uint32_t cdiff = fHeader.fTriggerCounter;
        uint64_t tdiff = fHeader.fTimeStamp;

        // The easiest way to detect whether the counters have been
        // reset or not is to detect a state change, because with
        // every state change they are reset. However, there are cases
        // when the trigger is switched on already (data run) and
        // the trigger is turned off ans switched on again within
        // a very short time, that the state of the previous and the
        // new report is the same. So in addition we have to check
        // for other indications. Any counter decreasing is a hint.
        // None of them should ever decrease. So all three are checked.
        const uint8_t state = fHeader.fState & FTM::States::kFtmStates;

        const bool first = state!=fPrevState ||
            fHeader.fTimeStamp<fTimeStamp ||
            fHeader.fTriggerCounter<fTriggerCounter ||
            fDynamicData.fOnTimeCounter<fTimeStampOn;

        if (!first)
        {
            tdiff -= fTimeStamp;
            odiff -= fTimeStampOn;
            cdiff -= fTriggerCounter;
        }

        // The observation time calculated in the first report is most likely
        // too large because the previous report is taken as reference,
        // but this is the best what could be done.
        const float rate = tdiff==0 ? 0 : 1e6*cdiff/tdiff;

        fTimeStamp      = fHeader.fTimeStamp;
        fTimeStampOn    = fDynamicData.fOnTimeCounter;
        fTriggerCounter = fHeader.fTriggerCounter;
        fPrevState      = state;

        const FTM::DimTriggerRates rates(fHeader, fDynamicData, fStaticData,
                                         rate, tdiff*1e-6, odiff*1e-6);

        fDimTriggerRates.setQuality(fHeader.fState);
        fDimTriggerRates.Update(rates);
    }

    void UpdateError()
    {
        ConnectionFTM::UpdateError();

        const FTM::DimError data(fHeader, fError);
        fDimError.Update(data);
    }

    void UpdateCounter()
    {
        ConnectionFTM::UpdateCounter();

        const uint32_t counter[6] =
        {
            fCounter[FTM::kHeader],
            fCounter[FTM::kStaticData],
            fCounter[FTM::kDynamicData],
            fCounter[FTM::kFtuList],
            fCounter[FTM::kErrorList],
            fCounter[FTM::kRegister],
        };

        fDimCounter.setQuality(fHeader.fState);
        fDimCounter.Update(counter);
    }

public:
    ConnectionDimFTM(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionFTM(ioservice, imp),
        fDimPassport      ("FTM_CONTROL/PASSPORT",        "X:1;S:1",
                                                          "Info about the FTM and FPGA version"
                                                          "|BoardId[int]:BoardId, hexCode"
                                                          "|DNA[int]:DNA of the FTM board"),
        fDimTriggerRates  ("FTM_CONTROL/TRIGGER_RATES",   "X:1;X:1;I:1;F:1;F:40;F:160;F:1;F:1",
                                                          "Patch,Board,Camera trigger rates"
                                                          "|FTMtimeStamp[us]:Time in microseconds, since trigger enabled or disabled"
                                                          "|OnTimeCounter[us]:Effective on-time, ie. FTM triggers (eg. w/o busy)"
                                                          "|TriggerCounter[int]:Counter of triggers since enabled or disabled"
                                                          "|TriggerRate[Hz]:Trigger rate"
                                                          "|BoardRate[Hz]:Trigger rate of individual FTUs"
                                                          "|PatchRate[Hz]:Trigger rate of individual patches"
                                                          "|ElapsedTime[s]:Time elapsed since previous report"
                                                          "|OnTime[s]:OnTime elapsed since previous report"),
        fDimError         ("FTM_CONTROL/ERROR",           "X:1;S:1;S:28", ""),
        fDimFtuList       ("FTM_CONTROL/FTU_LIST",        "X:1;X:1;S:1;C:4;X:40;C:40;C:40",
                                                          "Logs the changes of status of the FTUs"
                                                          "|FTMtimeStamp[us]:Time in microseconds"
                                                          "|ActiveFTU[bitpattern]:Description of enabled FTUs"
                                                          "|NumBoards[int]:Total number of enabled FTUs"
                                                          "|NumBoardsCrate[int]:Total number of enabled FTUs per crate"
                                                          "|DNA[hexCode]:Hex code identifier of FTUs"
                                                          "|Addr[bitpattern]:Crate address (hardware) of FTUs"
                                                          "|Ping[int]:Number of pings until FTU response"),
        fDimStaticData    ("FTM_CONTROL/STATIC_DATA",     "X:1;S:1;S:1;X:1;S:1;S:3;C:4;S:1;S:1;S:1;S:1;S:1;S:1;I:1;I:8;S:90;S:160;S:40;S:40",
                                                          "Configuration of FTM and FTUs"
                                                          "|FTMtimeStamp[us]:Time in microseconds, since trigger enabled or disabled"
                                                          "|GeneralSettings[bitpattern]:Status of the FTM settings (cf. FTM doc)"
                                                          "|LEDStatus[bitpattern]:Not Used"
                                                          "|ActiveFTU[bitpattern]:List of enabled FTUs"
                                                          "|TriggerInterval[bitpattern]:Period of cal. and ped. events (cf. FTM doc)"
                                                          "|TriggerSeq[int]:Sequence of calib. and pedestal events (LPint, LPext, Ped)"
                                                          "|LPSettings[bitpattern]:Settings of LP, enabled int, ext, intensity int, ext"
                                                          "|PhysTrigMult[int]:N for N out of 40 logic on FTM (Physics)"
                                                          "|CalibTrigMult[int]: N for N out of 40 logic on FTM (Calib)"
                                                          "|PhysTrigWindow[ns]:Coincidence window for N out of 40 (Physics)"
                                                          "|CalibTrigWindow[ns]:Coincidence window for N out of 40 (Calib)"
                                                          "|TrigDelay[ns]:Trigger delay applied on FTM"
                                                          "|TMDelay[ns]:TM delay applied on FTM"
                                                          "|DeadTime[ns]:Dead time applied after each event on the FTM"
                                                          "|ClkCond[bitpattern]:Clock conditionner settings on the FTM (DRS sampling freq.)"
                                                          "|PixEnabled[bitpattern]:Enabled pixels, pckd in 90 shorts (160*9bits=180bytes)"
                                                          "|PatchThresh[DACcounts]:Threshold of the trigger patches"
                                                          "|Multiplicity[DACcounts]:N out of 4 logic settings per FTU"
                                                          "|Prescaling[500ms]:Update rate of the rate counter"),
        fDimDynamicData   ("FTM_CONTROL/DYNAMIC_DATA",    "X:1;X:1;F:4;I:160;I:40;S:40;S:40;S:40;S:1",
                                                          "Regular reports sent by FTM"
                                                          "|FTMtimeStamp[us]:Time in microseconds, since trigger enabled or disabled"
                                                          "|OnTimeCounter[us]:Ontime, i.e. FTM processes triggers (e.g. No FAD busy)"
                                                          "|Temperatures[Nan]:not yet defined nor used (wanna be FTM onboard temps)"
                                                          "|TriggerPatchCounter[int]:counting since last update (prescaling)"
                                                          "|BoardsCounter[int]:FTU board counting after N out of 4 and since last update"
                                                          "|RateOverflow[bitpattern]:bits 0-4=patches overflow, 5=board overflow, 1 per board"
                                                          "|Prescaling[500ms]:Update rate of the rate counter"
                                                          "|CrcError[int]:Number of checksum error in RS485 communication"
                                                          "|State[int]:State value of the FTM firmware (cf. FTM doc)"),
        fDimCounter       ("FTM_CONTROL/COUNTER",         "I:1;I:1;I:1;I:1;I:1;I:1",
                                                          "Communication statistics to or from FTM control and FTM"
                                                          "|NumHeaders[int]:Num. of headers (any header) received by ftm control"
                                                          "|NumStaticData[int]:Num. of static data blocks (ftm and ftu settings)"
                                                          "|NumDynamicData[int]:Num. of dynamic data blocks (e.g. rates)"
                                                          "|NumFtuList[int]:Num. of FTU list (FTU identifiers, answer from ping)"
                                                          "|NumErrors[int]:Num. of error messages"
                                                          "|NumRegister[int]:Num. of answers from a single register accesess"),
        fTimeStamp(0), fTimeStampOn(0), fTriggerCounter(0), fPrevState(0)
    {
    }

    // A B [C] [D] E [F] G H [I] J K [L] M N O P Q R [S] T U V W [X] Y Z
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineFTM : public StateMachineAsio<T>
{
    int Wrap(function<void()> f)
    {
        f();
        return T::GetCurrentState();
    }

    function<int(const EventImp &)> Wrapper(function<void()> func)
    {
        return bind(&StateMachineFTM::Wrap, this, func);
    }

private:
    S fFTM;

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int SetRegister(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetRegister", 8))
            return T::kSM_FatalError;

        const uint32_t *dat = evt.Ptr<uint32_t>();

        if (dat[1]>uint16_t(-1))
        {
            ostringstream msg;
            msg << hex << "Value " << dat[1] << " out of range.";
            T::Error(msg);
            return T::GetCurrentState();
        }


        if (dat[0]>uint16_t(-1) || !fFTM.CmdSetRegister(dat[0], dat[1]))
        {
            ostringstream msg;
            msg << hex << "Address " << dat[0] << " out of range.";
            T::Error(msg);
        }

        return T::GetCurrentState();
    }

    int GetRegister(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "GetRegister", 4))
            return T::kSM_FatalError;

        const unsigned int addr = evt.GetInt();
        if (addr>uint16_t(-1) || !fFTM.CmdGetRegister(addr))
        {
            ostringstream msg;
            msg << hex << "Address " << addr << " out of range.";
            T::Error(msg);
        }

        return T::GetCurrentState();
    }

    int StartRun()
    {
        // This is a workaround... it seems that the FTM ignored the 'trigger on'
        // as long as it is still sending thresholds to the FTUs (and it seems
        // that this is the only command/confguration) which gets ignored.
        // So if we are configuring, we resent this command until we got a
        // reasonable answer (TriggerOn) back from the FTM.
        // There is no need to send the command here, because Execute
        // will be called immediately after this anyway before any
        // answer could be processed. So it would just guarantee that
        // the command is sent twice for no reason.

        fFTM.CmdStartRun();

        if (T::GetCurrentState()!=FTM::State::kConfigured1)
            return T::GetCurrentState();

        fCounterReg = fFTM.GetCounter(FTM::kRegister);
        return FTM::State::kConfigured2;
    }

    int TakeNevents(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "TakeNevents", 4))
            return T::kSM_FatalError;

        const unsigned int dat = evt.GetUInt();

        /*
        if (dat[1]>uint32_t(-1))
        {
            ostringstream msg;
            msg << hex << "Value " << dat[1] << " out of range.";
            T::Error(msg);
            return T::GetCurrentState();
        }*/

        fFTM.CmdTakeNevents(dat);

        return T::GetCurrentState();
    }

    int DisableReports(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "DisableReports", 1))
            return T::kSM_FatalError;

        fFTM.CmdDisableReports(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fFTM.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetHexOutput(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetHexOutput", 1))
            return T::kSM_FatalError;

        fFTM.SetHexOutput(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDynamicOut(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDynamicOut", 1))
            return T::kSM_FatalError;

        fFTM.SetDynamicOut(evt.GetBool());

        return T::GetCurrentState();
    }

    int LoadStaticData(const EventImp &evt)
    {
        if (fFTM.LoadStaticData(evt.GetString()))
            return T::GetCurrentState();

        ostringstream msg;
        msg << "Loading static data from file '" << evt.GetString() << "' failed ";

        if (errno)
            msg << "(" << strerror(errno) << ")";
        else
            msg << "(wrong size, expected " << sizeof(FTM::StaticData) << " bytes)";

        T::Warn(msg);

        return T::GetCurrentState();
    }

    int SaveStaticData(const EventImp &evt)
    {
        if (fFTM.SaveStaticData(evt.GetString()))
            return T::GetCurrentState();

        ostringstream msg;
        msg << "Writing static data to file '" << evt.GetString() << "' failed ";
        msg << "(" << strerror(errno) << ")";

        T::Warn(msg);

        return T::GetCurrentState();
    }

    int SetThreshold(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetThreshold", 8))
            return T::kSM_FatalError;

        const int32_t *data = evt.Ptr<int32_t>();

        if (!fFTM.SetThreshold(data[0], data[1]))
        {
            ostringstream msg;
            msg << "SetThreshold - Maximum allowed patch number 159, valid value range 0-0xffff (got: " << data[0] << " " << data[1] << ")";
            T::Warn(msg);
        }

        return T::GetCurrentState();
    }

    int SetSelectedThresholds(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetSelectedThresholds", 160*4))
            return T::kSM_FatalError;

        const int32_t *data = evt.Ptr<int32_t>();
        if (!fFTM.SetSelectedThresholds(data))
        {
            ostringstream msg;
            msg << "SetSelectedThresholds - Value out of range, maximum 0xffff.";
            T::Warn(msg);
        }

        return T::GetCurrentState();
    }

    int SetAllThresholds(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetAllThresholds", 160*4))
            return T::kSM_FatalError;

        const int32_t *data = evt.Ptr<int32_t>();
        if (!fFTM.SetAllThresholds(data))
        {
            ostringstream msg;
            msg << "SetAllThresholds - Value out of range [0; 0xffff]";
            T::Warn(msg);
        }

        return T::GetCurrentState();
    }

    int SetNoutof4(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetNoutof4", 8))
            return T::kSM_FatalError;

        const int32_t *data = evt.Ptr<int32_t>();

        if (!fFTM.SetNoutof4(data[0], data[1]))
            T::Warn("SetNoutof4 - Maximum allowed board number 39, valid value range 0-0xffff");

        return T::GetCurrentState();
    }

    int EnableFTU(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "EnableFTU", 5))
            return T::kSM_FatalError;

        const int32_t &board  = evt.Get<int32_t>();
        const int8_t  &enable = evt.Get<int8_t>(4);

        if (!fFTM.EnableFTU(board, enable))
            T::Warn("EnableFTU - Board number must be <40.");

        return T::GetCurrentState();
    }

    int ToggleFTU(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ToggleFTU", 4))
            return T::kSM_FatalError;

        if (!fFTM.ToggleFTU(evt.GetInt()))
            T::Warn("ToggleFTU - Allowed range of boards 0-39.");

        return T::GetCurrentState();
    }

    int SetTriggerInterval(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerInterval", 4))
            return T::kSM_FatalError;

        if (!fFTM.SetTriggerInterval(evt.GetInt()))
            T::Warn("SetTriggerInterval - Value out of range.");

        return T::GetCurrentState();
    }

    int SetTriggerDelay(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerDelay", 4))
            return T::kSM_FatalError;

        if (!fFTM.SetTriggerDelay(evt.GetInt()))
            T::Warn("SetTriggerDealy - Value out of range.");

        return T::GetCurrentState();
    }

    int SetTimeMarkerDelay(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTimeMarkerDelay", 4))
            return T::kSM_FatalError;

        if (!fFTM.SetTimeMarkerDelay(evt.GetInt()))
            T::Warn("SetTimeMarkerDelay - Value out of range.");

        return T::GetCurrentState();
    }

    int SetPrescaling(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetPrescaling", 4))
            return T::kSM_FatalError;

        if (!fFTM.SetPrescaling(evt.GetInt()-1))
            T::Warn("SetPrescaling - Value out of range.");

        return T::GetCurrentState();
    }

    int SetTriggerSeq(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerSeq", 6))
            return T::kSM_FatalError;

        const uint16_t *data = evt.Ptr<uint16_t>();

        if (!fFTM.SetTriggerSeq(data))
            T::Warn("SetTriggerSeq - Value out of range.");

        return T::GetCurrentState();
    }

    int SetDeadTime(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDeadTime", 4))
            return T::kSM_FatalError;

        if (!fFTM.SetDeadTime(evt.GetInt()))
            T::Warn("SetDeadTime - Value out of range.");

        return T::GetCurrentState();
    }

    int SetTriggerMultiplicity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerMultiplicity", 2))
            return T::kSM_FatalError;

        if (!fFTM.SetTriggerMultiplicity(evt.GetUShort()))
            T::Warn("SetTriggerMultiplicity -  Value out of range.");

        return T::GetCurrentState();
    }

    int SetCalibMultiplicity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetCalibMultiplicity", 2))
            return T::kSM_FatalError;

        if (!fFTM.SetCalibMultiplicity(evt.GetUShort()))
            T::Warn("SetCalibMultiplicity -  Value out of range.");

        return T::GetCurrentState();
    }

    int SetTriggerWindow(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetTriggerWindow", 2))
            return T::kSM_FatalError;

        if (!fFTM.SetTriggerWindow(evt.GetUShort()))
            T::Warn("SetTriggerWindow -  Value out of range.");

        return T::GetCurrentState();
    }

    int SetCalibWindow(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetCalibWindow", 2))
            return T::kSM_FatalError;

        if (!fFTM.SetCalibWindow(evt.GetUShort()))
            T::Warn("SetCalibWindow -  Value out of range.");

        return T::GetCurrentState();
    }

    int SetClockRegister(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetClockRegister", 8*8))
            return T::kSM_FatalError;

        const uint64_t *reg = evt.Ptr<uint64_t>();

        if (!fFTM.SetClockRegister(reg))
            T::Warn("SetClockRegister - Value out of range.");

        return T::GetCurrentState();
    }

    int SetClockFrequency(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetClockFrequency", 2))
            return T::kSM_FatalError;

        const map<uint16_t,array<uint64_t, 8>>::const_iterator it =
            fClockCondSetup.find(evt.GetUShort());

        if (it==fClockCondSetup.end())
        {
            T::Warn("SetClockFrequency - Frequency not supported.");
            return T::GetCurrentState();
        }

        if (!fFTM.SetClockRegister(it->second.data()))
            T::Warn("SetClockFrequency - Register values out of range.");

        return T::GetCurrentState();
    }

    int EnableLP(const EventImp &evt, FTM::StaticData::GeneralSettings lp, FTM::StaticData::LightPulserEnable group)
    {
        if (!CheckEventSize(evt.GetSize(), "EnableLP", 1))
            return T::kSM_FatalError;

        if (!fFTM.EnableLP(lp, group, evt.GetBool()))
            T::Warn("EnableLP - Invalid light pulser id.");

        return T::GetCurrentState();
    }

    int SetIntensity(const EventImp &evt, FTM::StaticData::GeneralSettings lp)
    {
        if (!CheckEventSize(evt.GetSize(), "SetIntensity", 2))
            return T::kSM_FatalError;

        if (!fFTM.SetIntensity(lp, evt.GetShort()))
            T::Warn("SetIntensity - Value out of range.");

        return T::GetCurrentState();
    }

    int Enable(const EventImp &evt, FTM::StaticData::GeneralSettings type)
    {
        if (!CheckEventSize(evt.GetSize(), "Enable", 1))
            return T::kSM_FatalError;

        fFTM.Enable(type, evt.GetBool());

        return T::GetCurrentState();
    }

    int EnablePixel(const EventImp &evt, bool b)
    {
        if (!CheckEventSize(evt.GetSize(), "EnablePixel", 2))
            return T::kSM_FatalError;

        if (!fFTM.EnablePixel(evt.GetUShort(), b))
            T::Warn("EnablePixel -  Value out of range.");

        return T::GetCurrentState();
    }

    int DisableAllPixelsExcept(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "DisableAllPixelsExcept", 2))
            return T::kSM_FatalError;

        if (!fFTM.DisableAllPixelsExcept(evt.GetUShort()))
            T::Warn("DisableAllPixelsExcept -  Value out of range.");

        return T::GetCurrentState();
    }

    int DisableAllPatchesExcept(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "DisableAllPatchesExcept", 2))
            return T::kSM_FatalError;

        if (!fFTM.DisableAllPatchesExcept(evt.GetUShort()))
            T::Warn("DisableAllPatchesExcept -  Value out of range.");

        return T::GetCurrentState();
    }

    int EnablePatch(const EventImp &evt, bool enable)
    {
        if (!CheckEventSize(evt.GetSize(), "EnablePatch", 2))
            return T::kSM_FatalError;

        if (!fFTM.EnablePatch(evt.GetUShort(), enable))
            T::Warn("EnablePatch -  Value out of range.");

        return T::GetCurrentState();
    }

    int TogglePixel(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "TogglePixel", 2))
            return T::kSM_FatalError;

        if (!fFTM.TogglePixel(evt.GetUShort()))
            T::Warn("TogglePixel -  Value out of range.");

        return T::GetCurrentState();
    }

    int ResetCrate(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ResetCrate", 2))
            return T::kSM_FatalError;

        fFTM.CmdResetCrate(evt.GetUShort());

        return T::GetCurrentState();
    }

    int Disconnect()
    {
        // Close all connections
        fFTM.PostClose(false);

        /*
         // Now wait until all connection have been closed and
         // all pending handlers have been processed
         poll();
         */

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fFTM.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fFTM.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fFTM.PostClose(true);

        return T::GetCurrentState();
    }

    /*
    int Transition(const Event &evt)
    {
        switch (evt.GetTargetState())
        {
        case kDisconnected:
        case kConnected:
        }

        return T::kSM_FatalError;
    }*/

    uint32_t fCounterReg;
    uint32_t fCounterStat;

    typedef map<string, FTM::StaticData> Configs;
    Configs fConfigs;
    Configs::const_iterator fTargetConfig;

    int ConfigureFTM(const EventImp &evt)
    {
        const string name = evt.GetText();

        fTargetConfig = fConfigs.find(name);
        if (fTargetConfig==fConfigs.end())
        {
            T::Error("ConfigureFTM - Run-type '"+name+"' not found.");
            return T::GetCurrentState();
        }

        T::Message("Starting configuration for '"+name+"' ["+to_string(fFTM.IsTxQueueEmpty())+"]");

        fCounterReg = fFTM.GetCounter(FTM::kRegister);
        fFTM.CmdStopRun();

        return FTM::State::kConfiguring1;
    }

    int ResetConfig()
    {
        return fFTM.GetState();
    }

    int Execute()
    {
        // If FTM is neither in data taking nor idle,
        // leave configuration state
        switch (fFTM.GetState())
        {
        case ConnectionFTM::kDisconnected: return FTM::State::kDisconnected;
        case ConnectionFTM::kConnected:    return FTM::State::kConnected;
        default:
            break;
        }

        // FIXME: Add timeouts and go to error state
        //        so that a configuration error can be handled
        switch (T::GetCurrentState())
        {
        case FTM::State::kConfiguring1:
            // If FTM has received an anwer to the stop_run command
            // the counter for the registers has been increased
            if (fFTM.GetCounter(FTM::kRegister)<=fCounterReg)
                return FTM::State::kConfiguring1;

            // If now the state is not idle as expected this means we had
            // an error (maybe old events waiting in the queue)
            if (fFTM.GetState()!=ConnectionFTM::kIdle &&
                fFTM.GetState()!=ConnectionFTM::kConfigured)
                return FTM::State::kConfigError1;

            fCounterStat = fFTM.GetCounter(FTM::kStaticData);

            fFTM.CmdSendStatDat(fTargetConfig->second);

            T::Message("Trigger successfully disabled... sending new configuration.");

            // Next state is: wait for the answer to our configuration
            return FTM::State::kConfiguring2;

        case FTM::State::kConfiguring2:
        case FTM::State::kConfigured1:
            // If FTM has received an anwer to the CmdSendStatDat
            // the counter for static data has been increased
            if (fFTM.GetCounter(FTM::kStaticData)<=fCounterStat)
                break;

            // If now the configuration is not what we expected
            // we had an error (maybe old events waiting in the queue?)
            if (fFTM.GetState()!=ConnectionFTM::kConfigured)
                return FTM::State::kConfigError2;

            // Check configuration again when a new static data block
            // will be received
            fCounterStat = fFTM.GetCounter(FTM::kStaticData);

            // This is also displayed when the ratecontrol sends its configuration...
            if (T::GetCurrentState()==FTM::State::kConfiguring2)
                T::Message("Sending new configuration was successfull.");
            else
                T::Message("Configuration successfully updated.");

            // Next state is: wait for the answer to our configuration
            return FTM::State::kConfigured1;

        // This state is set by StartRun [START_TRIGGER]
        case FTM::State::kConfigured2:
            // No answer to the CmdStartRun received yet... go on waiting
            if (fFTM.GetCounter(FTM::kRegister)<=fCounterReg)
                return FTM::State::kConfigured2;

            // Answer received and trigger enable acknowledged
            if (fFTM.GetState()==ConnectionFTM::kTriggerOn)
                return FTM::State::kTriggerOn;

            // If the trigger is not enabled, but the configuration
            // has changed go to error state (should never happen)
            if (fFTM.GetState()!=ConnectionFTM::kConfigured)
                return FTM::State::kConfigError2;

            // Send a new command... the previous one might have gone
            // ignored by the ftm because it was just after a
            // threshold setting during the configured state
            fFTM.CmdStartRun(false);

            // Set counter to wait for answer.
            fCounterReg = fFTM.GetCounter(FTM::kRegister);

            // Go on waiting for a proper acknowledge of the trigger enable
            return FTM::State::kConfigured2;

        case FTM::State::kConfigError1:
        case FTM::State::kConfigError2:
        //case FTM::State::kConfigError3:
            break;

        default:
            switch (fFTM.GetState())
            {
            case ConnectionFTM::kIdle:         return FTM::State::kIdle;
            case ConnectionFTM::kConfigured:   return FTM::State::kValid;
            case ConnectionFTM::kTriggerOn:    return FTM::State::kTriggerOn;
            default:
                throw runtime_error("StateMachineFTM - Execute() - Inavlid state.");
            }
        }

        return T::GetCurrentState();
    }

public:
    StateMachineFTM(ostream &out=cout) :
        StateMachineAsio<T>(out, "FTM_CONTROL"), fFTM(*this, *this)
    {
        // State names
        T::AddStateName(FTM::State::kDisconnected, "Disconnected",
                        "FTM board not connected via ethernet.");

        T::AddStateName(FTM::State::kConnected, "Connected",
                        "Ethernet connection to FTM established (no state received yet).");

        T::AddStateName(FTM::State::kIdle, "Idle",
                        "Ethernet connection to FTM established, FTM in idle state.");

        T::AddStateName(FTM::State::kValid, "Valid",
                        "FTM in idle state and the last sent and received static data block are bitwise identical.");

        T::AddStateName(FTM::State::kConfiguring1, "Configuring1",
                        "Command to disable run sent... waiting for response.");
        T::AddStateName(FTM::State::kConfiguring2, "Configuring2",
                        "New configuration sent... waiting for response.");
        T::AddStateName(FTM::State::kConfigured1,   "Configured1",
                        "Received answer identical with target configuration.");
        T::AddStateName(FTM::State::kConfigured2, "Configured2",
                        "Waiting for acknowledge of trigger enable.");

        T::AddStateName(FTM::State::kTriggerOn, "TriggerOn",
                        "Ethernet connection to FTM established, FTM trigger output to FADs enabled.");

        T::AddStateName(FTM::State::kConfigError1, "ErrorInConfig1", "Unexpected state received from FTM");
        T::AddStateName(FTM::State::kConfigError2, "ErrorInConfig2", "Unexpected state received from FTM");
        //T::AddStateName(FTM::State::kConfigError3, "ClockCondError", "Clock conditioner not locked");

        // FTM Commands
        T::AddEvent("TOGGLE_LED", FTM::State::kIdle, FTM::State::kValid)
            (Wrapper(bind(&ConnectionFTM::CmdToggleLed, &fFTM)))
            ("toggle led");

        T::AddEvent("PING", FTM::State::kIdle, FTM::State::kValid)
            (Wrapper(bind(&ConnectionFTM::CmdPing, &fFTM)))
            ("send ping");

        T::AddEvent("REQUEST_DYNAMIC_DATA", FTM::State::kIdle, FTM::State::kValid)
            (Wrapper(bind(&ConnectionFTM::CmdReqDynDat, &fFTM)))
            ("request transmission of dynamic data block");

        T::AddEvent("REQUEST_STATIC_DATA", FTM::State::kIdle, FTM::State::kValid)
            (Wrapper(bind(&ConnectionFTM::CmdReqStatDat, &fFTM)))
            ("request transmission of static data from FTM to memory");

        T::AddEvent("GET_REGISTER", "I", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::GetRegister, this, placeholders::_1))
            ("read register from address addr"
            "|addr[short]:Address of register");

        T::AddEvent("SET_REGISTER", "I:2", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetRegister, this, placeholders::_1))
            ("set register to value"
            "|addr[short]:Address of register"
            "|val[short]:Value to be set");

        T::AddEvent("START_TRIGGER", FTM::State::kIdle, FTM::State::kValid, FTM::State::kConfigured1, FTM::State::kConfigured2)
            (bind(&StateMachineFTM::StartRun, this))
            ("start a run (start distributing triggers)");

        T::AddEvent("STOP_TRIGGER", FTM::State::kTriggerOn)
            (Wrapper(bind(&ConnectionFTM::CmdStopRun, &fFTM)))
            ("stop a run (stop distributing triggers)");

        T::AddEvent("TAKE_N_EVENTS", "I", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::TakeNevents, this, placeholders::_1))
            ("take n events (distribute n triggers)|number[int]:Number of events to be taken");

        T::AddEvent("DISABLE_REPORTS", "B", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::DisableReports, this, placeholders::_1))
            ("disable sending rate reports"
             "|status[bool]:disable or enable that the FTM sends rate reports (yes/no)");

        T::AddEvent("SET_THRESHOLD", "I:2", FTM::State::kIdle, FTM::State::kValid, FTM::State::kConfigured1, FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::SetThreshold, this, placeholders::_1))
            ("Set the comparator threshold"
             "|Patch[idx]:Index of the patch (0-159), -1 for all"
             "|Threshold[counts]:Threshold to be set in binary counts");

        T::AddEvent("SET_SELECTED_THRESHOLDS", "I:160", FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::SetSelectedThresholds, this, placeholders::_1))
            ("Set the comparator thresholds. Only thresholds which are different and >=0 are sent."
             "|Thresholds[counts]:Threshold to be set in binary counts");

        T::AddEvent("SET_ALL_THRESHOLDS", "I:160", FTM::State::kIdle, FTM::State::kValid, FTM::State::kConfigured1)
            (bind(&StateMachineFTM::SetAllThresholds, this, placeholders::_1))
            ("Set the comparator thresholds"
             "|Thresholds[counts]:Threshold to be set in binary counts");

        T::AddEvent("SET_N_OUT_OF_4", "I:2", FTM::State::kIdle, FTM::State::kValid, FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::SetNoutof4, this, placeholders::_1))
            ("Set the comparator threshold"
             "|Board[idx]:Index of the board (0-39), -1 for all"
             "|Threshold[counts]:Threshold to be set in binary counts");

        T::AddEvent("SET_PRESCALING", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetPrescaling, this, placeholders::_1))
            ("Sets the FTU readout time intervals"
             "|time[0.5s]:The interval is given in units of 0.5s, i.e. 1 means 0.5s, 2 means 1s, ...");

        T::AddEvent("ENABLE_FTU", "I:1;B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnableFTU, this, placeholders::_1))
            ("Enable or disable FTU"
             "|Board[idx]:Index of the board (0-39), -1 for all"
             "|Enable[bool]:Whether FTU should be enabled or disabled (yes/no)");

        T::AddEvent("DISABLE_PIXEL", "S:1", FTM::State::kIdle, FTM::State::kValid, FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::EnablePixel, this, placeholders::_1, false))
            ("(-1 or all)");

        T::AddEvent("ENABLE_PIXEL", "S:1", FTM::State::kIdle, FTM::State::kValid, FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::EnablePixel, this, placeholders::_1, true))
            ("(-1 or all)");

        T::AddEvent("DISABLE_ALL_PIXELS_EXCEPT", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::DisableAllPixelsExcept, this, placeholders::_1))
            ("");

        T::AddEvent("DISABLE_ALL_PATCHES_EXCEPT", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::DisableAllPatchesExcept, this, placeholders::_1))
            ("");

        T::AddEvent("ENABLE_PATCH", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnablePatch, this, placeholders::_1, true))
            ("");

        T::AddEvent("DISABLE_PATCH", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnablePatch, this, placeholders::_1, false))
            ("");

        T::AddEvent("TOGGLE_PIXEL", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::TogglePixel, this, placeholders::_1))
            ("");

        T::AddEvent("TOGGLE_FTU", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::ToggleFTU, this, placeholders::_1))
            ("Toggle status of FTU (this is mainly meant to be used in the GUI)"
             "|Board[idx]:Index of the board (0-39)");

        T::AddEvent("SET_TRIGGER_INTERVAL", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTriggerInterval, this, placeholders::_1))
            ("Sets the trigger interval which is the distance between two consecutive artificial triggers."
             "|interval[ms]:The applied trigger interval in millisecond (min 1ms / 10bit)");

        T::AddEvent("SET_TRIGGER_DELAY", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTriggerDelay, this, placeholders::_1))
            (""
             "|delay[int]:The applied trigger delay is: delay*4ns+8ns");

        T::AddEvent("SET_TIME_MARKER_DELAY", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTimeMarkerDelay, this, placeholders::_1))
            (""
            "|delay[int]:The applied time marker delay is: delay*4ns+8ns");

        T::AddEvent("SET_DEAD_TIME", "I:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetDeadTime, this, placeholders::_1))
            (""
            "|dead_time[int]:The applied dead time is: dead_time*4ns+8ns");

        T::AddEvent("ENABLE_TRIGGER", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Enable, this, placeholders::_1, FTM::StaticData::kTrigger))
            ("Switch on the physics trigger"
             "|Enable[bool]:Enable physics trigger (yes/no)");

        // FIXME: Switch on/off depending on sequence
        T::AddEvent("ENABLE_EXT1", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Enable, this, placeholders::_1, FTM::StaticData::kExt1))
            ("Switch on the triggers through the first external line"
             "|Enable[bool]:Enable ext1 trigger (yes/no)");

        // FIXME: Switch on/off depending on sequence
        T::AddEvent("ENABLE_EXT2", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Enable, this, placeholders::_1, FTM::StaticData::kExt2))
            ("Switch on the triggers through the second external line"
             "|Enable[bool]:Enable ext2 trigger (yes/no)");

        T::AddEvent("ENABLE_VETO", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Enable, this, placeholders::_1, FTM::StaticData::kVeto))
            ("Enable veto line"
             "|Enable[bool]:Enable veto (yes/no)");

        T::AddEvent("ENABLE_CLOCK_CONDITIONER", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Enable, this, placeholders::_1, FTM::StaticData::kClockConditioner))
            ("Enable clock conidtioner output in favor of time marker output"
             "|Enable[bool]:Enable clock conditioner (yes/no)");

        T::AddEvent("ENABLE_GROUP1_LPINT", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnableLP, this, placeholders::_1, FTM::StaticData::kLPint, FTM::StaticData::kGroup1))
            ("");
        T::AddEvent("ENABLE_GROUP1_LPEXT", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnableLP, this, placeholders::_1, FTM::StaticData::kLPext, FTM::StaticData::kGroup1))
            ("");
        T::AddEvent("ENABLE_GROUP2_LPINT", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnableLP, this, placeholders::_1, FTM::StaticData::kLPint, FTM::StaticData::kGroup2))
            ("");
        T::AddEvent("ENABLE_GROUP2_LPEXT", "B:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::EnableLP, this, placeholders::_1, FTM::StaticData::kLPext, FTM::StaticData::kGroup2))
            ("");
        T::AddEvent("SET_INTENSITY_LPINT", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetIntensity, this, placeholders::_1, FTM::StaticData::kLPint))
            ("");
        T::AddEvent("SET_INTENSITY_LPEXT", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetIntensity, this, placeholders::_1, FTM::StaticData::kLPext))
            ("");


        T::AddEvent("SET_TRIGGER_SEQUENCE", "S:3", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTriggerSeq, this, placeholders::_1))
            ("Setup the sequence of artificial triggers produced by the FTM"
             "|Ped[short]:number of pedestal triggers in a row"
             "|LPext[short]:number of triggers of the external light pulser"
             "|LPint[short]:number of triggers of the internal light pulser");

        T::AddEvent("SET_TRIGGER_MULTIPLICITY", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTriggerMultiplicity, this, placeholders::_1))
            ("Setup the Multiplicity condition for physcis triggers"
             "|N[int]:Number of requirered coincident triggers from sum-patches (1-40)");

        T::AddEvent("SET_TRIGGER_WINDOW", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetTriggerWindow, this, placeholders::_1))
            ("");

        T::AddEvent("SET_CALIBRATION_MULTIPLICITY", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetCalibMultiplicity, this, placeholders::_1))
            ("Setup the Multiplicity condition for artificial (calibration) triggers"
             "|N[int]:Number of requirered coincident triggers from sum-patches (1-40)");

        T::AddEvent("SET_CALIBRATION_WINDOW", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetCalibWindow, this, placeholders::_1))
            ("");

        T::AddEvent("SET_CLOCK_FREQUENCY", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetClockFrequency, this, placeholders::_1))
            ("");

        T::AddEvent("SET_CLOCK_REGISTER", "X:8", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SetClockRegister, this, placeholders::_1))
            ("");

        // A new configure will first stop the FTM this means
        // we can allow it in idle _and_ taking data
        T::AddEvent("CONFIGURE", "C")(FTM::State::kIdle)(FTM::State::kValid)(FTM::State::kConfiguring1)(FTM::State::kConfiguring2)(FTM::State::kConfigured1)(FTM::State::kConfigured2)(FTM::State::kTriggerOn)
            (bind(&StateMachineFTM::ConfigureFTM, this, placeholders::_1))
            ("Configure a new run."
             "|time_max[s]:Maximum time before the run is closed in seconds (-1: unlimited)"
             "|num_max[int]:Maximum number of events before the run is closed in seconds (-1: unlimited)"
             "|run_type[string]:Run type which describes the runs");

        T::AddEvent("RESET_CONFIGURE")(FTM::State::kConfiguring1)(FTM::State::kConfiguring2)(FTM::State::kConfigured1)(FTM::State::kConfigured2)(FTM::State::kConfigError1)(FTM::State::kConfigError2)(FTM::State::kConfigError2)
            (bind(&StateMachineFTM::ResetConfig, this))
            ("Reset states during a configuration or in case of configuration error");



        T::AddEvent("RESET_CRATE", "S:1", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::ResetCrate, this, placeholders::_1))
            ("Reset one of the crates 0-3"
             "|crate[short]:Crate number to be reseted (0-3)");

        T::AddEvent("RESET_CAMERA", FTM::State::kIdle, FTM::State::kValid)
            (Wrapper(bind(&ConnectionFTM::CmdResetCamera, &fFTM)))
            ("Reset all crates. The commands are sent in the order 0,1,2,3");


        // Load/save static data block
        T::AddEvent("SAVE", "C", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::SaveStaticData, this, placeholders::_1))
            ("Saves the static data (FTM configuration) from memory to a file"
             "|filename[string]:Filename (can include a path), .bin is automatically added");

        T::AddEvent("LOAD", "C", FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::LoadStaticData, this, placeholders::_1))
            ("Loads the static data (FTM configuration) from a file into memory and sends it to the FTM"
             "|filename[string]:Filename (can include a path), .bin is automatically added");



        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineFTM::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("SET_HEX_OUTPUT", "B")
            (bind(&StateMachineFTM::SetHexOutput, this, placeholders::_1))
            ("enable or disable hex output for received data"
             "|hexout[bool]:disable or enable hex output for received data (yes/no)");

        T::AddEvent("SET_DYNAMIC_OUTPUT", "B")
            (bind(&StateMachineFTM::SetDynamicOut, this, placeholders::_1))
            ("enable or disable output for received dynamic data (data is still broadcasted via Dim)"
             "|dynout[bool]:disable or enable output for dynamic data (yes/no)");


        // Conenction commands
        T::AddEvent("DISCONNECT", FTM::State::kConnected, FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Disconnect, this))
            ("disconnect from ethernet");

        T::AddEvent("RECONNECT", "O", FTM::State::kDisconnected, FTM::State::kConnected, FTM::State::kIdle, FTM::State::kValid)
            (bind(&StateMachineFTM::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

        fFTM.StartConnect();
    }

    void SetEndpoint(const string &url)
    {
        fFTM.SetEndpoint(url);
    }

    map<uint16_t, array<uint64_t, 8>> fClockCondSetup;

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
        // ---------- General setup ----------
        fFTM.SetVerbose(!conf.Get<bool>("quiet"));
        fFTM.SetHexOutput(conf.Get<bool>("hex-out"));
        fFTM.SetDynamicOut(conf.Get<bool>("dynamic-out"));
        fFTM.SetDebugTx(conf.Get<bool>("debug-tx"));

        // ---------- Setup clock conditioner frequencies ----------
        const vector<uint16_t> freq = conf.Vec<uint16_t>("clock-conditioner.frequency");
        if (freq.empty())
            T::Warn("No frequencies for the clock-conditioner defined.");
        else
            T::Message("Defining clock conditioner frequencies");
        for (vector<uint16_t>::const_iterator it=freq.begin();
             it!=freq.end(); it++)
        {
            if (fClockCondSetup.count(*it)>0)
            {
                T::Error("clock-conditioner frequency defined twice.");
                return 1;
            }

            if (!conf.HasDef("clock-conditioner.R0.",  *it) ||
                !conf.HasDef("clock-conditioner.R1.",  *it) ||
                !conf.HasDef("clock-conditioner.R8.",  *it) ||
                !conf.HasDef("clock-conditioner.R9.",  *it) ||
                !conf.HasDef("clock-conditioner.R11.", *it) ||
                !conf.HasDef("clock-conditioner.R13.", *it) ||
                !conf.HasDef("clock-conditioner.R14.", *it) ||
                !conf.HasDef("clock-conditioner.R15.", *it))
            {
                T::Error("clock-conditioner values incomplete.");
                return 1;
            }

            array<uint64_t, 8> &arr = fClockCondSetup[*it];

            arr[0] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R0.",  *it);
            arr[1] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R1.",  *it);
            arr[2] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R8.",  *it);
            arr[3] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R9.",  *it);
            arr[4] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R11.", *it);
            arr[5] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R13.", *it);
            arr[6] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R14.", *it);
            arr[7] = conf.GetDef<Hex<uint32_t>>("clock-conditioner.R15.", *it);

            ostringstream out;
            out << " -> " << setw(4) << *it << "MHz:" << hex << setfill('0');
            for (int i=0; i<8; i++)
                out << " " << setw(8) << arr[i];
            T::Message(out.str());
        }

        // ---------- Setup run types ---------
        const vector<string> types = conf.Vec<string>("run-type");
        if (types.empty())
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

            if (!conf.HasDef("sampling-frequency.", *it))
            {
                T::Error("Neither sampling-frequency."+*it+" nor sampling-frequency.default found.");
                return 2;
            }

            const uint16_t frq = conf.GetDef<uint16_t>("sampling-frequency.", *it);

            FTM::StaticData data;
            data.SetClockRegister(fClockCondSetup[frq].data());

            // Trigger sequence ped:lp1:lp2
            // (data. is used here as an abbreviation for FTM::StaticData::
            if (!CheckConfigVal<bool>    (conf, true,                     "trigger.enable-trigger.",              *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "trigger.enable-external-1.",           *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "trigger.enable-external-2.",           *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "trigger.enable-veto.",                 *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "trigger.enable-clock-conditioner.",    *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "light-pulser.external.enable-group1.", *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "light-pulser.external.enable-group2.", *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "light-pulser.internal.enable-group1.", *it) ||
                !CheckConfigVal<bool>    (conf, true,                     "light-pulser.internal.enable-group2.", *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxSequence,        "trigger.sequence.pedestal.",           *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxSequence,        "trigger.sequence.lp-ext.",             *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxSequence,        "trigger.sequence.lp-int.",             *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxTriggerInterval, "trigger.sequence.interval.",           *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxMultiplicity,    "trigger.multiplicity-physics.",        *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxMultiplicity,    "trigger.multiplicity-calib.",          *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxWindow,          "trigger.coincidence-window-physics.",  *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxWindow,          "trigger.coincidence-window-calib.",    *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxDeadTime,        "trigger.dead-time.",                   *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxDelayTrigger,    "trigger.delay.",                       *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxDelayTimeMarker, "trigger.time-marker-delay.",           *it) ||
                !CheckConfigVal<uint16_t>(conf, 0xffff,                   "ftu-report-interval.",                 *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxIntensity,       "light-pulser.external.intensity.",     *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxIntensity,       "light-pulser.internal.intensity.",     *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxDAC,             "trigger.threshold.patch.",             *it) ||
                !CheckConfigVal<uint16_t>(conf, data.kMaxDAC,             "trigger.threshold.logic.",             *it) ||
                0)
                return 2;

            data.Enable(data.kTrigger,          conf.GetDef<bool>("trigger.enable-trigger.",           *it));
            data.Enable(data.kExt1,             conf.GetDef<bool>("trigger.enable-external-1.",        *it));
            data.Enable(data.kExt2,             conf.GetDef<bool>("trigger.enable-external-2.",        *it));
            data.Enable(data.kVeto,             conf.GetDef<bool>("trigger.enable-veto.",              *it));
            data.Enable(data.kClockConditioner, conf.GetDef<bool>("trigger.enable-clock-conditioner.", *it));

            data.EnableLPint(data.kGroup1, conf.GetDef<bool>("light-pulser.internal.enable-group1.", *it));
            data.EnableLPint(data.kGroup2, conf.GetDef<bool>("light-pulser.internal.enable-group2.", *it));
            data.EnableLPext(data.kGroup1, conf.GetDef<bool>("light-pulser.external.enable-group1.", *it));
            data.EnableLPext(data.kGroup2, conf.GetDef<bool>("light-pulser.external.enable-group2.", *it));

            // [ms] Interval between two artificial triggers (no matter which type) minimum 1ms, 10 bit
            data.fIntensityLPint      = conf.GetDef<uint16_t>("light-pulser.internal.intensity.", *it);
            data.fIntensityLPext      = conf.GetDef<uint16_t>("light-pulser.external.intensity.", *it);
            data.fTriggerInterval     = conf.GetDef<uint16_t>("trigger.sequence.interval.",          *it);
            data.fMultiplicityPhysics = conf.GetDef<uint16_t>("trigger.multiplicity-physics.",       *it);
            data.fMultiplicityCalib   = conf.GetDef<uint16_t>("trigger.multiplicity-calib.",         *it);
            data.fWindowPhysics       = conf.GetDef<uint16_t>("trigger.coincidence-window-physics.", *it); /// (4ns * x + 8ns)
            data.fWindowCalib         = conf.GetDef<uint16_t>("trigger.coincidence-window-calib.",   *it); /// (4ns * x + 8ns)
            data.fDelayTrigger        = conf.GetDef<uint16_t>("trigger.delay.",              *it); /// (4ns * x + 8ns)
            data.fDelayTimeMarker     = conf.GetDef<uint16_t>("trigger.time-marker-delay.",  *it); /// (4ns * x + 8ns)
            data.fDeadTime            = conf.GetDef<uint16_t>("trigger.dead-time.",          *it); /// (4ns * x + 8ns)

            data.SetPrescaling(conf.GetDef<uint16_t>("ftu-report-interval.", *it));

            const uint16_t seqped = conf.GetDef<uint16_t>("trigger.sequence.pedestal.",  *it);
            const uint16_t seqint = conf.GetDef<uint16_t>("trigger.sequence.lp-int.",    *it);
            const uint16_t seqext = conf.GetDef<uint16_t>("trigger.sequence.lp-ext.",    *it);

            data.SetSequence(seqped, seqint, seqext);

            data.EnableAllFTU();
            data.EnableAllPixel();

            const vector<uint16_t> pat1 = conf.Vec<uint16_t>("trigger.disable-patch.default");
            const vector<uint16_t> pat2 = conf.Vec<uint16_t>("trigger.disable-patch."+*it);

            const vector<uint16_t> pix1 = conf.Vec<uint16_t>("trigger.disable-pixel.default");
            const vector<uint16_t> pix2 = conf.Vec<uint16_t>("trigger.disable-pixel."+*it);

            const vector<uint16_t> ftu1 = conf.Vec<uint16_t>("disable-ftu.default");
            const vector<uint16_t> ftu2 = conf.Vec<uint16_t>("disable-ftu."+*it);

            vector<uint16_t> ftu, pat, pix;
            ftu.insert(ftu.end(), ftu1.begin(), ftu1.end());
            ftu.insert(ftu.end(), ftu2.begin(), ftu2.end());
            pat.insert(pat.end(), pat1.begin(), pat1.end());
            pat.insert(pat.end(), pat2.begin(), pat2.end());
            pix.insert(pix.end(), pix1.begin(), pix1.end());
            pix.insert(pix.end(), pix2.begin(), pix2.end());

            for (vector<uint16_t>::const_iterator ip=ftu.begin(); ip!=ftu.end(); ip++)
            {
                if (*ip>FTM::StaticData::kMaxPatchIdx)
                {
                    ostringstream str;
                    str << "disable-ftu.*=" << *ip << " exceeds allowed maximum of " << FTM::StaticData::kMaxPatchIdx << "!";
                    T::Error(str);
                    return 2;
                }
                data.DisableFTU(*ip);
            }
            for (vector<uint16_t>::const_iterator ip=pat.begin(); ip!=pat.end(); ip++)
            {
                if (*ip>FTM::StaticData::kMaxPatchIdx)
                {
                    ostringstream str;
                    str << "trigger.disable-patch.*=" << *ip << " exceeds allowed maximum of " << FTM::StaticData::kMaxPatchIdx << "!";
                    T::Error(str);
                    return 2;
                }
                data.EnablePatch(*ip, false);
            }
            for (vector<uint16_t>::const_iterator ip=pix.begin(); ip!=pix.end(); ip++)
            {
                if (*ip>FTM::StaticData::kMaxPixelIdx)
                {
                    ostringstream str;
                    str << "trigger.disable-pixel.*=" << *ip << " exceeds allowed maximum of " << FTM::StaticData::kMaxPixelIdx << "!";
                    T::Error(str);
                    return 2;
                }
                data.EnablePixel(*ip, false);
            }

            const uint16_t th0 = conf.GetDef<uint16_t>("trigger.threshold.patch.", *it);
            const uint16_t th1 = conf.GetDef<uint16_t>("trigger.threshold.logic.", *it);

            for (int i=0; i<40; i++)
            {
                data[i].fDAC[0] = th0;
                data[i].fDAC[1] = th0;
                data[i].fDAC[2] = th0;
                data[i].fDAC[3] = th0;
                data[i].fDAC[4] = th1;
            }

            fConfigs[*it] = data;

            // trigger.threshold.dac-0:

            /*
             threshold-A  data[n].fDAC[0] = val
             threshold-B  data[n].fDAC[1] = val
             threshold-C  data[n].fDAC[2] = val
             threshold-D  data[n].fDAC[3] = val
             threshold-H  data[n].fDAC[4] = val
             */

            // kMaxDAC = 0xfff,
        }

        // FIXME: Add a check about unsused configurations

        // ---------- FOR TESTING PURPOSE ---------

        //        fFTM.SetDefaultSetup(conf.Get<string>("default-setup"));
        fConfigs["test"] = FTM::StaticData();

        // ---------- Setup connection endpoint ---------
        SetEndpoint(conf.Get<string>("addr"));

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineFTM<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Control options");
    control.add_options()
        ("no-dim",        po_bool(),  "Disable dim services")
        ("addr,a",        var<string>("localhost:5000"),  "Network address of FTM")
        ("quiet,q",       po_bool(true), "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("hex-out",       po_bool(),  "Enable printing contents of all printed messages also as hex data.")
        ("dynamic-out",   po_bool(),  "Enable printing received dynamic data.")
        ("debug-tx",      po_bool(),  "Enable debugging of ethernet transmission.")
//        ("default-setup", var<string>(), "Binary file with static data loaded whenever a connection to the FTM was established.")
        ;

    po::options_description freq("Sampling frequency setup");
    freq.add_options()
        ("clock-conditioner.frequency",  vars<uint16_t>(),      "Frequencies for which to setup the clock-conditioner (replace the * in the following options by this definition)")
        ("clock-conditioner.R0.*",       var<Hex<uint32_t>>(),  "Clock-conditioner R0")
        ("clock-conditioner.R1.*",       var<Hex<uint32_t>>(),  "Clock-conditioner R1")
        ("clock-conditioner.R8.*",       var<Hex<uint32_t>>(),  "Clock-conditioner R8")
        ("clock-conditioner.R9.*",       var<Hex<uint32_t>>(),  "Clock-conditioner R9")
        ("clock-conditioner.R11.*",      var<Hex<uint32_t>>(),  "Clock-conditioner R11")
        ("clock-conditioner.R13.*",      var<Hex<uint32_t>>(),  "Clock-conditioner R13")
        ("clock-conditioner.R14.*",      var<Hex<uint32_t>>(),  "Clock-conditioner R14")
        ("clock-conditioner.R15.*",      var<Hex<uint32_t>>(),  "Clock-conditioner R15");

    po::options_description runtype("Run type configuration");
    runtype.add_options()
        ("run-type",                     vars<string>(),        "Name of run-types (replace the * in the following configuration by the case-sensitive names defined here)")
        ("sampling-frequency.*",         var<uint16_t>(),       "Sampling frequency as defined in the clock-conditioner.frequency")
        ("trigger.enable-trigger.*",             var<bool>(),   "Enable trigger output of physics trigger")
        ("trigger.enable-external-1.*",          var<bool>(),   "Enable external trigger line 1")
        ("trigger.enable-external-2.*",          var<bool>(),   "Enable external trigger line 2")
        ("trigger.enable-veto.*",                var<bool>(),   "Enable veto line")
        ("trigger.enable-clock-conditioner.*",   var<bool>(),   "")
        ("trigger.sequence.interval.*",          var<uint16_t>(),  "Interval between two artifical triggers in units of ms")
        ("trigger.sequence.pedestal.*",          var<uint16_t>(),  "Number of pedestal events in the sequence of artificial triggers")
        ("trigger.sequence.lp-int.*",            var<uint16_t>(),  "Number of LPint events in the sequence of artificial triggers")
        ("trigger.sequence.lp-ext.*",            var<uint16_t>(),  "Number of LPext events in the sequence of artificial triggers")
        ("trigger.multiplicity-physics.*",       var<uint16_t>(),  "Multiplicity for physics events (n out of 40)")
        ("trigger.multiplicity-calib.*",         var<uint16_t>(),  "Multiplicity for LPext events (n out of 40)")
        ("trigger.coincidence-window-physics.*", var<uint16_t>(),  "Coincidence window for physics triggers in units of n*4ns+8ns")
        ("trigger.coincidence-window-calib.*",   var<uint16_t>(),  "Coincidence window for LPext triggers in units of n*4ns+8ns")
        ("trigger.dead-time.*",                  var<uint16_t>(),  "Dead time after trigger in units of n*4ns+8ns")
        ("trigger.delay.*",                      var<uint16_t>(),  "Delay of the trigger send to the FAD boards after a trigger in units of n*4ns+8ns")
        ("trigger.time-marker-delay.*",          var<uint16_t>(),  "Delay of the time-marker after a trigger in units of n*4ns+8ns")
        ("trigger.disable-pixel.*",              vars<uint16_t>(), "")
        ("trigger.disable-patch.*",              vars<uint16_t>(), "")
        ("trigger.threshold.patch.*",            var<uint16_t>(),  "")
        ("trigger.threshold.logic.*",            var<uint16_t>(),  "")
        ("ftu-report-interval.*",                var<uint16_t>(),  "")
        ("disable-ftu.*",                        vars<uint16_t>(), "")
        ("light-pulser.external.enable-group1.*", var<bool>(),     "Enable LED group 1 of external light pulser")
        ("light-pulser.external.enable-group2.*", var<bool>(),     "Enable LED group 2 of external light pulser")
        ("light-pulser.internal.enable-group1.*", var<bool>(),     "Enable LED group 1 of internal light pulser")
        ("light-pulser.internal.enable-group2.*", var<bool>(),     "Enable LED group 2 of internal light pulser")
        ("light-pulser.external.intensity.*",     var<uint16_t>(), "Intensity of external light pulser")
        ("light-pulser.internal.intensity.*",     var<uint16_t>(), "Intensity of internal light pulser")
        ;

    conf.AddOptions(control);
    conf.AddOptions(freq);
    conf.AddOptions(runtype);
}

/*
 Extract usage clause(s) [if any] for SYNOPSIS.
 Translators: "Usage" and "or" here are patterns (regular expressions) which
 are used to match the usage synopsis in program output.  An example from cp
 (GNU coreutils) which contains both strings:
  Usage: cp [OPTION]... [-T] SOURCE DEST
    or:  cp [OPTION]... SOURCE... DIRECTORY
    or:  cp [OPTION]... -t DIRECTORY SOURCE...
 */
void PrintUsage()
{
    cout <<
        "The ftmctrl controls the FTM (FACT Trigger Master) board.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: ftmctrl [-c type] [OPTIONS]\n"
        "  or:  ftmctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineFTM<StateMachine, ConnectionFTM>>();

    /* Additional help text which is printed after the configuration
     options goes here */

    /*
     cout << "bla bla bla" << endl << endl;
     cout << endl;
     cout << "Environment:" << endl;
     cout << "environment" << endl;
     cout << endl;
     cout << "Examples:" << endl;
     cout << "test exam" << endl;
     cout << endl;
     cout << "Files:" << endl;
     cout << "files" << endl;
     cout << endl;
     */
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    //try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
            if (conf.Get<bool>("no-dim"))
                return RunShell<LocalStream, StateMachine, ConnectionFTM>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimFTM>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionFTM>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionFTM>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimFTM>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimFTM>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
