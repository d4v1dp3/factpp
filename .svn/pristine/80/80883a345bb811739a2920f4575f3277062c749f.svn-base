#ifndef FACT_HeadersDrive
#define FACT_HeadersDrive

namespace Drive
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kLocked,
            kUnavailable,    // IndraDrives not connected
            kAvailable,      // IndraDrives connected, but not in Af
            kBlocked,        // Drive blocked by manual operation of emergency button
            kArmed,          // IndraDrives Af, not yet initialized
            kInitialized,    // IndraDrives Af, initialized
            kStopping,
            kParking,
            kMoving,
            kApproaching,
            kTracking,
            kOnTrack,

            kPositioningFailed = /*StateMachineImp::kSM_Error*/0x100+1,
            kAllowedRangeExceeded,
            kInvalidCoordinates,
            //kSpeedLimitExceeded,

            kHardwareWarning = 0x1ff,
            kHardwareError,
        };
    };

    struct DimPointing
    {
    } __attribute__((__packed__));

    struct DimTracking
    {
    } __attribute__((__packed__));
/*
    struct DimStarguider
    {
        double fMissZd;
        double fMissAz;

        double fNominalZd;
        double fNominalAz;

        double fCenterX;
        double fCenterY;

        double fBrightness;

        uint16_t fNumCorrelated;
        uint16_t fNumLeds;
        uint16_t fNumRings;
        uint16_t fNumStars;

    } __attribute__((__packed__));
*/
    struct DimTPoint
    {
        double fRa;
        double fDec;

        double fNominalZd;
        double fNominalAz;

        double fPointingZd;
        double fPointingAz;

        double fFeedbackZd;
        double fFeedbackAz;

        uint16_t fNumLeds;
        uint16_t fNumRings;
 
        double fCenterX;
        double fCenterY;
        double fCenterMag;

        double fStarX;
        double fStarY;
        double fStarMag;

        double fRotation;

        double fDx;
        double fDy;

        double fRealMag;

    } __attribute__((__packed__));
};
#endif
