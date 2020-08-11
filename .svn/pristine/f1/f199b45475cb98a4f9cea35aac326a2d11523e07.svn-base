#include "Ring.h"

#include <iostream>

#include <math.h>

#include "Led.h"

using namespace std;

Ring::Ring(double x, double y) :
    fX(x), fY(y), fR(0), fPhi(0)
{
}

bool Ring::CalcCenter(Led i, Led j, Led k)
{
    double h1 = i.GetY() - j.GetY();

    if (h1==0)
    {
        std::swap(j, k);
        h1 = i.GetY() - j.GetY();
        if (h1==0)
        {
            cout << "Ring::CalcCenter: h1==0" <<endl;
            return false;
        }
    }

    double h2 = j.GetY() - k.GetY();

    if (h2==0)
    {
        std::swap(i, j);
        h2 = j.GetY() - k.GetY();
        if (h2==0)
        {
            cout << "Ring::CalcCenter: h2==0" << endl;
            return false;
        }
    }

    const double w1 = i.GetX() - j.GetX();
    const double w2 = j.GetX() - k.GetX();

    const double m1 = -w1/h1;
    const double m2 = -w2/h2;

    if (m2 - m1==0)
    {
        cout << "Ring::CalcCenter: All three points in a row! (m2-m1==0)" << endl;
        return false;
    }

    fX = ((m2*(j.GetX() + k.GetX()) + i.GetY() - k.GetY()        -m1*(i.GetX() + j.GetX()))/(m2-m1)/2);
    fY = ((m2*(i.GetY() + j.GetY()) + m1*m2*(k.GetX() - i.GetX())-m1*(j.GetY() + k.GetY()))/(m2-m1)/2);

    fR = hypot(fX - i.GetX(), fY - i.GetY());

    fMag = (i.GetMag() + j.GetMag() + k.GetMag())/3;

    return true;
}

void Ring::InterpolCenters(const vector<Ring> &rings)
{
    fX = 0;
    fY = 0;
    fR = 0;

    fMag=0;

    const int n=rings.size();
    if (n==0)
        return;

    for (auto it=rings.begin(); it!=rings.end(); it++)
    {
        fX   += it->GetX();
        fY   += it->GetY();
        fR   += it->GetR();
        fMag += it->GetMag();
    }

    fX   /= n;
    fY   /= n;
    fR   /= n;
    fMag /= n;
}
