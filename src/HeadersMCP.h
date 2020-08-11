#ifndef FACT_HeadersMCP
#define FACT_HeadersMCP

namespace MCP
{
    namespace State
    {
        enum states_t
        {
            kDimNetworkNA = 1,
            kDisconnected,
            kConnecting,
            kConnected,
            kIdle,
            kDummy, // Doesn't exist, kept to keep the numbers
            kConfiguring1,
            kConfiguring2,
            kConfiguring3,
            kCrateReset0,
            kCrateReset1,
            kCrateReset2,
            kCrateReset3,
            kConfigured,
            kTriggerOn,
            kTakingData,
        };
    }
}
#endif
