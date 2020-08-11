/*
 * Q3DCameraWidget.cc
 *
 *  Created on: Aug 26, 2011
 *      Author: lyard
 */
#include "Q3DCameraWidget.h"

#include <math.h>
#include <sstream>

#include <GL/glu.h>

#include <QMouseEvent>

    Q3DCameraWidget::Q3DCameraWidget(QWidget* pparent) : BasicGlCamera(pparent),
                                                         currentLoc()
    {
        _data.resize(432000);
        _colorR.resize(432000);
        _colorG.resize(432000);
        _colorB.resize(432000);
        _x.resize(432000);
        _y.resize(432000);
        _z.resize(432000);
        for (int i=0;i<432000;i++)
        {
            _data[i] = 0;
            _colorR[i] = 0;
            _colorG[i] = 0;
            _colorB[i] = 0;
            _x[i] = 0;
            _y[i] = 0;
            _z[i] = 0;
        }
        _warningWritten = false;
    }
    Q3DCameraWidget::~Q3DCameraWidget()
    {

    }
    void Q3DCameraWidget::timedUpdate()
    {
        updateGL();
    }

    int rotation =130;
    int rotationy = 30;
    float transZ = 0;
   void Q3DCameraWidget::mousePressEvent(QMouseEvent* cEvent)
    {

         if (cEvent->buttons()  & Qt::LeftButton)
         {
             rotationy = -60 + (cEvent->pos().y()/(float)height())*120.f;
             rotation = 130 + (cEvent->pos().x()/(float)width())*180.f;
         }
         else if (cEvent->buttons() & Qt::RightButton)
         {
             if (cEvent->pos().y() > height()/2)
                 transZ -= 0.5;
             else
                 transZ += 0.5;
         }
         updateGL();
    }
    void Q3DCameraWidget::mouseDoubleClickEvent(QMouseEvent *cEvent)
    {

    }
    void Q3DCameraWidget::mouseMoveEvent(QMouseEvent *cEvent)
    {
        if (cEvent->buttons() & Qt::LeftButton) {
            mousePressEvent(cEvent);
        }

    }
    void Q3DCameraWidget::paintGL()
    {
        makeCurrent();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        glTranslatef(-0.0,-0.0, -5);
        glTranslatef(0,0,(float)(transZ));
        glRotatef((float)rotationy,1.0,0.0,0.0);
        glRotatef((float)rotation, 0.0, 1.0, 0.0);

        glColor3f(1.0,0.0,0.0);

        glBegin(GL_TRIANGLES);
        for (int i=0;i<1439;i++)
        {
            for (int j=6;j<250;j++)
            {
                //get the 4 vertices that we need for drawing this patch
                glColor3f(_colorR[i*300+j],_colorG[i*300+j],_colorB[i*300+j]);
                glVertex3f(_x[i*300+j], _y[i*300+j], _z[i*300+j]);
                glColor3f(_colorR[i*300+j+1],_colorG[i*300+j+1],_colorB[i*300+j+1]);
                glVertex3f(_x[i*300+j+1], _y[i*300+j+1], _z[i*300+j+1]);
                glColor3f(_colorR[(i+1)*300+j],_colorG[(i+1)*300+j],_colorB[(i+1)*300+j]);
                glVertex3f(_x[(i+1)*300+j], _y[(i+1)*300+j], _z[(i+1)*300+j]);

                glColor3f(_colorR[i*300+j+1],_colorG[i*300+j+1],_colorB[i*300+j+1]);
                glVertex3f(_x[i*300+j+1], _y[i*300+j+1], _z[i*300+j+1]);
                glColor3f(_colorR[(i+1)*300+j+1],_colorG[(i+1)*300+j+1],_colorB[(i+1)*300+j+1]);
                glVertex3f(_x[(i+1)*300+j+1], _y[(i+1)*300+j+1], _z[(i+1)*300+j+1]);
                glColor3f(_colorR[(i+1)*300+j],_colorG[(i+1)*300+j],_colorB[(i+1)*300+j]);
                glVertex3f(_x[(i+1)*300+j], _y[(i+1)*300+j], _z[(i+1)*300+j]);

            }
        }
        glEnd();

    }
    void Q3DCameraWidget::calculateColorsAndPositions()
    {
        short min = 10000;
         short max = -10000;
         for (int k=0;k<1440;k++)
             for (int j=6;j<251;j++)
         {
               int  i = k*300+j;
             if (_data[i] < min)
                 min = _data[i];
             if (_data[i] > max)
                 max = _data[i];
         }
         float span = max - min;


         //max should be at one, min at -1

         for (int i=0;i<1440;i++)
         {
             for (int j=6;j<251;j++)
             {
                 _x[i*300+j] = -1 + (2.f*i)/1440.f;
                 _y[i*300+j] = -0.5 + 1.0f*(_data[i*300+j] - min)/span;
                 _z[i*300+j] = -1+(2.f*j)/300.f;
                 float value = (_data[i*300 + j] - min)/span;
                 if (value < 0.33)
                 {
                      _colorR[i*300+j] = 0;
                      _colorG[i*300+j] = 0;
                      _colorB[i*300+j] = value/0.33;
                 }
                 if (value >= 0.33 && value <= 0.66)
                 {
                     _colorR[i*300+j] = 0;
                     _colorG[i*300+j] = (value-0.33)/0.33;
                     _colorB[i*300+j] = 1 - ((value-0.33)/0.33);
                  }
                 if (value > 0.66)
                 {
                     _colorR[i*300+j] = (value-0.66)/0.33;
                      _colorG[i*300+j] = 1 - ((value-0.66)/0.33);
                      _colorB[i*300+j] = 0;

                 }
             }
         }



    }
    void Q3DCameraWidget::setData(float* ddata)
    {
        if (!_warningWritten)
        {
            _warningWritten = true;
            cout << "Info : 3D plotter disabled. requires more work so that less than 300 slices per pixel can be loaded" << endl;
            cout << "Contact Etienne (etienne.lyard@unige.ch) for more information." << endl;
        }
       //disabled for now as I am working with 150 slices only
/*        for (int i=0;i<1440;i++)
            for (int j=0;j<300;j++)
            _data[i*300+j] = (short)(ddata[i*300 + j]);
        calculateColorsAndPositions();
        if (isVisible())
            updateGL();
*/
    }
    void Q3DCameraWidget::setData(short* ddata)
    {
        if (!_warningWritten)
        {
            _warningWritten = true;
            cout << "Info : 3D plotter disabled. requires more work so that less than 300 slices per pixel can be loaded" << endl;
            cout << "Contact Etienne (etienne.lyard@unige.ch) for more information." << endl;
        }
            /*        for (int i=0;i<1440;i++)
            for (int j=0;j<300;j++)
                _data[i*300+j] = ddata[i* 300 + j];
        calculateColorsAndPositions();
        if (isVisible())
            updateGL();
*/
    }
    void Q3DCameraWidget::drawCameraBody()
    {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

        GLfloat color[4] = {0.8f, 1.f, 1.f, 1.f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0f);
        gluCylinder( gluNewQuadric(),
                        0.62,
                        0.62,
                        1.83,
                        30,
                        2 );
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );



    }

    void Q3DCameraWidget::initializeGL()
    {
        qglClearColor(QColor(25,25,38));

        glShadeModel(GL_SMOOTH);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_LIGHTING);
//        glEnable(GL_LIGHTING);
//        glEnable(GL_LIGHT0);
//        glEnable(GL_AUTO_NORMAL);
        glDisable(GL_CULL_FACE);
//        glCullFace(GL_FRONT);

        glEnable(GL_POLYGON_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    void Q3DCameraWidget::resizeGL(int cWidth, int cHeight)
    {
        glViewport(0,0,cWidth, cHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        GLfloat windowRatio = (float)cWidth/(float)cHeight;
        if (windowRatio < 1)
        {
//            windowRatio = 1.0f/windowRatio;
            gluPerspective(40.f, windowRatio, 1, 100);
//            gluOrtho2D(-viewSize, viewSize, -viewSize*windowRatio, viewSize*windowRatio);
            pixelSize = 2*viewSize/(float)cWidth;
            shownSizex = 2*viewSize;
            shownSizey = 2*viewSize*windowRatio;
        }
        else
        {
            gluPerspective(40.f, windowRatio,1, 8);
//            gluOrtho2D(-viewSize*windowRatio, viewSize*windowRatio, -viewSize, viewSize);
            pixelSize = 2*viewSize/(float)cHeight;
            shownSizex = 2*viewSize*windowRatio;
            shownSizey = 2*viewSize;
        }
        glMatrixMode(GL_MODELVIEW);
    }
