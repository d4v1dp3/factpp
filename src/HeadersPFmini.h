#ifndef FACT_HeadersPFmini
#define FACT_HeadersPFmini

namespace PFmini
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kReceiving
        };
    };

    struct Data
    {
        float    hum;
        float    temp;
    }  __attribute__((__packed__));

};
#endif
