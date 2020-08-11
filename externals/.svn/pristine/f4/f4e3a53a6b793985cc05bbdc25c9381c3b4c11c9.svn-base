#ifndef MARS_Prediction
#define MARS_Prediction

#include <math.h>

#include "nova.h"

namespace FACT
{
    double PredictI(const Nova::SolarObjects &so, const Nova::EquPosn &srcEqu)
    {
        // Derived moon properties
        const double angle = Nova::GetAngularSeparation(so.fMoonEqu, srcEqu);
        const double edist = so.fEarthDist/384400;

        // Current prediction
        const double sin_malt  = so.fMoonHrz.alt<0 ? 0 : sin(so.fMoonHrz.alt*M_PI/180);
        const double cos_mdist = cos(angle*M_PI/180);
        const double cos_salt  = cos(so.fSunHrz.alt*M_PI/180);

        const double c0 = pow(so.fMoonDisk, 2.63);
        const double c1 = pow(sin_malt,     0.60);
        const double c2 = pow(edist,       -2.00);
        const double c3 = exp(0.67*cos_mdist*cos_mdist*cos_mdist*cos_mdist);
        const double c4 = exp(-97.8+105.8*cos_salt*cos_salt);

        return  6.2 + 95.7*c0*c1*c2*c3 + c4; // [muA]
    }

    double PredictI(const double &jd, const Nova::EquPosn &srcEqu)
    {
        return PredictI(Nova::SolarObjects(jd), srcEqu);
    }
}

#endif
