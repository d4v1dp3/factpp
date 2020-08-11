/*
 * Q3DCameraWidget.h
 *
 *  Created on: Aug 26, 2011
 *      Author: lyard
 */

#ifndef Q3DCAMERAWIDGET_H_
#define Q3DCAMERAWIDGET_H_

#include "BasicGlCamera.h"
#include <QtCore/QTimer>
#include <iostream>


using namespace std;

struct cameraLocation
{
    float rotX;
    float rotY;
    float position[3];

    cameraLocation() : rotX(0), rotY(0), position{0,0,0}
    {}
    cameraLocation(float rx, float ry, float x, float y, float z): rotX(rx), rotY(ry), position{x,y,z}
    {}
};

struct float3
{
    float data[4];
    float3()
    {
        data[0] = data[1] = data[2] = 0;
        data[3] = 1.f;
    }
    float& operator [] (int index)
    {
        return data[index];
    }
};
class Q3DCameraWidget : public BasicGlCamera
{
    Q_OBJECT

public:
    Q3DCameraWidget(QWidget* pparent = 0);
    ~Q3DCameraWidget();
    void setData(float* data);
    void setData(short* data);
public Q_SLOTS:
    void timedUpdate();

protected:
    void paintGL();
    void initializeGL();
    void resizeGL(int cWidth, int cHeight);
    void drawCameraBody();
    float rotX, rotY;
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent *cEvent);
    void mouseMoveEvent(QMouseEvent *cEvent);

private:
    cameraLocation currentLoc;
    vector<short> _data;
    vector<float> _colorR;
    vector<float> _colorG;
    vector<float> _colorB;
    vector<float> _x;
    vector<float> _y;
    vector<float> _z;
    QTimer _timer;
    bool isPicking;
    void calculateColorsAndPositions();
    bool _warningWritten;

};

#endif /* Q3DCAMERAWIDGET_H_ */
