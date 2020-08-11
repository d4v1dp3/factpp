#include "MainWindow.h"

#include <iostream>
#include <sstream>

#include <QTimer>

#include "src/Dim.h"

#include "DockWindow.h"
#include "HtmlDelegate.h"
#include "CheckBoxDelegate.h"

using namespace std;

void MainWindow::MakeLEDs(QPushButton **arr, QGridLayout *lay, const char *slot) const
{
    arr[0]->setToolTip("Crate 0, Board 0, Index 0");

    for (int i=1; i<40; i++)
    {
        QPushButton *b = new QPushButton(static_cast<QWidget*>(arr[0]->parent()));

        b->setEnabled(arr[0]->isEnabled());
        b->setSizePolicy(arr[0]->sizePolicy());
        b->setMaximumSize(arr[0]->maximumSize());
        b->setIcon(arr[0]->icon());
        b->setIconSize(arr[0]->iconSize());
        b->setCheckable(arr[0]->isCheckable());
        b->setFlat(arr[0]->isFlat());

        ostringstream str;
        str << "Crate " << i/10 << ", Board " << i%10 << ", Index " << i;
        b->setToolTip(str.str().c_str());

        lay->addWidget(b, i/10+1, i%10+1, 1, 1);

        arr[i] = b;
    }

    const QString name = arr[0]->objectName();

    for (int i=0; i<40; i++)
    {
        arr[i]->setObjectName(name+QString::number(i));
        QObject::connect(arr[i], SIGNAL(clicked()), this, slot);
    }
}

MainWindow::MainWindow(QWidget *p) : QMainWindow(p)
{
    // setupUi MUST be called before the DimNetwork is initilized
    // In this way it can be ensured that nothing from the
    // DimNetwork arrives before all graphical elements are
    // initialized. This is a simple but very powerfull trick.
    setupUi(this);

    // Now here we can do further setup which should be done
    // before the gui is finally displayed.
    fDimCmdServers->setItemDelegate(new CheckBoxDelegate);
    fDimCmdCommands->setItemDelegate(new CheckBoxDelegate);
    fDimCmdDescription->setItemDelegate(new HtmlDelegate);

    fDimSvcServers->setItemDelegate(new CheckBoxDelegate);
    fDimSvcServices->setItemDelegate(new CheckBoxDelegate);
    fDimSvcDescription->setItemDelegate(new HtmlDelegate);

    // Set a default string to be displayed in a the status bar at startup
    fStatusBar->showMessage(PACKAGE_STRING "   |   " PACKAGE_URL "   |   report bugs to <" PACKAGE_BUGREPORT ">");

    // Initialize the 40 FTU Leds as a copy of the prototype LED
    fFtuLED[0] = fFtuLEDPrototype;
    MakeLEDs(fFtuLED, fFtuLedLayout, SLOT(slot_fFtuLED_clicked()));

    // Initialize the 40 FAD Leds as a copy of the prototype LED
    fFadLED[0] = fFadLEDPrototype;
    MakeLEDs(fFadLED, fFadLedLayout, SLOT(slot_fFadLED_clicked()));

    // Initialize a timer to update the displayed UTC time
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_TimeUpdate()));
    timer->start(100);
}

void MainWindow::slot_TimeUpdate()
{
    // Used toUTC to support also older Qt versions
    // toTime_t() always returns the datetime converted to UTC
    // dateTime() unfortunately returns our UTC always as LocalTime
    QDateTime now = QDateTime::currentDateTime().toUTC();
    now.setTimeSpec(Qt::LocalTime);

    if (now.toTime_t()==fUTC->dateTime().toTime_t())
        return;

    fUTC->setDateTime(now);
}


void MainWindow::SelectTab(const QString &name)
{
    for (int i=0; i<fTabWidget->count(); i++)
        if (fTabWidget->tabText(i)==name)
        {
            fTabWidget->setCurrentIndex(i);
            break;
        }
}

void MainWindow::on_fCommentInsertRow_clicked()
{
    if (fTableComments->model())
        fTableComments->model()->insertRow(fTableComments->model()->rowCount());
}

void MainWindow::on_fNoutof4Val_valueChanged(int val)
{
    const int32_t v[2] = { -1, val };

    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_N_OUT_OF_4", v);
}

void MainWindow::on_fRatesMin_valueChanged(int min)
{
    fRatesCanv->SetMin(min);
}

void MainWindow::on_fRatesMax_valueChanged(int max)
{
    fRatesCanv->SetMax(max);
}

void MainWindow::on_fShutdown_clicked()
{
    Dim::SendCommand("DIS_DNS/KILL_SERVERS", int(1));
}

void MainWindow::on_fShutdownAll_clicked()
{
    Dim::SendCommand("DIS_DNS/KILL_SERVERS", int(1));
    Dim::SendCommand("DIS_DNS/EXIT", int(1));
}

void MainWindow::on_fTabWidget_tabCloseRequested(int which)
{
    // To get the correct size we have to switch to this tab
    // An alternative would be to take the size of the current tab
    fTabWidget->setCurrentIndex(which);

    QWidget *w = fTabWidget->currentWidget(); //fTabWidget->widget(which);
    if (!w)
    {
        cout << "Weird... the tab requested to be closed doesn't exist!" << endl;
        return;
    }

    QDockWidget *d = w->findChild<QDockWidget*>();
    if (!d)
    {
        cout << "Sorry, tab requested to be closed contains no QDockWidget!" << endl;
        return;
    }

    new DockWindow(d, fTabWidget->tabText(which));
    fTabWidget->removeTab(which);

    if (fTabWidget->count()==1)
        fTabWidget->setTabsClosable(false);
}

void MainWindow::on_fMcpStartRun_clicked()
{
    struct Value
    {
        uint64_t time;
        uint64_t nevts;
        char type[];
    };

    const int idx1 = fMcpRunType->currentIndex();
    const int idx2 = fMcpTime->currentIndex();
    const int idx3 = fMcpNumEvents->currentIndex();

    const int64_t v2 = fMcpTime->itemData(idx2).toInt();
    const int64_t v3 = fMcpNumEvents->itemData(idx3).toInt();

    const QString rt = fMcpRunType->itemData(idx1).toString();

    const size_t len = sizeof(Value)+rt.length()+1;

    char *buf = new char[len];

    Value *val = reinterpret_cast<Value*>(buf);

    val->time  = v2;
    val->nevts = v3;

    strcpy(val->type, rt.toStdString().c_str());

    Dim::SendCommand("MCP/START", buf, len);

    delete [] buf;

}
void MainWindow::on_fMcpStopRun_clicked()
{
   Dim::SendCommand("MCP/STOP");
}

void MainWindow::on_fMcpReset_clicked()
{
   Dim::SendCommand("MCP/RESET");
}

void MainWindow::on_fLoggerStart_clicked()
{
    Dim::SendCommand("DATA_LOGGER/START_RUN_LOGGING");
}

void MainWindow::on_fLoggerStop_clicked()
{
    Dim::SendCommand("DATA_LOGGER/STOP_RUN_LOGGING");
}

void MainWindow::on_fFtmStartRun_clicked()
{
    Dim::SendCommand("FTM_CONTROL/START_TRIGGER");
}

void MainWindow::on_fFtmStopRun_clicked()
{
    Dim::SendCommand("FTM_CONTROL/STOP_TRIGGER");
}

/*
void MainWindow::on_fFadStartRun_clicked()
{
    Dim::SendCommand("FAD_CONTROL/START_RUN");
}

void MainWindow::on_fFadStopRun_clicked()
{
    Dim::SendCommand("FAD_CONTROL/STOP_RUN");
}
*/

void MainWindow::on_fFadDrsOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_DRS", uint8_t(true));
}

void MainWindow::on_fFadDrsOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_DRS", uint8_t(false));
}

void MainWindow::on_fFadDwriteOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_DWRITE", uint8_t(true));
}

void MainWindow::on_fFadDwriteOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_DWRITE", uint8_t(false));
}

void MainWindow::on_fFadSingleTrigger_clicked()
{
    Dim::SendCommand("FAD_CONTROL/SEND_SINGLE_TRIGGER");
}

void MainWindow::on_fFadTriggerLineOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_TRIGGER_LINE", uint8_t(true));
}

void MainWindow::on_fFadTriggerLineOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_TRIGGER_LINE", uint8_t(false));
}

void MainWindow::on_fFadContTriggerOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_CONTINOUS_TRIGGER", uint8_t(true));
}

void MainWindow::on_fFadContTriggerOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_CONTINOUS_TRIGGER", uint8_t(false));
}

void MainWindow::on_fFadBusyOnOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_BUSY_ON", uint8_t(true));
}

void MainWindow::on_fFadBusyOnOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_BUSY_ON", uint8_t(false));
}

void MainWindow::on_fFadBusyOffOn_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_BUSY_OFF", uint8_t(true));
}

void MainWindow::on_fFadBusyOffOff_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_BUSY_OFF", uint8_t(false));
}

void MainWindow::on_fFadSocket0_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_COMMAND_SOCKET_MODE", uint8_t(true));
}

void MainWindow::on_fFadSocket17_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ENABLE_COMMAND_SOCKET_MODE", uint8_t(false));
}

void MainWindow::on_fFadResetTriggerId_clicked()
{
    Dim::SendCommand("FAD_CONTROL/RESET_EVENT_COUNTER");
}

void MainWindow::FadSetFileFormat(uint16_t fmt)
{
    Dim::SendCommand("FAD_CONTROL/SET_FILE_FORMAT", fmt);
}

void MainWindow::on_fFadStart_clicked()
{
    Dim::SendCommand("FAD_CONTROL/START");
}

void MainWindow::on_fFadStop_clicked()
{
    Dim::SendCommand("FAD_CONTROL/STOP");
}

void MainWindow::on_fFadAbort_clicked()
{
    Dim::SendCommand("FAD_CONTROL/ABORT");
}

void MainWindow::on_fFadSoftReset_clicked()
{
    Dim::SendCommand("FAD_CONTROL/SOFT_RESET");
}

void MainWindow::on_fFadHardReset_clicked()
{
    Dim::SendCommand("FAD_CONTROL/HARD_RESET");
}

void MainWindow::slot_fFadLED_clicked()
{
    for (int32_t i=0; i<40; i++)
        if (sender()==fFadLED[i])
        {
            Dim::SendCommand("FAD_CONTROL/TOGGLE", i);
            break;
        }
}

void MainWindow::on_fFadPrescalerCmd_valueChanged(int val)
{
    Dim::SendCommand("FAD_CONTROL/SET_TRIGGER_RATE", uint32_t(val));
}

void MainWindow::on_fFadRunNumberCmd_valueChanged(int val)
{
    Dim::SendCommand("FAD_CONTROL/SET_RUN_NUMBER", uint64_t(val));
}

void MainWindow::on_fFadRoiCmd_valueChanged(int)
{
    const int32_t vals1[2] = { -1, fFadRoiCmd->value() };
    Dim::SendCommand("FAD_CONTROL/SET_REGION_OF_INTEREST", vals1);

    for (int ch=8; ch<36; ch+=9)
    {
        const int32_t vals2[2] = { ch,  fFadRoiCh9Cmd->value() };
        Dim::SendCommand("FAD_CONTROL/SET_REGION_OF_INTEREST", vals2);
    }
}

void MainWindow::FadDacCmd_valueChanged(uint16_t val, uint16_t idx)
{
    const uint32_t cmd[2] = { idx, val };
    Dim::SendCommand("FAD_CONTROL/SET_DAC_VALUE", cmd);
}

void MainWindow::on_fDrsCalibStart_clicked()
{
    Dim::SendCommand("FAD_CONTROL/START_DRS_CALIBRATION");
}

void MainWindow::on_fDrsCalibReset_clicked()
{
    Dim::SendCommand("FAD_CONTROL/RESET_SECONDARY_DRS_BASELINE");
}

void MainWindow::SetTriggerSequence()
{
    const uint16_t d[3] =
    {
        uint16_t(fTriggerSeqPed->value()),
        uint16_t(fTriggerSeqLPext->value()),
        uint16_t(fTriggerSeqLPint->value())
    };

    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TRIGGER_SEQUENCE", d);
}

/*
void MainWindow::on_fEnableTrigger_clicked(bool b)
{
    Dim::SendCommand("FTM_CONTROL/ENABLE_TRIGGER", b);
}

void MainWindow::on_fEnableExt1_clicked(bool b)
{
    Dim::SendCommand("FTM_CONTROL/ENABLE_EXT1", b);
}

void MainWindow::on_fEnableExt2_clicked(bool b)
{
    Dim::SendCommand("FTM_CONTROL/ENABLE_EXT2", b);
}

void MainWindow::on_fEnableVeto_clicked(bool b)
{
    Dim::SendCommand("FTM_CONTROL/ENABLE_VETO", b);
}
*/
void MainWindow::on_fPhysicsCoincidence_valueChanged(int v)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TRIGGER_MULTIPLICITY", v);
}

void MainWindow::on_fPhysicsWindow_valueChanged(int v)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TRIGGER_WINDOW", v/4-2);
}

void MainWindow::on_fCalibCoincidence_valueChanged(int v)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_CALIBRATION_MULTIPLICITY", v);
}

void MainWindow::on_fCalibWindow_valueChanged(int v)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_CALIBRATION_WINDOW", v/4-2);
}

void MainWindow::on_fTriggerInterval_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TRIGGER_INTERVAL", val);
}

void MainWindow::on_fTriggerDelay_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TRIGGER_DELAY", val/4-2);
}

void MainWindow::on_fTimeMarkerDelay_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_TIME_MARKER_DELAY", val/4-2);
}

void MainWindow::on_fDeadTime_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_DEAD_TIME", val/4-2);
}

void MainWindow::on_fPrescalingVal_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_PRESCALING", val);
}

void MainWindow::on_fPixelEnableAll_clicked()
{
    Dim::SendCommand("FTM_CONTROL/ENABLE_PIXEL", int16_t(-1));
}

void MainWindow::on_fPixelDisableAll_clicked()
{
    Dim::SendCommand("FTM_CONTROL/DISABLE_PIXEL", int16_t(-1));
}

void MainWindow::on_fEnableTrigger_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_TRIGGER", b==Qt::Checked);
}

void MainWindow::on_fEnableExt1_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_EXT1", b==Qt::Checked);
}

void MainWindow::on_fEnableExt2_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_EXT2", b==Qt::Checked);
}

void MainWindow::on_fEnableClockCond_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_CLOCK_CONDITIONER", b==Qt::Checked);
}

void MainWindow::on_fEnableVeto_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_VETO", b==Qt::Checked);
}

void MainWindow::on_fClockCondFreq_activated(int idx)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_CLOCK_FREQUENCY", fClockCondFreq->itemData(idx).toInt());
}

void MainWindow::slot_fFtuLED_clicked()
{
    for (int32_t i=0; i<40; i++)
        if (sender()==fFtuLED[i])
        {
            Dim::SendCommand("FTM_CONTROL/TOGGLE_FTU", i);
            break;
        }
}

void MainWindow::on_fFtuPing_toggled(bool checked)
{
    if (checked)
        Dim::SendCommand("FTM_CONTROL/PING");
}

void MainWindow::on_fFtuAllOn_clicked()
{
    static const struct Data { int32_t id; char on; } __attribute__((__packed__)) d = { -1, 1 };
    Dim::SendCommand("FTM_CONTROL/ENABLE_FTU", &d, sizeof(Data));
}

void MainWindow::on_fFtuAllOff_clicked()
{
    static const struct Data { int32_t id; char on; } __attribute__((__packed__)) d = { -1, 0 };
    Dim::SendCommand("FTM_CONTROL/ENABLE_FTU", &d, sizeof(Data));
}

void MainWindow::on_fLpIntIntensity_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_INTENSITY_LPINT", uint16_t(val));
}

void MainWindow::on_fLpExtIntensity_valueChanged(int val)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/SET_INTENSITY_LPEXT", uint16_t(val));
}

void MainWindow::on_fLpIntGroup1_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_GROUP1_LPINT", uint8_t(b));
}

void MainWindow::on_fLpExtGroup1_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_GROUP1_LPEXT", uint8_t(b));
}

void MainWindow::on_fLpIntGroup2_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_GROUP2_LPINT", uint8_t(b));
}

void MainWindow::on_fLpExtGroup2_stateChanged(int b)
{
    if (!fInHandler)
        Dim::SendCommand("FTM_CONTROL/ENABLE_GROUP2_LPEXT", uint8_t(b));
}

void MainWindow::on_fFeedbackDevMin_valueChanged(int min)
{
    fFeedbackDevCam->SetMin(min);
    fFeedbackDevCam->updateCamera();
}

void MainWindow::on_fFeedbackDevMax_valueChanged(int max)
{
    fFeedbackDevCam->SetMax(max);
    fFeedbackDevCam->updateCamera();
}

void MainWindow::on_fFeedbackCmdMin_valueChanged(int min)
{
    fFeedbackCmdCam->SetMin(min);
    fFeedbackCmdCam->updateCamera();
}

void MainWindow::on_fFeedbackCmdMax_valueChanged(int max)
{
    fFeedbackCmdCam->SetMax(max);
    fFeedbackCmdCam->updateCamera();
}

void MainWindow::on_fFeedbackStart_clicked()
{
    Dim::SendCommand("FEEDBACK/START",
                     (float)fFeedbackOvervoltage->value());
}

void MainWindow::on_fFeedbackStop_clicked()
{
    Dim::SendCommand("FEEDBACK/STOP");
}

void MainWindow::on_fFeedbackCalibrate_clicked()
{
    Dim::SendCommand("FEEDBACK/CALIBRATE");
}

void MainWindow::on_fBiasVoltDac_valueChanged(int val)
{
    fBiasVoltDacVolt->setValue(val*90./4096);
}

/*
void MainWindow::on_fBiasRequestStatus_clicked()
{
    if (!fInHandler)
        Dim::SendCommand("BIAS_CONTROL/REQUEST_STATUS");
}
*/

void MainWindow::on_fBiasSetToZero_clicked()
{
    if (!fInHandler)
        Dim::SendCommand("BIAS_CONTROL/SET_ZERO_VOLTAGE");
}

void MainWindow::on_fBiasReset_clicked()
{
    if (!fInHandler)
        Dim::SendCommand("BIAS_CONTROL/RESET_OVER_CURRENT_STATUS");
}


void MainWindow::on_fBiasApplyChVolt_clicked()       // SET_CHANNEL_VOLTAGE
{
    if (fInHandler)
        return;

    const struct Data { uint16_t ch; float val; } __attribute__((__packed__)) val = {
        uint16_t(fBiasHvBoard->value()*32+fBiasHvChannel->value()),
        float(fBiasVolt->value())
    };

    Dim::SendCommand("BIAS_CONTROL/SET_CHANNEL_VOLTAGE", &val, sizeof(Data));
}

void MainWindow::on_fBiasApplyChDac_clicked()
{
    if (fInHandler)
        return;

    const uint16_t val[2] =
    {
        uint16_t(fBiasHvBoard->value()*32+fBiasHvChannel->value()),
        uint16_t(fBiasVoltDac->value())
    };

    Dim::SendCommand("BIAS_CONTROL/SET_CHANNEL_DAC", val);
}

void MainWindow::on_fBiasApplyGlobalVolt_clicked()
{
    if (!fInHandler)
        Dim::SendCommand("BIAS_CONTROL/SET_GLOBAL_VOLTAGE", float(fBiasVolt->value()));
}
    
void MainWindow::on_fBiasApplyGlobalDac_clicked()
{
    if (!fInHandler)
        Dim::SendCommand("BIAS_CONTROL/SET_GLOBAL_DAC", uint16_t(fBiasVoltDac->value()));
}

void MainWindow::on_fBiasVoltMin_valueChanged(int min)
{
    fBiasCamV->SetMin(min);
    fBiasCamV->updateCamera();
}

void MainWindow::on_fBiasVoltMax_valueChanged(int max)
{
    fBiasCamV->SetMax(max);
    fBiasCamV->updateCamera();
}

void MainWindow::on_fBiasCurrentMin_valueChanged(int min)
{
    fBiasCamA->SetMin(min);
    fBiasCamA->updateCamera();
}

void MainWindow::on_fBiasCurrentMax_valueChanged(int max)
{
    fBiasCamA->SetMax(max);
    fBiasCamA->updateCamera();
}

void MainWindow::on_fChatSend_clicked()
{
    const string msg = fChatMessage->text().toStdString();
    if (Dim::SendCommand("CHAT/MSG", msg.c_str(), msg.length()+1))
        fChatMessage->clear();
}

void MainWindow::on_fStatusLoggerLed_clicked()
{
    SelectTab("Logger");
}

void MainWindow::on_fStatusChatLed_clicked()
{
    SelectTab("Chat");
}

void MainWindow::on_fStatusFTMLed_clicked()
{
    SelectTab("Trigger");
}

void MainWindow::on_fStatusFTULed_clicked()
{
    SelectTab("FTUs");
}

void MainWindow::on_fStatusFADLed_clicked()
{
    SelectTab("FAD");
}
