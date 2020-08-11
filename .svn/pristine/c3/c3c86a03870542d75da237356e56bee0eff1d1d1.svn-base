/*
 * QtGl.cpp
 *
 *  Created on: Jul 19, 2011
 *      Author: lyard
 *
 ******
 */
#include <math.h>
#include <fstream>

#include <boost/date_time/local_time/local_time.hpp>

#include "RawEventsViewer.h"

#include <QFileDialog>
#include <QMouseEvent>

#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <qwt_plot_zoomer.h>
#pragma GCC diagnostic pop

#include "src/Configuration.h"
#include "externals/factfits.h"


using namespace std;

#undef ACTUAL_NUM_PIXELS
#define ACTUAL_NUM_PIXELS 1440

#ifndef HAVE_QT4 // required with not qwt5-qt4 (assuming that this is the default for not Qt4)
#define setMajPen setMajorPen
#endif

//bounding box for diplaying the impulse curve
float bboxMin[2] = {-0.8,-0.9};
float bboxMax[2] = {0.8,-0.3};
/************************************************************
 * CALC BLUR COLOR if in blur display mode, calculate the interpolated
 * colour for a given vertex
 ************************************************************/
void RawDataViewer::calcBlurColor(int pixel,  int vertex)
{
    GLfloat color[3];
    int first, second;
    first = vertex-1;
    second = vertex;
    if (first < 0)
        first = 5;

    first = neighbors[pixel][first];
    second = neighbors[pixel][second];
//    cout << pixel << " " << vertex << " " << "first: " << first << " second: " << second << endl;
    for (int i=0;i<3;i++)
        color[i] = pixelsColor[pixel][i];
    float divide = 1;
    if (first != -1)
    {
        divide++;
        for (int i=0;i<3;i++)
            color[i] += pixelsColor[first][i];
    }
    if (second != -1)
    {
        divide++;
        for (int i=0;i<3;i++)
            color[i] += pixelsColor[second][i];
    }
    for (int i=0;i<3;i++)
        color[i] /= divide;

//    cout << color[0] << " " << color[1] << " " << color[2] << endl;

    glColor3fv(color);
}
void RawDataViewer::calcMidBlurColor(int pixel, int vertex)
{
    GLfloat color[3];
    int first;
    first = vertex-1;
    if (first < 0)
        first = 5;
    first = neighbors[pixel][first];
    for (int i=0;i<3;i++)
        color[i] = pixelsColor[pixel][i];
    float divide = 1;
    if (first != -1)
    {
        divide++;
        for (int i=0;i<3;i++)
            color[i] += pixelsColor[first][i];
    }
    for (int i=0;i<3;i++)
        color[i] /= divide;
    glColor3fv(color);
}
/************************************************************
 * DRAW BLURRY HEXAGON. draws a solid hexagon, with interpolated colours
 ************************************************************/
void RawDataViewer::drawBlurryHexagon(int index)
{

//per-pixel mesh
    GLfloat color[3];
    for (int i=0;i<3;i++)
        color[i] = pixelsColor[index][i];
    glBegin(GL_TRIANGLES);
    calcBlurColor(index, 0);
    glVertex2fv(verticesList[verticesIndices[index][0]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 1);
    glVertex2fv(verticesList[verticesIndices[index][1]]);

    glVertex2fv(verticesList[verticesIndices[index][1]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 2);
    glVertex2fv(verticesList[verticesIndices[index][2]]);

    glVertex2fv(verticesList[verticesIndices[index][2]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 3);
    glVertex2fv(verticesList[verticesIndices[index][3]]);

    glVertex2fv(verticesList[verticesIndices[index][3]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 4);
    glVertex2fv(verticesList[verticesIndices[index][4]]);

    glVertex2fv(verticesList[verticesIndices[index][4]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 5);
    glVertex2fv(verticesList[verticesIndices[index][5]]);

    glVertex2fv(verticesList[verticesIndices[index][5]]);
    glColor3fv(color);
    glVertex2fv(pixelsCoords[index]);

    calcBlurColor(index, 0);
    glVertex2fv(verticesList[verticesIndices[index][0]]);
    glEnd();

    return;
}

/************************************************************
 * DRAW CAMERA draws all the camera pixels
 ************************************************************/
void RawDataViewer::drawCamera(bool alsoWire)
{
    glLoadIdentity();
    if (!drawImpulse)
    {
        glTranslatef(0,-0.44,0);
        glRotatef(cameraRotation, 0,0,-1);
        if (cameraRotation == 90)
        {
            glTranslatef(-0.45,-0.45,0);
        }
        if (cameraRotation == -90)
        {
            glTranslatef(0.45,-0.45,0);
        }
        glScalef(1.5,1.5,1);
    }
    else
    {
        glRotatef(cameraRotation, 0,0,-1);
          if (cameraRotation == 90)
        {
            glTranslatef(-0.45/1.5,-0.45/1.5,0);
        }
        if (cameraRotation == -90)
        {
            glTranslatef(0.45/1.5,-0.45/1.5,0);
        }
  }
    glColor3f(0.5,0.5,0.5);
    glLineWidth(1.0);
    float color;

    for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
    {
        if (!nRoi)
          color = (float)(i)/(float)(ACTUAL_NUM_PIXELS);
        else

//        if (_softwareOrdering)
//            color = float(eventData[nRoi*i + whichSlice] + (VALUES_SPAN/2))/(float)(VALUES_SPAN-1);
//        else
            color = i<nPixels ? float(eventData[nRoi*hardwareMapping[i] + whichSlice]+(VALUES_SPAN/2))/(float)(VALUES_SPAN-1) : 0;
        if (logScale)
        {
            color *= 9;
            color += 1;
            color = log10(color);
        }

        if (color < ss[0])
        {
            pixelsColor[i][0] = tooLowValueCoulour[0];
            pixelsColor[i][1] = tooLowValueCoulour[1];
            pixelsColor[i][2] = tooLowValueCoulour[2];
            continue;
        }
        if (color > ss[4])
        {
            pixelsColor[i][0] = tooHighValueCoulour[0];
            pixelsColor[i][1] = tooHighValueCoulour[1];
            pixelsColor[i][2] = tooHighValueCoulour[2];
            continue;
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

    for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
    {

        glColor3fv(pixelsColor[i]);
        glLoadName(i);
if (drawBlur)
    drawBlurryHexagon(i);
else
    drawHexagon(i,true);

    }
    if (!alsoWire)
        return;
    glTranslatef(0,0,0.1f);
    glColor3f(0.0f,0.0f,0.0f);
    for (int i=0;i<ACTUAL_NUM_PIXELS;i++)
    {

        drawHexagon(i, false);
    }

}

/************************************************************
 * DRAW PIXEL CURVE. draws the raw impulse curve of the currently selected pixel
 ************************************************************/
void RawDataViewer::drawPixelCurve()
{
    float xRange = bboxMax[0] - bboxMin[0];
    float yRange = bboxMax[1] - bboxMin[1];

    glBegin(GL_LINES);
    glLineWidth(1.0f);
    glColor3f(0.0,0.0,0.0);
    glVertex2f(bboxMin[0], bboxMin[1]);
    glVertex2f(bboxMax[0], bboxMin[1]);
    glVertex2f(bboxMin[0], bboxMin[1]);
    glVertex2f(bboxMin[0], bboxMax[1]);
    glVertex2f(bboxMin[0], (bboxMin[1]+bboxMax[1])/2.0f);
    glVertex2f(bboxMax[0], (bboxMin[1]+bboxMax[1])/2.0f);
    glVertex2f(bboxMin[0] + xRange*nRoi/(float)(nRoi+nRoiTM),
               bboxMin[1]);
    glVertex2f(bboxMin[0] + xRange*nRoi/(float)(nRoi+nRoiTM),
               bboxMax[1]);
   glEnd();
    glTranslatef(0,0,0.1f);
    if (!nRoi)
          return;
     glBegin(GL_LINES);
    glColor3f(1.0f,1.0f,0.0f);
    float divideMe = (float)(VALUES_SPAN-1);
    float plusMe = (float)(VALUES_SPAN)/2;
    if (divideMe <= 0)
        divideMe = 1;

    /*
    if (drawCalibrationLoaded)
        plusMe += 0;//VALUES_SPAN/2;
    if (drawCalibrationLoaded && calibrationLoaded)
    {
        divideMe /=2;
        plusMe = 0 ;///=2;
        }*/

//    int mapping = _softwareOrdering ? selectedPixel : hardwareMapping[selectedPixel];
    int mapping = hardwareMapping[selectedPixel];
    const int hw = mapping;
    const PixelMapEntry& mapEntry = fPixelMap.index(selectedPixel);
    const int pixelIdInPatch = mapEntry.pixel();
    const int patchId = mapEntry.patch();
    const int boardId = mapEntry.board();
    const int crateId = mapEntry.crate();

    if (selectedPixel != -1)
    {
    for (int i=0;i<nRoi-1;i++)
    {
        float d1 = eventData[nRoi*hw + i]+plusMe;
        float d2 = eventData[nRoi*hw + i+1]+plusMe;
        if (!finite(d1)) d1 = 20000;
        if (!finite(d2)) d2 = 20000;
        glVertex2f(bboxMin[0] + xRange*i/(float)(nRoi+nRoiTM),
                   bboxMin[1] + yRange*(d1) /divideMe);
        glVertex2f(bboxMin[0] + xRange*(i+1)/(float)(nRoi+nRoiTM),
                   bboxMin[1] + yRange*(d2) /divideMe);
    }
    glEnd();

    glColor3f(0.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    if (pixelIdInPatch == 8)//this channel has a time marker
    {

        for (int i=0;i<nRoiTM-1;i++)
        {
            float d1 = eventData[nRoi*1440 + nRoiTM*(40*crateId + 4*boardId + patchId) + i] + plusMe;
            float d2 = eventData[nRoi*1440 + nRoiTM*(40*crateId + 4*boardId + patchId) + i+1] + plusMe;
            if (!finite(d1)) d1 = 20000;
            if (!finite(d2)) d2 = 20000;
            glVertex2f(bboxMin[0] + xRange*(i+nRoi)/(float)(nRoi+nRoiTM),
                       bboxMin[1] + yRange*(d1)/divideMe);
            glVertex2f(bboxMin[0] + xRange*(i+1+nRoi)/(float)(nRoi+nRoiTM),
                       bboxMin[1] + yRange*(d2) / divideMe);
        }
    }

    }
    glEnd();
    glTranslatef(0,0,0.1f);
    glBegin(GL_LINES);
    glColor3f(1.0,0.0,0.0);
    glVertex2f(bboxMin[0] + xRange*whichSlice/(float)(nRoi+nRoiTM),
               bboxMin[1]);
    glVertex2f(bboxMin[0] + xRange*whichSlice/(float)(nRoi+nRoiTM),
               bboxMax[1]);

    glEnd();

}
/************************************************************
 * CONSTRUCTOR.
 ************************************************************/
RawDataViewer::RawDataViewer(QWidget *cParent) : BasicGlCamera(cParent), RMSvalues(1440), Meanvalues(1440), Maxvalues(1440), PosOfMaxvalues(1440), VALUES_SPAN(4096)

{

    whichSlice = 0;

    nRoi = 0;
    nRoiTM = 0;
    offSetRoi = 0;
    eventNum = 0;
    rowNum = -1;
    eventStep = 1;
    selectedPixel = 393;
    inputFile = NULL;
    drawPatch = false;
    drawImpulse = true;
    drawBlur = false;
    loopCurrentEvent = false;
    fIsDrsCalibration = false;
    SetAutoRefresh(true);
    runType = "unkown";


}

void RawDataViewer::assignPixelMapFile(const string& map)
{
    PixelMap mypMap;
    if (map.empty())
    {
        if (!mypMap.Read("FACTmap111030.txt"))
        {
            if (!mypMap.Read("/swdev_nfs/FACT++/FACTmap111030.txt"))
            {
                if (!mypMap.Read("./FACTmap111030.txt"))
                {
                    cerr << "ERROR - Problems reading FACTmap111030.txt" << endl;
                    exit(-1);
                }
            }
        }
    }
    else
    {
        if (!mypMap.Read(map))
        {
            cerr << "ERROR - Problems reading mapping file '" << map << "'" << endl;
            exit(-1);
        }
    }

    assignPixelMap(mypMap);

    for (int i=0;i<160;i++)
    {
        const float color[3] = { 0.5, 0.5, 0.3 };

        for (int j=0;j<3;j++)
            patchesColor[i][j] = color[j];
    }
    //fZeroArray = NULL;

    _softwareOrdering = false;
}
/************************************************************
 *  DESTRUCTOR
 ************************************************************/
RawDataViewer::~RawDataViewer()
{
    if (inputFile != NULL)
    {
        inputFile->close();
        delete inputFile;
    }
    //if (fZeroArray != NULL)
    //    delete[] fZeroArray;
}
/*
void RawDataViewer::allocateZeroArray()
{
    if (fZeroArray == NULL)
    {
        fZeroArray = new char[8192];
    }
}
*/
/************************************************************
 * PAINT GL. main drawing function.
 ************************************************************/
void RawDataViewer::paintGL()
{
    //Should not be required, but apparently it helps when piping it through X forwarding
    glFinish();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0,0,-0.5);

    if (drawBlur)
    {
        glShadeModel(GL_SMOOTH);
        drawCamera(false);
    }
    else
    {
        glShadeModel(GL_FLAT);
        drawCamera(true);
    }
    glTranslatef(0,0,0.1f);
    if (drawPatch)
        drawPatches();
    glTranslatef(0,0,0.1f);
   if (!drawBlur && (selectedPixel != -1))
   {
        glLineWidth(1.0f);
        glColor3f(1.0,1.0,1.0);
        drawHexagon(selectedPixel, false);
   }
   glTranslatef(0,0,0.1f);
   if (drawImpulse)
   {
    //   glRotatef(cameraRotation, 0,0,1);
       glLoadIdentity();
       glLineWidth(2.0);
       drawPixelCurve();
   }
   glTranslatef(0,0,0.1f);
   DrawScale();
}

/************************************************************
 * MOUSE PRESS EVENT. mouse click handler.
 ************************************************************/
void RawDataViewer::mousePressEvent(QMouseEvent *cEvent)
{
    if (cEvent->pos().x() > width()-(width()/50.f))
    {
        toggleInterfaceDisplay();
        return;
    }
    lastPos = cEvent->pos();
    if (setCorrectSlice(cEvent))
        return;
    int face = PixelAtPosition(cEvent->pos());

        selectedPixel = face;
        emit signalCurrentPixel(face);

    updateGL();
}

/************************************************************
 * SET CORRECT SLICE. if displayed, figures out if the graph was
 * clicked, and if so, which slice should be displayed
 ************************************************************/
bool RawDataViewer::setCorrectSlice(QMouseEvent* cEvent)
{
    if (!drawImpulse)
        return false;
    float cx = (float)cEvent->x() * pixelSize - shownSizex/2;
    float cy = ((float)height()-(float)cEvent->y())*pixelSize - shownSizey/2;
    if (cx < bboxMin[0] ||
        cx > bboxMax[0] ||
        cy < bboxMin[1] ||
        cy > bboxMax[1])
        return false;
    whichSlice = (cx - bboxMin[0])*(nRoi+nRoiTM)/(bboxMax[0] - bboxMin[0]);
    if (whichSlice >= nRoi)
        whichSlice = nRoi-1;
    emit signalCurrentSlice(whichSlice);
    return true;
}

/************************************************************
 * MOUSE MOVE EVENT. used to track the dragging of slices display
 ************************************************************/
void RawDataViewer::mouseMoveEvent(QMouseEvent *cEvent)
{
    if (cEvent->buttons() & Qt::LeftButton) {
        setCorrectSlice(cEvent);
        updateGL();
    } else if (cEvent->buttons() & Qt::RightButton) {
        updateGL();
    }
    lastPos = cEvent->pos();
}

/************************************************************
 * MOUSE DOUBLE CLICK EVENT. used to select pixels
 ************************************************************/
void RawDataViewer::mouseDoubleClickEvent(QMouseEvent *cEvent)
{
    int face = PixelAtPosition(cEvent->pos());
    if (face != -1) {
        selectedPixel = face;
        emit signalCurrentPixel(face);
        updateGL();
        }
}

/************************************************************
 * OPEN FILE. opens a new fits file
 ************************************************************/
void RawDataViewer::openFile(string file, bool reopen)
{
    if (inputFile)
    {
        inputFile->close();
        delete inputFile;
    }
    try {
        inputFile = reopen && fIsDrsCalibration ? new zfits(file, "Events") :  new factfits(file, "Events");
    }
    catch (std::runtime_error e)
    {
        cout << "Something went wrong while loading fits. Aborting: " << e.what() << endl;
        return;
    }
    if (!*inputFile)
    {
        delete inputFile;
        inputFile = NULL;
        return;
    }
    vector<string> entriesToCheck;
    if (inputFile->IsCompressedFITS())
        entriesToCheck.push_back("ZNAXIS2");
    else
        entriesToCheck.push_back("NAXIS2");
    entriesToCheck.push_back("NROI");
    entriesToCheck.push_back("REVISION");
    entriesToCheck.push_back("RUNID");
    entriesToCheck.push_back("NBOARD");
    entriesToCheck.push_back("NPIX");
    entriesToCheck.push_back("NROITM");
    entriesToCheck.push_back("TIMESYS");
    entriesToCheck.push_back("DATE");
    entriesToCheck.push_back("NIGHT");
    entriesToCheck.push_back("CAMERA");
    entriesToCheck.push_back("DAQ");
    entriesToCheck.push_back("TSTART");
    entriesToCheck.push_back("TSTOP");


    for (vector<string>::const_iterator it=entriesToCheck.begin(); it != entriesToCheck.end(); it++)
    {
        try {
        if (!inputFile->HasKey(*it)){
            cout << "Warning: header keyword " << *it << " missing." << endl;
            }
        }
        catch (std::runtime_error e)
        {
            cout << e.what() << endl;
            return;
        }
    }

    nRows = 0;
    if (inputFile->IsCompressedFITS())
        nRows = inputFile->HasKey("ZNAXIS2") ? inputFile->GetInt("ZNAXIS2") : 0;
    else
        nRows = inputFile->HasKey("NAXIS2") ? inputFile->GetInt("NAXIS2") : 0;
    nRoi =           inputFile->HasKey("NROI") ?  inputFile->GetInt("NROI") : 0;
    runNumber =      inputFile->HasKey("RUNID") ?  inputFile->GetInt("RUNID") : -1;
    nTM =            inputFile->HasKey("NTMARK") ? inputFile->GetInt("NTMARK") : 0;

    runType = "unknown";
    if (inputFile->HasKey("RUNTYPE"))
    {
        runType = inputFile->GetStr("RUNTYPE");
        if (runType == "")
            runType = "unknown";
    }
    firstDataTime =  inputFile->HasKey("TSTART") ? inputFile->GetInt("TSTART") : -1;
    lastDataTime =   inputFile->HasKey("TSTOP") ? inputFile->GetInt("TSTOP"): -1;
    nRoiTM =         inputFile->HasKey("NROITM") ? inputFile->GetInt("NROITM") : 0;
    revision =       inputFile->HasKey("REVISION") ? inputFile->GetInt("REVISION") : -1;
    builderVersion = inputFile->HasKey("BLDVER") ? inputFile->GetInt("BLDVER") : -1;
    nBoards =        inputFile->HasKey("NBOARD") ? inputFile->GetInt("NBOARD") : 0;
    nPixels =        inputFile->HasKey("NPIX") ?  inputFile->GetInt("NPIX") : 0;
    timeSystem =     inputFile->HasKey("TIMESYS") ? inputFile->GetStr("TIMESYS") : "";
    creationDate =   inputFile->HasKey("DATE") ? inputFile->GetStr("DATE") : "";
    nightInt =       inputFile->HasKey("NIGHT") ? inputFile->GetInt("NIGHT") : 0;
    camera =         inputFile->HasKey("CAMERA") ? inputFile->GetStr("CAMERA") : "";
    daq =            inputFile->HasKey("DAQ") ? inputFile->GetStr("DAQ") : "";
    adcCount =       inputFile->HasKey("ADCRANGE") ? inputFile->GetFloat("ADCRANGE") : 2000;
    if (nPixels == 0)
    {
        cout << "could not read num pixels from fits header. Assuming 1440 (FACT)." << endl;
        nPixels = 1440;
    }
    if (nPixels > 1440)
        cout << "More than 1440 pixels not supported." << endl;
    if (nPixels==1440 && nRoi!=0)
        cout << "Time marker not suppoerted with number of pixel not euqal 1440." << endl;

    if (nRoi == 0 && !inputFile->HasKey("NROI"))
    {//let's try to figure out the roi from the column's format
        const fits::Table::Columns& cols = inputFile->GetColumns();
        if (cols.find("Data") == cols.end())
        {
            cout << "ERROR: Column \"Data\" could not be found. abortin load." << endl;
            return;
        }
        const fits::Table::Columns::const_iterator col = cols.find("Data");
        if (col->second.type != 'I')
        {
            cout << "ERROR: Data Column has type " << col->second.type << " while viewer expects I" << endl;
            return;
        }
        if (col->second.num % nPixels != 0)
        {
            cout << "ERROR: Num pixels (" << nPixels << ") does not match Data length (" << col->second.num << "). Aborting" << endl;
            return;
        }
        nRoi = col->second.num/nPixels;
        cout << "Estimate num samples per pixels to be " << nRoi;
        _softwareOrdering = true;
    }
    else
        _softwareOrdering = inputFile->Get<string>("ISMC", "F")=="T";

    if (inputFile->HasKey("OFFSET"))
        offSetRoi = inputFile->GetInt("OFFSET");

    nbOk = 0;//inputFile->GetInt("NBEVTOK");
    nbRej = 0;//inputFile->GetInt("NBEVTREJ");
    nbBad = 0;//inputFile->GetInt("NBEVTBAD");

    eventNum = 1;

    eventData.resize(1440*nRoi + 160*nRoiTM);
    rawEventData.resize(1440*nRoi + 160*nRoiTM);
    waveLetArray.resize(1024*1440);
    try
    {
        inputFile->SetPtrAddress("Data", rawEventData.data());
        if (inputFile->HasColumn("EventNum"))
            inputFile->SetPtrAddress("EventNum", &eventNum);
        else
            cout << "Warning: could not find column \"EventNum\"" << endl;
        if (inputFile->HasColumn("TriggerType"))
            inputFile->SetPtrAddress("TriggerType", &triggerType);
        else
            cout << "Warning: could not find column \"TriggerType\"" << endl;
        if (inputFile->HasColumn("SoftTrig"))
            inputFile->SetPtrAddress("SoftTrig", &softTrig);
        else
            cout << "Warning: could not find column \"SoftTrig\"" << endl;
        if (inputFile->HasColumn("BoardTime"))
            inputFile->SetPtrAddress("BoardTime", boardTime);
        else
            cout << "Warning: could not find column \"BoardTime\"" << endl;
        if (inputFile->HasColumn("StartCellData"))
            inputFile->SetPtrAddress("StartCellData", startPix);
        else
            cout << "Warning: could not find column \"StartCellData\"" << endl;
        if (inputFile->HasColumn("StartCellTimeMarker"))
            inputFile->SetPtrAddress("StartCellTimeMarker", startTM);
        else
            cout << "Warning: could not find column \"StartCellTimeMarker\"" << endl;
        if (inputFile->HasColumn("TimeMarker"))
            inputFile->SetPtrAddress("TimeMarker", rawEventData.data()+1440*nRoi);
        else
            cout << "Warning: could not find column \"TimeMarker\"" << endl;
    }
    catch (const runtime_error &e)
    {
        cout << e.what() << endl;
        cout << "Loading aborted." << endl;

        nRoi = nRows = 0;

        return;
    }

    try
    {
        pcTime[0] = pcTime[1] = 0;
        if (inputFile->HasColumn("UnixTimeUTC"))
            inputFile->SetPtrAddress("UnixTimeUTC", pcTime);
    }
    catch (const runtime_error&)
    {
            try
        {
            if (inputFile->HasColumn("PCTime"))
                inputFile->SetPtrAddress("PCTime", pcTime);
            else
                cout << "Warning: could not find column \"UnixTimeUTC\" nor \"PCTime\"" << endl;

        }
        catch (const runtime_error&)
        {

        }
    }


    if (!reopen)
    {
        int backupStep = eventStep;
        rowNum = -1;
        eventStep = 1;
        plusEvent();
        eventStep = backupStep;
    }
    else
        readEvent();

    emit newFileLoaded();
    emit signalCurrentPixel(selectedPixel);
}

void RawDataViewer::openCalibFile(string& file)
{
    //calibrationLoaded = false;
    string msg;
    try
    {
        msg = fDrsCalib.ReadFitsImp(file);
        if (msg.empty())
        {
            emit newFileLoaded();
            updateGL();
            return;
        }
    }
    catch (const runtime_error &e)
    {
        msg = string("Something went wrong while loading Drs Calib: ") + e.what() + string(".. Aborting file loading");
    }
    cerr << msg << endl;
    fDrsCalib.Clear();
}

template <typename T>
void RawDataViewer::getCalibrationDataForDisplay(const CalibDataTypes/* calibTypes*/,
                                                 const vector<T>& inputData,
                                                 const int roi,
                                                 const int roiTM)
{
    eventData.assign(inputData.begin(), inputData.end());
    nRoi=roi;
    nRoiTM=roiTM;

    long long min, max, mean;
    min = max = inputData[0];
    mean=0;
    for (int i=0;i<1440*roi + 160*roiTM;i++) {
        mean += inputData[i];
        if (inputData[i] > max)
            max = inputData[i];
        if (inputData[i] < min)
            min = inputData[i];
    }
    mean /= 1440*roi + 160*roiTM;
    for (int i=0;i<1440*roi + 160*roiTM;i++)
        eventData[i] -= (float)mean;
    VALUES_SPAN = max - min;
//    cout << VALUES_SPAN << " " << min << " " << max << " " << mean << endl;
//    cout << 1440*roi + 160*roiTM << " " << roi << " " << roiTM << " " << inputData.size() << endl;
}
/************************************************************
 * PLUS EVENT
 ************************************************************/
void RawDataViewer::plusEvent()
{
    eventStepping(true);
}
/************************************************************
 * MINUS EVENT
 ************************************************************/
void RawDataViewer::minusEvent()
{
    eventStepping(false);
}
/************************************************************
 * SET EVENT STEP
 ************************************************************/
void RawDataViewer::setEventStep(int step)
{
    eventStep = step;
}
/************************************************************
 * EVENT STEPPING
 ************************************************************/

void RawDataViewer::ApplyCalibration()
{
    eventData.assign(rawEventData.begin(), rawEventData.end());

    if (fIsDrsCalibration)
    {
        fDrsCalib.Apply(eventData.data(), rawEventData.data(), startPix, nRoi);
        DrsCalibrate::RemoveSpikes3(eventData.data(), nRoi*1440);
        //TODO apply calibration to the Time markers
    }

    vector<float> pixelStatsData(1440*4);
    DrsCalibrate::GetPixelStats(pixelStatsData.data(), eventData.data(), nRoi, 15, nRoiTM>0?5:60);

    for (vector<PixelMapEntry>::const_iterator it=fPixelMap.begin(); it!=fPixelMap.end(); it++)
    {
        //if (it->index>=nPixels)
        //    continue;

        Meanvalues[it->index]     = pixelStatsData[0*1440+it->hw()];
        RMSvalues[it->index]      = pixelStatsData[1*1440+it->hw()];
        Maxvalues[it->index]      = pixelStatsData[2*1440+it->hw()];
        PosOfMaxvalues[it->index] = pixelStatsData[3*1440+it->hw()];
    }

    if (isVisible())
        updateGL();
}

void RawDataViewer::eventStepping(bool plus)
{
    if (plus)
        rowNum += eventStep;
    else
        rowNum -= eventStep;
    if (rowNum >= nRows)
        rowNum -= nRows;
    if (rowNum < 0)
        rowNum += nRows;

    readEvent();
}

void RawDataViewer::readEvent()
{
    if (inputFile == NULL)
        return;
    inputFile->GetRow(rowNum);
    if (_softwareOrdering)
    {//remap pixels data according to hardware id
        if (nRoiTM != 0)
            cout << "Warning: did not expect Time Markers data from Monte-Carlo simulations. These will not be mapped properly." << endl;

        //first copy the data
        const vector<int16_t> tempData(rawEventData);//.begin(), rawEventData.begin()+1440*nRoi);
        rawEventData.assign(rawEventData.size(), 0);
        //copy back the data and re-map it on the fly
        for (int i=0;i<1440;i++)
            if (softwareMapping[i]<nPixels)
                for (int j=0;j<nRoi;j++)
                    rawEventData[i*nRoi + j] = tempData[softwareMapping[i]*nRoi + j];
    }
//    cout << "Getting row " << rowNum << endl;

    ApplyCalibration();

    emit signalCurrentEvent(eventNum);
    emit signalCurrentPixel(selectedPixel);
}

/************************************************************
 * NEXT SLICE. deprec ?
 ************************************************************/
void RawDataViewer::nextSlice()
{
    whichSlice++;
    if (whichSlice >= nRoi)
    {
        whichSlice = 0;
        if (!loopCurrentEvent)
        {
            int backupStep = eventStep;
            eventStep = 1;
            eventStepping(true);
            eventStep = backupStep;
        }
    }
    emit signalCurrentSlice(whichSlice);
    updateGL();
}
void RawDataViewer::previousSlice()
{
    whichSlice--;
    if (whichSlice < 0)
    {
        whichSlice = nRoi-1;
        if (!loopCurrentEvent)
        {
            int backupStep = eventStep;
            eventStep = 1;
            eventStepping(false);
            eventStep = backupStep;
        }
    }
    emit signalCurrentSlice(whichSlice);
    updateGL();
}
void RawDataViewer::setCurrentPixel(int pix)
{
 //   if (pix == -1)
 //       return;
    selectedPixel = pix;
    if (isVisible())
    updateGL();
     emit signalCurrentPixel(pix);
}
/*
void RawDataViewer::computePulsesStatistics()
{
    if (!inputFile)
    {
        cout << "A FITS file must be open in order to complete this operation" << endl;
        return;
    }


//    for (int i=0;i<nRows;i++)//for all events
//    {
//        inputFile->GetRow(rowNum);
//        for (int i=0;i<(1440+160)*nRoi;i++)
//            eventData[i] = (float)rawEventData[i];

//        for (int j=0;j<ACTUAL_NUM_PIXELS;j++)
///        {
    int j = selectedPixel;
    if (j == -1)
        return;
            for (int i=0;i<nRoi;i++)
            {
                aMeas[i] = eventData[j*nRoi+i];// * adcCount;

            }
            for (int i=0;i<nRoi;i++)
            {
                if (i==0)
                    n1mean[i] = aMeas[i+1];
                else
                {
                    if (i==1023)
                        n1mean[i] = aMeas[i-1];
                    else
                        n1mean[i] = (aMeas[i-1]+aMeas[i+1])/2.f;
                }
            }
            //find spike
            for (int i=0;i<nRoi-3;i++)
            {
                const float fract = 0.8f;
                float xx, xp, xpp;
                vCorr[i] = 0;//aMeas[i];
                xx = aMeas[i] - n1mean[i];
                if (xx < -8.f)
                {
                    xp = aMeas[i+1] - n1mean[i+1];
                    xpp = aMeas[i+2] - n1mean[i+2];
                    if ((aMeas[i+2] - (aMeas[i] + aMeas[i+3])/2.f) > 10.f)
                    {
                        vCorr[i+1] = (aMeas[i] + aMeas[i+3])/2.f;
                        vCorr[i+2] = (aMeas[i] + aMeas[i+3])/2.f;
                        i = i+2;
                    }
                    else
                    {
                        if ((xp > -2.*xx*fract) && (xpp < -10.f))
                        {
                            vCorr[i+1] = n1mean[i+1];
                            n1mean[i+2] = aMeas[i+1] - aMeas[i+3]/2.f;
                            i++;
                        }
                    }
                }
            }
            for (int i=0;i<nRoi;i++)
                n1mean[i] = aMeas[i]-n1mean[i];
 //       }
 //   }
}
*/
/************************************************************
 * UICONNECTOR CONSTRUCTOR
 ************************************************************/
UIConnector::UIConnector(QWidget *p)
{
    setupUi(this);
    initHistograms();

    updateSpinnerDisplay = true;
    updating = false;

    timer.setInterval(10.0);
    QObject::connect(&timer, SIGNAL(timeout()),
                     this, SLOT(nextSlicePlease()));

    QButtonGroup &scaleGroup = *new QButtonGroup(p);// = new QButtonGroup(canvas);
    QButtonGroup &animateGroup = *new QButtonGroup(p);// = new QButtonGroup(canvas);

    scaleGroup.addButton(currentPixelScale);
    scaleGroup.addButton(entireCameraScale);

    animateGroup.addButton(playEventsRadio);
    animateGroup.addButton(playSlicesRadio);
    animateGroup.addButton(playPixelsRadio);

    entireCameraScale->setChecked(true);

    RMS_window->enableText(false);
    Mean_window->enableText(false);
    PosOfMax_window->enableText(false);
    Max_window->enableText(false);

 //   RMS_window->ShowPatchCursor(true);

    QObject::connect(GLWindow, SIGNAL(colorPaletteHasChanged()),
                     this, SLOT(on_autoScaleColor_clicked()));
    QObject::connect(GLWindow, SIGNAL(signalCurrentSlice(int)),
                     this, SLOT(currentSliceHasChanged(int)));
    QObject::connect(GLWindow, SIGNAL(signalCurrentEvent(int)),
                     this, SLOT(currentEventHasChanged(int)));
    QObject::connect(GLWindow, SIGNAL(signalCurrentPixel(int)),
                     this, SLOT(pixelChanged(int)));
    QObject::connect(GLWindow, SIGNAL(newFileLoaded()),
                     this, SLOT(newFileLoaded()));

    QObject::connect(RMS_window, SIGNAL(signalCurrentPixel(int)),
                     GLWindow, SLOT(setCurrentPixel(int)));
    QObject::connect(Max_window, SIGNAL(signalCurrentPixel(int)),
                     GLWindow, SLOT(setCurrentPixel(int)));
    QObject::connect(PosOfMax_window, SIGNAL(signalCurrentPixel(int)),
                     GLWindow, SLOT(setCurrentPixel(int)));
    QObject::connect(Mean_window, SIGNAL(signalCurrentPixel(int)),
                     GLWindow, SLOT(setCurrentPixel(int)));



    show();
}
UIConnector::~UIConnector()
{
    grid1->detach();
    grid2->detach();
    grid3->detach();
    grid4->detach();
    grid5->detach();
    grid6->detach();
    boardsTimeHistoItem.detach();
    startCellHistoItem.detach();
    startTimeMarkHistoItem.detach();
    pixelValueCurveItem.detach();
    pixelAverageCurveItem.detach();
    aMeanCurveItem.detach();
    vCorrCurveItem.detach();
    meanCurveItem.detach();
}
void UIConnector::slicesPlusPlus()
{
    GLWindow->nextSlice();
}
void UIConnector::slicesMinusMinus()
{
    GLWindow->previousSlice();
}
void UIConnector::on_calibratedCheckBox_stateChanged(int state)
{
    GLWindow->fIsDrsCalibration = state;

    if (currentCalibFile.empty())
        GLWindow->openFile(currentFile, true);

    GLWindow->ApplyCalibration();

    threeD_Window->setData(GLWindow->eventData.data());

    if (autoScaleColor->isChecked())
        on_autoScaleColor_clicked();
    pixelChanged(GLWindow->selectedPixel);

}
/************************************************************
 * DRAW PATCHES CHECK CHANGE. checkbox handler
 ************************************************************/
void UIConnector::on_drawPatchCheckBox_stateChanged(int state)
{
    GLWindow->drawPatch = state;
    GLWindow->updateGL();
    RMS_window->fDrawPatch = state;
    RMS_window->updateGL();
    Mean_window->fDrawPatch = state;
    Mean_window->updateGL();
    Max_window->fDrawPatch = state;
    Max_window->updateGL();
    PosOfMax_window->fDrawPatch = state;
    PosOfMax_window->updateGL();
}
/************************************************************
 * DRAW IMPULSE CHECK CHANGE. checkbox handler
 ************************************************************/
void UIConnector::on_drawImpulseCheckBox_stateChanged(int state)
{
    GLWindow->drawImpulse = state;
    TriggerOffset_window->drawImpulse = state;
    Gain_window->drawImpulse = state;
    Baseline_window->drawImpulse = state;
    GLWindow->updateGL();
    TriggerOffset_window->updateGL();
    Gain_window->updateGL();
    Baseline_window->updateGL();
}
/************************************************************
 * DRAW BLUR CHECK CHANGE. checkbox handler
 ************************************************************/
void UIConnector::on_drawBlurCheckBox_stateChanged(int state)
{
    GLWindow->drawBlur = state;
    GLWindow->updateGL();
}
void UIConnector::on_loopOverCurrentEventBox_stateChanged(int state)
{
    GLWindow->loopCurrentEvent = state;
}

/************************************************************
 * NEXT SLICE PLEASE
 ************************************************************/
void UIConnector::nextSlicePlease()
{
    if (playEventsRadio->isChecked ())
        GLWindow->eventStepping(true);
    else
        if (playPixelsRadio->isChecked())
            GLWindow->setCurrentPixel((GLWindow->getCurrentPixel()+1)%1440);
        else
            GLWindow->nextSlice();
}

/************************************************************
 * SET VIEWER.
 ************************************************************/
//void UIConnector::setViewer(RawDataViewer* v)
//{
//    viewer = v;
//}
/************************************************************
 * SLICES PER SECOND CHANGED. timing ui handler
 ************************************************************/
void UIConnector::slicesPerSecondChanged(double value)
{
    timer.setInterval(1000.0/value);
}

void UIConnector::on_colorRange0_valueChanged(double value) { GLWindow->ss[0] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_colorRange1_valueChanged(double value) { GLWindow->ss[1] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_colorRange2_valueChanged(double value) { GLWindow->ss[2] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_colorRange3_valueChanged(double value) { GLWindow->ss[3] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_colorRange4_valueChanged(double value) { GLWindow->ss[4] = (float)value; GLWindow->updateGL(); }

void UIConnector::on_redValue0_valueChanged(double value) { GLWindow->rr[0] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_redValue1_valueChanged(double value) { GLWindow->rr[1] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_redValue2_valueChanged(double value) { GLWindow->rr[2] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_redValue3_valueChanged(double value) { GLWindow->rr[3] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_redValue4_valueChanged(double value) { GLWindow->rr[4] = (float)value; GLWindow->updateGL(); }

void UIConnector::on_greenValue0_valueChanged(double value) { GLWindow->gg[0] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_greenValue1_valueChanged(double value) { GLWindow->gg[1] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_greenValue2_valueChanged(double value) { GLWindow->gg[2] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_greenValue3_valueChanged(double value) { GLWindow->gg[3] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_greenValue4_valueChanged(double value) { GLWindow->gg[4] = (float)value; GLWindow->updateGL(); }

void UIConnector::on_blueValue0_valueChanged(double value) { GLWindow->bb[0] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_blueValue1_valueChanged(double value) { GLWindow->bb[1] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_blueValue2_valueChanged(double value) { GLWindow->bb[2] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_blueValue3_valueChanged(double value) { GLWindow->bb[3] = (float)value; GLWindow->updateGL(); }
void UIConnector::on_blueValue4_valueChanged(double value) { GLWindow->bb[4] = (float)value; GLWindow->updateGL(); }

/************************************************************
 * LOAD NEW FILE CLICKED. button handler
 ************************************************************/
void UIConnector::on_loadNewFileButton_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.open(this, SLOT(fileSelected(QString)));
    dialog.setVisible(true);
    dialog.exec();
}
void UIConnector::on_loadDRSCalibButton_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.open(this, SLOT(calibFileSelected(QString)));
    dialog.setVisible(true);
    dialog.exec();
}
/************************************************************
 * FILE SELECTED. return of the file open dialog handler
 ************************************************************/
void UIConnector::fileSelected(QString file)
{
    const bool reopen = !currentFile.empty();

    currentFile = file.toStdString();
    if (!currentFile.empty())
        GLWindow->openFile(currentFile, reopen && currentCalibFile.empty());
}
void UIConnector::calibFileSelected(QString file)
{
    const bool reopen = currentCalibFile.empty();

    currentCalibFile = file.toStdString();
    if (reopen && !currentFile.empty())
        GLWindow->openFile(currentFile, true);
    if (currentCalibFile != "")
        GLWindow->openCalibFile(currentCalibFile);


    if (GLWindow->fDrsCalib.fRoi != 0)
    {//spread the calibration data to the displayers
        Baseline_window->getCalibrationDataForDisplay(RawDataViewer::CALIB_BASELINE,
                                                      GLWindow->fDrsCalib.fOffset,
                                                      GLWindow->fDrsCalib.fRoi,
                                                      GLWindow->fDrsCalib.fNumTm);

        Gain_window->getCalibrationDataForDisplay(RawDataViewer::CALIB_GAIN,
                                                  GLWindow->fDrsCalib.fGain,
                                                  GLWindow->fDrsCalib.fRoi,
                                                  GLWindow->fDrsCalib.fNumTm);

        TriggerOffset_window->getCalibrationDataForDisplay(RawDataViewer::CALIB_TRIG_OFFSET,
                                                           GLWindow->fDrsCalib.fTrgOff,
                                                           GLWindow->fDrsCalib.fRoi,
                                                           GLWindow->fDrsCalib.fNumTm);

    }
}
/************************************************************
 * NEW FILE LOADED. update of the UI after a new file has been loaded
 ************************************************************/
void UIConnector::newFileLoaded()
{
    ostringstream str;

    //extract the file name only (no path) from the full name
    str << "File: ";
    if (currentFile.size() > 2)
        str << currentFile.substr(currentFile.find_last_of("//")+1, currentFile.size()) << "\n";
    else
        str << "--\n";
    str << "Calibration: ";
    if (currentCalibFile.size() > 2)
        str << currentCalibFile.substr(currentCalibFile.find_last_of("//")+1, currentCalibFile.size()) << "\n";
    else
        str << "--\n";
//    fileLoadedLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "Run number: " << GLWindow->runNumber << "\n";
//    runNumberLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "Number of Events: " << GLWindow->nRows << "\n";

    displayingEventBox->setMaximum(GLWindow->nRows-1);

    str << "Number of Slices: " << GLWindow->nRoi << "\n";// << "/1024";
//    numberOfSlicesLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "Number of Time Marks: " << GLWindow->nTM << "\n";
//    numberOfTimeMarksLabel->setText(QString(str.str().c_str()));

//    str.str("");
    str << "Run Type: " << GLWindow->runType << "\n";
//    runTypeLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "Time of 1st data: " << GLWindow->firstDataTime << "\n";
//    firstTimeLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "Time of last data: " << GLWindow->lastDataTime << "\n";
//    lastTimeLabel->setText(QString(str.str().c_str()));
//    str.str("");
    str << "SVN revision: " << GLWindow->revision << '\n';
    str << "Number of boards: " << GLWindow->nBoards << '\n';
    str << "Number of pixels: " << GLWindow->nPixels << '\n';
    str << "Number of Slices TM: " << GLWindow->nRoiTM << '\n';
    str << "Time system: " << GLWindow->timeSystem << '\n';
    str << "Date: " << GLWindow->creationDate << '\n';
    str << "Night: " << GLWindow->nightInt << '\n';
    str << "Camera: " << GLWindow->camera << '\n';
    str << "DAQ: " << GLWindow->daq << '\n';
    str << "ADC Count: " << GLWindow->adcCount << '\n';
    str << "NB Evts OK:" << GLWindow->nbOk << '\n';
    str << "NB Evts Rejected: " << GLWindow->nbRej << '\n';
    str << "NB Evts Bad: " << GLWindow->nbBad << '\n';
    extraInfoLabel->setText(QString(str.str().c_str()));

    /*
    if (GLWindow->calibrationLoaded)
    {
        drawCalibrationCheckBox->setEnabled(true);
    }*/


}
/************************************************************
 * PLAY PAUSE CLICKED. ui handler
 ************************************************************/
void UIConnector::on_playPauseButton_clicked()
{
    if (timer.isActive())
        timer.stop();
    else
        timer.start();
}

void UIConnector::displaySliceValue()
{
    if (!GLWindow->nRoi)
        return;
    if (GLWindow->selectedPixel == -1)
    {
        ostringstream str;
        str << " Current Pixel val.: --";
        currentPixelValue->setText(QString(str.str().c_str()));
        return;
    }
//    int mapping = GLWindow->_softwareOrdering ? GLWindow->selectedPixel : GLWindow->hardwareMapping[GLWindow->selectedPixel];
    int mapping = GLWindow->hardwareMapping[GLWindow->selectedPixel];
    const int idx = GLWindow->nRoi*mapping + GLWindow->whichSlice;

    ostringstream str;
    str << "Current Pixel val.: " << GLWindow->eventData[idx];
    currentPixelValue->setText(QString(str.str().c_str()));
}

/************************************************************
 * CURRENT SLICE HAS CHANGE. ui handler
 ************************************************************/
void UIConnector::currentSliceHasChanged(int slice)
{
    if (!GLWindow->nRoi)
        return;

    if (updateSpinnerDisplay)
        displayingSliceBox->setValue(slice);

    displaySliceValue();
}

/*****
 *******************************************************
 * CURRENT EVENT HAS CHANGED. ui handler
 ************************************************************/

double xval[50000];
double yval[50000];
void UIConnector::on_displayingEventBox_valueChanged(int cEvent)
{
//    cout << "Here " << updateSpinnerDisplay << endl;
    if (!updateSpinnerDisplay)
        return;
    updateSpinnerDisplay = false;
//    currentEventHasChanged(cEvent);
    GLWindow->rowNum = cEvent - GLWindow->eventStep;
    GLWindow->eventStepping(true);
    updateSpinnerDisplay = true;

//    GLWindow->updateGL();
}
void UIConnector::on_slicesPerSecValue_valueChanged(double value)
{
    timer.setInterval(1000.0/value);
}
void UIConnector::on_displayingSliceBox_valueChanged(int cSlice)
{
    updateSpinnerDisplay = false;
    currentSliceHasChanged(cSlice);
    updateSpinnerDisplay = true;
    GLWindow->whichSlice = cSlice;
    GLWindow->updateGL();
}
void UIConnector::currentEventHasChanged(int )
{

    RMS_window->SetData(GLWindow->RMSvalues);
    Mean_window->SetData(GLWindow->Meanvalues);
    PosOfMax_window->SetData(GLWindow->PosOfMaxvalues);
    Max_window->SetData(GLWindow->Maxvalues);
    threeD_Window->setData(GLWindow->eventData.data());//rawEventData);

    if (RMS_window->isVisible())
        RMS_window->updateGL();
    if (Mean_window->isVisible())
        Mean_window->updateGL();
    if (PosOfMax_window->isVisible())
        PosOfMax_window->updateGL();
    if (Max_window->isVisible())
        Max_window->updateGL();
    ostringstream str;
//    str << "Displaying Event " << cEvent;
//    QString qstr(str.str().c_str());
//    emit updateCurrentEventDisplay(qstr);
    if (updateSpinnerDisplay)
    {
        updateSpinnerDisplay = false;
        displayingEventBox->setValue(GLWindow->rowNum);
        updateSpinnerDisplay = true;
    }

 //   GLWindow->doWaveLetOnCurrentEventPlease();

        //retrieve the data that we want to display
    boost::posix_time::ptime hrTime( boost::gregorian::date(1970, boost::gregorian::Jan, 1),
            boost::posix_time::seconds(GLWindow->pcTime[0]) +  boost::posix_time::microsec(GLWindow->pcTime[1]));

    str.str("");
    str << "PC Time: " << boost::posix_time::to_iso_extended_string(hrTime);
    PCTimeLabel->setText(QString(str.str().c_str()));

    str.str("");
    str << "Software Trigger: " << GLWindow->softTrig;
    softwareTriggerLabel->setText(QString(str.str().c_str()));

    str.str("");
    str << "Trigger Type: " << GLWindow->triggerType;
    triggerTypeLabel->setText(QString(str.str().c_str()));

    displaySliceValue();

    if (autoScaleColor->isChecked())
        emit GLWindow->colorPaletteHasChanged();//autoScalePressed();

    boardsTimeList->clear();
    startPixelsList->clear();
    startTimeMarksList->clear();
    triggerDelayList->clear();
    std::map<int, int> boardsHistoMap;
    for (int i=0;i <NBOARDS; i++)
    {
        str.str("");
        str << i;
        if (i<10) str << " ";
        if (i<100) str << " ";
        if (i<1000) str << " ";
        str << ": " << GLWindow->boardTime[i];
        boardsTimeList->addItem(QString(str.str().c_str()));
        if (boardsHistoMap.find(GLWindow->boardTime[i]) != boardsHistoMap.end())
            boardsHistoMap[GLWindow->boardTime[i]]++;
        else
            boardsHistoMap[GLWindow->boardTime[i]] = 1;
    }
    std::map<int, int> pixelHistoMap;
    for (int i=0;i <NPIX; i++)
    {
        str.str("");
        str << i;
        if (i<10) str << " ";
        if (i<100) str << " ";
        if (i<1000) str << " ";
        str << ": " << GLWindow->startPix[i];
        startPixelsList->addItem(QString(str.str().c_str()));
        if (pixelHistoMap.find(GLWindow->startPix[i]) != pixelHistoMap.end())
            pixelHistoMap[GLWindow->startPix[i]]++;
        else
            pixelHistoMap[GLWindow->startPix[i]] = 1;
    }

    std::map<int, int> timeMarksMap;
    for (int i=0;i <NTMARK; i++)
    {
        str.str("");
        str << i;
        if (i<10) str << " ";
        if (i<100) str << " ";
        if (i<1000) str << " ";
        str << ": " << GLWindow->startTM[i];
        startTimeMarksList->addItem(QString(str.str().c_str()));
        if (timeMarksMap.find(GLWindow->startTM[i]) != timeMarksMap.end())
            timeMarksMap[GLWindow->startTM[i]]++;
        else
            timeMarksMap[GLWindow->startTM[i]] = 1;
    }
    std::map<int,int> delayMap;
    triggerDelayList->addItem(QString("Patch | Slice:Delay Slice:Delay..."));
    for (int i=0;i<NTMARK; i++)
    {
        str.str("");
        str << i << " | ";
        for (int j=0;j<GLWindow->nRoiTM;j++)
        {
            int value = GLWindow->eventData[1440*GLWindow->nRoi + i*GLWindow->nRoiTM + j];
            if (delayMap.find(value) != delayMap.end())
                 delayMap[value]++;
             else
                 delayMap[value] = 1;
            str << j << ":" << value << " ";
         }
        triggerDelayList->addItem(QString(str.str().c_str()));
    }

    std::map<int,int>::iterator it = boardsHistoMap.begin();
    int nsamples = 0;
    int previousValue = it->first-10;
    for (unsigned int i=0;i<boardsHistoMap.size();i++)
    {
        if (previousValue != it->first-1)
        {
            xval[nsamples] = previousValue+1;
            yval[nsamples] = 0;
            nsamples++;
            xval[nsamples] = it->first-1;
            yval[nsamples] = 0;
            nsamples++;
        }
        xval[nsamples] = it->first;
        yval[nsamples] = it->second;
        previousValue = it->first;
        it++;
        nsamples++;
        xval[nsamples] = previousValue;
        yval[nsamples] = 0;
        nsamples++;
        if (nsamples > 4090)
        {
            cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
            break;
        }
    }
    xval[nsamples] = it==boardsHistoMap.begin() ? 0 : (--it)->first+1;
    yval[nsamples] = 0;
    nsamples++;
 //   if (nsamples > 5)
#if QWT_VERSION < 0x060000
       boardsTimeHistoItem.setData(xval, yval, nsamples);
#else
       boardsTimeHistoItem.setSamples(xval, yval, nsamples);
#endif

    it = pixelHistoMap.begin();
    nsamples = 0;
    previousValue = it->first-10;
    for (unsigned int i=0;i<pixelHistoMap.size();i++)
    {
        if (previousValue != it->first-1)
        {
            xval[nsamples] = previousValue+1;
            yval[nsamples] = 0;
            nsamples++;
            xval[nsamples] = it->first-1;
            yval[nsamples] = 0;
            nsamples++;
        }
        xval[nsamples] = it->first;
        yval[nsamples] = it->second;
        previousValue = it->first;
        it++;
        nsamples++;
        xval[nsamples] = previousValue;
        yval[nsamples] = 0;
        nsamples++;
        if (nsamples > 4090)
        {
            cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
            break;
        }
   }
    xval[nsamples] = it==pixelHistoMap.begin() ? 0 : (--it)->first+1;
    yval[nsamples] = 0;
    nsamples++;
//    if (nsamples > 5)
#if QWT_VERSION < 0x060000
       startCellHistoItem.setData(xval, yval, nsamples);
#else
       startCellHistoItem.setSamples(xval, yval, nsamples);
#endif

    it = timeMarksMap.begin();
    nsamples = 0;
    previousValue = it->first-10;
    for (unsigned int i=0;i<timeMarksMap.size();i++)
    {
        if (previousValue != it->first-1)
        {
            xval[nsamples] = previousValue+1;
            yval[nsamples] = 0;
            nsamples++;
            xval[nsamples] = it->first-1;
            yval[nsamples] = 0;
            nsamples++;
        }
        xval[nsamples] = it->first;
        yval[nsamples] = it->second;
        previousValue = it->first;
        it++;
        nsamples++;
        xval[nsamples] = previousValue;
        yval[nsamples] = 0;
        nsamples++;
        if (nsamples > 4090)
        {
            cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
            break;
        }
    }
    xval[nsamples] = it==timeMarksMap.begin() ? 0 : (--it)->first+1;
    yval[nsamples] = 0;
    nsamples++;
 //   if (nsamples > 5)
#if QWT_VERSION < 0x060000
       startTimeMarkHistoItem.setData(xval, yval, nsamples);
#else
       startTimeMarkHistoItem.setSamples(xval, yval, nsamples);
#endif

    it = delayMap.begin();
    nsamples = 0;
    previousValue = it->first-10;
    for (unsigned int i=0;i<delayMap.size();i++)
    {
        if (previousValue != it->first-1)
        {
            xval[nsamples] = previousValue+1;
            yval[nsamples] = 0;
            nsamples++;
            xval[nsamples] = it->first-1;
            yval[nsamples] = 0;
            nsamples++;
        }
        xval[nsamples] = it->first;
        yval[nsamples] = it->second;
        previousValue = it->first;
        it++;
        nsamples++;
        xval[nsamples] = previousValue;
        yval[nsamples] = 0;
        nsamples++;
        if (nsamples > 4090)
        {
            cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
            break;
        }
    }
    xval[nsamples] = it==delayMap.begin() ? 0 : (--it)->first+1;
    yval[nsamples] = 0;
    nsamples++;
  //  if (nsamples > 5)
#if QWT_VERSION < 0x060000
       triggerDelayHistoItem.setData(xval, yval, nsamples);
#else
       triggerDelayHistoItem.setSamples(xval, yval, nsamples);
#endif
       //WAVELETS HACK
/*       std::map<int, int> valuesHistoMap;
       std::map<int, int> waveletHistoMap;
       for (int i=0;i<1024*1440;i++)
       {
           if (valuesHistoMap.find(GLWindow->rawEventData[i]) != valuesHistoMap.end())
               valuesHistoMap[GLWindow->rawEventData[i]]++;
           else
               valuesHistoMap[GLWindow->rawEventData[i]] = 1;
           if (waveletHistoMap.find(GLWindow->waveLetArray[i]) != waveletHistoMap.end())
               waveletHistoMap[GLWindow->waveLetArray[i]]++;
           else
               waveletHistoMap[GLWindow->waveLetArray[i]] = 1;
       }

       it = valuesHistoMap.begin();
       nsamples = 0;
       previousValue = it->first-10;
       cout << "Num values Original: " << valuesHistoMap.size() << endl;
       for (unsigned int i=0;i<valuesHistoMap.size();i++)
       {
           if (previousValue != it->first-1)
           {
               xval[nsamples] = previousValue+1;
               yval[nsamples] = 0;
               nsamples++;
               xval[nsamples] = it->first-1;
               yval[nsamples] = 0;
               nsamples++;
           }
           xval[nsamples] = it->first;
           yval[nsamples] = it->second;
           previousValue = it->first;
           it++;
           nsamples++;
           xval[nsamples] = previousValue;
           yval[nsamples] = 0;
           nsamples++;
           if (nsamples > 50000)
           {
               cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
               break;
           }
       }
       xval[nsamples] = it==valuesHistoMap.begin() ? 0 : (--it)->first+1;
       yval[nsamples] = 0;
       nsamples++;
     //  if (nsamples > 5)
   #if QWT_VERSION < 0x060000
          triggerDelayHistoItem.setData(xval, yval, nsamples);
   #else
          triggerDelayHistoItem.setSamples(xval, yval, nsamples);
   #endif

          it = waveletHistoMap.begin();
          nsamples = 0;
          previousValue = it->first-10;
          cout << "Num values WaveLets: " << waveletHistoMap.size() << endl;
          for (unsigned int i=0;i<waveletHistoMap.size();i++)
          {
              if (previousValue != it->first-1)
              {
                  xval[nsamples] = previousValue+1;
                  yval[nsamples] = 0;
                  nsamples++;
                  xval[nsamples] = it->first-1;
                  yval[nsamples] = 0;
                  nsamples++;
              }
              xval[nsamples] = it->first;
              yval[nsamples] = it->second;
              previousValue = it->first;
              it++;
              nsamples++;
              xval[nsamples] = previousValue;
              yval[nsamples] = 0;
              nsamples++;
              if (nsamples > 50000)
              {
                  cout << "Error: Maximum number of samples reached for histograms. skipping what's remaining" << endl;
                  break;
              }
          }
          xval[nsamples] = it==waveletHistoMap.begin() ? 0 : (--it)->first+1;
          yval[nsamples] = 0;
          nsamples++;
        //  if (nsamples > 5)
      #if QWT_VERSION < 0x060000
          startTimeMarkHistoItem.setData(xval, yval, nsamples);
      #else
          startTimeMarkHistoItem.setSamples(xval, yval, nsamples);
      #endif
*/
//END OF WAVELETS HACK
       //    startCellHistoZoom->setZoomBase(startCellHistoItem.boundingRect());
    QStack< QRectF > stack;
//    QRectF cRectangle = boardsTimeHistoItem.boundingRect();
    stack.push(scaleBoundingRectangle(boardsTimeHistoItem.boundingRect(), 1.05f));//cRectangle);//boardsTimeHistoItem.boundingRect());
    boardsTimeHistoZoom->setZoomStack(stack);
    stack.pop();
    stack.push(scaleBoundingRectangle(startCellHistoItem.boundingRect(), 1.05f));
    startCellHistoZoom->setZoomStack(stack);
    stack.pop();
    stack.push(scaleBoundingRectangle(startTimeMarkHistoItem.boundingRect(), 1.05f));
    startTimeMarkHistoZoom->setZoomStack(stack);
    stack.pop();
    stack.push(scaleBoundingRectangle(triggerDelayHistoItem.boundingRect(), 1.05f));
    triggerDelayHistoZoom->setZoomStack(stack);
    stack.pop();

    pixelChanged(GLWindow->selectedPixel);
}
//can't use a ref to rectangle, as the type must be converted first
QRectF UIConnector::scaleBoundingRectangle(QRectF rectangle, float scale)
{
    QPointF bottomRight = rectangle.bottomRight();
    QPointF topLeft = rectangle.topLeft();
    QPointF center = rectangle.center();
    return QRectF(topLeft + (topLeft-center)*(scale-1.0f), //top left
                  bottomRight + (bottomRight-center)*(scale-1.0f)); //bottom right
}
void UIConnector::initHistograms()
{
//    QwtPlot*     boardsTimeHisto;
//    QwtPlotHistogram boardsTimeHistoItem;
    grid1 = new QwtPlotGrid;
    grid1->enableX(false);
    grid1->enableY(true);
    grid1->enableXMin(false);
    grid1->enableYMin(false);
    grid1->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid1->attach(boardsTimeHisto);

    grid2 = new QwtPlotGrid;
    grid2->enableX(false);
    grid2->enableY(true);
    grid2->enableXMin(false);
    grid2->enableYMin(false);
    grid2->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid2->attach(startCellsHisto);

    grid3 = new QwtPlotGrid;
    grid3->enableX(false);
    grid3->enableY(true);
    grid3->enableXMin(false);
    grid3->enableYMin(false);
    grid3->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid3->attach(startTimeMarkHisto);

    grid4 = new QwtPlotGrid;
    grid4->enableX(false);
    grid4->enableY(true);
    grid4->enableXMin(false);
    grid4->enableYMin(false);
    grid4->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid4->attach(pixelValueCurve);

    grid6 = new QwtPlotGrid;
    grid6->enableX(false);
    grid6->enableY(true);
    grid6->enableXMin(false);
    grid6->enableYMin(false);
    grid6->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid6->attach(pixelAverageCurve);

    grid5 = new QwtPlotGrid;
    grid5->enableX(false);
    grid5->enableY(true);
    grid5->enableXMin(false);
    grid5->enableYMin(false);
    grid5->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    grid5->attach(triggerDelayHisto);

    boardsTimeHisto->setAutoReplot(true);
    startCellsHisto->setAutoReplot(true);
    startTimeMarkHisto->setAutoReplot(true);
    pixelValueCurve->setAutoReplot(true);
    pixelAverageCurve->setAutoReplot(true);
    triggerDelayHisto->setAutoReplot(true);
    boardsTimeHisto->setTitle("Boards time values");
    startCellsHisto->setTitle("Start Cell values");
    startTimeMarkHisto->setTitle("Start Time Marks values");
    pixelValueCurve->setTitle("Current pixel values");
    pixelAverageCurve->setTitle("Average pixels values");
    triggerDelayHisto->setTitle("Trigger Delays");

 //   boardsTimeHistoItem.setBrush(QBrush(Qt::red));
//    startCellHistoItem.setBrush(QBrush(Qt::red));
//    startTimeMarkHistoItem.setBrush(QBrush(Qt::red));
//    triggerDelayHistoItem.setBrush(QBrush(Qt::red));
//    pixelValueCurveItem.setBrush(QBrush(Qt::red));

    boardsTimeHistoItem.setPen(QColor(Qt::darkGreen));
    boardsTimeHistoItem.setStyle(QwtPlotCurve::Steps);
    startCellHistoItem.setPen(QColor(Qt::darkGreen));
    startCellHistoItem.setStyle(QwtPlotCurve::Steps);
    startTimeMarkHistoItem.setPen(QColor(Qt::darkGreen));
    startTimeMarkHistoItem.setStyle(QwtPlotCurve::Steps);
    triggerDelayHistoItem.setPen(QColor(Qt::darkGreen));
    triggerDelayHistoItem.setStyle(QwtPlotCurve::Steps);

    boardsTimeHistoItem.attach(boardsTimeHisto);
    startCellHistoItem.attach(startCellsHisto);
    startTimeMarkHistoItem.attach(startTimeMarkHisto);
    triggerDelayHistoItem.attach(triggerDelayHisto);

    //curve
//    pixelValueCurveItem.setSymbol(new QwtSymbol(QwtSymbol::Cross, Qt::NoBrush, QPen(Qt::black), QSize(5,5)));
    pixelValueCurveItem.setPen(QColor(Qt::black));
    pixelAverageCurveItem.setPen(QColor(Qt::black));
    aMeanCurveItem.setPen(QColor(Qt::darkGreen));
    vCorrCurveItem.setPen(QColor(Qt::red));
    meanCurveItem.setPen(QColor(Qt::blue));
    pixelValueCurveItem.setStyle(QwtPlotCurve::Lines);
    pixelAverageCurveItem.setStyle(QwtPlotCurve::Lines);
    aMeanCurveItem.setStyle(QwtPlotCurve::Lines);
    vCorrCurveItem.setStyle(QwtPlotCurve::Lines);
    meanCurveItem.setStyle(QwtPlotCurve::Lines);

//    pixelValueCurveItem.setCurveAttribute(QwtPlotCurve::Fitted);
    pixelValueCurveItem.attach(pixelValueCurve);
    pixelAverageCurveItem.attach(pixelAverageCurve);
//    aMeanCurveItem.attach(pixelValueCurve);
 //   vCorrCurveItem.attach(pixelValueCurve);
//    meanCurveItem.attach(pixelValueCurve);

    //FIXME delete these pointers with the destructor
    curveZoom = new QwtPlotZoomer(pixelValueCurve->canvas());
    curveZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    curveZoom->setTrackerPen(QPen(Qt::gray));
    averageCurveZoom = new QwtPlotZoomer(pixelAverageCurve->canvas());
    averageCurveZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    averageCurveZoom->setTrackerPen(QPen(Qt::gray));

    boardsTimeHistoZoom = new QwtPlotZoomer(boardsTimeHisto->canvas());
    boardsTimeHistoZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    boardsTimeHistoZoom->setTrackerPen(QPen(Qt::gray));

    startCellHistoZoom = new QwtPlotZoomer(startCellsHisto->canvas());
    startCellHistoZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    startCellHistoZoom->setTrackerPen(QPen(Qt::gray));

    startTimeMarkHistoZoom = new QwtPlotZoomer(startTimeMarkHisto->canvas());
    startTimeMarkHistoZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    startTimeMarkHistoZoom->setTrackerPen(QPen(Qt::gray));

    triggerDelayHistoZoom = new QwtPlotZoomer(triggerDelayHisto->canvas());
    triggerDelayHistoZoom->setRubberBandPen(QPen(Qt::gray, 2, Qt::DotLine));
    triggerDelayHistoZoom->setTrackerPen(QPen(Qt::gray));


}

void UIConnector::pixelChanged(int pixel)
{
    RMS_window->fWhite = pixel;
    Mean_window->fWhite = pixel;
    Max_window->fWhite = pixel;
    PosOfMax_window->fWhite = pixel;
    if (pixel != -1)
    {
        RMS_window->fWhitePatch = RMS_window->pixelsPatch[pixel];
        Mean_window->fWhitePatch = Mean_window->pixelsPatch[pixel];
        Max_window->fWhitePatch = Max_window->pixelsPatch[pixel];
        PosOfMax_window->fWhitePatch = PosOfMax_window->pixelsPatch[pixel];
    }
    else
    {
        RMS_window->fWhitePatch = -1;
        Mean_window->fWhitePatch = -1;
        Max_window->fWhitePatch = -1;
        PosOfMax_window->fWhitePatch = -1;
    }
    if (pixel == -1)
        return;
    int softwarePix = pixel;
//    if (!GLWindow->_softwareOrdering)
        pixel = GLWindow->hardwareMapping[pixel];

    HwIDBox->setValue(pixel);

    if (!GLWindow->nRoi)
        return;

int currentPixel = pixel;

    for (int i=0;i<GLWindow->nRoi;i++)
    {
        xval[i] = i;
        yval[i] = GLWindow->eventData[GLWindow->nRoi*currentPixel + i];
    }

int realNumSamples = GLWindow->nRoi;
    if (GLWindow->nRoiTM != 0)
    {
        const PixelMapEntry& mapEntry = GLWindow->fPixelMap.index(softwarePix);
        const int pixelIdInPatch = mapEntry.pixel();
        const int patchId = mapEntry.patch();
        const int boardId = mapEntry.board();
        const int crateId = mapEntry.crate();
        if (pixelIdInPatch == 8)
        {
            int TMIndex = 0;
            int xIndex = GLWindow->nRoi;
            int arrayIndex = GLWindow->nRoi;
            if (GLWindow->offSetRoi < 0)
                TMIndex -= GLWindow->offSetRoi;
            if (GLWindow->offSetRoi > 0)
                xIndex += GLWindow->offSetRoi;
            for (int i=TMIndex;i<GLWindow->nRoiTM;i++, xIndex++, arrayIndex++)
            {
                xval[arrayIndex] = xIndex;
                yval[arrayIndex] = GLWindow->eventData[GLWindow->nRoi*1440 + GLWindow->nRoiTM*(40*crateId + 4*boardId + patchId) + i];
            }
            realNumSamples += GLWindow->nRoiTM - TMIndex;
        }
      //  cout << pixelIdInPatch << " " ;
    }

#if QWT_VERSION < 0x060000
    pixelValueCurveItem.setData(xval, yval, realNumSamples);
#else
       pixelValueCurveItem.setSamples(xval, yval, realNumSamples);
#endif

//now compute the average value of all pixels
       currentPixel = 0;
       for (int i=0;i<GLWindow->nRoi;i++)
           yval[i] = 0;
       for (int j=0;j<1440;j++) {
           currentPixel = j;
           if (GLWindow->softwareMapping[j]>=GLWindow->nPixels)
               continue;
           for (int i=0;i<GLWindow->nRoi;i++)
           {
               xval[i] = i;
               yval[i] += GLWindow->eventData[GLWindow->nRoi*currentPixel + i];
           }
       }
       for (int i=0;i<GLWindow->nRoi;i++)
           yval[i] /= GLWindow->nPixels;
#if QWT_VERSION < 0x060000
       pixelAverageCurveItem.setData(xval, yval, GLWindow->nRoi);
#else
    pixelAverageCurveItem.setSamples(xval, yval, realNumSamples);
#endif

    QStack< QRectF > stack;
    stack.push(scaleBoundingRectangle(pixelValueCurveItem.boundingRect(), 1.5f));
    curveZoom->setZoomBase(scaleBoundingRectangle(pixelValueCurveItem.boundingRect(), 1.5f));
    curveZoom->setZoomStack(stack);
    stack.pop();
    stack.push(scaleBoundingRectangle(pixelAverageCurveItem.boundingRect(), 1.5f));
    averageCurveZoom->setZoomBase(scaleBoundingRectangle(pixelAverageCurveItem.boundingRect(), 1.5f));
    averageCurveZoom->setZoomStack(stack);
    stack.pop();

    displaySliceValue();
    if (autoScaleColor->isChecked())
        on_autoScaleColor_clicked();
}

void UIConnector::on_HwIDBox_valueChanged(int)
{
    updating = true;

    const int hwID = HwIDBox->value();

    const int crateID =  hwID/360;
    const int boardID = (hwID%360)/36;
    const int patchID = (hwID%36 )/9;
    const int pixelID =  hwID%9;

    SwIDBox->setValue(GLWindow->softwareMapping[hwID]);

    crateIDBox->setValue(crateID);
    boardIDBox->setValue(boardID);
    patchIDBox->setValue(patchID);
    pixelIDBox->setValue(pixelID);

    updating = false;

    GLWindow->selectedPixel = GLWindow->softwareMapping[hwID];
    GLWindow->updateGL();

    pixelChanged(GLWindow->selectedPixel);
}

void UIConnector::cbpxChanged()
{
    if (updating)
        return;

    const int hwid = crateIDBox->value()*360 + boardIDBox->value()*36 + patchIDBox->value()*9 + pixelIDBox->value();
    HwIDBox->setValue(hwid);
}

void UIConnector::on_SwIDBox_valueChanged(int swid)
{
    if (updating)
        return;

//    if (GLWindow->_softwareOrdering)
//        HwIDBox->setValue(swid);
//    else
        HwIDBox->setValue(GLWindow->hardwareMapping[swid]);
}

void UIConnector::on_autoScaleColor_clicked()
{
    if (!autoScaleColor->isChecked())
    {
        GLWindow->ss[0] = 0.496;
        GLWindow->ss[1] = 0.507;
        GLWindow->ss[2] = 0.518;
        GLWindow->ss[3] = 0.529;
        GLWindow->ss[4] = 0.540;;
        colorRange0->setValue(GLWindow->ss[0]);
        colorRange1->setValue(GLWindow->ss[1]);
        colorRange2->setValue(GLWindow->ss[2]);
        colorRange3->setValue(GLWindow->ss[3]);
        colorRange4->setValue(GLWindow->ss[4]);
        return;
    }
    if (!GLWindow->nRoi)
        return;

    int start = 0;
    int end   = 1440;

    if (!entireCameraScale->isChecked())
    {
        start = GLWindow->selectedPixel;
        end   = GLWindow->selectedPixel+1;
        if (end == 0)
        {
            start = 0;
            end = 1440;
        }
    }

    int min =  100000; //real min = -2048, int_16 = -32768 to 32767
    int max = -100000; //real max = 2047

    long average = 0;
    long numSamples = 0;
    int errorDetected = -1;

    for (int i=start;i<end;i++)
    {
        if (i==863)//keep crazy pixel out of the autoscale
            continue;

        if (GLWindow->softwareMapping[i]>=GLWindow->nPixels)
            continue;

        for (int j=10;j<GLWindow->nRoi-50;j++)
        {
            int cValue = GLWindow->eventData[i*GLWindow->nRoi+j];
            if (cValue > max && cValue < 32767)
                max = cValue;
            if (cValue < min && cValue > -32768)
               min = cValue;
            if (cValue < 32767 && cValue > -32768)
            {
                average+=cValue;
                numSamples++;
            }
            else
            {
                errorDetected = i;
            }
//            numSamples++;
        }
    }
    average /= numSamples;
    if (errorDetected != -1)
    {
        cout << "Overflow detected at pixel " << errorDetected << " (at least)" << endl;
    }
//    cout << "min: " << min << " max: " << max << " average: " << average << endl;
    float minRange = (float)(min+(GLWindow->VALUES_SPAN/2))/(float)(GLWindow->VALUES_SPAN-1);
    float maxRange = (float)(max+(GLWindow->VALUES_SPAN/2))/(float)(GLWindow->VALUES_SPAN-1);
    float midRange = (float)(average+(GLWindow->VALUES_SPAN/2))/(float)(GLWindow->VALUES_SPAN-1);
    if (GLWindow->logScale)
    {
        minRange *= 9;
        maxRange *= 9;
//        midRange *= 9;
        minRange += 1;
        maxRange += 1;
//        midRange += 1;
        minRange = log10(minRange);
        maxRange = log10(maxRange);
//        midRange = (minRange + maxRange)/2.f;
        midRange = log10(midRange);
    }

    GLWindow->ss[0] = minRange;
    colorRange0->setValue(GLWindow->ss[0]);
    GLWindow->ss[4] = maxRange;
    colorRange4->setValue(GLWindow->ss[4]);
//    GLWindow->ss[2] = midRange;
//    range2->setValue(GLWindow->ss[2]);
//    GLWindow->ss[1] = (minRange+midRange)/2;
//    range1->setValue(GLWindow->ss[1]);
//    GLWindow->ss[3] = (maxRange+midRange)/2;
//    range3->setValue(GLWindow->ss[3]);

    GLWindow->ss[2] = (maxRange+minRange)/2;
    colorRange2->setValue(GLWindow->ss[2]);

    GLWindow->ss[1] = minRange+(maxRange-minRange)/4;
    colorRange1->setValue(GLWindow->ss[1]);

    GLWindow->ss[3] = minRange+3*(maxRange-minRange)/4;
    colorRange3->setValue(GLWindow->ss[3]);
}

void PrintUsage()
{
    cout << "\n"
        "The FACT++ raw data viewer.\n"
        "\n"
        "Usage: viewer [OPTIONS] [datafile.fits[.gz|.fz] [calibration.drs.fits[.gz]]]\n"
        "  or:  viewer [OPTIONS]\n";
    cout << endl;

}

void PrintHelp()
{
    cout <<
            "\n"
         << endl;
}

int UIConnector::SetupConfiguration(Configuration &conf)
{
    RawDataViewer *canvas = GLWindow;

    if (conf.Has("mappingFile"))
    {
        canvas->assignPixelMapFile(conf.GetPrefixedString("mappingFile"));
    }
    else
        canvas->assignPixelMapFile("");

    if (!canvas->isFACT())
    {
        HwIDBox->setMaximum(71);
        SwIDBox->setMaximum(71);
        SwIDBox->setValue(0);
        HwIDBox->setValue(19);
        crateIDBox->setMaximum(0);
        crateIDBox->setEnabled(false);
        boardIDBox->setMaximum(1);
    }

    if (conf.Has("color.range"))
    {
        vector<double> value = conf.Vec<double>("color.range");
        if (value.size() != 5)
        {
            cout << "Error, colorRange option should have exactly 5 double values" << endl;
            return -1;
        }
        for (int i=0;i<5;i++)
            canvas->ss[i] = value[i];
    }

    if (conf.Has("color.red"))
    {
        vector<double> value = conf.Vec<double>("color.red");
        if (value.size() != 5)
        {
            cout << "Error, colorRed option should have exactly 5 double values" << endl;
            return -1;
        }
        for (int i=0;i<5;i++)
            canvas->rr[i] = value[i];
    }

    if (conf.Has("color.green"))
    {
        vector<double> value = conf.Vec<double>("color.green");
        if (value.size() != 5)
        {
            cout << "Error, colorGreen option should have exactly 5 double values" << endl;
            return -1;
        }
        for (int i=0;i<5;i++)
            canvas->gg[i] = value[i];
    }

    if (conf.Has("color.blue"))
    {
        vector<double> value = conf.Vec<double>("color.blue");
        if (value.size() != 5)
        {
            cout << "Error, colorBlue option should have exactly 5 double values" << endl;
            return -1;
        }
        for (int i=0;i<5;i++)
            canvas->bb[i] = value[i];
    }

    colorRange0->setValue(canvas->ss[0]);
    colorRange1->setValue(canvas->ss[1]);
    colorRange2->setValue(canvas->ss[2]);
    colorRange3->setValue(canvas->ss[3]);
    colorRange4->setValue(canvas->ss[4]);
    redValue0->setValue(canvas->rr[0]);
    redValue1->setValue(canvas->rr[1]);
    redValue2->setValue(canvas->rr[2]);
    redValue3->setValue(canvas->rr[3]);
    redValue4->setValue(canvas->rr[4]);
    greenValue0->setValue(canvas->gg[0]);
    greenValue1->setValue(canvas->gg[1]);
    greenValue2->setValue(canvas->gg[2]);
    greenValue3->setValue(canvas->gg[3]);
    greenValue4->setValue(canvas->gg[4]);
    blueValue0->setValue(canvas->bb[0]);
    blueValue1->setValue(canvas->bb[1]);
    blueValue2->setValue(canvas->bb[2]);
    blueValue3->setValue(canvas->bb[3]);
    blueValue4->setValue(canvas->bb[4]);

    if (conf.Has("drs"))
    {
        const QString qstr(conf.Get<string>("drs").c_str());
        calibFileSelected(qstr);
    }

    if (conf.Has("file"))
    {
        const QString qstr(conf.Get<string>("file").c_str());
        fileSelected(qstr);
    }


    return 0;
}

void SetupConfiguration(Configuration& conf)
{
    po::options_description configs("Raw Events Viewer Options");
    configs.add_options()
        ("color.range", vars<double>(), "Range of the display colours")
        ("color.red",   vars<double>(), "Range of red values")
        ("color.green", vars<double>(), "Range of green values")
        ("color.blue",  vars<double>(), "Range of blue values")
        ("file,f",      var<string>(),  "File to be loaded")
        ("drs,d",       var<string>(),  "DRS calibration file to be loaded")
        ("mappingFile", var<string>(),  "Which pixels mapping file to use")
        ;
    conf.AddOptions(configs);

    po::positional_options_description p;
    p.add("file", 1); // The first positional options
    p.add("drs",  2); // The first positional options
    conf.SetArgumentPositions(p);

}

/************************************************************
 * MAIN PROGRAM FUNCTION.
 ************************************************************/
int main(int argc, const char *argv[])
{
    QApplication app(argc, const_cast<char**>(argv));

    if (!QGLFormat::hasOpenGL()) {
        std::cerr << "This system has no OpenGL support" << std::endl;
        return 1;
    }

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);
    if (!conf.DoParse(argc, argv, PrintHelp))
        return 2;

    UIConnector myUi;
    if (myUi.SetupConfiguration(conf)<0)
        return 3;

    return app.exec();
}

