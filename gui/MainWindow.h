#ifndef FACT_MainWindow
#define FACT_MainWindow

#include "ui_design.h"

#include <QMainWindow>

class TObject;
class TCanvas;

class MainWindow : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT;

    void MakeLEDs(QPushButton **arr, QGridLayout *lay, const char *slot) const;

    void SelectTab(const QString &name);
    void SetTriggerSequence();
    void SetTriggerCoincidence();
    void SetCalibCoincidence();

protected:
    QPushButton *fFtuLED[40];
    QPushButton *fFadLED[40];

    bool fInHandler;

public:
    MainWindow(QWidget *p=0);

private slots:
    // Helper
    void on_fFtmStartRun_clicked();
    void on_fFtmStopRun_clicked();

    void on_fFadStart_clicked();
    void on_fFadStop_clicked();
    void on_fFadAbort_clicked();
    void on_fFadSoftReset_clicked();
    void on_fFadHardReset_clicked();

    void on_fLoggerStart_clicked();
    void on_fLoggerStop_clicked();

    void on_fMcpStartRun_clicked();
    void on_fMcpStopRun_clicked();
    void on_fMcpReset_clicked();

    // Comment Sql Table
    void on_fCommentInsertRow_clicked();

    // System status
    void on_fShutdown_clicked();
    void on_fShutdownAll_clicked();

    // Status LEDs signals
    void on_fStatusFTULed_clicked();
    void on_fStatusFTMLed_clicked();
    void on_fStatusFADLed_clicked();
    void on_fStatusLoggerLed_clicked();
    void on_fStatusChatLed_clicked();
    //void on_fStatusFTMEnable_stateChanged(int state);

    // Tab Widget
    void on_fTabWidget_tabCloseRequested(int which);
    virtual void on_fTabWidget_currentChanged(int) = 0;

    // Tab: FAD
    void slot_fFadLED_clicked();

//    void on_fFadStartRun_clicked();
//    void on_fFadStopRun_clicked();
    void on_fFadDrsOn_clicked();
    void on_fFadDrsOff_clicked();
    void on_fFadDwriteOn_clicked();
    void on_fFadDwriteOff_clicked();
    void on_fFadSingleTrigger_clicked();
    void on_fFadTriggerLineOn_clicked();
    void on_fFadTriggerLineOff_clicked();
    void on_fFadContTriggerOn_clicked();
    void on_fFadContTriggerOff_clicked();
    void on_fFadBusyOnOn_clicked();
    void on_fFadBusyOnOff_clicked();
    void on_fFadBusyOffOn_clicked();
    void on_fFadBusyOffOff_clicked();
    void on_fFadResetTriggerId_clicked();
    void on_fFadSocket0_clicked();
    void on_fFadSocket17_clicked();

    void FadSetFileFormat(uint16_t fmt);

    void on_fFadButtonFileFormatNone_clicked()  { FadSetFileFormat(0); }
    void on_fFadButtonFileFormatDebug_clicked() { FadSetFileFormat(1); }
    void on_fFadButtonFileFormatFits_clicked()  { FadSetFileFormat(2); }
    void on_fFadButtonFileFormatRaw_clicked()   { FadSetFileFormat(3); }
    void on_fFadButtonFileFormatZFits_clicked() { FadSetFileFormat(6); }

    void on_fFadPrescalerCmd_valueChanged(int);
    void on_fFadRunNumberCmd_valueChanged(int);
    void on_fFadRoiCmd_valueChanged(int = 0);
    void on_fFadRoiCh9Cmd_valueChanged(int) { on_fFadRoiCmd_valueChanged(); }

    void FadDacCmd_valueChanged(uint16_t, uint16_t);

    void on_fFadDac0Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 0); }
    void on_fFadDac1Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 1); }
    void on_fFadDac2Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 2); }
    void on_fFadDac3Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 3); }
    void on_fFadDac4Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 4); }
    void on_fFadDac5Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 5); }
    void on_fFadDac6Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 6); }
    void on_fFadDac7Cmd_valueChanged(int v) { FadDacCmd_valueChanged(v, 7); }

    void on_fDrsCalibStart_clicked();
    void on_fDrsCalibReset_clicked();

    void on_fDrsCalibStart2_clicked() { on_fDrsCalibStart_clicked(); }
    void on_fDrsCalibReset2_clicked() { on_fDrsCalibReset_clicked(); }

    // Tab: Adc
    virtual void DisplayEventData() = 0;
    void on_fAdcCrate_valueChanged(int)   { DisplayEventData(); }
    void on_fAdcBoard_valueChanged(int)   { DisplayEventData(); }
    void on_fAdcChip_valueChanged(int)    { DisplayEventData(); }
    void on_fAdcChannel_valueChanged(int) { DisplayEventData(); }

    // Tab: FTM
    void on_fEnableTrigger_stateChanged(int);
    void on_fEnableExt1_stateChanged(int);
    void on_fEnableExt2_stateChanged(int);
    void on_fEnableClockCond_stateChanged(int);
    void on_fEnableVeto_stateChanged(int);

    void on_fTriggerSeqPed_valueChanged(int)   { SetTriggerSequence(); }
    void on_fTriggerSeqLPint_valueChanged(int) { SetTriggerSequence(); }
    void on_fTriggerSeqLPext_valueChanged(int) { SetTriggerSequence(); }

    void on_fPhysicsCoincidence_valueChanged(int);
    void on_fPhysicsWindow_valueChanged(int);
    void on_fCalibCoincidence_valueChanged(int);
    void on_fCalibWindow_valueChanged(int);

    void on_fTriggerInterval_valueChanged(int);
    void on_fTriggerDelay_valueChanged(int);
    void on_fTimeMarkerDelay_valueChanged(int);
    void on_fDeadTime_valueChanged(int);
/*
    void on_fClockCondR0_valueChanged(int) { }
    void on_fClockCondR1_valueChanged(int) { }
    void on_fClockCondR8_valueChanged(int) { }
    void on_fClockCondR9_valueChanged(int) { }
    void on_fClockCondR11_valueChanged(int) { }
    void on_fClockCondR13_valueChanged(int) { }
    void on_fClockCondR14_valueChanged(int) { }
    void on_fClockCondR15_valueChanged(int) { }
*/
    void on_fPrescalingVal_valueChanged(int);

    void on_fClockCondFreq_activated(int);

    void on_fLpIntIntensity_valueChanged(int);
    void on_fLpExtIntensity_valueChanged(int);
    void on_fLpIntGroup1_stateChanged(int);
    void on_fLpExtGroup1_stateChanged(int);
    void on_fLpIntGroup2_stateChanged(int);
    void on_fLpExtGroup2_stateChanged(int);

    // Tab: FTUs
    void slot_fFtuLED_clicked();
    void on_fFtuPing_toggled(bool);
    void on_fFtuAllOn_clicked();
    void on_fFtuAllOff_clicked();

    // Tab: Feedback
    void on_fFeedbackDevMin_valueChanged(int);
    void on_fFeedbackDevMax_valueChanged(int);
    void on_fFeedbackCmdMin_valueChanged(int);
    void on_fFeedbackCmdMax_valueChanged(int);
    void on_fFeedbackStart_clicked();
    void on_fFeedbackStop_clicked();
    void on_fFeedbackCalibrate_clicked();

    // Tab: Bias
    virtual void BiasHvChannelChanged() = 0;
    virtual void BiasCamChannelChanged() = 0;
    void on_fBiasHvBoard_valueChanged(int)   { BiasHvChannelChanged(); }
    void on_fBiasHvChannel_valueChanged(int) { BiasHvChannelChanged(); }
    void on_fBiasCamCrate_valueChanged(int)  { BiasCamChannelChanged(); }
    void on_fBiasCamBoard_valueChanged(int)  { BiasCamChannelChanged(); }
    void on_fBiasCamPatch_valueChanged(int)  { BiasCamChannelChanged(); }
    void on_fBiasCamPixel_valueChanged(int)  { BiasCamChannelChanged(); }

    void on_fBiasVoltDac_valueChanged(int);

    void on_fBiasVoltMin_valueChanged(int); // FIXME: Could be set as slot in the designer
    void on_fBiasVoltMax_valueChanged(int); // FIXME: Could be set as slot in the designer

    void on_fBiasCurrentMin_valueChanged(int); // FIXME: Could be set as slot in the designer
    void on_fBiasCurrentMax_valueChanged(int); // FIXME: Could be set as slot in the designer

    void on_fBiasApplyChVolt_clicked();
    void on_fBiasApplyChDac_clicked();
    void on_fBiasApplyGlobalVolt_clicked();
    void on_fBiasApplyGlobalDac_clicked();

    void on_fBiasSetToZero_clicked();
    void on_fBiasReset_clicked();

    virtual void on_fBiasDispRefVolt_stateChanged(int) = 0;

    // Tab: Rates
    //virtual void UpdateThresholdIdx() = 0;
    virtual void on_fPixelIdx_valueChanged(int) = 0;
    //void on_fThresholdCrate_valueChanged(int) { UpdateThresholdIdx() ; }
    //void on_fThresholdBoard_valueChanged(int) { UpdateThresholdIdx() ; }
    //void on_fThresholdPatch_valueChanged(int) { UpdateThresholdIdx() ; }

    virtual void on_fPixelEnable_stateChanged(int) = 0;
    virtual void on_fThresholdVal_valueChanged(int) = 0;
    //virtual void on_fThresholdIdx_valueChanged(int) = 0;

    virtual void on_fBoardRatesEnabled_toggled(bool) = 0;

    void on_fNoutof4Val_valueChanged(int);

    void on_fRatesMin_valueChanged(int); // FIXME: Could be set as slot in the designer
    void on_fRatesMax_valueChanged(int); // FIXME: Could be set as slot in the designer
    void on_fPixelEnableAll_clicked();
    void on_fPixelDisableAll_clicked();

    virtual void on_fPixelDisableOthers_clicked() = 0;
    virtual void on_fThresholdDisableOthers_clicked() = 0;
    virtual void on_fThresholdEnablePatch_clicked() = 0;
    virtual void on_fThresholdDisablePatch_clicked() = 0;

    virtual void DisplayRates() = 0;
    void on_fRatePatch1_valueChanged(int) { DisplayRates(); }
    void on_fRatePatch2_valueChanged(int) { DisplayRates(); }
    void on_fRateBoard1_valueChanged(int) { DisplayRates(); }
    void on_fRateBoard2_valueChanged(int) { DisplayRates(); }

    // Tab: RateScan

    virtual void DisplayRateScan() = 0;
    void on_fRateScanPatch1_valueChanged(int) { DisplayRateScan(); }
    void on_fRateScanPatch2_valueChanged(int) { DisplayRateScan(); }
    void on_fRateScanBoard1_valueChanged(int) { DisplayRateScan(); }
    void on_fRateScanBoard2_valueChanged(int) { DisplayRateScan(); }

    // Tab: Chat
    void on_fChatSend_clicked();

    // Tab: Commands
    /// Needs access to DimNetwork thus it is implemented in the derived class
    virtual void on_fDimCmdSend_clicked() = 0;

    // Main menu
    //    void on_fMenuLogSaveAs_triggered(bool)

    virtual void slot_RootEventProcessed(TObject *, unsigned int, TCanvas *) = 0;
    virtual void slot_RootUpdate() = 0;
    virtual void slot_ChoosePixelThreshold(int) = 0;
    virtual void slot_ChooseBiasChannel(int) = 0;
    virtual void slot_CameraDoubleClick(int) = 0;
    virtual void slot_CameraMouseMove(int) = 0;
    void slot_TimeUpdate();
};

#endif
