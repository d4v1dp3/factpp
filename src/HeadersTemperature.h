#ifndef FACT_HeadersTemperature
#define FACT_HeadersTemperature

namespace Temperature
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
