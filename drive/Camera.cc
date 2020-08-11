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
!   Author(s): Thomas Bretz 1/2008 <mailto:tbretz@astro.uni-wuerzburg.de>
!
!   Copyright: MAGIC Software Development, 2000-2008
!
!
\* ======================================================================== */
#include "Camera.h"

#include <iostream>

#include "MVideo.h"
#include "PixClient.h"

using namespace std;

Camera::Camera(PixClient &client, Int_t nch) : MThread("Camera"), fClient(client), fVideo(0), fNumFrame(0), fChannel(nch)
{
    fVideo = new MVideo;
    fVideo->Open(nch);

    RunThread();
}

Camera::~Camera()
{
    // Shut down the thread
    CancelThread();

    // Now delete (close) the device
    delete fVideo;
}

void Camera::ProcessFrame(unsigned char *img)
{
    gettimeofday(&fTime, NULL);

#if 1
    for (int y=0; y<576; y++)
        for (int x=0; x<768; x++)
        {
            const Int_t p = (x+y*768)*4;
            fImg[x+y*768] = ((UInt_t)img[p+1]+(UInt_t)img[p+2]+(UInt_t)img[p+3])/3;
        }
#endif

#if 0
    unsigned char *dest = fImg;
    for (const unsigned char *ptr=img; ptr<img+768*576*4; ptr+=4)
        *dest++ = (UShort_t(ptr[1])+UShort_t(ptr[2])+UShort_t(ptr[3]))/3;
#endif

    fClient.ProcessFrame(fNumFrame-1, (byte*)fImg, &fTime);
}

Int_t Camera::Thread()
{
    fNumSkipped = 0;
    fNumFrame   = 0;

    if (!fVideo->IsOpen())
    {
        cout << "Camera::Thread: ERROR - Device not open." << endl;
        return kFALSE;
    }

    cout << "Start Camera::Thread at frame " << fNumFrame%fVideo->GetNumBuffers() << endl;

    for (int f=0; f<fVideo->GetNumBuffers(); f++)
    {
        if (!fVideo->CaptureStart(f))
            return kFALSE;
    }

    if (!fVideo->Start())
        return kFALSE;

    Int_t timeouts = 0;
    while (1)
    {

        /*
        // Switch channel if necessary
        switch (fVideo->SetChannel(fChannel))
        {
        case kFALSE:        // Error swucthing channel
            return kFALSE;
        case kSKIP:         // No channel switching necessary
            break;
        case kTRUE:         // Channel switched (skip filled buffers)
            for (int f=0; f<fVideo->GetNumBuffers(); f++)
                if (!fVideo->CaptureWait(fNumFrame+f))
                    return kFALSE;
            fNumFrame=0;
            for (int f=0; f<fVideo->GetNumBuffers(); f++)
                if (!fVideo->CaptureStart(f))
                    return kFALSE;
            break;
        }*/

        //cout << "*** Wait " << fNumFrame << endl;

        // Check and wait until capture into the next buffer is finshed
        unsigned char *img = 0;
        switch (fVideo->CaptureWait(fNumFrame, &img))
        {
        case kTRUE: // Process frame
            // If cacellation is requested cancel here
            TThread::CancelPoint();

            ProcessFrame(img);

            // If cacellation is requested cancel here
            TThread::CancelPoint();

            // Start to capture into the buffer which has just been processed
            if (!fVideo->CaptureStart(fNumFrame-1))
                break;

            fNumFrame++;
            timeouts = 0;
            continue;

        case kFALSE: // Waiting failed
            break;

        case -1:  // Skip frame
            usleep(10000); // Wait half a frame
            continue;

            fNumFrame--;
            fNumSkipped++;
            if (timeouts++<5)
                continue;

            cout << "ERROR - At least five captured images timed out." << endl;
            break;
        }

        break;
    }

    // Is this necessary?!?
    //for (int i=0; i<frames-1; i++)
    //    video.CaptureWait((f+i+1)%frames);

    cout << fNumFrame-1 << " frames processed." << endl;
    cout << fNumSkipped << " frames skipped." << endl;

    return kTRUE;
}

//void Camera::Loop(unsigned long nof)
//{
//}

void Camera::SetChannel(int chan)
{
    fChannel = chan;
//    CancelThread();
//    fVideo->SetChannel(chan);
//    RunThread();
}

