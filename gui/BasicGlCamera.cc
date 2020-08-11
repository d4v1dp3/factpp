#include "BasicGlCamera.h"

#include <math.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>

#include <GL/glu.h>

#include "src/Time.h"
#include "src/tools.h"

using namespace std;;

//static variables
PixelMap BasicGlCamera::fPixelMap;
GLfloat BasicGlCamera::pixelsCoords[MAX_NUM_PIXELS][3];
PixelsNeighbors BasicGlCamera::neighbors[MAX_NUM_PIXELS];
int BasicGlCamera::hardwareMapping[NPIX];
GLfloat BasicGlCamera::verticesList[NPIX*6][2];
vector<edge> BasicGlCamera::patchesIndices[160];
int BasicGlCamera::verticesIndices[NPIX][6];
int BasicGlCamera::pixelsPatch[NPIX];
int BasicGlCamera::softwareMapping[NPIX];

static const float _coord = 1/sqrt(3.);

//Coordinates of an hexagon of radius 1 and center 0
GLfloat hexcoords[6][2] = {{-1*_coord,  1},
                           { 1*_coord,  1},
                           { 2*_coord,  0},
                           { 1*_coord, -1},
                           {-1*_coord, -1},
                           {-2*_coord,  0}};



    BasicGlCamera::BasicGlCamera(QWidget* cParent)
    : QGLWidget(QGLFormat(QGL::DoubleBuffer |
                          QGL::DepthBuffer),cParent)
    {
#ifdef HAVE_QT4
        QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
#endif
        fWhite = -1;
        fWhitePatch = -1;
        fMin = -1;
        fMax = -1;
        fScaleLimit = -0.5;
        fTextSize = 0;
        autoRefresh = false;
        logScale = false;
        cameraRotation = +90;
        fTextEnabled = true;
        unitsText = "";
        titleText = "";
        dataText = "";
        pixelContourColour[0] = 0.1f;
        pixelContourColour[1] = 0.1f;
        pixelContourColour[2] = 0.1f;
        patchesCoulour[0] = 0.1f;
        patchesCoulour[1] = 0.1f;
        patchesCoulour[2] = 0.1f;
        highlightedPatchesCoulour[0] = 0.6f;
        highlightedPatchesCoulour[1] = 0.6f;
        highlightedPatchesCoulour[2] = 0.6f;
        highlightedPixelsCoulour[0] = 0.8f;
        highlightedPixelsCoulour[1] = 0.8f;
        highlightedPixelsCoulour[2] = 0.8f;

        regularPalettePlease(true);


        viewSize = 1.0f;
/*
       ifstream fin1("Trigger-Patches.txt");
       if (!fin1.is_open())
       {
           cout << "Error: file \"Trigger-Patches.txt\" missing. Aborting." << endl;
           exit(-1);
       }
       l=0;
        while (getline(fin1, buf, '\n'))
        {
            buf = Tools::Trim(buf);
            if (buf[0]=='#')
                continue;

            stringstream str(buf);
            for (int i=0; i<9; i++)
            {
                unsigned int n;
                str >> n;

                if (n>=1440)
                    continue;

                patches[l][i] = hardwareMapping[n];
            }
            l++;
        }

        //now construct the correspondance between pixels and patches
        for (int i=0;i<NTMARK;i++)
            for (int j=0;j<9;j++)
                pixelsPatch[softwareMapping[patches[i][j]]] = i;

        for (int i=0;i<1440;i++)
            updateNeighbors(i);

        buildPatchesIndices();
*/////////////////////////////////
        regularPalettePlease(true);
//        ss[0] = 0;    ss[1] = 0.25f; ss[2] = 0.5f; ss[3] = 0.75f; ss[4] = 1.0f;
//        rr[0] = 0.15; rr[1] = 0;     rr[2] = 0;    rr[3] = 1.0f;  rr[4] = 0.85f;
//        gg[0] = 0.15; gg[1] = 0;     gg[2] = 1;    gg[3] = 0;     gg[4] = 0.85f;
//        bb[0] = 0.15; bb[1] = 1;     bb[2] = 0;    bb[3] = 0;     bb[4] = 0.85f;

        fPixelStride = 1;
        fcSlice = 0;
        fData.resize(1440);
        for (int i=0;i<NPIX;i++)
            fData[i] = (double)i;///1.44;//(double)(i)/(double)(ACTUAL_NUM_PIXELS);

//        setFont(QFont("Arial", 8));
        int buttonShift=0;
        scaleLabel = new QLabel("Scale", this);
//        buttonShift += scaleLabel->height();

        linearButton = new QRadioButton("Linear", this);
        linearButton->move(scaleLabel->width(), buttonShift);
        buttonShift += linearButton->height();

        logButton = new QRadioButton("Log", this);
        logButton->move(scaleLabel->width(), buttonShift);
        buttonShift += logButton->height()*1.1f;

        colorPaletteLabel = new QLabel("Colour\nPalette", this);
        colorPaletteLabel->move(0, buttonShift);
 //       buttonShift += colorPaletteLabel->height();

        regularPaletteButton = new QRadioButton("Regular", this);
        regularPaletteButton->move(colorPaletteLabel->width(), buttonShift);
        buttonShift += regularPaletteButton->height();

        prettyPaletteButton = new QRadioButton("Pretty", this);
        prettyPaletteButton->move(colorPaletteLabel->width(), buttonShift);
        buttonShift += prettyPaletteButton->height();

        greyScalePaletteButton = new QRadioButton("Grey Scale", this);
        greyScalePaletteButton->move(colorPaletteLabel->width(), buttonShift);
        buttonShift += greyScalePaletteButton->height();

        glowingPaletteButton = new QRadioButton("Glowing", this);
        glowingPaletteButton->move(colorPaletteLabel->width(), buttonShift);
        buttonShift += glowingPaletteButton->height()*1.1f;

        rotationLabel = new QLabel("Camera\nRotation", this);
        rotationLabel->move(0, buttonShift);
 //       buttonShift += rotationLabel->height();

        unsigned short utf16Array;
        utf16Array = 0x00b0;
        QString degreeSymbol(QString::fromUtf16(&utf16Array, 1));
        QString zerostr("0" + degreeSymbol);
        zeroRotationButton = new QRadioButton(zerostr, this);
        zeroRotationButton->move(rotationLabel->width(), buttonShift);
        buttonShift += zeroRotationButton->height();
         QString minus90str("+90" + degreeSymbol);
        minus90RotationButton = new QRadioButton(minus90str, this);
        minus90RotationButton->move(rotationLabel->width(), buttonShift);
        buttonShift += minus90RotationButton->height();
        QString plus90str("-90"+degreeSymbol);
        plus90Rotationbutton = new QRadioButton(plus90str, this);
        plus90Rotationbutton->move(rotationLabel->width(), buttonShift);


        scaleGroup = new QButtonGroup(this);
        colorGroup = new QButtonGroup(this);
        rotationGroup = new QButtonGroup(this);
        scaleGroup->addButton(linearButton);
        scaleGroup->addButton(logButton);
        colorGroup->addButton(regularPaletteButton);
        colorGroup->addButton(prettyPaletteButton);
        colorGroup->addButton(greyScalePaletteButton);
        colorGroup->addButton(glowingPaletteButton);
        rotationGroup->addButton(zeroRotationButton);
        rotationGroup->addButton(minus90RotationButton);
        rotationGroup->addButton(plus90Rotationbutton);

        linearButton->setChecked(true);
        regularPaletteButton->setChecked(true);
//        zeroRotationButton->setChecked(true);
        minus90RotationButton->setChecked(true);
//        linearButton->palette.setColor();

        linearButton->setAutoFillBackground(true);
        logButton->setAutoFillBackground(true);
        regularPaletteButton->setAutoFillBackground(true);
        prettyPaletteButton->setAutoFillBackground(true);
        greyScalePaletteButton->setAutoFillBackground(true);
        glowingPaletteButton->setAutoFillBackground(true);
        zeroRotationButton->setAutoFillBackground(true);
        minus90RotationButton->setAutoFillBackground(true);
        plus90Rotationbutton->setAutoFillBackground(true);
        scaleLabel->setAutoFillBackground(true);
        colorPaletteLabel->setAutoFillBackground(true);
        rotationLabel->setAutoFillBackground(true);

        linearButton->hide();
        logButton->hide();
        regularPaletteButton->hide();
        prettyPaletteButton->hide();
        greyScalePaletteButton->hide();
        glowingPaletteButton->hide();
        zeroRotationButton->hide();
        minus90RotationButton->hide();
        plus90Rotationbutton->hide();
        scaleLabel->hide();
        colorPaletteLabel->hide();
        rotationLabel->hide();

        connect(linearButton, SIGNAL(toggled(bool)),
                 this, SLOT(linearScalePlease(bool)));
        connect(logButton, SIGNAL(toggled(bool)),
                 this, SLOT(logScalePlease(bool)));
        connect(regularPaletteButton, SIGNAL(toggled(bool)),
                 this, SLOT(regularPalettePlease(bool)));
        connect(prettyPaletteButton, SIGNAL(toggled(bool)),
                 this, SLOT(prettyPalettePlease(bool)));
        connect(greyScalePaletteButton, SIGNAL(toggled(bool)),
                 this, SLOT(greyScalePalettePlease(bool)));
        connect(glowingPaletteButton, SIGNAL(toggled(bool)),
                 this, SLOT(glowingPalettePlease(bool)));
        connect(zeroRotationButton, SIGNAL(toggled(bool)),
                 this, SLOT(zeroRotationPlease(bool)));
        connect(minus90RotationButton, SIGNAL(toggled(bool)),
                 this, SLOT(plus90RotationPlease(bool)));
        connect(plus90Rotationbutton, SIGNAL(toggled(bool)),
                 this, SLOT(minus90RotationPlease(bool)));

        connect(this, SIGNAL(signalUpdateCamera()),
                this, SLOT(timedUpdate()));
    }
    BasicGlCamera::~BasicGlCamera()
    {
    }
    bool BasicGlCamera::isFACT() const
    {
        return hexRadius<0.02;
    }
    void BasicGlCamera::assignPixelMap(const PixelMap& map)
    {
        fPixelMap = map;

        int cnt = 0;
        for (auto it=fPixelMap.begin(); it!=fPixelMap.end(); it++)
        {
            hardwareMapping[it->index] = it->hw();
            softwareMapping[it->hw()]  = it->index;
            if (it->gapd>0)
                cnt++;
        }

        if (cnt==64)
        {
            cout << "INFO: 64 SiPMs found in mapping file -- assuming HAWC's Eye geometry." << endl;
            hexRadius = 0.060f;
        }
        else
            hexRadius = 0.015f;

        //now construct the correspondance between pixels and patches
        for (int i=0;i<NTMARK;i++)
            for (int j=0;j<9;j++)
                pixelsPatch[softwareMapping[i*9+j]] = i;

        calculatePixelsCoords();

        for (int i=0;i<1440;i++)
        {
            for (int j=0;j<6;j++)
                neighbors[i][j] = -1;
            updateNeighbors(i);
        }

        buildVerticesList();

        buildPatchesIndices();

    }
    void BasicGlCamera::enableText(bool on)
    {
        fTextEnabled = on;
    }
    void BasicGlCamera::setPatchColor(int id, float color[3])
    {
        for (int i=0;i<9;i++)
            for (int j=0;j<3;j++)
                pixelsColor[softwareMapping[id*9+i]][j] = color[j];
    }
    void BasicGlCamera::setUnits(const string& units)
    {
        unitsText = units;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::setTitle(const string& title)
    {
        titleText = title;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::SetWhite(int idx)
    {
        fWhite = idx;
        fWhitePatch = pixelsPatch[fWhite];
        if (isVisible() && autoRefresh)
            updateGL();
//         CalculatePatchColor();
    }
    void BasicGlCamera::SetMin(int64_t min)
    {
//        cout << "min: " << min << endl;
        fMin = min;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::setAutoscaleLowerLimit(float val)
    {
        fScaleLimit = val;
        if (isVisible() && autoRefresh)
            updateGL();
    }

    void BasicGlCamera::SetMax(int64_t max)
    {
//        cout << "max: " << max << endl;
        fMax = max;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::linearScalePlease(bool checked)
    {
        if (!checked) return;
        logScale = false;
        pixelColorUpToDate = false;
        emit colorPaletteHasChanged();
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::UpdateText()
    {
        ostringstream str;
        float min, max, median;
        int ii=0;
        for (;ii<ACTUAL_NUM_PIXELS;ii++)
        {
            if (finite(fData[ii]))
            {
                min = max = fData[ii];
                break;
            }
        }
        double mean = 0;
        double rms = 0;
        median = 0;
        if (ii==ACTUAL_NUM_PIXELS)
        {
            fmin = fmax = fmean = frms = fmedian = 0;
            return;
        }

        vector<double> medianVec;
        medianVec.resize(ACTUAL_NUM_PIXELS);
        auto it = medianVec.begin();
        int numSamples = 0;
        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
            if (!finite(fData[i]))
                continue;
            if (fData[i] < min)
                min = fData[i];
            if (fData[i] > max)
                max = fData[i];
            mean += fData[i];
            rms += fData[i]*fData[i];
            //medianSet.insert(fData[i]);
            *it = fData[i];
            it++;
            numSamples++;
        }

//        vector<double> medianVec;
//        medianVec.resize(ACTUAL_NUM_PIXELS);
//        int iii=0;
//        for (auto it=medianVec.begin(); it != medianVec.end(); it++) {
//            *it = fData[iii];
//            iii++;
//        }
        sort(medianVec.begin(), medianVec.begin()+numSamples);


        mean /= numSamples;
        rms = sqrt((rms/numSamples) - (mean * mean));

//       multiset<double>::iterator it = medianSet.begin();
        auto jt = medianVec.begin();
        for (int i=0;i<(numSamples/2)-1;i++)
        {
//            it++;
            jt++;
        }
        median = *jt;
 //       cout << *it << " " << *jt << endl;
        if (numSamples%2==0){
        jt++;
        median += *jt;
        median /= 2;}

        str << "Min: " << min << endl << " Max: " << max << " Mean: " << mean << " RMS: " << rms << " Median: " << median;
        str << " Units: " << unitsText;
        dataText = str.str();

        fmin = min;
        fmax = max;
        fmean = mean;
        frms = rms;
        fmedian = median;
    }
    void BasicGlCamera::DrawCameraText()
    {
        if (!fTextEnabled)
            return;
        glPushMatrix();
        glLoadIdentity();
//         cout << width() << " " << height() << endl;
        int textSize = (int)(height()*14/600);
//        setFont(QFont("Times", textSize));
        qglColor(QColor(255,223,127));
        float shiftx = 0.01f;//0.55f;
        float shifty = 0.01f;//0.65f;
        renderText(-shownSizex/2.f + shiftx, 0.f, 0.f, QString(dataText.c_str()));//-shownSizey/2.f + shifty, 0.f, QString(dataText.c_str()));


//        int textLength = titleText.size();
        renderText(-shownSizex/2.f + shiftx, shownSizey/2.f - textSize*pixelSize - shifty, 0.f, QString(titleText.c_str()));

        glPopMatrix();

  //      textSize = (int)(600*14/600);
//        setFont(QFont("Times", textSize));
    }
    void BasicGlCamera::DrawScale()
    {
        glPushMatrix();
        glLoadIdentity();
        glPushAttrib(GL_POLYGON_BIT);
        glShadeModel(GL_SMOOTH);
        glBegin(GL_QUADS);
        float oneX = shownSizex/2.f - shownSizex/50.f;
        float twoX = shownSizex/2.f;
        float oneY = -shownSizey/2.f;
        float twoY = -shownSizey/4.f;
        float threeY = 0;
        float fourY = shownSizey/4.f;
        float fiveY = shownSizey/2.f;
        glColor3f(rr[0], gg[0], bb[0]);
        glVertex2f(oneX, oneY);
        glVertex2f(twoX, oneY);
        glColor3f(rr[1], gg[1], bb[1]);
        glVertex2f(twoX, twoY);
        glVertex2f(oneX, twoY);

        glVertex2f(oneX, twoY);
        glVertex2f(twoX, twoY);
        glColor3f(rr[2], gg[2], bb[2]);
        glVertex2f(twoX, threeY);
        glVertex2f(oneX, threeY);

        glVertex2f(oneX, threeY);
        glVertex2f(twoX, threeY);
        glColor3f(rr[3], gg[3], bb[3]);
        glVertex2f(twoX, fourY);
        glVertex2f(oneX, fourY);

        glVertex2f(oneX, fourY);
        glVertex2f(twoX, fourY);
        glColor3f(rr[4], gg[4], bb[4]);
        glVertex2f(twoX, fiveY);
        glVertex2f(oneX, fiveY);
        float zeroX = oneX - shownSizex/50.f;
        float zeroY = fiveY - shownSizey/50.f;
        glColor3fv(tooHighValueCoulour);
        glVertex2f(zeroX, fiveY);
        glVertex2f(oneX, fiveY);
        glVertex2f(oneX, zeroY);
        glVertex2f(zeroX, zeroY);
        glColor3fv(tooLowValueCoulour);
        glVertex2f(zeroX, -fiveY);
        glVertex2f(oneX, -fiveY);
        glVertex2f(oneX, -zeroY);
        glVertex2f(zeroX, -zeroY);
        glEnd();
        glTranslatef(0,0,0.1f);

        //draw linear/log tick marks
        glColor3f(0.f,0.f,0.f);
        glBegin(GL_LINES);
        float value;
        for (int i=1;i<10;i++)
        {
            if (logScale)
                value = log10(i);
            else
                value = (float)(i)/10.f;
            float yy = -shownSizey/2.f + value*shownSizey;
            glVertex2f(oneX, yy);
            glVertex2f(twoX, yy);
        }
        glEnd();
        glPopAttrib();
        glPopMatrix();
    }
    void BasicGlCamera::logScalePlease(bool checked)
    {
        if (!checked) return;
        logScale = true;
        pixelColorUpToDate = false;
        emit colorPaletteHasChanged();
          if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::regularPalettePlease(bool checked)
    {
        if (!checked) return;
        ss[0] = 0;    ss[1] = 0.25f; ss[2] = 0.5f; ss[3] = 0.75f; ss[4] = 1.0f;
        rr[0] = 0;    rr[1] = 0;     rr[2] = 0;    rr[3] = 1.0f;  rr[4] = 1;
        gg[0] = 0;    gg[1] = 1;     gg[2] = 1;    gg[3] = 1;     gg[4] = 0;
        bb[0] = 0.5f; bb[1] = 1;     bb[2] = 0;    bb[3] = 0;     bb[4] = 0;
        tooHighValueCoulour[0] = 1.f;
        tooHighValueCoulour[1] = 1.f;
        tooHighValueCoulour[2] = 1.f;
        tooLowValueCoulour[0] = 0.f;
        tooLowValueCoulour[1] = 0.f;
        tooLowValueCoulour[2] = 0.f;
        pixelColorUpToDate = false;

        emit colorPaletteHasChanged();

        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::prettyPalettePlease(bool checked)
    {
        if (!checked) return;
        ss[0] = 0.f;    ss[1] = 0.25f; ss[2] = 0.5f; ss[3] = 0.75f; ss[4] = 1.0f;
        rr[0] = 0.f; rr[1] = 0.35f;     rr[2] = 0.85f;    rr[3] = 1.0f;  rr[4] = 1.f;
        gg[0] = 0.f; gg[1] = 0.10f;     gg[2] = 0.20f;    gg[3] = 0.73f;     gg[4] = 1.f;
        bb[0] = 0.f; bb[1] = 0.03f;     bb[2] = 0.06f;    bb[3] = 0.00f;     bb[4] = 1.f;
        tooHighValueCoulour[0] = 0.f;
        tooHighValueCoulour[1] = 1.f;
        tooHighValueCoulour[2] = 0.f;
        tooLowValueCoulour[0] = 0.f;
        tooLowValueCoulour[1] = 0.f;
        tooLowValueCoulour[2] = 1.f;
        pixelColorUpToDate = false;

        emit colorPaletteHasChanged();

        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::greyScalePalettePlease(bool checked)
    {
        if (!checked) return;
        ss[0] = 0;    ss[1] = 0.25f; ss[2] = 0.5f; ss[3] = 0.75f; ss[4] = 1.0f;
        rr[0] = 0; rr[1] = 0.25f;     rr[2] = 0.5f;    rr[3] = 0.75f;  rr[4] = 1.0f;
        gg[0] = 0; gg[1] = 0.25f;     gg[2] = 0.5f;    gg[3] = 0.75f;     gg[4] = 1.0f;
        bb[0] = 0; bb[1] = 0.25f;     bb[2] = 0.5f;    bb[3] = 0.75f;     bb[4] = 1.0f;
        tooHighValueCoulour[0] = 0.f;
        tooHighValueCoulour[1] = 1.f;
        tooHighValueCoulour[2] = 0.f;
        tooLowValueCoulour[0] = 0.f;
        tooLowValueCoulour[1] = 0.f;
        tooLowValueCoulour[2] = 1.f;
        pixelColorUpToDate = false;

        emit colorPaletteHasChanged();

        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::glowingPalettePlease(bool checked)
    {
        if (!checked) return;
        ss[0] = 0;    ss[1] = 0.25f; ss[2] = 0.5f; ss[3] = 0.75f; ss[4] = 1.0f;
        rr[0] = 0.15; rr[1] = 0.5;     rr[2] = 1.f;    rr[3] = 0.0f;  rr[4] = 1.f;
        gg[0] = 0.15; gg[1] = 0.5;     gg[2] = 1.f;    gg[3] = 0.5f;     gg[4] = 0.5f;
        bb[0] = 0.15; bb[1] = 0.5;     bb[2] = 1;      bb[3] = 1.f;     bb[4] = 0.f;
        tooHighValueCoulour[0] = 1.f;
        tooHighValueCoulour[1] = 0.f;
        tooHighValueCoulour[2] = 0.f;
        tooLowValueCoulour[0] = 0.f;
        tooLowValueCoulour[1] = 1.f;
        tooLowValueCoulour[2] = 0.f;
        pixelColorUpToDate = false;

        emit colorPaletteHasChanged();

        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::SetAutoRefresh(bool on)
    {
        autoRefresh = on;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::zeroRotationPlease(bool checked)
    {
        if (!checked) return;
        cameraRotation = 0;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::plus90RotationPlease(bool checked)
    {
        if (!checked) return;
        cameraRotation = 90;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void BasicGlCamera::minus90RotationPlease(bool checked)
    {
        if (!checked) return;
        cameraRotation = -90;
        pixelColorUpToDate = false;
        if (isVisible() && autoRefresh)
            updateGL();
    }

    void BasicGlCamera::initializeGL()
    {
        qglClearColor(QColor(212,208,200));//25,25,38));
        glShadeModel(GL_FLAT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

   //     glEnable (GL_LINE_SMOOTH);
   //     glEnable (GL_BLEND);
   //     glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   //     glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

    }
    void BasicGlCamera::resizeGL(int cWidth, int cHeight)
    {
        glViewport(0, 0, cWidth, cHeight);
         glMatrixMode(GL_PROJECTION);
         glLoadIdentity();
         GLfloat windowRatio = (float)cWidth/(float)cHeight;
         if (windowRatio < 1)
         {
             windowRatio = 1.0f/windowRatio;
             gluOrtho2D(-viewSize, viewSize, -viewSize*windowRatio, viewSize*windowRatio);
             pixelSize = 2*viewSize/(float)cWidth;
             shownSizex = 2*viewSize;
             shownSizey = 2*viewSize*windowRatio;
         }
         else
         {
             gluOrtho2D(-viewSize*windowRatio, viewSize*windowRatio, -viewSize, viewSize);
             pixelSize = 2*viewSize/(float)cHeight;
             shownSizex = 2*viewSize*windowRatio;
             shownSizey = 2*viewSize;
         }
         glMatrixMode(GL_MODELVIEW);

         fTextSize = (int)(cWidth*12/600); //want a sized 12 font for a window of 600 pixels width
         setFont(QFont("Monospace", fTextSize));
    }
    void BasicGlCamera::paintGL()
    {
         glClear(GL_COLOR_BUFFER_BIT);
         glLoadIdentity();

         glTranslatef(0,-0.44,0);
         glScalef(1.5, 1.5, 1.5);

         drawCamera(true);

         drawPatches();
    }
    void BasicGlCamera::toggleInterfaceDisplay()
    {
        if (linearButton->isVisible())
        {
            linearButton->hide();
            logButton->hide();
            regularPaletteButton->hide();
            prettyPaletteButton->hide();
            greyScalePaletteButton->hide();
            glowingPaletteButton->hide();
            zeroRotationButton->hide();
            minus90RotationButton->hide();
            plus90Rotationbutton->hide();
            scaleLabel->hide();
            colorPaletteLabel->hide();
            rotationLabel->hide();
        }
        else
        {
            linearButton->show();
            logButton->show();
            regularPaletteButton->show();
            prettyPaletteButton->show();
            greyScalePaletteButton->show();
            glowingPaletteButton->show();
            zeroRotationButton->show();
            minus90RotationButton->show();
            plus90Rotationbutton->show();
            scaleLabel->show();
            colorPaletteLabel->show();
            rotationLabel->show();
        }
    }

    void BasicGlCamera::mousePressEvent(QMouseEvent *)
    {

    }
    void BasicGlCamera::mouseMoveEvent(QMouseEvent *)
    {

    }
    void BasicGlCamera::mouseDoubleClickEvent(QMouseEvent *)
    {

    }
    void BasicGlCamera::timedUpdate()
    {
        if (isVisible())
            updateGL();
    }
    void BasicGlCamera::updateCamera()
    {
        emit signalUpdateCamera();
    }
    void BasicGlCamera::drawCamera(bool alsoWire)
    {
//        cout << "Super PaintGL" << endl;
        glColor3f(0.5,0.5,0.5);
        glLineWidth(1.0);
        float color;

        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
            color = float(fData[i*fPixelStride+fcSlice]);// + ]eventData[nRoi*i + whichSlice]+(VALUES_SPAN/2))/(float)(VALUES_SPAN-1);
            int index = 0;
            while (ss[index] < color)
                index++;
            index--;
            float weight0 = (color-ss[index]) / (ss[index+1]-ss[index]);
            float weight1 = 1.0f-weight0;
            pixelsColor[i][0] = weight1*rr[index] + weight0*rr[index+1];
            pixelsColor[i][1] = weight1*gg[index] + weight0*gg[index+1];
            pixelsColor[i][2] = weight1*bb[index] + weight0*bb[index+1];
        }

        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
 //           if (i == 690 ||
 //               i == 70)
 //               continue;
            glColor3fv(pixelsColor[i]);
            glLoadName(i);

        drawHexagon(i,true);

        }
        if (!alsoWire)
            return;
        glColor3f(0.0f,0.0f,0.0f);
        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
//            if (i == 690 ||
//                i == 70)
//                continue;
            drawHexagon(i, false);
        }
    }
    void BasicGlCamera::drawPatches()
    {
        glLineWidth(2.0f);
        float backupRadius = hexRadius;
        hexRadius *= 0.95;
        glColor3f(0.5f, 0.5f, 0.3f);
        glBegin(GL_LINES);
        for (int i=0;i<NTMARK;i++)
        {
            for (unsigned int j=0;j<patchesIndices[i].size();j++)
            {
                glVertex2fv(verticesList[patchesIndices[i][j].first]);
                glVertex2fv(verticesList[patchesIndices[i][j].second]);
            }
        }
        glEnd();
        hexRadius = backupRadius;
    }
    int BasicGlCamera::PixelAtPosition(const QPoint &cPos)
    {
        const int MaxSize = 512;
        GLuint buffer[MaxSize];
        GLint viewport[4];

        makeCurrent();

        glGetIntegerv(GL_VIEWPORT, viewport);
        glSelectBuffer(MaxSize, buffer);
        glRenderMode(GL_SELECT);

        glInitNames();
        glPushName(0);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        GLfloat windowRatio = GLfloat(width()) / GLfloat(height());
        gluPickMatrix(GLdouble(cPos.x()), GLdouble(viewport[3] - cPos.y()),
                1.0, 1.0, viewport);

        if (windowRatio < 1)
         {
             windowRatio = 1.0f/windowRatio;
             gluOrtho2D(-viewSize, viewSize, -viewSize*windowRatio, viewSize*windowRatio);
         }
         else
         {
             gluOrtho2D(-viewSize*windowRatio, viewSize*windowRatio, -viewSize, viewSize);
         }

        glMatrixMode(GL_MODELVIEW);
        drawCamera(false);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        //for some reason that I cannot understand, the push/pop matrix doesn't do the trick here... bizarre
        //ok, so re-do the resizeGL thing.
        resizeGL(width(), height());

        if (!glRenderMode(GL_RENDER))
            return -1;

        return buffer[3];
    }
    void BasicGlCamera::drawHexagon(int index, bool solid)
    {
/*        float minX, maxX, minY, maxY;
        minX = minY = 1e10;
        maxX = maxY = -1e10;
        for (int i=0;i<1438;i++)
        {
            for (int j=0;j<6;j++)
            {
                if (verticesList[verticesIndices[i][j]][0] > maxX)
                    maxX = verticesList[verticesIndices[i][j]][0];
                if (verticesList[verticesIndices[i][j]][0] < minX)
                    minX = verticesList[verticesIndices[i][j]][0];
                if (verticesList[verticesIndices[i][j]][1] > maxY)
                    maxY = verticesList[verticesIndices[i][j]][1];
                if (verticesList[verticesIndices[i][j]][1] < minY)
                    minY = verticesList[verticesIndices[i][j]][1];
            }
        }
        cout << "Min, Max X: " << minX << " " << maxX << endl;
        cout << "Min, Max Y: " << minY << " " << maxY << endl;
        exit(0);*/
        if (solid)
            glBegin(GL_POLYGON);
        else
            glBegin(GL_LINE_LOOP);

        glVertex2fv(verticesList[verticesIndices[index][0]]);
        glVertex2fv(verticesList[verticesIndices[index][1]]);
        glVertex2fv(verticesList[verticesIndices[index][2]]);
        glVertex2fv(verticesList[verticesIndices[index][3]]);
        glVertex2fv(verticesList[verticesIndices[index][4]]);
        glVertex2fv(verticesList[verticesIndices[index][5]]);
        if (solid)
            glVertex2fv(verticesList[verticesIndices[index][0]]);

        glEnd();
    }

    void BasicGlCamera::updateNeighbors(int currentPixel)
    {
        hexTolerance = hexRadius/100.0f;

        float squaredDistance = 0;
        for (int i=0;i<currentPixel;i++)
        {
            squaredDistance = (pixelsCoords[i][0] - pixelsCoords[currentPixel][0])*
                              (pixelsCoords[i][0] - pixelsCoords[currentPixel][0]) +
                              (pixelsCoords[i][1] - pixelsCoords[currentPixel][1])*
                              (pixelsCoords[i][1] - pixelsCoords[currentPixel][1]);
            if (squaredDistance < 4*hexRadius*hexRadius*(1.0f+hexTolerance))//neighbor !
            {//ok, but which one ?
                if (fabs(pixelsCoords[i][0] - pixelsCoords[currentPixel][0]) < hexTolerance &&
                    pixelsCoords[i][1] < pixelsCoords[currentPixel][1]){//top
                    neighbors[i][0] = currentPixel;
                    neighbors[currentPixel][3] = i;
                    continue;}
                if (fabs(pixelsCoords[i][0] - pixelsCoords[currentPixel][0]) < hexTolerance &&
                    pixelsCoords[i][1] > pixelsCoords[currentPixel][1]){//bottom
                    neighbors[i][3] = currentPixel;
                    neighbors[currentPixel][0] = i;
                    continue;}
                if (pixelsCoords[i][0] > pixelsCoords[currentPixel][0] &&
                    pixelsCoords[i][1] > pixelsCoords[currentPixel][1]){//top right
                    neighbors[i][4] = currentPixel;
                    neighbors[currentPixel][1] = i;
                    continue;}
                if (pixelsCoords[i][0] > pixelsCoords[currentPixel][0] &&
                    pixelsCoords[i][1] < pixelsCoords[currentPixel][1]){//bottom right
                    neighbors[i][5] = currentPixel;
                    neighbors[currentPixel][2] = i;
                    continue;}
                if (pixelsCoords[i][0] < pixelsCoords[currentPixel][0] &&
                    pixelsCoords[i][1] > pixelsCoords[currentPixel][1]){//top left
                    neighbors[i][2] = currentPixel;
                    neighbors[currentPixel][5] = i;
                    continue;}
                if (pixelsCoords[i][0] < pixelsCoords[currentPixel][0] &&
                    pixelsCoords[i][1] < pixelsCoords[currentPixel][1]){//bottom left
                    neighbors[i][1] = currentPixel;
                    neighbors[currentPixel][4] = i;
                    continue;}
            }
        }
    }
    void BasicGlCamera::skipPixels(int start, int howMany)
    {
        for (int i=start;i<MAX_NUM_PIXELS-howMany;i++)
        {
            pixelsCoords[i][0] = pixelsCoords[i+howMany][0];
            pixelsCoords[i][1] = pixelsCoords[i+howMany][1];
        }
    }
    void BasicGlCamera::calculatePixelsCoords()
    {
        if (fabs(pixelsCoords[0][1])>0.000001)
            return;

        /****************** HAWC's Eye camera layout **************************/
        if (!isFACT())
        {
            static const double gsSin60 = sqrt(3.)/2;

            pixelsCoords[0][0] = 0;
            pixelsCoords[0][1] = 0;

            uint32_t cnt  = 1;
            for (int32_t ring=1; ring<=4; ring++)
            {
                for (int32_t s=0; s<6; s++)
                {
                    for (int i=1; i<=ring; i++)
                    {
                        switch (s)
                        {
                        case 0: // Direction South East
                            pixelsCoords[cnt][0] =  ring-i;
                            pixelsCoords[cnt][1] = (ring+i)*0.5;
                            break;

                        case 1: // Direction North East
                            pixelsCoords[cnt][0] = -i;
                            pixelsCoords[cnt][1] = ring-i*0.5;
                            break;

                        case 2: // Direction North
                            pixelsCoords[cnt][0] = -ring;
                            pixelsCoords[cnt][1] = ring*0.5-i;
                            break;

                        case 3: // Direction North West
                            pixelsCoords[cnt][0] = -(ring-i);
                            pixelsCoords[cnt][1] = -(ring+i)*0.5;
                            break;

                        case 4: // Direction South West
                            pixelsCoords[cnt][0] = i;
                            pixelsCoords[cnt][1] = 0.5*i-ring;
                            break;

                        case 5: // Direction South
                            pixelsCoords[cnt][0] = ring;
                            pixelsCoords[cnt][1] = i-ring*0.5;
                            break;

                        }
                        cnt++;
                    }
                }
            }

            pixelsCoords[cnt][0] = -4;
            pixelsCoords[cnt][1] = -4;

            cnt++;

            pixelsCoords[cnt][0] =  4;
            pixelsCoords[cnt][1] = -4;

            cnt++;

            pixelsCoords[cnt][0] = 4;
            pixelsCoords[cnt][1] = 4;

            cnt++;

            for (int i=0; i<8; i++)
            {
                pixelsCoords[cnt+i][0] = i-3.5;
                pixelsCoords[cnt+i][1] = 7.25 + (i%2)*0.75;
            }

            cnt += 8;

            for (;cnt<MAX_NUM_PIXELS; cnt++)
            {
                pixelsCoords[cnt][0] =  1000;
                pixelsCoords[cnt][1] =  1000;
            }

            for (int i=0; i<MAX_NUM_PIXELS; i++)
            {
                if (i<64)
                {
                    pixelsCoords[i][1] += 1.5;
                    pixelsCoords[i][0] *= gsSin60;
                }

                pixelsCoords[i][0] *= 2*hexRadius;
                pixelsCoords[i][1] *= 2*hexRadius;
                pixelsCoords[i][2]  = 0;
            }
        }
        else
        {
            /************************ FACT Camera layout **************************/
            pixelsCoords[0][0] = 0;
            pixelsCoords[0][1] = 0.3 - hexRadius;
            pixelsCoords[0][2] = 0;
            pixelsCoords[1][0] = 0;
            pixelsCoords[1][1] = 0.3+hexRadius;
            pixelsCoords[1][2] = 0;
            neighbors[0][0] = 1;
            neighbors[1][3] = 0;
            //from which side of the previous hexagon are we coming from ?
            int fromSide = 3;
            //to which side are we heading to ?
            int toSide = 0;
            for (int i=2;i<MAX_NUM_PIXELS;i++)
            {
                toSide = fromSide-1;
                if (toSide < 0)
                    toSide =5;
                while (neighbors[i-1][toSide] >= 0)
                {
                    toSide--;
                    if (toSide < 0)
                        toSide = 5;
            }
                fromSide = toSide + 3;
                if (fromSide > 5)
                    fromSide -= 6;
                //ok. now we now in which direction we're heading
                pixelsCoords[i][0] = pixelsCoords[i-1][0];
                pixelsCoords[i][1] = pixelsCoords[i-1][1];
                pixelsCoords[i][2] = pixelsCoords[i-1][2];
                switch (toSide)
                {
                case 0:
                    pixelsCoords[i][1] += 2*hexRadius;
                    break;
                case 1:
                    pixelsCoords[i][0] += (2*hexRadius)*sin(M_PI/3.0);
                    pixelsCoords[i][1] += (2*hexRadius)*cos(M_PI/3.0);
                    break;
                case 2:
                    pixelsCoords[i][0] += (2*hexRadius)*sin(M_PI/3.0);
                    pixelsCoords[i][1] -= (2*hexRadius)*cos(M_PI/3.0);
                    break;
                case 3:
                    pixelsCoords[i][1] -= 2*hexRadius;
                    break;
                case 4:
                    pixelsCoords[i][0] -= (2*hexRadius)*sin(M_PI/3.0);
                    pixelsCoords[i][1] -= (2*hexRadius)*cos(M_PI/3.0);
                    break;
                case 5:
                    pixelsCoords[i][0] -= (2*hexRadius)*sin(M_PI/3.0);
                    pixelsCoords[i][1] += (2*hexRadius)*cos(M_PI/3.0);
                    break;
                };
                //            pixelsCoords[i][1] -= hexRadius;

                updateNeighbors(i);
            }
            //Ok. So now we've circled around all the way to MAX_NUM_PIXELS
            //do the required shifts so that it matches the fact camera up to ACTUAL_NUM_PIXELS pixels
            //remember the location pixels 1438 and 1439, and re-assign them later on
            GLfloat backupCoords[4];
            skipPixels(1200, 1);
            skipPixels(1218, 3);
            skipPixels(1236, 1);
            skipPixels(1256, 1);
            skipPixels(1274, 3);
            skipPixels(1292, 3);
            skipPixels(1309, 6);
            skipPixels(1323, 7);
            skipPixels(1337, 6);
            skipPixels(1354, 6);
            skipPixels(1368, 7);
            //la c'est dans 1390 qu'il y a 1439
            backupCoords[0] = pixelsCoords[1390][0];
            backupCoords[1] = pixelsCoords[1390][1];
            skipPixels(1382, 9);
            skipPixels(1394, 12);
            skipPixels(1402, 15);
            skipPixels(1410, 12);
            //la c'est dans 1422 qu'il y a 1438
            backupCoords[2] = pixelsCoords[1422][0];
            backupCoords[3] = pixelsCoords[1422][1];
            skipPixels(1422, 12);
            skipPixels(1430, 15);

            pixelsCoords[1438][0] = backupCoords[2];
            pixelsCoords[1438][1] = backupCoords[3];
            pixelsCoords[1439][0] = backupCoords[0];
            pixelsCoords[1439][1] = backupCoords[1];
        }
    }

    void BasicGlCamera::buildVerticesList()
    {
        hexTolerance = hexRadius/100.0f;

        numVertices = 0;
         GLfloat cVertex[2];
         for (int i=0;i<NPIX;i++)
         {
             for (int j=0;j<6;j++)
             {
                 for (int k=0;k<2;k++)
                     cVertex[k] = hexcoords[j][k]*hexRadius + pixelsCoords[i][k];

                 bool found = false;
                 for (int k=0;k<numVertices;k++)
                 {
                     if ((cVertex[0] - verticesList[k][0])*
                         (cVertex[0] - verticesList[k][0]) +
                         (cVertex[1] - verticesList[k][1])*
                         (cVertex[1] - verticesList[k][1]) < hexTolerance*hexTolerance)
                         {
                             found = true;
                             break;
                         }
                 }
                 if (!found)
                 {
                     for (int k=0;k<2;k++)
                         verticesList[numVertices][k] = cVertex[k];
                     numVertices++;
                 }
             }
         }
//cout << "numVertices: " << numVertices << endl;
         for (int i=0;i<NPIX;i++)
         {
             for (int j=0;j<6;j++)
             {
                 for (int k=0;k<2;k++)
                     cVertex[k] = hexcoords[j][k]*hexRadius + pixelsCoords[i][k];

                 for (int k=0;k<numVertices;k++)
                 {
                     if ((cVertex[0] - verticesList[k][0])*
                          (cVertex[0] - verticesList[k][0]) +
                          (cVertex[1] - verticesList[k][1])*
                          (cVertex[1] - verticesList[k][1]) < hexTolerance*hexTolerance)
                          {
                             verticesIndices[i][j] = k;
                             break;
                          }
                 }
             }
         }
    }
    void BasicGlCamera::buildPatchesIndices()
    {
        vector<edge>::iterator it;
        bool erased = false;
//        patchesIndices.resize(NTMARK);
        for (int i=0;i<NTMARK;i++)//for all patches
        {
            patchesIndices[i].clear();
            for (int j=0;j<9;j++)//for all cells of the current patch
            {
                if (softwareMapping[i*9+j] >= ACTUAL_NUM_PIXELS)
                    continue;
                for (int k=0;k<6;k++)//for all sides of the current cell
                {
                    int first = k-1;
                    int second = k;
                    if (first < 0)
                        first = 5;
                    erased = false;
                    for (it=(patchesIndices[i]).begin(); it != (patchesIndices[i]).end(); it++)//check if this side is here already or not
                    {
                        const int idx = i*9+j;

                        if ((it->first == verticesIndices[softwareMapping[idx]][first] &&
                             it->second == verticesIndices[softwareMapping[idx]][second]) ||
                            (it->first == verticesIndices[softwareMapping[idx]][second] &&
                             it->second == verticesIndices[softwareMapping[idx]][first]))
                        {
                            patchesIndices[i].erase(it);
                            erased = true;
                            break;
                        }
                    }
                    if (!erased)
                    {
                        edge temp;
                        temp.first = verticesIndices[softwareMapping[i*9+j]][first];
                        temp.second = verticesIndices[softwareMapping[i*9+j]][second];
                        patchesIndices[i].push_back(temp);
                    }
                }
            }
        }
//        for (int i=0;i<NTMARK;i++)
//        {
//            cout << ".....................patch " << i << " size: " << patchesIndices[i].size() << endl;
//            for (unsigned int j=0;j<patchesIndices[i].size();j++)
//            {
//               if (patchesIndices[i][j].first < 0 || patchesIndices[i][j].first > 3013)
//                cout << patchesIndices[i][j].first << " and " << patchesIndices[i][j].second << " and " << j << endl;
//            }
//        }
    }

