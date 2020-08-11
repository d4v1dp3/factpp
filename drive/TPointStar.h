#ifndef COSY_TPointStar
#define COSY_TPointStar

#ifndef ROOT_TNamed
#include <TNamed.h>
#endif

class ZdAz;
class AltAz;

class MPointing;

class TPointStar : public TNamed
{
    friend std::istream &operator>>(std::istream &fin,  TPointStar &set);
    friend std::ostream &operator<<(std::ostream &fout, TPointStar &set);
private:
    Double_t fStarAz;
    Double_t fStarEl;

    Double_t fRawAz;
    Double_t fRawEl;

    Double_t fMag;

    void Init(const char *name=0, const char *title=0);
public:
    TPointStar(const char *name, const char *title=0) { Init(name, title); }
    TPointStar(Double_t sel=0, Double_t saz=0, Double_t rel=0, Double_t raz=0);
    TPointStar(const TPointStar &set) : TNamed(set)
    {
        fStarAz = set.fStarAz;
        fStarEl = set.fStarEl;
        fRawAz  = set.fRawAz;
        fRawEl  = set.fRawEl;
        fMag    = set.fMag;
    }

    Double_t GetMag() const { return fMag; }
    Double_t GetResidual(Double_t *err=0) const;

    Double_t GetDEl() const;//     { return (fRawEl-fStarEl)*TMath::RadToDeg(); }
    Double_t GetDZd() const;//     { return -GetDEl(); }
    Double_t GetDAz() const;//     { return (fRawAz-fStarAz)*TMath::RadToDeg(); }
    Double_t GetStarEl() const;//  { return fStarEl*TMath::RadToDeg(); }
    Double_t GetStarZd() const;//  { return 90.-fStarEl*TMath::RadToDeg(); }
    Double_t GetStarAz() const;//  { return fStarAz*TMath::RadToDeg(); }
    Double_t GetRawEl() const;//   { return fRawEl*TMath::RadToDeg(); }
    Double_t GetRawAz() const;//   { return fRawAz*TMath::RadToDeg(); }
    Double_t GetRawZd() const;//   { return 90.-fRawEl*TMath::RadToDeg(); }

    ZdAz  GetStarZdAz() const;//   { return ZdAz(TMath::Pi()/2-fStarEl, fStarAz); }
    AltAz GetStarAltAz() const;//  { return AltAz(fStarEl, fStarAz); }

    ZdAz  GetRawZdAz() const;//    { return ZdAz(TMath::Pi()/2-fRawEl, fRawAz); }
    AltAz GetRawAltAz() const;//   { return AltAz(fRawEl, fRawAz); }

    void Adjust(const MPointing &bend);
    void AdjustBack(const MPointing &bend);
};

std::istream &operator>>(std::istream &fin, TPointStar &set);
std::ostream &operator<<(std::ostream &out, TPointStar &set);

#endif
