#ifndef COSY_MStarguider
#define COSY_MStarguider

#include "PixClient.h"
#include "MGImage.h"
#include "Led.h"
#include "Camera.h"

#include "dic.hxx"
#include "dis.hxx"

#include <TGButton.h>

class TGMenuBar;
class TGPopupMenu;
class TGTextEntry;

class PixGetter;

class MGImage;
class MCaos;
class FilterLed;
class Ring;

//class Leds;
class MStarguider : public PixClient, public TGMainFrame, public DimCommandHandler
{
private:
    DimService fDimData;
    DimCommand fDimTPoint;
    DimCommand fDimScreenshot;

    Camera        *fGetter;

    TGMenuBar     *fMenu;
    MGImage       *fImage;

    TGPopupMenu   *fDisplay;
    TGPopupMenu   *fSetup;
    TGPopupMenu   *fInterpol;

    TGPopupMenu   *fCaosWrite;
    TGPopupMenu   *fCaosPrint;
    TGPopupMenu   *fCaosAnalyse;
    TGPopupMenu   *fCaOs;

private:
    MCaos         *fCaos;
    TTimer        *fTimer;

    Int_t fDx;
    Int_t fDy;

    byte fIntRate;
//    int  fWrtRate;

    Double_t fLastBright;
    Double_t fRadius; // LED radius [cm]

    Float_t fFindStarCut;
    Int_t   fFindStarBox;

    int fTPointMode;

    bool fScreenshotColor;
    std::string fScreenshotName;

    void WritePNG(const char *name, const byte *gbuf, const byte *cbuf);

    void Toggle(TGPopupMenu *p, UInt_t id);
    void SwitchOff(TGPopupMenu *p, UInt_t id);
    bool Interpolate(const unsigned long n, byte *img) const;

    void InitGui(Int_t channel);

    //void DrawZoomImage(const byte *img);
    //void DrawCosyImage(const byte *img);

    Bool_t HandleTimer(TTimer *t);

    void SetCut(Double_t cut);

public:
    MStarguider(Int_t channel);
    virtual ~MStarguider();

    //void SetupEnv(TEnv &env);

    void Layout();
    void CloseWindow();

    Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

    //Bool_t HandleDoubleClick(Event_t *event);

    //
    // Execution of one frame - this function may be overloaded!
    //
    void ProcessFrame(const unsigned long n, byte *img, struct timeval *tm);

    void commandHandler();       /// Overwritten DimCommand::commandHandler
};

#endif


