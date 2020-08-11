#include "TPointGui.h"

#include <iomanip>
#include <fstream>
#include <stdlib.h>

#include <TROOT.h>
#include <TClass.h>
#include <TSystem.h>

#include <TGLabel.h>
#include <TGButton.h>
#include <TGTextEntry.h>

#include <TView.h>
#include <TStyle.h>
#include <TCanvas.h>

#include <TText.h>
#include <TLine.h>
#include <TMarker.h>
#include <TPolyLine.h>

#include <TF1.h>
#include <TH2.h>
#include <TMath.h>
#include <TMinuit.h>
#include <TProfile.h>
#include <TGraphErrors.h>

#include "TPointStar.h"

using namespace std;

TPointGui::TPointGui(const string fname, const string mod) : TGMainFrame(gClient->GetRoot(), 650, 435, kHorizontalFrame), fExitLoopOnClose(kFALSE),
   fAzMin(0), fAzMax(360), fZdMin(0), fZdMax(90), fMagMax(10), fLimit(0.05)
{
    fCoordinates.SetOwner();
    fOriginal.SetOwner();

    fList = new TList;
    fList->SetOwner();

    gROOT->GetListOfCleanups()->Add(fList);
    fList->SetBit(kMustCleanup);

    fFont = gVirtualX->LoadQueryFont("7x13bold");

    TGLayoutHints *hints0 = new TGLayoutHints(kLHintsExpandY, 7, 5, 5, 0);
    TGLayoutHints *hints1 = new TGLayoutHints(kLHintsExpandX|kLHintsExpandY, 5, 7, 5, 6);
    fList->Add(hints0);
    fList->Add(hints1);

    TGGroupFrame *grp1 = new TGGroupFrame(this, "Control", kVerticalFrame);
    AddFrame(grp1, hints0);
    fList->Add(grp1);

    TGGroupFrame *grp2 = new TGGroupFrame(this, "Parameters", kHorizontalFrame);
    AddFrame(grp2, hints1);
    fList->Add(grp2);


    TGLayoutHints *hints4 = new TGLayoutHints(kLHintsExpandX, 5, 5,  3);
    TGLayoutHints *hints5 = new TGLayoutHints(kLHintsExpandX, 5, 5, 10);
    AddTextButton(grp1, "Load Pointing Model", kTbLoad,        hints5);
    AddTextButton(grp1, "Save Pointing Model", kTbSave,        hints4);
    AddTextButton(grp1, "Fit Parameters",      kTbFit,         hints5);
    AddTextButton(grp1, "Reset Parameters",    kTbReset,       hints4);
    AddTextButton(grp1, "Load Stars",          kTbLoadStars,   hints5);
    AddTextButton(grp1, "Reset Stars",         kTbResetStars,  hints4);
    AddTextButton(grp1, "Reload Stars",        kTbReloadStars, hints4);
    fList->Add(hints4);
    fList->Add(hints5);







    TGHorizontalFrame *comp = new TGHorizontalFrame(grp2, 1, 1);
    grp2->AddFrame(comp);
    fList->Add(comp);

    TGLayoutHints *hints3 = new TGLayoutHints(kLHintsLeft|kLHintsTop, 0, 10, 5, 0);
    fList->Add(hints3);

    TGVerticalFrame *vframe = new TGVerticalFrame(comp, 1, 1);

    for (int i=0; i<MPointing::GetNumPar(); i++)
        AddCheckButton(vframe, fBending.GetVarName(i), i);

    TGButton *but = (TGButton*)FindWidget(0);

    comp->AddFrame(vframe, hints3);
    fList->Add(vframe);

    vframe = new TGVerticalFrame(comp, 1, 1);
    comp->AddFrame(vframe, hints3);
    fList->Add(vframe);

    hints3 = new TGLayoutHints(kLHintsLeft|kLHintsTop, 0, 10, 5, 0);
    fList->Add(hints3);

    TGLabel *l = new TGLabel(vframe, "+000.0000");
    l->SetTextJustify(kTextRight);
    fList->Add(l);
    fLabel.Add(l);

    TGLayoutHints *h = new TGLayoutHints(kLHintsCenterY, 0, 0, but->GetHeight()-l->GetHeight());
    fList->Add(h);

    vframe->AddFrame(l,h);

    for (int i=1; i<MPointing::GetNumPar(); i++)
        AddLabel(vframe, "+000.0000", h)->SetTextJustify(kTextRight);

    vframe = new TGVerticalFrame(comp, 1, 1);
    comp->AddFrame(vframe, hints3);
    fList->Add(vframe);

    for (int i=0; i<MPointing::GetNumPar(); i++)
        AddLabel(vframe, "\xb1 00.0000\xb0", h)->SetTextJustify(kTextRight);

    hints3 = new TGLayoutHints(kLHintsLeft|kLHintsTop, 0, 20, 5, 0);
    fList->Add(hints3);

    TGLayoutHints *hreset = new TGLayoutHints(kLHintsLeft|kLHintsTop, 0, 0, 3, 1);
    fList->Add(hreset);

    TGVerticalFrame *vframe2 = new TGVerticalFrame(comp, 1, 1);
    comp->AddFrame(vframe2, hints3);
    fList->Add(vframe2);
    for (int i=0; i<MPointing::GetNumPar(); i++)
        AddResetButton(vframe2, i+2*MPointing::GetNumPar(), hreset,
                       but->GetHeight()-4);

    vframe = new TGVerticalFrame(comp, 1, 1);
    comp->AddFrame(vframe, hints3);
    fList->Add(vframe);

    for (int i=0; i<MPointing::GetNumPar(); i++)
        AddLabel(vframe, fBending.GetDescription(i), h);

    TGLayoutHints *hints6 = new TGLayoutHints(kLHintsExpandX, 5, 5, 4, 6);
    fList->Add(hints6);

    l = new TGLabel(grp1, "0000000 Data Sets loaded.");
    grp1->AddFrame(l, hints6);
    fList->Add(l);
    fLabel.Add(l);

    l = new TGLabel(grp1, "");
    l->SetTextJustify(kTextLeft);
    grp1->AddFrame(l, hints6);
    fList->Add(l);
    fLabel.Add(l);

    l = new TGLabel(grp1, "");
    l->SetTextJustify(kTextLeft);
    grp1->AddFrame(l, hints6);
    fList->Add(l);
    fLabel.Add(l);

    l = new TGLabel(grp1, "");
    l->SetTextJustify(kTextLeft);
    grp1->AddFrame(l, hints6);
    fList->Add(l);
    fLabel.Add(l);

    // ------------------------------------------------------------------

    TGLayoutHints *hintse1 = new TGLayoutHints(kLHintsExpandX|kLHintsBottom);
    TGLayoutHints *hintse2 = new TGLayoutHints(kLHintsExpandX, 2, 2);
    TGLayoutHints *hintse3 = new TGLayoutHints(kLHintsExpandX);
    TGLayoutHints *hintsl  = new TGLayoutHints(kLHintsExpandX, 1, 0, 5);
    fList->Add(hintse1);
    fList->Add(hintse2);
    fList->Add(hintse3);

    TGHorizontalFrame *entries = new TGHorizontalFrame(grp1, 1, 1);
    grp1->AddFrame(entries, hintse1);
    fList->Add(entries);

    TGVerticalFrame *v1 = new TGVerticalFrame(entries);
    TGVerticalFrame *v2 = new TGVerticalFrame(entries);
    TGVerticalFrame *v3 = new TGVerticalFrame(entries);
    entries->AddFrame(v1, hintse2);
    entries->AddFrame(v2, hintse2);
    entries->AddFrame(v3, hintse2);
    fList->Add(v1);
    fList->Add(v2);
    fList->Add(v3);

    TGLabel *label1 = new TGLabel(v1, "Az min/°");
    TGLabel *label2 = new TGLabel(v2, "Az max/°");
    TGLabel *label3 = new TGLabel(v3, "Mag min");
    TGLabel *label4 = new TGLabel(v1, "Zd min/°");
    TGLabel *label5 = new TGLabel(v2, "Zd max/°");
    TGLabel *label6 = new TGLabel(v3, "Limit/°");
    label1->SetTextJustify(kTextLeft);
    label2->SetTextJustify(kTextLeft);
    label3->SetTextJustify(kTextLeft);
    label4->SetTextJustify(kTextLeft);
    label5->SetTextJustify(kTextLeft);
    label6->SetTextJustify(kTextLeft);
    fList->Add(label1);
    fList->Add(label2);
    fList->Add(label3);
    fList->Add(label4);
    fList->Add(label5);
    fList->Add(label6);

    TGTextEntry *entry1 = new TGTextEntry(v1, Form("%.1f", fAzMin),  kIdAzMin);
    TGTextEntry *entry2 = new TGTextEntry(v2, Form("%.1f", fAzMax),  kIdAzMax);
    TGTextEntry *entry3 = new TGTextEntry(v3, Form("%.1f", fMagMax), kIdMagMax);
    TGTextEntry *entry4 = new TGTextEntry(v1, Form("%.1f", fZdMin),  kIdZdMin);
    TGTextEntry *entry5 = new TGTextEntry(v2, Form("%.1f", fZdMax),  kIdZdMax);
    TGTextEntry *entry6 = new TGTextEntry(v3, Form("%.3f", fLimit),  kIdLimit);
    entry1->SetToolTipText("TPoints with a real star located at Az<Az min are ignored in the fit.");
    entry2->SetToolTipText("TPoints with a real star located at Az>Az max are ignored in the fit.");
    entry2->SetToolTipText("TPoints with a artifiical magnitude Mag<Mag min are ignored in the fit.");
    entry4->SetToolTipText("TPoints with a real star located at Zd<Zd min are ignored in the fit.");
    entry5->SetToolTipText("TPoints with a real star located at Zd>Zd max are ignored in the fit.");
    entry6->SetToolTipText("TPoints with an residual after the fit > Limit are output.");
    entry1->Associate(this);
    entry2->Associate(this);
    entry3->Associate(this);
    entry4->Associate(this);
    entry5->Associate(this);
    entry6->Associate(this);
    v1->AddFrame(label1, hintsl);
    v1->AddFrame(entry1, hintse3);
    v1->AddFrame(label4, hintsl);
    v1->AddFrame(entry4, hintse3);
    v2->AddFrame(label2, hintsl);
    v2->AddFrame(entry2, hintse3);
    v2->AddFrame(label5, hintsl);
    v2->AddFrame(entry5, hintse3);
    v3->AddFrame(label3, hintsl);
    v3->AddFrame(entry3, hintse3);
    v3->AddFrame(label6, hintsl);
    v3->AddFrame(entry6, hintse3);
    fList->Add(entry1);
    fList->Add(entry2);
    fList->Add(entry3);
    fList->Add(entry4);
    fList->Add(entry5);
    fList->Add(entry6);

    // ------------------------------------------------------------------

    // FIXME: Move this to the rc-file.
    ((TGCheckButton*)FindWidget(0))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(1))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(3))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(4))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(5))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(6))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(7))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(8))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(11))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(12))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(13))->SetState(kButtonDown);
    ((TGCheckButton*)FindWidget(14))->SetState(kButtonDown);
/*
    ((TGCheckButton*)FindWidget(19))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(20))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(21))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(22))->SetState(kButtonDisabled);

    ((TGCheckButton*)FindWidget(19+2*MPointing::GetNumPar()))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(20+2*MPointing::GetNumPar()))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(21+2*MPointing::GetNumPar()))->SetState(kButtonDisabled);
    ((TGCheckButton*)FindWidget(22+2*MPointing::GetNumPar()))->SetState(kButtonDisabled);
*/
    SetWindowName("Telesto");
    SetIconName("Telesto");

    Layout();

    MapSubwindows();
    MapWindow();

    if (!fname.empty())
        LoadStars(fname.c_str());
    if (!mod.empty())
        fBending.Load(mod.c_str());

    DisplayBending();
    DisplayData();
}

TPointGui::~TPointGui()
{
    if (fFont)
        gVirtualX->DeleteFont(fFont);

    delete fList;

    if (fExitLoopOnClose)
        gSystem->ExitLoop();
}

void TPointGui::Fcn(Int_t &/*npar*/, Double_t */*gin*/, Double_t &f, Double_t *par, Int_t /*iflag*/)
{
    f = 0;

    MPointing bend;
    bend.SetParameters(par); // Set Parameters [deg] to MPointing

    int cnt = 0;
    for (int i=0; i<fCoordinates.GetSize(); i++)
    {
        TPointStar set = *(TPointStar*)fCoordinates.At(i);

        if (set.GetStarZd()<fZdMin || set.GetStarZd()>fZdMax ||
            set.GetStarAz()<fAzMin || set.GetStarAz()>fAzMax ||
            set.GetMag()   >fMagMax)
            continue;

        set.Adjust(bend);

        Double_t err = 0.0043; // [deg]
        Double_t res = set.GetResidual();//(&err);
        res /= err;

        f += res*res;
        cnt++;
    }

    f /= cnt;
}

void TPointGui::fcn(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag)
{
    ((TPointGui*)gMinuit->GetObjectFit())->Fcn(npar, gin, f, par, iflag);
}

void TPointGui::AddTextButton(TGCompositeFrame *f, TString txt, Int_t id, TGLayoutHints *h)
{
    TGButton *but = new TGTextButton(f, txt, id);
    but->Associate(this);
    f->AddFrame(but, h);
    fList->Add(but);

}

void TPointGui::AddCheckButton(TGCompositeFrame *f, TString txt, Int_t id, TGLayoutHints *h)
{
    TGButton *but = new TGCheckButton(f, txt, id);
    but->Associate(this);
    f->AddFrame(but, h);
    fList->Add(but);
}

void TPointGui::AddResetButton(TGCompositeFrame *f, Int_t id, TGLayoutHints *h, Int_t height)
{
    TGPictureButton *but = new TGPictureButton(f, "drive/skull.xpm", id);
    but->SetHeight(height); // Offsets from TGLayout
    but->SetWidth(height);
    but->Associate(this);
    f->AddFrame(but, h);
    fList->Add(but);
}

TGLabel *TPointGui::AddLabel(TGCompositeFrame *f, TString txt, TGLayoutHints *h)
{
    TGLabel *l = new TGLabel(f, txt/*, TGLabel::GetDefaultGC()(), fFont*/);
    f->AddFrame(l, h);
    fList->Add(l);
    fLabel.Add(l);
    return l;
}

void TPointGui::DisplayBending()
{
    TArrayD par, err;
    fBending.GetParameters(par);
    fBending.GetError(err);

    TGLabel *l;

    for (int i=0; i<MPointing::GetNumPar(); i++)
    {
        l = (TGLabel*)fLabel.At(i);
        l->SetText(Form("%.4f\xb0", par[i]));

        l = (TGLabel*)fLabel.At(MPointing::GetNumPar()+i);
        l->SetText(Form("\xb1 %8.4f\xb0", err[i]>0?err[i]:0));
    }
}

void TPointGui::DisplayData()
{
    TGLabel *l = (TGLabel*)fLabel.At(3*MPointing::GetNumPar());
    l->SetText(Form("%d data sets loaded.", fOriginal.GetSize()));
}

void TPointGui::DisplayResult(Double_t before, Double_t after, Double_t backw)
{
    TGLabel *l1 = (TGLabel*)fLabel.At(3*MPointing::GetNumPar()+1);
    l1->SetText(Form("Before: %.1f arcsec", before*360*3600/16384));

    TGLabel *l2 = (TGLabel*)fLabel.At(3*MPointing::GetNumPar()+2);
    l2->SetText(Form("After:  %.1f arcsec", after*360*3600/16384));

    TGLabel *l3 = (TGLabel*)fLabel.At(3*MPointing::GetNumPar()+3);
    l3->SetText(Form("Backw:  %.1f arcsec", backw*360*3600/16384));
}

void TPointGui::DrawMarker(TVirtualPad *pad, Double_t r0, Double_t phi0)
{
    TView *view = pad->GetView();

    if (!view)
    {
        cout << "No View!" << endl;
        return;
    }

    TMarker mark0;
    mark0.SetMarkerStyle(kFullDotLarge);
    mark0.SetMarkerColor(kBlue);

    r0 /= 90;
    phi0 *= TMath::DegToRad();

    Double_t x[6] = { r0*cos(phi0), r0*sin(phi0), 0, 0, 0, 0};

    view->WCtoNDC(x, x+3);

    mark0.DrawMarker(-x[3], x[4]);
}

void TPointGui::DrawPolLine(TVirtualPad *pad, Double_t r0, Double_t phi0, Double_t r1, Double_t phi1)
{
    TView *view = pad->GetView();

    if (!view)
    {
        cout << "No View!" << endl;
        return;
    }
    /*
    if (r0<0)
    {
        r0 = -r0;
        phi0 += 180;
    }
    if (r1<0)
    {
        r1 = -r1;
        phi1 += 180;
    }

    phi0 = fmod(phi0+360, 360);
    phi1 = fmod(phi1+360, 360);

    if (phi1-phi0<-180)
        phi1+=360;
    */
    TLine line;
    line.SetLineWidth(2);
    line.SetLineColor(kBlue);

    Double_t p0 = phi0<phi1?phi0:phi1;
    Double_t p1 = phi0<phi1?phi1:phi0;

    if (phi0>phi1)
    {
        Double_t d = r1;
        r1 = r0;
        r0 = d;
    }

    r0 /= 90;
    r1 /= 90;

    Double_t dr = r1-r0;
    Double_t dp = p1-p0;

    Double_t x0[3] = { r0*cos(p0*TMath::DegToRad()), r0*sin(p0*TMath::DegToRad()), 0};

    for (double i=p0+10; i<p1+10; i+=10)
    {
        if (i>p1)
            i=p1;

        Double_t r = dr/dp*(i-p0)+r0;
        Double_t p = TMath::DegToRad()*i;

        Double_t x1[3] = { r*cos(p), r*sin(p), 0};

        Double_t y0[3], y1[3];

        view->WCtoNDC(x0, y0);
        view->WCtoNDC(x1, y1);

        line.DrawLine(y0[0], y0[1], y1[0], y1[1]);

        x0[0] = x1[0];
        x0[1] = x1[1];
    }
}

void TPointGui::DrawSet(TVirtualPad *pad, TPointStar &set, Float_t scale, Float_t angle)
{
    Double_t r0   = set.GetRawZd();
    Double_t phi0 = set.GetRawAz()-angle;
    Double_t r1   = set.GetStarZd();
    Double_t phi1 = set.GetStarAz()-angle;

    if (r0<0)
    {
        r0 = -r0;
        phi0 += 180;
    }
    if (r1<0)
    {
        r1 = -r1;
        phi1 += 180;
    }

    phi0 = fmod(phi0+360, 360);
    phi1 = fmod(phi1+360, 360);

    if (phi1-phi0<-180)
        phi1+=360;

    if (scale<0 || scale>1000)
        scale = -1;

    if (scale>0)
    {
        Double_t d = r1-r0;
        r0 += scale*d;
        r1 -= scale*d;
        d = phi1-phi0;
        phi0 += scale*d;
        phi1 -= scale*d;

        DrawPolLine(pad, r0, phi0, r1, phi1);
        DrawMarker(pad,  r0, phi0);
    }
    else
        DrawMarker(pad,  r1, phi1);
}

void TPointGui::DrawHorizon(TVirtualPad *pad, const char *fname) const
{
    TView *view = pad->GetView();

    if (!view)
    {
        cout << "No View!" << endl;
        return;
    }

    ifstream fin(fname);
    if (!fin)
    {
        cout << "ERROR - " << fname << " not found." << endl;
        return;
    }

    TPolyLine poly;
    poly.SetLineWidth(2);
    poly.SetLineColor(12);
    poly.SetLineStyle(8);

    while (1)
    {
        TString line;
        line.ReadLine(fin);
        if (!fin)
            break;

        Float_t az, alt;
        sscanf(line.Data(), "%f %f", &az, &alt);

        Float_t zd = 90-alt;

        az *= TMath::DegToRad();
        zd /= 90;

        Double_t x[6] = { zd*cos(az), zd*sin(az), 0, 0, 0, 0};
        view->WCtoNDC(x, x+3);
        poly.SetNextPoint(-x[3], x[4]);
    }

    poly.DrawClone()->SetBit(kCanDelete);

}

TString TPointGui::OpenDialog(TString &dir, EFileDialogMode mode)
{
    static const char *gOpenTypes[] =
    {
        "TPoint files",     "*.txt",
        "Collection files", "*.col",
        "Model files",      "*.mod",
        "All files",        "*",
        NULL,           NULL
    };

    //static TString dir("tpoint/");

    TGFileInfo fi; // fFileName and fIniDir deleted in ~TGFileInfo

    fi.fFileTypes = (const char**)gOpenTypes;
    fi.fIniDir    = StrDup(dir);

    new TGFileDialog(fClient->GetRoot(), this, mode, &fi);

    if (!fi.fFilename)
        return "";

    dir = fi.fIniDir;

    return fi.fFilename;
}

void TPointGui::LoadCollection(TString fname)
{
    ifstream fin(fname);
    if (!fin)
    {
        cout << "Collection '" << fname << "' not found!" << endl;
        return;
    }

    while (1)
    {
        TString line;
        line.ReadLine(fin);
        if (!fin)
            break;

        line = line.Strip(TString::kBoth);
        if (line[0]=='#')
            continue;
        if (line.Length()==0)
            continue;
/*
        if (!line.EndsWith(".txt"))
        {
            cout << "WARNING: " << line << endl;
            continue;
        }
*/
        LoadStars(line);
    }
}

void TPointGui::LoadStars(TString fname)
{
    if (fname.EndsWith(".col"))
    {
        LoadCollection(fname);
        fFileNameStars = fname;
        SetWindowName(Form("Telesto (%s)", fFileNameStars.Data()));
        return;
    }

    const Int_t size = fOriginal.GetSize();

    ifstream fin(fname);

    while (fin && fin.get()!='\n');
    while (fin && fin.get()!='\n');
    while (fin && fin.get()!='\n');
    if (!fin)
    {
        cout << "File '" << fname << "' not found!" << endl;
        return;
    }

    TPointStar set(fname);

    while (1)
    {
        fin >> set;  // Read data from file [deg], it is stored in [rad]
        if (!fin)
            break;

//        if (set.GetRawZd()>60)
//            continue;

        fOriginal.Add(new TPointStar(set));
    }

    cout << "Found " << fOriginal.GetSize()-size;
    cout << " sets of coordinates in " << fname;
    cout << " (Total=" << fOriginal.GetSize() << ")" << endl;

    fFileNameStars = fname;
    SetWindowName(Form("Telesto (%s)", fFileNameStars.Data()));
}

Float_t TPointGui::GetFloat(Int_t id) const
{
    return atof(static_cast<TGTextEntry*>(FindWidget(id))->GetText());
}

Bool_t TPointGui::ProcessMessage(Long_t msg, Long_t mp1, Long_t)
{
    // cout << "Msg: " << hex << GET_MSG(msg) << endl;
    // cout << "SubMsg: " << hex << GET_SUBMSG(msg) << dec << endl;

    static TString dirmod("tpoint/");
    static TString dircol("tpoint/");

    switch (GET_MSG(msg))
    {
    case kC_COMMAND:
        switch (GET_SUBMSG(msg))
        {
        case kCM_BUTTON:
            switch (mp1)
            {
            case kTbFit:
                {
                    Double_t before=0;
                    Double_t after=0;
                    Double_t backw=0;
                    Fit(before, after, backw);
                    DisplayBending();
                    DisplayResult(before, after, backw);
                }
                return kTRUE;
            case kTbLoad:
                fBending.Load(OpenDialog(dirmod));
                DisplayBending();
                return kTRUE;
            case kTbSave:
                fBending.Save(OpenDialog(dirmod, kFDSave));
                return kTRUE;
            case kTbLoadStars:
                LoadStars(OpenDialog(dircol));
                DisplayData();
                return kTRUE;
            case kTbReset:
                fBending.Reset();
                DisplayBending();
                return kTRUE;
            case kTbReloadStars:
                fOriginal.Delete();
                LoadStars(fFileNameStars); // FIXME: Use TGLabel!
                DisplayData();
                return kTRUE;
            case kTbResetStars:
                fOriginal.Delete();
                DisplayData();
                return kTRUE;
            }

            // In the default cas a reset button must have been pressed
            fBending[mp1-2*MPointing::GetNumPar()] = 0;
            DisplayBending();
            return kTRUE;
        }
        return kTRUE;

    case kC_TEXTENTRY:
        switch (GET_SUBMSG(msg))
        {
        case kTE_TEXTCHANGED:
            switch (mp1)
            {
            case kIdAzMin:
                fAzMin = GetFloat(kIdAzMin);
                return kTRUE;
            case kIdAzMax:
                fAzMax = GetFloat(kIdAzMax);
                return kTRUE;
            case kIdZdMin:
                fZdMin = GetFloat(kIdZdMin);
                return kTRUE;
            case kIdZdMax:
                fZdMax = GetFloat(kIdZdMax);
                return kTRUE;
            case kIdMagMax:
                fMagMax = GetFloat(kIdMagMax);
                return kTRUE;
            case kIdLimit:
                fLimit = GetFloat(kIdLimit);
                return kTRUE;
            }
            return kTRUE;

        }
        return kTRUE;

    }
    return kTRUE;
}

void TPointGui::Fit(Double_t &before, Double_t &after, Double_t &backw)
{
    if (fOriginal.GetSize()==0)
    {
        cout << "Sorry, no input data loaded..." << endl;
        return;
    }

    fCoordinates.Delete();
    for (int i=0; i<fOriginal.GetSize(); i++)
        fCoordinates.Add(new TPointStar(*(TPointStar*)fOriginal.At(i)));

    cout << "-----------------------------------------------------------------------" << endl;

    gStyle->SetOptStat("emro");

    TH1F hres1("Res1", " Residuals before correction ", fOriginal.GetSize()/3, 0, 0.3);
    TH1F hres2("Res2", " Residuals after correction ",  fOriginal.GetSize()/3, 0, 0.3);
    TH1F hres3("Res3", " Residuals after backward correction ",  fOriginal.GetSize()/3, 0, 0.3);

    TProfile proaz ("ProAz",  " \\Delta profile vs. Az",  24, 0, 360);
    TProfile prozd ("ProZd",  " \\Delta profile vs. Zd",  30, 0,  90);
    TProfile promag("ProMag", " \\Delta profile vs. Mag", 10, 1,  10);

    hres1.SetXTitle("\\Delta [\\circ]");
    hres1.SetYTitle("Counts");

    hres2.SetXTitle("\\Delta [\\circ]");
    hres2.SetYTitle("Counts");

    hres3.SetXTitle("\\Delta [\\circ]");
    hres3.SetYTitle("Counts");

    TGraph gdaz;
    TGraph gdzd;
    TGraph gaz;
    TGraph gzd;
    TGraphErrors graz;
    TGraphErrors grzd;
    TGraphErrors grmag;
    TGraph gmaz;
    TGraph gmzd;

    gdaz.SetTitle(" \\Delta Az vs. Zd ");
    gdzd.SetTitle(" \\Delta Zd vs. Az ");

    gaz.SetTitle(" \\Delta Az vs. Az ");
    gzd.SetTitle(" \\Delta Zd vs. Zd ");

    gmaz.SetTitle(" \\Delta Az vs. Mag ");
    gmzd.SetTitle(" \\Delta Zd vs. Mag ");

    graz.SetTitle(" \\Delta vs. Az ");
    grzd.SetTitle(" \\Delta vs. Zd ");
    grmag.SetTitle(" \\Delta vs. Mag ");

    TMinuit minuit(MPointing::GetNumPar());  //initialize TMinuit with a maximum of 5 params
    minuit.SetObjectFit(this);
    minuit.SetPrintLevel(-1);
    minuit.SetFCN(fcn);

    fBending.SetMinuitParameters(minuit, MPointing::GetNumPar()); // Init Parameters [deg]

    for (int i=0; i<MPointing::GetNumPar(); i++)
    {
        TGButton *l = (TGButton*)FindWidget(i);
        minuit.FixParameter(i);
        if (l->GetState()==kButtonDown)
            minuit.Release(i);
    }

    //minuit.Command("SHOW PARAMETERS");
    //minuit.Command("SHOW LIMITS");

    cout << endl;
    cout << "Starting fit..." << endl;
    cout << "For the fit an measurement error in the residual of ";
    cout << "0.02deg (=1SE) is assumed." << endl;
    cout << endl;

    Int_t ierflg = 0;
    ierflg = minuit.Migrad();
    cout << "Migrad returns " << ierflg << endl;
    // minuit.Release(2);
    ierflg = minuit.Migrad();
    cout << "Migrad returns " << ierflg << endl << endl;

    //
    // Get Fit Results
    //
    fBending.GetMinuitParameters(minuit);
    fBending.PrintMinuitParameters(minuit);
    cout << endl;
    //fBending.Save("bending_magic.txt");


    //
    // Make a copy of all list entries
    //
    TList list;
    list.SetOwner();
    for (int i=0; i<fCoordinates.GetSize(); i++)
        list.Add(new TPointStar(*(TPointStar*)fCoordinates.At(i)));

    //
    // Correct for Offsets only
    //
    TArrayD par;
    fBending.GetParameters(par);
    for (int i=2; i<MPointing::GetNumPar(); i++)
        par[i]=0;

    MPointing b2;
    b2.SetParameters(par);

    cout << endl << "Sets with Residual exceeding " << fLimit << "deg:" << endl;
    cout << "   StarAz  StarEl      RawAz   RawEl      Mag Residual  Filename" << endl;

    //
    // Calculate correction and residuals
    //
    for (int i=0; i<fCoordinates.GetSize(); i++)
    {
        TPointStar orig = *(TPointStar*)fCoordinates.At(i);

        TPointStar &set0 = *(TPointStar*)fCoordinates.At(i);

        ZdAz za(set0.GetStarZdAz());
        za *= 180/M_PI;

        //
        // Correct for offsets only
        //
        TPointStar set1(set0);
        set1.Adjust(b2);

        hres1.Fill(set1.GetResidual());

        set0.Adjust(fBending);
        hres2.Fill(set0.GetResidual());

        Double_t dz = fmod(set0.GetDAz()+720, 360);
        if (dz>180)
            dz -= 360;

        Double_t err;
        Double_t resi = set0.GetResidual(&err);

        gdzd.SetPoint(i, za.Az(), set0.GetDZd());
        gdaz.SetPoint(i, za.Zd(), dz);
        graz.SetPoint(i, za.Az(), resi);
        graz.SetPointError(i, 0, err);
        grzd.SetPoint(i, za.Zd(), resi);
        grzd.SetPointError(i, 0, err);

        if (resi>fLimit) // 0.13
            cout << " " << orig << "  <" << Form("%5.3f", resi) << ">  " << orig.GetName() << endl;

        proaz.Fill(za.Az(), set0.GetResidual(&err));
        prozd.Fill(za.Zd(), set0.GetResidual(&err));
        promag.Fill(set0.GetMag(), set0.GetResidual(&err));

        gaz.SetPoint( i, za.Az(), dz);
        gzd.SetPoint( i, za.Zd(), set0.GetDZd());
        if (set0.GetMag()>=-20)
        {
            grmag.SetPoint(i, set0.GetMag(), set0.GetResidual(&err));
            grmag.SetPointError(i, 0, err);
            gmaz.SetPoint( i, set0.GetMag(), dz);
            gmzd.SetPoint( i, set0.GetMag(), set0.GetDZd());
        }
    }

    cout << "done." << endl << endl;

    //
    // Check for overflows
    //
    const Stat_t ov = hres2.GetBinContent(hres2.GetNbinsX()+1);
    if (ov>0)
        cout << "WARNING: " << ov << " overflows in residuals." << endl;



    cout << dec << endl;
    cout << "             Number of calls to FCN: " << minuit.fNfcn << endl;
    cout << "Minimum value found for FCN (Chi^2): " << minuit.fAmin << endl;
    cout << "                    Fit-Probability: " << TMath::Prob(minuit.fAmin/*fOriginal.GetSize()*/, fOriginal.GetSize()-minuit.GetNumFreePars())*100 << "%" << endl;
    cout << "                          Chi^2/NDF: " << minuit.fAmin/(fOriginal.GetSize()-minuit.GetNumFreePars()) << endl;
    //cout << "Prob(?): " << TMath::Prob(fChisquare,ndf);



    //
    // Print all data sets for which the backward correction is
    // twice times worse than the residual gotten from the
    // bending correction itself
    //
    cout << endl;
    cout << "Checking backward correction (raw-->star):" << endl;
    for (int i=0; i<fCoordinates.GetSize(); i++)
    {
        TPointStar set0(*(TPointStar*)list.At(i));
        TPointStar &set1 = *(TPointStar*)list.At(i);

        set0.AdjustBack(fBending);
        set1.Adjust(fBending);

        const Double_t res0 = set0.GetResidual();
        const Double_t res1 = set1.GetResidual();
        const Double_t diff = TMath::Abs(res0-res1);

        hres3.Fill(res0);

        if (diff<hres2.GetMean()*0.66)
            continue;

        cout << "DBack: " << setw(6) << set0.GetStarZd() << " " << setw(7) << set0.GetStarAz() << ":  ";
        cout << "ResB="<< setw(7) << res0*60 << "  ResF=" << setw(7) << res1*60 << "  |ResB-ResF|=" << setw(7) << diff*60 << " arcmin" << endl;
    }
    cout << "OK." << endl;
    cout << endl;

    const Double_t max1 = TMath::Max(gaz.GetHistogram()->GetMaximum(), gdaz.GetHistogram()->GetMaximum());
    const Double_t max2 = TMath::Max(gzd.GetHistogram()->GetMaximum(), gdzd.GetHistogram()->GetMaximum());
    const Double_t max3 = TMath::Max(grzd.GetHistogram()->GetMaximum(), graz.GetHistogram()->GetMaximum());

    const Double_t min1 = TMath::Min(gaz.GetHistogram()->GetMinimum(), gdaz.GetHistogram()->GetMinimum());
    const Double_t min2 = TMath::Min(gzd.GetHistogram()->GetMinimum(), gdzd.GetHistogram()->GetMinimum());
    const Double_t min3 = TMath::Min(grzd.GetHistogram()->GetMinimum(), graz.GetHistogram()->GetMinimum());

    const Double_t absmax1 = 0.05;//TMath::Max(max1, TMath::Abs(min1));
    const Double_t absmax2 = 0.05;//TMath::Max(max2, TMath::Abs(min2));
    const Double_t absmax3 = 0.05;//TMath::Max(max3, TMath::Abs(min3));

    gaz.SetMaximum(absmax1);
    gzd.SetMaximum(absmax2);
    gdaz.SetMaximum(absmax1);
    gdzd.SetMaximum(absmax2);
    gmaz.SetMaximum(absmax1);
    gmzd.SetMaximum(absmax2);
    graz.SetMaximum(absmax3);
    grzd.SetMaximum(absmax3);
    grmag.SetMaximum(absmax3);
    gaz.SetMinimum(-absmax1);
    gzd.SetMinimum(-absmax2);
    gdaz.SetMinimum(-absmax1);
    gdzd.SetMinimum(-absmax2);
    gmaz.SetMinimum(-absmax1);
    gmzd.SetMinimum(-absmax2);
    graz.SetMinimum(0);
    grzd.SetMinimum(0);
    grmag.SetMinimum(0);

    TCanvas *c1;

    if (gROOT->FindObject("CanvGraphs"))
        c1 = dynamic_cast<TCanvas*>(gROOT->FindObject("CanvGraphs"));
    else
        c1=new TCanvas("CanvGraphs", "Graphs");

    gROOT->SetSelectedPad(0);
    c1->SetSelectedPad(0);
    c1->SetBorderMode(0);
    c1->SetFrameBorderMode(0);
    c1->Clear();

    c1->SetFillColor(kWhite);
#ifndef PRESENTATION
    c1->Divide(3,3,1e-10,1e-10);
#else
    c1->Divide(2,2,1e-10,1e-10);
#endif
    c1->SetFillColor(kWhite);

    TGraph *g=0;

    TLine line;
    line.SetLineColor(kGreen);
    line.SetLineWidth(2);
#ifndef PRESENTATION
    c1->cd(1);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)gaz.DrawClone("A*");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Az [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta Az [\\circ]");

    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
    line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);

    c1->cd(2);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)gdaz.DrawClone("A*");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Zd [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta Az [\\circ]");
    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
    line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);
    cout << "Mean dAz: " << g->GetMean(2) << " \xb1 " << g->GetRMS(2) <<  endl;

    c1->cd(3);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    if (gmaz.GetN()>0)
    {
        g=(TGraph*)gmaz.DrawClone("A*");
        g->SetBit(kCanDelete);
        g->GetHistogram()->SetXTitle("Mag");
        g->GetHistogram()->SetYTitle("\\Delta Az [\\circ]");
        line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
        line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);
    }
#endif

#ifndef PRESENTATION
    c1->cd(4);
#else
    c1->cd(1);
#endif
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)gdzd.DrawClone("A*");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Az [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta Zd [\\circ]");
    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
    line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);
    cout << "Mean dZd: " << g->GetMean(2) << " \xb1 " << g->GetRMS(2) <<  endl;
    cout << endl;

#ifndef PRESENTATION
    c1->cd(5);
#else
    c1->cd(2);
#endif
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)gzd.DrawClone("A*");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Zd [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta Zd [\\circ]");
    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
    line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);
#ifndef PRESENTATION
    c1->cd(6);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    if (gmzd.GetN()>0)
    {
        g=(TGraph*)gmzd.DrawClone("A*");
        g->SetBit(kCanDelete);
        g->GetHistogram()->SetXTitle("Mag");
        g->GetHistogram()->SetYTitle("\\Delta Zd [\\circ]");
        line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
        line.DrawLine(g->GetXaxis()->GetXmin(), -360./16384, g->GetXaxis()->GetXmax(), -360./16384);
    }
#endif

#ifndef PRESENTATION
    c1->cd(7);
#else
    c1->cd(3);
#endif
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)graz.DrawClone("AP");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Az [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta [\\circ]");
    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);

    proaz.SetLineWidth(2);
    proaz.SetLineColor(kBlue);
    proaz.SetMarkerColor(kBlue);
    proaz.DrawCopy("pc hist same");

#ifndef PRESENTATION
    c1->cd(8);
#else
    c1->cd(4);
#endif
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    g=(TGraph*)grzd.DrawClone("AP");
    g->SetBit(kCanDelete);
    g->GetHistogram()->SetXTitle("Zd [\\circ]");
    g->GetHistogram()->SetYTitle("\\Delta [\\circ]");
    line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);

    prozd.SetLineWidth(2);
    prozd.SetLineColor(kBlue);
    prozd.SetMarkerColor(kBlue);
    prozd.DrawCopy("pc hist same");

#ifndef PRESENTATION
    c1->cd(9);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetGridx();
    gPad->SetGridy();
    if (grmag.GetN()>0)
    {
        g=(TGraph*)grmag.DrawClone("AP");
        g->SetBit(kCanDelete);
        g->GetHistogram()->SetXTitle("Mag");
        g->GetHistogram()->SetYTitle("\\Delta [\\circ]");
        line.DrawLine(g->GetXaxis()->GetXmin(),  360./16384, g->GetXaxis()->GetXmax(),  360./16384);
    }
    promag.SetLineWidth(2);
    promag.SetLineColor(kBlue);
    promag.SetMarkerColor(kBlue);
    promag.DrawCopy("pc hist same");
#endif

    //
    // Print out the residual before and after correction in several
    // units
    //
    cout << fCoordinates.GetSize() << " data sets." << endl << endl;
    cout << "Total Spread of Residual:" << endl;
    cout << "-------------------------" << endl;
    cout << "before: " << Form("%6.4f", hres1.GetMean()) << " \xb1 " << Form("%6.4f", hres1.GetRMS()) << " deg \t";
    cout << "before: " << Form("%4.1f", hres1.GetMean()*3600) << " \xb1 " << Form("%.1f", hres1.GetRMS()*3600) << " arcsec" << endl;
    cout << "after:  " << Form("%6.4f", hres2.GetMean()) << " \xb1 " << Form("%6.4f", hres2.GetRMS()) << " deg \t";
    cout << "after:  " << Form("%4.1f", hres2.GetMean()*3600) << " \xb1 " << Form("%.1f", hres2.GetRMS()*3600) << " arcsec" << endl;
    cout << "backw:  " << Form("%6.4f", hres3.GetMean()) << " \xb1 " << Form("%6.4f", hres3.GetRMS()) << " deg \t";
    cout << "backw:  " << Form("%4.1f", hres3.GetMean()*3600) << " \xb1 " << Form("%.1f", hres3.GetRMS()*3600) << " arcsec" << endl;
    cout << endl;
    cout << "before: " << Form("%4.1f", hres1.GetMean()*16348/360) << " \xb1 " << Form("%.1f", hres1.GetRMS()*16384/360) << " SE \t\t";
    cout << "before: " << Form("%4.1f", hres1.GetMean()*60*60/23.4) << " \xb1 " << Form("%.1f", hres1.GetRMS()*60*60/23.4) << " pix" << endl;
    cout << "after:  " << Form("%4.1f", hres2.GetMean()*16384/360) << " \xb1 " << Form("%.1f", hres2.GetRMS()*16384/360) << " SE \t\t";
    cout << "after:  " << Form("%4.1f", hres2.GetMean()*60*60/23.4) << " \xb1 " << Form("%.1f", hres2.GetRMS()*60*60/23.4) << " pix" << endl;
    cout << "backw:  " << Form("%4.1f", hres3.GetMean()*16384/360) << " \xb1 " << Form("%.1f", hres3.GetRMS()*16384/360) << " SE \t\t";
    cout << "backw:  " << Form("%4.1f", hres3.GetMean()*60*60/23.4) << " \xb1 " << Form("%.1f", hres3.GetRMS()*60*60/23.4) << " pix" << endl;
    cout << endl;
    cout << endl; // ±


    before = hres1.GetMean()*16384/360;
    after  = hres2.GetMean()*16384/360;
    backw  = hres3.GetMean()*16384/360;


    gStyle->SetOptStat(1110);
    gStyle->SetStatFormat("6.2g");

    if (gROOT->FindObject("CanvResiduals"))
        c1 = dynamic_cast<TCanvas*>(gROOT->FindObject("CanvResiduals"));
    else
        c1=new TCanvas("CanvResiduals", "Residuals", 800, 800);

    gROOT->SetSelectedPad(0);
    c1->SetSelectedPad(0);
    c1->Clear();
    c1->SetFillColor(kWhite);

    c1->Divide(2, 2, 1e-10, 1e-10);

    c1->cd(2);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    hres1.SetLineColor(kRed);
    hres1.DrawCopy();

    gPad->Update();

    line.DrawLine(360./16384, gPad->GetUymin(), 360./16384, gPad->GetUymax());

    c1->cd(4);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    hres2.SetLineColor(kBlue);
    TH1 *h=hres2.DrawCopy();
    TF1 f("mygaus", "(gaus)", 0, 1);
    f.SetLineColor(kMagenta/*6*/);
    f.SetLineWidth(1);
    f.SetParameter(0, h->GetBinContent(1));
    f.FixParameter(1, 0);
    f.SetParameter(2, h->GetRMS());
    h->Fit("mygaus", "QR");
    hres3.SetLineColor(kCyan);
    hres3.SetLineStyle(kDashed);
    hres3.DrawCopy("same");
    cout << "Gaus-Fit  Sigma: " << f.GetParameter(2) << "\xb0" << endl;
    cout << "Fit-Probability: " << f.GetProb()*100 << "%" << endl;
    cout << "      Chi^2/NDF: " << f.GetChisquare() << "/" << f.GetNDF() << " = " << f.GetChisquare()/f.GetNDF() << endl;
    gPad->Update();
    line.DrawLine(360./16384, gPad->GetUymin(), 360./16384, gPad->GetUymax());

    c1->cd(1);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetTheta(90);
    gPad->SetPhi(90);
    TH2F h2res1("Res2D1", " Dataset positions on the sky ", 36, 0, 360,  8, 0, 90);
    h2res1.SetBit(TH1::kNoStats);
    h2res1.DrawCopy("surf1pol");
    gPad->Modified();
    gPad->Update();
    DrawHorizon(gPad);
    for (int i=0; i<fOriginal.GetSize(); i++)
        DrawSet(gPad, *(TPointStar*)fOriginal.At(i));//, 10./hres1.GetMean());

    TText text;
    text.SetTextAlign(22);
    text.DrawText( 0.00,  0.66, "N");
    text.DrawText( 0.66,  0.00, "E");
    text.DrawText( 0.00, -0.66, "S");
    text.DrawText(-0.66,  0.00, "W");

    c1->cd(3);
    gPad->SetBorderMode(0);
    gPad->SetFrameBorderMode(0);
    gPad->SetTheta(90);
    gPad->SetPhi(90);
    h2res1.SetTitle(" Arb. Residuals after correction (scaled) ");
    h2res1.DrawCopy("surf1pol");
    gPad->Modified();
    gPad->Update();
//        for (int i=0; i<fCoordinates.GetSize(); i++)
//            DrawSet(gPad, *(Set*)fCoordinates.At(i), 10./hres2.GetMean(), par[0]);

    RaiseWindow();
}

TObject *TPointGui::FindWidget(Int_t id) const
{
    if (id<0)
        return NULL;

    TObject *obj;
    TIter Next(fList);
    while ((obj=Next()))
    {
        const TGWidget *wid = (TGWidget*)obj->IsA()->DynamicCast(TGWidget::Class(), obj);
        if (!wid)
            continue;

        if (id == wid->WidgetId())
            return obj;
    }
    return NULL;
}
