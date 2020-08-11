#ifndef FACT_MainImp
#define FACT_MainImp

class MainImp
{
public:
    virtual ~MainImp() {}
    virtual int Run(bool) = 0;
    virtual void Stop(int) = 0;
};

#endif
