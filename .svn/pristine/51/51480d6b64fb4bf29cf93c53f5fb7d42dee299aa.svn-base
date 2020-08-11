#ifndef FACT_HeadersSQM
#define FACT_HeadersSQM

namespace SQM
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kInvalid,
            kValid,
        };
    };

    struct Data
    {
        float    mag;        // Magnitude per square arc second (0.00m upper brightness limit)
        uint32_t freq;       // Frequency of sensor in Hz
        uint32_t counts;     // Period of sensor in counts (counts occur at 14.7456MHz/32)
        float    period;     // Period of sensor in seconds (millisecond resolution)
        float    temp;       // Temperature measured at light sensor in degC
    }  __attribute__((__packed__));

};
#endif
