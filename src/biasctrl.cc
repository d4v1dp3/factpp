#include <functional>

#include <boost/bind.hpp>

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "ConnectionUSB.h"
#include "Configuration.h"
#include "Console.h"
#include "PixelMap.h"

#include "tools.h"

#include "LocalControl.h"
#include "HeadersBIAS.h"

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std::placeholders;
using namespace std;

// We can do that because we do not include other headers than HeadersBIAS
using namespace BIAS;

// ------------------------------------------------------------------------

class ConnectionBias : public ConnectionUSB
{
    boost::asio::deadline_timer fSyncTimer;
    boost::asio::deadline_timer fRampTimer;
    boost::asio::deadline_timer fUpdateTimer;

    vector<uint8_t> fBuffer;
    vector<uint8_t> fBufferRamp;
    vector<uint8_t> fBufferUpdate;

    bool fIsVerbose;
    bool fIsDummyMode;

    vector<bool>     fPresent;

    int64_t fWrapCounter;
    int64_t fSendCounter;

    int16_t fGlobalDacCmd;      // Command value to be reached

    int16_t fRampStep;
    int16_t fRampTime;

    uint32_t fUpdateTime;
    uint16_t fSyncTime;
    uint32_t fReconnectDelay;

    int  fIsInitializing;
    bool fIsRamping;
    int  fWaitingForAnswer;

    vector<uint64_t> fCounter;

    Time fLastConnect;

    int32_t  fEmergencyLimit;
    bool     fEmergencyShutdown;

protected:

    vector<int16_t>  fCurrent;     // Current in ADC units (12bit = 5mA)

    virtual void UpdateV(const Time = Time())
    {
    }

    virtual void UpdateVgapd()
    {
    }

public:
    virtual void UpdateVA()
    {
    }

    // ====================================================

protected:
    vector<float> fOperationVoltage;      // Operation voltage of GAPDs
    //vector<float> fChannelOffset;         // User defined channel offset

    vector<float> fCalibrationOffset;     // Bias crate channel offset
    vector<float> fCalibrationSlope;      // Bias crate channel slope

    float fVoltageMaxAbs;  // Maximum voltage
    float fVoltageMaxRel;  // Maximum voltgage above (what?)

    vector<uint16_t> fDacTarget;    // Target values
    vector<uint16_t> fDacCommand;   // Last sent command value
    vector<uint16_t> fDacActual;    // Actual value

    // ====================================================

private:
    vector<char> GetCmd(uint16_t board, uint16_t channel, Command_t cmd, uint16_t dac=0)
    {
        vector<char> data(3);

        /*
        if (board>kNumBoards)
            return;
        if (channel>kNumChannelsPerBoard)
            return;
        if (dac>0xfff)
            return;
        */

        data[0] = (cmd<<5) | (board<<1) | (((channel&16)>>4) & 1);
        data[1] = (channel<<4) | (dac>>8);
        data[2] =  dac&0xff;

        return data;
    }

    vector<char> GetCmd(Command_t cmd, uint16_t id=0, uint16_t dac=0)
    {
        const unsigned int board   = id/kNumChannelsPerBoard;
        const unsigned int channel = id%kNumChannelsPerBoard;

        return GetCmd(board, channel, cmd, dac);
    }

    bool CheckMessageLength(int received, int expected, const string &msg)
    {
        if (received==expected)
            return true;

        ostringstream str;
        str << msg << ": Expected " << expected << " bytes in answer, but got " << received << endl;
        Error(str);

        return false;
    }

    bool EvalAnswer(const uint8_t *answer, uint16_t id, int command)
    {
        answer += id*3;

        const uint16_t status = (answer[0]>>7)&1;
        const uint16_t wrap   = (answer[0]>>4)&7;
        const uint16_t ddd    = ((uint16_t(answer[0])&0xf)<<8) | answer[1];
        const uint16_t error  = (answer[2]>>4)&0xf;
        const uint16_t board  =  answer[2]&0xf;

        // 0x10 00 7f
        //   status = 0
        //   wrap   = 1
        //   ddd    = 0
        //   error  = not present
        //   board  = 15

        /*
        Out() << dec << setw(2) << board << '|' << wrap << " ";
        if (id%8==7)
            Out() << endl;
            */

        if (fWrapCounter>=0)
        {
            if ((fWrapCounter+1)%8 != wrap)
            {
                ostringstream msg;
                msg << "Corrupted answer (id=" << id << "): received wrap counter " << wrap << " doesn't match last one " << fWrapCounter << " ";
                msg << " (fSendCounter=" << fSendCounter << ")";
                Error(msg);
                return false;
            }
        }

        fWrapCounter = wrap;

        if (command==kSynchronize)
        {
            ostringstream msg;
            msg << hex << setfill('0');
            msg << "Initial answer received: 0x";
            msg << setw(2) << (int)answer[2];
            msg << setw(2) << (int)answer[1];
            msg << setw(2) << (int)answer[0];
            Message(msg);

            if (status!=0 || ddd!=0 || error!=0 || board!=0)
            {
                Warn("Initial answer doesn't seem to be a reset as naively expected.");

                //ostringstream msg;
                //msg << hex << setfill('0');
                //msg << "S=" << status << " D=" << ddd << " E=" << error << " B=" << board;
                //Message(msg);
            }

            fSendCounter = wrap;

            msg.str("");
            msg << "Setting fSendCounter to " << wrap;
            Info(msg);

            return true;
        }

        if (error==0x8) // No device
        {
            Message("Reset button on crate pressed!");
            RampAllDacs(0);
            return true;
        }

        if (command==kCmdReset)
        {
            if (status==0 && ddd==0 && error==0 && board==0)
            {
                Message("Reset successfully executed.");
                return true;
            }

            Warn("Answer to 'reset' command contains unexpected data.");
            return false;
        }

        if (command==kCmdGlobalSet)
        {
            if (status==0 && ddd==0 && error==0 && board==0)
            {
                for (int i=0; i<kNumChannels; i++)
                    fDacActual[i] = fGlobalDacCmd;

                fGlobalDacCmd = -1;

                return true;
            }

            Warn("Answer to 'global set' command contains unexpected data.");
            return false;
        }

        if ((command&0xff)==kExpertChannelSet)
            id = command>>8;

        const int cmd = command&3;

        if (cmd==kCmdRead || cmd==kCmdChannelSet)
        {
            if (board!=id/kNumChannelsPerBoard)
            {
                ostringstream out;
                out << "Talked to board " << id/kNumChannelsPerBoard << ", but got answer from board " <<  board << " (fSendCounter=" << fSendCounter << ")";
                Error(out);
                return false;
            }

            // Not present
            if (error==0x7 || error==0xf)
            {
                fPresent[board] = false;
                fCurrent[id]    = 0x8000;
                return true;
            }

            // There is no -0 therefore we make a trick and replace it by -1.
            // This is not harmfull, because typical zero currents are in the
            // order of one to three bits anyway and they are never stable.
            fCurrent[id]    = status ? -(ddd==0?1:ddd) : ddd;
            fPresent[board] = true;

            if (!fEmergencyShutdown)
            {
                if (fCurrent[id]<0 && id!=91)
                {
                    Warn("OverCurrent detected (first ch="+to_string(id)+").");
                    fEmergencyShutdown = true;
                }

                if (fEmergencyLimit>0 && fCurrent[id]>fEmergencyLimit && !fEmergencyShutdown)
                {
                    Warn("Emergency limit exceeded (first ch="+to_string(id)+").");
                    fEmergencyShutdown = true;
                }

                if (fEmergencyShutdown)
                {
                    Error("Emergency ramp down initiated.");
                    Dim::SendCommandNB("MCP/STOP");
                    RampAllDacs(0);
                }
            }
        }

        if (cmd==kCmdChannelSet)
            fDacActual[id] = fDacCommand[id];

        return true;

    }

private:
    void DelayedReconnect()
    {
        const Time now;

        // If we have been connected without a diconnect for at least 60s
        // we can reset the delay.
        if (now-fLastConnect>boost::posix_time::seconds(60))
            fReconnectDelay = 1;

        ostringstream msg;
        msg << "Automatic reconnect in " << fReconnectDelay << "s after being connected for ";
        msg << (now-fLastConnect).seconds() << "s";
        Info(msg);

        CloseImp(fReconnectDelay);
        fReconnectDelay *= 2;
    }

    void HandleReceivedData(const vector<uint8_t> &buf, size_t bytes_received, int command, int send_counter)
    {
#ifdef DEBUG
    ofstream fout("received.txt", ios::app);
    fout << Time() << ": ";
    for (unsigned int i=0; i<bytes_received; i++)
        fout << hex << setfill('0') << setw(2) << (uint16_t)buf[i];
    fout << endl;
#endif

        // Now print the received message if requested by the user
        if (fIsVerbose/* && command!=kUpdate*/)
        {
            Out() << endl << kBold << dec << "Data received (size=" << bytes_received << "):" << endl;
            Out() << " Command=" << command << " fWrapCounter=" << fWrapCounter << " fSendCounter=" << fSendCounter << " fIsInitializing=" << fIsInitializing << " fIsRamping=" << fIsRamping;
            Out() << hex << setfill('0');

            for (size_t i=0; i<bytes_received/3; i++)
            {
                if (i%8==0)
                    Out() << '\n' << setw(2) << bytes_received/24 << "| ";

                Out() << setw(2) << uint16_t(buf[i*3+2]);
                Out() << setw(2) << uint16_t(buf[i*3+1]);
                Out() << setw(2) << uint16_t(buf[i*3+0]) << " ";
            }
            Out() << endl;
        }

        const int cmd = command&0xf;

        // Check the number of received_byted according to the answer expected
        if ((cmd==kSynchronize      && !CheckMessageLength(bytes_received, 3,                "Synchronization")) ||
            (cmd==kCmdReset         && !CheckMessageLength(bytes_received, 3,                "CmdReset"))        ||
            (cmd==kCmdRead          && !CheckMessageLength(bytes_received, 3*kNumChannels,   "CmdRead"))         ||
            (cmd==kCmdChannelSet    && !CheckMessageLength(bytes_received, 3*kNumChannels,   "CmdChannelSet"))   ||
            (cmd==kExpertChannelSet && !CheckMessageLength(bytes_received, 3,                "CmdExpertChannelSet")))
        {
            CloseImp(-1);
            return;
        }

        // Now evaluate the whole bunch of messages
        for (size_t i=0; i<bytes_received/3; i++)
        {
            if (!EvalAnswer(buf.data(), i, command))
            {
                DelayedReconnect();
                return;
            }
        }

        if (command==kSynchronize)
        {
            Message("Stream successfully synchronized.");
            fIsInitializing = 2;

            // Cancel sending of the next 0
            fSyncTimer.cancel();
            fCounter[0]++;

            // Start continous reading of all channels
            ScheduleUpdate(100);
            return;
        }

        if (send_counter%8 != fWrapCounter)
        {
            ostringstream msg;
            msg << "Corrupted answer: received wrap counter " << fWrapCounter  << " is not send counter " << send_counter << "%8.";
            Error(msg);

            DelayedReconnect();
        }


        // Check if new values have been received
        if (cmd==kCmdRead || cmd==kCmdChannelSet || cmd==kExpertChannelSet)
            UpdateVA();

        // ----- Take action depending on what is going on -----

        if (command==kCmdReset)
        {
            Message("Reset command successfully answered...");

            fCounter[1]++;

            // Re-start cyclic reading of values after a short time
            // to allow the currents to become stable. This ensures that
            // we get an update soon but wait long enough to get reasonable
            // values
            fUpdateTimer.cancel();

            if (fUpdateTime==0)
                ReadAllChannels(true);
            else
            {
                Message("...restarting automatic readout.");
                ScheduleUpdate(100);
            }
        }

        if (command==kResetChannels)
        {
            ExpertReset(false);
            fCounter[5]++;
        }

        if (command==kUpdate)
        {
            ScheduleUpdate(fUpdateTime);
            fCounter[2]++;
        }

        // If we are ramping, schedule a new ramp step
        if (command==kCmdChannelSet && fIsRamping)
        {
            bool oc = false;
            for (int ch=0; ch<kNumChannels; ch++)
                if (fPresent[ch/kNumChannelsPerBoard] && fCurrent[ch]<0 && ch!=91)
                    oc = true;

            if (oc)
            {
                if (!fEmergencyShutdown)
                {
                    Warn("OverCurrent detected - emergency ramp down initiated.");
                    Dim::SendCommandNB("MCP/STOP");
                    RampAllDacs(0);
                    fEmergencyShutdown = true;
                }
            }
            else
                ScheduleRampStep();

            fCounter[3]++;
        }

        if (command==kCmdRead)
            fCounter[4]++;

        if ((command&0xff)==kExpertChannelSet)
            fCounter[6]++;

        if (command==kCmdGlobalSet)
            fCounter[7]++;
    }

    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int command, int send_counter)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
            {
                ostringstream msg;
                msg << "Connection closed by remote host (BIAS, fSendCounter=" << fSendCounter << ")";
                Warn(msg);
            }

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
            CloseImp(-1);//err!=ba::error::basic_errors::operation_aborted);
            return;
        }

        // Check if the number of received bytes is correctly dividable by 3
        // This check should never fail - just for sanity
        if (bytes_received%3)
        {
            Error("Number of received bytes not a multiple of 3, can't read data.");
            CloseImp(-1);
            return;
        }

        // We have three different parallel streams:
        //  1) The setting of voltages due to ramping
        //  2) The cynclic request of the currents
        //  3) Answers to commands
        // For each of these three streams an own buffer is needed, otherwise
        // a buffer which is filled in the background might overwrite
        // a buffer which is currently evaluated. In all other programs
        // this is no problem because the boards don't answer and if
        // they do the answer identifies itself. Consequently,
        // there is always only one async_read in progress. Here we have
        // three streams which need to be connected somehow to the
        // commands.

        // Maybe a better possibility would be to setup a command
        // queue (each command will be queued in a buffer)
        // and whenever an answer has been received, a new async_read is
        // scheduled.
        // Build a command queue<pair<command, vector<char>>>
        ///  This replaces the send counter and the command argument
        //   in handleReceivedData

        switch (command&0xff)
        {
        case kSynchronize:
        case kCmdReset:
        case kExpertChannelSet:
        case kCmdGlobalSet:
        case kResetChannels:
        case kCmdRead:
            HandleReceivedData(fBuffer, bytes_received, command, send_counter);
            fWaitingForAnswer = -1;
            return;

        case kCmdChannelSet:
            HandleReceivedData(fBufferRamp, bytes_received, command, send_counter);
            return;

        case kUpdate:
            HandleReceivedData(fBufferUpdate, bytes_received, command, send_counter);
            return;
        }
    }

    // --------------------------------------------------------------------

    void HandleSyncTimer(int counter, const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
        {
            if (fIsInitializing==1)
                Warn("Synchronization aborted...");
            // case 0 and 2 should not happen
            return;
        }

        if (error)
        {
            ostringstream str;
            str << "Synchronization timer: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            CloseImp(-1);
            return;
        }

        if (!is_open())
        {
            Warn("Synchronization in progress, but disconnected.");
            return;
        }

        ostringstream msg;
        msg << "Synchronization time expired (" << counter << ")";
        Info(msg);

        if (fIsInitializing)
        {
            PostMessage("\0", 1);

            if (counter==2)
            {
                Error("Synchronization attempt timed out.");
                CloseImp(-1);
                return;
            }

            ScheduleSync(counter+1);
            return;
        }

        Info("Synchronisation successfull.");
    }

    void ScheduleSync(int counter=0)
    {
        fSyncTimer.expires_from_now(boost::posix_time::milliseconds(fSyncTime));
        fSyncTimer.async_wait(boost::bind(&ConnectionBias::HandleSyncTimer, this, counter, dummy::error));
    }

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        // We connect for the first time or haven't received
        // a valid warp counter yet... this procedure also sets
        // our volatges to 0 if we have connected but never received
        // any answer.
        if (fWrapCounter<0)
        {
            fDacTarget.assign(kNumChannels, 0);
            fDacCommand.assign(kNumChannels, 0);
            fDacActual.assign(kNumChannels, 0);
        }

        // Reset everything....
        fSendCounter    = -1;
        fWrapCounter    = -1;
        fGlobalDacCmd   = -1;
        fIsInitializing =  1;
        fIsRamping      = false;

        fLastConnect = Time();

        // Send a single 0 (and possible two consecutive 0's
        // to make sure we are in sync with the device)
        PostMessage("\0", 1);
        AsyncRead(ba::buffer(fBuffer, 3), kSynchronize, 0);//++fSendCounter);
        fWaitingForAnswer = kSynchronize;

        // Wait for some time before sending the next 0
        ScheduleSync();
    }

    // --------------------------------------------------------------------

    void HandleUpdateTimer(const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
        {
            Warn("Update timer aborted...");
            fIsRamping = false;
            return;
        }

        if (error)
        {
            ostringstream str;
            str << "Update timer: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            CloseImp(-1);
            return;
        }

        if (!is_open())
            return;

        if (fUpdateTime==0 && fIsInitializing!=2)
            return;

        if (fIsRamping)
            ScheduleUpdate(fUpdateTime);
        else
            ReadAllChannels(true);

        fIsInitializing = 0;
    }

    void ScheduleUpdate(int millisec)
    {
        fUpdateTimer.expires_from_now(boost::posix_time::milliseconds(millisec));
        fUpdateTimer.async_wait(boost::bind(&ConnectionBias::HandleUpdateTimer, this, dummy::error));
    }

    // --------------------------------------------------------------------

    void PrintLineCmdDac(int b, int ch, const vector<uint16_t> &dac)
    {
        Out() << setw(2) << b << "|";

        for (int c=ch; c<ch+4; c++)
        {
            const int id = c+kNumChannelsPerBoard*b;
            Out() << " " << setw(4) << int32_t(dac[id])<<"/"<<fDacActual[id] << ":" << setw(5) << ConvertDacToVolt(id, fDacTarget[id]);
        }
        Out() << endl;
    }

    void PrintCommandDac(const vector<uint16_t> &dac)
    {
        Out() << dec << setprecision(2) << fixed << setfill(' ');
        for (int b=0; b<kNumBoards; b++)
        {
            if (!fPresent[b])
            {
                Out() << setw(2) << b << "-" << endl;
                continue;
            }

            PrintLineCmdDac(b,  0, dac);
            PrintLineCmdDac(b,  4, dac);
            PrintLineCmdDac(b,  8, dac);
            PrintLineCmdDac(b, 12, dac);
            PrintLineCmdDac(b, 16, dac);
            PrintLineCmdDac(b, 20, dac);
            PrintLineCmdDac(b, 24, dac);
            PrintLineCmdDac(b, 28, dac);
        }
    }

    void SetAllChannels(const vector<uint16_t> &dac, bool special=false)
    {
        if (fIsDummyMode)
        {
            PrintCommandDac(dac);
            return;
        }

        vector<char> data;
        data.reserve(kNumChannels*3);

        for (int ch=0; ch<kNumChannels; ch++)
        {
            // FIXME: dac[ch] += calib_offset
            const vector<char> cmd = GetCmd(kCmdChannelSet, ch, dac[ch]);
            data.insert(data.end(), cmd.begin(), cmd.end());

            fDacCommand[ch] = dac[ch];
        }

        fSendCounter += kNumChannels;

        PostMessage(data);
        AsyncRead(ba::buffer(special ? fBuffer : fBufferRamp, kNumChannels*3),
                  special ? kResetChannels : kCmdChannelSet, fSendCounter);

        if (special)
            fWaitingForAnswer = kResetChannels;
    }

    uint16_t RampOneStep(uint16_t ch)
    {
        if (fDacTarget[ch]>fDacActual[ch])
            return fDacActual[ch]+fRampStep>fDacTarget[ch] ? fDacTarget[ch] : fDacActual[ch]+fRampStep;

        if (fDacTarget[ch]<fDacActual[ch])
            return fDacActual[ch]-fRampStep<fDacTarget[ch] ? fDacTarget[ch] : fDacActual[ch]-fRampStep;

        return fDacActual[ch];
    }

    bool RampOneStep()
    {
        if (fRampTime<0)
        {
            Warn("Ramping step time not yet set... ramping not started.");
            return false;
        }
        if (fRampStep<0)
        {
            Warn("Ramping step not yet set... ramping not started.");
            return false;
        }

        vector<uint16_t> dac(kNumChannels);

        bool identical = true;
        for (int ch=0; ch<kNumChannels; ch++)
        {
            dac[ch] = RampOneStep(ch);
            if (dac[ch]!=fDacActual[ch] && fPresent[ch/kNumChannelsPerBoard])
                identical = false;
        }

        if (identical)
        {
            Info("Ramping: target values reached.");
            return false;
        }

        if (fWaitingForAnswer<0)
        {
            SetAllChannels(dac);
            return true;
        }

        ostringstream msg;
        msg << "RampOneStep while waiting for answer to last command (id=" << fWaitingForAnswer << ")... ramp step delayed.";
        Warn(msg);

        // Delay ramping
        ScheduleRampStep();
        return true;
    }

    void HandleRampTimer(const bs::error_code &error)
    {
        if (error==ba::error::basic_errors::operation_aborted)
        {
            Warn("Ramping aborted...");
            fIsRamping = false;
            return;
        }

        if (error)
        {
            ostringstream str;
            str << "Ramping timer: " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            fIsRamping = false;
            CloseImp(-1);
            return;
        }

        if (!is_open())
        {
            Warn("Ramping in progress, but disconnected.");
            fIsRamping = false;
            return;
        }

        if (!fIsRamping)
        {
            Error("Ramp handler called although no ramping in progress.");
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fRampTimer.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        fIsRamping = RampOneStep();
    }

    void ScheduleRampStep()
    {
        fRampTimer.expires_from_now(boost::posix_time::milliseconds(fRampTime));
        fRampTimer.async_wait(boost::bind(&ConnectionBias::HandleRampTimer, this, dummy::error));
    }

public:
    ConnectionBias(ba::io_service& ioservice, MessageImp &imp) : ConnectionUSB(ioservice, imp()),
        fSyncTimer(ioservice),
        fRampTimer(ioservice),
        fUpdateTimer(ioservice),
        fBuffer(3*kNumChannels),
        fBufferRamp(3*kNumChannels),
        fBufferUpdate(3*kNumChannels),
        fIsVerbose(false),
        fIsDummyMode(false),
        fPresent(kNumBoards),
        fWrapCounter(-1),
        fRampStep(-1),
        fRampTime(-1),
        fUpdateTime(3000),
        fSyncTime(333),
        fReconnectDelay(1),
        fIsRamping(false),
        fWaitingForAnswer(-1),
        fCounter(8),
        fEmergencyLimit(0),
        fEmergencyShutdown(false),
        fCurrent(kNumChannels),
        fOperationVoltage(kNumChannels, 0),
        //fChannelOffset(kNumChannels),
        fCalibrationOffset(kNumChannels),
        fCalibrationSlope(kNumChannels, 90000),
        fVoltageMaxAbs(75),
        fVoltageMaxRel(2),
        fDacTarget(kNumChannels),
        fDacCommand(kNumChannels),
        fDacActual(kNumChannels)
    {
        SetLogStream(&imp);
    }

    // --------------------------------------------------------------------

    bool CheckDac(uint16_t dac)
    {
        if (dac<4096)
            return true;

        ostringstream msg;
        msg << "CheckDac - Dac value of " << dac << " exceeds maximum of 4095.";
        Error(msg);
        return false;
    }

    bool CheckChannel(uint16_t ch)
    {
        if (ch<kNumChannels)
            return true;

        ostringstream msg;
        msg << "CheckChannel - Channel " << ch << " out of range [0;" << kNumChannels-1 << "].";
        Error(msg);
        return false;
    }

    bool CheckChannelVoltage(uint16_t ch, float volt)
    {
        if (volt>fVoltageMaxAbs)
        {
            ostringstream msg;
            msg << "CheckChannelVoltage - Set voltage " << volt << "V of channel " << ch << " exceeds absolute limit of " << fVoltageMaxAbs << "V.";
            Warn(msg);
            return false;
        }

        if (fOperationVoltage[ch]<=0)
            return true;

        if (volt>fOperationVoltage[ch]+fVoltageMaxRel) // FIXME: fVoltageMaxRel!!!
        {
            ostringstream msg;
            msg << "CheckChannelVoltage - Set voltage " << volt << "V of channel " << ch << " exceeds limit of " << fVoltageMaxRel << "V above operation voltage " << fOperationVoltage[ch] << "V + limit " << fVoltageMaxRel << "V.";
            Error(msg);
            return false;
        }

        return true;
    }

    // --------------------------------------------------------------------

    bool RampSingleChannelDac(uint16_t ch, uint16_t dac)
    {
        if (!CheckChannel(ch))
            return false;

        if (!CheckDac(dac))
            return false;

        fDacTarget[ch] = dac;
        UpdateV();

        if (!fIsRamping)
            fIsRamping = RampOneStep();

        return true;
    }

    bool RampAllChannelsDac(const vector<uint16_t> &dac)
    {
        for (int ch=0; ch<kNumChannels; ch++)
            if (!CheckDac(dac[ch]))
                return false;

        fDacTarget = dac;
        UpdateV();

        if (!fIsRamping)
            fIsRamping = RampOneStep();

        return true;
    }

    bool RampAllDacs(uint16_t dac)
    {
        return RampAllChannelsDac(vector<uint16_t>(kNumChannels, dac));
    }

    // --------------------------------------------------------------------

    uint16_t ConvertVoltToDac(uint16_t ch, double volt)
    {
        if (fCalibrationSlope[ch]<=0)
            return 0;

        const double current = (volt-fCalibrationOffset[ch])/fCalibrationSlope[ch];
        return current<0 ? 0 : nearbyint(current*4096000); // Current [A] to dac [ /= 1mA/4096]
    }

    double ConvertDacToVolt(uint16_t ch, uint16_t dac)
    {
        if (fCalibrationSlope[ch]<=0)
            return 0;

        const double current = dac/4096000.;  // Convert dac to current [A] [ *= 1mA/4096]
        return current*fCalibrationSlope[ch] + fCalibrationOffset[ch];
    }

    // --------------------------------------------------------------------

    bool RampSingleChannelVoltage(uint16_t ch, float volt)
    {
        if (!CheckChannel(ch))
            return false;

        if (!CheckChannelVoltage(ch, volt))
            return false;

        const uint16_t dac = ConvertVoltToDac(ch, volt);
        return RampSingleChannelDac(ch, dac);
    }

    bool RampAllChannelsVoltage(const vector<float> &volt)
    {
        vector<uint16_t> dac(kNumChannels);
        for (size_t ch=0; ch<kNumChannels; ch++)
        {
            if (!CheckChannelVoltage(ch, volt[ch]))
                return false;

            dac[ch] = ConvertVoltToDac(ch, volt[ch]);
        }

        return RampAllChannelsDac(dac);
    }

    bool RampAllVoltages(float volt)
    {
        return RampAllChannelsVoltage(vector<float>(kNumChannels, volt));
    }

    // --------------------------------------------------------------------

    /*
    bool RampSingleChannelOffset(uint16_t ch, float offset, bool relative)
    {
        if (!CheckChannel(ch))
            return false;

//        if (relative)
//            offset += fDacActual[ch]*90./4096 - fBreakdownVoltage[ch];

        const float volt = fBreakdownVoltage[ch]>0 ? fBreakdownVoltage[ch] + offset : 0;

        if (!RampSingleChannelVoltage(ch, volt))
            return false;

        fChannelOffset[ch] = offset;

        return true;
    }

    bool RampAllChannelsOffset(vector<float> offset, bool relative)
    {
        vector<float> volt(kNumChannels);

//        if (relative)
//            for (size_t ch=0; ch<kNumChannels; ch++)
//                offset[ch] += fDacActual[ch]*90./4096 - fBreakdownVoltage[ch];

        for (size_t ch=0; ch<kNumChannels; ch++)
            volt[ch] = fBreakdownVoltage[ch]>0 ? fBreakdownVoltage[ch] + offset[ch] : 0;

        if (!RampAllChannelsVoltage(volt))
            return false;

        fChannelOffset = offset;

        return true;
    }

    bool RampAllOffsets(float offset, bool relative)
    {
        return RampAllChannelsOffset(vector<float>(kNumChannels, offset), relative);
    }
    */

    /*
    bool RampSingleChannelOvervoltage(float offset)
    {
        return RampAllChannelsOvervoltage(vector<float>(kNumChannels, offset));
    }
    bool RampAllOvervoltages(const vector<float> &overvoltage)
    {
        vector<float> volt(kNumChannels);

        for (size_t ch=0; ch<kNumChannels; ch++)
            volt[ch] = fBreakdownVoltage[ch] + fOvervoltage[ch] + fChannelOffset[ch];

#warning What about empty channels?

        if (!RampAllChannelsVoltage(volt))
            return false;

        for (size_t ch=0; ch<kNumChannels; ch++)
            fOvervoltage[ch] = overvoltage[ch];

        return true;
    }*/

    // --------------------------------------------------------------------

    void OverCurrentReset()
    {
        if (fWaitingForAnswer>=0)
        {
            ostringstream msg;
            msg << "OverCurrentReset - Answer on last command (id=" << fWaitingForAnswer << ") not yet received.";
            Error(msg);
            return;
        }

        if (fIsRamping)
        {
            Warn("OverCurrentReset - Ramping in progres.");
            RampStop();
        }

        vector<uint16_t> dac(fDacActual);

        for (int ch=0; ch<kNumChannels; ch++)
            if (fCurrent[ch]<0)
                dac[ch] = 0;

        SetAllChannels(dac, true);
    }

    void ReadAllChannels(bool special = false)
    {
        if (!special && fWaitingForAnswer>=0)
        {
            ostringstream msg;
            msg << "ReadAllChannels - Answer on last command (id=" << fWaitingForAnswer << ") not yet received.";
            Error(msg);
            return;
        }

        vector<char> data;
        data.reserve(kNumChannels*3);

        for (int ch=0; ch<kNumChannels; ch++)
        {
            const vector<char> cmd = GetCmd(kCmdRead, ch);
            data.insert(data.end(), cmd.begin(), cmd.end());
        }

        fSendCounter += kNumChannels;

        PostMessage(data);
        AsyncRead(ba::buffer(special ? fBufferUpdate : fBuffer, kNumChannels*3),
                  special ? kUpdate : kCmdRead, fSendCounter);

        if (!special)
            fWaitingForAnswer = kCmdRead;
    }

    bool SetReferences(const vector<float> &volt, const vector<float> &offset, const vector<float> &slope)
    {
        if (volt.size()!=kNumChannels)
        {
            ostringstream out;
            out << "SetReferences - Given vector has " << volt.size() << " elements - expected " << kNumChannels << endl;
            Error(out);
            return false;
        }
        if (offset.size()!=kNumChannels)
        {
            ostringstream out;
            out << "SetReferences - Given vector has " << offset.size() << " elements - expected " << kNumChannels << endl;
            Error(out);
            return false;
        }
        if (slope.size()!=kNumChannels)
        {
            ostringstream out;
            out << "SetReferences - Given vector has " << slope.size() << " elements - expected " << kNumChannels << endl;
            Error(out);
            return false;
        }

        fOperationVoltage  = volt;
        fCalibrationOffset = offset;
        fCalibrationSlope  = slope;

        UpdateVgapd();

        return true;
    }

    // --------------------------------------------------------------------

    void RampStop()
    {
        fRampTimer.cancel();
        fIsRamping = false;

        Message("Ramping stopped.");
    }

    void RampStart()
    {
        if (fIsRamping)
        {
            Warn("RampStart - Ramping already in progress... ignored.");
            return;
        }

        fIsRamping = RampOneStep();
    }

    void SetRampTime(uint16_t val)
    {
        fRampTime = val;
    }

    void SetRampStep(uint16_t val)
    {
        fRampStep = val;
    }

    uint16_t GetRampStepVolt() const
    {
        return fRampStep*90./4096;
    }

    bool IsRamping() const { return fIsRamping; }

    // -------------------------------------------------------------------

    void ExpertReset(bool expert_mode=true)
    {
        if (expert_mode && fWaitingForAnswer>=0)
        {
            ostringstream msg;
            msg << "ExpertReset - Answer on last command (id=" << fWaitingForAnswer << ") not yet received.";
            Error(msg);
            return;
        }

        if (expert_mode)
            Warn("EXPERT MODE: Sending reset.");

        PostMessage(GetCmd(kCmdReset));
        AsyncRead(ba::buffer(fBuffer, 3), kCmdReset, ++fSendCounter);
        fWaitingForAnswer = kCmdReset;
    }


    bool ExpertChannelSetDac(uint16_t ch, uint16_t dac)
    {
        if (fWaitingForAnswer>=0)
        {
            ostringstream msg;
            msg << "ExpertChannelSetDac - Answer on last command (id=" << fWaitingForAnswer << ") not yet received.";
            Error(msg);
            return false;
        }

        if (!CheckDac(dac))
            return false;

        fDacCommand[ch] = dac;

        ostringstream msg;
        msg << "EXPERT MODE: Sending 'ChannelSet' (set ch " << ch << " to DAC=" << dac << ")";
        Warn(msg);

        // FIXME: dac += calib_offset
        PostMessage(GetCmd(kCmdChannelSet, ch, dac));
        AsyncRead(ba::buffer(fBuffer, 3), kExpertChannelSet|(ch<<8), ++fSendCounter);
        fWaitingForAnswer = kExpertChannelSet|(ch<<8);

        return true;
    }

    bool ExpertChannelSetVolt(uint16_t ch, double volt)
    {
        return ExpertChannelSetDac(ch, volt*4096/90.);
    }

    bool ExpertGlobalSetDac(uint16_t dac)
    {
        if (fWaitingForAnswer>=0)
        {
            ostringstream msg;
            msg << "ExpertGlobalSetDac - Answer on last command (id=" << fWaitingForAnswer << ") not yet received.";
            Error(msg);
            return false;
        }

        if (!CheckDac(dac))
            return false;

        if (fGlobalDacCmd>=0)
        {
            Error("ExpertGlobalSetDac - Still waiting for previous answer to 'GlobalSet'");
            return false;
        }

        fGlobalDacCmd = dac;

        ostringstream msg;
        msg << "EXPERT MODE: Sending 'GlobalSet' (DAC=" << dac << ")";
        Warn(msg);

        PostMessage(GetCmd(kCmdGlobalSet, 0, dac));
        AsyncRead(ba::buffer(fBuffer, 3), kCmdGlobalSet, ++fSendCounter);
        fWaitingForAnswer = kCmdGlobalSet;

        return true;
    }

    bool ExpertGlobalSetVolt(float volt)
    {
        return ExpertGlobalSetDac(volt*4096/90);
    }

    // --------------------------------------------------------------------

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetDummyMode(bool b)
    {
        fIsDummyMode = b;
    }

    void PrintInfo()
    {
        Out() << endl << kBold << dec << '\n';
        Out() << "fWrapCounter    = " << fWrapCounter << '\n';
        Out() << "fSendCounter    = " << fSendCounter%8 << " (" << fSendCounter << ")" << '\n';
        Out() << "fIsInitializing = " << fIsInitializing << '\n';
        Out() << "fIsRamping      = " << fIsRamping << '\n';
        Out() << "Answer counter:" << '\n';
        Out() << " - Synchronization: " << fCounter[0] << '\n';
        Out() << " - Reset:           " << fCounter[1] << '\n';
        Out() << " - Request update:  " << fCounter[2] << '\n';
        Out() << " - Ramp step:       " << fCounter[3] << '\n';
        Out() << " - Read:            " << fCounter[4] << '\n';
        Out() << " - Reset channels:  " << fCounter[5] << '\n';
        Out() << " - Global set:      " << fCounter[7] << '\n';
        Out() << " - Channel set:     " << fCounter[6] << '\n' << endl;
    }

    void PrintLineA(int b, int ch)
    {
        Out() << setw(2) << b << "|";

        for (int c=ch; c<ch+8; c++)
        {
            const int id = c+kNumChannelsPerBoard*b;
            Out() << (fCurrent[id]<0?kRed:kGreen);
            Out() << " " << setw(7) << abs(fCurrent[id])*5000/4096.;
        }
        Out() << endl;

    }

    void PrintA()
    {
        Out() << dec << setprecision(2) << fixed << setfill(' ');
        for (int b=0; b<kNumBoards; b++)
        {
            if (!fPresent[b])
            {
                Out() << setw(2) << b << "-" << endl;
                continue;
            }

            PrintLineA(b,  0);
            PrintLineA(b,  8);
            PrintLineA(b, 16);
            PrintLineA(b, 24);
        }
    }

    void PrintLineV(int b, int ch)
    {
        Out() << setw(2) << b << "|";

        for (int c=ch; c<ch+4; c++)
        {
            const int id = c+kNumChannelsPerBoard*b;
            Out() << " ";
            Out() << (fDacActual[id]==fDacTarget[id]?kGreen:kRed);
            //Out() << setw(5) << fDacActual[id]*90/4096. << '/';
            //Out() << setw(5) << fDacTarget[id]*90/4096.;

            Out() << setw(5) << ConvertDacToVolt(id, fDacActual[id]) << '/';
            Out() << setw(5) << ConvertDacToVolt(id, fDacTarget[id]);
        }
        Out() << endl;
    }

    void PrintV()
    {
        Out() << dec << setprecision(2) << fixed << setfill(' ');
        for (int b=0; b<kNumBoards; b++)
        {
            if (!fPresent[b])
            {
                Out() << setw(2) << b << "-" << endl;
                continue;
            }

            PrintLineV(b,  0);
            PrintLineV(b,  4);
            PrintLineV(b,  8);
            PrintLineV(b, 12);
            PrintLineV(b, 16);
            PrintLineV(b, 20);
            PrintLineV(b, 24);
            PrintLineV(b, 28);
        }
    }

    void PrintLineGapd(int b, int ch)
    {
        Out() << setw(2) << b << "|";

        for (int c=ch; c<ch+8; c++)
        {
            const int id = c+kNumChannelsPerBoard*b;
            Out() << " " << setw(5) << fOperationVoltage[id];
        }
        Out() << endl;
    }

    void PrintReferenceVoltage()
    {
        Out() << dec << setprecision(2) << fixed << setfill(' ');
        for (int b=0; b<kNumBoards; b++)
        {
            if (!fPresent[b])
            {
                Out() << setw(2) << b << "-" << endl;
                continue;
            }

            PrintLineGapd(b,  0);
            PrintLineGapd(b,  8);
            PrintLineGapd(b, 16);
            PrintLineGapd(b, 24);
        }
    }

    // -------------------------------------------------------------------

    void SetUpdateInterval(uint32_t val)
    {
        fUpdateTime = val;

        if (!IsConnected() || fIsInitializing)
            return;

        fUpdateTimer.cancel();

        if (fUpdateTime>0)
            ScheduleUpdate(fUpdateTime);
    }

    void SetSyncDelay(uint16_t val)
    {
        fSyncTime = val;
    }

    void SetVoltMaxAbs(float max)
    {
        if (max>90)
            max = 90;
        if (max<0)
            max = 0;

        fVoltageMaxAbs = max;
    }

    void SetVoltMaxRel(float max)
    {
        if (max>90)
            max = 90;
        if (max<0)
            max = 0;

        fVoltageMaxRel = max;
    }

    uint16_t GetVoltMaxAbs() const
    {
        return fVoltageMaxAbs;
    }

    uint16_t GetVoltMaxRel() const
    {
        return fVoltageMaxRel;
    }

    bool HasOvercurrent() const
    {
        for (int ch=0; ch<kNumChannels; ch++)
            if (fPresent[ch/kNumChannelsPerBoard] && fCurrent[ch]<0 && ch!=91)
                return true;

        return false;
    }

    bool IsVoltageOff() const
    {
        for (int ch=0; ch<kNumChannels; ch++)
            if (fPresent[ch/kNumChannelsPerBoard] && fDacActual[ch]!=0)
                return false;

        return true;
    }

    State::states_t GetStatus()
    {
        if (!IsConnected())
            return State::kDisconnected;

        if (IsConnecting())
            return State::kConnecting;

        if (fIsInitializing)
            return State::kInitializing;

        if (fIsRamping)
            return State::kRamping;

        if (HasOvercurrent())
            return State::kOverCurrent;

        if (IsVoltageOff())
            return State::kVoltageOff;

        for (int ch=0; ch<kNumChannels; ch++)
            if (fPresent[ch/kNumChannelsPerBoard] && fDacActual[ch]!=fDacTarget[ch])
                return State::kNotReferenced;

        return State::kVoltageOn;
    }

    void SetReconnectDelay(uint32_t delay=1)
    {
        fReconnectDelay = delay;
    }

    void SetEmergencyLimit(int32_t limit=0)
    {
        fEmergencyLimit = limit;
    }

    void ResetEmergencyShutdown()
    {
        fEmergencyShutdown = false;
    }

    bool IsEmergencyShutdown() const
    {
        return fEmergencyShutdown;
    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimBias : public ConnectionBias
{
private:

    DimDescribedService fDimCurrent;
    DimDescribedService fDimDac;
    DimDescribedService fDimVolt;
    DimDescribedService fDimGapd;

public:
    void UpdateVA()
    {
        const Time now;

        UpdateV(now);

        fDimCurrent.setTime(now);
        fDimCurrent.Update(fCurrent);
    }

private:
    void UpdateV(const Time now=Time())
    {
        const bool rc = !memcmp(fDacActual.data(), fDacTarget.data(), kNumChannels*2);

        vector<uint16_t> val(2*kNumChannels);
        memcpy(val.data(),              fDacActual.data(), kNumChannels*2);
        memcpy(val.data()+kNumChannels, fDacTarget.data(), kNumChannels*2);
        fDimDac.setTime(now);
        fDimDac.setQuality(rc);
        fDimDac.Update(val);

        vector<float> volt(kNumChannels);
        for (float ch=0; ch<kNumChannels; ch++)
            volt[ch] = ConvertDacToVolt(ch, fDacActual[ch]);
        fDimVolt.setTime(now);
        fDimVolt.setQuality(rc);
        fDimVolt.Update(volt);
    }

    void UpdateVgapd()
    {
        vector<float> volt;
        volt.reserve(3*kNumChannels);
        volt.insert(volt.end(), fOperationVoltage.begin(),   fOperationVoltage.end());
        volt.insert(volt.end(), fCalibrationOffset.begin(),  fCalibrationOffset.end());
        volt.insert(volt.end(), fCalibrationSlope.begin(),   fCalibrationSlope.end());
        fDimGapd.Update(volt);
    }

public:
    ConnectionDimBias(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionBias(ioservice, imp),
        fDimCurrent("BIAS_CONTROL/CURRENT", "S:416",
                    "|I[dac]:Bias current (conversion: 5000uA/4096dac)"),
        fDimDac("BIAS_CONTROL/DAC", "S:416;S:416",
                "|U[dac]:Current dac setting"
                "|Uref[dac]:Reference dac setting"),
        fDimVolt("BIAS_CONTROL/VOLTAGE", "F:416",
                 "|Uout[V]:Output voltage"),
        fDimGapd("BIAS_CONTROL/NOMINAL", "F:416;F:416;F:416",
                 "|Uop[V]:Nominal operation voltage at 25deg C"
                 "|Uoff[V]:Bias crate channel calibration offsets"
                 "|Rcal[Ohm]:Bias crate channel calibration slope")
    {
    }

    // A B [C] [D] E [F] G H [I] J K [L] M N O P Q R [S] T U V W [X] Y Z
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineBias : public StateMachineAsio<T>
{
    int Wrap(boost::function<void()> f)
    {
        f();
        return T::GetCurrentState();
    }

    function<int(const EventImp &)> Wrapper(function<void()> func)
    {
        return bind(&StateMachineBias::Wrap, this, func);
    }

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

private:
    S fBias;

    bool fExpertMode;

    Time fSunRise;

    // --------------------------------------------------------------------

    // SET_GLOBAL_DAC_VALUE
    int SetGlobalDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetGlobalDac", 2))
            return false;

        fBias.RampAllDacs(evt.GetUShort());

        return T::GetCurrentState();
    }

    // SET_ALL_CHANNELS_DAC
    int SetAllChannelsDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetAllChannelsDac", 2*416))
            return false;

        const uint16_t *ptr = evt.Ptr<uint16_t>();

        fBias.RampAllChannelsDac(vector<uint16_t>(ptr, ptr+416));

        return T::GetCurrentState();
    }

    // SET_CHANNEL_DAC_VALUE
    int SetChannelDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetChannelDac", 4))
            return false;

        fBias.RampSingleChannelDac(evt.Get<uint16_t>(), evt.Get<uint16_t>(2));

        return T::GetCurrentState();
    }

    // --------------------------------------------------------------------

    // SET_CHANNEL_VOLTAGE
    int SetChannelVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetChannelVolt", 6))
            return false;

        fBias.RampSingleChannelVoltage(evt.GetUShort(), evt.Get<float>(2));

        return T::GetCurrentState();
    }

    // SET_GLOBAL_VOLTAGE
    int SetGlobalVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetGlobalVolt", 4))
            return false;

        fBias.RampAllVoltages(evt.GetFloat());

        return T::GetCurrentState();
    }

    // SET_ALL_CHANNELS_VOLTAGES
    int SetAllChannelsVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetAllChannelsVolt", 4*kNumChannels))
            return false;

        const float *ptr = evt.Ptr<float>();
        fBias.RampAllChannelsVoltage(vector<float>(ptr, ptr+kNumChannels));

        return T::GetCurrentState();
    }

    // --------------------------------------------------------------------

/*    // INCREASE_GLOBAL_VOLTAGE
    int IncGlobalVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "IncGlobalVolt", 4))
            return false;

        fBias.RampAllOffsets(evt.GetFloat(), true);

        return T::GetCurrentState();
    }

    // INCREASE_CHANNEL_VOLTAGE
    int IncChannelVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "IncChannelVolt", 6))
            return false;

        fBias.RampSingleChannelOffset(evt.Get<uint16_t>(), evt.Get<float>(2), true);

        return T::GetCurrentState();
    }

    // INCREASE_ALL_CHANNELS_VOLTAGES
    int IncAllChannelsVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "IncAllChannelsVolt", 4*kNumChannels))
            return false;

        const float *ptr = evt.Ptr<float>();
        fBias.RampAllChannelsOffset(vector<float>(ptr, ptr+416), true);

        return T::GetCurrentState();
    }
*/
    // --------------------------------------------------------------------

    int ExpertSetGlobalVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ExpertSetGlobalVolt", 4))
            return false;

        fBias.ExpertGlobalSetVolt(evt.GetFloat());

        return T::GetCurrentState();
    }

    int ExpertSetGlobalDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ExpertSetGlobalDac", 2))
            return false;

        fBias.ExpertGlobalSetDac(evt.GetUShort());

        return T::GetCurrentState();
    }

    int ExpertSetChannelVolt(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ExpertSetChannelVolt", 6))
            return false;

        fBias.ExpertChannelSetVolt(evt.GetUShort(), evt.Get<float>(2));

        return T::GetCurrentState();
    }

    int ExpertSetChannelDac(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "ExpertSetChannelDac", 4))
            return false;

        fBias.ExpertChannelSetDac(evt.Get<uint16_t>(), evt.Get<uint16_t>(2));

        return T::GetCurrentState();
    }

    int ExpertLoadMapFile(const EventImp &evt)
    {
        if (evt.GetSize()==0)
        {
            T::Warn("ExpertLoadMapFile - No file name given.");
            return T::GetCurrentState();
        }

        if (fBias.GetStatus()!=State::kVoltageOff)
        {
            T::Warn("ExpertLoadMapFile - Voltage must have been turned off.");
            return T::GetCurrentState();
        }

        BiasMap map;

        try
        {
            map.Read(evt.GetText());
        }
        catch (const runtime_error &e)
        {
            T::Warn("Getting reference voltages failed: "+string(e.what()));
            return T::GetCurrentState();
        }

        if (!fBias.SetReferences(map.Vgapd(), map.Voffset(), map.Vslope()))
        {
            T::Warn("Setting reference voltages failed.");
            return T::GetCurrentState();
        }

        fBias.UpdateVA();

        T::Info("Successfully loaded new mapping '"+evt.GetString()+"'");

        return T::GetCurrentState();
    }

    // --------------------------------------------------------------------

    int SetUpdateInterval(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetUpdateInterval", 4))
            return false;

        fBias.SetUpdateInterval(evt.Get<int32_t>()<0 ? 0 : evt.Get<uint32_t>());

        return T::GetCurrentState();
    }

    int Disconnect()
    {
        // Close all connections
        fBias.PostClose(-1);

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
        fBias.PostClose(-1);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fBias.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fBias.SetReconnectDelay();
        fBias.PostClose(0);

        return T::GetCurrentState();
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fBias.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDummyMode(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDummyMode", 1))
            return T::kSM_FatalError;

        fBias.SetDummyMode(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetExpertMode(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetExpertMode", 1))
            return T::kSM_FatalError;

        fExpertMode = evt.GetBool();

        if (fExpertMode)
            T::Warn("Expert commands enabled -- please ensure that you EXACTLY know what you do. These commands can destroy the system.");

        return T::GetCurrentState();
    }

    int Shutdown(const string &reason)
    {
        fBias.RampAllDacs(0);
        T::Info("Emergency shutdown initiated ["+reason+"].");
        return State::kLocked;
    }

    int Unlock()
    {
        fBias.ResetEmergencyShutdown();
        return fBias.GetStatus();
    }

    int Execute()
    {
        const int state = fBias.GetStatus();

        if (fBias.IsEmergencyShutdown()/* && state>State::kInitializing && state<State::kExpertMode*/)
        {
            // This needs to be repeated for the case that in between a different command was processed
            if (!fBias.IsVoltageOff())
                fBias.RampAllDacs(0);

            return State::kLocked;
        }

        const Time now;
        if (now>fSunRise)
        {
            const bool shutdown =
                state==State::kRamping       ||
                state==State::kVoltageOn     ||
                state==State::kNotReferenced ||
                state==State::kOverCurrent;

            if (shutdown)
                Shutdown("beginning of civil twilight");

            fSunRise = now.GetNextSunRise(-6);

            ostringstream msg;
            msg << "During next sun-rise nautical twilight will end at " << fSunRise;
            T::Info(msg);

            if (shutdown)
                return State::kLocked;
        }

        if (T::GetCurrentState()==State::kLocked)
            return T::GetCurrentState();

        if (fExpertMode && state>=State::kConnected)
            return State::kExpertMode;

        return state;
    }

public:
    StateMachineBias(ostream &out=cout) :
        StateMachineAsio<T>(out, "BIAS_CONTROL"), fBias(*this, *this),
        fExpertMode(false), fSunRise(Time().GetNextSunRise(-6))
    {
        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                        "Bias-power supply not connected via USB.");

        T::AddStateName(State::kConnecting, "Connecting",
                        "Trying to establish USB connection to bias-power supply.");

        T::AddStateName(State::kInitializing, "Initializing",
                        "USB connection to bias-power supply established, synchronizing USB stream.");

        T::AddStateName(State::kConnected, "Connected",
                        "USB connection to bias-power supply established.");

        T::AddStateName(State::kNotReferenced, "NotReferenced",
                        "Internal reference voltage does not match last sent voltage.");

        T::AddStateName(State::kVoltageOff, "VoltageOff",
                        "All voltages are supposed to be switched off.");

        T::AddStateName(State::kVoltageOn, "VoltageOn",
                        "At least one voltage is switched on and all are at reference.");

        T::AddStateName(State::kOverCurrent, "OverCurrent",
                        "At least one channel is in over current state.");

        T::AddStateName(State::kExpertMode, "ExpertMode",
                        "Special (risky!) mode to directly send command to the bias-power supply.");

        T::AddStateName(State::kRamping, "Ramping",
                        "Voltage ramping in progress.");

        T::AddStateName(State::kLocked, "Locked",
                        "Locked due to emergency shutdown, no commands accepted except UNLOCK.");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineBias::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("ENABLE_DUMMY_MODE", "B:1")
            (bind(&StateMachineBias::SetDummyMode, this, placeholders::_1))
            ("Enable dummy mode. In this mode SetAllChannels prints informations instead of sending anything to the bias crate."
             "|enable[bool]:disable or enable dummy mode");

        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected, State::kVoltageOff)
            (bind(&StateMachineBias::Disconnect, this))
            ("disconnect from USB");
        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected, State::kVoltageOff)
            (bind(&StateMachineBias::Reconnect, this, placeholders::_1))
            ("(Re)connect USB connection to Bias power supply, a new address can be given"
             "|tty[string]:new USB address");


        T::AddEvent("SET_UPDATE_INTERVAL", "I:1")
            (bind(&StateMachineBias::SetUpdateInterval, this, placeholders::_1))
            ("Set the updat einterval how often the currents are requested"
             "|interval[ms]:Update interval in milliseconds");



        T::AddEvent("REQUEST_STATUS", State::kConnected, State::kVoltageOn, State::kVoltageOff, State::kNotReferenced, State::kOverCurrent)
            (Wrapper(bind(&ConnectionBias::ReadAllChannels, &fBias, false)))
            ("Asynchronously request the status (current) of all channels.");

        T::AddEvent("RESET_OVER_CURRENT_STATUS", State::kOverCurrent)
            (Wrapper(bind(&ConnectionBias::OverCurrentReset, &fBias)))
            ("Set all channels in over current state to 0V and send a system reset to reset the over current flags.");


        T::AddEvent("SET_CHANNEL_DAC", "S:1;S:1")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetChannelDac, this, placeholders::_1))
            ("Set a new target value in DAC counts for a single channel. Starts ramping if necessary."
             "|channel[short]:Channel for which to set the target voltage [0-415]"
             "|voltage[dac]:Target voltage in DAC units for the given channel");
        T::AddEvent("SET_GLOBAL_DAC", "S:1")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetGlobalDac, this, placeholders::_1))
            ("Set a new target value for all channels in DAC counts. Starts ramping if necessary. (This command is not realized with the GLOBAL SET command.)"
             "|voltage[dac]:Global target voltage in DAC counts.");
        T::AddEvent("SET_ALL_CHANNELS_DAC", "S:416")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetAllChannelsDac, this, placeholders::_1))
            ("Set a new target value for all channels in DAC counts. Starts ramping if necessary."
             "|voltage[dac]:Global target voltage in DAC counts for all channels");


        T::AddEvent("SET_CHANNEL_VOLTAGE", "S:1;F:1")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetChannelVolt, this, placeholders::_1))
            ("Set a new target voltage for a single channel. Starts ramping if necessary."
             "|channel[short]:Channel for which to set the target voltage [0-415]"
             "|voltage[V]:Target voltage in volts for the given channel (will be converted to DAC units)");
        T::AddEvent("SET_GLOBAL_VOLTAGE", "F:1")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetGlobalVolt, this, placeholders::_1))
            ("Set a new target voltage for all channels. Starts ramping if necessary. (This command is not realized with the GLOBAL SET command.)"
             "|voltage[V]:Global target voltage in volts (will be converted to DAC units)");
        T::AddEvent("SET_ALL_CHANNELS_VOLTAGE", "F:416")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::SetAllChannelsVolt, this, placeholders::_1))
            ("Set all channels to the given new reference voltage. Starts ramping if necessary."
             "|voltage[V]:New reference voltage for all channels");

/*
        T::AddEvent("INCREASE_CHANNEL_VOLTAGE", "S:1;F:1", State::kConnected, State::kVoltageOff, State::kVoltageOn, State::kNotReferenced, State::kOverCurrent)
            (bind(&StateMachineBias::IncChannelVolt, this, placeholders::_1))
            ("Increases the voltage of all channels by the given offset. Starts ramping if necessary. (This command is not realized with the GLOBAL SET command.)"
             "|channel[short]:Channel for which to adapt the voltage [0-415]"
             "|offset[V]:Offset to be added to all channels (will be converted to DAC counts)");
        T::AddEvent("INCREASE_GLOBAL_VOLTAGE", "F:1", State::kConnected, State::kVoltageOff, State::kVoltageOn, State::kNotReferenced, State::kOverCurrent)
            (bind(&StateMachineBias::IncGlobalVolt, this, placeholders::_1))
            ("Increases the voltage of all channels by the given offset. Starts ramping if necessary. (This command is not realized with the GLOBAL SET command.)"
             "|offset[V]:Offset to be added to all channels (will be converted to DAC counts)");
        T::AddEvent("INCREASE_ALL_CHANNELS_VOLTAGE", "F:416", State::kConnected, State::kVoltageOff, State::kVoltageOn, State::kNotReferenced, State::kOverCurrent)
            (bind(&StateMachineBias::IncAllChannelsVolt, this, placeholders::_1))
            ("Add the given voltages to the current reference voltages. Starts ramping if necessary."
             "offset[V]:Offsets to be added to the reference voltage of all channels in volts");
*/



        T::AddEvent("SET_ZERO_VOLTAGE")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (Wrapper(bind(&ConnectionBias::RampAllDacs, &fBias, 0)))
            ("Set all channels to a zero reference voltage. Starts ramping if necessary.");
        T::AddEvent("SHUTDOWN")(State::kConnected)(State::kVoltageOff)(State::kVoltageOn)(State::kNotReferenced)(State::kOverCurrent)(State::kRamping)
            (bind(&StateMachineBias::Shutdown, this, "user request"))
            ("Same as SET_ZERO_VOLTAGE; but goes to locked state afterwards.");

        T::AddEvent("UNLOCK", State::kLocked)
            (bind(&StateMachineBias::Unlock, this))
            ("Unlock if in locked state.");





        T::AddEvent("STOP", State::kConnected, State::kRamping)
            (Wrapper(bind(&ConnectionBias::RampStop, &fBias)))
            ("Stop an on-going ramping");

        T::AddEvent("START", State::kConnected, State::kNotReferenced)
            (Wrapper(bind(&ConnectionBias::RampStart, &fBias)))
            ("Start a ramping if no ramping is in progress and if reference values differ from current voltages");



        T::AddEvent("PRINT_INFO")
            (Wrapper(bind(&ConnectionBias::PrintInfo, &fBias)))
            ("Print a table with all current read back with the last request operation");
        T::AddEvent("PRINT_CURRENTS")
            (Wrapper(bind(&ConnectionBias::PrintA, &fBias)))
            ("Print a table with all current read back with the last request operation");
        T::AddEvent("PRINT_VOLTAGES")
            (Wrapper(bind(&ConnectionBias::PrintV, &fBias)))
            ("Print a table with all voltages (current and reference voltages as currently in memory)");
        T::AddEvent("PRINT_GAPD_REFERENCE_VOLTAGES")
            (Wrapper(bind(&ConnectionBias::PrintReferenceVoltage, &fBias)))
            ("Print the G-APD reference values (breakdown voltage + overvoltage) obtained from file");


        T::AddEvent("EXPERT_MODE", "B:1")
            (bind(&StateMachineBias::SetExpertMode, this, placeholders::_1))
            ("Enable usage of expert commands (note that for safty reasons the are exclusive with the standard commands)");

        T::AddEvent("EXPERT_RESET", State::kExpertMode)
            (Wrapper(bind(&ConnectionBias::ExpertReset, &fBias, true)))
            ("Send the RESET command (note that this is possibly harmfull command)");

        T::AddEvent("EXPERT_SET_GLOBAL_VOLTAGE", "F:1", State::kExpertMode)
            (bind(&StateMachineBias::ExpertSetGlobalVolt, this, placeholders::_1))
            ("Send the global set command. The given voltage is converted to DAC counts.");

        T::AddEvent("EXPERT_SET_GLOBAL_DAC", "S:1", State::kExpertMode)
            (bind(&StateMachineBias::ExpertSetGlobalDac, this, placeholders::_1))
            ("Send the global set command.");

        T::AddEvent("EXPERT_SET_CHANNEL_VOLTAGE", "S:1;F:1", State::kExpertMode)
            (bind(&StateMachineBias::ExpertSetChannelVolt, this, placeholders::_1))
            ("Send a single channel set command. The given voltage is converted to DAC commands.");

        T::AddEvent("EXPERT_SET_CHANNEL_DAC", "S:1;S:1", State::kExpertMode)
            (bind(&StateMachineBias::ExpertSetChannelDac, this, placeholders::_1))
            ("Send a single channel set command.");

        T::AddEvent("EXPERT_LOAD_MAP_FILE", "C", State::kExpertMode)
            (bind(&StateMachineBias::ExpertLoadMapFile, this, placeholders::_1))
            ("Load a new mapping file.");
    }

    ~StateMachineBias() { T::Warn("TODO: Implement rampming at shutdown!"); }

    int EvalOptions(Configuration &conf)
    {
        // FIXME: Read calib_offset
        // FIXME: Check calib offset being smaller than +/-0.25V

        fBias.SetVerbose(!conf.Get<bool>("quiet"));
        fBias.SetDummyMode(conf.Get<bool>("dummy-mode"));

        if (conf.Has("dev"))
        {
            fBias.SetEndpoint(conf.Get<string>("dev"));
            T::Message("Setting device to "+fBias.URL());
        }

        const uint16_t step = conf.Get<uint16_t>("ramp-step");
        const uint16_t time = conf.Get<uint16_t>("ramp-delay");

        if (step>230) // 5V
        {
            T::Error("ramp-step exceeds allowed range.");
            return 1;
        }

        fBias.SetRampStep(step);
        fBias.SetRampTime(time);
        fBias.SetUpdateInterval(conf.Get<uint32_t>("update-interval"));
        fBias.SetEmergencyLimit(conf.Get<uint16_t>("emergency-limit"));
        fBias.SetSyncDelay(conf.Get<uint16_t>("sync-delay"));

        ostringstream str1, str2;
        str1 << "Ramping in effective steps of " << fBias.GetRampStepVolt() << "V";
        str2 << "Ramping with a delay per step of " << time << "ms";
        T::Message(str1);
        T::Message(str2);

        // --------------------------------------------------------------------------

        const float maxabsv = conf.Get<float>("volt-max-abs");
        const float maxrelv = conf.Get<float>("volt-max-rel");
        if (maxabsv>90)
        {
            T::Error("volt-max exceeds 90V.");
            return 2;
        }
        if (maxabsv>75)
            T::Warn("volt-max exceeds 75V.");
        if (maxabsv<70)
            T::Warn("volt-max below 70V.");
        if (maxabsv<0)
        {
            T::Error("volt-max negative.");
            return 3;
        }

        fBias.SetVoltMaxAbs(maxabsv);
        fBias.SetVoltMaxRel(maxrelv);

        ostringstream str3, str4;
        str3 << "Effective maximum allowed absolute voltage: " << fBias.GetVoltMaxAbs() << "V";
        str4 << "Effective maximum difference w.r.t to G-APD reference: " << fBias.GetVoltMaxRel() << "V";
        T::Message(str3);
        T::Message(str4);

        // --------------------------------------------------------------------------

        BiasMap map;

        if (!conf.Has("bias-map-file") && !conf.Has("bias-database"))
        {
            T::Error("Neither bias-map-file not bias-database specified.");
            return 5;
        }

        try
        {
            if (conf.Has("bias-map-file"))
                map.Read(conf.GetPrefixedString("bias-map-file"));

            //if (conf.Has("bias-database"))
            //    map.Retrieve(conf.Get<string>("bias-database"));
        }
        catch (const runtime_error &e)
        {
            T::Error("Getting reference voltages failed: "+string(e.what()));
            return 7;
        }

        if (!fBias.SetReferences(map.Vgapd(), map.Voffset(), map.Vslope()))
        {
            T::Error("Setting reference voltages failed.");
            return 8;
        }

        // --------------------------------------------------------------------------

        if (conf.Has("dev"))
            fBias.Connect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineBias<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("BIAS control options");
    control.add_options()
        ("no-dim,d",        po_bool(),  "Disable dim services")
        ("dev",             var<string>(),       "Device address of USB port to bias-power supply")
        ("quiet,q",         po_bool(true),       "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("dummy-mode",      po_bool(),           "Dummy mode - SetAllChannels prints info instead of sending new values.")
        ("ramp-delay",      var<uint16_t>(15),   "Delay between the answer of one ramping step and sending the next ramp command to all channels in milliseconds.")
        ("ramp-step",       var<uint16_t>(46),   "Maximum step in DAC counts during ramping (Volt = DAC*90/4096)")
        ("update-interval", var<uint32_t>(3000), "Interval between two current requests in milliseconds")
        ("sync-delay",      var<uint16_t>(500),  "Delay between sending the inital 0's after a newly established connection to synchronize the output stream in milliseconds")
        ("volt-max-abs",    var<float>(75),      "Absolte upper limit for the voltage (in Volts)")
        ("volt-max-rel",    var<float>(3.5),     "Relative upper limit for the voltage w.r.t. the G-APD reference voltage (in Volts)")
        ("bias-map-file",   var<string>(),       "File with nominal and offset voltages for each channel.")
        ("bias-database",   var<string>(),       "")
        ("emergency-limit", var<uint16_t>(2200), "A current limit in ADC counts which, if exceeded, will initiate an emergency shutdown (0=off)")
        ;

    conf.AddOptions(control);
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
        "The biasctrl program controls the bias-power supply boards.\n"
        "\n"
        "Note: At default the program is started without a command line (user) "
        "interface. In this case Actions/Commands are available via Dim "
        "exclusively.\n"
        "Use the -c option to start the program with a command line interface.\n"
        "\n"
        "In the running application:\n"
        "Use h or help to print a short help message about its usage.\n"
        "\n"
        "Usage: biasctrl [-c type] [OPTIONS]\n"
        "  or:  biasctrl [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineBias<StateMachine,ConnectionBias>>();

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
                return RunShell<LocalStream, StateMachine, ConnectionBias>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimBias>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionBias>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionBias>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimBias>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimBias>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
