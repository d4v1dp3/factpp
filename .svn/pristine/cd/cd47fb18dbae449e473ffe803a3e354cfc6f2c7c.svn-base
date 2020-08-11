#ifndef FACT_DimErrorRedirecter
#define FACT_DimErrorRedirecter

#include <dis.hxx>

class MessageImp;

class DimErrorRedirecter : public DimErrorHandler, public DimExitHandler
{
private:
    static int fDimErrorRedireterCnt;

    MessageImp &fMsg;

    void errorHandler(int severity, int code, char *msg);
    void exitHandler(int code) { exit(code); }

public:
    DimErrorRedirecter(MessageImp &imp);
    ~DimErrorRedirecter();
};

#endif
