#ifndef COSY_Ring
#define COSY_Ring

#include <vector>

#include "Led.h"

class Rings;

class Ring
{
private:
    double fX;
    double fY;
    double fR;
    double fPhi;

    double fMag;

    double sqr(double x) { return x*x; }

public:
    Ring(double x=0, double y=0);

    void SetXY(double x=0, double y=0) { fX=x; fY=y; }
    void SetPhi(double phi) { fPhi=phi; }

    double GetX() const   { return fX; }
    double GetY() const   { return fY; }
    double GetR() const   { return fR; }
    double GetPhi() const { return fPhi; }

    double GetMag() const { return fMag; }

    bool CalcCenter(Led, Led, Led);
    void InterpolCenters(const std::vector<Ring> &rings);
};

#endif
