#ifndef FACT_HeadersPower
#define FACT_HeadersPower

#include <iosfwd>
#include <stdint.h>

class QString;
class QDomNamedNodeMap;

namespace Power
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kSystemOff,
            kCameraOn  =  4,
            kBiasOn    =  8,
            kDriveOn   = 16,
            kCameraOff = kBiasOn|kDriveOn,
            kBiasOff   = kCameraOn|kDriveOn,
            kDriveOff  = kCameraOn|kBiasOn,
            kSystemOn  = kCameraOn|kBiasOn|kDriveOn,
            kCoolingFailure
        };
    };

    struct Status
    {
        bool fWaterLevelOk;
        bool fWaterFlowOk;

        bool fPwr24VOn;
        bool fPwrPumpOn;
        bool fPwrBiasOn;
        bool fPwrDriveOn;

        bool fDriveMainSwitchOn;
        bool fDriveFeedbackOn;

        Status() { }

        bool Set(bool &rc, const QString &value);
        bool Set(const QDomNamedNodeMap &map);

        void Print(std::ostream &out, const char *title, const bool &val, const char *t="enabled", const char *f="disabled");
        void Print(std::ostream &out);      

        uint8_t GetVal() const
        {
            return
                fWaterLevelOk      <<0 |
                fWaterFlowOk       <<1 |
                fPwr24VOn          <<2 |
                fPwrPumpOn         <<3 |
                fPwrDriveOn        <<4 |
                fDriveMainSwitchOn <<5 |
                fDriveFeedbackOn   <<6;
        }

    } __attribute__((__packed__));
};
#endif
