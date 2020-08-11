#ifndef FACT_HeadersFSC
#define FACT_HeadersFSC

namespace FSC
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected    = 2,
        };
    }

    enum {
        kNumResistanceChannels = 64,
        kNumResistanceRegs     =  8,
        kNumVoltageChannels    = 84,
        kNumVoltageRegs        = 11
    };


    struct BinaryOutput_t
    {
        uint8_t  ad7719_readings_since_last_muxing;
        uint8_t  ad7719_current_channel;
        uint32_t ad7719_current_reading;
        uint8_t  ad7719_enables[kNumResistanceRegs];
        uint8_t  ad7719_channels_ready[kNumResistanceRegs];
        uint32_t ad7719_values[kNumResistanceChannels];
        uint16_t ad7719_values_checksum;

        uint8_t  adc_readings_since_last_muxing;
        uint8_t  adc_current_channel;
        uint16_t adc_current_reading;
        uint8_t  adc_enables[kNumVoltageRegs];
        uint8_t  adc_channels_ready[kNumVoltageRegs];
        uint16_t adc_values[kNumVoltageChannels];
        uint16_t adc_values_checksum;

        uint8_t  ad7719_measured_all;    // treat it as a bool
        uint8_t  adc_measured_all;       // treat it as a bool

        uint8_t  app_reset_source;
        uint32_t time_sec;
        uint16_t time_ms;
    } __attribute__((__packed__));
}

#endif

