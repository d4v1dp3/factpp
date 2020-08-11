#ifndef FACT_HeadersToO
#define FACT_HeadersToO

namespace ToO
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected    = 2,
            kArmed        = 3,
        };

    }

    struct DataGRB
    {
        uint16_t type;
        //uint32_t trigid;
        double ra;
        double dec;
        double err;
    } __attribute__((__packed__));;
}

#endif
