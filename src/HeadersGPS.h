#ifndef FACT_HeadersGPS
#define FACT_HeadersGPS

namespace GPS
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kDisabled,
            kEnabled,
            kLocked
        };
    };

    struct NEMA
    {
        float    time;
        float    lat;
        float    lng;
        float    hdop;
        float    height;
        float    geosep;
        uint16_t count;
        uint16_t qos;
    }  __attribute__((__packed__));

};
#endif
