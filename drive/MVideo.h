#ifndef MARS_MVideo
#define MARS_MVideo

#ifndef MAGIC_MAGIC
#include "MAGIC.h"
#endif

#ifndef __CINT__
#ifndef __LINUX_VIDEODEV_H
#include "videodev.h"  // video4linux
#endif
#ifndef __LINUX_VIDEODEV2_H
#include <linux/videodev2.h> // video4linux2
#endif
#endif

#include <vector>

struct v4l2_queryctrl;
struct v4l2_input;
struct v4l2_standard;
struct v4l2_buffer;

class TEnv;

class MVideoCtrl : public TObject
{
    friend class MVideo;
private:
    UInt_t  fId;
    //enum v4l2_ctrl_type  type;
    TString fName;
    Int_t   fMinimum;
    Int_t   fMaximum;
    Int_t   fStep;
    Int_t   fDefault;
    UInt_t  fFlags;

    UInt_t  fValue;

public:
    MVideoCtrl(const v4l2_queryctrl &ctrl);
    const char *GetName() const { return fName; }
    const char *GetTitle() const { return Form("Range=[%d;%d] Step=%d Def=%d", fMinimum, fMaximum, fStep, fDefault); }

    //ClassDef(MVideoCtrl, 0) // Helper class to enumare device controls
};

class MVideo
{
private:
    TString fPath; // Device path

    int fFileDesc; // File descriptor

    unsigned char *fMapBuffer;

protected:
    struct video_capability fCaps;      // Device capabilities
    struct video_channel    fChannel;   // Channel information
    //struct video_mbuf       fBuffer;    // Buffer information
    struct video_picture    fProp;      // Picture properties
    struct video_tuner      fAbil;      // Tuner abilities

    ULong64_t fVideoStandard;

    std::vector<v4l2_input>      fInputs;
    std::vector<v4l2_standard>   fStandards;
    std::vector<std::pair<v4l2_buffer, void*> > fBuffers;

    TList fControls;

private:
    int Ioctl(int req, void *opt, bool allowirq=true, bool force=false) const;

    void Reset();

    Bool_t EnumerateControls(UInt_t id);
    Bool_t EnumerateControls();
    Bool_t GetCapabilities();
    Bool_t GetProperties();
    Bool_t GetTunerAbilities();
    Bool_t GetVideoStandard();
    Bool_t Init(Int_t channel);

    template<class S>
        Bool_t Enumerate(std::vector<S> &s, int request);

    void PrintInputs() const;
    void PrintStandards() const;

    // Conversion functions
    TString GetDevType(int type) const;
    TString GetChannelFlags(Int_t flags) const;
    TString GetChannelType(Int_t type) const;
    TString GetTunerFlags(Int_t type) const;
    TString GetTunerMode(Int_t type) const;
    TString GetPalette(Int_t pal) const;

public:
    MVideo(const char *path="/dev/video0");
    virtual ~MVideo() { Close(); }

    // Getter
    Bool_t IsOpen() const { return fFileDesc>0 && fBuffers.size()>0; }
    Bool_t CanCapture() const;
    Bool_t HasTuner() const;
    Int_t  GetNumBuffers() const;

    Int_t  GetWidth() const;
    Int_t  GetHeight() const;

    // Control
    Bool_t Open(Int_t channel=0);
    Int_t  Close();

    Int_t SetChannel(Int_t chan);
    Bool_t ReadControl(MVideoCtrl &vctrl) const;
    Bool_t WriteControl(MVideoCtrl &vctrl, Int_t val) const;
    Bool_t SetControls(TEnv &env) const;
    Bool_t ResetControl(MVideoCtrl &vctrl) const;
    Bool_t ResetControls() const;

    // Image capture
    Bool_t CaptureStart(unsigned int frame) const;
    Int_t  CaptureWait(unsigned int frame, unsigned char **ptr=0) const;
    Bool_t Start();

    // Support
    void Print() const;

    // hardware features
    //void SetPicPar(int  bright, int  hue, int  contrast);
    //void GetPicPar(int *bright, int *hue, int *contrast);

    //ClassDef(MVideo, 0) // Interface to Video4Linux at a simple level

};

#endif
