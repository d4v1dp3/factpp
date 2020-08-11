#ifndef FACT_DataCalib
#define FACT_DataCalib

#include "DataWriteFits2.h"
#include "externals/DrsCalib.h"

class DimDescribedService;

using namespace std;

class DataCalib : public DataWriteFits2, public DrsCalibrate
{
    static DrsCalibration fData;

    static std::vector<float> fStats;     /// Storage for mean and rms values

    /// State of the DRS calibration: Positiove numbers mean that
    /// we are in a run, negative mean that it is closed
    static bool fProcessing;

    DimDescribedService &fDim;     // DimService through which statistics updates are transmitted
    DimDescribedService &fDimRuns; // DimService through which statistics updates are transmitted

//    uint16_t fDAC[8];

//    void WriteFitsImp(const std::string &filename, const std::vector<float> &vec) const;

    int GetDrsStep() const { return fData.fStep; }

public:
    DataCalib(const std::string &path, uint64_t night, uint32_t id, const DrsCalibration &calib, DimDescribedService &dim, DimDescribedService &runs, MessageImp &imp) : DataWriteFits2(path, night, id, calib, imp), fDim(dim), fDimRuns(runs)
    {
    }

    static void Restart();
    static bool ResetTrgOff(DimDescribedService &dim, DimDescribedService &runs);
    static void Update(DimDescribedService &dim, DimDescribedService &runs);

    bool Open(const RUN_HEAD &h, const FAD::RunDescription &d);
    bool WriteEvt(const EVT_CTRL2 &);
    bool Close(const EVT_CTRL2 &);

    //static void Apply(int16_t *val, const int16_t *start, uint32_t roi);
    static void Apply(float *vec, int16_t *val, const int16_t *start, uint32_t roi)
    {
        fData.Apply(vec, val, start, roi);
    }

    static bool ReadFits(const string &fname, MessageImp &msg);

    static bool IsValid() { return fData.IsValid(); }
    static int  GetStep() { return fData.fStep; }

    static const DrsCalibration &GetCalibration() { return fData; }
};

#endif
