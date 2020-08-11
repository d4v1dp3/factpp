#ifndef COSY_Led
#define COSY_Led

#include <stdint.h>

class Ring;

class Led
{
private:
    double fX;
    double fY;
    double fPhi;

    double fMag;

public:
    Led(double x=0, double y=0, double phi=0, double mag=0) :
        fX(x), fY(y), fPhi(phi), fMag(mag)
    {
    }

        /*
    int32_t Compare(const TObject *obj) const
    {
        const Led *const l = (Led*)obj;

        if (fPhi<l->fPhi)
            return -1;

        if (fPhi>l->fPhi)
            return 1;

        return 0;
    }*/

    void SetX(double x)     { fX=x; }
    void SetY(double y)     { fY=y; }
    void SetPhi(double phi) { fPhi=phi; }

    double GetX() const    { return fX; }
    double GetY() const    { return fY; }
    double GetPhi() const  { return fPhi; }
    double GetMag() const  { return fMag; }

    void AddOffset(double dx, double dy) { fX+=dx; fY+=dy; }

    //bool IsSortable() const { return kTRUE; }

    double CalcPhi(const Ring &ring);

    //void Print(Option_t *o=NULL) const;
};

#endif
