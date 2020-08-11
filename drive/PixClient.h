#ifndef COSY_PixClient
#define COSY_PixClient

#ifdef __CINT__
struct timeval;
#else
#include <unistd.h>
#include <sys/time.h>
#endif


typedef unsigned char byte;

class PixClient
{
public:
    virtual ~PixClient() { }
    virtual void ProcessFrame(const unsigned long n,
                              byte *img, struct timeval *tm);
};

#endif
