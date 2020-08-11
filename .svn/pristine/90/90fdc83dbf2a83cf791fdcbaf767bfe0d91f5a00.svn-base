#ifndef MARS_MPointing
#define MARS_MPointing

#ifndef ROOT_TArrayD
#include <TArrayD.h>
#endif

#ifndef ROOT_TVector2
#include <TVector2.h>
#endif

#ifndef ROOT_TVector3
#include <TVector3.h>
#endif

#ifndef MARS_MParContainer
#include "MParContainer.h"
#endif

// ---------------------------------------------------

#define XY TVector2

inline TVector2 Div(const TVector2 &v1, const TVector2 &v2)
{
    return TVector2(v1.X()/v2.X(), v1.Y()/v2.Y());
}
inline TVector2 Mul(const TVector2 &v1, const TVector2 &v2)
{
    return TVector2(v1.X()*v2.X(), v1.Y()*v2.Y());
}

inline TVector2 operator-(const TVector2 &v) { return TVector2(-v.X(), -v.Y()); }

class AltAz : public XY
{
public:
    AltAz(double alt=0, double az=0) : XY(alt, az) {}

    double Alt() const { return fX; }
    double Az()  const { return fY; }

    void operator*=(double c) { fX*=c; fY*=c; }
    void operator/=(double c) { fX*=c; fY*=c; }

    void Alt(double d) { fX=d; }
    void Az(double d)  { fY=d; }
    void operator*=(const XY &c)    { fX*=c.X(); fY*=c.Y(); }
    void operator/=(const XY &c)    { fX/=c.X(); fY/=c.Y(); }
    void operator-=(const AltAz &c) { fX-=c.fX; fY-=c.fY; }
    void operator+=(const AltAz &c) { fX+=c.fX; fY+=c.fY; }

    AltAz operator/(double c) const { return AltAz(fX/c, fY/c); }
    AltAz operator*(double c) const { return AltAz(fX*c, fY*c); }
    AltAz operator*(const XY &c) const { return AltAz(fX*c.X(), fY*c.Y()); }
    AltAz operator/(const XY &c) const { return AltAz(fX/c.X(), fY/c.Y()); }
    AltAz operator+(const AltAz &c) const { return AltAz(fX+c.fX, fY+c.fY); }
    AltAz operator-(const AltAz &c) const { return AltAz(fX-c.fX, fY-c.fY); }
    AltAz operator-() const { return AltAz(-fX, -fY); }

    ClassDef(AltAz, 0)
};

class ZdAz : public XY
{
public:
    ZdAz(double zd=0, double az=0) : XY(zd, az) {}
    ZdAz(const ZdAz &c) : XY(c) {}

    void operator*=(double c) { fX*=c; fY*=c; }
    void operator/=(double c) { fX*=c; fY*=c; }

    double Zd() const { return fX; }
    double Az() const { return fY; }

    void Zd(double d) { fX=d; }
    void Az(double d) { fY=d; }
    void operator*=(const XY &c)   { fX*=c.X(); fY*=c.Y(); }
    void operator/=(const XY &c)   { fX/=c.X(); fY/=c.Y(); }
    void operator-=(const ZdAz &c) { fX-=c.fX; fY-=c.fY; }
    void operator+=(const ZdAz &c) { fX+=c.fX; fY+=c.fY; }

    ZdAz operator/(double c) const { return ZdAz(fX/c, fY/c); }
    ZdAz operator*(double c) const { return ZdAz(fX*c, fY*c); }
    ZdAz operator*(const XY &c) const { return ZdAz(fX*c.X(), fY*c.Y()); }
    ZdAz operator/(const XY &c) const { return ZdAz(fX/c.X(), fY/c.Y()); }
    ZdAz operator+(const ZdAz &c) const { return ZdAz(fX+c.fX, fY+c.fY); }
    ZdAz operator-(const ZdAz &c) const { return ZdAz(fX-c.fX, fY-c.fY); }
    ZdAz operator-() const { return ZdAz(-fX, -fY); }

    // MSlewing only?!?
    double Ratio() const { return fX/fY; }
    void Round();
    void Abs();

    ClassDef(ZdAz, 0)
};

class RaDec : public XY
{
public:
    RaDec(double ra=0, double dec=0) : XY(ra, dec) {}

    double Ra()  const { return fX; }
    double Dec() const { return fY; }

    void operator*=(double c) { fX*=c; fY*=c; }
    void operator/=(double c) { fX*=c; fY*=c; }

    void Ra(double x)  { fX = x; }
    void Dec(double y) { fY = y; }

    RaDec operator/(double c) const { return RaDec(fX/c, fY/c); }
    RaDec operator*(double c) const { return RaDec(fX*c, fY*c); }
    RaDec operator*(const XY &c) const { return RaDec(fX*c.X(), fY*c.Y()); }
    RaDec operator+(const RaDec &c) const { return RaDec(fX+c.fX, fY+c.fY); }
    RaDec operator-(const RaDec &c) const { return RaDec(fX-c.fX, fY-c.fY); }
    RaDec operator-() const { return RaDec(-fX, -fY); }

    ClassDef(RaDec, 0)
};

// ---------------------------------------------------

class TMinuit;

class MPointing : public MParContainer
{
private:
    enum {
        kIA,           // [rad] Index Error in Elevation
        kIE,           // [rad] Index Error in Azimuth
        kFLOP,         // [rad] Vertical Sag
        kAN,           // [rad] Az-El Nonperpendicularity
        kAW,           // [rad] Left-Right Collimation Error
        kNPAE,         // [rad] Azimuth Axis Misalignment (N-S)
        kCA,           // [rad] Azimuth Axis Misalignment (E-W)
        kTF,           // [rad] Tube fluxture (sin)
        kTX,           // [rad] Tube fluxture (tan)
        kECES,         // [rad] Nasmyth rotator displacement, horizontal
        kACES,         // [rad] Nasmyth rotator displacement, vertical
        kECEC,         // [rad] Alt/Az Coude Displacement (N-S)
        kACEC,         // [rad] Alt/Az Coude Displacement (E-W)
        kNRX,          // [rad] Elevation Centering Error (sin)
        kNRY,          // [rad] Azimuth Centering Error (sin)
        kCRX,          // [rad] Elevation Centering Error (cos)
        kCRY,          // [rad] Azimuth Centering Error (cos)
        kMAGIC1,       // [rad] Magic Term (what is it?)
        kMAGIC2,       // [rad] Magic Term (what is it?)
        kPX,           // [rad] Starguider calibration fixed offset x
        kPY,           // [rad] Starguider calibration fixed offset y
        kDX,           // [rad] Starguider calibration additional offset dx
        kDY,           // [rad] Starguider calibration additional offset dy
        kNumPar   // Number of elements
    };


    Double_t fIe   ; // [rad] Index Error in Elevation
    Double_t fIa   ; // [rad] Index Error in Azimuth
    Double_t fFlop ; // [rad] Vertical Sag
    Double_t fNpae ; // [rad] Az-El Nonperpendicularity
    Double_t fCa   ; // [rad] Left-Right Collimation Error
    Double_t fAn   ; // [rad] Azimuth Axis Misalignment (N-S)
    Double_t fAw   ; // [rad] Azimuth Axis Misalignment (E-W)
    Double_t fTf   ; // [rad] Tube fluxture (sin)
    Double_t fTx   ; // [rad] Tube fluxture (tan)
    Double_t fNrx  ; // [rad] Nasmyth rotator displacement, horizontal
    Double_t fNry  ; // [rad] Nasmyth rotator displacement, vertical
    Double_t fCrx  ; // [rad] Alt/Az Coude Displacement (N-S)
    Double_t fCry  ; // [rad] Alt/Az Coude Displacement (E-W)
    Double_t fEces ; // [rad] Elevation Centering Error (sin)
    Double_t fAces ; // [rad] Azimuth Centering Error (sin)
    Double_t fEcec ; // [rad] Elevation Centering Error (cos)
    Double_t fAcec ; // [rad] Azimuth Centering Error (cos)
    Double_t fMagic1; // [rad] Magic Term (what is it?)
    Double_t fMagic2; // [rad] Magic Term (what is it?)

    Double_t fPx;    // [rad] Starguider calibration fixed offset x
    Double_t fPy;    // [rad] Starguider calibration fixed offset y
    Double_t fDx;    // [rad] Starguider calibration additional offset dx
    Double_t fDy;    // [rad] Starguider calibration additional offset dy

    Double_t **fCoeff; //!
    TString   *fNames; //!
    TString   *fDescr; //!

    TArrayD   fError;

    void Init(const char *name=0, const char *title=0);

    void Clear(Option_t *o="")
    {
        for (int i=0; i<kNumPar; i++)
        {
            *fCoeff[i] = 0;
            fError[i] = -1;
        }
    }

    static Double_t Sign(Double_t val, Double_t alt);
    AltAz CalcAnAw(const AltAz &p, Int_t sign) const;

public:
    MPointing() : fError(kNumPar) { Init(); Clear(); }
    MPointing(const char *name) : fError(kNumPar) { Init(); Clear(); Load(name); }
    virtual ~MPointing() { delete [] fNames; delete [] fCoeff; delete [] fDescr; }

    Bool_t Load(const char *name);
    Bool_t Save(const char *name);

    void Reset();

    ZdAz     Correct(const ZdAz &zdaz) const;
    AltAz    Correct(const AltAz &aaz) const;
    TVector3 Correct(const TVector3 &v) const;

    ZdAz     CorrectBack(const ZdAz &zdaz) const;
    AltAz    CorrectBack(const AltAz &aaz) const;
    TVector3 CorrectBack(const TVector3 &v) const;

    ZdAz     operator()(const ZdAz &zdaz)  const { return Correct(zdaz); }
    AltAz    operator()(const AltAz &aaz)  const { return Correct(aaz); }
    TVector3 operator()(const TVector3 &v) const { return Correct(v); }

    ZdAz operator()(const ZdAz &zdaz, void (*fcn)(ZdAz &zdaz, Double_t *par)) const
    {
        Double_t par[kNumPar];
        GetParameters(par);
        ZdAz za = zdaz;
        fcn(za, par);
        return za;
    }

    AltAz operator()(const AltAz &aaz, void (*fcn)(AltAz &aaz, Double_t *par)) const
    {
        Double_t par[kNumPar];
        GetParameters(par);
        AltAz aa = aaz;
        fcn(aa, par);
        return aa;
    }

    TVector3 operator()(const TVector3 &aaz, void (*fcn)(TVector3 &aaz, Double_t *par)) const
    {
        Double_t par[kNumPar];
        GetParameters(par);
        TVector3 v = aaz;
        fcn(v, par);
        return v;
    }

    AltAz    AddOffsets(const AltAz &aa) const;
    ZdAz     AddOffsets(const ZdAz &zdaz) const;
    TVector3 AddOffsets(const TVector3 &v) const;

    AltAz    SubtractOffsets(const AltAz &aa) const;
    ZdAz     SubtractOffsets(const ZdAz &zdaz) const;
    TVector3 SubtractOffsets(const TVector3 &v) const;

    void SetParameters(const Double_t *par, Int_t n=kNumPar);
    void GetParameters(Double_t *par, Int_t n=kNumPar) const;

    void SetParameters(const TArrayD &par)
    {
        SetParameters(par.GetArray(), par.GetSize());
    }
    void GetParameters(TArrayD &par) const
    {
        par.Set(kNumPar);
        GetParameters(par.GetArray());
    }
    void GetError(TArrayD &par) const;

    Double_t &operator[](UInt_t i) { return *fCoeff[i]; }

    void SetMinuitParameters(TMinuit &m, Int_t n=-1) const;
    void GetMinuitParameters(TMinuit &m, Int_t n=-1);
    void PrintMinuitParameters(TMinuit &m, Int_t n=-1) const;

    const TString &GetVarName(int i) const { return fNames[i]; }
    const TString &GetDescription(int i) const { return fDescr[i]; }

    /*
     Double_t GetIe() const { return fIe; }
     Double_t GetIa() const { return fIa; }
     Double_t GetCa() const { return fCa; }
     Double_t GetAn() const { return fAn; }
     Double_t GetAw() const { return fAw; }
     Double_t GetNrx() const { return fNrx; }
     Double_t GetNry() const { return fNry; }
     Double_t GetCrx() const { return fNrx; }
     Double_t GetCry() const { return fNry; }
     Double_t GetEces() const { return fEces; }
     Double_t GetEcec() const { return fEcec; }
     Double_t GetAces() const { return fAces; }
     Double_t GetAcec() const { return fAcec; }
     Double_t GetNpae() const { return fNpae; }
     */

    TVector2 GetDxy() const;// { return TVector2(fDx, fDy)*TMath::RadToDeg(); }

    Double_t GetPx() const;// { return fPx*TMath::RadToDeg(); }
    Double_t GetPy() const;// { return fPy*TMath::RadToDeg(); }

    Bool_t IsPxValid() const { return fError[kPX]>0; }
    Bool_t IsPyValid() const { return fError[kPY]>0; }

    static const Int_t GetNumPar() { return kNumPar; }

    ClassDef(MPointing, 2) // Pointing Model for MAGIC
};

#endif
