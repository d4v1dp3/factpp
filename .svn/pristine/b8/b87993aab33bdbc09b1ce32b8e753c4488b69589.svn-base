#ifndef COSY_TPointGui
#define COSY_TPointGui

#ifndef ROOT_TGFrame
#include <TGFrame.h>
#endif

#ifndef ROOT_TGFileDialog
#include <TGFileDialog.h>
#endif

#include "MPointing.h"

class TPointStar;
class TVirtualPad;
class TGLabel;
class TList;

class TPointGui : public TGMainFrame
{
private:
    enum {
        kTbFit = 1024,
        kTbLoad,
        kTbSave,
        kTbLoadStars,
        kTbReset,
        kTbResetStars,
        kTbReloadStars,

        kIdAzMin,
        kIdAzMax,
        kIdZdMin,
        kIdZdMax,
        kIdMagMax,
        kIdLimit,
    };

    TList *fList;

    TList fOriginal;
    TList fCoordinates;
    TList fLabel;

    MPointing fBending;

    TString fFileNameStars;

    FontStruct_t fFont;

    Bool_t fExitLoopOnClose;

    Float_t fAzMin;
    Float_t fAzMax;
    Float_t fZdMin;
    Float_t fZdMax;
    Float_t fMagMax;

    Float_t fLimit;

    void Fcn(Int_t &/*npar*/, Double_t */*gin*/, Double_t &f, Double_t *par, Int_t /*iflag*/);
    static void fcn(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag);

    TObject *FindWidget(Int_t id) const;

    void AddTextButton(TGCompositeFrame *f, TString txt, Int_t id=-1, TGLayoutHints *h=0);
    void AddCheckButton(TGCompositeFrame *f, TString txt, Int_t id=-1, TGLayoutHints *h=0);
    void AddResetButton(TGCompositeFrame *f, Int_t id, TGLayoutHints *h, Int_t height);
    TGLabel *AddLabel(TGCompositeFrame *f, TString txt, TGLayoutHints *h=0);

    void DisplayBending();
    void DisplayData();
    void DisplayResult(Double_t before, Double_t after, Double_t backw);

    void DrawMarker(TVirtualPad *pad, Double_t r0, Double_t phi0);
    void DrawPolLine(TVirtualPad *pad, Double_t r0, Double_t phi0, Double_t r1, Double_t phi1);
    void DrawSet(TVirtualPad *pad, TPointStar &set, Float_t scale=-1, Float_t angle=0);
    void DrawHorizon(TVirtualPad *pad, const char *fname="drive/horizon.dat") const;

    TString OpenDialog(TString &dir, EFileDialogMode mode=kFDOpen);

    void LoadCollection(TString fname);
    void LoadStars(TString fname="tpoint.txt");

    Bool_t ProcessMessage(Long_t msg, Long_t mp1, Long_t);

    void Fit(Double_t &before, Double_t &after, Double_t &backw);

    Float_t GetFloat(Int_t id) const;

public:
    TPointGui(const std::string fname, const std::string mod);
    ~TPointGui();

    void SetExitLoopOnClose(Bool_t b=kTRUE) { fExitLoopOnClose=b; }
};

#endif
