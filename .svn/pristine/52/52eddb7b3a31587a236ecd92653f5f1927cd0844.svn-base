#ifndef FACT_HeadersBiasTemp
#define FACT_HeadersBiasTemp

namespace BiasTemp
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kReceiving,
        };
    }

    struct Data
    {
        uint64_t time;
        float temp[10];
        double avg;
        double rms;
    } __attribute__((__packed__));
}

#endif
