#ifndef WRITER_H
#define WRITER_H

#ifdef __CINT__
struct timeval;
#else
#include <TROOT.h>
#include <sys/time.h>
#endif

class TVector2;

typedef unsigned char byte;

class Writer;

class Writer 
{
public:
    virtual ~Writer() { }

    static void Ppm(const char *fname, const byte *img, struct timeval *date, const TVector2 xy);
    static void Png(const char *fname, const byte *buf, struct timeval *date, const TVector2 xy);

    ClassDef(Writer, 0)
};

#endif
