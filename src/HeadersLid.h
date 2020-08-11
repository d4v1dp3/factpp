#ifndef FACT_HeadersLid
#define FACT_HeadersLid

namespace Lid
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kUnidentified,
            kInconsistent,
            kUnknown,
            kPowerProblem,
            kOvercurrent,
            kClosed,
            kOpen,
            kMoving,
            kLocked
        };
    };
};
#endif
