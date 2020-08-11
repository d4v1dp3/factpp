#ifndef FACT_HeadersRateScan
#define FACT_HeadersRateScan

namespace RateScan
{
    namespace State
    {
        enum states_t
        {
            kDimNetworkNA = 1,
            kDisconnected,
            kConnecting,
            kConnected,
            kConfiguring,
            kInProgress,
            kPaused,
        };
    }
}

#endif
