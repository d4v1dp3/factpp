#ifndef FACT_HeadersRainSensor
#define FACT_HeadersRainSensor

namespace RainSensor
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kValid
        };
    };
};
#endif
