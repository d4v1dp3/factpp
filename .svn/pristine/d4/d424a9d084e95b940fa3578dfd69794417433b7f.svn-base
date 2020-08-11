#ifndef FACT_HeadersFAD
#define FACT_HeadersFAD

#ifdef __cplusplus
#include <ostream>

// For debugging
#include <iostream>

#include "ByteOrder.h"

// ====================================================================

namespace FAD
{
#endif
    enum Enable
    {
        kCmdDrsEnable         = 0x0600,  // CMD_DENABLE/CMD_DISABLE
        kCmdDwrite            = 0x0800,  // CMD_DWRITE_RUN/CMD_DWRITE_STOP
        kCmdSclk              = 0x1000,  // CMD_SCLK_ON/OFF
        kCmdSrclk             = 0x1500,  // CMD_SRCLK_ON/OFF
        kCmdTriggerLine       = 0x1800,  // CMD_TRIGGERS_ON/CMD_TRIGGERS_OFF
        kCmdContTrigger       = 0x1f00,
        kCmdRun               = 0x2200,  // CMD_Start/Stop
        kCmdBusyOff           = 0x2400,  //
        kCmdBusyOn            = 0x3200,  //
        kCmdResetEventCounter = 0x2A00,  //
        kCmdSocket            = 0x3000,  // CMD_mode_command/CMD_mode_all_sockets
        kCmdSingleTrigger     = 0xA000,  // CMD_Trigger
    };

    enum Commands
    {
        kCmdWriteExecute      = 0x0400,         // Configure FAD with the current config ram

        kCmdWrite             = 0x0500,         // write to Config-RAM
        kCmdWriteRoi          = kCmdWrite|0x00, // Baseaddress ROI-Values
        kCmdWriteDac          = kCmdWrite|0x24, // Baseaddress DAC-Values

        kCmdWriteRate         = kCmdWrite|0x2c, // Continous trigger rate
        kCmdWriteRunNumberMSW = kCmdWrite|0x2d, // Run Number most  significant word
        kCmdWriteRunNumberLSW = kCmdWrite|0x2e, // Run Number least significant word

        /*
         kCmdRead            = 0x0a00,         // read from Config-RAM
         kCmdReadRoi         = kCmdRead|0x00,  // Baseaddress ROI-Values
         kCmdReadDac         = kCmdRead|0x24,  // Baseaddress DAC-Values
         */

        kCmdPhaseIncrease   = 0x1200,         // CMD_PS_DIRINC
        kCmdPhaseDecrease   = 0x1300,         // CMD_PS_DIRDEC
        kCmdPhaseApply      = 0x1400,         // CMD_PS_DO
        kCmdPhaseReset      = 0x1700,         // CMD_PS_RESET
    };

    namespace State
    {
        enum States
        {
            // State Machine states
            kOffline = 1,   // StateMachineImp::kSM_UserMode
            kDisconnected,
            kConnecting,
            kConnected,
            kConfiguring1,
            kConfiguring2,
            kConfiguring3,
            kConfigured,
            kRunInProgress
        };
    }

    enum FileFormat_t
    {
        kNone = 0,  // Nothing is written just some little output in the log-stream
        kDebug,     // The contents of the headers are output to the console
        kFits,      // FITS file written with streamer class ofits
        kRaw,       // Raw binary streams are written
        kCalib,     // DRS calibration in progress
        kCfitsio,   // FITS file written with cfitsio
        kZFits,     // Compressed FITS file written
    };

    enum
    {
        kMaxBins            = 1024,
        kNumTemp            = 4,
        kNumDac             = 8,
        kNumChips           = 4,
        kNumChannelsPerChip = 9,
        kNumChannels        = kNumChips*kNumChannelsPerChip,
    };

    enum
    {
        kMaxRegAddr   = 0xff,    // Highest address in config-ram
        kMaxRegValue  = 0xffff,
        kMaxDacAddr   = kNumDac-1,
        kMaxDacValue  = 0xffff,
        kMaxRoiAddr   = kNumChannels-1,
        kMaxRoiValue  = kMaxBins,
        kMaxRunNumber = 0xffffffff,
    };

    enum
    {
        kDelimiterStart = 0xfb01,
        kDelimiterEnd   = 0x04fe,
    };

    // --------------------------------------------------------

    struct EventHeader
    {
#ifdef __cplusplus
        enum Bits
        {
            kDenable       = 1<<11,
            kDwrite        = 1<<10,
            //kRefClkTooHigh = 1<< 9,
            kRefClkTooLow  = 1<< 8,
            kDcmLocked     = 1<< 7,
            kDcmReady      = 1<< 6,
            kSpiSclk       = 1<< 5,
            kBusyOff       = 1<< 4,  // Busy continously off
            kTriggerLine   = 1<< 3,  // Trigger line enabled
            kContTrigger   = 1<< 2,  // Cont trigger enabled
            kSock17        = 1<< 1,  // Socket 1-7 for data transfer
            kBusyOn        = 1<< 0,  // Busy continously on
        };

        enum TriggerType
        {
            kLPext    = 0x0100,
            kLPint    = 0x0200,
            kPedestal = 0x0400,
            kLPset    = 0x7800,
            kTIM      = 0x8000,

            kExt1     = 0x0001,
            kExt2     = 0x0002,
            kAll      = kLPext|kLPint|kTIM|kPedestal|kExt1|kExt2
        };
#endif
        // Einmalig:     (new header changes entry in array --> send only if array changed)
        // ----------------------------------
        // Event builder stores an array with all available values.
        // Disconnected boards are removed (replaced by def values)
        // Any received header information is immediately put in the array.
        // The array is transmitted whenever it changes.
        // This will usually happen only very rarely when a new connection
        // is opened.
        //
        // Array[40] of BoardId
        // Array[40] of Version
        // Array[40] of DNA

        // Slow changes: (new header changes entry in array --> send only if arra changed)
        // -------------------------------------------
        // Event builder stores an array with all available values.
        // Disconnected boards can be kept in the arrays.
        // Any received header information is immediately put in the array.
        // The array is transmitted whenever it changes.
        //
        // Connection status (disconnected, connecting, connected) / Array[40]
        // Consistency of PLLLCK       / Array[  40] of PLLLCK
        // Consistency of Trigger type / Array[  40] of trigger type
        // Consistency of ROI          / Array[1440] of ROI
        // Consistency of RefClock     / Array[  40] of ref clock
        // Consistency of DAC values   / Array[ 400] of DAC values
        // Consistency of run number   / Array[  40] of Run numbers

        // Fast changes  (new header changes value --> send only if something changed)
        // -------------------
        // Event builder stores an internal array of all boards and
        //  transmits the min/max values determined from the array
        //  only if they have changed. Disconnected boards are not considered.
        //
        // Maximum/minimum Event counter of all boards in memory + board id
        // Maximum/minimum time stamp    of all boards in memory + board id
        // Maximum/minimum temp          of all boards in memory + board id

        // Unknown:
        // ------------------
        // Trigger Id ?
        // TriggerGeneratorPrescaler ?
        // Number of Triggers to generate ?


        // ------------------------------------------------------------

        uint16_t fStartDelimiter;     // 0x04FE
        uint16_t fPackageLength;
        uint16_t fVersion;
        uint16_t fStatus;
        //
        uint16_t fTriggerCrc;          // Receiver timeout / CRC ; 1 byte each
        uint16_t fTriggerType;
        uint32_t fTriggerCounter;
        //
        uint32_t fEventCounter;
        uint32_t fFreqRefClock;
        //
        uint16_t fBoardId;
        uint16_t fAdcClockPhaseShift;
        uint16_t fNumTriggersToGenerate;
        uint16_t fTriggerGeneratorPrescaler;
        //
        uint64_t fDNA; // Xilinx DNA
        //
        uint32_t fTimeStamp;
        uint32_t fRunNumber;
        //
        int16_t  fTempDrs[kNumTemp];   // In units of 1/16 deg(?)
        //
        uint16_t fDac[kNumDac];
        //
#ifdef __cplusplus

        EventHeader() { init(*this); }
        EventHeader(const uint16_t *ptr)
        {
            *this = std::vector<uint16_t>(ptr, ptr+sizeof(EventHeader)/2);
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"

        void operator=(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);

            Reverse(&fEventCounter);
            Reverse(&fTriggerCounter);
            Reverse(&fFreqRefClock);
            Reverse(&fTimeStamp);
            Reverse(&fRunNumber);

            for (int i=0; i<8; i+=2)
                std::swap(reinterpret_cast<uint8_t*>(&fDNA)[i],
                          reinterpret_cast<uint8_t*>(&fDNA)[i+1]);
        }

        std::vector<uint16_t> HtoN() const
        {
            EventHeader h(*this);

            Reverse(&h.fEventCounter);
            Reverse(&h.fFreqRefClock);
            Reverse(&h.fTimeStamp);
            Reverse(&h.fRunNumber);

            for (int i=0; i<8; i+=2)
                std::swap(reinterpret_cast<uint8_t*>(&h.fDNA)[i],
                          reinterpret_cast<uint8_t*>(&h.fDNA)[i+1]);

            return htoncpy(h);
        }

#pragma clang diagnostic pop

        bool operator==(const EventHeader &h) const
        {
            return
                (fStatus&~(kSock17|kBusyOn)) == (h.fStatus&~(kSock17|kBusyOn)) &&
                fRunNumber == h.fRunNumber &&
                fEventCounter == h.fEventCounter &&
                fAdcClockPhaseShift == h.fAdcClockPhaseShift &&
                fTriggerGeneratorPrescaler == h.fTriggerGeneratorPrescaler &&
                memcmp(fDac, h.fDac, sizeof(fDac))==0;
        }
        bool operator!=(const EventHeader &h) const { return !operator==(h); }

        float GetTemp(int i) const { return fTempDrs[i]/16.; }

        uint8_t PLLLCK() const         { return fStatus>>12; }

        bool HasDenable() const        { return fStatus&kDenable; }
        bool HasDwrite() const         { return fStatus&kDwrite; }
//        bool IsRefClockTooHigh() const { return fStatus&kRefClkTooHigh; }
        bool IsRefClockTooLow() const  { return fStatus&kRefClkTooLow; }
        bool IsDcmLocked() const       { return fStatus&kDcmLocked; }
        bool IsDcmReady() const        { return fStatus&kDcmReady; }
        bool HasSpiSclk() const        { return fStatus&kSpiSclk; }
        bool HasBusyOn() const         { return fStatus&kBusyOn; }
        bool HasBusyOff() const        { return fStatus&kBusyOff; }
        bool HasTriggerEnabled() const { return fStatus&kTriggerLine; }
        bool HasContTriggerEnabled() const { return fStatus&kContTrigger; }
        bool IsInSock17Mode() const    { return fStatus&kSock17; }

        int  GetTriggerLogic() const { return (fTriggerType>>2)&0x3f; }
        bool HasTriggerExt1() const  { return fTriggerType&kExt1; }
        bool HasTriggerExt2() const  { return fTriggerType&kExt2; }
        bool HasTIMsource() const    { return fTriggerType&kTIM; }
        bool HasTriggerLPext() const { return fTriggerType&kLPext; }
        bool HasTriggerLPint() const { return fTriggerType&kLPint; }
        bool HasTriggerPed() const   { return fTriggerType&kPedestal; }
        bool IsTriggerPhys() const   { return !(fTriggerType&kAll); }
        int  GetTriggerLPset() const { return (fTriggerType&kLPset)>>11; }

        uint16_t Crate() const { return fBoardId>>8; }
        uint16_t Board() const { return fBoardId&0xff; }

        uint16_t Id() const { return Crate()*10+Board(); }

        void Enable(Bits pos, bool enable=true)
        {
            if (enable)
                fStatus |= pos;
            else
                fStatus &= ~pos;
        }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;
#endif

    } __attribute__((__packed__));

    struct ChannelHeader
    {
        uint16_t fId;
        uint16_t fStartCell;
        uint16_t fRegionOfInterest;
        uint16_t fDummy;
        // uint16_t fData[];

#ifdef __cplusplus
        ChannelHeader() { init(*this); }

        void operator=(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);
        }

        std::vector<uint16_t> HtoN() const
        {
            ChannelHeader h(*this);
            return htoncpy(h);
        }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;

        uint16_t Chip() const    { return fId>>4; }
        uint16_t Channel() const { return fId&0xf; }
#endif
    } __attribute__((__packed__));

    // Package ends with:
    //   0x4242
    //   0x04fe

    struct Configuration
    {
        bool     fDwrite;
        bool     fDenable;
        bool     fContinousTrigger;
        uint16_t fTriggerRate;
        uint16_t fRoi[FAD::kNumChannelsPerChip];
        uint16_t fDac[FAD::kNumDac];

#ifdef __cplusplus
        Configuration() { init(*this); }
#endif
    };

    struct RunDescription
    {
        uint32_t maxtime;
        uint32_t maxevt;
        uint32_t night;

        std::string name;

        Configuration reference;
    };

    // --------------------------------------------------------------------
#ifdef __cplusplus
    inline std::ostream &operator<<(std::ostream &out, const EventHeader &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const ChannelHeader &h)
    {
        h.print(out);
        return out;
    }
#endif

#ifdef __cplusplus
};
#endif

#endif
