#ifndef COSY_Camera
#define COSY_Camera

#ifndef MARS_MThread
#include "MThread.h"
#endif
/*
#ifndef COSY_PixGetter
#include "PixGetter.h"
#endif
*/
class MVideo;
class PixClient;

class Camera : /*public PixGetter,*/ public MThread
{
private:
    //
    // Geometry
    //
    static const int cols  = 768;
    static const int rows  = 576;
    static const int depth = 4;

    unsigned char fImg[cols*rows];
    struct timeval fTime;

    PixClient &fClient;

    MVideo *fVideo;

    UInt_t fNumFrame;
    UInt_t fNumSkipped;

    UInt_t fChannel;

    Int_t Thread();
    void  ProcessFrame(unsigned char *img);

public:
    Camera(PixClient &client, Int_t ch=0);
    virtual ~Camera();

    void SetChannel(int);

    void ExitLoop() { CancelThread(); }

    //ClassDef(Camera, 0)
};

#endif
