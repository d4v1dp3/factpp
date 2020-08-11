#include "QCameraWidget.h"

#include <sstream>
#include <iostream>

#include <QMouseEvent>

using namespace std;

    QCameraWidget::QCameraWidget(QWidget *pparent) : BasicGlCamera(pparent)
    {
        fBold.resize(1440, false);
        fEnable.resize(1440, true);
        lastFace = -1;
        fShowPixelMoveOver = false;
        fShowPatchMoveOver = false;
        fDrawPatch = false;

        CalculatePixelsColor();

   }

    void QCameraWidget::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         glLoadIdentity();

         glTranslatef(0,-0.44,0);
         glTranslatef(-0.1,0,0);
         glRotatef(cameraRotation, 0,0,-1);
         if (cameraRotation == 90)
         {
             glTranslatef(-0.45,-0.45,0);
  //           cout << "correction" << endl;
         }
         if (cameraRotation == -90)
         {
             glTranslatef(0.45,-0.45,0);
         }
         glScalef(1.5, 1.5, 1.0);
         glTranslatef(0,0,-0.5);
         drawCamera(true);
         glTranslatef(0,0,0.1f);

         if (fDrawPatch)
             drawPatches();
         glTranslatef(0,0,0.1f);

         glLineWidth(1.0f);
         glColor3fv(highlightedPixelsCoulour);
         for (vector<int>::iterator it = highlightedPixels.begin(); it!= highlightedPixels.end(); it++)
         {
             drawHexagon(*it, false);
         }

        glLineWidth(1.0f);
        glTranslatef(0,0,0.1f);

        //glColor3f(1.f - pixelsColor[fWhite][0],1.f - pixelsColor[fWhite][1],1.f - pixelsColor[fWhite][2]);
        if (fWhite != -1)
        {
            glColor3f(1.f, 0.f, 0.f);
            drawHexagon(fWhite, false);
        }
        DrawCameraText();

        DrawScale();

//        if (linearButton->isVisible())
//            repaintInterface();
    }
    void QCameraWidget::drawCamera(bool alsoWire)
    {

        if (!pixelColorUpToDate)
            CalculatePixelsColor();
        glLineWidth(1.0);
        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
            glColor3fv(pixelsColor[i]);
            glLoadName(i);
            drawHexagon(i,true);
        }
        if (!alsoWire)
            return;
        glTranslatef(0,0,0.1f);
        glColor3fv(pixelContourColour);//0.0f,0.0f,0.0f);
        for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
        {
            drawHexagon(i, false);
        }
    }
    void QCameraWidget::DrawCameraText()
    {
        if (!fTextEnabled) return;

        glPushMatrix();
        glLoadIdentity();



//        int textSize = (int)(width()*14/600);
//        setFont(QFont("Monospace", textSize));
        qglColor(QColor(25, 22, 12));

        //first let's draw the usual data
        //title
        renderText(-shownSizex/2.f + 0.01f, shownSizey/2.f - fTextSize*pixelSize - 0.01f, 0.f, QString(titleText.c_str()));
        //stats
        ostringstream str;
        str.precision(2);
        str.setf(ios::fixed,ios::floatfield);
        str << "Med " << fmedian;// << unitsText;
        renderText(3, height()-3-4*fTextSize-35, QString(str.str().c_str()));
        str.str("");
        str << "Avg " << fmean;// <<  unitsText;
        renderText(3, height()-3-3*fTextSize-27, QString(str.str().c_str()));
        str.str("");
        str << "RMS " << frms;// <<  unitsText;
        renderText(3, height()-3-2*fTextSize-21, QString(str.str().c_str()));
        str.str("");
        str << "Min " << fmin;// << unitsText;
        renderText(3, height()-3-3, QString(str.str().c_str()));
        str.str("");
        str << "Max " << fmax;// << unitsText;
        renderText(3, height()-3-1*fTextSize-8, QString(str.str().c_str()));
        //then draw the values beside the scale
        //the difficulty here is to write the correct min/max besides the scale
        //it depends whether the actual mean of the data is given by the user
        //or not. the values given by user are fMin and fMax, while the data
        //real min/max are fmin and fmax (I know, quite confusing... sorry about that)
        //so. first let's see what is the span of one pixel
        float min = (fMin < fScaleLimit || fMax < fScaleLimit) ? fmin : fMin;
        float max = (fMin < fScaleLimit || fMax < fScaleLimit) ? fmax : fMax;
//        textSize = (int)(height()*12/600);
 //       setFont(QFont("Monospace", textSize));
        float pixelSpan = (height() - fTextSize - 1)/(max - min);

        //draw the scale values
        float value = min;
        int fontWidth = fTextSize;
        if (fTextSize > 12) fontWidth--;
        if (fTextSize > 10) fontWidth--;
        if (fTextSize > 7) fontWidth--;//else fontWidth -=1;
//        if (fTextSize < 7) fontWidth++;
        for (int i=0;i<11;i++)
        {
            str.str("");
            str << value;
            if (i==0 || i==10)
                str << ' ' << unitsText;
            str << ' ';
            int h = (value - min)*pixelSpan;
            if (logScale && h != 0)
            {
                float fh = h;
                float mult = (max - min)*pixelSpan;
                fh = log10(h*10.f/mult);
                fh *= mult;
                h = (int)fh;
            }
            h = height()-h;
            int w = width() - (width()/50) - fontWidth*str.str().size();
            if (i==0 || i==10) w -= width()/50;
            if (i!=0 && i!=10) h -= fTextSize/2;
            renderText(w, h, QString(str.str().c_str()));
            value += (max - min)/10;
        }

/*
        str.str("");
        str << min << unitsText;
        int fontWidth = textSize;
        if (textSize > 12) fontWidth-=3; else fontWidth -= 2;
        //height of min ?
        int hmin = (min - min)*pixelSpan;
        hmin = height() - hmin;
        renderText(width() - (width()/25) - fontWidth*str.str().size(), hmin, QString(str.str().c_str()));
        str.str("");
        str << max << unitsText;
        int hmax = (max - min)*pixelSpan;
        hmax = height() - hmax;
        renderText(width() - (width()/25) - fontWidth*str.str().size(), hmax, QString(str.str().c_str()));
*/
        glPopMatrix();

//        textSize = (int)(600*14/600);
//        setFont(QFont("Times", textSize));
    }
    void QCameraWidget::drawPatches()
    {
        glLineWidth(3.0f);
        glColor3fv(patchesCoulour);
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
         glTranslatef(0,0,0.1f);

         glColor3fv(highlightedPatchesCoulour);
         glBegin(GL_LINES);
         for (vector<int>::iterator it=highlightedPatches.begin(); it!= highlightedPatches.end(); it++)
         {
             for (unsigned int j=0;j<patchesIndices[*it].size();j++)
             {
                 glVertex2fv(verticesList[patchesIndices[*it][j].first]);
                 glVertex2fv(verticesList[patchesIndices[*it][j].second]);
             }
         }
         glEnd();
         if (fWhitePatch != -1)
         {
             glTranslatef(0,0,0.01);
             glColor3f(1.f, 0.6f, 0.f);//patchColour);//[0],patchColour[1],patchColour[2]);//0.5f, 0.5f, 0.3f);
             glBegin(GL_LINES);
             for (unsigned int j=0;j<patchesIndices[fWhitePatch].size();j++)
             {
                 glVertex2fv(verticesList[patchesIndices[fWhitePatch][j].first]);
                 glVertex2fv(verticesList[patchesIndices[fWhitePatch][j].second]);
             }
             glEnd();
         }

    }
    void QCameraWidget::Reset()
    {
        fBold.assign(1440, false);
    }

    void QCameraWidget::mousePressEvent(QMouseEvent *cEvent)
    {
        if (cEvent->pos().x() > width()-(width()/50.f))
        {
            toggleInterfaceDisplay();
            return;
        }
        int face = PixelAtPosition(cEvent->pos());
//        cout << face << endl;
        if (face != -1) {
            fWhite = face;
            fWhitePatch = pixelsPatch[fWhite];
 //           CalculatePatchColor();
            emit signalCurrentPixel(face);
            }
        else
        {
            fWhite = -1;
            fWhitePatch = -1;
        }
        updateGL();
   }
    void QCameraWidget::mouseMoveEvent(QMouseEvent* cEvent)
    {
        int face = PixelAtPosition(cEvent->pos());
        if (face != -1 && lastFace != face) {
            emit signalPixelMoveOver(face);
        }
        if (lastFace != face)
        {
            if (fShowPixelMoveOver)
                fWhite = face;

            if (fShowPatchMoveOver)
                fWhitePatch = face != -1 ? pixelsPatch[face] : -1;
        }

        if (fShowPixelMoveOver || fShowPatchMoveOver)
            if (lastFace != face && isVisible())
                updateGL();

        lastFace = face;
    }
    void QCameraWidget::mouseDoubleClickEvent(QMouseEvent* cEvent)
    {
        int face = PixelAtPosition(cEvent->pos());
        if (face != -1) {
 //           cout << "Event !" << endl;
            fWhite = face;
             fWhitePatch = pixelsPatch[fWhite];
 //          highlightPixel(face);
 //           highlightPatch(fWhitePatch);
 //           CalculatePatchColor();
            emit signalPixelDoubleClick(face);
       }
        else
        {
            fWhite = -1;
            fWhitePatch = -1;
 //           clearHighlightedPixels();
 //           clearHighlightedPatches();
        }
        updateGL();

    }
    void QCameraWidget::ShowPixelCursor(bool on)
    {
        fShowPixelMoveOver = on;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void QCameraWidget::ShowPatchCursor(bool on)
    {
        fShowPatchMoveOver = on;
        if (isVisible() && autoRefresh)
            updateGL();
    }
    void QCameraWidget::SetEnable(int idx, bool b)
    {
         fEnable[idx] = b;
     }

     double QCameraWidget::GetData(int idx)
     {
         return fData[idx];
     }
     const char* QCameraWidget::GetName()
     {
         return "QCameraWidget";
     }
     char *QCameraWidget::GetObjectInfo(int px, int py)
     {

         static stringstream stream;
         static string str;
         const int pixel = this->PixelAtPosition(QPoint(px, py));
         if (pixel >= 0)
         {
             stream << "Pixel=" << pixel << "   Data=" << fData[pixel] << '\0';
         }
         str = stream.str();
         return const_cast<char*>(str.c_str());
     }
     void QCameraWidget::CalculatePixelsColor()
     {
         double dmin = fData[0];
          double dmax = fData[0];
          for (int ii=0;ii<ACTUAL_NUM_PIXELS;ii++)
          {
              if (finite(fData[ii]))
              {
                  dmin = dmax = fData[ii];
                  break;
              }
          }
          if (fMin < fScaleLimit || fMax < fScaleLimit)
          {
              for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
              {
                  if (!finite(fData[i])) continue;
                  if (!fEnable[i]) continue;
                  if (fData[i] > dmax) dmax = fData[i];
                  if (fData[i] < dmin) dmin = fData[i];
              }
          }
          if (fMin > fScaleLimit) dmin = fMin;
          if (fMax > fScaleLimit) dmax = fMax;
//          cout << "min: " << dmin << " max: " << dmax << " fMin: " << fMin << " fMax: " << fMax << endl;
          float color;
          for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
           {
              if (!fEnable[i])
              {
//                  cout << "not enabled !" << i << endl;
                  pixelsColor[i][0] = 0.1f;
                  pixelsColor[i][1] = 0.1f;
                  pixelsColor[i][2] = 0.15f;
                  continue;
              }
              if (!finite(fData[i]))
              {
//                  cout << "not enabled !" << i << endl;
                  pixelsColor[i][0] = 0.9f;
                  pixelsColor[i][1] = 0.0f;
                  pixelsColor[i][2] = 0.9f;
                  continue;
              }
              if (fData[i] < dmin)
               {
                   pixelsColor[i][0] = tooLowValueCoulour[0];
                   pixelsColor[i][1] = tooLowValueCoulour[1];
                   pixelsColor[i][2] = tooLowValueCoulour[2];
                   continue;
               }
               if (fData[i] > dmax)
               {
                   pixelsColor[i][0] = tooHighValueCoulour[0];
                   pixelsColor[i][1] = tooHighValueCoulour[1];
                   pixelsColor[i][2] = tooHighValueCoulour[2];
                   continue;
               }
               color = float((fData[i]-dmin)/(dmax-dmin));
               if (logScale)
               {
                   color *= 9;
                   color += 1;
                   color = log10(color);
               }

               int index = 0;
               while (ss[index] < color && index < 4)
                   index++;
               index--;
               if (index < 0) index = 0;
               float weight0 = (color-ss[index]) / (ss[index+1]-ss[index]);
               if (weight0 > 1.0f) weight0 = 1.0f;
               if (weight0 < 0.0f) weight0 = 0.0f;
               float weight1 = 1.0f-weight0;
               pixelsColor[i][0] = weight1*rr[index] + weight0*rr[index+1];
               pixelsColor[i][1] = weight1*gg[index] + weight0*gg[index+1];
               pixelsColor[i][2] = weight1*bb[index] + weight0*bb[index+1];
          }
          CalculatePatchColor();
          UpdateText();
          pixelColorUpToDate = true;
     }
     void QCameraWidget::CalculatePatchColor()
     {
         return;
         //calculate the patch contour color. let's use the anti-colour of the pixels
         GLfloat averagePatchColour[3] = {0.0f,0.0f,0.0f};
         for (int i=0;i<9;i++)
             for (int j=0;j<3;j++)
                 averagePatchColour[j] += pixelsColor[softwareMapping[fWhitePatch*9+i]][j];
         for (int j=0;j<3;j++)
             averagePatchColour[j] /= 9;
         for (int j=0;j<3;j++)
             patchColour[j] = 1.0f - averagePatchColour[j];
     }
     void QCameraWidget::SetData(const valarray<double> &ddata)
     {
//             fData = ddata;
         for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
             fData[i] = ddata[i];
         pixelColorUpToDate = false;
         if (isVisible() && autoRefresh)
             updateGL();
     }

void QCameraWidget::SetData(const valarray<float> &ddata)
     {
//             fData = ddata;
         for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
             fData[i] = ddata[i];
         pixelColorUpToDate = false;
         if (isVisible() && autoRefresh)
             updateGL();
     }


     void QCameraWidget::highlightPixel(int idx, bool highlight)
     {
         if (idx < 0 || idx >= ACTUAL_NUM_PIXELS)
         {
           cout << "Error: requested pixel highlight out of bounds" << endl;
           return;
         }

         const vector<int>::iterator v = ::find(highlightedPixels.begin(), highlightedPixels.end(), idx);
         if (highlight)
         {
             if (v==highlightedPixels.end())
                 highlightedPixels.push_back(idx);
         }
         else
         {
             if (v!=highlightedPixels.end())
                 highlightedPixels.erase(v);
         }

         if (isVisible() && autoRefresh)
             updateGL();
     }
     void QCameraWidget::highlightPatch(int idx, bool highlight)
     {
         if (idx < 0 || idx >= NTMARK)
         {
             cout << "Error: requested patch highlight out of bounds" << endl;
             return;
         }

         const vector<int>::iterator v = ::find(highlightedPatches.begin(), highlightedPatches.end(), idx);
         if (highlight)
         {
             if (v==highlightedPatches.end())
                 highlightedPatches.push_back(idx);
         }
         else
         {
             if (v!=highlightedPatches.end())
                 highlightedPatches.erase(v);
         }

         if (isVisible() && autoRefresh)
             updateGL();

     }
     void QCameraWidget::clearHighlightedPatches()
     {
         highlightedPatches.clear();
         if (isVisible() && autoRefresh)
             updateGL();
     }
     void QCameraWidget::clearHighlightedPixels()
     {
         highlightedPixels.clear();
         if (isVisible() && autoRefresh)
             updateGL();
     }




