#ifndef MGIMAGE_H
#define MGIMAGE_H

//
// This File contains the declaration of the MGImage-class
//
//   Author: Thomas Bretz
//   Version: V1.0 (1-8-2000)

#ifndef ROOT_TGFrame
#include <TGFrame.h>
#endif
#ifndef ROOT_TGX11
#include <TGX11.h>
#endif

class TMutex;

typedef unsigned char byte;

class MGImage : public TGFrame
{
    XImage *fImage;

    GContext_t fDefGC;
    //Pixmap_t   fPixmap;

    UInt_t fWidth;
    UInt_t fHeight;

    TMutex *fMuxPixmap; //! test

    enum
    {
        kNeedRedraw = BIT(17),
        kSyncMode   = BIT(18)
    };

    void DrawImg16(unsigned short *d, char *s, char *e);
    void DrawImg24(char *d, char *s, char *e);
    void DrawColImg16(unsigned short *d, char *s1, char *s2, char *e);
    void DrawColImg24(char *d, char *s1, char *s2, char *e);

public:
    MGImage(const TGWindow* p, UInt_t w, UInt_t h, UInt_t options = kSunkenFrame, ULong_t back = fgDefaultFrameBackground);
    ~MGImage();

    void DoRedraw();

    void DrawImg(const byte *buffer);
    void DrawColImg(const byte *gbuf, const byte *cbuf);

    void EnableSyncMode()  { SetBit(kSyncMode); }
    void DisableSyncMode() { ResetBit(kSyncMode); }

    static UChar_t Color(int col);
    static void    DrawCircle(UChar_t *buf, int w, int h, Float_t x, Float_t y, Float_t r, UChar_t col);
    static void    DrawHexagon(UChar_t *buf, int w, int h, Float_t x, Float_t y, Float_t r, UChar_t col, Int_t style=1);
    static void    DrawLine(UChar_t *buf, int w, int h, Float_t x1, Float_t y1, Float_t x2, Float_t y2, UChar_t col, Int_t style=1);
    static void    DrawBox(UChar_t *buf, int w, int h, Float_t x1, Float_t y1, Float_t x2, Float_t y2, UChar_t col, Int_t style=1);
    static void    DrawDot(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, UChar_t col);
    static void    DrawMultiply(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, Float_t size, UChar_t col);
    static void    DrawCross(UChar_t *buf, int w, int h, Float_t cx, Float_t cy, Float_t size, UChar_t col);
};

#endif // MGIMAGE_H
