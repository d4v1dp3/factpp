/*
 * QtGl.h
 *
 *  Created on: Jul 20, 2011
 *      Author: lyard
 */

#ifndef QTGL_H_
#define QTGL_H_

#define NBOARDS      40      // max. number of boards
#define NPIX       1440      // max. number of pixels
#define NTMARK      160      // max. number of timeMarker signals

#include <string>
#include <valarray>

#include <QObject>

#include "BasicGlCamera.h"

#include <qwt_plot_curve.h>

class QwtPlotZoomer;
class QwtPlotGrid;

#ifndef Q_MOC_RUN
#include "src/DataCalib.h"
#include "externals/ofits.h"
#endif

/*************************************************
 * Class Raw Data Viewer. FACT raw data diplayer
 *************************************************/
class RawDataViewer : public BasicGlCamera//QGLWidget
{
    Q_OBJECT

    friend class UIConnector;

    enum CalibDataTypes {
        CALIB_BASELINE,
        CALIB_GAIN,
        CALIB_TRIG_OFFSET
    };

public:
    DrsCalibration fDrsCalib;

    bool fIsDrsCalibration;

    RawDataViewer(QWidget *parent = 0);
    ~RawDataViewer();
    void openFile(std::string file, bool reopen);
    void openCalibFile(std::string& file);

    template <typename T>
    void getCalibrationDataForDisplay(const CalibDataTypes calibTypes,
                                                     const vector<T>& inputData,
                                                     const int roi,
                                                     const int roiTM);
    int getCurrentPixel(){return selectedPixel;}
    void assignPixelMapFile(const string& map="");
public Q_SLOTS:
    void plusEvent();
    void minusEvent();
    void readEvent();
    void setEventStep(int step);
    void nextSlice();
    void previousSlice();
    void setCurrentPixel(int);


Q_SIGNALS:
    void signalCurrentEvent(int event);
    void signalCurrentSlice(int slice);
    void newFileLoaded();
    void signalCurrentPixel(int pixel);
    void signalAutoScaleNeeded();

protected:
//    void initializeGL();
//    void resizeGL(int width, int height);
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void drawCamera(bool alsoWire);
//    void drawPatches();
//    int PixelAtPosition(const QPoint &pos);
//    void drawHexagon(int index, bool solid);
    int selectedPixel;
    vector<float> eventData;
    //vector<float> rmsData;
    vector<int16_t> rawEventData;
    vector<int16_t> waveLetArray;
    valarray<double> RMSvalues;//(1440);
    valarray<double> Meanvalues;
    valarray<double> Maxvalues;
    valarray<double> PosOfMaxvalues;

    ///Used to load zero data in case of missing fits columns
    //void allocateZeroArray();
    //char* fZeroArray;
    //bool telling whether the data is natively ordered in software or hardware id
    //used to correctly display monte-carlo data.
    bool _softwareOrdering;

private:
    void drawPixelCurve();
//    void updateNeighbors(int currentPixel);
//    void skipPixels(int start, int howMany);
//    void calculatePixelsCoords();
    bool setCorrectSlice(QMouseEvent* event);
    void eventStepping(bool plus);
//    void buildVerticesList();
//    void buildPatchesIndices();
    void calcBlurColor(int pixel, int vertex);
    void calcMidBlurColor(int pixel, int vertex);
    void drawBlurryHexagon(int index);
    int whichSlice;
 //   float shownSizex;
 //   float shownSizey;
    bool drawPatch;
    bool drawImpulse;
    bool drawBlur;
    bool loopCurrentEvent;
    //allocate the maximum size for one event
    uint32_t boardTime[NBOARDS];
    int16_t startPix[NPIX];
    int16_t startTM[NTMARK];
    int32_t pcTime[2];
    uint32_t softTrig;
    uint16_t triggerType;
    int nRows;
    int rowNum;
    int eventNum;
    int nRoi;
    int nRoiTM;
    int offSetRoi;
    int runNumber;
    int nTM;
    std::string runType;
    int firstDataTime;
    int lastDataTime;
    int revision;
    int builderVersion;
    int nBoards;
    int nPixels;
    std::string timeSystem;
    std::string creationDate;
    int nightInt;
    std::string camera;
    std::string daq;
    float adcCount;
    int nbOk;
    int nbRej;
    int nbBad;

    int eventStep;





//    int hardwareMapping[1440];
//    int softwareMapping[1440];
////    int patches[160][9];
    GLfloat patchesColor[160][3];
//    vector<edge> patchesIndices[160];
    fits* inputFile;
//    std::fits* calibInputFile;
//    float baseLineMean[1440*1024];
//    float gainMean[1440*1024];
//    float triggerOffsetMean[1440*1024];
//    bool calibrationLoaded;
//    bool drawCalibrationLoaded;

    QPoint lastPos;
public:
    /*
    void computePulsesStatistics();
    double aMeas[1024];
    double n1mean[1024];
    double n2mean[1024];
    double vCorr[1024];
    */
    int64_t VALUES_SPAN;

    void ApplyCalibration();

//    GLfloat pixelsCoords[MAX_NUM_PIXELS][3];
//    PixelsNeighbors neighbors[MAX_NUM_PIXELS];
 //   GLfloat pixelsColor[ACTUAL_NUM_PIXELS][3];
//    GLfloat verticesList[ACTUAL_NUM_PIXELS*6][2];
//    int verticesIndices[ACTUAL_NUM_PIXELS][6];
//    int numVertices;
};

/*************************************************
 * Class UIConnector. used to connect the interface to the raw data displayer
 *************************************************/
#include "ui_viewer.h"

class Configuration;

class UIConnector : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT
private:
    QTimer timer;
    std::string currentFile;
    std::string currentCalibFile;

    QRectF scaleBoundingRectangle(QRectF rectangle, float scale);

    bool updateSpinnerDisplay;
    bool updating;

    void initHistograms();

public:
    UIConnector(QWidget *parent = 0);
    ~UIConnector();

public Q_SLOTS:
    void fileSelected(QString file);
    void calibFileSelected(QString file);

    void newFileLoaded();
    void slicesPerSecondChanged(double value);
    void nextSlicePlease();
    void currentSliceHasChanged(int slice);
    void currentEventHasChanged(int event);

    void on_playPauseButton_clicked();
    void on_loadNewFileButton_clicked();
    void on_loadDRSCalibButton_clicked();

    void on_drawPatchCheckBox_stateChanged(int);
    void on_drawImpulseCheckBox_stateChanged(int);
    void on_drawBlurCheckBox_stateChanged(int);
    void on_loopOverCurrentEventBox_stateChanged(int);

    void on_colorRange0_valueChanged(double);
    void on_colorRange1_valueChanged(double);
    void on_colorRange2_valueChanged(double);
    void on_colorRange3_valueChanged(double);
    void on_colorRange4_valueChanged(double);
    void on_redValue0_valueChanged(double);
    void on_redValue1_valueChanged(double);
    void on_redValue2_valueChanged(double);
    void on_redValue3_valueChanged(double);
    void on_redValue4_valueChanged(double);
    void on_greenValue0_valueChanged(double);
    void on_greenValue1_valueChanged(double);
    void on_greenValue2_valueChanged(double);
    void on_greenValue3_valueChanged(double);
    void on_greenValue4_valueChanged(double);
    void on_blueValue0_valueChanged(double);
    void on_blueValue1_valueChanged(double);
    void on_blueValue2_valueChanged(double);
    void on_blueValue3_valueChanged(double);
    void on_blueValue4_valueChanged(double);

    void on_slicesPerSecValue_valueChanged(double);

    void pixelChanged(int);

    void cbpxChanged();

    void on_HwIDBox_valueChanged(int = 0);
    void on_SwIDBox_valueChanged(int);
    void on_crateIDBox_valueChanged(int) { cbpxChanged(); }
    void on_boardIDBox_valueChanged(int) { cbpxChanged(); }
    void on_patchIDBox_valueChanged(int) { cbpxChanged(); }
    void on_pixelIDBox_valueChanged(int) { cbpxChanged(); }

    void on_autoScaleColor_clicked();
    void on_entireCameraScale_toggled(bool) { on_autoScaleColor_clicked(); }
    void on_currentPixelScale_toggled(bool) { on_autoScaleColor_clicked(); }

    void slicesPlusPlus();
    void slicesMinusMinus();

    void on_calibratedCheckBox_stateChanged(int state);
    void on_displayingSliceBox_valueChanged(int);
    void on_displayingEventBox_valueChanged(int);

    void displaySliceValue();

    int SetupConfiguration(Configuration &conf);

private:
    QwtPlotCurve boardsTimeHistoItem;
    QwtPlotCurve startCellHistoItem;
    QwtPlotCurve startTimeMarkHistoItem;
    QwtPlotCurve pixelValueCurveItem;
    QwtPlotCurve pixelAverageCurveItem;
    QwtPlotCurve aMeanCurveItem;
    QwtPlotCurve vCorrCurveItem;
    QwtPlotCurve meanCurveItem;
    QwtPlotCurve triggerDelayHistoItem;

    QwtPlotZoomer* curveZoom;
    QwtPlotZoomer* averageCurveZoom;
    QwtPlotZoomer* boardsTimeHistoZoom;
    QwtPlotZoomer* startCellHistoZoom;
    QwtPlotZoomer* startTimeMarkHistoZoom;
    QwtPlotZoomer* triggerDelayHistoZoom;

    //declare the grids here, because I must have access to them to detach them properly at destruction time (bug and crash with "bad" versions of qwt)
    QwtPlotGrid* grid1;
    QwtPlotGrid* grid2;
    QwtPlotGrid* grid3;
    QwtPlotGrid* grid4;
    QwtPlotGrid* grid5;
    QwtPlotGrid* grid6;
};

#endif /* QTGL_H_ */
