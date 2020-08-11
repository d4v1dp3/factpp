//
// This File contains the definition of the MGImage-class
//
//   Author: Thomas Bretz
//   Version: V1.0 (1-8-2000)
//
// x11/src/GX11Gui.cxx
//

//////////////////////////////////////////////////////////////////////////////
//
// MGImage
//
// If sync-mode is enabled the Redraw function is secured by a mutex (ignore
// error messages about it comming from root) This has the advantage that
// if you use a timer for screen update reading and writing the image is
// synchronized. In this way you don't get flickering half images.
//
//////////////////////////////////////////////////////////////////////////////
#include "MGImage.h"

#include <iostream>

#include <TGX11.h>
#include <TMutex.h>

#include <TMath.h>

//#include "MLog.h"
//#include "MLogManip.h"

using namespace std;

MGImage::MGImage(const TGWindow* p, UInt_t w, UInt_t h, UInt_t options, ULong_t back)
    : TGFrame(p, w, h, options, back), fWidth(w), fHeight(h)
{
    // p = pointer to MainFrame (not owner)
    // w = width of frame
    // h = width of frame

    //
    // Creat drawing semaphore
    //
    fMuxPixmap = new TMutex;

    Resize(GetWidth(), GetHeight());

    //
    // create empty pixmap
    //
    fDefGC  = gVirtualX->CreateGC(fId, 0);
    fImage  = (XImage*)gVirtualX->CreateImage(fWidth, fHeight);

    cout << "Detected Color Depth: " << gVirtualX->GetDepth() << endl;
}

MGImage::~MGImage()
{
//    if (fMuxPixmap->Lock()==13)
//        cout << "MGImage::~MGImage - mutex is already locked by this thread" << endl;

    cout << "Deleting MGImage..." << endl;

    gVirtualX->DeleteGC(fDefGC);
    gVirtualX->DeleteImage((Drawable_t)fImage);

    //cout << fMuxPixmap->UnLock() << endl;

    delete fMuxPixmap;

    cout << "MGImage destroyed." << endl;
}

void MGImage::DoRedraw()
{
    if (TestBit(kSyncMode))
        while (fMuxPixmap->Lock()==13)
            usleep(1);

    //    gVirtualX->DrawLine(fId, fDefGC, 0, 0, fWidth+2, 0);
    //    gVirtualX->DrawLine(fId, fDefGC, 0, 0, 0, fHeight+2);
    //    gVirtualX->DrawLine(fId, fDefGC, fWidth+2, 0,  fWidth+2, fHeight+2);
    //    gVirtualX->DrawLine(fId, fDefGC, 0, fHeight+2, fWidth+2, fHeight+2);

    //    if (TestBit(kNeedRedraw))
    {
        gVirtualX->PutImage(fId, fDefGC, (Drawable_t)fImage, 0, 0, 0, 0,
                            fWidth, fHeight);
        ResetBit(kNeedRedraw);
    }

    if (TestBit(kSyncMode))
        if (fMuxPixmap->UnLock()==13)
            cout << "MGImage::DoRedraw - tried to unlock mutex locked by other thread." << endl;
}

void MGImage::DrawImg16(unsigned short *d, char *s, char *e)
{
    // d=destination, s=source, e=end
    // rrrrrggg gggbbbbb
    //
    while (s<e)
    {
        //         11111100    11111000      11111000
        // *d++ = (*s&0xfc) | (*s&0xf8)<<5 | (*s&0xf8)<<11;

        //      11111000       11111100       11111000
        *d++ = (*s&0xf8)<<8 | (*s&0xfc)<<3 | (*s>>3);
        s++;
    }
}

void MGImage::DrawImg24(char *d, char *s, char *e)
{
    // d=destination, s=source, e=end
    // rrrrrrrr gggggggg bbbbbbbb aaaaaaaa
    //
    while (s<e)
    {
        *d++ = *s;
        *d++ = *s;
        *d++ = *s++;
        d++;
    }
}

void MGImage::DrawImg(const byte *buffer)
{
    if (TestBit(kSyncMode))
        while (fMuxPixmap->Lock()==13)
            usleep(1);
    else
    {
        const Int_t rc = fMuxPixmap->Lock();
        if (rc==13)
            cout << "MGImage::DrawImg - mutex is already locked by this thread" << endl;
        if (rc)
            return;
    }

    switch (gVirtualX->GetDepth())
    {
    case 8:
        memcpy(fImage->data, buffer, fWidth*fHeight);
        break;
    case 16:
        DrawImg16((unsigned short*)fImage->data, (char*)buffer, (char*)(buffer+fWidth*fHeight));
        break;
    case 24:
        DrawImg24(fImage->data, (char*)buffer, (char*)(buffer+fWidth*fHeight));
        break;
    default:
        cout << "Sorry, " << gVirtualX->GetDepth() << "bit color depth not yet implemented." << endl;
    }

    SetBit(kNeedRedraw);

    if (fMuxPixmap->UnLock()==13)
        cout << "MGImage::DrawImage - tried to unlock mutex locked by other thread." << endl;
}

void MGImage::DrawColImg16(unsigned short *d, char *s1, char *s2, char *e)
{
    // d=destination, s1=source1, s2=source2, e=end
    // d:  rrrrrggg gggbbbbb
    // s2:          00rrggbb
    //
    while (s1<e)
    {
        if (*s2)
        {    
            //      00000011   00001100        00110000
            //*d++ = (*s2&0x3) | (*s2&0xb)<<3 | (*s2&0x30)<<7;
            *d++ = (*s2&0x3)<<3 | (*s2&0xb)<<6 | (*s2&0x30)<<10;
        }
        else
        {
            //      11111100     11111000        11111100
            *d++ = (*s1&0xfc) | (*s1&0xf8)<<5 | (*s1&0xfc)<<11;
        }
        s1++;
        s2++;
    }
}

void MGImage::DrawColImg24(char *d, char *s1, char *s2, char *e)
{
    // d=destination, s1=source1, s2=source2, e=end
    while (s1<e)
    {
        if (*s2)
        {
            *d++ = ((*s2>>4)&0x3)*85;
            *d++ = ((*s2>>2)&0x3)*85;
            *d++ = ((*s2++ )&0x3)*85;
            d++;
            s1++;
        }
        else
        {
            *d++ = *s1;
            *d++ = *s1;
            *d++ = *s1++;
            d++;
            s2++;
        }
    }
}

void MGImage::DrawColImg(const byte *gbuf, const byte *cbuf)
{
    if (TestBit(kSyncMode))
        while (fMuxPixmap->Lock()==13)
            usleep(1);
    else
    {
        const Int_t rc = fMuxPixmap->Lock();
        if (rc==13)
            cout << "MGImage::DrawColImg - mutex is already locked by this thread" << endl;
        if (rc)
            return;
    }

    // FROM libAfterImage:
    // -------------------
    //#define ALPHA_TRANSPARENT      	0x00
    //#define ALPHA_SEMI_TRANSPARENT 	0x7F
    //#define ALPHA_SOLID            	0xFF
    // * Lowermost 8 bits - Blue channel
    // * bits  8 to 15    - Green channel
    // * bits 16 to 23    - Red channel
    // * bits 24 to 31    - Alpha channel
    //#define ARGB32_White    		0xFFFFFFFF
    //#define ARGB32_Black    		0xFF000000

    // FIXME: This loop depends on the screen color depth
    switch (gVirtualX->GetDepth())
    {
    case 16:
        DrawColImg16((unsigned short*)fImage->data, (char*)gbuf, (char*)cbuf, (char*)(gbuf+fWidth*fHeight));
        break;
    case 24:
        DrawColImg24(fImage->data, (char*)gbuf, (char*)cbuf, (char*)(gbuf+fWidth*fHeight));
        break;
    default:
        cout << "Sorry, " << gVirtualX->GetDepth() << "bit color depth not yet implemented." << endl;
    }

    SetBit(kNeedRedraw);

    if (fMuxPixmap->UnLock()==13)
        cout << "MGImage::DrawColImage - tried to unlock mutex locked by other thread." << endl;
}

// --------------------------------------------------------------------------
//
// Convert root colors to arbitrary bitmap coordinates
//
UChar_t MGImage::Color(int col)
{
    switch (col)
    {
    case kBlack:  return 0;
    case kWhite:  return 0xff;
    case kYellow: return 0x0f;
    case kRed:    return 2;
    case kGreen:  return 2<<2;
    case kBlue:   return 2<<4;
    default:
        return 0;
    }
}

// --------------------------------------------------------------------------
//
// Draw a line into the buffer (size w*h) from (x1, y1) to (x2, y2) with
// the color col and the line style style (default: solid)
//
void MGImage::DrawLine(UChar_t *buf, int w, int h, Float_t x1, Float_t y1, Float_t x2, Float_t y2, UChar_t col, Int_t style)
{
    const Int_t    step = style==kSolid?1:3;
    const Double_t len  = TMath::Hypot(x2-x1, y2-y1);
    const Double_t dx   = (x2-x1)/len*step;
    const Double_t dy   = (y2-y1)/len*step;

    Double_t x = x1;
    Double_t y = y1;

    for (int i=0; i<len; i+=step)
    {
        x+= dx;
        y+= dy;

        const Int_t iy = TMath::Nint(y);
        if (iy<0 || iy>=h)
            continue;

        const Int_t ix = TMath::Nint(x);
        if (ix<0 || ix>=w)
            continue;

        buf[ix+iy*w] = col;
    }
}

// --------------------------------------------------------------------------
//
// Draw a box into the buffer (size w*h) from (x1, y1) to (x2, y2) with
// the color col and the line style style (default: solid)
//
void MGImage::DrawBox(UChar_t *buf, int w, int h, Float_t x1, Float_t y1, Float_t x2, Float_t y2, UChar_t col, Int_t style)
{
    DrawLine(buf, w, h, x1, y1, x2, y1, col, style);
    DrawLine(buf, w, h, x1, y2, x2, y1, col, style);
    DrawLine(buf, w, h, x1, y1, x1, y2, col, style);
    DrawLine(buf, w, h, x2, y1, x2, y2, col, style);
}

// --------------------------------------------------------------------------
//
// Draw a hexagon into the buffer (size w*h) around (x, y) with radius r and
// the color col.
//
void MGImage::DrawHexagon(UChar_t *buf, int w, int h, Float_t px, Float_t py, Float_t d, UChar_t col, Int_t style)
{
    const Int_t np = 6;

    const Double_t dy[np+1] = { .5   , 0.    , -.5   , -.5   , 0.    ,  .5   , .5    };
    const Double_t dx[np+1] = { .2886,  .5772,  .2886, -.2886, -.5772, -.2886, .2886 };

    //
    //  calculate the positions of the pixel corners
    //
    Double_t x[np+1], y[np+1];
    for (Int_t i=0; i<np+1; i++)
    {
        x[i] = px + dx[i]*d;
        y[i] = py + dy[i]*d;
    }

    for (int i=0; i<6; i++)
        DrawLine(buf, w, h, x[i], y[i], x[i+1], y[i+1], col, style);
}

// --------------------------------------------------------------------------
//
// Draw a circle into the buffer (size w*h) around (x, y) with radius r and
// the color col.
//
void MGImage::DrawCircle(UChar_t *buf, int w, int h, Float_t x, Float_t y, Float_t r, UChar_t col)
{
    const Int_t n = TMath::Nint(sqrt(2.)*r*TMath::Pi()/2);
    for (int i=0; i<n-1; i++)
    {
        const Double_t angle = TMath::TwoPi()*i/n;

        const Double_t dx = r*cos(angle);
        const Double_t dy = r*sin(angle);

        const Int_t x1 = TMath::Nint(x+dx);
        const Int_t x2 = TMath::Nint(x-dx);

        const Int_t y1 = TMath::Nint(y+dy);
        if (y1>=0 && y1<h)
        {
            if (x1>=0 && x1<w)
                buf[x1+y1*w] = col;

            if (x2>=0 && x2<w)
                buf[x2+y1*w] = col;
        }

        const Int_t y2 = TMath::Nint(y-dy);
        if (y2>=0 && y2<h)
        {
            if (x1>=0 && x1<w)
                buf[x1+y2*w] = col;

            if (x2>=0 && x2<w)
                buf[x2+y2*w] = col;
        }
    }
}

// --------------------------------------------------------------------------
//
// Draw a dot into the buffer (size w*h) at (x, y) with color col.
//
void MGImage::DrawDot(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, UChar_t col)
{
    const Int_t x1 = TMath::Nint(cx);
    const Int_t y1 = TMath::Nint(cy);

    if (x1>=0 && y1>=0 && x1<w && y1<h)
        buf[x1+y1*w] = col;
}

// --------------------------------------------------------------------------
//
// Draw a line into the buffer. The TObject must be a TLine.
// Currently only solid and non sloid line are supported.
//
/*
void MGImage::DrawLine(TObject *o, UChar_t *buf, int w, int h, Double_t scale)
{
    TLine *l = dynamic_cast<TLine*>(o);
    if (!l)
        return;

    const Double_t x1 = 0.5*w-(l->GetX1()/scale);
    const Double_t x2 = 0.5*w-(l->GetX2()/scale);
    const Double_t y1 = 0.5*h-(l->GetY1()/scale);
    const Double_t y2 = 0.5*h-(l->GetY2()/scale);

    const Int_t col = Color(l->GetLineColor());
    DrawLine(buf, w, h, x1, y1, x2, y2, col, l->GetLineStyle());
}
*/
void MGImage::DrawMultiply(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, Float_t size, UChar_t col)
{
    DrawLine(buf, w, h, cx-size, cy-size, cx+size, cy+size, col);
    DrawLine(buf, w, h, cx+size, cy-size, cx-size, cy+size, col);
}

void MGImage::DrawCross(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, Float_t size, UChar_t col)
{
    DrawLine(buf, w, h, cx-size, cy, cx+size, cy, col);
    DrawLine(buf, w, h, cx, cy-size, cx, cy+size, col);
}

// --------------------------------------------------------------------------
//
// Draw marker into the buffer. The TObject must be a TMarker.
// Currently kCircle, kMultiply and KDot are supported.
/*
void MGImage::DrawMarker(TObject *o, UChar_t *buf, int w, int h, Double_t scale)
{
    TMarker *m = dynamic_cast<TMarker*>(o);
    if (!m)
        return;

    Double_t x = 0.5*w-(m->GetX()/scale);
    Double_t y = 0.5*h-(m->GetY()/scale);

    Int_t col = Color(m->GetMarkerColor());

    switch (m->GetMarkerStyle())
    {
    case kCircle:
        DrawCircle(buf, w, h, x, y, m->GetMarkerSize()*2+1, col);
        break;
    case kDot:
        DrawDot(buf, w, h, x, y, col);
        break;
    case kMultiply:
        DrawMultiply(buf, w, h, x, y, m->GetMarkerSize()*2+1, col);
        break;
    case kCross:
        DrawCross(buf, w, h, x, y, m->GetMarkerSize()*2+1, col);
        break;
    }
}
*/