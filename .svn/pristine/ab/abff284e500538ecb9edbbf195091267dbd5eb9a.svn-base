#ifndef CAOS_FilterLed
#define CAOS_FilterLed

#include <stdint.h>
#include <vector>

class Led;
class Ring;

class FilterLed
{
    uint8_t *fImg;
    int fW;
    int fH;
    int fBoxX;
    int fBoxY;
    float fCut;

    float FindCluster(int &cnt, float *sum, uint32_t x, uint32_t y,
                        uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) const;

    void GetMinMax(const int offset, uint8_t *min, uint8_t *max) const;
    int  GetMeanPosition(const int x, const int y, const int boxx, const int boxy) const;
    int  GetMeanPosition(const int x, const int y, const int boxx, const int boxy,
			 float &mx, float &my, unsigned int &sum) const;

    int  GetMeanPositionBox(const int x, const int y,
                            const int boxx, const int boxy) const;
    int  GetMeanPositionBox(const int x, const int y,
                            const int boxx, const int boxy, float &mx, float &my,
                            unsigned int &sum) const;

    void DrawBox(const int x1, const int y1,
                 const int x2, const int y2,
                 const int col) const;

public:
    FilterLed(uint8_t *img, int w, int h, double cut=2.5) : fImg(img),
        fW(w), fH(h), fBoxX(w), fBoxY(h), fCut(cut)
    {
    }

    FilterLed(uint8_t *img, int w, int h, int boxx, int boxy, double cut=2.5) : fImg(img),
        fW(w), fH(h), fBoxX(boxx), fBoxY(boxy), fCut(cut)
    {
    }
    virtual ~FilterLed() { }

    void SetBox(int box)   { fBoxX = fBoxY = box; }
    void SetBox(int boxx, int boxy)   { fBoxX = boxx; fBoxY = boxy; }
    void SetCut(float cut) { fCut = cut; }
    void FindStar(std::vector<Led> &leds, int xc, int yc, bool circle=false) const;

    void Execute(std::vector<Led> &leds, int xc, int yc, double &bright) const;
    void Execute(std::vector<Led> &leds, int xc, int yc) const;
    void Execute(std::vector<Led> &leds) const { Execute(leds, fW/2, fH/2); }

    void MarkPoint(const Led &led) const;
    void MarkPoint(float x, float y, float mag) const;
    void Stretch() const;

    void DrawCircle(float cx, float cy, float r, uint8_t col=0x40) const;
    void DrawCircle(float r, uint8_t col=0x40) const { DrawCircle(fW/2, fH/2, r, col); }
    void DrawCircle(const Ring &c, uint8_t col=0x40) const;
    void DrawCircle(const Ring &c, double r, uint8_t col) const;
    void DrawHexagon(float cx, float cy, float r, uint8_t col=0x40) const;
    void DrawHexagon(const Ring &c, double r, uint8_t col) const;
};

#endif
