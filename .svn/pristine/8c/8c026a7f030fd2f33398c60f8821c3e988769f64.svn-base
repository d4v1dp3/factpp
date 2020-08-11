#ifndef FACT_HeadersFTM
#define FACT_HeadersFTM

#include <ostream>

// For debugging
#include <iostream>

#include "ByteOrder.h"

// ====================================================================


namespace FTM
{
    enum States
    {
        kFtmUndefined = 0,

        // FTM internal states
        kFtmIdle    = 1, ///< Trigger output disabled, configuration possible
        kFtmConfig  = 2, ///< FTM and FTUs are being reconfigured
        kFtmRunning = 3, ///< Trigger output enabled, configuration ignored
        kFtmCalib   = 4,

        kFtmStates  = 0x0ff,
        kFtmLocked  = 0x100,

    };

    // idle:    not locked: 0x2711
    // running: not locked: 0x2713

    namespace State
    {
        enum StateMachine
        {
            kDisconnected = 1,
            kConnected,
            kIdle,
            kValid,
            kTriggerOn,
            kConfiguring1,
            kConfiguring2,
            kConfigured1,
            kConfigured2,

            kConfigError1 = 0x101,
            kConfigError2 = 0x102,
            //kConfigError3 = 0x103,
        };
    }

    /// Command codes for FTM communication
    enum Commands
    {
        // First word
        kCmdRead           = 0x0001, ///< Request data
        kCmdWrite          = 0x0002, ///< Send data
        kCmdStartRun       = 0x0004, ///< Enable the trigger output
        kCmdStopRun        = 0x0008, ///< Disable the trigger output
        kCmdPing           = 0x0010, ///< Ping all FTUs (get FTU list)
        kCmdCrateReset     = 0x0020, ///< Reboot (no power cycle) all FTUs and FADs of one crate
        kCmdDisableReports = 0x0040, ///< Disable transmission of rate-reports (dynamic data)
        kCmdConfigFTU      = 0x0080, ///< Configure single FTU board
        kCmdToggleLed      = 0xc000,

        // second word for read and write
        kCmdStaticData     = 0x0001, ///< Specifies that static (configuration) data is read/written
        kCmdDynamicData    = 0x0002, ///< Specifies that dynamic data is read/written
        kCmdRegister       = 0x0004, ///< Specifies that a register is read/written

        // second word for StartRun
        kStartRun          = 0x0001, ///< ...until kCmdStopRun
        kTakeNevents       = 0x0002, ///< ...fixed number of events

        // second word for kCmdCrateReset
        kResetCrate0       = 0x0001,
        kResetCrate1       = 0x0002,
        kResetCrate2       = 0x0004,
        kResetCrate3       = 0x0008,
    };


    /// Types sent in the header of the following data
    enum Types
    {
        kHeader      = 0,  ///< Local extension to identify a header in fCounter
        kStaticData  = 1,  ///< Static (configuration) data
        kDynamicData = 2,  ///< Dynamic data (rates)
        kFtuList     = 3,  ///< FTU list (answer of ping)
        kErrorList   = 4,  ///< Error list (error when FTU communication failed)
        kRegister    = 5,  ///< A requested register value
    };

    // --------------------------------------------------------------------

    enum Delimiter
    {
        kDelimiterStart = 0xfb01, ///< Start delimiter send before each header
        kDelimiterEnd   = 0x04fe  ///< End delimiter send after each data block
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"

    struct Header
    {
        uint16_t fDelimiter;      ///< Start delimiter
        uint16_t fType;           ///< Type of the data to be received after the header
        uint16_t fDataSize;       ///< Size in words to be received after the header (incl end delim.)
        uint16_t fState;          ///< State of the FTM central state machine
        uint64_t fBoardId;        ///< FPGA device DNA (unique chip id)
        uint16_t fFirmwareId;     ///< Version number
        uint32_t fTriggerCounter; ///< FTM internal counter of all trigger decision independant of trigger-line enable/disable (reset: start/stop run)
        uint64_t fTimeStamp;      ///< Internal counter (micro-seconds, reset: start/stop run)

        Header() { init(*this); }

        std::vector<uint16_t> HtoN() const
        {
            Header h(*this);

            Reverse(&h.fBoardId);
            Reverse(&h.fTriggerCounter);
            Reverse(&h.fTimeStamp);

            return htoncpy(h);
        }
        void operator=(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);

            Reverse(&fBoardId);
            Reverse(&fTriggerCounter);
            Reverse(&fTimeStamp);
        }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;

    } __attribute__((__packed__));

    struct DimPassport
    {
        uint64_t fBoardId;
        uint16_t fFirmwareId;

        DimPassport(const Header &h) :
            fBoardId(h.fBoardId),
            fFirmwareId(h.fFirmwareId)
        {
        }
    } __attribute__((__packed__));

    /*
    struct DimTriggerCounter
    {
        uint64_t fTimeStamp;
        uint32_t fTriggerCounter;

        DimTriggerCounter(const Header &h) :
            fTimeStamp(h.fTimeStamp),
            fTriggerCounter(h.fTriggerCounter)
       {
        }
    } __attribute__((__packed__));
    */

    struct StaticDataBoard
    {
        uint16_t fEnable[4];   /// enable of 4x9 pixels coded as 4x9bits
        uint16_t fDAC[5];      /// 0-3 (A-D) Threshold of patches, 4 (H) Threshold for N out of 4 (12 bit each)
        uint16_t fPrescaling;  /// Internal readout time of FTUs for trigger counter

        StaticDataBoard() { init(*this); }

        void print(std::ostream &out) const;

    } __attribute__((__packed__));

    struct StaticData
    {
        enum Limits
        {
            kMaxMultiplicity    = 40,      ///< Minimum required trigger multiplicity
            kMaxWindow          = 0xf,     ///< (4ns * x + 8ns) At least N (multiplicity) rising edges (trigger signal) within this window
            kMaxDeadTime        = 0xffff,  ///< (4ns * x + 8ns)
            kMaxDelayTimeMarker = 0x3ff,   ///< (4ns * x + 8ns)
            kMaxDelayTrigger    = 0x3ff,   ///< (4ns * x + 8ns)
            kMaxTriggerInterval = 0x3ff,   ///< 
            kMaxIntensity       = 0x7f,
            kMaxSequence        = 0x1f,
            kMaxDAC             = 0xfff,
            kMaxAddr            = 0xfff,
            kMaxPatchIdx        = 159,
            kMaxPixelIdx        = 1439,
            kMaskSettings       = 0xf,
            kMaskLEDs           = 0xf,
        };

        enum GeneralSettings
        {
            kTrigger    = 0x80,  ///< Physics trigger decision (PhysicTrigger)
            kPedestal   = 0x40,  ///< Pedestal trigger (artifical)
            kLPint      = 0x20,  ///< Enable artificial trigger after light pulse (LP2)
            kLPext      = 0x10,  ///< Enable trigger decision after light pulse (CalibrationTrigger, LP1)
            kExt2       = 0x08,  ///< External trigger signal 2
            kExt1       = 0x04,  ///< External trigger signal 1
            kVeto       = 0x02,  ///< Veto trigger decision / artifical triggers
            kClockConditioner = 0x01,  ///< Select clock conditioner frequency (1) / time marker (0) as output
        };

        enum LightPulserEnable
        {
            kGroup1 = 0x40,
            kGroup2 = 0x80,
        };

        uint16_t fGeneralSettings;         /// Enable for different trigger types / select for TIM/ClockConditioner output (only 8 bit used)
        uint16_t fStatusLEDs;              /// only 8 bit used
        uint16_t fTriggerInterval;         /// [ms] Interval between two artificial triggers (no matter which type) minimum 1ms, 10 bit
        uint16_t fTriggerSequence;         /// Ratio between trigger types send as artificial trigger (in this order) 3x5bit
        uint8_t  fIntensityLPext;          /// Intensity of LEDs (0-127)
        uint8_t  fEnableLPext;             /// Enable for LED group 1/2 (LightPulserEnable)
        uint8_t  fIntensityLPint;          /// Intensity of LEDs (0-127)
        uint8_t  fEnableLPint;             /// Enable for LED group 1/2 (LightPulserEnable)
        uint32_t fDummy0;
        uint16_t fMultiplicityPhysics;     /// Required trigger multiplicity for physcis triggers (0-40)
        uint16_t fMultiplicityCalib;       /// Required trigger multiplicity calibration (LPext) triggers (0-40)
        uint16_t fDelayTrigger;            /// (4ns * x + 8ns) FTM internal programmable delay between trigger decision and output
        uint16_t fDelayTimeMarker;         /// (4ns * x + 8ns) FTM internal programmable delay between trigger descision and time marker output
        uint16_t fDeadTime;                /// (4ns * x + 8ns) FTM internal programmable dead time after trigger decision
        uint32_t fClockConditioner[8];     /// R0, R1, R8, R9, R11, R13, R14, R15
        uint16_t fWindowPhysics;           /// (4ns * x + 8ns) At least N (multiplicity) rising edges (trigger signal) within this window
        uint16_t fWindowCalib;             /// (4ns * x + 8ns) At least N (multiplicity) rising edges (trigger signal) within this window
        uint16_t fDummy1;

        StaticDataBoard fBoard[4][10];      // 4 crates * 10 boards (Crate0/FTU0 == readout time of FTUs)

        uint16_t fActiveFTU[4];             // 4 crates * 10 bits   (FTU enable)

        StaticData() { init(*this); }
        StaticData(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);

            for (int i=0; i<8; i++)
                Reverse(fClockConditioner+i);
        }

        std::vector<uint16_t> HtoN() const
        {
            StaticData d(*this);
            for (int i=0; i<8; i++)
                Reverse(d.fClockConditioner+i);

            return htoncpy(d);
        }

        bool operator==(StaticData d) const
        {
            for (int i=0; i<4; i++)
                for (int j=0; j<10; j++)
                    memcpy(d.fBoard[i][j].fDAC, fBoard[i][j].fDAC, sizeof(uint16_t)*5);
            return memcmp(this, &d, sizeof(StaticData))==0;
        }

        bool valid() const { static StaticData empty; return memcmp(this, &empty, sizeof(FTM::StaticData))!=0; }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;

        StaticDataBoard &operator[](int i) { return fBoard[i/10][i%10]; }
        const StaticDataBoard &operator[](int i) const { return fBoard[i/10][i%10]; }

        void EnableFTU(int i)  { fActiveFTU[i/10] |=  (1<<(i%10)); }
        void DisableFTU(int i) { fActiveFTU[i/10] &= ~(1<<(i%10)); }

        void EnableAllFTU()    { for (int i=0; i<4; i++) fActiveFTU[i] = 0x3ff; }
        void DisableAllFTU()   { for (int i=0; i<4; i++) fActiveFTU[i] = 0;     }

        void EnableLPint(LightPulserEnable group, bool enable)
        {
            if (enable)
                fEnableLPint |= group;
            else
                fEnableLPint &= ~group;
        }

        void EnableLPext(LightPulserEnable group, bool enable)
        {
            if (enable)
                fEnableLPext |= group;
            else
                fEnableLPext &= ~group;
        }

        void ToggleFTU(int i)  { fActiveFTU[i/10] ^= (1<<(i%10)); }

        void Enable(GeneralSettings type, bool enable)
        {
	    if (enable)
		fGeneralSettings |= uint16_t(type);
	    else
                fGeneralSettings &= ~uint16_t(type);
        }

        bool IsEnabled(GeneralSettings type) const { return fGeneralSettings&uint16_t(type); }

        uint16_t *EnablePixel(int idx, bool enable)
        {
            const int pixel = idx%9;
            const int patch = (idx/9)%4;
            const int board = (idx/9)/4;

            uint16_t &pix = fBoard[board/10][board%10].fEnable[patch];

            if (enable)
                pix |= (1<<pixel);
            else
                pix &= ~(1<<pixel);

            return &pix;
        }

        void EnablePatch(int idx, bool enable)
        {
            const int patch = idx%4;
            const int board = idx/4;

            fBoard[board/10][board%10].fEnable[patch] = enable ? 0x1ff : 0;
        }

        void EnableAllPixel()
        {
            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    for (int p=0; p<4; p++)
                        fBoard[c][b].fEnable[p] = 0x1ff;
        }

        bool Enabled(uint16_t idx) const
        {
            const int pixel = idx%9;
            const int patch = (idx/9)%4;
            const int board = (idx/9)/4;

            return (fBoard[board/10][board%10].fEnable[patch]>>pixel)&1;
        }

        uint8_t GetSequencePed() const   { return (fTriggerSequence>>10)&0x1f; }
        uint8_t GetSequenceLPint() const { return (fTriggerSequence>> 5)&0x1f; }
        uint8_t GetSequenceLPext() const { return (fTriggerSequence)    &0x1f; }

        void SetSequence(uint8_t ped, uint8_t lpint, uint8_t lpext)
        {
            fTriggerSequence = ((ped&0x1f)<<10)|((lpint&0x1f)<<5)|(lpext&0x1f);

            Enable(kPedestal, ped  >0);
            Enable(kLPext,    lpext>0);
            Enable(kLPint,    lpint>0);
        }

        void SetClockRegister(const uint64_t reg[])
        {
            for (int i=0; i<8; i++)
                fClockConditioner[i] = reg[i];
        }

        void SetPrescaling(uint16_t val)
        {
            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    fBoard[c][b].fPrescaling = val;
        }

    } __attribute__((__packed__));

    // DimStructures must be a multiple of two... I don't know why
    struct DimStaticData
    {
        uint64_t fTimeStamp;
        //8
        uint16_t fGeneralSettings;         // only 8 bit used
        uint16_t fStatusLEDs;              // only 8 bit used
        uint64_t fActiveFTU;               // 40 bits in row
        //20
        uint16_t fTriggerInterval;         // only 10 bit used
        //22
        uint16_t fTriggerSeqLPint;         // only 5bits used
        uint16_t fTriggerSeqLPext;         // only 5bits used
        uint16_t fTriggerSeqPed;           // only 5bits used
        // 28
        uint8_t  fEnableLPint;             /// Enable for LED group 1/2 (LightPulserEnable)
        uint8_t  fEnableLPext;             /// Enable for LED group 1/2 (LightPulserEnable)
        uint8_t  fIntensityLPint;          /// Intensity of LEDs (0-127)
        uint8_t  fIntensityLPext;          /// Intensity of LEDs (0-127)
        //32
        uint16_t fMultiplicityPhysics;      // 0-40
        uint16_t fMultiplicityCalib;        // 0-40
        //36
        uint16_t fWindowPhysics;
        uint16_t fWindowCalib;
        //40
        uint16_t fDelayTrigger;
        uint16_t fDelayTimeMarker;
        uint32_t fDeadTime;
        //48
        uint32_t fClockConditioner[8];
        //64
        uint16_t fEnable[90];  // 160*9bit = 180byte
        uint16_t fThreshold[160];
        uint16_t fMultiplicity[40];     // N out of 4
        uint16_t fPrescaling[40];
        // 640+64 = 704

        bool HasTrigger() const     { return fGeneralSettings & StaticData::kTrigger; }
        bool HasPedestal() const    { return fGeneralSettings & StaticData::kPedestal; }
        bool HasLPext() const       { return fGeneralSettings & StaticData::kLPext; }
        bool HasLPint() const       { return fGeneralSettings & StaticData::kLPint; }
        bool HasExt2() const        { return fGeneralSettings & StaticData::kExt2; }
        bool HasExt1() const        { return fGeneralSettings & StaticData::kExt1; }
        bool HasVeto() const        { return fGeneralSettings & StaticData::kVeto; }
        bool HasClockConditioner() const { return fGeneralSettings & StaticData::kClockConditioner; }

        bool HasLPextG1() const { return fEnableLPext&StaticData::kGroup1; }
        bool HasLPextG2() const { return fEnableLPext&StaticData::kGroup2; }
        bool HasLPintG1() const { return fEnableLPint&StaticData::kGroup1; }
        bool HasLPintG2() const { return fEnableLPint&StaticData::kGroup2; }

        bool IsActive(int i) const { return fActiveFTU&(uint64_t(1)<<i); }
        bool IsEnabled(int i) const { return fEnable[i/16]&(1<<(i%16)); }

        DimStaticData() { memset(this, 0, sizeof(DimStaticData)); }

        DimStaticData(const Header &h, const StaticData &d) :
            fTimeStamp(h.fTimeStamp),
            fGeneralSettings(d.fGeneralSettings),
            fStatusLEDs(d.fStatusLEDs),
            fActiveFTU( uint64_t(d.fActiveFTU[0])      |
                       (uint64_t(d.fActiveFTU[1])<<10) |
                       (uint64_t(d.fActiveFTU[2])<<20) |
                       (uint64_t(d.fActiveFTU[3])<<30)),
            fTriggerInterval(d.fTriggerInterval),
            fTriggerSeqLPint((d.fTriggerSequence>>5)&0x1f),
            fTriggerSeqLPext((d.fTriggerSequence)&0x1f),
            fTriggerSeqPed((d.fTriggerSequence>>10)&0x1f),
            fEnableLPint(d.fEnableLPint),
            fEnableLPext(d.fEnableLPext),
            fIntensityLPint(d.fIntensityLPint),
            fIntensityLPext(d.fIntensityLPext),
            fMultiplicityPhysics(d.fMultiplicityPhysics),
            fMultiplicityCalib(d.fMultiplicityCalib),
            fWindowPhysics(d.fWindowPhysics*4+8),
            fWindowCalib(d.fWindowCalib*4+8),
            fDelayTrigger(d.fDelayTrigger*4+8),
            fDelayTimeMarker(d.fDelayTimeMarker*4+8),
            fDeadTime(uint32_t(d.fDeadTime)*4+8)
        {
            memcpy(fClockConditioner, d.fClockConditioner, sizeof(uint32_t)*8);

            uint16_t src[160];
            for (int i=0; i<40; i++)
            {
                for (int j=0; j<4; j++)
                {
                    src[i*4+j] = d[i].fEnable[j];
                    fThreshold[i*4+j] = d[i].fDAC[j];
                }

                fMultiplicity[i] = d[i].fDAC[4];
                fPrescaling[i] = d[i].fPrescaling+1;
            }
            bitcpy(fEnable, 90, src, 160, 9);
        }

    } __attribute__((__packed__));


    struct DynamicDataBoard
    {
        uint32_t fRatePatch[4];   // Patch 0,1,2,3
        uint32_t fRateTotal;      // Sum

        uint16_t fOverflow;       // Patches: bits 0-3, total 4
        uint16_t fCrcError;

        void print(std::ostream &out) const;

        void reverse()
        {
            for (int i=0; i<4; i++)
                Reverse(fRatePatch+i);

            Reverse(&fRateTotal);
        }

        uint32_t &operator[](int i) { return fRatePatch[i]; }

    }  __attribute__((__packed__));


    struct DynamicData
    {
        uint64_t fOnTimeCounter;
        uint16_t fTempSensor[4];  // U45, U46, U48, U49

        DynamicDataBoard fBoard[4][10];      // 4 crates * 10 boards

        DynamicData() { init(*this); }

        std::vector<uint16_t> HtoN() const
        {
            DynamicData d(*this);

            Reverse(&d.fOnTimeCounter);

            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    d.fBoard[c][b].reverse();

            return htoncpy(d);
        }

        void operator=(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);

            Reverse(&fOnTimeCounter);

            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    fBoard[c][b].reverse();
        }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;

        DynamicDataBoard &operator[](int i) { return fBoard[i/10][i%10]; }
        const DynamicDataBoard &operator[](int i) const { return fBoard[i/10][i%10]; }

    } __attribute__((__packed__));


    struct DimDynamicData
    {
        uint64_t fTimeStamp;

        uint64_t fOnTimeCounter;
        float    fTempSensor[4];

        uint32_t fRatePatch[160];

        uint32_t fRateBoard[40];
        uint16_t fRateOverflow[40];

        uint16_t fPrescaling[40];

        uint16_t fCrcError[40];

        uint16_t fState;

        DimDynamicData(const Header &h, const DynamicData &d, const StaticData &s) :
            fTimeStamp(h.fTimeStamp),
            fOnTimeCounter(d.fOnTimeCounter),
            fState(h.fState)
        {
            for (int i=0; i<4; i++)
                fTempSensor[i] = d.fTempSensor[i];

            for (int i=0; i<40; i++)
            {
                fRateBoard[i]    = d[i].fRateTotal;
                fRateOverflow[i] = d[i].fOverflow;
                fCrcError[i]     = d[i].fCrcError;
                for (int j=0; j<4; j++)
                    fRatePatch[i*4+j] = d[i].fRatePatch[j];

                fPrescaling[i] = s[i].fPrescaling+1;
            }
        }

    } __attribute__((__packed__));

    struct DimTriggerRates
    {
        uint64_t fTimeStamp;
        uint64_t fOnTimeCounter;
        uint32_t fTriggerCounter;
        float    fTriggerRate;
        float    fBoardRate[40];
        float    fPatchRate[160];

        float fElapsedTime;
        float fOnTime;

        DimTriggerRates() { memset(this, 0, sizeof(DimTriggerRates)); }

        DimTriggerRates(const Header &h, const DynamicData &d, const StaticData &s, float rate, float et, float ot) :
            fTimeStamp(h.fTimeStamp), fOnTimeCounter(d.fOnTimeCounter),
            fTriggerCounter(h.fTriggerCounter), fTriggerRate(rate),
            fElapsedTime(et), fOnTime(ot)
        {
            for (int i=0; i<40; i++)
            {
                if ((d[i].fOverflow>>4)&1)
                    fBoardRate[i] = float(UINT32_MAX+1)*2/(s[i].fPrescaling+1);
                else
                    fBoardRate[i] = float(d[i].fRateTotal)*2/(s[i].fPrescaling+1);

                // FIXME: Include fCrcError in calculation
                //fRateOverflow[i] = d[i].fOverflow;
                for (int j=0; j<4; j++)
                    if ((d[i].fOverflow>>j)&1)
                        fPatchRate[i*4+j] = float(UINT32_MAX+1)*2/(s[i].fPrescaling+1);
                    else
                        fPatchRate[i*4+j] = float(d[i].fRatePatch[j])*2/(s[i].fPrescaling+1);
            }
        }

    } __attribute__((__packed__));


    struct FtuResponse
    {
        uint16_t fPingAddr;       // Number of Pings and addr (pings= see error)
        uint64_t fDNA;
        uint16_t fErrorCounter;   //

        void reverse() { Reverse(&fDNA); }

        void print(std::ostream &out) const;

    } __attribute__((__packed__));

#pragma clang diagnostic pop

    struct FtuList
    {
        uint16_t fNumBoards;         /// Total number of boards responded
        uint16_t fNumBoardsCrate[4]; /// Num of board responded in crate 0-3
        uint16_t fActiveFTU[4];      /// List of active FTU boards in crate 0-3

        FtuResponse fFTU[4][10];

        FtuList() { init(*this); }

        std::vector<uint16_t> HtoN() const
        {
            FtuList d(*this);

            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    d.fFTU[c][b].reverse();

            return htoncpy(d);
        }

        void operator=(const std::vector<uint16_t> &vec)
        {
            ntohcpy(vec, *this);

            for (int c=0; c<4; c++)
                for (int b=0; b<10; b++)
                    fFTU[c][b].reverse();
        }

        void clear() { reset(*this); }
        void print(std::ostream &out) const;

        FtuResponse &operator[](int i) { return fFTU[i/10][i%10]; }
        const FtuResponse &operator[](int i) const { return fFTU[i/10][i%10]; }

    } __attribute__((__packed__));

    struct DimFtuList
    {
        uint64_t fTimeStamp;
        uint64_t fActiveFTU;

        uint16_t fNumBoards;          /// Number of boards answered in total
        uint8_t  fNumBoardsCrate[4];  /// Number of boards answered per crate

        uint64_t fDNA[40];            /// DNA of FTU board
        uint8_t  fAddr[40];           /// Address of FTU board
        uint8_t  fPing[40];           /// Number of pings until response (same as in Error)

        DimFtuList(const Header &h, const FtuList &d) :
            fTimeStamp(h.fTimeStamp),
            fActiveFTU( uint64_t(d.fActiveFTU[0])      |
                       (uint64_t(d.fActiveFTU[1])<<10) |
                       (uint64_t(d.fActiveFTU[2])<<20) |
                       (uint64_t(d.fActiveFTU[3])<<30)),
            fNumBoards(d.fNumBoards)
        {
            for (int i=0; i<4; i++)
                fNumBoardsCrate[i] = d.fNumBoardsCrate[i];

            for (int i=0; i<40; i++)
            {
                fDNA[i]  =  d[i].fDNA;
                fAddr[i] =  d[i].fPingAddr&0x3f;
                fPing[i] = (d[i].fPingAddr>>8)&0x3;
            }
        }

        bool IsActive(int i) const { return fActiveFTU&(uint64_t(1)<<i); }

    } __attribute__((__packed__));


    struct Error
    {
        uint16_t fNumCalls;   // 0=error, >1 needed repetition but successfull

        uint16_t fDelimiter;
        uint16_t fDestAddress;
        uint16_t fSrcAddress;
        uint16_t fFirmwareId;
        uint16_t fCommand;
        uint16_t fData[21];
        uint16_t fCrcErrorCounter;
        uint16_t fCrcCheckSum;

        Error() { init(*this); }

        std::vector<uint16_t> HtoN() const
        {
            return htoncpy(*this);
        }

        void operator=(const std::vector<uint16_t> &vec) { ntohcpy(vec, *this); }

        void clear() { reset(*this); }

        uint16_t &operator[](int idx) { return fData[idx]; }
        const uint16_t &operator[](int idx) const { return fData[idx]; }

        void print(std::ostream &out) const;

    } __attribute__((__packed__));

    struct DimError
    {
        uint64_t fTimeStamp;
        Error    fError;

        DimError(const Header &h, const Error &e) :
            fTimeStamp(h.fTimeStamp),
            fError(e)
        {
            fError.fDestAddress = (e.fDestAddress&0x3)*10 + ((e.fDestAddress>>2)&0xf);
            fError.fSrcAddress  = (e.fSrcAddress &0x3)*10 + ((e.fSrcAddress >>2)&0xf);
        }

    }  __attribute__((__packed__));

    /*
    struct Command
    {
        uint16_t fStartDelimiter;
        uint16_t fCommand;
        uint16_t fParam[3];

        Command() { init(*this); }

        void HtoN() { hton(*this); }
        void NtoH() { ntoh(*this); }

        void operator=(const std::vector<uint16_t> &vec) { ntohcpy(vec, *this); }

        void clear() { reset(*this); }


     } __attribute__((__packed__));
    */

    // --------------------------------------------------------------------

    inline std::ostream &operator<<(std::ostream &out, const FtuResponse &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const Header &h)
    {
        h.print(out);
        return out;
    }


    inline std::ostream &operator<<(std::ostream &out, const FtuList &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const DynamicDataBoard &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const DynamicData &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const StaticDataBoard &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const StaticData &h)
    {
        h.print(out);
        return out;
    }

    inline std::ostream &operator<<(std::ostream &out, const Error &h)
    {
        h.print(out);
        return out;
    }
};

#endif
