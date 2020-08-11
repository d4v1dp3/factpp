#ifndef FACT_HeadersAgilent
#define FACT_HeadersAgilent

namespace Agilent
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kVoltageOff,
            kVoltageLow,
            kVoltageOn,
            kVoltageHigh,
        };
    }

    struct Data
    {
        float fVoltageSet;
        float fVoltageMeasured;

        float fCurrentLimit;
        float fCurrentMeasured;

        Data() : fVoltageSet(-1), fVoltageMeasured(-1), fCurrentLimit(-1), fCurrentMeasured(-1) { }
    };
}

#endif
