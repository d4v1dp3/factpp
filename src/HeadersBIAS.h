#ifndef FACT_HeadersBIAS
#define FACT_HeadersBIAS

namespace BIAS
{
    enum
    {
        kNumBoards           = 13,
        kNumChannelsPerBoard = 32,
        kNumChannels = kNumBoards*kNumChannelsPerBoard
    };

    enum Command_t
    {
        // Communication commands
        kCmdReset         =  0,
        kCmdRead          =  1,
        kCmdGlobalSet     =  2,
        kCmdChannelSet    =  3,

        // Internal command names
        kResetChannels    = 0x10|kCmdChannelSet,
        kUpdate           = 0x10|kCmdRead,
        kExpertChannelSet = 0x14|kCmdChannelSet,
        kSynchronize      = 0x1e,
    };

    enum
    {
        kMaxDac = 0xfff
    };

    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,//StateMachineImp::kSM_UserMode,
            kConnecting,
            kInitializing,
            kConnected,
            kRamping,
            kOverCurrent,
            kVoltageOff,
            kNotReferenced,
            kVoltageOn,
            kExpertMode, // 'forward' declaration to be used in StateMachineBias
            kLocked,
        };
    }
}

#endif
