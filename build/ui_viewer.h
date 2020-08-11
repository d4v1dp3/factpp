/********************************************************************************
** Form generated from reading UI file 'viewer.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIEWER_H
#define UI_VIEWER_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include <QtOpenGL/QGLWidget>
#include "../Q3DCameraWidget.h"
#include "../QCameraWidget.h"
#include "RawEventsViewer.h"
#include "qwt_plot.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QHBoxLayout *horizontalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QTabWidget *tabWidget_2;
    QWidget *tab_5;
    QGridLayout *gridLayout_3;
    RawDataViewer *GLWindow;
    QWidget *tab_6;
    QVBoxLayout *verticalLayout_2;
    QTabWidget *tabWidget_3;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_3;
    QListWidget *boardsTimeList;
    QLabel *label_13;
    QListWidget *startPixelsList;
    QLabel *StartTimeMarksList;
    QListWidget *startTimeMarksList;
    QLabel *label_4;
    QListWidget *triggerDelayList;
    QWidget *tab_4;
    QVBoxLayout *verticalLayout_3;
    QwtPlot *boardsTimeHisto;
    QwtPlot *startCellsHisto;
    QwtPlot *startTimeMarkHisto;
    QwtPlot *triggerDelayHisto;
    QWidget *tab;
    QGridLayout *gridLayout_4;
    QwtPlot *pixelValueCurve;
    QWidget *tab_7;
    QGridLayout *gridLayout_41;
    QwtPlot *pixelAverageCurve;
    QWidget *tab_15;
    QGridLayout *gridLayout_11;
    QTabWidget *tabWidget_4;
    QWidget *tab_16;
    QGridLayout *gridLayout_12;
    QCameraWidget *RMS_window;
    QWidget *tab_17;
    QGridLayout *gridLayout_13;
    QCameraWidget *Mean_window;
    QWidget *tab_18;
    QGridLayout *gridLayout_14;
    QCameraWidget *Max_window;
    QWidget *tab_19;
    QGridLayout *gridLayout_15;
    QCameraWidget *PosOfMax_window;
    QWidget *tab_11;
    QGridLayout *gridLayout_7;
    QTabWidget *tabWidget;
    QWidget *tab_12;
    QGridLayout *gridLayout_8;
    RawDataViewer *Baseline_window;
    QWidget *tab_13;
    QGridLayout *gridLayout_9;
    RawDataViewer *Gain_window;
    QWidget *tab_14;
    QGridLayout *gridLayout_10;
    RawDataViewer *TriggerOffset_window;
    QWidget *tab_2;
    QHBoxLayout *horizontalLayout_7;
    Q3DCameraWidget *threeD_Window;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLabel *extraInfoLabel;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_5;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_10;
    QSpinBox *HwIDBox;
    QLabel *label_11;
    QSpinBox *SwIDBox;
    QGridLayout *gridLayout_5;
    QLabel *label_6;
    QLabel *label_8;
    QLabel *label_9;
    QSpinBox *crateIDBox;
    QSpinBox *boardIDBox;
    QSpinBox *patchIDBox;
    QSpinBox *pixelIDBox;
    QLabel *label_2;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_7;
    QHBoxLayout *horizontalLayout_10;
    QLabel *displayingEventLabel;
    QSpinBox *displayingEventBox;
    QLabel *displayingSliceLabel;
    QSpinBox *displayingSliceBox;
    QLabel *currentPixelValue;
    QLabel *triggerTypeLabel;
    QLabel *softwareTriggerLabel;
    QLabel *PCTimeLabel;
    QGridLayout *gridLayout;
    QDoubleSpinBox *colorRange4;
    QDoubleSpinBox *greenValue4;
    QDoubleSpinBox *redValue4;
    QDoubleSpinBox *blueValue4;
    QDoubleSpinBox *redValue3;
    QDoubleSpinBox *redValue2;
    QDoubleSpinBox *redValue1;
    QDoubleSpinBox *redValue0;
    QLabel *label_20;
    QLabel *label_21;
    QLabel *label_22;
    QLabel *label_23;
    QDoubleSpinBox *colorRange0;
    QDoubleSpinBox *greenValue0;
    QDoubleSpinBox *blueValue0;
    QDoubleSpinBox *colorRange1;
    QDoubleSpinBox *greenValue1;
    QDoubleSpinBox *blueValue1;
    QDoubleSpinBox *colorRange2;
    QDoubleSpinBox *greenValue2;
    QDoubleSpinBox *blueValue2;
    QDoubleSpinBox *colorRange3;
    QDoubleSpinBox *greenValue3;
    QDoubleSpinBox *blueValue3;
    QHBoxLayout *horizontalLayout;
    QPushButton *playPauseButton;
    QLabel *label_24;
    QDoubleSpinBox *slicesPerSecValue;
    QCheckBox *drawImpulseCheckBox;
    QCheckBox *drawBlurCheckBox;
    QCheckBox *drawPatchCheckBox;
    QCheckBox *loopOverCurrentEventBox;
    QPushButton *autoScaleColor;
    QRadioButton *entireCameraScale;
    QRadioButton *currentPixelScale;
    QHBoxLayout *horizontalLayout_5;
    QRadioButton *playSlicesRadio;
    QRadioButton *playEventsRadio;
    QRadioButton *playPixelsRadio;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *loadNewFileButton;
    QPushButton *loadDRSCalibButton;
    QCheckBox *calibratedCheckBox;
    QHBoxLayout *horizontalLayout_4;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(934, 786);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setSizeConstraint(QLayout::SetDefaultConstraint);
        tabWidget_2 = new QTabWidget(centralwidget);
        tabWidget_2->setObjectName(QString::fromUtf8("tabWidget_2"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(tabWidget_2->sizePolicy().hasHeightForWidth());
        tabWidget_2->setSizePolicy(sizePolicy);
        tab_5 = new QWidget();
        tab_5->setObjectName(QString::fromUtf8("tab_5"));
        tab_5->setLayoutDirection(Qt::LeftToRight);
        gridLayout_3 = new QGridLayout(tab_5);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        GLWindow = new RawDataViewer(tab_5);
        GLWindow->setObjectName(QString::fromUtf8("GLWindow"));
        GLWindow->setEnabled(true);
        sizePolicy.setHeightForWidth(GLWindow->sizePolicy().hasHeightForWidth());
        GLWindow->setSizePolicy(sizePolicy);

        gridLayout_3->addWidget(GLWindow, 0, 0, 1, 1);

        tabWidget_2->addTab(tab_5, QString());
        tab_6 = new QWidget();
        tab_6->setObjectName(QString::fromUtf8("tab_6"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(tab_6->sizePolicy().hasHeightForWidth());
        tab_6->setSizePolicy(sizePolicy1);
        verticalLayout_2 = new QVBoxLayout(tab_6);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        tabWidget_3 = new QTabWidget(tab_6);
        tabWidget_3->setObjectName(QString::fromUtf8("tabWidget_3"));
        sizePolicy.setHeightForWidth(tabWidget_3->sizePolicy().hasHeightForWidth());
        tabWidget_3->setSizePolicy(sizePolicy);
        tabWidget_3->setTabShape(QTabWidget::Rounded);
        tabWidget_3->setElideMode(Qt::ElideNone);
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(tab_3->sizePolicy().hasHeightForWidth());
        tab_3->setSizePolicy(sizePolicy2);
        verticalLayout_6 = new QVBoxLayout(tab_3);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        label_3 = new QLabel(tab_3);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        verticalLayout_6->addWidget(label_3);

        boardsTimeList = new QListWidget(tab_3);
        boardsTimeList->setObjectName(QString::fromUtf8("boardsTimeList"));
        QFont font;
        font.setFamily(QString::fromUtf8("FreeMono"));
        boardsTimeList->setFont(font);

        verticalLayout_6->addWidget(boardsTimeList);

        label_13 = new QLabel(tab_3);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        verticalLayout_6->addWidget(label_13);

        startPixelsList = new QListWidget(tab_3);
        startPixelsList->setObjectName(QString::fromUtf8("startPixelsList"));
        startPixelsList->setFont(font);

        verticalLayout_6->addWidget(startPixelsList);

        StartTimeMarksList = new QLabel(tab_3);
        StartTimeMarksList->setObjectName(QString::fromUtf8("StartTimeMarksList"));

        verticalLayout_6->addWidget(StartTimeMarksList);

        startTimeMarksList = new QListWidget(tab_3);
        startTimeMarksList->setObjectName(QString::fromUtf8("startTimeMarksList"));
        sizePolicy2.setHeightForWidth(startTimeMarksList->sizePolicy().hasHeightForWidth());
        startTimeMarksList->setSizePolicy(sizePolicy2);
        startTimeMarksList->setFont(font);

        verticalLayout_6->addWidget(startTimeMarksList);

        label_4 = new QLabel(tab_3);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        verticalLayout_6->addWidget(label_4);

        triggerDelayList = new QListWidget(tab_3);
        triggerDelayList->setObjectName(QString::fromUtf8("triggerDelayList"));

        verticalLayout_6->addWidget(triggerDelayList);

        tabWidget_3->addTab(tab_3, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QString::fromUtf8("tab_4"));
        verticalLayout_3 = new QVBoxLayout(tab_4);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        boardsTimeHisto = new QwtPlot(tab_4);
        boardsTimeHisto->setObjectName(QString::fromUtf8("boardsTimeHisto"));

        verticalLayout_3->addWidget(boardsTimeHisto);

        startCellsHisto = new QwtPlot(tab_4);
        startCellsHisto->setObjectName(QString::fromUtf8("startCellsHisto"));

        verticalLayout_3->addWidget(startCellsHisto);

        startTimeMarkHisto = new QwtPlot(tab_4);
        startTimeMarkHisto->setObjectName(QString::fromUtf8("startTimeMarkHisto"));

        verticalLayout_3->addWidget(startTimeMarkHisto);

        triggerDelayHisto = new QwtPlot(tab_4);
        triggerDelayHisto->setObjectName(QString::fromUtf8("triggerDelayHisto"));

        verticalLayout_3->addWidget(triggerDelayHisto);

        tabWidget_3->addTab(tab_4, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout_4 = new QGridLayout(tab);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        pixelValueCurve = new QwtPlot(tab);
        pixelValueCurve->setObjectName(QString::fromUtf8("pixelValueCurve"));

        gridLayout_4->addWidget(pixelValueCurve, 0, 0, 1, 1);

        tabWidget_3->addTab(tab, QString());
        tab_7 = new QWidget();
        tab_7->setObjectName(QString::fromUtf8("tab_7"));
        gridLayout_41 = new QGridLayout(tab_7);
        gridLayout_41->setObjectName(QString::fromUtf8("gridLayout_41"));
        pixelAverageCurve = new QwtPlot(tab_7);
        pixelAverageCurve->setObjectName(QString::fromUtf8("pixelAverageCurve"));

        gridLayout_41->addWidget(pixelAverageCurve, 0, 0, 1, 1);

        tabWidget_3->addTab(tab_7, QString());

        verticalLayout_2->addWidget(tabWidget_3);

        tabWidget_2->addTab(tab_6, QString());
        tab_15 = new QWidget();
        tab_15->setObjectName(QString::fromUtf8("tab_15"));
        gridLayout_11 = new QGridLayout(tab_15);
        gridLayout_11->setObjectName(QString::fromUtf8("gridLayout_11"));
        tabWidget_4 = new QTabWidget(tab_15);
        tabWidget_4->setObjectName(QString::fromUtf8("tabWidget_4"));
        tab_16 = new QWidget();
        tab_16->setObjectName(QString::fromUtf8("tab_16"));
        gridLayout_12 = new QGridLayout(tab_16);
        gridLayout_12->setObjectName(QString::fromUtf8("gridLayout_12"));
        RMS_window = new QCameraWidget(tab_16);
        RMS_window->setObjectName(QString::fromUtf8("RMS_window"));
        RMS_window->setEnabled(true);
        sizePolicy2.setHeightForWidth(RMS_window->sizePolicy().hasHeightForWidth());
        RMS_window->setSizePolicy(sizePolicy2);
        RMS_window->setMaximumSize(QSize(10000, 10000));

        gridLayout_12->addWidget(RMS_window, 0, 0, 1, 1);

        tabWidget_4->addTab(tab_16, QString());
        tab_17 = new QWidget();
        tab_17->setObjectName(QString::fromUtf8("tab_17"));
        gridLayout_13 = new QGridLayout(tab_17);
        gridLayout_13->setObjectName(QString::fromUtf8("gridLayout_13"));
        Mean_window = new QCameraWidget(tab_17);
        Mean_window->setObjectName(QString::fromUtf8("Mean_window"));
        Mean_window->setEnabled(true);
        sizePolicy2.setHeightForWidth(Mean_window->sizePolicy().hasHeightForWidth());
        Mean_window->setSizePolicy(sizePolicy2);
        Mean_window->setMaximumSize(QSize(10000, 10000));

        gridLayout_13->addWidget(Mean_window, 0, 0, 1, 1);

        tabWidget_4->addTab(tab_17, QString());
        tab_18 = new QWidget();
        tab_18->setObjectName(QString::fromUtf8("tab_18"));
        gridLayout_14 = new QGridLayout(tab_18);
        gridLayout_14->setObjectName(QString::fromUtf8("gridLayout_14"));
        Max_window = new QCameraWidget(tab_18);
        Max_window->setObjectName(QString::fromUtf8("Max_window"));
        Max_window->setEnabled(true);
        sizePolicy2.setHeightForWidth(Max_window->sizePolicy().hasHeightForWidth());
        Max_window->setSizePolicy(sizePolicy2);
        Max_window->setMaximumSize(QSize(10000, 10000));

        gridLayout_14->addWidget(Max_window, 0, 0, 1, 1);

        tabWidget_4->addTab(tab_18, QString());
        tab_19 = new QWidget();
        tab_19->setObjectName(QString::fromUtf8("tab_19"));
        gridLayout_15 = new QGridLayout(tab_19);
        gridLayout_15->setObjectName(QString::fromUtf8("gridLayout_15"));
        PosOfMax_window = new QCameraWidget(tab_19);
        PosOfMax_window->setObjectName(QString::fromUtf8("PosOfMax_window"));
        PosOfMax_window->setEnabled(true);
        sizePolicy2.setHeightForWidth(PosOfMax_window->sizePolicy().hasHeightForWidth());
        PosOfMax_window->setSizePolicy(sizePolicy2);
        PosOfMax_window->setMaximumSize(QSize(10000, 10000));

        gridLayout_15->addWidget(PosOfMax_window, 0, 0, 1, 1);

        tabWidget_4->addTab(tab_19, QString());

        gridLayout_11->addWidget(tabWidget_4, 0, 0, 1, 1);

        tabWidget_2->addTab(tab_15, QString());
        tab_11 = new QWidget();
        tab_11->setObjectName(QString::fromUtf8("tab_11"));
        gridLayout_7 = new QGridLayout(tab_11);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        tabWidget = new QTabWidget(tab_11);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tab_12 = new QWidget();
        tab_12->setObjectName(QString::fromUtf8("tab_12"));
        gridLayout_8 = new QGridLayout(tab_12);
        gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
        Baseline_window = new RawDataViewer(tab_12);
        Baseline_window->setObjectName(QString::fromUtf8("Baseline_window"));
        Baseline_window->setEnabled(true);
        sizePolicy.setHeightForWidth(Baseline_window->sizePolicy().hasHeightForWidth());
        Baseline_window->setSizePolicy(sizePolicy);

        gridLayout_8->addWidget(Baseline_window, 0, 0, 1, 1);

        tabWidget->addTab(tab_12, QString());
        tab_13 = new QWidget();
        tab_13->setObjectName(QString::fromUtf8("tab_13"));
        gridLayout_9 = new QGridLayout(tab_13);
        gridLayout_9->setObjectName(QString::fromUtf8("gridLayout_9"));
        Gain_window = new RawDataViewer(tab_13);
        Gain_window->setObjectName(QString::fromUtf8("Gain_window"));
        Gain_window->setEnabled(true);
        sizePolicy.setHeightForWidth(Gain_window->sizePolicy().hasHeightForWidth());
        Gain_window->setSizePolicy(sizePolicy);

        gridLayout_9->addWidget(Gain_window, 0, 0, 1, 1);

        tabWidget->addTab(tab_13, QString());
        tab_14 = new QWidget();
        tab_14->setObjectName(QString::fromUtf8("tab_14"));
        gridLayout_10 = new QGridLayout(tab_14);
        gridLayout_10->setObjectName(QString::fromUtf8("gridLayout_10"));
        TriggerOffset_window = new RawDataViewer(tab_14);
        TriggerOffset_window->setObjectName(QString::fromUtf8("TriggerOffset_window"));
        TriggerOffset_window->setEnabled(true);
        sizePolicy.setHeightForWidth(TriggerOffset_window->sizePolicy().hasHeightForWidth());
        TriggerOffset_window->setSizePolicy(sizePolicy);

        gridLayout_10->addWidget(TriggerOffset_window, 0, 0, 1, 1);

        tabWidget->addTab(tab_14, QString());

        gridLayout_7->addWidget(tabWidget, 0, 0, 1, 1);

        tabWidget_2->addTab(tab_11, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        horizontalLayout_7 = new QHBoxLayout(tab_2);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        threeD_Window = new Q3DCameraWidget(tab_2);
        threeD_Window->setObjectName(QString::fromUtf8("threeD_Window"));
        threeD_Window->setEnabled(true);
        sizePolicy2.setHeightForWidth(threeD_Window->sizePolicy().hasHeightForWidth());
        threeD_Window->setSizePolicy(sizePolicy2);
        threeD_Window->setMaximumSize(QSize(10000, 10000));

        horizontalLayout_7->addWidget(threeD_Window);

        tabWidget_2->addTab(tab_2, QString());

        horizontalLayout_3->addWidget(tabWidget_2);


        horizontalLayout_2->addLayout(horizontalLayout_3);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        extraInfoLabel = new QLabel(centralwidget);
        extraInfoLabel->setObjectName(QString::fromUtf8("extraInfoLabel"));
        sizePolicy2.setHeightForWidth(extraInfoLabel->sizePolicy().hasHeightForWidth());
        extraInfoLabel->setSizePolicy(sizePolicy2);
        QFont font1;
        font1.setPointSize(8);
        extraInfoLabel->setFont(font1);
        extraInfoLabel->setScaledContents(false);
        extraInfoLabel->setWordWrap(true);

        verticalLayout->addWidget(extraInfoLabel);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        QFont font2;
        font2.setPointSize(11);
        font2.setBold(true);
        font2.setWeight(75);
        label_5->setFont(font2);
        label_5->setAlignment(Qt::AlignCenter);

        verticalLayout_5->addWidget(label_5);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        label_10 = new QLabel(centralwidget);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_6->addWidget(label_10);

        HwIDBox = new QSpinBox(centralwidget);
        HwIDBox->setObjectName(QString::fromUtf8("HwIDBox"));
        HwIDBox->setMaximum(10000);
        HwIDBox->setValue(393);

        horizontalLayout_6->addWidget(HwIDBox);

        label_11 = new QLabel(centralwidget);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_6->addWidget(label_11);

        SwIDBox = new QSpinBox(centralwidget);
        SwIDBox->setObjectName(QString::fromUtf8("SwIDBox"));
        SwIDBox->setMaximum(1440);

        horizontalLayout_6->addWidget(SwIDBox);


        verticalLayout_5->addLayout(horizontalLayout_6);

        gridLayout_5 = new QGridLayout();
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        gridLayout_5->addWidget(label_6, 2, 0, 1, 1);

        label_8 = new QLabel(centralwidget);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        gridLayout_5->addWidget(label_8, 2, 1, 1, 1);

        label_9 = new QLabel(centralwidget);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        gridLayout_5->addWidget(label_9, 2, 2, 1, 1);

        crateIDBox = new QSpinBox(centralwidget);
        crateIDBox->setObjectName(QString::fromUtf8("crateIDBox"));
        crateIDBox->setMaximum(3);
        crateIDBox->setValue(0);

        gridLayout_5->addWidget(crateIDBox, 3, 0, 1, 1);

        boardIDBox = new QSpinBox(centralwidget);
        boardIDBox->setObjectName(QString::fromUtf8("boardIDBox"));
        boardIDBox->setMaximum(9);
        boardIDBox->setValue(0);

        gridLayout_5->addWidget(boardIDBox, 3, 1, 1, 1);

        patchIDBox = new QSpinBox(centralwidget);
        patchIDBox->setObjectName(QString::fromUtf8("patchIDBox"));
        patchIDBox->setMaximum(3);
        patchIDBox->setValue(0);

        gridLayout_5->addWidget(patchIDBox, 3, 2, 1, 1);

        pixelIDBox = new QSpinBox(centralwidget);
        pixelIDBox->setObjectName(QString::fromUtf8("pixelIDBox"));
        pixelIDBox->setMaximum(8);
        pixelIDBox->setValue(0);

        gridLayout_5->addWidget(pixelIDBox, 3, 3, 1, 1);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_5->addWidget(label_2, 2, 3, 1, 1);


        verticalLayout_5->addLayout(gridLayout_5);


        verticalLayout->addLayout(verticalLayout_5);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(2);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        QFont font3;
        font3.setPointSize(12);
        font3.setBold(true);
        font3.setWeight(75);
        label_7->setFont(font3);
        label_7->setAlignment(Qt::AlignCenter);

        verticalLayout_4->addWidget(label_7);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        displayingEventLabel = new QLabel(centralwidget);
        displayingEventLabel->setObjectName(QString::fromUtf8("displayingEventLabel"));

        horizontalLayout_10->addWidget(displayingEventLabel);

        displayingEventBox = new QSpinBox(centralwidget);
        displayingEventBox->setObjectName(QString::fromUtf8("displayingEventBox"));

        horizontalLayout_10->addWidget(displayingEventBox);

        displayingSliceLabel = new QLabel(centralwidget);
        displayingSliceLabel->setObjectName(QString::fromUtf8("displayingSliceLabel"));

        horizontalLayout_10->addWidget(displayingSliceLabel);

        displayingSliceBox = new QSpinBox(centralwidget);
        displayingSliceBox->setObjectName(QString::fromUtf8("displayingSliceBox"));
        displayingSliceBox->setMaximum(1023);

        horizontalLayout_10->addWidget(displayingSliceBox);


        verticalLayout_4->addLayout(horizontalLayout_10);

        currentPixelValue = new QLabel(centralwidget);
        currentPixelValue->setObjectName(QString::fromUtf8("currentPixelValue"));

        verticalLayout_4->addWidget(currentPixelValue);

        triggerTypeLabel = new QLabel(centralwidget);
        triggerTypeLabel->setObjectName(QString::fromUtf8("triggerTypeLabel"));

        verticalLayout_4->addWidget(triggerTypeLabel);

        softwareTriggerLabel = new QLabel(centralwidget);
        softwareTriggerLabel->setObjectName(QString::fromUtf8("softwareTriggerLabel"));

        verticalLayout_4->addWidget(softwareTriggerLabel);

        PCTimeLabel = new QLabel(centralwidget);
        PCTimeLabel->setObjectName(QString::fromUtf8("PCTimeLabel"));

        verticalLayout_4->addWidget(PCTimeLabel);


        verticalLayout->addLayout(verticalLayout_4);


        horizontalLayout_2->addLayout(verticalLayout);

        horizontalLayout_2->setStretch(0, 10);

        gridLayout_2->addLayout(horizontalLayout_2, 0, 0, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        colorRange4 = new QDoubleSpinBox(centralwidget);
        colorRange4->setObjectName(QString::fromUtf8("colorRange4"));
        colorRange4->setEnabled(true);
        colorRange4->setDecimals(3);
        colorRange4->setMaximum(1);
        colorRange4->setSingleStep(0.05);

        gridLayout->addWidget(colorRange4, 0, 7, 1, 1);

        greenValue4 = new QDoubleSpinBox(centralwidget);
        greenValue4->setObjectName(QString::fromUtf8("greenValue4"));
        greenValue4->setDecimals(3);
        greenValue4->setMaximum(1);
        greenValue4->setSingleStep(0.05);

        gridLayout->addWidget(greenValue4, 2, 7, 1, 1);

        redValue4 = new QDoubleSpinBox(centralwidget);
        redValue4->setObjectName(QString::fromUtf8("redValue4"));
        redValue4->setDecimals(3);
        redValue4->setMaximum(1);
        redValue4->setSingleStep(0.05);

        gridLayout->addWidget(redValue4, 1, 7, 1, 1);

        blueValue4 = new QDoubleSpinBox(centralwidget);
        blueValue4->setObjectName(QString::fromUtf8("blueValue4"));
        blueValue4->setDecimals(3);
        blueValue4->setMaximum(1);
        blueValue4->setSingleStep(0.05);

        gridLayout->addWidget(blueValue4, 3, 7, 1, 1);

        redValue3 = new QDoubleSpinBox(centralwidget);
        redValue3->setObjectName(QString::fromUtf8("redValue3"));
        redValue3->setDecimals(3);
        redValue3->setMaximum(1);
        redValue3->setSingleStep(0.05);

        gridLayout->addWidget(redValue3, 1, 6, 1, 1);

        redValue2 = new QDoubleSpinBox(centralwidget);
        redValue2->setObjectName(QString::fromUtf8("redValue2"));
        redValue2->setDecimals(3);
        redValue2->setMaximum(1);
        redValue2->setSingleStep(0.05);

        gridLayout->addWidget(redValue2, 1, 5, 1, 1);

        redValue1 = new QDoubleSpinBox(centralwidget);
        redValue1->setObjectName(QString::fromUtf8("redValue1"));
        redValue1->setDecimals(3);
        redValue1->setMaximum(1);
        redValue1->setSingleStep(0.05);

        gridLayout->addWidget(redValue1, 1, 4, 1, 1);

        redValue0 = new QDoubleSpinBox(centralwidget);
        redValue0->setObjectName(QString::fromUtf8("redValue0"));
        redValue0->setDecimals(3);
        redValue0->setMaximum(1);
        redValue0->setSingleStep(0.05);

        gridLayout->addWidget(redValue0, 1, 3, 1, 1);

        label_20 = new QLabel(centralwidget);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_20, 1, 2, 1, 1);

        label_21 = new QLabel(centralwidget);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        label_21->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_21, 0, 2, 1, 1);

        label_22 = new QLabel(centralwidget);
        label_22->setObjectName(QString::fromUtf8("label_22"));
        label_22->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_22, 2, 2, 1, 1);

        label_23 = new QLabel(centralwidget);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_23, 3, 2, 1, 1);

        colorRange0 = new QDoubleSpinBox(centralwidget);
        colorRange0->setObjectName(QString::fromUtf8("colorRange0"));
        colorRange0->setEnabled(true);
        colorRange0->setDecimals(3);
        colorRange0->setMaximum(1);
        colorRange0->setSingleStep(0.05);

        gridLayout->addWidget(colorRange0, 0, 3, 1, 1);

        greenValue0 = new QDoubleSpinBox(centralwidget);
        greenValue0->setObjectName(QString::fromUtf8("greenValue0"));
        greenValue0->setDecimals(3);
        greenValue0->setMaximum(1);
        greenValue0->setSingleStep(0.05);

        gridLayout->addWidget(greenValue0, 2, 3, 1, 1);

        blueValue0 = new QDoubleSpinBox(centralwidget);
        blueValue0->setObjectName(QString::fromUtf8("blueValue0"));
        blueValue0->setDecimals(3);
        blueValue0->setMaximum(1);
        blueValue0->setSingleStep(0.05);

        gridLayout->addWidget(blueValue0, 3, 3, 1, 1);

        colorRange1 = new QDoubleSpinBox(centralwidget);
        colorRange1->setObjectName(QString::fromUtf8("colorRange1"));
        colorRange1->setDecimals(3);
        colorRange1->setMaximum(1);
        colorRange1->setSingleStep(0.05);

        gridLayout->addWidget(colorRange1, 0, 4, 1, 1);

        greenValue1 = new QDoubleSpinBox(centralwidget);
        greenValue1->setObjectName(QString::fromUtf8("greenValue1"));
        greenValue1->setDecimals(3);
        greenValue1->setMaximum(1);
        greenValue1->setSingleStep(0.05);

        gridLayout->addWidget(greenValue1, 2, 4, 1, 1);

        blueValue1 = new QDoubleSpinBox(centralwidget);
        blueValue1->setObjectName(QString::fromUtf8("blueValue1"));
        blueValue1->setDecimals(3);
        blueValue1->setMaximum(1);
        blueValue1->setSingleStep(0.05);

        gridLayout->addWidget(blueValue1, 3, 4, 1, 1);

        colorRange2 = new QDoubleSpinBox(centralwidget);
        colorRange2->setObjectName(QString::fromUtf8("colorRange2"));
        colorRange2->setDecimals(3);
        colorRange2->setMaximum(1);
        colorRange2->setSingleStep(0.05);

        gridLayout->addWidget(colorRange2, 0, 5, 1, 1);

        greenValue2 = new QDoubleSpinBox(centralwidget);
        greenValue2->setObjectName(QString::fromUtf8("greenValue2"));
        greenValue2->setDecimals(3);
        greenValue2->setMaximum(1);
        greenValue2->setSingleStep(0.05);

        gridLayout->addWidget(greenValue2, 2, 5, 1, 1);

        blueValue2 = new QDoubleSpinBox(centralwidget);
        blueValue2->setObjectName(QString::fromUtf8("blueValue2"));
        blueValue2->setDecimals(3);
        blueValue2->setMaximum(1);
        blueValue2->setSingleStep(0.05);

        gridLayout->addWidget(blueValue2, 3, 5, 1, 1);

        colorRange3 = new QDoubleSpinBox(centralwidget);
        colorRange3->setObjectName(QString::fromUtf8("colorRange3"));
        colorRange3->setDecimals(3);
        colorRange3->setMaximum(1);
        colorRange3->setSingleStep(0.05);

        gridLayout->addWidget(colorRange3, 0, 6, 1, 1);

        greenValue3 = new QDoubleSpinBox(centralwidget);
        greenValue3->setObjectName(QString::fromUtf8("greenValue3"));
        greenValue3->setDecimals(3);
        greenValue3->setMaximum(1);
        greenValue3->setSingleStep(0.05);

        gridLayout->addWidget(greenValue3, 2, 6, 1, 1);

        blueValue3 = new QDoubleSpinBox(centralwidget);
        blueValue3->setObjectName(QString::fromUtf8("blueValue3"));
        blueValue3->setDecimals(3);
        blueValue3->setMaximum(1);
        blueValue3->setSingleStep(0.05);

        gridLayout->addWidget(blueValue3, 3, 6, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        playPauseButton = new QPushButton(centralwidget);
        playPauseButton->setObjectName(QString::fromUtf8("playPauseButton"));

        horizontalLayout->addWidget(playPauseButton);

        label_24 = new QLabel(centralwidget);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(label_24);

        slicesPerSecValue = new QDoubleSpinBox(centralwidget);
        slicesPerSecValue->setObjectName(QString::fromUtf8("slicesPerSecValue"));
        slicesPerSecValue->setMinimum(1);
        slicesPerSecValue->setMaximum(1000);
        slicesPerSecValue->setSingleStep(1);
        slicesPerSecValue->setValue(100);

        horizontalLayout->addWidget(slicesPerSecValue);


        gridLayout->addLayout(horizontalLayout, 3, 8, 1, 1);

        drawImpulseCheckBox = new QCheckBox(centralwidget);
        drawImpulseCheckBox->setObjectName(QString::fromUtf8("drawImpulseCheckBox"));
        drawImpulseCheckBox->setLayoutDirection(Qt::RightToLeft);
        drawImpulseCheckBox->setChecked(true);

        gridLayout->addWidget(drawImpulseCheckBox, 0, 0, 1, 1);

        drawBlurCheckBox = new QCheckBox(centralwidget);
        drawBlurCheckBox->setObjectName(QString::fromUtf8("drawBlurCheckBox"));
        drawBlurCheckBox->setLayoutDirection(Qt::RightToLeft);

        gridLayout->addWidget(drawBlurCheckBox, 3, 0, 1, 1);

        drawPatchCheckBox = new QCheckBox(centralwidget);
        drawPatchCheckBox->setObjectName(QString::fromUtf8("drawPatchCheckBox"));
        drawPatchCheckBox->setLayoutDirection(Qt::RightToLeft);
        drawPatchCheckBox->setChecked(false);
        drawPatchCheckBox->setTristate(false);

        gridLayout->addWidget(drawPatchCheckBox, 1, 0, 1, 1);

        loopOverCurrentEventBox = new QCheckBox(centralwidget);
        loopOverCurrentEventBox->setObjectName(QString::fromUtf8("loopOverCurrentEventBox"));
        loopOverCurrentEventBox->setLayoutDirection(Qt::RightToLeft);

        gridLayout->addWidget(loopOverCurrentEventBox, 2, 0, 1, 1);

        autoScaleColor = new QPushButton(centralwidget);
        autoScaleColor->setObjectName(QString::fromUtf8("autoScaleColor"));
        autoScaleColor->setCheckable(true);
        autoScaleColor->setChecked(true);

        gridLayout->addWidget(autoScaleColor, 1, 1, 1, 1);

        entireCameraScale = new QRadioButton(centralwidget);
        entireCameraScale->setObjectName(QString::fromUtf8("entireCameraScale"));
        entireCameraScale->setLayoutDirection(Qt::RightToLeft);
        entireCameraScale->setChecked(true);

        gridLayout->addWidget(entireCameraScale, 2, 1, 1, 1);

        currentPixelScale = new QRadioButton(centralwidget);
        currentPixelScale->setObjectName(QString::fromUtf8("currentPixelScale"));
        currentPixelScale->setLayoutDirection(Qt::RightToLeft);

        gridLayout->addWidget(currentPixelScale, 3, 1, 1, 1);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        playSlicesRadio = new QRadioButton(centralwidget);
        playSlicesRadio->setObjectName(QString::fromUtf8("playSlicesRadio"));
        playSlicesRadio->setChecked(true);

        horizontalLayout_5->addWidget(playSlicesRadio);

        playEventsRadio = new QRadioButton(centralwidget);
        playEventsRadio->setObjectName(QString::fromUtf8("playEventsRadio"));

        horizontalLayout_5->addWidget(playEventsRadio);

        playPixelsRadio = new QRadioButton(centralwidget);
        playPixelsRadio->setObjectName(QString::fromUtf8("playPixelsRadio"));

        horizontalLayout_5->addWidget(playPixelsRadio);


        gridLayout->addLayout(horizontalLayout_5, 2, 8, 1, 1);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        loadNewFileButton = new QPushButton(centralwidget);
        loadNewFileButton->setObjectName(QString::fromUtf8("loadNewFileButton"));

        horizontalLayout_8->addWidget(loadNewFileButton);

        loadDRSCalibButton = new QPushButton(centralwidget);
        loadDRSCalibButton->setObjectName(QString::fromUtf8("loadDRSCalibButton"));

        horizontalLayout_8->addWidget(loadDRSCalibButton);


        gridLayout->addLayout(horizontalLayout_8, 0, 8, 1, 1);

        calibratedCheckBox = new QCheckBox(centralwidget);
        calibratedCheckBox->setObjectName(QString::fromUtf8("calibratedCheckBox"));
        calibratedCheckBox->setLayoutDirection(Qt::RightToLeft);
        calibratedCheckBox->setChecked(false);

        gridLayout->addWidget(calibratedCheckBox, 0, 1, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));

        gridLayout->addLayout(horizontalLayout_4, 1, 8, 1, 1);


        gridLayout_2->addLayout(gridLayout, 1, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        QWidget::setTabOrder(tabWidget_2, HwIDBox);
        QWidget::setTabOrder(HwIDBox, SwIDBox);
        QWidget::setTabOrder(SwIDBox, crateIDBox);
        QWidget::setTabOrder(crateIDBox, boardIDBox);
        QWidget::setTabOrder(boardIDBox, patchIDBox);
        QWidget::setTabOrder(patchIDBox, pixelIDBox);
        QWidget::setTabOrder(pixelIDBox, displayingEventBox);
        QWidget::setTabOrder(displayingEventBox, displayingSliceBox);
        QWidget::setTabOrder(displayingSliceBox, drawImpulseCheckBox);
        QWidget::setTabOrder(drawImpulseCheckBox, calibratedCheckBox);
        QWidget::setTabOrder(calibratedCheckBox, drawPatchCheckBox);
        QWidget::setTabOrder(drawPatchCheckBox, autoScaleColor);
        QWidget::setTabOrder(autoScaleColor, loopOverCurrentEventBox);
        QWidget::setTabOrder(loopOverCurrentEventBox, entireCameraScale);
        QWidget::setTabOrder(entireCameraScale, drawBlurCheckBox);
        QWidget::setTabOrder(drawBlurCheckBox, currentPixelScale);
        QWidget::setTabOrder(currentPixelScale, colorRange0);
        QWidget::setTabOrder(colorRange0, colorRange1);
        QWidget::setTabOrder(colorRange1, colorRange2);
        QWidget::setTabOrder(colorRange2, colorRange3);
        QWidget::setTabOrder(colorRange3, colorRange4);
        QWidget::setTabOrder(colorRange4, redValue0);
        QWidget::setTabOrder(redValue0, redValue1);
        QWidget::setTabOrder(redValue1, redValue2);
        QWidget::setTabOrder(redValue2, redValue3);
        QWidget::setTabOrder(redValue3, redValue4);
        QWidget::setTabOrder(redValue4, greenValue0);
        QWidget::setTabOrder(greenValue0, greenValue1);
        QWidget::setTabOrder(greenValue1, greenValue2);
        QWidget::setTabOrder(greenValue2, greenValue3);
        QWidget::setTabOrder(greenValue3, greenValue4);
        QWidget::setTabOrder(greenValue4, blueValue0);
        QWidget::setTabOrder(blueValue0, blueValue1);
        QWidget::setTabOrder(blueValue1, blueValue2);
        QWidget::setTabOrder(blueValue2, blueValue3);
        QWidget::setTabOrder(blueValue3, blueValue4);
        QWidget::setTabOrder(blueValue4, loadNewFileButton);
        QWidget::setTabOrder(loadNewFileButton, loadDRSCalibButton);
        QWidget::setTabOrder(loadDRSCalibButton, playSlicesRadio);
        QWidget::setTabOrder(playSlicesRadio, playEventsRadio);
        QWidget::setTabOrder(playEventsRadio, playPixelsRadio);
        QWidget::setTabOrder(playPixelsRadio, playPauseButton);
        QWidget::setTabOrder(playPauseButton, slicesPerSecValue);
        QWidget::setTabOrder(slicesPerSecValue, triggerDelayList);
        QWidget::setTabOrder(triggerDelayList, tabWidget_3);
        QWidget::setTabOrder(tabWidget_3, startTimeMarksList);
        QWidget::setTabOrder(startTimeMarksList, tabWidget_4);
        QWidget::setTabOrder(tabWidget_4, tabWidget);
        QWidget::setTabOrder(tabWidget, boardsTimeList);
        QWidget::setTabOrder(boardsTimeList, startPixelsList);

        retranslateUi(MainWindow);
        QObject::connect(autoScaleColor, SIGNAL(toggled(bool)), entireCameraScale, SLOT(setEnabled(bool)));
        QObject::connect(autoScaleColor, SIGNAL(toggled(bool)), currentPixelScale, SLOT(setEnabled(bool)));

        tabWidget_2->setCurrentIndex(0);
        tabWidget_3->setCurrentIndex(1);
        tabWidget_4->setCurrentIndex(0);
        tabWidget->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_5), QApplication::translate("MainWindow", "Camera", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("MainWindow", "Boards time values", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("MainWindow", "Start Cells", 0, QApplication::UnicodeUTF8));
        StartTimeMarksList->setText(QApplication::translate("MainWindow", "Start Time Marks", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("MainWindow", "Trigger Delays", 0, QApplication::UnicodeUTF8));
        tabWidget_3->setTabText(tabWidget_3->indexOf(tab_3), QApplication::translate("MainWindow", "Arrays data", 0, QApplication::UnicodeUTF8));
        tabWidget_3->setTabText(tabWidget_3->indexOf(tab_4), QApplication::translate("MainWindow", "Histograms", 0, QApplication::UnicodeUTF8));
        tabWidget_3->setTabText(tabWidget_3->indexOf(tab), QApplication::translate("MainWindow", "Pixel Curves", 0, QApplication::UnicodeUTF8));
        tabWidget_3->setTabText(tabWidget_3->indexOf(tab_7), QApplication::translate("MainWindow", "Average Curve", 0, QApplication::UnicodeUTF8));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_6), QApplication::translate("MainWindow", "Data", 0, QApplication::UnicodeUTF8));
        tabWidget_4->setTabText(tabWidget_4->indexOf(tab_16), QApplication::translate("MainWindow", "RMS", 0, QApplication::UnicodeUTF8));
        tabWidget_4->setTabText(tabWidget_4->indexOf(tab_17), QApplication::translate("MainWindow", "Mean", 0, QApplication::UnicodeUTF8));
        tabWidget_4->setTabText(tabWidget_4->indexOf(tab_18), QApplication::translate("MainWindow", "Max", 0, QApplication::UnicodeUTF8));
        tabWidget_4->setTabText(tabWidget_4->indexOf(tab_19), QApplication::translate("MainWindow", "Pos. of Max.", 0, QApplication::UnicodeUTF8));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_15), QApplication::translate("MainWindow", "Statistics", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_12), QApplication::translate("MainWindow", "Baseline", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_13), QApplication::translate("MainWindow", "Gain", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_14), QApplication::translate("MainWindow", "Trigger Offset", 0, QApplication::UnicodeUTF8));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_11), QApplication::translate("MainWindow", "Calibration", 0, QApplication::UnicodeUTF8));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_2), QApplication::translate("MainWindow", "3D", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "FACT - Raw events viewer - v0.7", 0, QApplication::UnicodeUTF8));
        extraInfoLabel->setText(QApplication::translate("MainWindow", "File loaded: none\n"
"Calibration file loaded: none\n"
"Run number:\n"
"Number of Events:\n"
"Number ofSlices:\n"
"Number of Time Marks:\n"
"Run Type:\n"
"Time of 1st data:\n"
"Time of last data:\n"
"SVN revision:\n"
"Number of boards:\n"
"Number of pixels:\n"
"Number of Slices TM:\n"
"Time system:\n"
"Date:\n"
"Night:\n"
"Camera:\n"
"DAQ:\n"
"ADC Count:\n"
"NB Evts OK:\n"
"NB Evts Rejected:\n"
"NB Evts Bad:\n"
"", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("MainWindow", "Selected Pixel", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("MainWindow", "Hw ID", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("MainWindow", "Sw ID", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("MainWindow", "Crate", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("MainWindow", "Board", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("MainWindow", "Patch", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "Ch", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("MainWindow", "Current Event Information", 0, QApplication::UnicodeUTF8));
        displayingEventLabel->setText(QApplication::translate("MainWindow", "Event", 0, QApplication::UnicodeUTF8));
        displayingSliceLabel->setText(QApplication::translate("MainWindow", "Slice  ", 0, QApplication::UnicodeUTF8));
        currentPixelValue->setText(QApplication::translate("MainWindow", "Current Pixel val.:", 0, QApplication::UnicodeUTF8));
        triggerTypeLabel->setText(QApplication::translate("MainWindow", "Trigger Type:", 0, QApplication::UnicodeUTF8));
        softwareTriggerLabel->setText(QApplication::translate("MainWindow", "Software Trigger: ", 0, QApplication::UnicodeUTF8));
        PCTimeLabel->setText(QApplication::translate("MainWindow", "PC Time:", 0, QApplication::UnicodeUTF8));
        label_20->setText(QApplication::translate("MainWindow", "Red", 0, QApplication::UnicodeUTF8));
        label_21->setText(QApplication::translate("MainWindow", "Ranges", 0, QApplication::UnicodeUTF8));
        label_22->setText(QApplication::translate("MainWindow", "Green", 0, QApplication::UnicodeUTF8));
        label_23->setText(QApplication::translate("MainWindow", "Blue", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        playPauseButton->setToolTip(QApplication::translate("MainWindow", "Play/Pause events animation", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        playPauseButton->setText(QApplication::translate("MainWindow", "Play/Pause", 0, QApplication::UnicodeUTF8));
        label_24->setText(QApplication::translate("MainWindow", "Slices per sec", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        slicesPerSecValue->setToolTip(QApplication::translate("MainWindow", "Number of slices to display per seconds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        drawImpulseCheckBox->setToolTip(QApplication::translate("MainWindow", "Whether the impulse of the current pixel should be drawn below the camera or not", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        drawImpulseCheckBox->setText(QApplication::translate("MainWindow", "Draw Impulse", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        drawBlurCheckBox->setToolTip(QApplication::translate("MainWindow", "Draw the pixels blurred (for having \"nice\", non-scientific images).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        drawBlurCheckBox->setText(QApplication::translate("MainWindow", "Blur pixels", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        drawPatchCheckBox->setToolTip(QApplication::translate("MainWindow", "Whether the pixels clustering should be drawn or not", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        drawPatchCheckBox->setText(QApplication::translate("MainWindow", "Draw Patches", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        loopOverCurrentEventBox->setToolTip(QApplication::translate("MainWindow", "Whether the animation should loop over the current event, or continue to the next event", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        loopOverCurrentEventBox->setText(QApplication::translate("MainWindow", "Loop event", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        autoScaleColor->setToolTip(QApplication::translate("MainWindow", "Rescale the coloring to match the values of the current event", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        autoScaleColor->setText(QApplication::translate("MainWindow", "AutoScale", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        entireCameraScale->setToolTip(QApplication::translate("MainWindow", "Use the entire camera to do the scaling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        entireCameraScale->setText(QApplication::translate("MainWindow", "entire camera", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        currentPixelScale->setToolTip(QApplication::translate("MainWindow", "Use the current pixel only to do the scaling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        currentPixelScale->setText(QApplication::translate("MainWindow", "current pixel", 0, QApplication::UnicodeUTF8));
        playSlicesRadio->setText(QApplication::translate("MainWindow", "play Slices", 0, QApplication::UnicodeUTF8));
        playEventsRadio->setText(QApplication::translate("MainWindow", "play Events", 0, QApplication::UnicodeUTF8));
        playPixelsRadio->setText(QApplication::translate("MainWindow", "play Pixels", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        loadNewFileButton->setToolTip(QApplication::translate("MainWindow", "Load a new fits file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        loadNewFileButton->setText(QApplication::translate("MainWindow", "Load Data", 0, QApplication::UnicodeUTF8));
        loadDRSCalibButton->setText(QApplication::translate("MainWindow", "Load DRS calib.", 0, QApplication::UnicodeUTF8));
        calibratedCheckBox->setText(QApplication::translate("MainWindow", "Calibrated data", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIEWER_H
