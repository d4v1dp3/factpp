#include "FilterLed.h"

#include <memory.h>   // memset
#include <math.h>
#include <iostream> // cout

#include "Led.h"
#include "Ring.h"

#include "MGImage.h"

using namespace std;

class ClusterFinder
{
private:
    uint8_t *fImg;

    uint32_t fW;
    uint32_t fH;

    int32_t fX0;
    int32_t fX1;

    int32_t fY0;
    int32_t fY1;

    uint32_t fLimitingSize;

    uint32_t fCount;
    float fSumX;
    float fSumY;

    float FindCluster(int32_t x, int32_t y)
    {
        // if edge is touched stop finding cluster
        if (x<fX0 || x>=fX1 || y<fY0 || y>=fY1)
            return -1;

        if (fCount>fLimitingSize)
            return -2;

        // get the value
        float val = fImg[y*fW+x];

        // if its empty we have found the border of the cluster
        if (val==0)
            return 0;

        // mark the point as processed
        fImg[y*fW+x] = 0;

        fSumX += x*val; // sumx
        fSumY += y*val; // sumy
        fCount++;

        float rc[4];
        rc[0] = FindCluster(x+1, y  );
        rc[1] = FindCluster(x,   y+1);
        rc[2] = FindCluster(x-1, y  );
        rc[3] = FindCluster(x,   y-1);

        for (int i=0; i<4; i++)
        {
            if (rc[i]<0) // check if edge is touched
                return rc[i];

            val += rc[i];
        }

        return val;
    }

public:
    ClusterFinder(uint8_t *img, uint32_t w, uint32_t h) : fImg(0), fLimitingSize(999)
    {
        fW = w;
        fH = h;

        fX0 = 0;
        fY0 = 0;
        fX1 = fW;
        fY1 = fH;

        fImg = new uint8_t[fW*fH];

        memcpy(fImg, img, fW*fH);
    }

    ~ClusterFinder()
    {
        delete [] fImg;
    }
    Double_t GetSumX() const { return fSumX; }
    Double_t GetSumY() const { return fSumY; }

    uint32_t GetCount() const { return fCount; }

    void SetLimitingSize(uint32_t lim) { fLimitingSize=lim; }

    float FindClusterAt(int32_t x, int32_t y)
    {
        fCount = 0;
        fSumX  = 0;
        fSumY  = 0;

        return FindCluster(x, y);
    }

    void SetRange(int32_t x0=0, int32_t y0=0, int32_t x1=0, int32_t y1=0)
    {
        fX0 = x0;
        fY0 = y0;
        fX1 = x1==0?fW:x1;
        fY1 = y1==0?fH:y1;
    }

    void FindCluster(vector<Led> &leds, int32_t x0=0, int32_t y0=0, int32_t x1=0, int32_t y1=0)
    {
        fX0 = x0;
        fY0 = y0;
        fX1 = x1==0?fW:x1;
        fY1 = y1==0?fH:y1;

        for (int32_t x=fX0; x<fX1; x++)
            for (int32_t y=fY0; y<fY1; y++)
            {
                const uint8_t &b = fImg[y*fW+x];
                if (b==0)
                    continue;

                const float mag = FindClusterAt(x, y);
                if (fCount>999)
                {
                    cout << "ERROR - Spot with Size>999 detected..." << endl;
                    return;
                }

                if (mag>0 && fCount>4)
                    leds.push_back(Led(fSumX/mag, fSumY/mag, 0, mag));
            }
        //leds.Compress();
    }
};


void FilterLed::DrawBox(const int x1, const int y1,
                        const int x2, const int y2,
                        const int col) const
{
    MGImage::DrawBox(fImg, 768, 576, x1, y1, x2, y2, col);
}

void FilterLed::MarkPoint(float px, float py, float mag) const
{
    const int x = (int)(px+.5);
    const int y = (int)(py+.5);
    const int m = (int)(mag);

    DrawBox(x-8, y, x-5, y, m);
    DrawBox(x, y+5, x, y+8, m);
    DrawBox(x+5, y, x+8, y, m);
    DrawBox(x, y-8, x, y-5, m);
}

void FilterLed::MarkPoint(const Led &led) const
{
    /*
    int32_t M = (int)(log(led.GetMag())*20);

    cout << led.GetMag() << endl;

    if (M>0xff)
        M=0xff;
    if (M<0xc0)
        M=0xc0;
        */

    const int x = (int)(led.GetX()+.5);
    const int y = (int)(led.GetY()+.5);

    MarkPoint(x, y, 0xff);
}

void FilterLed::DrawCircle(float cx, float cy, float r, uint8_t col) const
{
    MGImage::DrawCircle(fImg, 768, 576, cx, cy, r, col);
}

void FilterLed::DrawHexagon(float cx, float cy, float r, uint8_t col) const
{
    MGImage::DrawHexagon(fImg, 768, 576, cx, cy, r, col);
}

void FilterLed::DrawCircle(const Ring &l, uint8_t col) const
{
    DrawCircle(l.GetX(), l.GetY(), l.GetR(), col);
}

void FilterLed::DrawCircle(const Ring &l, double r, uint8_t col) const
{
    DrawCircle(l.GetX(), l.GetY(), r, col);
}

void FilterLed::DrawHexagon(const Ring &l, double r, uint8_t col) const
{
    DrawHexagon(l.GetX(), l.GetY(), r, col);
}

void FilterLed::GetMinMax(const int offset, uint8_t *min, uint8_t *max) const
{
    *min = fImg[0];
    *max = fImg[0];

    uint8_t *s = (uint8_t*)fImg;
    const uint8_t *e0 = s+fW*fH;

    //
    // calculate mean value (speed optimized)
    //
    while (s<e0)
    {
        const uint8_t *e = s+fH-offset;
        s += offset;

        while (s<e)
        {
            if (*s>*max)
            {
                *max = *s;
                if (*max-*min==255)
                    return;
            }
            if (*s<*min)
            {
                *min = *s;
                if (*max-*min==255)
                    return;
            }
            s++;
        }
        s+=offset;
    }
}

int FilterLed::GetMeanPosition(const int x, const int y,
                               const int boxx, const int boxy,
                               float &mx, float &my, unsigned int &sum) const
{
    unsigned int sumx=0;
    unsigned int sumy=0;

    sum=0;
    for (int dx=x-boxx; dx<x+boxx+1; dx++)
        for (int dy=y-boxy; dy<y+boxy+1; dy++)
        {
            const uint8_t &m = fImg[dy*fW+dx];

            sumx += m*dx;
            sumy += m*dy;
            sum  += m;
        }

    mx = (float)sumx/sum;
    my = (float)sumy/sum;

    return (int)my*fW + (int)mx;
}

int FilterLed::GetMeanPosition(const int x, const int y, const int boxx, const int boxy) const
{
    float mx, my;
    unsigned int sum;
    return GetMeanPosition(x, y, boxx, boxy, mx, my, sum);
}

int FilterLed::GetMeanPositionBox(const int x, const int y,
                                  const int boxx, const int boxy,
                                  float &mx, float &my, unsigned int &sum) const
{
    //-------------------------------
    // Improved algorithm:
    // 1. Look for the largest five-pixel-cross signal inside the box
    int x0 = max(x-boxx+1,   0);
    int y0 = max(y-boxy+1,   0);

    int x1 = min(x+boxx+1-1, fW);
    int y1 = min(y+boxy+1-1, fH);

    int maxx=0;
    int maxy=0;

    unsigned int max =0;
    for (int dx=x0; dx<x1; dx++)
    {
        for (int dy=y0; dy<y1; dy++)
        {
            const unsigned int sumloc =
                fImg[(dy+0)*fW + (dx-1)] +
                fImg[(dy+0)*fW + (dx+1)] +
                fImg[(dy+1)*fW + dx] +
                fImg[(dy+0)*fW + dx] +
                fImg[(dy-1)*fW + dx];

            if(sumloc<=max)
                continue;

            maxx=dx;
            maxy=dy;
            max =sumloc;
	}
    }

    // 2. Calculate mean position inside a circle around
    // the highst cross-signal with radius of 6 pixels.
    ClusterFinder find(fImg, fW, fH);
    find.SetLimitingSize(9999);
    find.SetRange(x0, y0, x1, y1);

    const float mag = find.FindClusterAt(maxx, maxy);

    mx = find.GetSumX()/mag;
    my = find.GetSumY()/mag;

    sum = (int)(mag+0.5);

    return (int)my*fW + (int)mx;
}

int FilterLed::GetMeanPositionBox(const int x, const int y,
                                  const int boxx, const int boxy) const
{
    float mx, my;
    unsigned int sum;
    return GetMeanPositionBox(x, y, boxx, boxy, mx, my, sum);
}

void FilterLed::Execute(vector<Led> &leds, int xc, int yc) const
{
    double bright;
    Execute(leds, xc, yc, bright);
}

void FilterLed::Execute(vector<Led> &leds, int xc, int yc, double &bright) const
{
    const int x0 = max(xc-fBoxX, 0);
    const int y0 = max(yc-fBoxY, 0);
    const int x1 = min(xc+fBoxX, fW);
    const int y1 = min(yc+fBoxY, fH);

    const int wx = x1-x0;
    const int hy = y1-y0;

    double sum = 0;
    double sq  = 0;

    for (int x=x0; x<x1; x++)
        for (int y=y0; y<y1; y++)
        {
            uint8_t &b = fImg[y*fW+x];

            // Skip saturating pixels
            if (b>0xf0)
                continue;

            sum += b;
            sq  += b*b;
        }

    sum /= wx*hy;
    sq  /= wx*hy;

    bright=sum;

    
    // 254 because b<=max and not b<max
    const double sdev = sqrt(sq-sum*sum);
    const uint8_t   max  = sum+fCut*sdev>254 ? 254 : (uint8_t)(sum+fCut*sdev);

    //
    // clean image from noise
    // (FIXME: A lookup table could accelerate things...
    //
    for (int x=x0; x<x1; x++)
        for (int y=y0; y<y1; y++)
        {
            uint8_t &b = fImg[y*fW+x];
            if (b<=max)
                b = 0;
        }

    ClusterFinder find(fImg, fW, fH);
    find.FindCluster(leds, x0, y0, x1, y1);
}

void FilterLed::FindStar(vector<Led> &leds, int xc, int yc, bool box) const
{
    // fBox: radius of the inner (signal) box
    // Radius of the outer box is fBox*sqrt(2)

    //
    // Define inner box in which to search the signal
    //
    const int x0 = max(xc-fBoxX, 0);
    const int y0 = max(yc-fBoxY, 0);
    const int x1 = min(xc+fBoxX, fW);
    const int y1 = min(yc+fBoxY, fH);

    //
    // Define outer box (excluding inner box) having almost
    // the same number of pixels in which the background
    // is calculated
    //
    const double sqrt2 = sqrt(2.);

    const int xa = max(xc-(int)nearbyint(fBoxX*sqrt2), 0);
    const int ya = max(yc-(int)nearbyint(fBoxY*sqrt2), 0);
    const int xb = min(xc+(int)nearbyint(fBoxX*sqrt2), fW);
    const int yb = min(yc+(int)nearbyint(fBoxY*sqrt2), fH);

    //
    // Calculate average and sdev for a square
    // excluding the inner part were we expect
    // the signal to be.
    //
    double sum = 0;
    double sq  = 0;

    int n=0;
    for (int x=xa; x<xb; x++)
        for (int y=ya; y<yb; y++)
        {
            if (x>=x0 && x<x1 && y>=y0 && y<y1)
                continue;

            uint8_t &b = fImg[y*fW+x];

            sum += b;
            sq  += b*b;
            n++;
        }

    sum /= n;
    sq  /= n;

    // 254 because b<=max and not b<max
    const double sdev = sqrt(sq-sum*sum);
    const uint8_t   max  = sum+fCut*sdev>254 ? 254 : (uint8_t)(sum+fCut*sdev);

    //
    // clean image from noise
    // (FIXME: A lookup table could accelerate things...
    //
    n=0;
    for (int x=x0; x<x1; x++)
        for (int y=y0; y<y1; y++)
        {
            uint8_t &b = fImg[y*fW+x];
            if (b<=max)
                b = 0;
            else
                n++;
        }

    //
    // Mark the background region
    //
    for (int x=xa; x<xb; x+=2)
    {
        fImg[ya*fW+x]=0xf0;
        fImg[yb*fW+x]=0xf0;
    }
    for (int y=ya; y<yb; y+=2)
    {
        fImg[y*fW+xa]=0xf0;
        fImg[y*fW+xb]=0xf0;
    }

    //
    // Check if any pixel found...
    //
    if (n<5)
        return;

    //
    // Get the mean position of the star
    //
    float mx, my;
    unsigned int mag;
    int pos = box ? GetMeanPositionBox(xc, yc, fBoxX-1, fBoxY-1, mx, my, mag)
        : GetMeanPosition(xc, yc, fBoxX-1, fBoxY-1, mx, my, mag);

    if (pos<0 || pos>=fW*fH || fImg[pos]<sum+fCut*sdev)
        return;

    //    cout << "Mean=" << sum << "  SDev=" << sdev << "  :  ";
    //    cout << "Sum/n = " << sum << "/" << n << " = " << (n==0?0:mag/n) << endl;

    leds.push_back(Led(mx, my, 0, -2.5*log10((float)mag)+13.7));
}

void FilterLed::Stretch() const
{
    uint8_t min, max;
    GetMinMax(25, &min, &max);

    if (min==max || max-min>230) // 255/230=1.1
        return;

    const float scale = 255./(max-min);

    uint8_t *b = fImg;
    const uint8_t *e = fImg+fW*fH;

    while (b<e)
    {
        if (*b<min)
        {
            *b++=0;
            continue;
        }
        if (*b>max)
        {
            *b++=255;
            continue;
        }
        *b = (uint8_t)((*b-min)*scale);
        b++;
    }
}
