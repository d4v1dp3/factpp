#ifndef CAOS_MCaos
#define CAOS_MCaos

#ifndef CAOS_Ring
#include "Ring.h"
#endif

class MCaos
{
private:
    std::vector<Led> fPositions;
    std::vector<Led> fLeds;

    int16_t  fMinNumberLeds; // minimum number of detected leds required
    double   fMinRadius;     // minimum radius for cut in ring radius
    double   fMaxRadius;     // maximum radius for cut in ring radius
    uint16_t fSizeBox;       // Size of the search box (side length in units of pixels)
    double   fCut;           // Cleaning level (sigma above noise)

    int32_t fNumDetectedRings;

    Ring fCenter;
    std::vector<Ring> fRings;

    void CalcCenters(const std::vector<Led> &leds, float min, float max);
    int32_t CalcRings(std::vector<Led> &leds, float min=-1, float max=-1);
    const Ring &GetCenter() const { return fCenter; }

public:
    MCaos() : fMinRadius(236.7), fMaxRadius(238.6), fSizeBox(19), fCut(3.5)
    {
    }

    ~MCaos()
    {
    }

    void AddPosition(float x, float y, float phi)
    {
        fPositions.push_back(Led(x, y, phi));
    }

    void ReadResources(const char *name="leds.txt");

    void SetMinNumberLeds(int16_t n)
    {
	fMinNumberLeds = n;
    }

    void SetMinRadius(double min) { fMinRadius=min; }
    void SetMaxRadius(double max) { fMaxRadius=max; }

    int32_t GetNumDetectedLEDs() const  { return fLeds.size(); }
    int32_t GetNumDetectedRings() const { return fNumDetectedRings; }

    Ring Run(uint8_t *img);
};

#endif
