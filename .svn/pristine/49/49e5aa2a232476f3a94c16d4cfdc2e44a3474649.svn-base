#ifndef BASIC_GL_CAMERA_H_
#define BASIC_GL_CAMERA_H_

#define NBOARDS      40      // max. number of boards
#define NPIX       1440      // max. number of pixels
#define NTMARK      160      // max. number of timeMarker signals

#define MAX_NUM_PIXELS 1600
#define ACTUAL_NUM_PIXELS 1440

#include <vector>

#include <QtOpenGL/QGLWidget>

#include "externals/PixelMap.h"

class QMouseEvent;
class QRadioButton;
class QLabel;
class QButtonGroup;

///structure for storing edges of hexagons (for blurry display)
struct edge
{
    int first;
    int second;
};

///structure for storing neighbors of pixels. For camera position calculation and blurry display
struct PixelsNeighbors
{
    //neighbors. clockwise, starting from top
    int neighbors[6];
    PixelsNeighbors()
    {
        for (int i=0;i<6;i++)
            neighbors[i] = -1;
    }
    int& operator[](int index){return neighbors[index];}
};

class BasicGlCamera : public QGLWidget
{
    Q_OBJECT

public:
    BasicGlCamera(QWidget* parent = 0);
    ~BasicGlCamera();

    int fWhite;
    int fWhitePatch;

    int64_t fMin;
    int64_t fMax;
    float fScaleLimit;
    void setAutoscaleLowerLimit(float);

    int fTextSize;

    static PixelMap fPixelMap;
    static int pixelsPatch[NPIX];

    bool pixelColorUpToDate;

    GLfloat patchColour[3];
    GLfloat pixelContourColour[3];
    GLfloat patchesCoulour[3];
    GLfloat highlightedPatchesCoulour[3];
    GLfloat highlightedPixelsCoulour[3];
    GLfloat tooHighValueCoulour[3];
    GLfloat tooLowValueCoulour[3];

    std::string dataText;
    std::string unitsText;
    std::string titleText;

    void setUnits(const std::string& units);
    void setTitle(const std::string& title);
    void SetWhite(int idx);
    void SetMin(int64_t min);
    void SetMax(int64_t max);
    void SetAutoRefresh(bool on);
    void updateCamera();
    void assignPixelMap(const PixelMap& );
    void enableText(bool);

    bool fTextEnabled;

    float ss[5];// = {0.00, 0.25, 0.5, 0.75, 1.00};
    float rr[5];// = {0.15, 0.00, 0.00, 1.00, 0.85};
    float gg[5];// = {0.15, 0.00, 1.00, 0.00, 0.85};
    float bb[5];// = {0.15, 1.00, 0.00, 0.00, 0.85};

public Q_SLOTS:
        void linearScalePlease(bool);
        void logScalePlease(bool);
        void regularPalettePlease(bool);
        void prettyPalettePlease(bool);
        void greyScalePalettePlease(bool);
        void glowingPalettePlease(bool);
        void zeroRotationPlease(bool);
        void plus90RotationPlease(bool);
        void minus90RotationPlease(bool);
        void timedUpdate();

Q_SIGNALS:
         void signalCurrentPixel(int pixel);
         void signalPixelMoveOver(int pixel);
         void signalPixelDoubleClick(int pixel);
         void colorPaletteHasChanged();
         void signalUpdateCamera();



protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void drawCamera(bool alsoWire);
    virtual void drawPatches();
    virtual void setPatchColor(int id, float color[3]);
    virtual int PixelAtPosition(const QPoint &pos);
    virtual void DrawCameraText();
    void drawHexagon(int index, bool solid);

    int fPixelStride;
    int fcSlice;
    std::vector<double>fData;

 //   bool recalcColorPlease;
    static GLfloat pixelsCoords[MAX_NUM_PIXELS][3];
    static PixelsNeighbors neighbors[MAX_NUM_PIXELS];
    static int hardwareMapping[NPIX];
    GLfloat pixelsColor[NPIX][3];
    static  GLfloat verticesList[NPIX*6][2];
    static std::vector<edge> patchesIndices[160];
    static int verticesIndices[NPIX][6];
    static int softwareMapping[NPIX];
    float shownSizex;
    float shownSizey;
    float pixelSize;
    virtual void UpdateText();
    void DrawScale();
    void toggleInterfaceDisplay();
    QRadioButton* linearButton;
    QRadioButton* logButton;
    QRadioButton* regularPaletteButton;
    QRadioButton* prettyPaletteButton;
    QRadioButton* greyScalePaletteButton;
    QRadioButton* glowingPaletteButton;
    QRadioButton* zeroRotationButton;
    QRadioButton* minus90RotationButton;
    QRadioButton* plus90Rotationbutton;
    QLabel*       scaleLabel;
    QLabel*       colorPaletteLabel;
    QLabel*       rotationLabel;
    QButtonGroup* scaleGroup;
    QButtonGroup* colorGroup;
    QButtonGroup* rotationGroup;


    bool logScale;
    int cameraRotation;
    void buildVerticesList();
    virtual void buildPatchesIndices();
    void updateNeighbors(int currentPixel);
    void calculatePixelsCoords();
    float viewSize;
    bool autoRefresh;

    bool isFACT() const;

  private:
    void skipPixels(int start, int howMany);
    float hexRadius;
    float hexTolerance;
     int numVertices;
  protected:
     float fmin, fmax, fmean, frms, fmedian;


};

#endif
