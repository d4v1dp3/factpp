#ifndef FACT_HeadersTNGWeather
#define FACT_HeadersTNGWeather

namespace TNGWeather
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

        float  fTemperature;
        float  fTempTrend;
        float  fDewPoint;
        float  fHumidity;
        float  fAirPressure;
        float  fWindSpeed;
        float  fWindDirection;
        float  fDustTotal;
        float  fSolarimeter;

    } __attribute__((__packed__));

    struct DimSeeing
    {
        DimSeeing() { memset(this, 0, sizeof(DimSeeing)); }

        float  fSeeing;
        float  fSeeingMed;
        float  fSeeingStdev;

    } __attribute__((__packed__));
};
#endif
