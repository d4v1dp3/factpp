#include "TPointStar.h"

#include <iostream>
#include <TMath.h>

#include "MPointing.h"

using namespace std;

void TPointStar::Init(const char *name, const char *title)
{
    fName  = name  ? name  : "TPointStar";
    fTitle = title ? title : "A set of TPoints";
}

TPointStar::TPointStar(Double_t sel, Double_t saz, Double_t rel, Double_t raz) :
    fStarAz(saz*TMath::DegToRad()),
    fStarEl(sel*TMath::DegToRad()),
    fRawAz(raz*TMath::DegToRad()),
    fRawEl(rel*TMath::DegToRad()), fMag(-25)
{
    Init();
}

Double_t TPointStar::GetDEl() const     { return (fRawEl-fStarEl)*TMath::RadToDeg(); }
Double_t TPointStar::GetDZd() const     { return -GetDEl(); }
Double_t TPointStar::GetDAz() const     { return (fRawAz-fStarAz)*TMath::RadToDeg(); }
Double_t TPointStar::GetStarEl() const  { return fStarEl*TMath::RadToDeg(); }
Double_t TPointStar::GetStarZd() const  { return 90.-fStarEl*TMath::RadToDeg(); }
Double_t TPointStar::GetStarAz() const  { return fStarAz*TMath::RadToDeg(); }
Double_t TPointStar::GetRawEl() const   { return fRawEl*TMath::RadToDeg(); }
Double_t TPointStar::GetRawAz() const   { return fRawAz*TMath::RadToDeg(); }
Double_t TPointStar::GetRawZd() const   { return 90.-fRawEl*TMath::RadToDeg(); }

ZdAz  TPointStar::GetStarZdAz() const   { return ZdAz(TMath::Pi()/2-fStarEl, fStarAz); }
AltAz TPointStar::GetStarAltAz() const  { return AltAz(fStarEl, fStarAz); }

ZdAz  TPointStar::GetRawZdAz() const    { return ZdAz(TMath::Pi()/2-fRawEl, fRawAz); }
AltAz TPointStar::GetRawAltAz() const   { return AltAz(fRawEl, fRawAz); }

void TPointStar::Adjust(const MPointing &bend)
{
    AltAz p = bend(GetStarAltAz());
    fStarEl = p.Alt();
    fStarAz = p.Az();
}

void TPointStar::AdjustBack(const MPointing &bend)
{
    AltAz p = bend.CorrectBack(GetRawAltAz());
    fRawEl = p.Alt();
    fRawAz = p.Az();
}

Double_t TPointStar::GetResidual(Double_t *err) const
{
    const Double_t del = fRawEl-fStarEl;
    const Double_t daz = fRawAz-fStarAz;

    const double x = cos(fRawEl) * cos(fStarEl) * cos(fStarAz-fRawAz);
    const double y = sin(fRawEl) * sin(fStarEl);

    const Double_t d = x + y;

    if (err)
    {
        // Error of one pixel in the CCD
        const Double_t e1 = 45./3600*TMath::DegToRad()  /4 * 0.5;

        // Error of the SE readout
        const Double_t e2 = 360./16384*TMath::DegToRad()/4 * 0.5;

        const Double_t e11 =  sin(del)+cos(fRawEl)*sin(fStarEl)*(1-cos(daz));
        const Double_t e12 =  cos(fRawEl)*cos(fStarEl)*sin(daz);

        const Double_t e21 = -sin(del)+sin(fRawEl)*cos(fStarEl)*(1-cos(daz));
        const Double_t e22 = -cos(fRawEl)*cos(fStarEl)*sin(daz);

        const Double_t err1  = sqrt(1-d*d);
        const Double_t err2  = (e11*e11 + e12*e12)*e1*e1;
        const Double_t err3  = (e21*e21 + e22*e22)*e2*e2;

        *err = sqrt(err2+err3)/err1 * TMath::RadToDeg();
    }

    const Double_t dist = acos(d);
    return dist * TMath::RadToDeg();
}

istream &operator>>(istream &fin, TPointStar &set)
{
    TString str;
    do
    {
        str.ReadLine(fin);
        if (!fin)
            return fin;
    } while (str[0]=='#');

    Float_t v[4], mag;
    Int_t n = sscanf(str.Data(), "%f %f %f %f %*f %*f %*f %*f %*f %*f %f", v, v+1, v+2, v+3, &mag);
    if (n<4)
    {
        cout << "Read: ERROR - Not enough numbers" << endl;
        return fin;
    }
    set.fMag = n<5 ? -25 : mag;

    set.fStarAz = v[0]*TMath::DegToRad();
    set.fStarEl = v[1]*TMath::DegToRad();

    set.fRawAz  = v[2]*TMath::DegToRad();
    set.fRawEl  = v[3]*TMath::DegToRad();



    if (fin)
    {
        Double_t res, err;
        res = set.GetResidual(&err);
        cout << "Read: " << v[0] << " " << v[1] << "  :  " << v[2] << " " << v[3] << "  :  " << v[2]-v[0] << " " << v[3]-v[1] << "  :  " << res << " " << err << " " << err/res << endl;
    }

    return fin;
}

ostream &operator<<(ostream &out, TPointStar &set)
{
    out << Form("%8.3f", set.fStarAz*TMath::RadToDeg()) << " ";
    out << Form("%7.3f", set.fStarEl*TMath::RadToDeg()) << "   ";
    out << Form("%8.3f", set.fRawAz*TMath::RadToDeg()) << " ";
    out << Form("%7.3f", set.fRawEl*TMath::RadToDeg()) << "   ";
    out << Form("%6.3f", set.fMag);

    return out;
}
