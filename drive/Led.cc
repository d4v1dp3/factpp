#include "Led.h"

#include <math.h>

#include "Ring.h"

using namespace std;

double Led::CalcPhi(const Ring &ring)
{
    return atan2(fY-ring.GetY(), fX-ring.GetX())*180/M_PI;
}

/*
void Led::Print(Option_t *o) const
{
    cout << "Led: ";
    //cout << "x="   << MString::Format("%5.1f", fX)   << "+-" << fDx   << ", ";
    //cout << "y="   << MString::Format("%5.1f", fY)   << "+-" << fDy   << ", ";
    //cout << "phi=" << MString::Format("%6.1f", fPhi) << "+-" << fDphi << ", ";
    cout << "mag=" << fMag << endl;
}
*/
