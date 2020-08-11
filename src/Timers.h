class Timers
{
private:
    std::ostream &fOut;

    Time fT[4];

    int fSum[2];
    int fCnt;

    int fNwait;

public:
    Timers(std::ostream &out=std::cout) : fOut(out) { fSum[0] = fSum[1] = fCnt = fNwait = 0;/*fT[0] = Time(); fT[1] = Time(); fT[2] = Time(); fT[3] = Time();*/ }

    void SetT() { fT[1] = Time(); }

    void Proc(bool cond, int maxwait=5000, int interval=10000)
    {
        fT[2] = Time();

        fSum[0] += (fT[2]-fT[1]).total_microseconds();
        fSum[1] += (fT[1]-fT[3]).total_microseconds();
        fCnt++;

        if (cond)
        {
            usleep(std::max(maxwait-(int)(fT[2]-fT[3]).total_microseconds(), 1));
            fNwait++;
        }

        const int diff = (fT[2]-fT[0]).total_milliseconds();
        if (diff > interval)
        {
            fOut << "Rate(10s):  poll=" << fSum[0]/fCnt << "us   exec=" << fSum[1]/fCnt << "us   (" << fNwait <<"/" << fCnt << ")" << std::endl;

            fSum[0] = fSum[1] = fCnt = fNwait = 0;
            fT[0] = Time();
        }

        fT[3] = Time();
    }
};
