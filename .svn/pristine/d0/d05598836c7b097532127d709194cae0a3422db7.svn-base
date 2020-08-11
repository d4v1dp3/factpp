#ifndef Q_CAMERA_WIDGET_H_
#define Q_CAMERA_WIDGET_H_

#include "BasicGlCamera.h"
#include <valarray>
#include <set>

class QCameraWidget : public BasicGlCamera
{
    Q_OBJECT

    typedef std::pair<double, double> Position;
    typedef std::vector<Position> Positions;

    //FIXME this variable seems to be deprecated
    Positions fGeom;

    std::vector<bool> fBold;
    std::vector<bool> fEnable;

    std::vector<int> highlightedPatches;
    std::vector<int> highlightedPixels;


    int lastFace;
    bool fShowPixelMoveOver;
    bool fShowPatchMoveOver;

public:
    bool fDrawPatch;
    void highlightPixel(int idx, bool highlight=true);
    void highlightPatch(int idx, bool highlight=true);
    void clearHighlightedPatches();
    void clearHighlightedPixels();
    QCameraWidget(QWidget *pparent = 0);
    void paintGL();
    void mousePressEvent(QMouseEvent *cEvent);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void Reset();
    void drawCamera(bool alsoWire);
    void DrawCameraText();
    void drawPatches();
     void SetEnable(int idx, bool b);
     double GetData(int idx);
    const char *GetName();

     int GetIdx(float px, float py);
     char *GetObjectInfo(int px, int py);

     void SetData(const std::valarray<double> &ddata);
     void SetData(const std::valarray<float> &ddata);


     void ShowPixelCursor(bool);
     void ShowPatchCursor(bool);

private:
     void CalculatePixelsColor();
     void CalculatePatchColor();

};

typedef QCameraWidget Camera;
#endif
