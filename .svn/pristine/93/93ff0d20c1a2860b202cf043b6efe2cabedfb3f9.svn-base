#include "MCaos.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <math.h>

#include "Led.h"
#include "FilterLed.h"

using namespace std;

void MCaos::ReadResources(const char *name)
{
    ifstream fin(name);
    if (!fin)
    {
        cout << "ERROR - Cannot open " << name << endl;
        return;
    }

    fPositions.clear();

    cout << " Reading " << name << ":" << endl;
    cout << "------------------------------" << endl;
    while (1)
    {
        double px, py, phi;
        fin >> px >> py >> phi;
        if (!fin)
            break;

        cout << " Led #" << fPositions.size() << ":  ";
        cout << setw(3) << px << " ";
        cout << setw(3) << py << "  (";
        cout << setw(3) << phi << ")\n";
        AddPosition(px, py, phi);
    }
    cout << "Found " << fPositions.size() << " leds." << endl;
}

void MCaos::CalcCenters(const vector<Led> &leds, float min, float max)
{
    fRings.clear();

    const int nPoints = leds.size();

    // A minimum of at least 3 points is mandatory!
    if (nPoints<fMinNumberLeds || nPoints<3)
        return;

//    ofstream fout("rings.txt", ios::app);

    for (int i=0; i<nPoints-2; i++)
        for (int j=i+1; j<nPoints-1; j++)
            for (int k=j+1; k<nPoints; k++)
            {
                Ring ring;
                if (!ring.CalcCenter(leds[i], leds[j], leds[k]))
                    continue;

//                fout << i+j*10+k*100 << " " << ring.GetR() << " " << ring.GetX() << " " << ring.GetY() << endl;

                //
                //filter and remove rings with too big or too small radius
                //
                if ((min>=0&&ring.GetR()<min) || (max>=0&&ring.GetR()>max))
                    continue;

                fRings.push_back(ring);
            }
}

int32_t MCaos::CalcRings(std::vector<Led> &leds, float min, float max)
{
    CalcCenters(leds, min, max);

    fCenter.InterpolCenters(fRings);

    for (auto it=leds.begin(); it!=leds.end(); it++)
        it->CalcPhi(fCenter);

    return fRings.size();
}

Ring MCaos::Run(uint8_t *img)
{
    fLeds.clear();

    //          img  width height radius sigma
    FilterLed f(img, 768, 576, fSizeBox, fSizeBox, fCut);

    for (auto it=fPositions.begin(); it!=fPositions.end(); it++)
    {
        std::vector<Led> arr;

        // Try to find Led in this area
        f.Execute(arr, floor(it->GetX()), floor(it->GetY()));

        // Loop over newly found Leds
        for (auto jt=arr.begin(); jt!=arr.end(); jt++)
        {
            // Add Offset to Led
            //jt->AddOffset(it->GetDx(), it->GetDy());

            // Remember the expected phi for each detected led
            jt->SetPhi(it->GetPhi());

            // Mark Led in image (FIXME: Move to MStarguider)
            f.MarkPoint(jt->GetX(), jt->GetY(), jt->GetMag());
        }

        fLeds.insert(fLeds.end(), arr.begin(), arr.end());
    }

    fNumDetectedRings = CalcRings(fLeds, fMinRadius, fMaxRadius);

    double sumphi = 0;
    for (auto it=fLeds.begin(); it!=fLeds.end(); it++)
    {
        //cout << it->CalcPhi(fCenter) << "|" << it->GetPhi() << " ";
        double dphi = it->CalcPhi(fCenter) - it->GetPhi();
        if (dphi>M_PI)
            dphi -= 2*M_PI;
        if (dphi<-M_PI)
            dphi += 2*M_PI;

        sumphi += dphi;
    }
    //cout << endl;

    fCenter.SetPhi(sumphi/fLeds.size());

    return fCenter;
}
