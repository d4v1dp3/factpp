#ifndef FACT_HeadersMagicWeather
#define FACT_HeadersMagicWeather

namespace MagicWeather
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

    struct DimWeather
    {
        DimWeather() { memset(this, 0, sizeof(DimWeather)); }

        uint16_t fStatus;

        float    fTemp;
        float    fDew;
        float    fHum;
        float    fPress;
        float    fWind;
        float    fGusts;
        float    fDir;

    } __attribute__((__packed__));
}

#endif
