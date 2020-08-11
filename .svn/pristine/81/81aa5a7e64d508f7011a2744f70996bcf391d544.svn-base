/* ======================================================================== *\
!
! *
! * This file is part of MARS, the MAGIC Analysis and Reconstruction
! * Software. It is distributed to you in the hope that it can be a useful
! * and timesaving tool in analysing Data of imaging Cerenkov telescopes.
! * It is distributed WITHOUT ANY WARRANTY.
! *
! * Permission to use, copy, modify and distribute this software and its
! * documentation for any purpose is hereby granted without fee,
! * provided that the above copyright notice appear in all copies and
! * that both that copyright notice and this permission notice appear
! * in supporting documentation. It is provided "as is" without express
! * or implied warranty.
! *
!
!
!   Author(s): Thomas Bretz, 2003 <mailto:tbretz@astro.uni-wuerzburg.de>
!
!   Copyright: MAGIC Software Development, 2000-2007
!
!
\* ======================================================================== */

/////////////////////////////////////////////////////////////////////////////
//
// MPointing
// =========
//
// This is the class used for the pointing correction done in the MAGIC
// drive software cosy. NEVER CHANGE IT WITHOUT CONTACTING THE AUTHOR FIRST!
//
// Variables/Coefficients
// ----------------------
//
//    Double_t fIe   ; // [rad] Index Error in Elevation
//    Double_t fIa   ; // [rad] Index Error in Azimuth
//
//    Double_t fFlop ; // [rad] Vertical Sag
//     * do not use if not data: Zd<0
//
//    Double_t fNpae ; // [rad] Az-El Nonperpendicularity
//
//    Double_t fCa   ; // [rad] Left-Right Collimation Error
//
//    Double_t fAn   ; // [rad] Azimuth Axis Misalignment (N-S)
//    Double_t fAw   ; // [rad] Azimuth Axis Misalignment (E-W)
//
//    Double_t fTf   ; // [rad] Tube fluxture (sin)
//     * same as ecec if no data: Zd<0
//    Double_t fTx   ; // [rad] Tube fluxture (tan)
//     * do not use with NPAE if no data: Zd<0
//
//    Double_t fNrx  ; // [rad] Nasmyth rotator displacement, horizontan
//    Double_t fNry  ; // [rad] Nasmyth rotator displacement, vertical
//
//    Double_t fCrx  ; // [rad] Alt/Az Coude Displacement (N-S)
//    Double_t fCry  ; // [rad] Alt/Az Coude Displacement (E-W)
//
//    Double_t fEces ; // [rad] Elevation Centering Error (sin)
//    Double_t fAces ; // [rad] Azimuth Centering Error (sin)
//    Double_t fEcec ; // [rad] Elevation Centering Error (cos)
//    Double_t fAcec ; // [rad] Azimuth Centering Error (cos)
//
//    Double_t fMagic1;// [rad] MAGIC culmination hysteresis
//    Double_t fMagic2;// [rad] undefined
//
//    Double_t fDx;    // [rad] X-offset in camera (for starguider calibration)
//    Double_t fDy;    // [rad] Y-offset in camera (for starguider calibration)
//
//
//  Class Version 2:
//  ----------------
//    + fPx
//    + fPy
//    + fDx
//    + fDy
//
//
////////////////////////////////////////////////////////////////////////////
#include "MPointing.h"

#include <fstream>

#include <TMinuit.h>

#include "MLog.h"
#include "MLogManip.h"

#include "MTime.h"

ClassImp(AltAz);
ClassImp(ZdAz);
ClassImp(RaDec);
ClassImp(MPointing);

using namespace std;

#undef DEBUG
//#define DEBUG(txt) txt
#define DEBUG(txt)

void ZdAz::Round()
{
    fX = TMath::Nint(fX);
    fY = TMath::Nint(fY);
}

void ZdAz::Abs()
{
    fX = TMath::Abs(fX);
    fY = TMath::Abs(fY);
}

void MPointing::Init(const char *name, const char *title)
{
    fName  = name  ? name  : "MPointing";
    fTitle = title ? title : "Pointing correction model for the MAGIC telescope";

    fCoeff = new Double_t*[kNumPar];
    fNames = new TString[kNumPar];
    fDescr = new TString[kNumPar];

    fCoeff[kIA]     = &fIa;      fNames[kIA]     = "IA";
    fCoeff[kIE]     = &fIe;      fNames[kIE]     = "IE";
    fCoeff[kFLOP]   = &fFlop;    fNames[kFLOP]   = "FLOP";
    fCoeff[kAN]     = &fAn;      fNames[kAN]     = "AN";
    fCoeff[kAW]     = &fAw;      fNames[kAW]     = "AW";
    fCoeff[kNPAE]   = &fNpae;    fNames[kNPAE]   = "NPAE";
    fCoeff[kCA]     = &fCa;      fNames[kCA]     = "CA";
    fCoeff[kTF]     = &fTf;      fNames[kTF]     = "TF";
    fCoeff[kTX]     = &fTx;      fNames[kTX]     = "TX";
    fCoeff[kECES]   = &fEces;    fNames[kECES]   = "ECES";
    fCoeff[kACES]   = &fAces;    fNames[kACES]   = "ACES";
    fCoeff[kECEC]   = &fEcec;    fNames[kECEC]   = "ECEC";
    fCoeff[kACEC]   = &fAcec;    fNames[kACEC]   = "ACEC";
    fCoeff[kNRX]    = &fNrx;     fNames[kNRX]    = "NRX";
    fCoeff[kNRY]    = &fNry;     fNames[kNRY]    = "NRY";
    fCoeff[kCRX]    = &fCrx;     fNames[kCRX]    = "CRX";
    fCoeff[kCRY]    = &fCry;     fNames[kCRY]    = "CRY";
    fCoeff[kMAGIC1] = &fMagic1;  fNames[kMAGIC1] = "MAGIC1";
    fCoeff[kMAGIC2] = &fMagic2;  fNames[kMAGIC2] = "MAGIC2";
    fCoeff[kPX]     = &fPx;      fNames[kPX]     = "PX";
    fCoeff[kPY]     = &fPy;      fNames[kPY]     = "PY";
    fCoeff[kDX]     = &fDx;      fNames[kDX]     = "DX";
    fCoeff[kDY]     = &fDy;      fNames[kDY]     = "DY";

    fDescr[kIA]     =  "Index Error Azimuth";
    fDescr[kIE]     =  "Index Error Zenith Distance";
    fDescr[kFLOP]   =  "Vertical Sag";
    fDescr[kAN]     =  "Azimuth Axis Misalignment (N-S)";
    fDescr[kAW]     =  "Azimuth Axis Misalignment (E-W)";
    fDescr[kNPAE]   =  "Az-El Nonperpendicularity";
    fDescr[kCA]     =  "Left-Right Collimation Error";
    fDescr[kTF]     =  "Tube fluxture (sin)";
    fDescr[kTX]     =  "Tube fluxture (tan)";
    fDescr[kECES]   =  "Elevation Centering Error (sin)";
    fDescr[kACES]   =  "Azimuth Centering Error (sin)";
    fDescr[kECEC]   =  "Elevation Centering Error (cos)";
    fDescr[kACEC]   =  "Azimuth Centering Error (cos)";
    fDescr[kNRX]    =  "Nasmyth rotator displacement (horizontal)";
    fDescr[kNRY]    =  "Nasmyth rotator displacement (vertical)";
    fDescr[kCRX]    =  "Alt/Az Coude Displacement (N-S)";
    fDescr[kCRY]    =  "Alt/Az Coude Displacement (E-W)";
    fDescr[kMAGIC1] =  "MAGIC culmination hysteresis";
    fDescr[kMAGIC2] =  "n/a";
    fDescr[kPX]     =  "Starguider calibration fixed offset x";
    fDescr[kPY]     =  "Starguider calibration fixed offset y";
    fDescr[kDX]     =  "Starguider calibration additional offset dx";
    fDescr[kDY]     =  "Starguider calibration additional offset dy";
}

void MPointing::Reset()
{
    Clear();
}

Bool_t MPointing::Load(const char *name)
{
    /*
     ! MMT 1987 July 8
     ! T   36   7.3622   41.448  -0.0481
     !   IA        -37.5465    20.80602
     !   IE        -13.9180     1.25217
     !   NPAE       +7.0751    26.44763
     !   CA         -6.9149    32.05358
     !   AN         +0.5053     1.40956
     !   AW         -2.2016     1.37480
     ! END
     */

    ifstream fin(name);
    if (!fin)
    {
        *fLog << err << "ERROR - Cannot open file '" << name << "'" << endl;
        return kFALSE;
    }

    char c;
    while (fin && fin.get()!='\n');
    fin >> c;

    if (c!='S' && c!='s')
    {
        *fLog << err << "Error: This in not a model correcting the star position (" << c << ")" << endl;
        return kFALSE;
    }

    Clear();

    cout << endl;

    Double_t val;
    fin >> val;
    *fLog << inf;
    //*fLog << "Number of observed stars: " << val << endl;
    fin >> val;
    //*fLog << "Sky RMS: " << val << "\"" << endl;
    fin >> val;
    //*fLog << "Refraction Constant A: " << val << "\"" << endl;
    fin >> val;
    //*fLog << "Refraction Constant B: " << val << "\"" << endl;

    *fLog << endl;

    *fLog << "  & = Name            Value                 Sigma " << endl;
    *fLog << "--------------------------------------------------" << endl;

    while (fin)
    {
        TString str;
        fin >> str;
        if (!fin)
        {
            *fLog << err << "ERROR - Reading file " << name << endl;
            return kFALSE;
        }

        str = str.Strip(TString::kBoth);

        if (str=="END")
            break;

        TString sout;

        if (str[0]=='#')
            continue;

        if (str[0]=='&')
        {
            sout += " & ";
            str.Remove(0);
        }
        else
            sout += "   ";

        if (str[1]=='=')
        {
            sout += "=  ";
            str.Remove(0);
        }
        else
            sout += "   ";

        fin >> val;

        sout += str;
        sout += '\t';
        sout += Form("%11f", val);
        sout += UTF8::kDeg;
        sout += "     \t";
        val *= TMath::DegToRad();

        // Find parameter
        Int_t n = -1;
        for (int i=0; i<kNumPar; i++)
            if (str==fNames[i])
            {
                n = i;
                *fCoeff[i] = val;
                break;
            }

        fin >> val;
        sout += Form("%9f%s", val, UTF8::kDeg);

        if (*fCoeff[n]!=0 || val>0)
            *fLog << sout << endl;

        if (!fin)
        {
            *fLog << err << "ERROR - Reading line " << str << endl;
            return kFALSE;
        }

        if (n<0)
        {
            *fLog << warn << "WARNING - Parameter " << str << " unknown." << endl;
            continue;
        }

        // corresponding error
        fError[n] = val*TMath::DegToRad();
    }
    *fLog << endl;

    fName = name;

    return kTRUE;
}

Bool_t MPointing::Save(const char *name)
{
    /*
     ! MMT 1987 July 8
     ! T   36   7.3622   41.448  -0.0481
     !   IA        -37.5465    20.80602
     !   IE        -13.9180     1.25217
     !   NPAE       +7.0751    26.44763
     !   CA         -6.9149    32.05358
     !   AN         +0.5053     1.40956
     !   AW         -2.2016     1.37480
     ! END
     */

    ofstream fout(name);
    if (!fout)
    {
        cout << "Error: Cannot open file '" << name << "'" << endl;
        return kFALSE;
    }

    MTime t;
    t.Now();

    fout << "MAGIC1 " << t << endl;
    fout << "S   00   000000   000000  0000000" << endl;
    fout << setprecision(8);
    for (int i=0; i<kNumPar; i++)
    {
        fout << " " << setw(6) << GetVarName(i) << " ";
        fout << setw(13) << *fCoeff[i]*kRad2Deg << "   ";
        fout << setw(11) << fError[i]*kRad2Deg << endl;
    }
    fout << "END" << endl;

    fName = name;

    return kTRUE;
}

Double_t MPointing::Sign(Double_t val, Double_t alt)
{
    // Some pointing corrections are defined as Delta ZA, which
    // is (P. Wallace) defined [0,90]deg while Alt is defined
    // [0,180]deg
    return (TMath::Pi()/2-alt < 0 ? -val : val);
}

AltAz MPointing::AddOffsets(const AltAz &aa) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p = aa;

    const AltAz I(fIe, fIa);
    p += I;

    return p;
}

ZdAz MPointing::AddOffsets(const ZdAz &zdaz) const
{
    AltAz p(TMath::Pi()/2-zdaz.Zd(), zdaz.Az());

    AltAz c = AddOffsets(p);

    return ZdAz(TMath::Pi()/2-c.Alt(), c.Az());
}

TVector3 MPointing::AddOffsets(const TVector3 &v) const
{
    AltAz p(TMath::Pi()/2-v.Theta(), v.Phi());
    AltAz c = AddOffsets(p);

    TVector3 rc;
    rc.SetMagThetaPhi(1, TMath::Pi()/2-c.Alt(), c.Az());
    return rc;
}

AltAz MPointing::SubtractOffsets(const AltAz &aa) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p = aa;

    const AltAz I(fIe, fIa);
    p -= I;

    return p;
}

ZdAz MPointing::SubtractOffsets(const ZdAz &zdaz) const
{
    AltAz p(TMath::Pi()/2-zdaz.Zd(), zdaz.Az());

    AltAz c = SubtractOffsets(p);

    return ZdAz(TMath::Pi()/2-c.Alt(), c.Az());
}

TVector3 MPointing::SubtractOffsets(const TVector3 &v) const
{
    AltAz p(TMath::Pi()/2-v.Theta(), v.Phi());
    AltAz c = SubtractOffsets(p);

    TVector3 rc;
    rc.SetMagThetaPhi(1, TMath::Pi()/2-c.Alt(), c.Az());
    return rc;
}

AltAz MPointing::CalcAnAw(const AltAz &p, Int_t sign) const
{
    // Corrections for AN and AW without approximations
    // as done by Patrick Wallace. The approximation cannot
    // be used for MAGIC because the correctioon angle
    // AW (~1.5deg) is not small enough.

    // Vector in cartesian coordinates
    TVector3 v1;

    // Set Azimuth and Elevation
    v1.SetMagThetaPhi(1, TMath::Pi()/2-p.Alt(), p.Az());


    TVector3 v2(v1);
//    cout << sign << endl;

//    cout << "v1: " << v1.Theta()*TMath::RadToDeg() << " " << v1.Phi()*TMath::RadToDeg() << endl;

    // Rotate around the x- and y-axis
    v1.RotateY(sign*fAn);
    v1.RotateX(sign*fAw);

//    cout << "v1: " << v1.Theta()*TMath::RadToDeg() << " " << v1.Phi()*TMath::RadToDeg() << endl;
//    cout << "v2: " << v2.Theta()*TMath::RadToDeg() << " " << v2.Theta()*TMath::RadToDeg() << endl;

   // cout << "dv: " << (v2.Theta()-v1.Theta())*TMath::RadToDeg() << " " << (v2.Phi()-v1.Phi())*TMath::RadToDeg() << endl;

    Double_t dalt = v1.Theta()-v2.Theta();
    Double_t daz  = v1.Phi()  -v2.Phi();

    //cout << dalt*TMath::RadToDeg() << " " << daz*TMath::RadToDeg() << endl;

    if (daz>TMath::Pi())
        daz -= TMath::TwoPi();
    if (daz<-TMath::Pi())
        daz += TMath::TwoPi();

//    if (daz>TMath::Pi()/2)
//    {
//    }

    AltAz d(dalt, daz);
    return d;

    // Calculate Delta Azimuth and Delta Elevation
    /*
    AltAz d(TMath::Pi()/2-v1.Theta(), v1.Phi());

    cout << "p :  " << p.Alt()*TMath::RadToDeg() << " " << p.Az()*TMath::RadToDeg() << endl;
    cout << "d :  " << d.Alt()*TMath::RadToDeg() << " " << d.Az()*TMath::RadToDeg() << endl;
    d -= p;
    cout << "d-p: " << d.Alt()*TMath::RadToDeg() << " " << d.Az()*TMath::RadToDeg() << endl;
    d *= sign;
    cout << "d* : " << d.Alt()*TMath::RadToDeg() << " " << d.Az()*TMath::RadToDeg() << endl;


    cout << "p2:  " << 90-p.Alt()*TMath::RadToDeg() << " " << p.Az()*TMath::RadToDeg() << endl;
    cout << "d2:  " << 90-d.Alt()*TMath::RadToDeg() << " " << d.Az()*TMath::RadToDeg() << endl;

    Int_t s1 = 90-d.Alt()*TMath::RadToDeg() < 0 ? -1 : 1;
    Int_t s2 = 90-p.Alt()*TMath::RadToDeg() < 0 ? -1 : 1;


    if (s1 != s2)
    {
        //90-d.Alt() <-- -90+d.Alt()

        d.Alt(d.Alt()-TMath::Pi());
        cout << "Alt-" << endl;
    }
    cout << "d': " << 90-d.Alt()*TMath::RadToDeg() << " " << d.Az()*TMath::RadToDeg() << endl;*/
 /*
    // Fix 'direction' of output depending on input vector
    if (TMath::Pi()/2-sign*p.Alt()<0)
    {
        d.Alt(d.Alt()-TMath::Pi());
        cout << "Alt-" << endl;
    }
    //if (TMath::Pi()/2-sign*p.Alt()>TMath::Pi())
    //{
    //    d.Alt(TMath::Pi()-d.Alt());
    //    cout << "Alt+" << endl;
    //}

    // Align correction into [-180,180]
    while (d.Az()>TMath::Pi())
    {
        d.Az(d.Az()-TMath::Pi()*2);
        cout << "Az-" << endl;
    }
    while (d.Az()<-TMath::Pi())
    {
        d.Az(d.Az()+TMath::Pi()*2);
        cout << "Az+" << endl;
    }
   */
    return d;
}


AltAz MPointing::Correct(const AltAz &aa) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p = aa;

    DEBUG(cout << setprecision(16));
    DEBUG(cout << "Bend7: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz CRX(-fCrx*sin(p.Az()-p.Alt()),  fCrx*cos(p.Az()-p.Alt())/cos(p.Alt()));
    const AltAz CRY(-fCry*cos(p.Az()-p.Alt()), -fCry*sin(p.Az()-p.Alt())/cos(p.Alt()));
    p += CRX;
    p += CRY;

    DEBUG(cout << "Bend6: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz NRX(fNrx*sin(p.Alt()), -fNrx);
    const AltAz NRY(fNry*cos(p.Alt()), -fNry*tan(p.Alt()));
    p += NRX;
    p += NRY;

    DEBUG(cout << "Bend5: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz CES(-fEces*sin(p.Alt()), -fAces*sin(p.Az()));
    const AltAz CEC(-fEcec*cos(p.Alt()), -fAcec*cos(p.Az()));
    p += CES;
    p += CEC;

    DEBUG(cout << "Bend4: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz TX(Sign(fTx/tan(p.Alt()), p.Alt()), 0);
    const AltAz TF(Sign(fTf*cos(p.Alt()), p.Alt()), 0);
    //p += TX;
    p += TF;


    DEBUG(cout << "Bend3: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    /*
     //New Corrections for NPAE and CA:
     TVector3 v(1.,1.,1.); // Vector in cartesian coordinates

     //Set Azimuth and Elevation
     v.SetPhi(p.Az());
     v.SetTheta(TMath::Pi()/2-p.Alt());
     //Rotation Vectors:
     TVector3 vNpae(             cos(p.Az()),              sin(p.Az()),             0);
     TVector3   vCa( -cos(p.Az())*cos(p.Alt()), -sin(p.Az())*cos(p.Alt()), sin(p.Alt()));
     //Rotate around the vectors vNpae and vCa
     v.Rotate(fNpae, vNpae);
     v.Rotate(fCa,     vCa);

     p.Az(v.Phi());
     p.Alt(TMath::Pi()/2-v.Theta());
     */

    //Old correction terms for Npae and Ca:
    const AltAz CA(0, -fCa/cos(p.Alt()));
    p += CA;

    const AltAz NPAE(0, -fNpae*tan(p.Alt()));
    p += NPAE;

    DEBUG(cout << "Bend2: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz ANAW(CalcAnAw(p, -1));
    p += ANAW;

    /* Old correction terms for An and Aw:
     const AltAz AW( fAw*sin(p.Az()), -fAw*cos(p.Az())*tan(p.Alt()));
     const AltAz AN(-fAn*cos(p.Az()), -fAn*sin(p.Az())*tan(p.Alt()));
     p += AW;
     p += AN;
    */

    DEBUG(cout << "Bend1: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz FLOP(Sign(fFlop, p.Alt()), 0);
    p += FLOP;

    const AltAz MAGIC1(fMagic1*TMath::Sign(1., sin(p.Az())), 0);
    p += MAGIC1;

    const AltAz I(fIe, fIa);
    p += I;

    DEBUG(cout << "Bend0: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    return p;
}

AltAz MPointing::CorrectBack(const AltAz &aa) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p = aa;

    DEBUG(cout << setprecision(16));
    DEBUG(cout << "Back0: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz I(fIe, fIa);
    p -= I;

    const AltAz MAGIC1(fMagic1*TMath::Sign(1., sin(p.Az())), 0);
    p -= MAGIC1;

    //const AltAz MAGIC1(fMagic1*sin(p.Az()), 0);
    //p -= MAGIC1;

    const AltAz FLOP(Sign(fFlop, p.Alt()), 0);
    p -= FLOP;

    DEBUG(cout << "Back1: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    /* Old correction terms for An and Aw:
     const AltAz AN(-fAn*cos(p.Az()), -fAn*sin(p.Az())*tan(p.Alt()));
     const AltAz AW( fAw*sin(p.Az()), -fAw*cos(p.Az())*tan(p.Alt()));
     p -= AN;
     p -= AW;
     */

    const AltAz ANAW(CalcAnAw(p, -1));
    p -= ANAW;

    DEBUG(cout << "Back2: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    //Old Correction terms for Npae and Ca:
    const AltAz NPAE(0, -fNpae*tan(p.Alt()));
    p -= NPAE;

    const AltAz CA(0, -fCa/cos(p.Alt()));
    p -= CA;

    /*
     //New Correction term for Npae and Ca:
     TVector3 v2(1.,1.,1.); // Vector in cartesian coordinates
     //Set Azimuth and Elevation
     v2.SetPhi(p.Az());
     v2.SetTheta(TMath::Pi()/2-p.Alt());
     //Rotation Vectors:
     TVector3 vNpae(             cos(p.Az()),              sin(p.Az()),             0);
     TVector3   vCa( -cos(p.Az())*cos(p.Alt()), -sin(p.Az())*cos(p.Alt()), sin(p.Alt()));
     //Rotate around the vectors vCa and vNpae
     v2.Rotate(-fCa,     vCa);
     v2.Rotate(-fNpae, vNpae);

     p.Az(v2.Phi());
     p.Alt(TMath::Pi()/2-v2.Theta());
    */

    DEBUG(cout << "Back3: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz TF(Sign(fTf*cos(p.Alt()), p.Alt()), 0);
    const AltAz TX(Sign(fTx/tan(p.Alt()), p.Alt()), 0);
    p -= TF;
    //p -= TX;

    DEBUG(cout << "Back4: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz CEC(-fEcec*cos(p.Alt()), -fAcec*cos(p.Az()));
    const AltAz CES(-fEces*sin(p.Alt()), -fAces*sin(p.Az()));
    p -= CEC;
    p -= CES;

    DEBUG(cout << "Back5: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz NRY(fNry*cos(p.Alt()), -fNry*tan(p.Alt()));
    const AltAz NRX(fNrx*sin(p.Alt()), -fNrx);
    p -= NRY;
    p -= NRX;

    DEBUG(cout << "Back6: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    const AltAz CRY(-fCry*cos(p.Az()-p.Alt()), -fCry*sin(p.Az()-p.Alt())/cos(p.Alt()));
    const AltAz CRX(-fCrx*sin(p.Az()-p.Alt()),  fCrx*cos(p.Az()-p.Alt())/cos(p.Alt()));
    p -= CRY;
    p -= CRX;

    DEBUG(cout << "Back7: " << 90-p.Alt()*180/TMath::Pi() << " " << p.Az()*180/TMath::Pi() << endl);

    return p;
}

ZdAz MPointing::Correct(const ZdAz &zdaz) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p(TMath::Pi()/2-zdaz.Zd(), zdaz.Az());
    AltAz c = Correct(p);
    return ZdAz(TMath::Pi()/2-c.Alt(), c.Az());
}

TVector3 MPointing::Correct(const TVector3 &v) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p(TMath::Pi()/2-v.Theta(), v.Phi());
    AltAz c = Correct(p);
    TVector3 rc;
    rc.SetMagThetaPhi(1, TMath::Pi()/2-c.Alt(), c.Az());
    return rc;
}

ZdAz MPointing::CorrectBack(const ZdAz &zdaz) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p(TMath::Pi()/2-zdaz.Zd(), zdaz.Az());
    AltAz c = CorrectBack(p);
    return ZdAz(TMath::Pi()/2-c.Alt(), c.Az());
}

TVector3 MPointing::CorrectBack(const TVector3 &v) const
{
    // Correct [rad]
    // zdaz    [rad]
    AltAz p(TMath::Pi()/2-v.Theta(), v.Phi());
    AltAz c = CorrectBack(p);
    TVector3 rc;
    rc.SetMagThetaPhi(1, TMath::Pi()/2-c.Alt(), c.Az());
    return rc;
}

void MPointing::SetParameters(const Double_t *par, Int_t n)
{
    Clear();

    while (n--)
        *fCoeff[n] = par[n]/kRad2Deg;
}

void MPointing::GetParameters(Double_t *par, Int_t n) const
{
    while (n--)
        par[n] = *fCoeff[n]*kRad2Deg;
}

void MPointing::GetError(TArrayD &par) const
{
    par = fError;
    for (int i=0; i<kNumPar; i++)
        par[i] *= TMath::RadToDeg();
}

TVector2 MPointing::GetDxy() const
{
    return TVector2(fDx, fDy)*TMath::RadToDeg();
}

Double_t MPointing::GetPx() const
{
    return fPx*TMath::RadToDeg();
}

Double_t MPointing::GetPy() const
{
    return fPy*TMath::RadToDeg();
}

void MPointing::SetMinuitParameters(TMinuit &m, Int_t n) const
{
    if (n<0)
        n = kNumPar;

    Int_t ierflg = 0;

    while (n--)
        m.mnparm(n, fNames[n], *fCoeff[n]*kRad2Deg,  1, -360, 360, ierflg);
}

void MPointing::GetMinuitParameters(TMinuit &m, Int_t n)
{
    if (n<0 || n>m.GetNumPars())
        n = m.GetNumPars();

    while (n--)
    {
        m.GetParameter(n, *fCoeff[n], fError[n]);
        *fCoeff[n] /= kRad2Deg;
        fError[n]  /= kRad2Deg;
    }
}
/*
void FormatPar(TMinuit &m, Int_t n)
{
    Double_t par, err;
    m.GetParameter(n, par, err);

    int expp = (int)log10(par);
    int expe = (int)log10(err);

    if (err<2*pow(10, expe))
        expe--;

    Int_t exp = expe>expp ? expp : expe;

    par = (int)(par/pow(10, exp)) * pow(10, exp);
    err = (int)(err/pow(10, exp)) * pow(10, exp);

    cout << par << " +- " << err << flush;
}
*/
void MPointing::PrintMinuitParameters(TMinuit &m, Int_t n) const
{
    if (n<0)
        n = m.GetNumPars();

    cout << setprecision(3);

    Double_t par, er;

    while (n--)
    {
        m.GetParameter(n, par, er);
        cout << Form(" %2d %6s: ", n, (const char*)fNames[n]);
        cout << setw(8) << par << " \xb1 " << setw(6) <<  er << endl;
    }
}
