#undef EXPERT
#define FACT

#include "MStarguider.h"

#ifdef HAVE_PNG
#include <png.h>
#endif
#include <iostream>
#include <fstream>

#include <math.h>

#include <TEnv.h>
#include <TSystem.h>

#include <TGMenu.h>
#include <TGLabel.h>
#include <TGButton.h>
#include <TGSplitter.h>    // TGHorizontal3DLine
#include <TGTextEntry.h>
#include <TGLayout.h>

#include "MCaos.h"
#include "MGImage.h"
#include "Camera.h"
#include "../src/DimSetup.h"
#include "Led.h"
//#include "Writer.h"
#include "FilterLed.h"
//#include "CaosFilter.h"

using namespace std;

enum {
    IDM_kFilter,
    IDM_kFindStar,
    IDM_kCaosFilter,
    IDM_kStarguider,
    IDM_kStretch,
    IDM_kInput,
    IDM_kSetup,
    IDM_kCut,
    IDM_kInterpol250,
    IDM_kInterpol125,
    IDM_kInterpol50,
    IDM_kInterpol25,
    IDM_kInterpol10,
    IDM_kInterpol5,
    IDM_kInterpol2,
    IDM_kInterpol1,
    //IDM_kCaosPrintRings,
    //IDM_kCaosPrintLeds,
    IDM_kCaosWriteStart,
    IDM_kCaosWriteStop,
    IDM_kResetHistograms,
};

Bool_t MStarguider::HandleTimer(TTimer *)
{
    if (IsMapped())
        fImage->DoRedraw();
 
    return kTRUE;
}

#define kZOOM 96
/*
XY MStarguider::GetCoordinates() const
{
    return fPZdAz->GetCoordinates();
}
*/
void MStarguider::InitGui(Int_t)
{
    //fList = new MGList;
    //fList->SetOwner();

/*
 const TGWindow *p=gClient->GetRoot();

    fChannel = new TGPopupMenu(p);
    fChannel->AddEntry("Starfield Camera", IDM_kChannel1);
    fChannel->AddEntry("TPoint Camera",    IDM_kChannel2);
    //fChannel->AddEntry("Read from File",   IDM_kChannel3);
    //if (channel<0)
    //    fChannel->CheckEntry(IDM_kChannel3);
    //else
        fChannel->CheckEntry(channel==0?IDM_kChannel1:IDM_kChannel2);
    fChannel->Associate(this);
    fList->Add(fChannel);

    fFileType = new TGPopupMenu(p);
    fFileType->AddEntry("PP&M", IDM_kPPM);
    fFileType->AddEntry("&PNG", IDM_kPNG);
    fFileType->CheckEntry(IDM_kPNG);
    fFileType->Associate(this);
    fList->Add(fFileType);

    fWriteType = new TGPopupMenu(p);
    fWriteType->AddEntry("&Once",      IDM_kOnce);
    fWriteType->AddEntry("&Continous", IDM_kContinous);
    fWriteType->CheckEntry(IDM_kOnce);
    fWriteType->Associate(this);
    fList->Add(fWriteType);

    fWriteRate = new TGPopupMenu(p);
    fWriteRate->AddEntry("25/s", IDM_kRate25ps);
    fWriteRate->AddEntry("5/s",  IDM_kRate5ps);
    fWriteRate->AddEntry("1s",   IDM_kRate1s);
    fWriteRate->AddEntry("5s",   IDM_kRate5s);
    fWriteRate->AddEntry("30s",  IDM_kRate30s);
    fWriteRate->AddEntry("1min", IDM_kRate1m);
    fWriteRate->AddEntry("5min", IDM_kRate5m);
    fWriteRate->CheckEntry(IDM_kRate1m);
    fWriteRate->Associate(this);
    fList->Add(fWriteRate);

    fWrtRate = 25*60;

    fLimMag = new TGPopupMenu(p);
    fLimMag->AddEntry("3", IDM_kLimMag3);
    fLimMag->AddEntry("4", IDM_kLimMag4);
    fLimMag->AddEntry("5", IDM_kLimMag5);
    fLimMag->AddEntry("6", IDM_kLimMag6);
    fLimMag->AddEntry("7", IDM_kLimMag7);
    fLimMag->AddEntry("8", IDM_kLimMag8);
    fLimMag->AddEntry("9", IDM_kLimMag9);
    fLimMag->CheckEntry(IDM_kLimMag9);
    fLimMag->Associate(this);
    fList->Add(fLimMag);
    */
    //fSao->SetLimitMag(9.0);

    const TGWindow *p=gClient->GetRoot();

    fInterpol = new TGPopupMenu(p);
    fInterpol->AddEntry("250", IDM_kInterpol250);
    fInterpol->AddEntry("125", IDM_kInterpol125);
    fInterpol->AddEntry("50",  IDM_kInterpol50);
    fInterpol->AddEntry("25",  IDM_kInterpol25);
    fInterpol->AddEntry("10",  IDM_kInterpol10);
    fInterpol->AddEntry("5",   IDM_kInterpol5);
    fInterpol->AddEntry("2",   IDM_kInterpol2);
    fInterpol->AddEntry("Off", IDM_kInterpol1);
    fInterpol->Associate(this);
    //fList->Add(fInterpol);

    TString disp=gVirtualX->DisplayName();
    cout << "Display: " << disp << endl;
    if (disp.First(':')>=0)
        disp=disp(0, disp.First(':'));

    if (disp.IsNull() || disp==(TString)"localhost")
    {
        fInterpol->CheckEntry(IDM_kInterpol25);
        fIntRate = 25;
    }
    else
    {
        fInterpol->CheckEntry(IDM_kInterpol125);
        fIntRate = 125;
    }

/*
    fCaosPrint = new TGPopupMenu(p);
    fCaosPrint->AddEntry("&Leds",  IDM_kCaosPrintLeds);
    fCaosPrint->AddEntry("&Rings", IDM_kCaosPrintRings);
    fCaosPrint->Associate(this);
    fList->Add(fCaosPrint);

    fCaosWrite = new TGPopupMenu(p);
    fCaosWrite->AddEntry("&Start", IDM_kCaosWriteStart);
    fCaosWrite->AddEntry("Sto&p",  IDM_kCaosWriteStop);
    fCaosWrite->DisableEntry(IDM_kCaosWriteStop);
    fCaosWrite->Associate(this);
    fList->Add(fCaosWrite);

    fCaosAnalyse = new TGPopupMenu(p);
    fCaosAnalyse->AddEntry("S&tart Analysis", IDM_kCaosAnalStart);
    fCaosAnalyse->AddEntry("St&op Analysis",  IDM_kCaosAnalStop);
    fCaosAnalyse->DisableEntry(IDM_kCaosAnalStop);
    fCaosAnalyse->Associate(this);
    fList->Add(fCaosAnalyse);
*/
    fMenu = new TGMenuBar(this, 0, 0, kHorizontalFrame);
    fDisplay       = fMenu->AddPopup("&Display");
    //fMode          = fMenu->AddPopup("&Mode");
    //fWritePictures = fMenu->AddPopup("&WritePics");
    fSetup         = fMenu->AddPopup("&Setup");
    //fOperations    = fMenu->AddPopup("&Operations");
    fMenu->Resize(fMenu->GetDefaultSize());
    AddFrame(fMenu);

    //
    // Create Menu for MStarguider Display
    //
    //fDisplay = new MMGPopupMenu(p);
    fDisplay->AddEntry("&Filter",               IDM_kFilter);
    fDisplay->AddEntry("Stretch",               IDM_kStretch);
    fDisplay->AddSeparator();
    fDisplay->AddEntry("Find &Star",            IDM_kFindStar);
    fDisplay->AddEntry("C&aos Filter",          IDM_kCaosFilter);
    //fDisplay->AddSeparator();
    //if (channel>=0)
    //    fDisplay->AddPopup("&Input",   fChannel);
    // fDisplay->CheckEntry(IDM_kStretch);
    fDisplay->CheckEntry(IDM_kFindStar);
    fDisplay->CheckEntry(IDM_kCaosFilter);
    fDisplay->Associate(this);

    //fMode->AddEntry("Tpoint",     IDM_kTpointMode);
    //fMode->Associate(this);
/*
    fWritePictures->AddEntry("&Start",      IDM_kStart);
    fWritePictures->AddEntry("Sto&p",       IDM_kStop);
    fWritePictures->AddSeparator();
    //fWritePictures->AddPopup("File &Type",  fFileType);
    fWritePictures->AddPopup("&Write Type", fWriteType);
    fWritePictures->AddPopup("Write &Rate", fWriteRate);
    fWritePictures->DisableEntry(IDM_kStop);
    fWritePictures->Associate(this);
    */
    fSetup->AddPopup("Disp. &Interpolation", fInterpol);
    fSetup->Associate(this);

/*
    fCaOs = new TGPopupMenu(p);
    //fCaOs->AddPopup("&Write",   fCaosWrite);
    fCaOs->AddPopup("&Print",   fCaosPrint);
    //fCaOs->AddPopup("&Analyse", fCaosAnalyse);
    fCaOs->Associate(this);
    fList->Add(fCaOs);
*/
    //RA,Dec for catalog
    /*
    fCRaDec = new MGCoordinates(this, kETypeRaDec);
    fCRaDec->Move(4, fMenu->GetDefaultHeight()+584);
    AddFrame(fCRaDec);

    //telescope position
    fCZdAz = new MGCoordinates(this, kETypeZdAz, 2);
    fCZdAz->Move(240+12+28, fMenu->GetDefaultHeight()+597);
    AddFrame(fCZdAz);

    //starguider position
    fPZdAz = new MGCoordinates(this, kETypeZdAz, 2);
    fPZdAz->Move(240+12+28, fMenu->GetDefaultHeight()+640);
    AddFrame(fPZdAz);

    //mispointing
    fDZdAz = new MGCoordinates(this, kETypeZdAz, 2);
    fDZdAz->Move(240+12+28, fMenu->GetDefaultHeight()+683);
    AddFrame(fDZdAz);

    fSZdAz = new MGCoordinates(this, kETypeZdAz, 2);
    fSZdAz->Move(240+12+28, fMenu->GetDefaultHeight()+795);
    AddFrame(fSZdAz);

    fGNumStars = new MGNumStars(this, 235);
    fGNumStars->DrawText("Number of stars");
    fGNumStars->Move(278, fMenu->GetDefaultHeight()+713);
    fList->Add(fGNumStars);

    fTPoint = new TGTextButton(this, "TPoint");
    //fTPoint->Move(4, fMenu->GetDefaultHeight()+785);
    fTPoint->Move(170, fMenu->GetDefaultHeight()+785);
    fTPoint->AllowStayDown(kTRUE);
    AddFrame(fTPoint);

    fStargTPoint = new TGTextButton(this, "StargTPoint");
     fStargTPoint->Move(170, fMenu->GetDefaultHeight()+785);
     fStargTPoint->AllowStayDown(kTRUE);
     AddFrame(fStargTPoint);

     fFps = new TGLabel(this, "---fps");
    fFps->SetTextJustify(kTextRight);
    fFps->Move(650-495, fMenu->GetDefaultHeight()+714+23);
    AddFrame(fFps);

    fPosZoom = new TGLabel(this, "(----, ----) ----.--d/----.--d");
    fPosZoom->SetTextJustify(kTextLeft);
    fPosZoom->Move(4, fMenu->GetDefaultHeight()+765);
    AddFrame(fPosZoom);

    fSkyBright = new TGLabel(this, "Sky Brightness: ---         ");
    fSkyBright->SetTextJustify(kTextLeft);
    fSkyBright->Move(4, fMenu->GetDefaultHeight()+785);
    AddFrame(fSkyBright);

    TGLabel *l = new TGLabel(this, "deg");
    l->SetTextJustify(kTextLeft);
    l->Move(606-412, fMenu->GetDefaultHeight()+669);
    AddFrame(l);

    l = new TGLabel(this, "arcsec/pix");
    l->SetTextJustify(kTextLeft);
    l->Move(606-412, fMenu->GetDefaultHeight()+692);
    AddFrame(l);

    l = new TGLabel(this, "sigma");
    l->SetTextJustify(kTextLeft);
    l->Move(606-412, fMenu->GetDefaultHeight()+715);
    AddFrame(l);

    fCZdAzText = new TGLabel(this, "Zd/Az telescope pointing at");
    fCZdAzText->SetTextJustify(kTextLeft);
    fCZdAzText->Move(240+12+20+7, fMenu->GetDefaultHeight()+584-5);
    AddFrame(fCZdAzText);

    fPZdAzText = new TGLabel(this, "Zd/Az starguider pointing at");
    fPZdAzText->SetTextJustify(kTextLeft);
    fPZdAzText->Move(240+12+20+7, fMenu->GetDefaultHeight()+630+20-5-23);
    AddFrame(fPZdAzText);

    fDZdAzText = new TGLabel(this, "Zd/Az mispointing");
    fDZdAzText->SetTextJustify(kTextLeft);
    fDZdAzText->Move(240+12+20+7, fMenu->GetDefaultHeight()+676+2*20-5-46);
    AddFrame(fDZdAzText);
*/
    // Set input box for rotation angle
    /*
    fAngle = new TGTextEntry(this, "           ", IDM_kAngle);
    fAngle->SetAlignment(kTextCenterX);
    fAngle->Move(547-410, fMenu->GetDefaultHeight()+667);
    AddFrame(fAngle);

    //SetRotationAngle(-0.2);

    // Set input box for pixel size
    fPixSize = new TGTextEntry(this, "           ", IDM_kPixSize);
    fPixSize->SetAlignment(kTextCenterX);
    fPixSize->Move(547-410, fMenu->GetDefaultHeight()+690);
    AddFrame(fPixSize);
    */
    //SetPixSize(48.9);

    // Set input box for cleaning cut
    //fCut = new TGTextEntry(this, "           ", IDM_kCut);
    //fCut->SetAlignment(kTextCenterX);
    //fCut->Move(547-410, fMenu->GetDefaultHeight()+713);
    //AddFrame(fCut);

    //SetCut(3.0);

    // TGHorizontal3DLine *fLineSep = new TGHorizontal3DLine(this);
    // AddFrame(fLineSep, new TGLayoutHints (kLHintsNormal | kLHintsExpandX));
    // fList->Add(fLineSep);

    //
    // Create Image Display
    /*
    fZoomImage = new MGImage(this, kZOOM, kZOOM);
    // fZoomImage->Move(768-kZOOM-2, 700-kZOOM-2);
    fZoomImage->Move(4, 700-kZOOM-2+85);
    AddFrame(fZoomImage);
    */
    fImage = new MGImage(this, 768, 576);
    fImage->Move(0, fMenu->GetDefaultHeight());
    AddFrame(fImage);

    const Int_t w = 768;
    const Int_t h = 576;
    SetWMSizeHints(w, h, w, h, 1, 1);  // set the smallest and biggest size of the Main frame

    //
    // Make everything visible
    //
    SetWindowName("TPoint Main Window");
    SetIconName("TPoint");

    MapSubwindows();
    //fTPoint->UnmapWindow();
    //fStargTPoint->UnmapWindow();
    //fGStarg->UnmapWindow();
    //fGNumStars->UnmapWindow();
    //fCRaDec->UnmapWindow();
    //fCZdAz->UnmapWindow();
    //fCZdAzText->UnmapWindow();
    //fPZdAz->UnmapWindow();
    //fPZdAzText->UnmapWindow();
    //fDZdAz->UnmapWindow();
    //fDZdAzText->UnmapWindow();
    //fSZdAz->UnmapWindow();
    //fSkyBright->UnmapWindow();
    MapWindow();


    //IconifyWindow();

    //------------------------------------------------------------
    //    XY xy(3.819444, 24.05333);
    //    fCRaDec->SetCoordinates(xy);
    //    fRaDec->Set(xy.X()*360/24, xy.Y());
    //------------------------------------------------------------
}

MStarguider::MStarguider(Int_t channel) : TGMainFrame(gClient->GetRoot(), 768, 840),
fDimData("TPOINT/DATA", "D:11", (void*)NULL, 0),
fDimTPoint("TPOINT/EXECUTE", "", this),
fDimScreenshot("TPOINT/SCREENSHOT", "B:1;C", this),
fRadius(200), fFindStarCut(2.), fFindStarBox(30), fTPointMode(0)
{
    cout << " #### FIXME: Make MCaos Thread safe!" << endl;

    // This means that all objects added with AddFrame are deleted
    // automatically, including all LayoutHints.
    SetCleanup();

    fCaos = new MCaos;
    fCaos->ReadResources("leds_fact.txt");

    InitGui(channel);

    fTimer=new TTimer(this, 1000/25); // 40ms
    fTimer->TurnOn();

    gVirtualX->GrabButton(fId, kButton2, 0, 0, 0, 0, kTRUE);

    fGetter = new Camera(*this, channel);

    DimClient::setNoDataCopy();
    DimServer::start("TPOINT");
}

MStarguider::~MStarguider()
{
    DimServer::stop();

    fGetter->ExitLoop();
    delete fGetter;

    gVirtualX->GrabButton(fId, kButton2, 0, 0, 0, 0, kFALSE);

    fTimer->TurnOff();
    delete fTimer;

    delete fInterpol;

    //delete fList;
    delete fCaos;

    cout << "Camera Display destroyed." << endl;
}

/*
void MStarguider::SetupEnv(TEnv &env)
{
    fCaos->ReadEnv(env, "TPointLeds", kTRUE);
    //fStargCaos->ReadEnv(env, "StarguiderLeds", kTRUE);

    //SetRotationAngle(env.GetValue("Starguider.RotationAngle", fSao->GetRotationAngle()));
    //SetCut(env.GetValue("Starguider.CleaningLevel", atof(fCut->GetText())));

    //SetPixSize(env.GetValue("StarguiderLeds.ArcsecPerPixel", fSao->GetPixSize()));

    fRadius = env.GetValue("Leds.Radius", fRadius);

    fStarguiderW = env.GetValue("Starguider.Width",  fStarguiderW);
    fStarguiderH = env.GetValue("Starguider.Height", fStarguiderH);
    fStarguiderX = env.GetValue("Starguider.X",      fStarguiderX);
    fStarguiderY = env.GetValue("Starguider.Y",      fStarguiderY);

    fSkyOffsetX = env.GetValue("Starguider.SkyOffsetX", fSkyOffsetX);
    fSkyOffsetY = env.GetValue("Starguider.SkyOffsetY", fSkyOffsetY);

    fFindStarBox = env.GetValue("FindStar.SizeBox",       fFindStarBox);
    fFindStarCut = env.GetValue("FindStar.CleaningLevel", fFindStarCut);
}
*/

void MStarguider::Layout()
{
    // Resize(GetDefaultSize());
}

void MStarguider::CloseWindow()
{
    cout << "EventDisplay::CloseWindow: Exit Application Loop." << endl;

    //fClient.ExitLoop();
    //    cout << "FIXME: ExitLoop not called!!!!!!" << endl;
    fGetter->ExitLoop();
    gSystem->ExitLoop();
}

void MStarguider::SwitchOff(TGPopupMenu *p, UInt_t id)
{
    p->UnCheckEntry(id);
    p->DisableEntry(id);
}

/*
void MStarguider::SetChannel()
{
    if (fChannel->IsEntryChecked(IDM_kChannel3))
    {
        if (dynamic_cast<PngReader*>(fGetter)==0)
        {
            delete fGetter;
            fGetter=new PngReader(*this);
        }
    }
    else
    {
        const Int_t ch = fChannel->IsEntryChecked(IDM_kChannel1) ? 0 : 1;
        if (dynamic_cast<Camera*>(fGetter)==0)
        {
            delete fGetter;
            fGetter = new Camera(*this, ch);
        }
        else
            fGetter->SetChannel(ch);
    }
}*/

void MStarguider::Toggle(TGPopupMenu *p, UInt_t id)
{
    if (p->IsEntryChecked(id))
        p->UnCheckEntry(id);
    else
        p->CheckEntry(id);
}


Bool_t MStarguider::ProcessMessage(Long_t msg, Long_t mp1, Long_t)
{
    switch (GET_MSG(msg))
    {
    case kC_TEXTENTRY:
        if (GET_SUBMSG(msg)==kTE_ENTER)
            switch (mp1)
            {
                /*
            case IDM_kPixSize:
                {
                    const Float_t pixsize = atof(fPixSize->GetText());
                    gLog << all << "Pixel Size changed to " << pixsize << "\"/pix" << endl;
                    fSao->SetPixSize(pixsize);
                    return kTRUE;
                }
            case IDM_kAngle:
                {
                    const Float_t angle = atof(fAngle->GetText());
                    gLog << all << "Rotation Angle changed to " << angle << "deg" << endl;
                    fSao->SetRotationAngle(angle);
                    return kTRUE;
                }
            case IDM_kCut:
                {
                    const Float_t cut = atof(fCut->GetText());
                    gLog << all << "Starguider cleaning level changed to " << cut << " sigma." << endl;
                    return kTRUE;
                }*/
            }
        return kTRUE;

    case kC_COMMAND:
        switch (GET_SUBMSG(msg))
        {
        case kCM_MENU:
            switch (mp1)
            {
/*
            case IDM_kTpointMode:
                Toggle(fMode, IDM_kTpointMode);

                if (fMode->IsEntryChecked(IDM_kTpointMode))
                {
                    //unchecking not needed items
                    //general
                    SwitchOff(fDisplay, IDM_kFilter);
                    SwitchOff(fChannel, IDM_kChannel3);

                    //from starguider
                    //SwitchOff(fDisplay, IDM_kStargCaosFilter);
                    //SwitchOff(fDisplay, IDM_kCatalog);
                    //SwitchOff(fDisplay, IDM_kStarguider);
                    //ToggleStarguider();
                    //fMode->UnCheckEntry(IDM_kStarguiderMode);
                    //SwitchOff(fOperations, IDM_kStargAnalysis);
                    //ToggleStargAnalysis();

                    //switch camera
                    SwitchOff(fChannel, IDM_kChannel1);
                    fChannel->CheckEntry(IDM_kChannel2);

                    SetChannel();

                    //checking needed items
                    fDisplay->UnCheckEntry(IDM_kStretch);
                    fDisplay->CheckEntry(IDM_kCaosFilter);
                    ToggleCaosFilter();
                    fDisplay->CheckEntry(IDM_kFindStar);
                    fTPoint->MapWindow();
                }
                else
                {
                    //enable
                    //starguider items
                    //fDisplay->EnableEntry(IDM_kStargCaosFilter);
                    //fDisplay->EnableEntry(IDM_kCatalog);
                    //fDisplay->EnableEntry(IDM_kStarguider);
                    //fOperations->EnableEntry(IDM_kStargAnalysis);

                    //general
                    fDisplay->EnableEntry(IDM_kFilter);
                    fChannel->EnableEntry(IDM_kChannel1);
                    fChannel->EnableEntry(IDM_kChannel3);

                    //tpoint
                    fDisplay->UnCheckEntry(IDM_kCaosFilter);
                    ToggleCaosFilter();
                    fDisplay->UnCheckEntry(IDM_kFindStar);
                    fTPoint->UnmapWindow();
                }
                return kTRUE;
*/
            case IDM_kFilter:
                Toggle(fDisplay, IDM_kFilter);           
                return kTRUE;

            case IDM_kFindStar:
                Toggle(fDisplay, IDM_kFindStar);
                //ToggleFindStar();
                return kTRUE;

            case IDM_kStretch:
                Toggle(fDisplay, IDM_kStretch);
                return kTRUE;

            case IDM_kCaosFilter:
                Toggle(fDisplay, IDM_kCaosFilter);
                //ToggleCaosFilter();
                return kTRUE;
/*
            case IDM_kCaosPrintLeds:
            case IDM_kCaosPrintRings:
                Toggle(fCaosPrint, mp1);
                return kTRUE;

            case IDM_kCaosAnalStart:
                fCaosAnalyse->DisableEntry(IDM_kCaosAnalStart);
                fCaosAnalyse->EnableEntry(IDM_kCaosAnalStop);
                //fCaos->InitHistograms();
                return kTRUE;

            case IDM_kCaosAnalStop:
                fCaosAnalyse->DisableEntry(IDM_kCaosAnalStop);
                fCaosAnalyse->EnableEntry(IDM_kCaosAnalStart);
                fCaos->ShowHistograms();
                fCaos->DeleteHistograms();
                return kTRUE;

            case IDM_kCaosWriteStart:
                fCaosWrite->DisableEntry(IDM_kCaosWriteStart);
                fCaosWrite->EnableEntry(IDM_kCaosWriteStop);
                fCaos->OpenFile();
                return kTRUE;

            case IDM_kCaosWriteStop:
                fCaosWrite->DisableEntry(IDM_kCaosWriteStop);
                fCaosWrite->EnableEntry(IDM_kCaosWriteStart);
                fCaos->CloseFile();
                return kTRUE;

            case IDM_kStart:
                fWritePictures->DisableEntry(IDM_kStart);
                fWritePictures->EnableEntry(IDM_kStop);
                return kTRUE;

            case IDM_kStop:
                fWritePictures->DisableEntry(IDM_kStop);
                fWritePictures->EnableEntry(IDM_kStart);
                return kTRUE;

            case IDM_kPNG:
                fFileType->CheckEntry(IDM_kPNG);
                fFileType->UnCheckEntry(IDM_kPPM);
                return kTRUE;

            case IDM_kPPM:
                fFileType->CheckEntry(IDM_kPPM);
                fFileType->UnCheckEntry(IDM_kPNG);
                return kTRUE;

            case IDM_kOnce:
                fWriteType->CheckEntry(IDM_kOnce);
                fWriteType->UnCheckEntry(IDM_kContinous);
                return kTRUE;

            case IDM_kContinous:
                fWriteType->CheckEntry(IDM_kContinous);
                fWriteType->UnCheckEntry(IDM_kOnce);
                return kTRUE;

            case IDM_kRate25ps:
            case IDM_kRate5ps:
            case IDM_kRate1s:
            case IDM_kRate5s:
            case IDM_kRate30s:
            case IDM_kRate1m:
            case IDM_kRate5m:
                for (int i=IDM_kRate25ps; i<=IDM_kRate5m; i++)
                    if (mp1==i)
                        fWriteRate->CheckEntry(i);
                    else
                        fWriteRate->UnCheckEntry(i);
                switch (mp1)
                {
                case IDM_kRate25ps:
                    fWrtRate = 1;
                    return kTRUE;
                case IDM_kRate5ps:
                    fWrtRate = 5;
                    return kTRUE;
                case IDM_kRate1s:
                    fWrtRate = 25;
                    return kTRUE;
                case IDM_kRate5s:
                    fWrtRate = 5*25;
                    return kTRUE;
                case IDM_kRate30s:
                    fWrtRate = 30*25;
                    return kTRUE;
                case IDM_kRate1m:
                    fWrtRate = 60*25;
                    return kTRUE;
                case IDM_kRate5m:
                    fWrtRate = 5*60*25;
                    return kTRUE;
                }
                return kTRUE;

            case IDM_kChannel1:
            case IDM_kChannel2:
                {
                    const Int_t ch0 = fChannel->IsEntryChecked(IDM_kChannel1) ? 0 : 1;
                    const Int_t ch1 = mp1==IDM_kChannel1                      ? 0 : 1;

		    if (ch0==ch1)
                        return kTRUE;

                    fChannel->CheckEntry  (ch1==0?IDM_kChannel1:IDM_kChannel2);
                    fChannel->UnCheckEntry(ch1==1?IDM_kChannel1:IDM_kChannel2);

                    SetChannel();
                }
                return kTRUE;
             */
            case IDM_kInterpol250:
            case IDM_kInterpol125:
            case IDM_kInterpol50:
            case IDM_kInterpol25:
            case IDM_kInterpol10:
            case IDM_kInterpol5:
            case IDM_kInterpol2:
            case IDM_kInterpol1:
                for (int i=IDM_kInterpol250; i<=IDM_kInterpol1; i++)
                    if (mp1==i)
                        fInterpol->CheckEntry(i);
                    else
                        fInterpol->UnCheckEntry(i);
                switch (mp1)
                {
                case IDM_kInterpol1:
                    fIntRate = 1;
                    return kTRUE;
                case IDM_kInterpol2:
                    fIntRate = 2;
                    return kTRUE;
                case IDM_kInterpol5:
                    fIntRate = 5;
                    return kTRUE;
                case IDM_kInterpol10:
                    fIntRate = 10;
                    return kTRUE;
                case IDM_kInterpol25:
                    fIntRate = 25;
                    return kTRUE;
                case IDM_kInterpol50:
                    fIntRate = 50;
                    return kTRUE;
                case IDM_kInterpol125:
                    fIntRate = 125;
                    return kTRUE;
                case IDM_kInterpol250:
                    fIntRate = 250;
                    return kTRUE;
                }
                return kTRUE;
            }
            break;
        }
        break;
    }

    return kTRUE;
}
/*
void MStarguider::DrawZoomImage(const byte *img)
{
    byte zimg[kZOOM*kZOOM];
    for (int y=0; y<kZOOM; y++)
        for (int x=0; x<kZOOM; x++)
            zimg[x+y*kZOOM] = img[(fDx+(x-kZOOM/2)/2)+(fDy+(y-kZOOM/2)/2)*768];

    fZoomImage->DrawImg(zimg);
}

void MStarguider::DrawCosyImage(const byte *img)
{
    if (!fCosy)
        return;

    byte simg[(768/2-1)*(576/2-1)];
    for (int y=0; y<576/2-1; y++)
        for (int x=0; x<768/2-1; x++)
            simg[x+y*(768/2-1)] = ((unsigned int)img[2*x+2*y*768]+img[2*x+2*y*768+1]+img[2*x+2*(y+1)*768]+img[2*x+2*(y+1)*768+1])/4;

    fCosy->GetWin()->GetImage()->DrawImg(simg);
}

Led MStarguider::FindStar(const FilterLed &f, const FilterLed &f2, const Ring &center, Int_t numleds, Int_t numrings)
{
    // Get tracking coordinates
    //const XY xy = fCRaDec->GetCoordinates();  // [h, deg]

    if (center.GetX()<=0 && center.GetY()<=0)
    {
        cout << "Couldn't determine center of the camera." << endl;
        //if (fTPoint->IsDown() && fCosy && fCosy->GetDriveCom())
        //    fCosy->GetDriveCom()->SendTPoint(false, 'T', fTPointStarMag, fTPointStarName, AltAz(), ZdAz(), xy, 0, 0, t, center, Led(), numleds, numrings);    // Report
        return;
    }

    // Try to find the star
    Leds leds;
    f.FindStar(leds, (Int_t)center.GetX(), (Int_t)center.GetY(), true);

    // Check whether star found
    if (leds.size()==0)
    {
        cout << "No star found." << endl;
        //if (fTPoint->IsDown() && fCosy && fCosy->GetDriveCom())
        //    fCosy->GetDriveCom()->SendTPoint(false, 'T', fTPointStarMag, fTPointStarName, AltAz(), ZdAz(), xy, 0, 0, t, center, Led(), numleds, numrings);    // Report
        return;
    }

    cout << "Found star @ " << flush;

    const Led &star = leds.front();

    star.Print();
    f2.MarkPoint(star.GetX(), star.GetY(), 2<<2);

    return star;

    const RaDec rd(xy.X()*MAstro::HorToRad(), xy.Y()*TMath::DegToRad());

    // Initialize Star Catalog on the camera plane
    MGeomCamMagic geom;
    MAstroCamera ac;
    ac.SetGeom(geom);
    ac.SetRadiusFOV(3);
    //ac.SetObservatory(*fSao);
    ac.SetTime(t);
    ac.SetRaDec(rd.Ra(), rd.Dec());

    // Convert from Pixel to millimeter (1pix=2.6mm) [deg/pix / deg/mm = mm/pix]
    // Correct for abberation.
    const double sec_per_pix = 45.311;
    const double mm_to_deg   = 0.011693;
    const Double_t conv = sec_per_pix/3600/mm_to_deg / 1.0713;

    // Adapt coordinate system (GUIs and humans are counting Y in different directions)
    const Double_t dx = (star->GetX()-center.GetX())*conv;
    const Double_t dy = (center.GetY()-star->GetY())*conv;

    // Convert offset from camera plane into local ccordinates
    Double_t dzd, daz;
    ac.GetDiffZdAz(dx, dy, dzd, daz);

    ZdAz zdaz(dzd,daz);

    // Check TPoint data set request
    if (!fMode->IsEntryChecked(IDM_kTpointMode) || !fTPoint->IsDown())
        return;

    // If no file open: open new file
    if (!fOutTp)
    {
        //
        // open tpoint file
        //

        const TString name = MCosy::GetFileName("tpoint", "tpoint", "txt");
        cout << "TPoint File ********* " << name << " ********** " << endl;

        fOutTp = new ofstream(name);
        *fOutTp << "Magic Model  TPOINT data file" << endl;
        *fOutTp << ": ALTAZ" << endl;
        *fOutTp << "49 48 0 ";
        *fOutTp << t << endl;
        // temp(°C) pressure(mB) height(m) humidity(1) wavelength(microm) troplapserate(K/m
    }

    // Output Ra/Dec the drive system thinks that it is currently tracking
    //cout << "TPoint Star: " << xy.X() << "h " << xy.Y() << "°" << endl;

    // From the star position in the camera we calculate the Alt/Az
    // position we are currently tracking (real pointing position)
    fSao->SetMjd(t.GetMjd());
    AltAz za0 = fSao->CalcAltAz(rd)*kRad2Deg;

    //ZdAz za0 = fSao->GetZdAz();
    za0 -= AltAz(-dzd, daz);
    fAltAzOffsetFromTp = AltAz(-dzd, daz);
    fTimeFromTp=t;

    // From the Shaftencoders we get the current 'pointing' position
    // as it is seen by the drive system (system pointing position)
    const ZdAz za1 = fCosy->GetSePos()*360; // [deg]


    // Write real pointing position
    //cout << "     Alt/Az: " << za0.Alt() << "° " << za0.Az() << "°" << endl;
    *fOutTp << setprecision(7) << za0.Az() << " " << za0.Alt() << " ";

    // Write system pointing position
    //cout << "     SE-Pos: " << 90-za1.Zd() << "° " << za1.Az() << "°" << endl;
    *fOutTp << fmod(za1.Az()+360, 360) << " " << 90-za1.Zd();

    *fOutTp << " " << xy.X() << " " << xy.Y();
    *fOutTp << " " << -dzd << " " << -daz;
    *fOutTp << " " << setprecision(11) << t.GetMjd();
    *fOutTp << " " << setprecision(4) << center.GetMag();
    *fOutTp << " " << star->GetMag();
    *fOutTp << " " << center.GetX() << " " << center.GetY();
    *fOutTp << " " << star->GetX() << " " << star->GetY();
    *fOutTp << " " << numleds << " " << numrings;
    *fOutTp << " 0 0 0";
    *fOutTp << " " << fTPointStarMag << " " << fTPointStarName;
    *fOutTp << endl;

    gLog << all << "TPoint successfully taken." << endl;

    MLog &outrep = *fCosy->GetOutRep();
    if (outrep.Lock("MStarguider::FindStar"))
    {
        outrep << "FINDSTAR-REPORT 00 " << MTime(-1) << " " << setprecision(7);
        outrep << 90-za0.Alt() << " " << za0.Az() << " ";
        outrep << za1.Zd() << " " << za1.Az() << " ";
        outrep << xy.X() << " " << xy.Y() << " ";
        outrep << -dzd << " " << -daz << " ";
        outrep << star->GetX() << " " << star->GetY() << " ";
        outrep << center.GetX() << " " << center.GetY() << " ";
        outrep << dx/conv << " " << dy/conv << " " << star->GetMag();
        outrep << setprecision(11) << t.GetMjd() << endl;
        outrep.UnLock("MStarguider::FindStar");
    }
    
//    return zdaz;

    if (!fCosy)
        return;

    MDriveCom *com = fCosy->GetDriveCom();
    if (!com)
        return;

    com->SendTPoint(true, 'T', fTPointStarMag, fTPointStarName, za0, za1, xy, -dzd, -daz, t, center, *star, numleds, numrings);    // Report
}
*/

void MStarguider::WritePNG(const char *name, const byte *gbuf, const byte *cbuf)
{
    string fname(name);
    if (fname.size()<4 || fname.compare(fname.size()-4,4, ".png")==0)
        fname += ".png";

    cout << "Writing PNG '" << fname << "'" << endl;

#ifdef HAVE_PNG
    //
    // open file
    //
    FILE *fd = fopen(fname.c_str(), "w");
    if (!fd)
    {
        cout << "Warning: Cannot open file for writing." << endl;
        return;
    }

    //
    // allocate memory
    //
    png_structp fPng = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               NULL, NULL, NULL);

    if (!fPng)
    {
        cout << "Warning: Unable to create PNG structure" << endl;
        fclose(fd);
        return;
    }


    png_infop fInfo = png_create_info_struct(fPng);

    if (!fInfo)
    {
        cout << "Warning: Unable to create PNG info structure" << endl;
        png_destroy_write_struct (&fPng, NULL);
        fclose(fd);
        return;
    }

    fInfo->width      = 768;
    fInfo->height     = 576;
    fInfo->bit_depth  = 8;
    fInfo->color_type = PNG_COLOR_TYPE_RGB;
//    fInfo->color_type = PNG_COLOR_TYPE_GRAY;

    //
    // set jump-back point in case of errors
    //
    if (setjmp(fPng->jmpbuf))
    {
        cout << "longjmp Warning: PNG encounterd an error!" << endl;
        png_destroy_write_struct (&fPng, &fInfo);
        fclose(fd);
        return;
    }

    //
    // connect file to PNG-Structure
    //
    png_init_io(fPng, fd);

    // png_set_compression_level (fPng, Z_BEST_COMPRESSION);

    //
    // Write header
    //
    png_write_info(fPng, fInfo);

    png_byte buf[768*576*3];

    png_byte *d = buf;
    const byte *g = gbuf;
    const byte *c = cbuf;

    // d=destination, s1=source1, s2=source2, e=end
    while (d<buf+768*576*3)
    {
        if (fScreenshotColor && *c)
        {
            *d++ = ((*c>>4)&0x3)*85;
            *d++ = ((*c>>2)&0x3)*85;
            *d++ = ((*c++ )&0x3)*85;
            g++;
        }
        else
        {
            *d++ = *g;
            *d++ = *g;
            *d++ = *g++;
            c++;
        }
    }

    //
    // Write bitmap data
    //
    for (unsigned int y=0; y<768*576*3; y+=768*3)
	png_write_row (fPng, buf+y);

    //
    // Write footer
    //
    png_write_end (fPng, fInfo);

    //
    // free memory
    //
    png_destroy_write_struct (&fPng, &fInfo);

    fclose(fd);
#else
    cout << "Sorry, no png support compiled into tpoint." << endl;
#endif
}

bool MStarguider::Interpolate(const unsigned long n, byte *img) const
{
    if (fIntRate<=1)
        return true;

    const int rate = fTPointMode>0 ? 5*25 : fIntRate;

    static unsigned short myimg[768*576];

    unsigned short *f = myimg;

    const byte *end = img+768*576;

    if (n%rate)
    {
        while (img<end)
            *f++ += *img++;
        return false;
    }
    else
    {
        while (img<end)
        {
            *img = (*img + *f)/rate;
            ++img;
            *f++ = 0;
        }

        return true;
    }
}

void MStarguider::ProcessFrame(const unsigned long n, byte *img,
			       struct timeval *tm)
{
    if (!Interpolate(n, img))
        return;

    if (fTPointMode==2)
    {
        fTPointMode=1;
        return;
    }

    byte cimg[768*576];
    memset(cimg, 0, 768*576);

    FilterLed f (img,  768, 576, 2.5); // 2.5
    FilterLed f2(cimg, 768, 576);      // former color 0xb0

    if (!fTPointMode && fScreenshotName.empty() && fDisplay->IsEntryChecked(IDM_kStretch))
        f.Stretch();

    // Visual Filter, whole FOV
    if (!fTPointMode && fDisplay->IsEntryChecked(IDM_kFilter))
    {
        vector<Led> leds;
        f.Execute(leds, 768/2, 576/2);
        for (auto it=leds.begin(); it!=leds.end(); it++)
            f.MarkPoint(*it);
    }

    // Find Center of Camera for Caos and Tpoints
    int numleds  = 0;
    int numrings = 0;
    Ring center(-1, -1);//(5, 5);

    if (fTPointMode || fDisplay->IsEntryChecked(IDM_kCaosFilter))
    {
        center   = fCaos->Run(img);
        numleds  = fCaos->GetNumDetectedLEDs();
        numrings = fCaos->GetNumDetectedRings();
    }

    //cout << "cx=" << center.GetX() << "   cy=" << center.GetY() << "   Nled=" << numleds << "   Nrings=" << numrings << endl;

    // Find Star at Center---for Tpoint Procedure
    Led star(-1, -1);
    if (center.GetX()>0 && center.GetY()>0)
    {
        if (fTPointMode || fDisplay->IsEntryChecked(IDM_kFindStar))
        {
            // Set search Paremeters (FIXME: Get them from user input!)
            f.SetCut(fFindStarCut+1.5);  // FindStar.CleaningLevel
            f.SetBox(fFindStarBox+12/*+80*/);  // FindStar.SizeBox

            // Try to find the star
            vector<Led> leds;
            f.FindStar(leds, (Int_t)center.GetX(), (Int_t)center.GetY(), true);

            // Check whether star found
            if (leds.size()>0)
            {
                //cout << "Found star @ " << flush;
                //leds[0].Print();
                f2.MarkPoint(leds[0].GetX(), leds[0].GetY(), 2<<2);
                star = leds[0];
            }
        }

        // DrawZoomImage(img);
        // DrawCosyImage(img);

        // Position corresponding to the camera center (53.2, 293.6)
        // Draw Circles around center of Camera
        if (fTPointMode || fDisplay->IsEntryChecked(IDM_kCaosFilter))
        {
            f2.DrawCircle(center, 0x0a);
            f2.DrawCircle(center,   7.0,
                          fDisplay->IsEntryChecked(IDM_kFindStar)?3:0xb0);
            //f2.DrawCircle(center, 115.0, 0x0a);
            //f2.DrawCircle(center, 230.0, 0x0a);
            //f2.DrawCircle(center, 245.0, 0x0a);
        }
    }

    if (fTPointMode ||
        fDisplay->IsEntryChecked(IDM_kCaosFilter) ||
        fDisplay->IsEntryChecked(IDM_kFindStar))
        fImage->DrawColImg(img, cimg);
    else
        fImage->DrawImg(img);

    if (fTPointMode && !fScreenshotName.empty())
    {
        WritePNG(fScreenshotName.c_str(), img, cimg);
        fScreenshotName = "";
        fTPointMode = 0;
        return;
    }


    if (star.GetX()<0 || star.GetY()<0 || fTPointMode==0)
        return;

    if (fTPointMode==1)
        fTPointMode=0;

    // Convert from Pixel to millimeter (1pix=2.6mm) [deg/pix / deg/mm = mm/pix]
    // Correct for abberation.

    //const float Dleds =     510; // 5.96344 deg
    //const float Dpix  = 2*237.58;

    // The DC reflector elongates light from off axis sources
    // This is a correction. It is 7% for MAGIC (1:1) and
    // less for FACT (1:1.4). This is an estimate from a
    // Orbit mode observation at 0.17deg distance to the
    // camera center [4.90m might also not be very accurate
    // depending on the position of the CCD camera]
    const double abberation  = 1.0638; //1.0713;
    const double sec_per_pix = 45.14;  //45.311;  (atan(510mm/2 / 4.90m) / 237.58)           // FACT LEDs

    const double conv = sec_per_pix/abberation;

    const double dx = star.GetX()-center.GetX();
    const double dy = star.GetY()-center.GetY();

    //const double dxx = - conv * (star.GetX()-center.GetX());
    //const double dyy =   conv * (star.GetY()-center.GetY());

    const double dphi = - center.GetPhi() * M_PI/180;

    // The sign is because the pixels are not counted in
    // both directions in the same direction as Zd/Az
    const double dxx = - conv * (dx*cos(dphi) - dy*sin(dphi));
    const double dyy =   conv * (dx*sin(dphi) + dy*cos(dphi));

    double arr[11] = {
        dxx, dyy,
        double(numleds), double(numrings),
        center.GetX(), center.GetY(), center.GetMag(),
        star.GetX(),   star.GetY(),   star.GetMag(),
        center.GetPhi()
    };

    fDimData.setData(arr, 11*sizeof(double));
    fDimData.setQuality(0);
    fDimData.setTimestamp(tm->tv_sec, tm->tv_usec/1000);
    fDimData.updateService();
}

/*
void MStarguider::UpdatePosZoom()
{
    MString txt;
    if (fDisplay->IsEntryChecked(IDM_kCatalog))
    {
        // FIXME: Necessary?
        fSao->Now();
        AltAz aa = fSao->CalcAltAzFromPix(fDx, fDy)*kRad2Deg;
        if (aa.Az()<0)
            aa.Az(aa.Az()+360);
        txt.Form("(%d, %d) %.1fd/%.1fd", fDx, fDy, -aa.Alt(), aa.Az()-180);
    }
    else
        txt.Form("(%d, %d)", fDx, fDy);
    fPosZoom->SetText(txt);
}

Bool_t MStarguider::HandleDoubleClick(Event_t *event)
{
    const Int_t w = fImage->GetWidth();
    const Int_t h = fImage->GetHeight();
    const Int_t x = fImage->GetX();
    const Int_t y = fImage->GetY();

    if (!(event->fX>x && event->fX<x+w && event->fY>y && event->fY<y+h))
        return kTRUE;

    Int_t dx = event->fX-x;
    Int_t dy = event->fY-y;

    if (dx<kZOOM/4) dx=kZOOM/4;
    if (dy<kZOOM/4) dy=kZOOM/4;
    if (dx>766-kZOOM/4) dx=766-kZOOM/4;
    if (dy>574-kZOOM/4) dy=574-kZOOM/4;

    fDx = dx;
    fDy = dy;

    //UpdatePosZoom();
    return kTRUE;
}

*/

/// Overwritten DimCommand::commandHandler
void MStarguider::commandHandler()
{
    DimCommand *cmd = getCommand();
    if (!cmd)
        return;

    if (cmd==&fDimTPoint)
    {
        fTPointMode = 2;
        fScreenshotName = "";
        cout << "DimCommand[TPOINT]: " << cmd->itsSize << " " << string((char*)cmd->itsData, cmd->itsSize) << endl;
        return;
    }

    if (cmd==&fDimScreenshot && fTPointMode==0)
    {
        if (cmd->itsSize<2)
            return;

        fTPointMode = 2;
        fScreenshotColor = ((uint8_t*)cmd->itsData)[0];
        fScreenshotName = string((char*)cmd->itsData+1, cmd->itsSize-1);
        cout << "DimCommand[SCREENSHOT]: " << fScreenshotName << endl;
        return;
    }

    cout << "DimCommand[UNKNOWN]: " << cmd->itsSize << " " << string((char*)cmd->itsData, cmd->itsSize) << endl;
}
