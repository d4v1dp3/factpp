#ifndef FACT_FactGui
#define FACT_FactGui

#include "MainWindow.h"

#include <iomanip>
#include <valarray>

#include <boost/regex.hpp>

#include <QTimer>
#include <QtSql/QSqlError>
#include <QtSql/QSqlTableModel>
#include <QStandardItemModel>

#include "CheckBoxDelegate.h"

#include "src/Dim.h"
#include "src/Converter.h"
#include "src/Configuration.h"
#include "src/DimNetwork.h"
#include "src/tools.h"
#include "src/DimData.h"
#include "externals/PixelMap.h"

#ifdef HAVE_ROOT
#include "TROOT.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH2.h"
#include "TBox.h"
#include "TStyle.h"
#include "TMarker.h"
#include "TColor.h"
#endif

#include "QCameraWidget.h"

#include "src/FAD.h"
#include "src/HeadersMCP.h"
#include "src/HeadersFTM.h"
#include "src/HeadersFAD.h"
#include "src/HeadersFSC.h"
#include "src/HeadersBIAS.h"
#include "src/HeadersDrive.h"
#include "src/HeadersFeedback.h"
#include "src/HeadersRateScan.h"
#include "src/HeadersRateControl.h"
#include "src/HeadersMagicWeather.h"

using namespace std;

// #########################################################################

class FactGui : public MainWindow, public DimNetwork, public DimInfoHandler
{
private:
    class FunctionEvent : public QEvent
    {
    public:
        function<void(const QEvent &)> fFunction;

        FunctionEvent(const function<void(const QEvent &)> &f)
            : QEvent((QEvent::Type)QEvent::registerEventType()),
            fFunction(f) { }

        bool Exec() { fFunction(*this); return true; }
    };

    valarray<int8_t> fFtuStatus;

    PixelMap fPixelMap;

    //vector<int>  fPixelMapHW; // Software -> Hardware
    vector<int> fPatchMapHW; // Software -> Hardware

    bool fInChoosePatchTH;   // FIXME. Find a better solution
    bool fInChooseBiasHv;    // FIXME. Find a better solution
    bool fInChooseBiasCam;   // FIXME. Find a better solution

    DimStampedInfo fDimDNS;

    DimStampedInfo fDimLoggerStats;
    DimStampedInfo fDimLoggerFilenameNight;
    DimStampedInfo fDimLoggerFilenameRun;
    DimStampedInfo fDimLoggerNumSubs;

    DimStampedInfo fDimFtmPassport;
    DimStampedInfo fDimFtmTriggerRates;
    DimStampedInfo fDimFtmError;
    DimStampedInfo fDimFtmFtuList;
    DimStampedInfo fDimFtmStaticData;
    DimStampedInfo fDimFtmDynamicData;
    DimStampedInfo fDimFtmCounter;

    DimStampedInfo fDimFadWriteStats;
    DimStampedInfo fDimFadStartRun;
    DimStampedInfo fDimFadRuns;
    DimStampedInfo fDimFadEvents;
    DimStampedInfo fDimFadRawData;
    DimStampedInfo fDimFadEventData;
    DimStampedInfo fDimFadConnections;
    DimStampedInfo fDimFadFwVersion;
    DimStampedInfo fDimFadRunNumber;
    DimStampedInfo fDimFadDNA;
    DimStampedInfo fDimFadTemperature;
    DimStampedInfo fDimFadPrescaler;
    DimStampedInfo fDimFadRefClock;
    DimStampedInfo fDimFadRoi;
    DimStampedInfo fDimFadDac;
    DimStampedInfo fDimFadDrsCalibration;
    DimStampedInfo fDimFadStatus;
    DimStampedInfo fDimFadStatistics1;
    //DimStampedInfo fDimFadStatistics2;
    DimStampedInfo fDimFadFileFormat;

    DimStampedInfo fDimFscTemp;
    DimStampedInfo fDimFscVolt;
    DimStampedInfo fDimFscCurrent;
    DimStampedInfo fDimFscHumidity;

    DimStampedInfo fDimFeedbackCalibration;
    DimStampedInfo fDimFeedbackCalibrated;

    DimStampedInfo fDimBiasNominal;
    DimStampedInfo fDimBiasVolt;
    DimStampedInfo fDimBiasDac;
    DimStampedInfo fDimBiasCurrent;

    DimStampedInfo fDimRateScan;

    DimStampedInfo fDimMagicWeather;

    map<string, DimInfo*> fServices;

    // ========================== LED Colors ================================

    enum LedColor_t
    {
        kLedRed,
        kLedGreen,
        kLedGreenWarn,
        kLedGreenCheck,
        kLedGreenBar,
        kLedYellow,
        kLedOrange,
        kLedGray,
        kLedWarnBorder,
        kLedWarn,
        kLedWarnTriangleBorder,
        kLedWarnTriangle,
        kLedInProgress,
    };

    void SetLedColor(QPushButton *button, LedColor_t col, const Time &t)
    {
        switch (col)
        {
        case kLedRed:
            button->setIcon(QIcon(":/Resources/icons/red circle 1.png"));
            break;

        case kLedGreen:
            button->setIcon(QIcon(":/Resources/icons/green circle 1.png"));
            break;

        case kLedGreenBar:
            button->setIcon(QIcon(":/Resources/icons/green bar.png"));
            break;

        case kLedGreenWarn:
            button->setIcon(QIcon(":/Resources/icons/green warn.png"));
            break;

        case kLedGreenCheck:
            button->setIcon(QIcon(":/Resources/icons/green check.png"));
            break;

        case kLedYellow:
            button->setIcon(QIcon(":/Resources/icons/yellow circle 1.png"));
            break;

        case kLedOrange:
            button->setIcon(QIcon(":/Resources/icons/orange circle 1.png"));
            break;

        case kLedGray:
            button->setIcon(QIcon(":/Resources/icons/gray circle 1.png"));
            break;

        case kLedWarnBorder:
            button->setIcon(QIcon(":/Resources/icons/warning 1.png"));
            break;

        case kLedWarn:
            button->setIcon(QIcon(":/Resources/icons/warning 2.png"));
            break;

        case kLedWarnTriangle:
            button->setIcon(QIcon(":/Resources/icons/warning 3.png"));
            break;

        case kLedWarnTriangleBorder:
            button->setIcon(QIcon(":/Resources/icons/warning 4.png"));
            break;

        case kLedInProgress:
            button->setIcon(QIcon(":/Resources/icons/in progress.png"));
            break;

        }

        //button->setToolTip("Last change: "+QDateTime::currentDateTimeUtc().toString()+" UTC");
        button->setToolTip(("Last change: "+t.GetAsStr()+" (UTC)").c_str());
    }

    // ===================== Services and Commands ==========================

    QStandardItem *AddServiceItem(const string &server, const string &service, bool iscmd)
    {
        QListView *servers     = iscmd ? fDimCmdServers     : fDimSvcServers;
        QListView *services    = iscmd ? fDimCmdCommands    : fDimSvcServices;
        QListView *description = iscmd ? fDimCmdDescription : fDimSvcDescription;

        QStandardItemModel *m = dynamic_cast<QStandardItemModel*>(servers->model());
        if (!m)
        {
            m = new QStandardItemModel(this);
            servers->setModel(m);
            services->setModel(m);
            description->setModel(m);
        }

        QList<QStandardItem*> l = m->findItems(server.c_str());

        if (l.size()>1)
        {
            cout << "hae" << endl;
            return 0;
        }

        QStandardItem *col = l.size()==0 ? NULL : l[0];

        if (!col)
        {
            col = new QStandardItem(server.c_str());
            m->appendRow(col);

            if (!services->rootIndex().isValid())
            {
                services->setRootIndex(col->index());
                servers->setCurrentIndex(col->index());
            }
        }

        QStandardItem *item = 0;
        for (int i=0; i<col->rowCount(); i++)
        {
            QStandardItem *coli = col->child(i);
            if (coli->text().toStdString()==service)
                return coli;
        }

        item = new QStandardItem(service.c_str());
        col->appendRow(item);
        col->sortChildren(0);

        if (!description->rootIndex().isValid())
        {
            description->setRootIndex(item->index());
            services->setCurrentIndex(item->index());
        }

        if (!iscmd)
            item->setCheckable(true);

        return item;
    }

    void AddDescription(QStandardItem *item, const vector<Description> &vec)
    {
        if (!item)
            return;
        if (vec.size()==0)
            return;

        item->setToolTip(vec[0].comment.c_str());

        const string str = Description::GetHtmlDescription(vec);

        QStandardItem *desc = new QStandardItem(str.c_str());
        desc->setSelectable(false);
        item->setChild(0, 0, desc);
    }

    void AddServer(const string &s)
    {
        DimNetwork::AddServer(s);

        const State state = GetState(s, GetCurrentState(s));

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleAddServer, this, s, state)));
    }

    void AddService(const string &server, const string &service, const string &fmt, bool iscmd)
    {
        const vector<Description> v = GetDescription(server, service);

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleAddService, this, server, service, fmt, iscmd, v)));
    }

    void RemoveService(string server, string service, bool iscmd)
    {
        UnsubscribeService(server+'/'+service, true);

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleRemoveService, this, server, service, iscmd)));
    }

    void RemoveAllServices(const string &server)
    {
        UnsubscribeAllServices(server);

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleRemoveAllServices, this, server)));
    }

    void AddDescription(const string &server, const string &service, const vector<Description> &vec)
    {
        const bool iscmd = IsCommand(server, service)==true;

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleAddDescription, this, server, service, vec, iscmd)));
    }

    // ======================================================================

    void handleAddServer(const string &server, const State &state)
    {
        handleStateChanged(Time(), server, state);
    }

    void handleAddService(const string &server, const string &service, const string &/*fmt*/, bool iscmd, const vector<Description> &vec)
    {
        QStandardItem *item = AddServiceItem(server, service, iscmd);
        AddDescription(item, vec);
    }

    void handleRemoveService(const string &server, const string &service, bool iscmd)
    {
        QListView *servers = iscmd ? fDimCmdServers : fDimSvcServers;

        QStandardItemModel *m = dynamic_cast<QStandardItemModel*>(servers->model());
        if (!m)
            return;

        QList<QStandardItem*> l = m->findItems(server.c_str());
        if (l.size()!=1)
            return;

        for (int i=0; i<l[0]->rowCount(); i++)
        {
            QStandardItem *row = l[0]->child(i);
            if (row->text().toStdString()==service)
            {
                l[0]->removeRow(row->index().row());
                return;
            }
        }
    }

    void handleRemoveAllServices(const string &server)
    {
        handleStateChanged(Time(), server, State(-2, "Offline", "No connection via DIM."));

        QStandardItemModel *m = 0;
        if ((m=dynamic_cast<QStandardItemModel*>(fDimCmdServers->model())))
        {
            QList<QStandardItem*> l = m->findItems(server.c_str());
            if (l.size()==1)
                m->removeRow(l[0]->index().row());
        }

        if ((m = dynamic_cast<QStandardItemModel*>(fDimSvcServers->model())))
        {
            QList<QStandardItem*> l = m->findItems(server.c_str());
            if (l.size()==1)
                m->removeRow(l[0]->index().row());
        }
    }

    void handleAddDescription(const string &server, const string &service, const vector<Description> &vec, bool iscmd)
    {
        QStandardItem *item = AddServiceItem(server, service, iscmd);
        AddDescription(item, vec);
    }

    // ======================================================================

    void SubscribeService(const string &service)
    {
        if (fServices.find(service)!=fServices.end())
        {
            cerr << "ERROR - We are already subscribed to " << service << endl;
            return;
        }

        fServices[service] = new DimStampedInfo(service.c_str(), (void*)NULL, 0, this);
    }

    void UnsubscribeService(const string &service, bool allow_unsubscribed=false)
    {
        const map<string,DimInfo*>::iterator i=fServices.find(service);

        if (i==fServices.end())
        {
            if (!allow_unsubscribed)
                cerr << "ERROR - We are not subscribed to " << service << endl;
            return;
        }

        delete i->second;

        fServices.erase(i);
    }

    void UnsubscribeAllServices(const string &server)
    {
        for (map<string,DimInfo*>::iterator i=fServices.begin();
             i!=fServices.end(); i++)
            if (i->first.substr(0, server.length()+1)==server+'/')
            {
                delete i->second;
                fServices.erase(i);
            }
    }

    // ======================= DNS ==========================================

    uint32_t fDimVersion;

    void UpdateGlobalStatus()
    {
        ostringstream dns;
        dns << (fDimVersion==0?"No connection":"Connection");
        dns << " to DIM DNS (" << getenv("DIM_DNS_NODE") << ")";
        dns << (fDimVersion==0?".":" established");

        ostringstream str;
        str << "V" << fDimVersion/100 << 'r' << fDimVersion%100;

        LedColor_t led = kLedGreen;
        if (fDimVersion>0)
        {
            dns << fixed << setprecision(1) << right;
            if (fFreeSpaceLogger!=UINT64_MAX)
                dns << "<pre> * Data logger:   " << setw(7) << fFreeSpaceLogger*1e-7 << " GB</pre>";
            if (fFreeSpaceData!=UINT64_MAX)
                dns << "<pre> * Event Builder: " << setw(7) << fFreeSpaceData*1e-7 << " GB</pre>";

            if (fFreeSpaceLogger<500000000 || fFreeSpaceData<500000000)
                led = kLedGreenWarn;
            if (fFreeSpaceLogger<200000000 || fFreeSpaceData<200000000)
                led = kLedWarnTriangleBorder;

            if (led!=kLedGreen)
                str << " (Disk space!)";
        }

        fStatusDNSLabel->setToolTip(dns.str().c_str());

        SetLedColor(fStatusDNSLed, fDimVersion==0 ? kLedRed : led, Time());

        fStatusDNSLabel->setText(fDimVersion==0?"Offline":str.str().c_str());
    }

    void handleDimDNS(const DimData &d)
    {
        fDimVersion = d.size()!=4 ? 0 : d.get<uint32_t>();

        UpdateGlobalStatus();

        fShutdown->setEnabled(fDimVersion!=0);
        fShutdownAll->setEnabled(fDimVersion!=0);
    }


    // ======================= Logger =======================================

    uint64_t fFreeSpaceLogger;

    void handleLoggerStats(const DimData &d)
    {
        const bool connected = d.size()!=0;

        fLoggerET->setEnabled(connected);
        fLoggerRate->setEnabled(connected);
        fLoggerWritten->setEnabled(connected);
        fLoggerFreeSpace->setEnabled(connected);
        fLoggerSpaceLeft->setEnabled(connected);

        fFreeSpaceLogger = UINT64_MAX;
        UpdateGlobalStatus();

        if (!connected)
            return;

        const uint64_t *vals = d.ptr<uint64_t>();

        const size_t space   = vals[0];
        const size_t written = vals[1];
        const size_t rate    = float(vals[2])/vals[3];

        fFreeSpaceLogger = space;
        UpdateGlobalStatus();

        fLoggerFreeSpace->setSuffix(" MB");
        fLoggerFreeSpace->setDecimals(0);
        fLoggerFreeSpace->setValue(space*1e-6);

        if (space>   1000000)  // > 1GB
        {
            fLoggerFreeSpace->setSuffix(" GB");
            fLoggerFreeSpace->setDecimals(2);
            fLoggerFreeSpace->setValue(space*1e-9);
        }
        if (space>=  3000000)  // >= 3GB
        {
            fLoggerFreeSpace->setSuffix(" GB");
            fLoggerFreeSpace->setDecimals(1);
            fLoggerFreeSpace->setValue(space*1e-9);
        }
        if (space>=100000000)  // >= 100GB
        {
            fLoggerFreeSpace->setSuffix(" GB");
            fLoggerFreeSpace->setDecimals(0);
            fLoggerFreeSpace->setValue(space*1e-9);
        }

        fLoggerET->setTime(QTime().addSecs(rate>0?space/rate:0));
        fLoggerRate->setValue(rate*1e-3); // kB/s
        fLoggerWritten->setValue(written*1e-6);

        fLoggerRate->setSuffix(" kB/s");
        fLoggerRate->setDecimals(2);
        fLoggerRate->setValue(rate);
        if (rate>   2)  // > 2kB/s
        {
            fLoggerRate->setSuffix(" kB/s");
            fLoggerRate->setDecimals(1);
            fLoggerRate->setValue(rate);
        }
        if (rate>=100)  // >100kB/s
        {
            fLoggerRate->setSuffix(" kB/s");
            fLoggerRate->setDecimals(0);
            fLoggerRate->setValue(rate);
        }
        if (rate>=1000)  // >100kB/s
        {
            fLoggerRate->setSuffix(" MB/s");
            fLoggerRate->setDecimals(2);
            fLoggerRate->setValue(rate*1e-3);
        }
        if (rate>=10000)  // >1MB/s
        {
            fLoggerRate->setSuffix(" MB/s");
            fLoggerRate->setDecimals(1);
            fLoggerRate->setValue(rate*1e-3);
        }
        if (rate>=100000)  // >10MB/s
        {
            fLoggerRate->setSuffix(" MB/s");
            fLoggerRate->setDecimals(0);
            fLoggerRate->setValue(rate*1e-3);
        }

        if (space/1000000>static_cast<size_t>(fLoggerSpaceLeft->maximum()))
            fLoggerSpaceLeft->setValue(fLoggerSpaceLeft->maximum());  // GB
        else
            fLoggerSpaceLeft->setValue(space/1000000);  // MB
    }

    void handleLoggerFilenameNight(const DimData &d)
    {
        const bool connected = d.size()!=0;

        fLoggerFilenameNight->setEnabled(connected);
        if (!connected)
            return;

        fLoggerFilenameNight->setText(d.c_str()+4);

        const uint32_t files = d.get<uint32_t>();

        SetLedColor(fLoggerLedLog,  files&1 ? kLedGreen : kLedGray, d.time);
        SetLedColor(fLoggerLedRep,  files&2 ? kLedGreen : kLedGray, d.time);
        SetLedColor(fLoggerLedFits, files&4 ? kLedGreen : kLedGray, d.time);
    }

    void handleLoggerFilenameRun(const DimData &d)
    {
        const bool connected = d.size()!=0;

        fLoggerFilenameRun->setEnabled(connected);
        if (!connected)
            return;

        fLoggerFilenameRun->setText(d.c_str()+4);

        const uint32_t files = d.get<uint32_t>();

        SetLedColor(fLoggerLedLog,  files&1 ? kLedGreen : kLedGray, d.time);
        SetLedColor(fLoggerLedRep,  files&2 ? kLedGreen : kLedGray, d.time);
        SetLedColor(fLoggerLedFits, files&4 ? kLedGreen : kLedGray, d.time);
    }

    void handleLoggerNumSubs(const DimData &d)
    {
        const bool connected = d.size()!=0;

        fLoggerSubscriptions->setEnabled(connected);
        fLoggerOpenFiles->setEnabled(connected);
        if (!connected)
            return;

        const uint32_t *vals = d.ptr<uint32_t>();

        fLoggerSubscriptions->setValue(vals[0]);
        fLoggerOpenFiles->setValue(vals[1]);
    }


    // ===================== All ============================================

    bool CheckSize(const DimData &d, size_t sz, bool print=true) const
    {
        if (d.size()==0)
            return false;

        if (d.size()!=sz)
        {
            if (print)
                cerr << "Size mismatch in " << d.name << ": Found=" << d.size() << " Expected=" << sz << endl;
            return false;
        }

        return true;
    }

    // ===================== FAD ============================================

    uint64_t fFreeSpaceData;

    void handleFadWriteStats(const DimData &d)
    {
        const bool connected = d.size()!=0;

        fEvtBuilderET->setEnabled(connected);
        fEvtBuilderRate->setEnabled(connected);
        fEvtBuilderWritten->setEnabled(connected);
        fEvtBuilderFreeSpace->setEnabled(connected);
        fEvtBuilderSpaceLeft->setEnabled(connected);

        fFreeSpaceData = UINT64_MAX;
        UpdateGlobalStatus();

        if (!connected)
            return;

        const uint64_t *vals = d.ptr<uint64_t>();

        const size_t space   = vals[0];
        const size_t written = vals[1];
        const size_t rate    = float(vals[2])/vals[3];

        fFreeSpaceData = space;
        UpdateGlobalStatus();

        fEvtBuilderFreeSpace->setSuffix(" MB");
        fEvtBuilderFreeSpace->setDecimals(0);
        fEvtBuilderFreeSpace->setValue(space*1e-6);

        if (space>   1000000)  // > 1GB
        {
            fEvtBuilderFreeSpace->setSuffix(" GB");
            fEvtBuilderFreeSpace->setDecimals(2);
            fEvtBuilderFreeSpace->setValue(space*1e-9);
        }
        if (space>=  3000000)  // >= 3GB
        {
            fEvtBuilderFreeSpace->setSuffix(" GB");
            fEvtBuilderFreeSpace->setDecimals(1);
            fEvtBuilderFreeSpace->setValue(space*1e-9);
        }
        if (space>=100000000)  // >= 100GB
        {
            fEvtBuilderFreeSpace->setSuffix(" GB");
            fEvtBuilderFreeSpace->setDecimals(0);
            fEvtBuilderFreeSpace->setValue(space*1e-9);
        }

        fEvtBuilderET->setTime(QTime().addSecs(rate>0?space/rate:0));
        fEvtBuilderRate->setValue(rate*1e-3); // kB/s
        fEvtBuilderWritten->setValue(written*1e-6);

        fEvtBuilderRate->setSuffix(" kB/s");
        fEvtBuilderRate->setDecimals(2);
        fEvtBuilderRate->setValue(rate);
        if (rate>   2)  // > 2kB/s
        {
            fEvtBuilderRate->setSuffix(" kB/s");
            fEvtBuilderRate->setDecimals(1);
            fEvtBuilderRate->setValue(rate);
        }
        if (rate>=100)  // >100kB/s
        {
            fEvtBuilderRate->setSuffix(" kB/s");
            fEvtBuilderRate->setDecimals(0);
            fEvtBuilderRate->setValue(rate);
        }
        if (rate>=1000)  // >100kB/s
        {
            fEvtBuilderRate->setSuffix(" MB/s");
            fEvtBuilderRate->setDecimals(2);
            fEvtBuilderRate->setValue(rate*1e-3);
        }
        if (rate>=10000)  // >1MB/s
        {
            fEvtBuilderRate->setSuffix(" MB/s");
            fEvtBuilderRate->setDecimals(1);
            fEvtBuilderRate->setValue(rate*1e-3);
        }
        if (rate>=100000)  // >10MB/s
        {
            fEvtBuilderRate->setSuffix(" MB/s");
            fEvtBuilderRate->setDecimals(0);
            fEvtBuilderRate->setValue(rate*1e-3);
        }

        if (space/1000000>static_cast<size_t>(fEvtBuilderSpaceLeft->maximum()))
            fEvtBuilderSpaceLeft->setValue(fEvtBuilderSpaceLeft->maximum());  // GB
        else
            fEvtBuilderSpaceLeft->setValue(space/1000000);  // MB
    }

    void handleFadRuns(const DimData &d)
    {
        if (d.size()==0)
            return;

        if (d.size()<8)
        {
            cerr << "Size mismatch in " << d.name << ": Found=" << d.size() << " Expected>=8" << endl;
            return;
        }

        const uint32_t *ptr = d.ptr<uint32_t>();

        fEvtBldLastOpened->setValue(ptr[0]);
        fEvtBldLastClosed->setValue(ptr[1]);

        if (d.size()>=8)
            fEvtBldFilename->setText(d.ptr<char>(8));

        fEvtBldLastOpened->setEnabled(d.qos);
        fEvtBldLastClosed->setEnabled(d.qos);
        fEvtBldFilename->setEnabled(d.qos);
    }

    void handleFadStartRun(const DimData &d)
    {
        if (!CheckSize(d, 16))
            return;

        const int64_t *runs = d.ptr<int64_t>();

        fFadRunNoCur->setValue(runs[0]);
        fFadRunNoNext->setValue(runs[1]);
        fFadRunNoCur->setEnabled(runs[0]>=0);
        //fMcpStopRun->setEnabled(runs[0]>=0);

    }

    void handleFadEvents(const DimData &d)
    {
        if (!CheckSize(d, 16))
            return;

        const uint32_t *ptr = d.ptr<uint32_t>();

        fEvtsSuccessCurRun->setValue(ptr[0]);
        fEvtsSuccessTotal->setValue(ptr[1]);
        fEvtBldEventId->setValue(ptr[2]);
        fFadEvtCounter->setValue(ptr[2]);
        fEvtBldTriggerId->setValue(ptr[3]);
    }

    void handleFadTemperature(const DimData &d)
    {
        if (d.size()==0)
        {
            fFadTempMin->setEnabled(false);
            fFadTempMax->setEnabled(false);
            SetLedColor(fFadLedTemp, kLedGray, d.time);
            return;
        }

        if (!CheckSize(d, sizeof(uint16_t)+160*sizeof(float)))
            return;

        const float *ptr = d.ptr<float>(2);

        fFadTempMin->setEnabled(true);
        fFadTempMax->setEnabled(true);

        float min =  FLT_MAX;
        float max = -FLT_MAX;

        vector<float> mn(40,  FLT_MAX);
        vector<float> mx(40, -FLT_MAX);
        for (int i=0; i<160; i++)
        {
            if (!finite(ptr[i]))
                continue;

            if (ptr[i]<min)
                min = ptr[i];
            if (ptr[i]>max)
                max = ptr[i];

            if (ptr[i]<mn[i/4])
                mn[i/4] = ptr[i];
            if (ptr[i]>mx[i/4])
                mx[i/4] = ptr[i];
        }

        fFadTempMin->setValue(min);
        fFadTempMax->setValue(max);

        handleFadToolTip(d.time, fFadTempMin, mn.data());
        handleFadToolTip(d.time, fFadTempMax, mx.data());
    }

    void handleFadRefClock(const DimData &d)
    {
        if (d.size()==0)
        {
            fFadRefClockMin->setEnabled(false);
            fFadRefClockMax->setEnabled(false);
            SetLedColor(fFadLedRefClock, kLedGray, d.time);
            return;
        }

        if (!CheckSize(d, sizeof(uint16_t)+40*sizeof(float)))
            return;

        const float *ptr = d.ptr<float>(2);

        fFadRefClockMin->setEnabled(true);
        fFadRefClockMax->setEnabled(true);

        float min =  FLT_MAX;
        float max = -FLT_MAX;
        for (int i=0; i<40; i++)
        {
            if (!finite(ptr[i]))
                continue;

            if (ptr[i]<min)
                min = ptr[i];
            if (ptr[i]>max)
                max = ptr[i];
        }

        fFadRefClockMin->setValue(min);
        fFadRefClockMax->setValue(max);

        const int64_t diff = int64_t(max) - int64_t(min);

        SetLedColor(fFadLedRefClock, abs(diff)>3?kLedRed:kLedGreen, d.time);

        handleFadToolTip(d.time, fFadLedRefClock, ptr);
    }

    void handleFadRoi(const DimData &d)
    {
        if (d.size()==0)
        {
            fFadRoi->setEnabled(false);
            fFadRoiCh9->setEnabled(false);
            //SetLedColor(fFadLedRoi, kLedGray, d.time);
            return;
        }

        if (!CheckSize(d, 2*sizeof(uint16_t)))
            return;

        const uint16_t *ptr = d.ptr<uint16_t>();

        fFadRoi->setEnabled(true);
	fFadRoiCh9->setEnabled(true);

	fFadRoi->setValue(ptr[0]);
	fFadRoiCh9->setValue(ptr[1]);

        //SetLedColor(fFadLedRoi, kLedGray, d.time);
    }

    void handleDac(QPushButton *led, QSpinBox *box, const DimData &d, int idx)
    {
        if (d.size()==0)
        {
            box->setEnabled(false);
            SetLedColor(led, kLedGray, d.time);
            return;
        }

        const uint16_t *ptr = d.ptr<uint16_t>()+idx*42;

        box->setEnabled(true);
        box->setValue(ptr[40]==ptr[41]?ptr[40]:0);

        SetLedColor(led, ptr[40]==ptr[41]?kLedGreen:kLedOrange, d.time);
        handleFadToolTip(d.time, led, ptr);
    }

    void handleFadDac(const DimData &d)
    {
        if (!CheckSize(d, 8*42*sizeof(uint16_t)) && d.size()!=0)
            return;

        handleDac(fFadLedDac0, fFadDac0, d, 0);
        handleDac(fFadLedDac1, fFadDac1, d, 1);
        handleDac(fFadLedDac2, fFadDac2, d, 2);
        handleDac(fFadLedDac3, fFadDac3, d, 3);
        handleDac(fFadLedDac4, fFadDac4, d, 4);
        handleDac(fFadLedDac5, fFadDac5, d, 5);
        handleDac(fFadLedDac6, fFadDac6, d, 6);
        handleDac(fFadLedDac7, fFadDac7, d, 7);
    }

    EVENT *fEventData;
#ifdef HAVE_ROOT
    void DrawHorizontal(TH1 *hf, double xmax, TH1 &h, double scale)
    {
        for (Int_t i=1;i<=h.GetNbinsX();i++)
        {
            if (h.GetBinContent(i)<0.5 || h.GetBinContent(i)>h.GetEntries()-0.5)
                continue;

            TBox * box=new TBox(xmax, h.GetBinLowEdge(i),
                                xmax+h.GetBinContent(i)*scale,
                                h.GetBinLowEdge(i+1));

            box->SetFillStyle(0);
            box->SetLineColor(h.GetLineColor());
            box->SetLineStyle(kSolid);
            box->SetBit(kCannotPick|kNoContextMenu);
            //box->Draw();

            hf->GetListOfFunctions()->Add(box);
        }
    }
#endif
    void DisplayEventData()
    {
        if (!fEventData)
            return;

#ifdef HAVE_ROOT
	TCanvas *c = fAdcDataCanv->GetCanvas();

        TH1 *hf = dynamic_cast<TH1*>(c->FindObject("Frame"));
        TH1 *h  = dynamic_cast<TH1*>(c->FindObject("EventData"));
	TH1 *d0 = dynamic_cast<TH1*>(c->FindObject("DrsCalib0"));
	TH1 *d1 = dynamic_cast<TH1*>(c->FindObject("DrsCalib1"));
        TH1 *d2 = dynamic_cast<TH1*>(c->FindObject("DrsCalib2"));

        const int roi = fAdcPhysical->isChecked() ? 1024 : (fEventData->Roi>0 ? fEventData->Roi : 1);

        if ((hf && hf->GetNbinsX()!=roi) ||
            (dynamic_cast<TH2*>(h) && !fAdcPersistent->isChecked()) ||
            (!dynamic_cast<TH2*>(h) && fAdcPersistent->isChecked()))
        {
            delete hf;
            delete h;
            delete d0;
            delete d1;
            delete d2;
            d0 = 0;
            d1 = 0;
            d2 = 0;
	    hf = 0;
        }

	if (!hf)
        {
            hf = new TH1F("Frame", "", roi, -0.5, roi-0.5);
            hf->SetDirectory(0);
            hf->SetBit(kCanDelete);
            hf->SetStats(kFALSE);
            hf->SetYTitle("Voltage [mV]");
            hf->GetXaxis()->CenterTitle();
            hf->GetYaxis()->CenterTitle();
            hf->SetMinimum(-1250);
            hf->SetMaximum(2150);

            if (!fAdcPersistent->isChecked())
                h = new TH1F("EventData", "", roi, -0.5, roi-0.5);
            else
            {
                h = new TH2F("EventData", "", roi, -0.5, roi-0.5, 6751, -2350.5*2000/4096, 4400.5*2000/4096);
                h->SetContour(50);
                gStyle->SetPalette(1, 0);
            }

            h->SetDirectory(0);
            h->SetBit(kCanDelete);
            h->SetMarkerStyle(kFullDotMedium);
            h->SetMarkerColor(kBlue);

            c->GetListOfPrimitives()->Add(hf, "");

            if (dynamic_cast<TH2*>(h))
                c->GetListOfPrimitives()->Add(h, "col same");
        }

        if (d0 && !(fDrsCalibBaselineOn->isChecked() && fDrsCalibBaseline->value()>0))
        {
            delete d0;
            d0 = 0;
        }
        if (d1 && !(fDrsCalibGainOn->isChecked() && fDrsCalibGain->value()>0))
        {
            delete d1;
            d1 = 0;
        }
        if (d2 && !(fDrsCalibTrgOffsetOn->isChecked() && fDrsCalibTrgOffset->value()>0))
        {
            delete d2;
            d2 = 0;
        }

        if (!d0 && fDrsCalibBaselineOn->isChecked() && fDrsCalibBaseline->value()>0)
        {
            d0 = new TH1F("DrsCalib0", "", roi, -0.5, roi-0.5);
	    d0->SetDirectory(0);
            d0->SetBit(kCanDelete);
	    d0->SetMarkerStyle(kFullDotSmall);
	    d0->SetMarkerColor(kRed);
	    d0->SetLineColor(kRed);
            c->GetListOfPrimitives()->Add(d0, "PEX0same");
        }

        if (!d1 && fDrsCalibGainOn->isChecked() && fDrsCalibGain->value()>0)
        {
            d1 = new TH1F("DrsCalib1", "", roi, -0.5, roi-0.5);
	    d1->SetDirectory(0);
            d1->SetBit(kCanDelete);
	    d1->SetMarkerStyle(kFullDotSmall);
	    d1->SetMarkerColor(kMagenta);
            d1->SetLineColor(kMagenta);
            c->GetListOfPrimitives()->Add(d1, "PEX0same");
        }

        if (!d2 && fDrsCalibTrgOffsetOn->isChecked() && fDrsCalibTrgOffset->value()>0)
        {
            d2 = new TH1F("DrsCalib2", "", roi, -0.5, roi-0.5);
            d2->SetDirectory(0);
            d2->SetBit(kCanDelete);
	    d2->SetMarkerStyle(kFullDotSmall);
	    d2->SetMarkerColor(kGreen);
            d2->SetLineColor(kGreen);
            c->GetListOfPrimitives()->Add(d2, "PEX0same");
        }

        if (!dynamic_cast<TH2*>(h) && !c->GetListOfPrimitives()->FindObject(h))
            c->GetListOfPrimitives()->Add(h, "PLsame");

        // -----------------------------------------------------------

        const uint32_t p =
            fAdcChannel->value()    +
            fAdcChip->value()   *  9+
            fAdcBoard->value()  * 36+
            fAdcCrate->value()  *360;

        ostringstream str;
        str << "CBPX = " << fAdcCrate->value() << '|' << fAdcBoard->value() << '|' << fAdcChip->value() << '|' << fAdcChannel->value() << " (" << p << ")";
        str << "   EventNum = " << fEventData->EventNum;
        str << "   TriggerNum = " << fEventData->TriggerNum;
        str << "   TriggerType = " << fEventData->TriggerType;
        str << "   BoardTime = " << fEventData->BoardTime[fAdcBoard->value()+fAdcCrate->value()*10];
        str << "   (" << Time(fEventData->PCTime, fEventData->PCUsec) << ")";
        hf->SetTitle(str.str().c_str());
        str.str("");
        str << "ADC Pipeline (start cell: " << fEventData->StartPix[p] << ")";
	hf->SetXTitle(str.str().c_str());

        // -----------------------------------------------------------

        const int16_t start = fEventData->StartPix[p];

        fDrsCalibBaseline->setEnabled(fDrsCalibBaseline->value()>0);
        fDrsCalibGain->setEnabled(fDrsCalibGain->value()>0);
        fDrsCalibTrgOffset->setEnabled(fDrsCalibTrgOffset->value()>0);
        fDrsCalibROI->setEnabled(fDrsCalibROI->value()>0);

        fDrsCalibBaseline2->setEnabled(fDrsCalibBaseline->value()>0);
        fDrsCalibGain2->setEnabled(fDrsCalibGain->value()>0);
        fDrsCalibTrgOffset2->setEnabled(fDrsCalibTrgOffset->value()>0);
        fDrsCalibROI2->setEnabled(fDrsCalibROI->value()>0);

        SetLedColor(fFadLedDrsBaseline, fDrsCalibBaseline->value()>0 ?kLedGreen:kLedGray, Time());
        SetLedColor(fFadLedDrsGain,     fDrsCalibGain->value()>0     ?kLedGreen:kLedGray, Time());
        SetLedColor(fFadLedDrsTrgOff,   fDrsCalibTrgOffset->value()>0?kLedGreen:kLedGray, Time());

        if (d0)//fDrsCalibBaseline->value()==0  || start<0)
            d0->Reset();
        if (d1)//fDrsCalibGain->value()==0      || start<0)
            d1->Reset();
        if (d2)//fDrsCalibTrgOffset->value()==0 || start<0)
            d2->Reset();

        if (!dynamic_cast<TH2*>(h))
            h->Reset();
        if (d0)
            d0->SetEntries(0);
        if (d1)
            d1->SetEntries(0);
        if (d2)
            d2->SetEntries(0);

        for (int i=0; i<fEventData->Roi; i++)
        {
            // FIXME: physcial: i -> (i+start)%1024
            // FIXME: logical:  i ->  i

            const int ii = fAdcPhysical->isChecked() ? (i+start)%1024 : i;

            //if (dynamic_cast<TH2*>(h))
                h->Fill(ii, reinterpret_cast<float*>(fEventData->Adc_Data)[p*fEventData->Roi+i]);
            //else
            //    h->SetBinContent(i+1, reinterpret_cast<float*>(fEventData->Adc_Data)[p*fEventData->Roi+i]);
            if (start<0)
                continue;

            if (d0)
            {
                d0->SetBinContent(ii+1, fDrsCalibration[1440*1024*0 + p*1024+(start+i)%1024]);
                d0->SetBinError(ii+1,   fDrsCalibration[1440*1024*1 + p*1024+(start+i)%1024]);

            }
            if (d1)
            {
                d1->SetBinContent(ii+1, fDrsCalibration[1440*1024*2 + p*1024+(start+i)%1024]);
                d1->SetBinError(ii+1,   fDrsCalibration[1440*1024*3 + p*1024+(start+i)%1024]);
            }
            if (d2)
            {
                d2->SetBinContent(ii+1, fDrsCalibration[1440*1024*4 + p*1024 + i]);
                d2->SetBinError(ii+1,   fDrsCalibration[1440*1024*5 + p*1024 + i]);
            }
        }

        // -----------------------------------------------------------
        if (fAdcDynamicScale->isEnabled() && fAdcDynamicScale->isChecked())
        {
            h->SetMinimum();
            h->SetMaximum();

            hf->SetMinimum(h->GetMinimum());
            hf->SetMaximum(h->GetMaximum());
        }
        if (fAdcManualScale->isEnabled() && fAdcManualScale->isChecked())
	{
            if (h->GetMinimumStored()==-1111)
            {
                h->SetMinimum(-1150);//-1026);
                hf->SetMinimum(-1150);//-1026);
            }
            if (h->GetMaximumStored()==-1111)
            {
                h->SetMaximum(2150);//1025);
                hf->SetMaximum(2150);//1025);
            }
        }

        if (fAdcAutoScale->isEnabled() && fAdcAutoScale->isChecked())
        {
            h->SetMinimum();
            h->SetMaximum();

            if (h->GetMinimum()<hf->GetMinimum())
                hf->SetMinimum(h->GetMinimum());
            if (h->GetMaximum()>hf->GetMaximum())
                hf->SetMaximum(h->GetMaximum());
        }

        if (dynamic_cast<TH2*>(h))
        {
            h->SetMinimum();
            h->SetMaximum();
        }

        // -----------------------------------------------------------

        const int imin = ceil(hf->GetMinimum());
        const int imax = floor(hf->GetMaximum());

        TH1S hd("", "", imax-imin+1, imin-0.5, imax+0.5);
        hd.SetDirectory(0);
        TH1S h0("", "", imax-imin+1, imin-0.5, imax+0.5);
        h0.SetDirectory(0);
        TH1S h1("", "", imax-imin+1, imin-0.5, imax+0.5);
        h1.SetDirectory(0);
        TH1S h2("", "", imax-imin+1, imin-0.5, imax+0.5);
        h2.SetDirectory(0);
        hd.SetLineColor(h->GetLineColor());
        if (d0)
            h0.SetLineColor(d0->GetLineColor());
        if (d1)
            h1.SetLineColor(d1->GetLineColor());
        if (d2)
            h2.SetLineColor(d2->GetLineColor());

        for (int i=0; i<fEventData->Roi; i++)
        {
            if (!dynamic_cast<TH2*>(h))
                hd.Fill(h->GetBinContent(i+1));
            if (d0)
                h0.Fill(d0->GetBinContent(i+1));
            if (d1)
                h1.Fill(d1->GetBinContent(i+1));
            if (d2)
                h2.Fill(d2->GetBinContent(i+1));
        }

        double mm = hd.GetMaximum(hd.GetEntries());
        if (h0.GetMaximum(h0.GetEntries())>mm)
            mm = h0.GetMaximum();
        if (h1.GetMaximum(h1.GetEntries())>mm)
            mm = h1.GetMaximum();
        if (h2.GetMaximum(h2.GetEntries())>mm)
            mm = h2.GetMaximum();

        TIter Next(hf->GetListOfFunctions());
        TObject *obj = 0;
        while ((obj=Next()))
            if (dynamic_cast<TBox*>(obj))
                delete hf->GetListOfFunctions()->Remove(obj);

        const double l = h->GetBinLowEdge(h->GetXaxis()->GetLast()+1);
        const double m = c->GetX2();

        const double scale = 0.9*(m-l)/mm;

        DrawHorizontal(hf, l, h2, scale);
        DrawHorizontal(hf, l, h1, scale);
        DrawHorizontal(hf, l, h0, scale);
        DrawHorizontal(hf, l, hd, scale);

        // -----------------------------------------------------------

	c->Modified();
	c->Update();
#endif
    }

    void handleFadRawData(const DimData &d)
    {
	if (d.size()==0)
            return;

        if (fAdcStop->isChecked())
            return;

	const EVENT &dat = d.ref<EVENT>();

        if (d.size()<sizeof(EVENT))
        {
            cerr << "Size mismatch in " << d.name << ": Found=" << d.size() << " Expected>=" << sizeof(EVENT) << endl;
            return;
        }

        if (d.size()!=sizeof(EVENT)+dat.Roi*4*1440+dat.Roi*4*160)
        {
            cerr << "Size mismatch in " << d.name << ": Found=" << d.size() << " Expected=" << dat.Roi*4*1440+sizeof(EVENT) << " [roi=" << dat.Roi << "]" << endl;
            return;
        }

        delete [] reinterpret_cast<char*>(fEventData);
        fEventData = reinterpret_cast<EVENT*>(new char[d.size()]);
        memcpy(fEventData, d.ptr<void>(), d.size());

        DisplayEventData();
    }

    void handleFadEventData(const DimData &d)
    {
        if (!CheckSize(d, 4*1440*sizeof(float)))
            return;

        if (fEventsStop->isChecked())
            return;

        const float *ptr = d.ptr<float>();

        valarray<double> arr1(1440);
        valarray<double> arr2(1440);
        valarray<double> arr3(1440);
        valarray<double> arr4(1440);

        for (vector<PixelMapEntry>::const_iterator it=fPixelMap.begin(); it!=fPixelMap.end(); it++)
        {
            arr1[it->index] = ptr[0*1440+it->hw()];
            arr2[it->index] = ptr[1*1440+it->hw()];
            arr3[it->index] = ptr[2*1440+it->hw()];
            arr4[it->index] = ptr[3*1440+it->hw()];
        }

        fEventCanv1->SetData(arr1);
        fEventCanv2->SetData(arr2);
        fEventCanv3->SetData(arr3);
        fEventCanv4->SetData(arr4);

        fEventCanv1->updateCamera();
        fEventCanv2->updateCamera();
        fEventCanv3->updateCamera();
        fEventCanv4->updateCamera();
    }

    vector<float> fDrsCalibration;

    void handleFadDrsCalibration(const DimData &d)
    {
        const size_t sz = 1024*1440*6+1024*160*2;

        if (d.size()==0)
        {
            fDrsCalibBaseline->setValue(-1);
            fDrsCalibGain->setValue(-1);
            fDrsCalibTrgOffset->setValue(-1);
            fDrsCalibROI->setValue(-1);

            fDrsCalibBaseline2->setValue(-1);
            fDrsCalibGain2->setValue(-1);
            fDrsCalibTrgOffset2->setValue(-1);
            fDrsCalibROI2->setValue(-1);

            fDrsCalibration.assign(sz, 0);
            DisplayEventData();
            return;
        }

        if (!CheckSize(d, sz*sizeof(float)+4*sizeof(uint32_t)))
            // Do WHAT?
            return;

        const uint32_t *run = d.ptr<uint32_t>();

        fDrsCalibROI->setValue(run[0]);
        fDrsCalibBaseline->setValue(run[1]);
        fDrsCalibGain->setValue(run[2]);
        fDrsCalibTrgOffset->setValue(run[3]);

        fDrsCalibROI2->setValue(run[0]);
        fDrsCalibBaseline2->setValue(run[1]);
        fDrsCalibGain2->setValue(run[2]);
        fDrsCalibTrgOffset2->setValue(run[3]);

        const float *dat = d.ptr<float>(sizeof(uint32_t)*4);
        fDrsCalibration.assign(dat, dat+sz);

        DisplayEventData();
    }

//    vector<uint8_t> fFadConnections;

    void handleFadConnections(const DimData &d)
    {
        if (!CheckSize(d, 41))
        {
            fStatusEventBuilderLabel->setText("Offline");
            fStatusEventBuilderLabel->setToolTip("FADs or fadctrl seems to be offline.");
            fGroupEthernet->setEnabled(false);
            fGroupOutput->setEnabled(false);

            SetLedColor(fStatusEventBuilderLed, kLedGray, d.time);
            return;
        }

        const uint8_t *ptr = d.ptr<uint8_t>();

        for (int i=0; i<40; i++)
        {
            const uint8_t stat1 = ptr[i]&3;
            const uint8_t stat2 = ptr[i]>>3;

            if (stat1==0 && stat2==0)
            {
                SetLedColor(fFadLED[i], kLedGray,   d.time);
                continue;
            }
            if (stat1>=2 && stat2==8)
            {
                SetLedColor(fFadLED[i], stat1==2?kLedGreen:kLedGreenCheck,  d.time);
                continue;
            }

            if (stat1==1 && stat2==1)
                SetLedColor(fFadLED[i], kLedRed, d.time);
            else
                SetLedColor(fFadLED[i], kLedOrange, d.time);
        }


        const bool runs = ptr[40]!=0;

        fStatusEventBuilderLabel->setText(runs?"Running":"Not running");
        fStatusEventBuilderLabel->setToolTip(runs?"Event builder thread running.":"Event builder thread stopped.");

        fGroupEthernet->setEnabled(runs);
        fGroupOutput->setEnabled(runs);

        SetLedColor(fStatusEventBuilderLed, runs?kLedGreen:kLedRed, d.time);

//        fFadConnections.assign(ptr, ptr+40);
    }

    template<typename T>
        void handleFadToolTip(const Time &time, QWidget *w, T *ptr)
    {
        ostringstream tip;
        tip << "<table border='1'><tr><th colspan='11'>" << time.GetAsStr() << " (UTC)</th></tr><tr><th></th>";
        for (int b=0; b<10; b++)
            tip << "<th>" << b << "</th>";
        tip << "</tr>";

        for (int c=0; c<4; c++)
        {
            tip << "<tr><th>" << c << "</th>";
            for (int b=0; b<10; b++)
                tip << "<td>" << ptr[c*10+b] << "</td>";
            tip << "</tr>";
        }
        tip << "</table>";

        w->setToolTip(tip.str().c_str());
    }

    template<typename T, class S>
        void handleFadMinMax(const DimData &d, QPushButton *led, S *wmin, S *wmax=0)
    {
        if (!CheckSize(d, 42*sizeof(T)))
            return;

        const T *ptr = d.ptr<T>();
        const T  min = ptr[40];
        const T  max = ptr[41];

        if (max==0 && min>max)
            SetLedColor(led, kLedGray, d.time);
        else
            SetLedColor(led, min==max?kLedGreen: kLedOrange, d.time);

        if (!wmax && max!=min)
            wmin->setValue(0);
        else
            wmin->setValue(min);

        if (wmax)
            wmax->setValue(max);

        handleFadToolTip(d.time, led, ptr);
    }

    void handleFadFwVersion(const DimData &d)
    {
        handleFadMinMax<float, QDoubleSpinBox>(d, fFadLedFwVersion, fFadFwVersion);
    }

    void handleFadRunNumber(const DimData &d)
    {
        handleFadMinMax<uint32_t, QSpinBox>(d, fFadLedRunNumber, fFadRunNumber);
    }

    void handleFadPrescaler(const DimData &d)
    {
        handleFadMinMax<uint16_t, QSpinBox>(d, fFadLedPrescaler, fFadPrescaler);
    }

    void handleFadDNA(const DimData &d)
    {
        if (!CheckSize(d, 40*sizeof(uint64_t)))
            return;

        const uint64_t *ptr = d.ptr<uint64_t>();

        ostringstream tip;
        tip << "<table width='100%'>";
        tip << "<tr><th>Crate</th><td></td><th>Board</th><td></td><th>DNA</th></tr>";

        for (int i=0; i<40; i++)
        {
            tip << dec;
            tip << "<tr>";
            tip << "<td align='center'>" << i/10 << "</td><td>:</td>";
            tip << "<td align='center'>" << i%10 << "</td><td>:</td>";
            tip << hex;
            tip << "<td>0x" << setfill('0') << setw(16) << ptr[i] << "</td>";
            tip << "</tr>";
        }
        tip << "</table>";

        fFadDNA->setText(tip.str().c_str());
    }

    void SetFadLed(QPushButton *led, const DimData &d, uint16_t bitmask, bool invert=false)
    {
        if (d.size()==0)
        {
            SetLedColor(led, kLedGray, d.time);
            return;
        }

        const bool      quality = d.ptr<uint16_t>()[0]&bitmask;
        const bool      value   = d.ptr<uint16_t>()[1]&bitmask;
        const uint16_t *ptr     = d.ptr<uint16_t>()+2;

        SetLedColor(led, quality?kLedOrange:(value^invert?kLedGreen:kLedGreenBar), d.time);

        ostringstream tip;
        tip << "<table border='1'><tr><th colspan='11'>" << d.time.GetAsStr() << " (UTC)</th></tr><tr><th></th>";
        for (int b=0; b<10; b++)
            tip << "<th>" << b << "</th>";
        tip << "</tr>";

        /*
	 tip << "<tr>" << hex;
	 tip << "<th>" << d.ptr<uint16_t>()[0] << " " << (d.ptr<uint16_t>()[0]&bitmask) << "</th>";
	 tip << "<th>" << d.ptr<uint16_t>()[1] << " " << (d.ptr<uint16_t>()[1]&bitmask) << "</th>";
	 tip << "</tr>";
	 */

        for (int c=0; c<4; c++)
        {
            tip << "<tr><th>" << dec << c << "</th>" << hex;
            for (int b=0; b<10; b++)
            {
                tip << "<td>"
                    << (ptr[c*10+b]&bitmask)
                    << "</td>";
            }
            tip << "</tr>";
        }
        tip << "</table>";

        led->setToolTip(tip.str().c_str());
    }

    void handleFadStatus(const DimData &d)
    {
        if (d.size()!=0 && !CheckSize(d, 42*sizeof(uint16_t)))
            return;

        SetFadLed(fFadLedDrsEnabled,     d, FAD::EventHeader::kDenable);
        SetFadLed(fFadLedDrsWrite,       d, FAD::EventHeader::kDwrite);
        SetFadLed(fFadLedDcmLocked,      d, FAD::EventHeader::kDcmLocked);
        SetFadLed(fFadLedDcmReady,       d, FAD::EventHeader::kDcmReady);
        SetFadLed(fFadLedSpiSclk,        d, FAD::EventHeader::kSpiSclk);
        SetFadLed(fFadLedRefClockTooLow, d, FAD::EventHeader::kRefClkTooLow, true);
        SetFadLed(fFadLedBusyOn,         d, FAD::EventHeader::kBusyOn);
        SetFadLed(fFadLedBusyOff,        d, FAD::EventHeader::kBusyOff);
        SetFadLed(fFadLedTriggerLine,    d, FAD::EventHeader::kTriggerLine);
        SetFadLed(fFadLedContTrigger,    d, FAD::EventHeader::kContTrigger);
        SetFadLed(fFadLedSocket,         d, FAD::EventHeader::kSock17);
        SetFadLed(fFadLedPllLock,        d, 0xf000);
    }

    void handleFadStatistics1(const DimData &d)
    {
        if (!CheckSize(d, sizeof(GUI_STAT)))
            return;

        const GUI_STAT &stat = d.ref<GUI_STAT>();

        fFadBufferMax->setValue(stat.totMem/1000000);  // Memory::allocated
        fFadBuffer->setMaximum(stat.maxMem/100);       // g_maxMem
        fFadBuffer->setValue(stat.usdMem/100);         // Memory::inuse

        uint32_t sum = 0;
        int cnt = 0;

        for (int i=0; i<40; i++)
        {
            if (stat.numConn[i]==1)
            {
                sum += stat.rateBytes[i];
                cnt++;
            }
        }

        fFadEvtConn->setValue(cnt);

        fFadEvtBufNew->setValue(stat.bufNew);  // Incomplete in buffer (evtCtrl)
        fFadEvtBufEvt->setValue(stat.bufTot);  // Complete events in buffer (max_inuse)

        fFadEvtCheck->setValue(stat.bufEvt);  // Complete in buffer
        fFadEvtWrite->setValue(stat.bufWrite);
        fFadEvtProc->setValue(stat.bufProc);

        if (stat.deltaT==0)
            return;

        //fFadEthernetRateMin->setValue(min/stat.deltaT);
        //fFadEthernetRateMax->setValue(max/stat.deltaT);
        fFadEthernetRateTot->setValue(sum/stat.deltaT);
        fFadEthernetRateAvg->setValue(cnt==0 ? 0 : sum/cnt/stat.deltaT);

        fFadTransmission->setValue(1000*stat.rateNew/stat.deltaT);
        fFadWriteRate->setValue(1000*stat.rateWrite/stat.deltaT);
    }

    /*
    void handleFadStatistics2(const DimData &d)
    {
        if (!CheckSize(d, sizeof(EVT_STAT)))
            return;

        //const EVT_STAT &stat = d.ref<EVT_STAT>();
    }*/

    void handleFadFileFormat(const DimData &d)
    {
        if (!CheckSize(d, sizeof(uint16_t)))
            return;

        const uint16_t &fmt = d.get<uint16_t>();

        SetLedColor(fFadLedFileFormatNone,  fmt==FAD::kNone ?kLedGreen:kLedGray, d.time);
        SetLedColor(fFadLedFileFormatDebug, fmt==FAD::kDebug?kLedGreen:kLedGray, d.time);
        SetLedColor(fFadLedFileFormatRaw,   fmt==FAD::kRaw  ?kLedGreen:kLedGray, d.time);
        SetLedColor(fFadLedFileFormatFits,  fmt==FAD::kFits ?kLedGreen:kLedGray, d.time);
        SetLedColor(fFadLedFileFormatZFits, fmt==FAD::kZFits?kLedGreen:kLedGray, d.time);
        SetLedColor(fFadLedFileFormatCalib, fmt==FAD::kCalib?kLedGreen:kLedGray, d.time);
    }

    // ===================== FTM ============================================

    FTM::DimTriggerRates fTriggerRates;

    void UpdateTriggerRate(const FTM::DimTriggerRates &sdata)
    {
#ifdef HAVE_ROOT
        TCanvas *c = fFtmRateCanv->GetCanvas();

        TH1 *h = (TH1*)c->FindObject("TimeFrame");

        if (sdata.fTimeStamp<=fTriggerRates.fTimeStamp)
        {
            fGraphFtmRate.Set(0);

            const double tm = Time().RootTime();

            h->SetBins(1, tm, tm+60);
            h->GetXaxis()->SetTimeFormat("%M'%S\"%F1995-01-01 00:00:00 GMT");
            h->GetXaxis()->SetTitle("Time");

            c->Modified();
            c->Update();
            return;
        }

        const double t1 = h->GetXaxis()->GetXmax();
        const double t0 = h->GetXaxis()->GetXmin();

        const double now = t0+sdata.fTimeStamp/1000000.;

        h->SetBins(h->GetNbinsX()+1, t0, now+1);
        fGraphFtmRate.SetPoint(fGraphFtmRate.GetN(), now, sdata.fTriggerRate);

        if (t1-t0>300)
        {
            h->GetXaxis()->SetTimeFormat("%Hh%M'%F1995-01-01 00:00:00 GMT");
            h->GetXaxis()->SetTitle("Time");
        }

        h->SetMinimum(0);

        c->Modified();
        c->Update();
#endif
    }

    void UpdateRatesCam(const FTM::DimTriggerRates &sdata)
    {
        if (fThresholdIdx->value()>=0)
        {
            const int isw = fThresholdIdx->value();
            const int ihw = fPatchMapHW[isw];
            fPatchRate->setValue(sdata.fPatchRate[ihw]);
            fBoardRate->setValue(sdata.fBoardRate[ihw/4]);
        }

        const bool b = fBoardRatesEnabled->isChecked();

        valarray<double> dat(0., 1440);

        // fPatch converts from software id to software patch id
        for (int i=0; i<1440; i++)
        {
            const int ihw = fPixelMap.index(i).hw()/9;
            dat[i] = b ? sdata.fBoardRate[ihw/4] : sdata.fPatchRate[ihw];
        }

        fRatesCanv->SetData(dat);
        fRatesCanv->updateCamera();
    }

    int64_t fTimeStamp0;

    void on_fBoardRatesEnabled_toggled(bool)
    {
        UpdateRatesCam(fTriggerRates);
    }

    void UpdateRatesGraphs(const FTM::DimTriggerRates &sdata)
    {
#ifdef HAVE_ROOT
        if (fTimeStamp0<0)
        {
            fTimeStamp0 = sdata.fTimeStamp;
            return;
        }

        TCanvas *c = fFtmRateCanv->GetCanvas();

        TH1 *h = (TH1*)c->FindObject("TimeFrame");

        const double tdiff = sdata.fTimeStamp-fTimeStamp0;
        fTimeStamp0 = sdata.fTimeStamp;

        if (tdiff<0)
        {
            for (int i=0; i<160; i++)
                fGraphPatchRate[i].Set(0);
            for (int i=0; i<40; i++)
                fGraphBoardRate[i].Set(0);

            return;
        }

        //const double t1 = h->GetXaxis()->GetXmax();
        const double t0 = h->GetXaxis()->GetXmin();

        for (int i=0; i<160; i++)
            if (fFtuStatus[i/4]>0)
                fGraphPatchRate[i].SetPoint(fGraphPatchRate[i].GetN(),
                                            t0+sdata.fTimeStamp/1000000., sdata.fPatchRate[i]);
        for (int i=0; i<40; i++)
            if (fFtuStatus[i]>0)
                fGraphBoardRate[i].SetPoint(fGraphBoardRate[i].GetN(),
                                            t0+sdata.fTimeStamp/1000000., sdata.fBoardRate[i]);

        c->Modified();
        c->Update();
#endif
    }

    void handleFtmTriggerRates(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimTriggerRates)))
            return;

        const FTM::DimTriggerRates &sdata = d.ref<FTM::DimTriggerRates>();

        fFtmTime->setText(QString::number(sdata.fTimeStamp/1000000., 'f', 6)+ " s");
        fTriggerCounter->setText(QString::number(sdata.fTriggerCounter));

        if (sdata.fTimeStamp>0)
            fTriggerCounterRate->setValue(1000000.*sdata.fTriggerCounter/sdata.fTimeStamp);
        else
            fTriggerCounterRate->setValue(0);

        // ----------------------------------------------

        fOnTime->setText(QString::number(sdata.fOnTimeCounter/1000000., 'f', 6)+" s");

        if (sdata.fTimeStamp>0)
            fOnTimeRel->setValue(100.*sdata.fOnTimeCounter/sdata.fTimeStamp);
        else
            fOnTimeRel->setValue(0);

        // ----------------------------------------------

        UpdateTriggerRate(sdata);
        UpdateRatesGraphs(sdata);
        UpdateRatesCam(sdata);

        fTriggerRates = sdata;
    }

    void handleFtmCounter(const DimData &d)
    {
        if (!CheckSize(d, sizeof(uint32_t)*6))
            return;

        const uint32_t *sdata = d.ptr<uint32_t>();

        fFtmCounterH->setValue(sdata[0]);
        fFtmCounterS->setValue(sdata[1]);
        fFtmCounterD->setValue(sdata[2]);
        fFtmCounterF->setValue(sdata[3]);
        fFtmCounterE->setValue(sdata[4]);
        fFtmCounterR->setValue(sdata[5]);
    }

    void handleFtmDynamicData(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimDynamicData)))
            return;

        const FTM::DimDynamicData &sdata = d.ref<FTM::DimDynamicData>();

        fFtmTemp0->setValue(sdata.fTempSensor[0]*0.1);
        fFtmTemp1->setValue(sdata.fTempSensor[1]*0.1);
        fFtmTemp2->setValue(sdata.fTempSensor[2]*0.1);
        fFtmTemp3->setValue(sdata.fTempSensor[3]*0.1);

        SetLedColor(fClockCondLed, sdata.fState&FTM::kFtmLocked ? kLedGreen : kLedRed, d.time);
    }

    void DisplayRates()
    {
#ifdef HAVE_ROOT
        TCanvas *c = fFtmRateCanv->GetCanvas();

        TList * l = c->GetListOfPrimitives();


        while (c->FindObject("PatchRate"))
            l->Remove(c->FindObject("PatchRate"));

        while (c->FindObject("BoardRate"))
            l->Remove(c->FindObject("BoardRate"));

        if (fRatePatch1->value()>=0)
        {
            fGraphPatchRate[fRatePatch1->value()].SetLineColor(kRed);
            fGraphPatchRate[fRatePatch1->value()].SetMarkerColor(kRed);
            l->Add(&fGraphPatchRate[fRatePatch1->value()], "PL");
        }
        if (fRatePatch2->value()>=0)
        {
            fGraphPatchRate[fRatePatch2->value()].SetLineColor(kGreen);
            fGraphPatchRate[fRatePatch2->value()].SetMarkerColor(kGreen);
            l->Add(&fGraphPatchRate[fRatePatch2->value()], "PL");
        }
        if (fRateBoard1->value()>=0)
        {
            fGraphBoardRate[fRateBoard1->value()].SetLineColor(kMagenta);
            fGraphBoardRate[fRateBoard1->value()].SetMarkerColor(kMagenta);
            l->Add(&fGraphBoardRate[fRateBoard1->value()], "PL");
        }
        if (fRateBoard2->value()>=0)
        {
            fGraphBoardRate[fRateBoard2->value()].SetLineColor(kCyan);
            fGraphBoardRate[fRateBoard2->value()].SetMarkerColor(kCyan);
            l->Add(&fGraphBoardRate[fRateBoard2->value()], "PL");
        }

        c->Modified();
        c->Update();
#endif
    }

    FTM::DimStaticData fFtmStaticData;

    void SetFtuLed(int idx, int counter, const Time &t)
    {
        if (counter==0 || counter>3)
            counter = 3;

        if (counter<0)
            counter = 0;

        const LedColor_t col[4] = { kLedGray, kLedGreen, kLedOrange, kLedRed };

        SetLedColor(fFtuLED[idx], col[counter], t);

        fFtuStatus[idx] = counter;
    }

    void SetFtuStatusLed(const Time &t)
    {
        const int max = fFtuStatus.max();

        switch (max)
        {
        case 0:
            SetLedColor(fStatusFTULed, kLedGray, t);
            fStatusFTULabel->setText("All disabled");
            fStatusFTULabel->setToolTip("All FTUs are disabled");
            break;

        case 1:
            SetLedColor(fStatusFTULed, kLedGreen, t);
            fStatusFTULabel->setToolTip("Communication with FTU is smooth.");
            fStatusFTULabel->setText("ok");
            break;

        case 2:
            SetLedColor(fStatusFTULed, kLedOrange, t);
            fStatusFTULabel->setText("Warning");
            fStatusFTULabel->setToolTip("At least one FTU didn't answer immediately");
            break;

        case 3:
            SetLedColor(fStatusFTULed, kLedRed, t);
            fStatusFTULabel->setToolTip("At least one FTU didn't answer!");
            fStatusFTULabel->setText("ERROR");
            break;
        }

        const int cnt = count(&fFtuStatus[0], &fFtuStatus[40], 0);
        fFtuAllOn->setEnabled(cnt!=0);
        fFtuAllOff->setEnabled(cnt!=40);
    }

    void handleFtmStaticData(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimStaticData)))
            return;

        const FTM::DimStaticData &sdata = d.ref<FTM::DimStaticData>();

        fTriggerInterval->setValue(sdata.fTriggerInterval);
        fPhysicsCoincidence->setValue(sdata.fMultiplicityPhysics);
        fCalibCoincidence->setValue(sdata.fMultiplicityCalib);
        fPhysicsWindow->setValue(sdata.fWindowPhysics);
        fCalibWindow->setValue(sdata.fWindowCalib);

        fTriggerDelay->setValue(sdata.fDelayTrigger);
        fTimeMarkerDelay->setValue(sdata.fDelayTimeMarker);
        fDeadTime->setValue(sdata.fDeadTime);

        fClockCondR0->setValue(sdata.fClockConditioner[0]);
        fClockCondR1->setValue(sdata.fClockConditioner[1]);
        fClockCondR8->setValue(sdata.fClockConditioner[2]);
        fClockCondR9->setValue(sdata.fClockConditioner[3]);
        fClockCondR11->setValue(sdata.fClockConditioner[4]);
        fClockCondR13->setValue(sdata.fClockConditioner[5]);
        fClockCondR14->setValue(sdata.fClockConditioner[6]);
        fClockCondR15->setValue(sdata.fClockConditioner[7]);

        const uint32_t R0  = sdata.fClockConditioner[0];
        const uint32_t R14 = sdata.fClockConditioner[6];
        const uint32_t R15 = sdata.fClockConditioner[7];

        const uint32_t Ndiv = (R15&0x1ffff00)<<2;
        const uint32_t Rdiv = (R14&0x007ff00)>>8;
        const uint32_t Cdiv = (R0 &0x000ff00)>>8;

        double freq = 40.*Ndiv/(Rdiv*Cdiv);

        fClockCondFreqRes->setValue(freq);

        //fClockCondFreq->setEditText("");
        fClockCondFreq->setCurrentIndex(0);

        fTriggerSeqPed->setValue(sdata.fTriggerSeqPed);
        fTriggerSeqLPint->setValue(sdata.fTriggerSeqLPint);
        fTriggerSeqLPext->setValue(sdata.fTriggerSeqLPext);

        fLpIntIntensity->setValue(sdata.fIntensityLPint);
        fLpExtIntensity->setValue(sdata.fIntensityLPext);

        fLpIntGroup1->setChecked(sdata.HasLPintG1());
        fLpIntGroup2->setChecked(sdata.HasLPintG2());
        fLpExtGroup1->setChecked(sdata.HasLPextG1());
        fLpExtGroup2->setChecked(sdata.HasLPextG2());

        fEnableTrigger->setChecked(sdata.HasTrigger());
        fEnableVeto->setChecked(sdata.HasVeto());
        fEnableExt1->setChecked(sdata.HasExt1());
        fEnableExt2->setChecked(sdata.HasExt2());
        fEnableClockCond->setChecked(sdata.HasClockConditioner());

        uint16_t multiplicity = sdata.fMultiplicity[0];

        for (int i=0; i<40; i++)
        {
            if (!sdata.IsActive(i))
                SetFtuLed(i, -1, d.time);
            else
            {
                if (fFtuStatus[i]==0)
                    SetFtuLed(i, 1, d.time);
            }
            fFtuLED[i]->setChecked(false);

            if (sdata.fMultiplicity[i]!=multiplicity)
                multiplicity = -1;

        }
        SetFtuStatusLed(d.time);

        fNoutof4Val->setValue(multiplicity);

        for (vector<PixelMapEntry>::const_iterator it=fPixelMap.begin(); it!=fPixelMap.end(); it++)
            fRatesCanv->SetEnable(it->index, sdata.IsEnabled(it->hw()));

        const PixelMapEntry &entry = fPixelMap.index(fPixelIdx->value());
        fPixelEnable->setChecked(sdata.IsEnabled(entry.hw()));

        if (fThresholdIdx->value()>=0)
        {
            const int isw = fThresholdIdx->value();
            const int ihw = fPatchMapHW[isw];
            fThresholdVal->setValue(sdata.fThreshold[ihw]);
        }

        fPrescalingVal->setValue(sdata.fPrescaling[0]);

        fFtmStaticData = sdata;
    }

    void handleFtmPassport(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimPassport)))
            return;

        const FTM::DimPassport &sdata = d.ref<FTM::DimPassport>();

        stringstream str1, str2;
        str1 << hex << "0x" << setfill('0') << setw(16) << sdata.fBoardId;
        str2 << sdata.fFirmwareId;

        fFtmBoardId->setText(str1.str().c_str());
        fFtmFirmwareId->setText(str2.str().c_str());
    }

    void handleFtmFtuList(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimFtuList)))
            return;

        fFtuPing->setChecked(false);

        const FTM::DimFtuList &sdata = d.ref<FTM::DimFtuList>();

        stringstream str;
        str << "<table width='100%'>" << setfill('0');
        str << "<tr><th>Num</th><th></th><th>Addr</th><th></th><th>DNA</th></tr>";
        for (int i=0; i<40; i++)
        {
            str << "<tr>";
            str << "<td align='center'>"   << dec << i << hex << "</td>";
            str << "<td align='center'>:</td>";
            str << "<td align='center'>0x" << setw(2)  << (int)sdata.fAddr[i] << "</td>";
            str << "<td align='center'>:</td>";
            str << "<td align='center'>0x" << setw(16) << sdata.fDNA[i] << "</td>";
            str << "</tr>";
        }
        str << "</table>";

        fFtuDNA->setText(str.str().c_str());

        fFtuAnswersTotal->setValue(sdata.fNumBoards);
        fFtuAnswersCrate0->setValue(sdata.fNumBoardsCrate[0]);
        fFtuAnswersCrate1->setValue(sdata.fNumBoardsCrate[1]);
        fFtuAnswersCrate2->setValue(sdata.fNumBoardsCrate[2]);
        fFtuAnswersCrate3->setValue(sdata.fNumBoardsCrate[3]);

        for (int i=0; i<40; i++)
            SetFtuLed(i, sdata.IsActive(i) ? sdata.fPing[i] : -1, d.time);

        SetFtuStatusLed(d.time);
    }

    void handleFtmError(const DimData &d)
    {
        if (!CheckSize(d, sizeof(FTM::DimError)))
            return;

        const FTM::DimError &sdata = d.ref<FTM::DimError>();

        SetFtuLed(sdata.fError.fDestAddress, sdata.fError.fNumCalls, d.time);
        SetFtuStatusLed(d.time);

        // FIXME: Write to special window!
        //Out() << "Error:" << endl;
        //Out() << sdata.fError << endl;
    }

    // ========================== FSC =======================================

    void SetFscValue(QDoubleSpinBox *box, const DimData &d, int idx, bool enable)
    {
        //box->setEnabled(enable);
        if (!enable)
        {
            box->setToolTip(d.time.GetAsStr().c_str());
            return;
        }

        ostringstream str;
        str << d.time << "  --  " << d.get<float>() << "s";

        box->setToolTip(str.str().c_str());
        box->setValue(d.get<float>(idx*4+4));
    }


    void handleFscTemp(const DimData &d)
    {
        const bool enable = d.size()>0 && CheckSize(d, 60*sizeof(float));
        if (!enable)
            return;

        QDoubleSpinBox *boxes[] = {
            fTempCam00, fTempCam01,
            fTempCam10, fTempCam11, fTempCam12, fTempCam13, fTempCam14, 
            fTempCam20, fTempCam21, fTempCam22, fTempCam23, fTempCam24, fTempCam25,
            fTempCam30, fTempCam31, fTempCam32, fTempCam33, fTempCam34, 
            fTempCam40, fTempCam41, fTempCam42, fTempCam43, fTempCam44, fTempCam45,
            fTempCam50, fTempCam51, fTempCam52, fTempCam53, fTempCam54,
            fTempCam60, fTempCam61,
            // 0:b/f 1:b/f 2:b/f 3:b/f
            fTempCrate0back, fTempCrate0front,
            fTempCrate1back, fTempCrate1front,
            fTempCrate2back, fTempCrate2front,
            fTempCrate3back, fTempCrate3front,
            // 0:b/f 1:b/f 2:b/f 3:b/f
            fTempPS0back, fTempPS0front,
            fTempPS1back, fTempPS1front,
            fTempPS2back, fTempPS2front,
            fTempPS3back, fTempPS3front,
            // AUX PS:  FTM t/b; FSC t/b
            fTempAuxFTMtop, fTempAuxFTMbottom,
            fTempAuxFSCtop, fTempAuxFSCbottom,
            // Backpanel:  FTM t/b; FSC t/b
            fTempBackpanelFTMtop, fTempBackpanelFTMbottom,
            fTempBackpanelFSCtop, fTempBackpanelFSCbottom,
            // top front/back; bottom front/back
            fTempSwitchboxTopFront,    fTempSwitchboxTopBack,
            fTempSwitchboxBottomFront, fTempSwitchboxBottomBack,
        };

        for (int i=0; i<59; i++)
            SetFscValue(boxes[i], d, i, enable);

        if (!enable)
            return;

        const float *ptr = d.ptr<float>();

        double avg = 0;
        int    num = 0;
        for (int i=1; i<32; i++)
            if (ptr[i]!=0)
            {
                avg += ptr[i];
                num ++;
            }

        fTempCamAvg->setValue(num?avg/num:0);
    }

    void handleFscVolt(const DimData &d)
    {
        const bool enable = d.size()>0 && CheckSize(d, 31*sizeof(float));
        if (!enable)
            return;

        QDoubleSpinBox *boxes[] = {
            fVoltFad00, fVoltFad10, fVoltFad20, fVoltFad30,
            fVoltFad01, fVoltFad11, fVoltFad21, fVoltFad31,
            fVoltFad02, fVoltFad12, fVoltFad22, fVoltFad32,
            fVoltFPA00, fVoltFPA10, fVoltFPA20, fVoltFPA30,
            fVoltFPA01, fVoltFPA11, fVoltFPA21, fVoltFPA31,
            fVoltFPA02, fVoltFPA12, fVoltFPA22, fVoltFPA32,
            fVoltETH0,  fVoltETH1,
            fVoltFTM0,  fVoltFTM1,
            fVoltFFC,   fVoltFLP,
        };

        for (int i=0; i<30; i++)
            SetFscValue(boxes[i], d, i, enable);
    }

    void handleFscCurrent(const DimData &d)
    {
        const bool enable = d.size()>0 && CheckSize(d, 31*sizeof(float));
        if (!enable)
            return;

        QDoubleSpinBox *boxes[] = {
            fAmpFad00, fAmpFad10, fAmpFad20, fAmpFad30,
            fAmpFad01, fAmpFad11, fAmpFad21, fAmpFad31,
            fAmpFad02, fAmpFad12, fAmpFad22, fAmpFad32,
            fAmpFPA00, fAmpFPA10, fAmpFPA20, fAmpFPA30,
            fAmpFPA01, fAmpFPA11, fAmpFPA21, fAmpFPA31,
            fAmpFPA02, fAmpFPA12, fAmpFPA22, fAmpFPA32,
            fAmpETH0,  fAmpETH1,
            fAmpFTM0,  fAmpFTM1,
            fAmpFFC,   fAmpFLP,
        };

        for (int i=0; i<30; i++)
            SetFscValue(boxes[i], d, i, enable);
    }

    void handleFscHumidity(const DimData &d)
    {
        const bool enable = d.size()>0 && CheckSize(d, 5*sizeof(float));

        SetFscValue(fHumidity1, d, 0, enable);
        SetFscValue(fHumidity2, d, 1, enable);
        SetFscValue(fHumidity3, d, 2, enable);
        SetFscValue(fHumidity4, d, 3, enable);
    }

    // ========================== Feedback ==================================

#ifdef HAVE_ROOT
    TGraphErrors fGraphFeedbackDev;
    TGraphErrors fGraphFeedbackCmd;

    void UpdateFeedback(TQtWidget &rwidget, const Time &time, TGraphErrors &graph, double avg, double rms)
    {
        TCanvas *c = rwidget.GetCanvas();

        TH1 *h = (TH1*)c->FindObject("TimeFrame");

        while (graph.GetN()>500)
            graph.RemovePoint(0);

        const double now = time.RootTime();

        while (graph.GetN()>0 && now-graph.GetX()[0]>3600)
            graph.RemovePoint(0);

        const int n = graph.GetN();

        const double xmin = n>0 ? graph.GetX()[0] : now;

        h->SetBins(n+1, xmin-1, now+1);
        graph.SetPoint(n, now, avg);
        graph.SetPointError(n, 0, rms);

        h->GetXaxis()->SetTimeFormat(now-xmin>300 ? "%Hh%M'%F1995-01-01 00:00:00 GMT" : "%M'%S\"%F1995-01-01 00:00:00 GMT");

        c->Modified();
        c->Update();
    }
#endif

    vector <float> fVecFeedbackCurrents;

    void handleFeedbackCalibratedCurrents(const DimData &d)
    {
        if (!CheckSize(d, (416+1+1+1+1+1+416+1+1)*sizeof(float)+sizeof(uint32_t)))
            return;

        const float *ptr = d.ptr<float>();
        const float *Uov = ptr+416+6;

        fVecFeedbackCurrents.assign(ptr, ptr+416);

        valarray<double> datc(0., 1440);
        valarray<double> datu(0., 1440);

        // fPatch converts from software id to software patch id
        for (int i=0; i<1440; i++)
        {
            const PixelMapEntry &entry = fPixelMap.index(i);

            datc[i] = fVecFeedbackCurrents[entry.hv()];
            datu[i] = Uov[entry.hv()];

            if (fVecBiasCurrent.size()>0)
            {
                fBiasCamA->SetEnable(i, uint16_t(fVecBiasCurrent[entry.hv()])!=0x8000);
                fBiasCamA->highlightPixel(i, fVecBiasCurrent[entry.hv()]<0);
            }
        }

        fBiasCamA->SetData(datc);
        fBiasCamA->updateCamera();

        UpdateBiasValues();

        // --------------------------------------------------------

        double avg = 0;
        double rms = 0;

        for (int i=0; i<320; i++)
        {
            avg += Uov[i];
            rms += Uov[i]*Uov[i];
        }

        avg /= 320;
        rms /= 320;
        rms = sqrt(rms-avg*avg);


        fFeedbackDevCam->SetData(datu);
        //fFeedbackCmdCam->SetData(cmd);

        fFeedbackDevCam->updateCamera();
        //fFeedbackCmdCam->updateCamera();

#ifdef HAVE_ROOT
        UpdateFeedback(*fFeedbackDev, d.time, fGraphFeedbackDev, avg, rms);
        //UpdateFeedback(*fFeedbackCmd, d.time, fGraphFeedbackCmd, avgcmd, rmscmd);
#endif
    }

    // ======================= Rate Scan ====================================
#ifdef HAVE_ROOT
    TGraph fGraphRateScan[201];
#endif
    void UpdateRateScan(uint32_t th, const float *rates)
    {
#ifdef HAVE_ROOT
        TCanvas *c = fRateScanCanv->GetCanvas();

        TH1 *h = (TH1*)c->FindObject("Frame");

        if (fGraphRateScan[0].GetN()==0 || th<fGraphRateScan[0].GetX()[fGraphRateScan[0].GetN()-1])
        {
            h->SetBins(1, th<10 ? 0 : th-10, th+10);
            h->SetMinimum(1);
            h->SetMaximum(rates[0]*2);

            for (int i=0; i<201; i++)
            {
                fGraphRateScan[i].Set(0);
                fGraphRateScan[i].SetPoint(fGraphRateScan[i].GetN(), th, rates[i]);
            }

            c->SetGrid();
            c->SetLogy();

            c->Modified();
            c->Update();
            return;
        }

        const double dac = h->GetXaxis()->GetXmin();
        h->SetBins(h->GetNbinsX()+1, dac, th+10);

        for (int i=0; i<201; i++)
            fGraphRateScan[i].SetPoint(fGraphRateScan[i].GetN(), th, rates[i]);

        c->Modified();
        c->Update();
#endif
    }

    void DisplayRateScan()
    {
#ifdef HAVE_ROOT
        TCanvas *c = fRateScanCanv->GetCanvas();

        TList *l = c->GetListOfPrimitives();

        while (c->FindObject("PatchRate"))
            l->Remove(c->FindObject("PatchRate"));

        while (c->FindObject("BoardRate"))
            l->Remove(c->FindObject("BoardRate"));

        if (fRateScanPatch1->value()>=0)
        {
            fGraphRateScan[fRateScanPatch1->value()+41].SetLineColor(kRed);
            fGraphRateScan[fRateScanPatch1->value()+41].SetMarkerColor(kRed);
            l->Add(&fGraphRateScan[fRateScanPatch1->value()+41], "PL");
        }
        if (fRateScanPatch2->value()>=0)
        {
            fGraphRateScan[fRateScanPatch2->value()+41].SetLineColor(kGreen);
            fGraphRateScan[fRateScanPatch2->value()+41].SetMarkerColor(kGreen);
            l->Add(&fGraphRateScan[fRateScanPatch2->value()+41], "PL");
        }
        if (fRateScanBoard1->value()>=0)
        {
            fGraphRateScan[fRateScanBoard1->value()+1].SetLineColor(kMagenta);
            fGraphRateScan[fRateScanBoard1->value()+1].SetMarkerColor(kMagenta);
            l->Add(&fGraphRateScan[fRateScanBoard1->value()+1], "PL");
        }
        if (fRateScanBoard2->value()>=0)
        {
            fGraphRateScan[fRateScanBoard2->value()+1].SetLineColor(kCyan);
            fGraphRateScan[fRateScanBoard2->value()+1].SetMarkerColor(kCyan);
            l->Add(&fGraphRateScan[fRateScanBoard2->value()+1], "PL");
        }

        c->Modified();
        c->Update();
#endif
    }

    void handleRateScan(const DimData &d)
    {
        if (!CheckSize(d, 206*sizeof(float)))
            return;

        UpdateRateScan(d.get<uint32_t>(8), d.ptr<float>(20));
    }

    // ===================== MAGIC Weather ==================================

    void handleMagicWeather(const DimData &d)
    {
        if (!CheckSize(d, 7*sizeof(float)+sizeof(uint16_t)))
            return;

        const float *ptr = d.ptr<float>(2);

        fMagicTemp->setValue(ptr[0]);
        fMagicDew->setValue(ptr[1]);
        fMagicHum->setValue(ptr[2]);
        fMagicPressure->setValue(ptr[3]);
        fMagicWind->setValue(ptr[4]);
        fMagicGusts->setValue(ptr[5]);

        static const char *dir[] =
        {
            "N", "NNE", "NE", "ENE",
            "E", "ESE", "SE", "SSE",
            "S", "SSW", "SW", "WSW",
            "W", "WNW", "NW", "NNW"
        };

        const uint16_t i = uint16_t(floor(fmod(ptr[6]+11.25, 360)/22.5));
        fMagicWindDir->setText(dir[i%16]);
    }

    // ========================== FSC =======================================

    vector<float>   fVecBiasVolt;
    vector<int16_t> fVecBiasDac;
    vector<int16_t> fVecBiasCurrent;

    void handleBiasVolt(const DimData &d)
    {
        if (!CheckSize(d, 416*sizeof(float)))
            return;

        const float *ptr = d.ptr<float>();
        fVecBiasVolt.assign(ptr, ptr+416);

        on_fBiasDispRefVolt_stateChanged();
    }

    void handleBiasDac(const DimData &d)
    {
        if (!CheckSize(d, 2*416*sizeof(int16_t)))
            return;

        const int16_t *ptr = d.ptr<int16_t>();
        fVecBiasDac.assign(ptr, ptr+2*416);

        on_fBiasDispRefVolt_stateChanged();
        UpdateBiasValues();
    }

    int fStateFeedback;

    void handleBiasCurrent(const DimData &d)
    {
        if (!CheckSize(d, 416*sizeof(int16_t)))
            return;

        const int16_t *ptr = d.ptr<int16_t>();

        fVecBiasCurrent.assign(ptr, ptr+416);

        valarray<double> dat(0., 1440);

        // fPatch converts from software id to software patch id
        for (int i=0; i<1440; i++)
        {
            const PixelMapEntry &entry = fPixelMap.index(i);

            dat[i] = abs(ptr[entry.hv()]) * 5000./4096;

            fBiasCamA->SetEnable(i, uint16_t(ptr[entry.hv()])!=0x8000);
            fBiasCamA->highlightPixel(i, ptr[entry.hv()]<0);
        }

        if (fStateFeedback<Feedback::State::kCalibrated)
            fBiasCamA->SetData(dat);

        fBiasCamA->updateCamera();

        UpdateBiasValues();
    }

    // ====================== MessageImp ====================================

    bool fChatOnline;

    void handleStateChanged(const Time &time, const string &server,
                            const State &s)
    {
        // FIXME: Prefix tooltip with time
        if (server=="MCP")
        {
            // FIXME: Enable FTU page!!!
            fStatusMCPLabel->setText(s.name.c_str());
            fStatusMCPLabel->setToolTip(s.comment.c_str());

            if (s.index<MCP::State::kDisconnected) // No Dim connection
                SetLedColor(fStatusMCPLed, kLedGray, time);
            if (s.index==MCP::State::kDisconnected) // Disconnected
                SetLedColor(fStatusMCPLed, kLedRed, time);
            if (s.index==MCP::State::kConnecting) // Connecting
                SetLedColor(fStatusMCPLed, kLedOrange, time);
            if (s.index==MCP::State::kConnected) // Connected
                SetLedColor(fStatusMCPLed, kLedYellow, time);
            if (s.index==MCP::State::kIdle || s.index>=MCP::State::kConfigured) // Idle, TriggerOn, TakingData
                SetLedColor(fStatusMCPLed, kLedGreen, time);

            if (s.index>MCP::State::kIdle && s.index<MCP::State::kConfigured)
                SetLedColor(fStatusMCPLed, kLedGreenBar, time);

            fMcpStartRun->setEnabled(s.index>=MCP::State::kIdle);
            fMcpStopRun->setEnabled(s.index>=MCP::State::kIdle);
            fMcpReset->setEnabled(s.index>=MCP::State::kIdle && MCP::State::kConfigured);
        }

        if (server=="FTM_CONTROL")
        {
            // FIXME: Enable FTU page!!!
            fStatusFTMLabel->setText(s.name.c_str());
            fStatusFTMLabel->setToolTip(s.comment.c_str());

            bool enable = false;
            const bool configuring =
                s.index==FTM::State::kConfiguring1 ||
                s.index==FTM::State::kConfiguring2 ||
                s.index==FTM::State::kConfigured1  ||
                s.index==FTM::State::kConfigured2;

            if (s.index<FTM::State::kDisconnected) // No Dim connection
                SetLedColor(fStatusFTMLed, kLedGray, time);
            if (s.index==FTM::State::kDisconnected) // Dim connection / FTM disconnected
                SetLedColor(fStatusFTMLed, kLedYellow, time);
            if (s.index==FTM::State::kConnected    ||
                s.index==FTM::State::kIdle         ||
                s.index==FTM::State::kValid        ||
                configuring) // Dim connection / FTM connected
                SetLedColor(fStatusFTMLed, kLedGreen, time);
            if (s.index==FTM::State::kTriggerOn) // Dim connection / FTM connected
                SetLedColor(fStatusFTMLed, kLedGreenCheck, time);
            if (s.index==FTM::State::kConnected ||
                s.index==FTM::State::kIdle ||
                s.index==FTM::State::kValid) // Dim connection / FTM connected
                enable = true;
            if (s.index>=FTM::State::kConfigError1) // Dim connection / FTM connected
                SetLedColor(fStatusFTMLed, kLedGreenWarn, time);

            fFtmStartRun->setEnabled(!configuring && enable && s.index!=FTM::State::kTriggerOn);
            fFtmStopRun->setEnabled(!configuring && !enable);

            fTriggerWidget->setEnabled(enable);
            fFtuGroupEnable->setEnabled(enable);
            fRatesControls->setEnabled(enable);
            fFtuWidget->setEnabled(s.index>FTM::State::kDisconnected);

            if (s.index>=FTM::State::kConnected)
                SetFtuStatusLed(time);
            else
            {
                SetLedColor(fStatusFTULed, kLedGray, time);
                fStatusFTULabel->setText("Offline");
                fStatusFTULabel->setToolTip("FTM is not online.");
            }
        }

        if (server=="FAD_CONTROL")
        {
            fStatusFADLabel->setText(s.name.c_str());
            fStatusFADLabel->setToolTip(s.comment.c_str());

            bool enable = false;

            if (s.index<FAD::State::kOffline) // No Dim connection
            {
                SetLedColor(fStatusFADLed, kLedGray, time);

                // Timing problem - sometimes they stay gray :(
                //for (int i=0; i<40; i++)
                //    SetLedColor(fFadLED[i], kLedGray, time);

                /*
                 fStatusEventBuilderLabel->setText("Offline");
                 fStatusEventBuilderLabel->setToolTip("No connection to fadctrl.");
                 fEvtBldWidget->setEnabled(false);

                 SetLedColor(fStatusEventBuilderLed, kLedGray, time);
                 */
            }
            if (s.index==FAD::State::kOffline) // Dim connection / FTM disconnected
                SetLedColor(fStatusFADLed, kLedRed, time);
            if (s.index==FAD::State::kDisconnected) // Dim connection / FTM disconnected
                SetLedColor(fStatusFADLed, kLedOrange, time);
	    if (s.index==FAD::State::kConnecting) // Dim connection / FTM disconnected
            {
		SetLedColor(fStatusFADLed, kLedYellow, time);
		// FIXME FIXME FIXME: The LEDs are not displayed when disabled!
                enable = true;
            }
            if (s.index>=FAD::State::kConnected) // Dim connection / FTM connected
            {
                SetLedColor(fStatusFADLed, kLedGreen, time);
                enable = true;
            }

            fFadWidget->setEnabled(enable);

            fFadStart->setEnabled    (s.index==FAD::State::kOffline);
            fFadStop->setEnabled     (s.index >FAD::State::kOffline);
            fFadAbort->setEnabled    (s.index >FAD::State::kOffline);
            fFadSoftReset->setEnabled(s.index >FAD::State::kOffline);
            fFadHardReset->setEnabled(s.index >FAD::State::kOffline);
        }

	if (server=="FSC_CONTROL")
        {
            fStatusFSCLabel->setText(s.name.c_str());
            fStatusFSCLabel->setToolTip(s.comment.c_str());

            bool enable = false;

            if (s.index<FSC::State::kDisconnected)  // No Dim connection
                SetLedColor(fStatusFSCLed, kLedGray, time);
            if (s.index==FSC::State::kDisconnected) // Dim connection / FTM disconnected
                SetLedColor(fStatusFSCLed, kLedRed, time);
            if (s.index>=FSC::State::kConnected)    // Dim connection / FTM disconnected
            {
                SetLedColor(fStatusFSCLed, kLedGreen, time);
                enable = true;
            }

            fAuxWidget->setEnabled(enable);
        }

        if (server=="DRIVE_CONTROL")
        {
            fStatusDriveLabel->setText(s.name.c_str());
            fStatusDriveLabel->setToolTip(s.comment.c_str());

            if (s.index<Drive::State::kDisconnected) // No Dim connection
                SetLedColor(fStatusDriveLed, kLedGray, time);
            if (s.index==Drive::State::kDisconnected) // Dim connection / No connection to cosy
                SetLedColor(fStatusDriveLed, kLedRed, time);
            if (s.index==Drive::State::kConnected || /*s.index==Drive::State::kNotReady ||*/ s.index==Drive::State::kLocked)  // Not Ready
                SetLedColor(fStatusDriveLed, kLedGreenBar, time);
            if (s.index==Drive::State::kConnected || s.index==Drive::State::kArmed)  // Connected / Armed
                SetLedColor(fStatusDriveLed, kLedGreen, time);
            if (s.index==Drive::State::kMoving)  // Moving
                SetLedColor(fStatusDriveLed, kLedInProgress, time);
            if (s.index==Drive::State::kTracking || s.index==Drive::State::kOnTrack)  // Tracking
                SetLedColor(fStatusDriveLed, kLedGreenCheck, time);
            if (s.index>=0xff)  // Error
                SetLedColor(fStatusDriveLed, kLedGreenWarn, time);
        }

        if (server=="BIAS_CONTROL")
        {
            fStatusBiasLabel->setText(s.name.c_str());
            fStatusBiasLabel->setToolTip(s.comment.c_str());

            if (s.index<1) // No Dim connection
                SetLedColor(fStatusBiasLed, kLedGray, time);
            if (s.index==BIAS::State::kDisconnected) // Dim connection / FTM disconnected
                SetLedColor(fStatusBiasLed, kLedRed, time);
            if (s.index==BIAS::State::kConnecting || s.index==BIAS::State::kInitializing) // Connecting / Initializing
                SetLedColor(fStatusBiasLed, kLedOrange, time);
            if (s.index==BIAS::State::kVoltageOff || BIAS::State::kLocked) // At reference
                SetLedColor(fStatusBiasLed, kLedGreenBar, time);
            if (s.index==BIAS::State::kNotReferenced) // At reference
                SetLedColor(fStatusBiasLed, kLedGreenWarn, time);
            if (s.index==BIAS::State::kRamping) // Ramping
                SetLedColor(fStatusBiasLed, kLedInProgress, time);
            if (s.index==BIAS::State::kVoltageOn) // At reference
                SetLedColor(fStatusBiasLed, kLedGreenCheck, time);
            if (s.index==BIAS::State::kOverCurrent) // Over current
                SetLedColor(fStatusBiasLed, kLedWarnBorder, time);
            if (s.index==BIAS::State::kExpertMode) // ExpertMode
                SetLedColor(fStatusBiasLed, kLedWarnTriangleBorder, time);

            fBiasWidget->setEnabled(s.index>=BIAS::State::kInitializing);
        }

        if (server=="FEEDBACK")
        {
            fStatusFeedbackLabel->setText(s.name.c_str());
            fStatusFeedbackLabel->setToolTip(s.comment.c_str());

            const bool connected = s.index> Feedback::State::kConnecting;
            const bool idle      = s.index==Feedback::State::kCalibrated || s.index==Feedback::State::kWaitingForData;

            if (s.index<=Feedback::State::kConnecting) // NoDim / Disconnected
                SetLedColor(fStatusFeedbackLed, kLedRed, time);
            if (s.index<Feedback::State::kDisconnected) // No Dim connection
                SetLedColor(fStatusFeedbackLed, kLedGray, time);
            if (s.index==Feedback::State::kConnecting) // Connecting
                SetLedColor(fStatusFeedbackLed, kLedOrange, time);
            if (connected) // Connected
                SetLedColor(fStatusFeedbackLed, kLedYellow, time);
            if (idle)
                SetLedColor(fStatusFeedbackLed, kLedGreen, time);
            if (s.index==Feedback::State::kInProgress)
                SetLedColor(fStatusFeedbackLed, kLedGreenCheck, time);
            if (s.index==Feedback::State::kCalibrating)
                SetLedColor(fStatusFeedbackLed, kLedInProgress, time);

            fFeedbackWidget->setEnabled(connected);

            fFeedbackCalibrate->setEnabled(s.index==Feedback::State::kConnected || s.index==Feedback::State::kCalibrated);
            fFeedbackStart->setEnabled(s.index==Feedback::State::kCalibrated);
            fFeedbackStop->setEnabled(s.index>Feedback::State::kCalibrated);
            fFeedbackOvervoltage->setEnabled(connected);

            const bool enable = s.index>=Feedback::State::kCalibrated;

            fFeedbackFrameLeft->setEnabled(enable);
            fFeedbackCanvLeft->setEnabled(enable);

            fStateFeedback = s.index;
        }

        if (server=="RATE_CONTROL")
        {
            fStatusRateControlLabel->setText(s.name.c_str());
            fStatusRateControlLabel->setToolTip(s.comment.c_str());

            if (s.index==RateControl::State::kInProgress)
                SetLedColor(fStatusRateControlLed, kLedGreenCheck, time);
            if (s.index==RateControl::State::kGlobalThresholdSet)
                SetLedColor(fStatusRateControlLed, kLedGreen, time);
            if (s.index==RateControl::State::kSettingGlobalThreshold)
                SetLedColor(fStatusRateControlLed, kLedInProgress, time);
            if (s.index==RateControl::State::kConnected)
                SetLedColor(fStatusRateControlLed, kLedGreenBar, time);
            if (s.index==RateControl::State::kConnecting)
                SetLedColor(fStatusRateControlLed, kLedOrange, time);
            if (s.index<RateControl::State::kConnecting)
                SetLedColor(fStatusRateControlLed, kLedRed, time);
            if (s.index<RateControl::State::kDisconnected)
                SetLedColor(fStatusRateControlLed, kLedGray, time);
        }

        if (server=="DATA_LOGGER")
        {
            fStatusLoggerLabel->setText(s.name.c_str());
            fStatusLoggerLabel->setToolTip(s.comment.c_str());

            bool enable = true;

            if (s.index<30)   // Ready/Waiting
                SetLedColor(fStatusLoggerLed, kLedYellow, time);
            if (s.index==30)   // Ready/Waiting
                SetLedColor(fStatusLoggerLed, kLedGreen, time);
            if (s.index<-1)     // Offline
            {
                SetLedColor(fStatusLoggerLed, kLedGray, time);
                enable = false;
            }
            if (s.index>=0x100) // Error
                SetLedColor(fStatusLoggerLed, kLedRed, time);
            if (s.index==40)   // Logging
                SetLedColor(fStatusLoggerLed, kLedGreen, time);

            fLoggerWidget->setEnabled(enable);
            fLoggerStart->setEnabled(s.index>-1 && s.index<30);
            fLoggerStop->setEnabled(s.index>=30);
        }

        if (server=="MAGIC_WEATHER")
        {
            fStatusWeatherLabel->setText(s.name.c_str());

            if (s.index==MagicWeather::State::kReceiving)
                SetLedColor(fStatusWeatherLed, kLedGreen, time);
            if (s.index<MagicWeather::State::kReceiving)
                SetLedColor(fStatusWeatherLed, kLedRed, time);
            if (s.index<MagicWeather::State::kConnected)
                SetLedColor(fStatusWeatherLed, kLedGray, time);
        }

        if (server=="CHAT")
        {
            fStatusChatLabel->setText(s.name.c_str());

            fChatOnline = s.index==0;

            SetLedColor(fStatusChatLed, fChatOnline ? kLedGreen : kLedGray, time);

            fChatSend->setEnabled(fChatOnline);
            fChatMessage->setEnabled(fChatOnline);
        }

        if (server=="RATESCAN")
            fRateScanControls->setEnabled(s.index>=RateScan::State::kConnected);

        if (server=="SCHEDULER")
        {
            fStatusSchedulerLabel->setText(s.name.c_str());

            SetLedColor(fStatusSchedulerLed, s.index>=0 ? kLedGreen : kLedRed, time);
        }
    }

    void on_fTabWidget_currentChanged(int which)
    {
        if (fTabWidget->tabText(which)=="Chat")
            fTabWidget->setTabIcon(which, QIcon());
    }

    void handleWrite(const Time &time, const string &text, int qos)
    {
        stringstream out;

        if (text.substr(0, 6)=="CHAT: ")
        {
            if (qos==MessageImp::kDebug)
                return;

            out << "<font size='-1' color='navy'>[<B>";
            out << time.GetAsStr("%H:%M:%S");
            out << "</B>]</FONT>  " << text.substr(6);
            fChatText->append(out.str().c_str());

            if (fTabWidget->tabText(fTabWidget->currentIndex())=="Chat")
                return;

            static int num = 0;
            if (num++<2)
                return;

            for (int i=0; i<fTabWidget->count(); i++)
                if (fTabWidget->tabText(i)=="Chat")
                {
                    fTabWidget->setTabIcon(i, QIcon(":/Resources/icons/warning 3.png"));
                    break;
                }

            return;
        }


        out << "<font style='font-family:monospace' color='";

        switch (qos)
        {
        case kMessage: out << "black";   break;
        case kInfo:    out << "green";   break;
        case kWarn:    out << "#FF6600"; break;
        case kError:   out << "maroon";  break;
        case kFatal:   out << "maroon";  break;
        case kDebug:   out << "navy";    break;
        default:       out << "navy";    break;
        }
        out << "'>";
        out << time.GetAsStr("%H:%M:%S.%f").substr(0,12);
        out << " - " << text << "</font>";

        fLogText->append(out.str().c_str());

        if (qos>=kWarn && qos!=kDebug && qos!=kComment)
            fTextEdit->append(out.str().c_str());
    }

    void IndicateStateChange(const Time &time, const string &server)
    {
        const State s = GetState(server, GetCurrentState(server));

        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleStateChanged, this, time, server, s)));
    }

    int Write(const Time &time, const string &txt, int qos)
    {
        QApplication::postEvent(this,
           new FunctionEvent(bind(&FactGui::handleWrite, this, time, txt, qos)));

        return 0;
    }

    // ====================== Dim infoHandler================================

    void handleDimService(const string &txt)
    {
        fDimSvcText->append(txt.c_str());
    }

    void infoHandlerService(DimInfo &info)
    {
        const string fmt = string(info.getFormat()).empty() ? "C" : info.getFormat();

        stringstream dummy;
        const Converter conv(dummy, fmt, false);

        const Time tm(info.getTimestamp(), info.getTimestampMillisecs()*1000);

        stringstream out;
        out << "<font size'-1' color='navy'>[";
        out << tm.GetAsStr("%H:%M:%S.%f").substr(0,12);
        out << "]</font>   <B>" << info.getName() << "</B> - ";

        uint8_t iserr = 2;
        if (!conv)
        {
            out << "Compilation of format string '" << fmt << "' failed!";
        }
        else
        {
            try
            {
                const string dat = info.getSize()==0 ? "&lt;empty&gt;" : conv.GetString(info.getData(), info.getSize());
                out << dat;
                iserr = info.getSize()==0;
            }
            catch (const runtime_error &e)
            {
                out << "Conversion to string failed!<pre>" << e.what() << "</pre>";
            }
        }

        // srand(hash<string>()(string(info.getName())));
        // int bg = rand()&0xffffff;

        int bg = hash<string>()(string(info.getName()));

        // allow only light colors
        bg = ~(bg&0x1f1f1f)&0xffffff;

        if (iserr==2)
            bg = 0xffffff;

        stringstream bgcol;
        bgcol << hex << setfill('0') << setw(6) << bg;

        const string col = iserr==0 ? "black" : (iserr==1 ? "#FF6600" : "black");
        const string str = "<table width='100%' bgcolor=#"+bgcol.str()+"><tr><td><font color='"+col+"'>"+out.str()+"</font></td></tr></table>";

        QApplication::postEvent(this,
                                new FunctionEvent(bind(&FactGui::handleDimService, this, str)));
    }

    void CallInfoHandler(void (FactGui::*handler)(const DimData&), const DimData &d)
    {
        fInHandler = true;
        (this->*handler)(d);
        fInHandler = false;
    }

    /*
    void CallInfoHandler(const boost::function<void()> &func)
    {
        // This ensures that newly received values are not sent back to the emitter
        // because changing the value emits the valueChanged signal (or similar)
        fInHandler = true;
        func();
        fInHandler = false;
    }*/

    void PostInfoHandler(void (FactGui::*handler)(const DimData&))
    {
        //const boost::function<void()> f = boost::bind(handler, this, DimData(getInfo()));

        FunctionEvent *evt = new FunctionEvent(bind(&FactGui::CallInfoHandler, this, handler, DimData(getInfo())));
        // FunctionEvent *evt = new FunctionEvent(boost::bind(&FactGui::CallInfoHandler, this, f));
        // FunctionEvent *evt = new FunctionEvent(boost::bind(handler, this, DimData(getInfo()))));

        QApplication::postEvent(this, evt);
    }

    void infoHandler()
    {
        // Initialize the time-stamp (what a weird workaround...)
        if (getInfo())
            getInfo()->getTimestamp();

        if (getInfo()==&fDimDNS)
            return PostInfoHandler(&FactGui::handleDimDNS);
#ifdef DEBUG_DIM
        cout << "HandleDimInfo " << getInfo()->getName() << endl;
#endif
        if (getInfo()==&fDimLoggerStats)
            return PostInfoHandler(&FactGui::handleLoggerStats);

//        if (getInfo()==&fDimFadFiles)
//            return PostInfoHandler(&FactGui::handleFadFiles);

        if (getInfo()==&fDimFadWriteStats)
            return PostInfoHandler(&FactGui::handleFadWriteStats);

        if (getInfo()==&fDimFadConnections)
            return PostInfoHandler(&FactGui::handleFadConnections);

        if (getInfo()==&fDimFadFwVersion)
            return PostInfoHandler(&FactGui::handleFadFwVersion);

        if (getInfo()==&fDimFadRunNumber)
            return PostInfoHandler(&FactGui::handleFadRunNumber);

        if (getInfo()==&fDimFadDNA)
            return PostInfoHandler(&FactGui::handleFadDNA);

        if (getInfo()==&fDimFadTemperature)
            return PostInfoHandler(&FactGui::handleFadTemperature);

        if (getInfo()==&fDimFadRefClock)
            return PostInfoHandler(&FactGui::handleFadRefClock);

	if (getInfo()==&fDimFadRoi)
            return PostInfoHandler(&FactGui::handleFadRoi);

        if (getInfo()==&fDimFadDac)
            return PostInfoHandler(&FactGui::handleFadDac);

        if (getInfo()==&fDimFadDrsCalibration)
            return PostInfoHandler(&FactGui::handleFadDrsCalibration);

        if (getInfo()==&fDimFadPrescaler)
            return PostInfoHandler(&FactGui::handleFadPrescaler);

        if (getInfo()==&fDimFadStatus)
            return PostInfoHandler(&FactGui::handleFadStatus);

        if (getInfo()==&fDimFadStatistics1)
            return PostInfoHandler(&FactGui::handleFadStatistics1);

        //if (getInfo()==&fDimFadStatistics2)
        //    return PostInfoHandler(&FactGui::handleFadStatistics2);

        if (getInfo()==&fDimFadFileFormat)
            return PostInfoHandler(&FactGui::handleFadFileFormat);

        if (getInfo()==&fDimFadEvents)
            return PostInfoHandler(&FactGui::handleFadEvents);

        if (getInfo()==&fDimFadRuns)
            return PostInfoHandler(&FactGui::handleFadRuns);

        if (getInfo()==&fDimFadStartRun)
            return PostInfoHandler(&FactGui::handleFadStartRun);

	if (getInfo()==&fDimFadRawData)
            return PostInfoHandler(&FactGui::handleFadRawData);

        if (getInfo()==&fDimFadEventData)
            return PostInfoHandler(&FactGui::handleFadEventData);

/*
        if (getInfo()==&fDimFadSetup)
            return PostInfoHandler(&FactGui::handleFadSetup);
*/
        if (getInfo()==&fDimLoggerFilenameNight)
            return PostInfoHandler(&FactGui::handleLoggerFilenameNight);

        if (getInfo()==&fDimLoggerNumSubs)
            return PostInfoHandler(&FactGui::handleLoggerNumSubs);

        if (getInfo()==&fDimLoggerFilenameRun)
            return PostInfoHandler(&FactGui::handleLoggerFilenameRun);

        if (getInfo()==&fDimFtmTriggerRates)
            return PostInfoHandler(&FactGui::handleFtmTriggerRates);

        if (getInfo()==&fDimFtmCounter)
            return PostInfoHandler(&FactGui::handleFtmCounter);

        if (getInfo()==&fDimFtmDynamicData)
            return PostInfoHandler(&FactGui::handleFtmDynamicData);

        if (getInfo()==&fDimFtmPassport)
            return PostInfoHandler(&FactGui::handleFtmPassport);

        if (getInfo()==&fDimFtmFtuList)
            return PostInfoHandler(&FactGui::handleFtmFtuList);

        if (getInfo()==&fDimFtmStaticData)
            return PostInfoHandler(&FactGui::handleFtmStaticData);

        if (getInfo()==&fDimFtmError)
            return PostInfoHandler(&FactGui::handleFtmError);

        if (getInfo()==&fDimFscTemp)
            return PostInfoHandler(&FactGui::handleFscTemp);

        if (getInfo()==&fDimFscVolt)
            return PostInfoHandler(&FactGui::handleFscVolt);

        if (getInfo()==&fDimFscCurrent)
            return PostInfoHandler(&FactGui::handleFscCurrent);

        if (getInfo()==&fDimFscHumidity)
            return PostInfoHandler(&FactGui::handleFscHumidity);

        //if (getInfo()==&fDimBiasNominal)
        //    return PostInfoHandler(&FactGui::handleBiasNominal);

        if (getInfo()==&fDimBiasVolt)
            return PostInfoHandler(&FactGui::handleBiasVolt);

        if (getInfo()==&fDimBiasDac)
            return PostInfoHandler(&FactGui::handleBiasDac);

        if (getInfo()==&fDimBiasCurrent)
            return PostInfoHandler(&FactGui::handleBiasCurrent);

        if (getInfo()==&fDimFeedbackCalibrated)
            return PostInfoHandler(&FactGui::handleFeedbackCalibratedCurrents);

        if (getInfo()==&fDimRateScan)
            return PostInfoHandler(&FactGui::handleRateScan);

        if (getInfo()==&fDimMagicWeather)
            return PostInfoHandler(&FactGui::handleMagicWeather);

//        if (getInfo()==&fDimFadFiles)
//            return PostInfoHandler(&FactGui::handleFadFiles);

        for (map<string,DimInfo*>::iterator i=fServices.begin(); i!=fServices.end(); i++)
            if (i->second==getInfo())
            {
                infoHandlerService(*i->second);
                return;
            }

        //DimNetwork::infoHandler();
    }


    // ======================================================================

    bool event(QEvent *evt)
    {
        if (dynamic_cast<FunctionEvent*>(evt))
            return static_cast<FunctionEvent*>(evt)->Exec();

        if (dynamic_cast<CheckBoxEvent*>(evt))
        {
            const QStandardItem &item = static_cast<CheckBoxEvent*>(evt)->item;
            const QStandardItem *par  = item.parent();
            if (par)
            {
                const QString server  = par->text();
                const QString service = item.text();

                const string s = (server+'/'+service).toStdString();

                if (item.checkState()==Qt::Checked)
                    SubscribeService(s);
                else
                    UnsubscribeService(s);
            }
        }

        return MainWindow::event(evt); // unrecognized
    }

    void on_fDimCmdSend_clicked()
    {
        const QString server    = fDimCmdServers->currentIndex().data().toString();
        const QString command   = fDimCmdCommands->currentIndex().data().toString();
        const QString arguments = fDimCmdLineEdit->displayText();

        // FIXME: Sending a command exactly when the info Handler changes
        //        the list it might lead to confusion.
        try
        {
            SendDimCommand(server.toStdString(), command.toStdString()+" "+arguments.toStdString());
            fTextEdit->append("<font color='green'>Command '"+server+'/'+command+"' successfully emitted.</font>");
            fDimCmdLineEdit->clear();
        }
        catch (const runtime_error &e)
        {
            stringstream txt;
            txt << e.what();

            string buffer;
            while (getline(txt, buffer, '\n'))
                fTextEdit->append(("<font color='red'><pre>"+buffer+"</pre></font>").c_str());
        }
    }

    void slot_RootEventProcessed(TObject *obj, unsigned int evt, TCanvas *canv)
    {
#ifdef HAVE_ROOT
        // kMousePressEvent       // TCanvas processed QEvent mousePressEvent
        // kMouseMoveEvent        // TCanvas processed QEvent mouseMoveEvent
        // kMouseReleaseEvent     // TCanvas processed QEvent mouseReleaseEvent
        // kMouseDoubleClickEvent // TCanvas processed QEvent mouseDoubleClickEvent
        // kKeyPressEvent         // TCanvas processed QEvent keyPressEvent
        // kEnterEvent            // TCanvas processed QEvent enterEvent
        // kLeaveEvent            // TCanvas processed QEvent leaveEvent

        if (dynamic_cast<TCanvas*>(obj))
            return;

        TQtWidget *tipped = static_cast<TQtWidget*>(sender());

        if (evt==11/*kMouseReleaseEvent*/)
            return;

        if (evt==61/*kMouseDoubleClickEvent*/)
            return;

        if (obj)
        {
            // Find the object which will get picked by the GetObjectInfo
            // due to buffer overflows in many root-versions
            // in TH1 and TProfile we have to work around and implement
            // our own GetObjectInfo which make everything a bit more
            // complicated.
            canv->cd();
#if ROOT_VERSION_CODE > ROOT_VERSION(5,22,00)
            const char *objectInfo =
                obj->GetObjectInfo(tipped->GetEventX(),tipped->GetEventY());
#else
            const char *objectInfo = dynamic_cast<TH1*>(obj) ?
                "" : obj->GetObjectInfo(tipped->GetEventX(),tipped->GetEventY());
#endif

            QString tipText;
            tipText += obj->GetName();
            tipText += " [";
            tipText += obj->ClassName();
            tipText += "]: ";
            tipText += objectInfo;

            fStatusBar->showMessage(tipText, 3000);
        }

        gSystem->DispatchOneEvent(kFALSE);
        //gSystem->ProcessEvents();
        //QWhatsThis::display(tipText)
#endif
    }

    void slot_RootUpdate()
    {
#ifdef HAVE_ROOT
        gSystem->DispatchOneEvent(kFALSE);
        //gSystem->ProcessEvents();
        QTimer::singleShot(10, this, SLOT(slot_RootUpdate()));
#endif
    }

    void ChoosePatchThreshold(Camera &cam, int isw)
    {
        cam.Reset();

        fThresholdIdx->setValue(isw);

        const int ihw = isw<0 ? 0 : fPatchMapHW[isw];

        fPatchRate->setEnabled(isw>=0);
        fThresholdCrate->setEnabled(isw>=0);
        fThresholdBoard->setEnabled(isw>=0);
        fThresholdPatch->setEnabled(isw>=0);

        if (isw<0)
            return;

        const int patch = ihw%4;
        const int board = (ihw/4)%10;
        const int crate = (ihw/4)/10;

        fInChoosePatchTH = true;

        fThresholdCrate->setValue(crate);
        fThresholdBoard->setValue(board);
        fThresholdPatch->setValue(patch);

        fInChoosePatchTH = false;

        fThresholdVal->setValue(fFtmStaticData.fThreshold[ihw]);
        fPatchRate->setValue(fTriggerRates.fPatchRate[ihw]);
        fBoardRate->setValue(fTriggerRates.fBoardRate[ihw/4]);

        // Loop over the software idx of all pixels
//        for (unsigned int i=0; i<1440; i++)
//            if (fPatchHW[i]==ihw)
//                cam.SetBold(i);
    }

    void slot_ChoosePixelThreshold(int isw)
    {
        fPixelIdx->setValue(isw);

        const PixelMapEntry &entry = fPixelMap.index(isw);
        fPixelEnable->setChecked(fFtmStaticData.IsEnabled(entry.hw()));
    }

    void slot_CameraDoubleClick(int isw)
    {
        fPixelIdx->setValue(isw);

        const PixelMapEntry &entry = fPixelMap.index(isw);
        Dim::SendCommand("FTM_CONTROL/TOGGLE_PIXEL", uint16_t(entry.hw()));
    }

    void slot_CameraMouseMove(int isw)
    {
        const PixelMapEntry &entry = fPixelMap.index(isw);

        QString tipText;
        tipText += fRatesCanv->GetName();
        ostringstream str;
        str << setfill('0') <<
            "  ||  HW: " << entry.crate() << "|" << entry.board() << "|" << entry.patch() << "|" << entry.pixel() << " (crate|board|patch|pixel)" <<
            "  ||  HV: " << entry.hv_board << "|" << setw(2) << entry.hv_channel << " (board|channel)" <<
            "  ||  ID: " << isw;


        tipText += str.str().c_str();
        fStatusBar->showMessage(tipText, 3000);
    }

    void on_fPixelIdx_valueChanged(int isw)
    {
        int ii = 0;
        for (; ii<160; ii++)
            if (fPixelMap.index(isw).hw()/9==fPatchMapHW[ii])
                break;

        fRatesCanv->SetWhite(isw);
        ChoosePatchThreshold(*fRatesCanv, ii);

        const PixelMapEntry &entry = fPixelMap.index(isw);
        fPixelEnable->setChecked(fFtmStaticData.IsEnabled(entry.hw()));
    }

    // ------------------- Bias display ---------------------

    void UpdateBiasValues()
    {
        const int b = fBiasHvBoard->value();
        const int c = fBiasHvChannel->value();

        const int ihw = b*32+c;

        if (fVecBiasVolt.size()>0)
        {
            fBiasVoltCur->setValue(fVecBiasVolt[ihw]);
            SetLedColor(fBiasNominalLed,
                        fVecBiasDac[ihw]==fVecBiasDac[ihw+416]?kLedGreen:kLedRed, Time());
        }

        if (fVecBiasCurrent.size()>0)
        {
            const double val = abs(fVecBiasCurrent[ihw]) * 5000./4096;
            fBiasCurrent->setValue(val);
            SetLedColor(fBiasOverCurrentLed,
                        fVecBiasCurrent[ihw]<0?kLedRed:kLedGreen, Time());
        }

        const bool calibrated = fStateFeedback>=Feedback::State::kCalibrated &&
            fVecFeedbackCurrents.size()>0;

        fBiasCalibrated->setValue(calibrated ? fVecFeedbackCurrents[ihw] : 0);
        fBiasCalibrated->setEnabled(calibrated);
    }

    void UpdateBiasCam(const PixelMapEntry &entry)
    {
        fInChooseBiasCam = true;

        fBiasCamCrate->setValue(entry.crate());
        fBiasCamBoard->setValue(entry.board());
        fBiasCamPatch->setValue(entry.patch());
        fBiasCamPixel->setValue(entry.pixel());

        fInChooseBiasCam = false;
    }

    void BiasHvChannelChanged()
    {
        if (fInChooseBiasHv)
            return;

        const int b  = fBiasHvBoard->value();
        const int ch = fBiasHvChannel->value();

        // FIXME: Mark corresponding patch in camera
        const PixelMapEntry &entry = fPixelMap.hv(b, ch);
        fBiasCamV->SetWhite(entry.index);
        fBiasCamA->SetWhite(entry.index);
        fBiasCamV->updateCamera();
        fBiasCamA->updateCamera();

        UpdateBiasCam(entry);
        UpdateBiasValues();
    }

    void UpdateBiasHv(const PixelMapEntry &entry)
    {
        fInChooseBiasHv = true;

        fBiasHvBoard->setValue(entry.hv_board);
        fBiasHvChannel->setValue(entry.hv_channel);

        fInChooseBiasHv = false;
    }

    void BiasCamChannelChanged()
    {
        if (fInChooseBiasCam)
            return;

        const int crate = fBiasCamCrate->value();
        const int board = fBiasCamBoard->value();
        const int patch = fBiasCamPatch->value();
        const int pixel = fBiasCamPixel->value();

        // FIXME: Display corresponding patches
        const PixelMapEntry &entry = fPixelMap.cbpx(crate, board, patch, pixel);
        fBiasCamV->SetWhite(entry.index);
        fBiasCamA->SetWhite(entry.index);
        fBiasCamV->updateCamera();
        fBiasCamA->updateCamera();

        UpdateBiasHv(entry);
        UpdateBiasValues();
    }

    void slot_ChooseBiasChannel(int isw)
    {
        const PixelMapEntry &entry = fPixelMap.index(isw);

        UpdateBiasHv(entry);
        UpdateBiasCam(entry);
        UpdateBiasValues();
    }

    void on_fBiasDispRefVolt_stateChanged(int = 0)
    {
        // FIXME: Display patches for which ref==cur

        valarray<double> dat(0., 1440);
        fBiasCamV->setTitle("Applied BIAS voltage");

        if (fVecBiasVolt.size()>0 && fVecBiasDac.size()>0)
        {
            for (int i=0; i<1440; i++)
            {
                const PixelMapEntry &entry = fPixelMap.index(i);

                dat[i] = fVecBiasVolt[entry.hv()];
                fBiasCamV->highlightPixel(i, fVecBiasDac[entry.hv()]!=fVecBiasDac[entry.hv()+416]);
            }

            fBiasCamV->SetData(dat);
        }

        fBiasCamV->updateCamera();
    }

    // ------------------------------------------------------

    void on_fPixelEnable_stateChanged(int b)
    {
        if (fInHandler)
            return;

        const PixelMapEntry &entry = fPixelMap.index(fPixelIdx->value());

        Dim::SendCommand(b==Qt::Unchecked ?
                         "FTM_CONTROL/DISABLE_PIXEL" : "FTM_CONTROL/ENABLE_PIXEL",
                         uint16_t(entry.hw()));
    }

    void on_fPixelDisableOthers_clicked()
    {
        const PixelMapEntry &entry = fPixelMap.index(fPixelIdx->value());
        Dim::SendCommand("FTM_CONTROL/DISABLE_ALL_PIXELS_EXCEPT", uint16_t(entry.hw()));
    }

    void on_fThresholdDisableOthers_clicked()
    {
        const int16_t isw = fThresholdIdx->value();
        const int16_t ihw = isw<0 ? -1 : fPatchMapHW[isw];
        if (ihw<0)
            return;

        Dim::SendCommand("FTM_CONTROL/DISABLE_ALL_PATCHES_EXCEPT", ihw);
    }

    void on_fThresholdEnablePatch_clicked()
    {
        const int16_t isw = fThresholdIdx->value();
        const int16_t ihw = isw<0 ? -1 : fPatchMapHW[isw];
       if (ihw<0)
            return;

        Dim::SendCommand("FTM_CONTROL/ENABLE_PATCH", ihw);
    }

    void on_fThresholdDisablePatch_clicked()
    {
        const int16_t isw = fThresholdIdx->value();
        const int16_t ihw = isw<0 ? -1 : fPatchMapHW[isw];
        if (ihw<0)
            return;

        Dim::SendCommand("FTM_CONTROL/DISABLE_PATCH", ihw);
    }

    void on_fThresholdVal_valueChanged(int v)
    {
        fThresholdVolt->setValue(2500./4095*v);

        const int32_t isw = fThresholdIdx->value();
        const int32_t ihw = isw<0 ? -1 : fPatchMapHW[isw];

        const int32_t d[2] = { ihw, v };

        if (!fInHandler)
            Dim::SendCommand("FTM_CONTROL/SET_THRESHOLD", d);
    }

#ifdef HAVE_ROOT
    TGraph fGraphFtmTemp[4];
    TGraph fGraphFtmRate;
    TGraph fGraphPatchRate[160];
    TGraph fGraphBoardRate[40];

    TH1 *DrawTimeFrame(const char *ytitle)
    {
        const double tm = Time().RootTime();

        TH1F *h=new TH1F("TimeFrame", "", 1, tm, tm+60);//Time().RootTime()-1./24/60/60, Time().RootTime());
        h->SetDirectory(0);
        h->SetBit(kCanDelete);
        h->SetStats(kFALSE);
//        h.SetMinimum(0);
//        h.SetMaximum(1);
        h->SetXTitle("Time");
        h->SetYTitle(ytitle);
        h->GetXaxis()->CenterTitle();
	h->GetYaxis()->CenterTitle();
        h->GetXaxis()->SetTimeDisplay(true);
        h->GetXaxis()->SetTimeFormat("%Mh%S'%F1995-01-01 00:00:00 GMT");
	h->GetXaxis()->SetLabelSize(0.025);
	h->GetYaxis()->SetLabelSize(0.025);
        h->GetYaxis()->SetTitleOffset(1.2);
	//        h.GetYaxis()->SetTitleSize(1.2);
        return h;
    }
#endif

    pair<string,string> Split(const string &str) const
    {
        const size_t p = str.find_first_of('|');
        if (p==string::npos)
            return make_pair(str, "");

        return make_pair(str.substr(0, p), str.substr(p+1));
    }

public:
    FactGui(Configuration &conf) :
        fFtuStatus(40), 
        /*fPixelMapHW(1440),*/ fPatchMapHW(160), 
        fInChoosePatchTH(false),
        fInChooseBiasHv(false), fInChooseBiasCam(false),
        fDimDNS("DIS_DNS/VERSION_NUMBER", 1, int(0), this),
        //-
        fDimLoggerStats        ("DATA_LOGGER/STATS",            (void*)NULL, 0, this),
        fDimLoggerFilenameNight("DATA_LOGGER/FILENAME_NIGHTLY", (void*)NULL, 0, this),
        fDimLoggerFilenameRun  ("DATA_LOGGER/FILENAME_RUN",     (void*)NULL, 0, this),
        fDimLoggerNumSubs      ("DATA_LOGGER/NUM_SUBS",         (void*)NULL, 0, this),
        //-
        fDimFtmPassport        ("FTM_CONTROL/PASSPORT",         (void*)NULL, 0, this),
        fDimFtmTriggerRates    ("FTM_CONTROL/TRIGGER_RATES",    (void*)NULL, 0, this),
        fDimFtmError           ("FTM_CONTROL/ERROR",            (void*)NULL, 0, this),
        fDimFtmFtuList         ("FTM_CONTROL/FTU_LIST",         (void*)NULL, 0, this),
        fDimFtmStaticData      ("FTM_CONTROL/STATIC_DATA",      (void*)NULL, 0, this),
        fDimFtmDynamicData     ("FTM_CONTROL/DYNAMIC_DATA",     (void*)NULL, 0, this),
        fDimFtmCounter         ("FTM_CONTROL/COUNTER",          (void*)NULL, 0, this),
        //-
        fDimFadWriteStats      ("FAD_CONTROL/STATS",              (void*)NULL, 0, this),
        fDimFadStartRun        ("FAD_CONTROL/START_RUN",          (void*)NULL, 0, this),
        fDimFadRuns            ("FAD_CONTROL/RUNS",               (void*)NULL, 0, this),
        fDimFadEvents          ("FAD_CONTROL/EVENTS",             (void*)NULL, 0, this),
        fDimFadRawData         ("FAD_CONTROL/RAW_DATA",           (void*)NULL, 0, this),
        fDimFadEventData       ("FAD_CONTROL/EVENT_DATA",         (void*)NULL, 0, this),
        fDimFadConnections     ("FAD_CONTROL/CONNECTIONS",        (void*)NULL, 0, this),
        fDimFadFwVersion       ("FAD_CONTROL/FIRMWARE_VERSION",   (void*)NULL, 0, this),
        fDimFadRunNumber       ("FAD_CONTROL/RUN_NUMBER",         (void*)NULL, 0, this),
        fDimFadDNA             ("FAD_CONTROL/DNA",                (void*)NULL, 0, this),
        fDimFadTemperature     ("FAD_CONTROL/TEMPERATURE",        (void*)NULL, 0, this),
        fDimFadPrescaler       ("FAD_CONTROL/PRESCALER",          (void*)NULL, 0, this),
        fDimFadRefClock        ("FAD_CONTROL/REFERENCE_CLOCK",    (void*)NULL, 0, this),
        fDimFadRoi             ("FAD_CONTROL/REGION_OF_INTEREST", (void*)NULL, 0, this),
        fDimFadDac             ("FAD_CONTROL/DAC",                (void*)NULL, 0, this),
        fDimFadDrsCalibration  ("FAD_CONTROL/DRS_CALIBRATION",    (void*)NULL, 0, this),
        fDimFadStatus          ("FAD_CONTROL/STATUS",             (void*)NULL, 0, this),
        fDimFadStatistics1     ("FAD_CONTROL/STATISTICS1",        (void*)NULL, 0, this),
        //fDimFadStatistics2     ("FAD_CONTROL/STATISTICS2",        (void*)NULL, 0, this),
        fDimFadFileFormat      ("FAD_CONTROL/FILE_FORMAT",        (void*)NULL, 0, this),
        //-
        fDimFscTemp            ("FSC_CONTROL/TEMPERATURE",        (void*)NULL, 0, this),
        fDimFscVolt            ("FSC_CONTROL/VOLTAGE",            (void*)NULL, 0, this),
        fDimFscCurrent         ("FSC_CONTROL/CURRENT",            (void*)NULL, 0, this),
        fDimFscHumidity        ("FSC_CONTROL/HUMIDITY",           (void*)NULL, 0, this),
        //-
        fDimFeedbackCalibrated ("FEEDBACK/CALIBRATED_CURRENTS",   (void*)NULL, 0, this),
        //-
        fDimBiasNominal        ("BIAS_CONTROL/NOMINAL",           (void*)NULL, 0, this),
        fDimBiasVolt           ("BIAS_CONTROL/VOLTAGE",           (void*)NULL, 0, this),
        fDimBiasDac            ("BIAS_CONTROL/DAC",               (void*)NULL, 0, this),
        fDimBiasCurrent        ("BIAS_CONTROL/CURRENT",           (void*)NULL, 0, this),
        //-
        fDimRateScan           ("RATE_SCAN/DATA",                 (void*)NULL, 0, this),
        //-
        fDimMagicWeather       ("MAGIC_WEATHER/DATA",             (void*)NULL, 0, this),
        //-
        fDimVersion(0),
        fFreeSpaceLogger(UINT64_MAX), fFreeSpaceData(UINT64_MAX),
        fEventData(0),
        fDrsCalibration(1440*1024*6+160*1024*2),
        fTimeStamp0(0)
    {
        fClockCondFreq->addItem("--- Hz",  QVariant(-1));
        fClockCondFreq->addItem("800 MHz", QVariant(800));
        fClockCondFreq->addItem("1 GHz",   QVariant(1000));
        fClockCondFreq->addItem("2 GHz",   QVariant(2000));
        fClockCondFreq->addItem("3 GHz",   QVariant(3000));
        fClockCondFreq->addItem("4 GHz",   QVariant(4000));
        fClockCondFreq->addItem("5 GHz",   QVariant(5000));

        cout << "-- run counter ---" << endl;
        fMcpNumEvents->addItem("unlimited", QVariant(0));
        const vector<uint32_t> runcount = conf.Vec<uint32_t>("run-count");
        for (vector<uint32_t>::const_iterator it=runcount.begin(); it!=runcount.end(); it++)
        {
            cout << *it << endl;
            ostringstream str;
            str << *it;
            fMcpNumEvents->addItem(str.str().c_str(), QVariant(*it));
        }

        cout << "-- run times ---" << endl;
        fMcpTime->addItem("unlimited", QVariant(0));
        const vector<string> runtime = conf.Vec<string>("run-time");
        for (vector<string>::const_iterator it=runtime.begin(); it!=runtime.end(); it++)
        {
            const pair<string,string> p = Split(*it);
            cout << *it << "|" << p.second << "|" << p.first << "|" << endl;
            fMcpTime->addItem(p.second.c_str(), QVariant(stoi(p.first)));
        }

        cout << "-- run types ---" << endl;
        const vector<string> runtype = conf.Vec<string>("run-type");
        for (vector<string>::const_iterator it=runtype.begin(); it!=runtype.end(); it++)
        {
            const pair<string,string> p = Split(*it);
            cout << *it << "|" << p.second << "|" << p.first << "|" << endl;
            fMcpRunType->addItem(p.second.c_str(), QVariant(p.first.c_str()));
        }

        fTriggerWidget->setEnabled(false);
        fFtuWidget->setEnabled(false);
        fFtuGroupEnable->setEnabled(false);
        fRatesControls->setEnabled(false);
        fFadWidget->setEnabled(false);
        fGroupEthernet->setEnabled(false);
        fGroupOutput->setEnabled(false);
        fLoggerWidget->setEnabled(false);
        fBiasWidget->setEnabled(false);
        fAuxWidget->setEnabled(false);

        fChatSend->setEnabled(false);
        fChatMessage->setEnabled(false);

        DimClient::sendCommand("CHAT/MSG", "GUI online.");
        // + MessageDimRX

        // --------------------------------------------------------------------------

        if (!fPixelMap.Read(conf.GetPrefixedString("pixel-map-file")))
        {
            cerr << "ERROR - Problems reading " << conf.Get<string>("pixel-map-file") << endl;
            exit(-1);
        }

        // --------------------------------------------------------------------------

        ifstream fin3("PatchList.txt");

        string buf;

        int l = 0;
        while (getline(fin3, buf, '\n'))
        {
            buf = Tools::Trim(buf);
            if (buf[0]=='#')
                continue;

            unsigned int softid, hardid;

            stringstream str(buf);

            str >> softid;
            str >> hardid;

            if (softid>=fPatchMapHW.size())
                continue;

            fPatchMapHW[softid] = hardid-1;

            l++;
        }

        if (l!=160)
            cerr << "WARNING - Problems reading PatchList.txt" << endl;

        // --------------------------------------------------------------------------

        fCommentsWidget->setEnabled(false);

        static const boost::regex expr("(([[:word:].-]+)(:(.+))?@)?([[:word:].-]+)(:([[:digit:]]+))?(/([[:word:].-]+))");

        const string database = conf.Get<string>("CommentDB");

        if (!database.empty())
        {
            boost::smatch what;
            if (!boost::regex_match(database, what, expr, boost::match_extra))
                throw runtime_error("Couldn't parse '"+database+"'.");

            if (what.size()!=10)
                throw runtime_error("Error parsing '"+database+"'.");

            const string user   = what[2];
            const string passwd = what[4];
            const string server = what[5];
            const string db     = what[9];
            const int port      = atoi(string(what[7]).c_str());

            QSqlDatabase qdb = QSqlDatabase::addDatabase("QMYSQL");
            qdb.setHostName(server.c_str());
            qdb.setDatabaseName(db.c_str());
            qdb.setUserName(user.c_str());
            qdb.setPassword(passwd.c_str());
            qdb.setPort(port);
            qdb.setConnectOptions("CLIENT_SSL=1;MYSQL_OPT_RECONNECT=1");
            if (qdb.open())
            {
                QSqlTableModel *model = new QSqlTableModel(fTableComments, qdb);
                model->setTable("runcomments");
                model->setEditStrategy(QSqlTableModel::OnManualSubmit);

                const bool ok2 = model->select();

                if (ok2)
                {
                    fTableComments->setModel(model);
                    fTableComments->resizeColumnsToContents();
                    fTableComments->resizeRowsToContents();

                    connect(fCommentSubmit, SIGNAL(clicked()), model, SLOT(submitAll()));
                    connect(fCommentRevert, SIGNAL(clicked()), model, SLOT(revertAll()));
                    connect(fCommentUpdateLayout, SIGNAL(clicked()), fTableComments, SLOT(resizeColumnsToContents()));
                    connect(fCommentUpdateLayout, SIGNAL(clicked()), fTableComments, SLOT(resizeRowsToContents()));

                    fCommentsWidget->setEnabled(true);
                }
                else
                    cout << "\n==> ERROR: Select on table failed.\n" << endl;
            }
            else
                cout << "\n==> ERROR: Connection to database failed:\n           "
                    << qdb.lastError().text().toStdString() << endl << endl;
        }

        // --------------------------------------------------------------------------
#ifdef HAVE_ROOT

        fGraphFeedbackDev.SetLineColor(kBlue);
        fGraphFeedbackDev.SetMarkerColor(kBlue);
        fGraphFeedbackDev.SetMarkerStyle(kFullDotMedium);

        fGraphFeedbackCmd.SetLineColor(kBlue);
        fGraphFeedbackCmd.SetMarkerColor(kBlue);
        fGraphFeedbackCmd.SetMarkerStyle(kFullDotMedium);

        // Evolution of control deviation
        // Evolution of command values (bias voltage change)
        fGraphFeedbackDev.SetName("ControlDev");
        fGraphFeedbackCmd.SetName("CommandVal");

        TCanvas *c = fFeedbackDev->GetCanvas();
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.03);
        c->SetTopMargin(0.03);
        c->SetGrid();

	TH1 *hf = DrawTimeFrame("Overvoltage [V]   ");
        hf->GetXaxis()->SetLabelSize(0.07);
        hf->GetYaxis()->SetLabelSize(0.07);
        hf->GetYaxis()->SetTitleSize(0.08);
        hf->GetYaxis()->SetTitleOffset(0.55);
        hf->GetXaxis()->SetTitle("");
        hf->GetYaxis()->SetRangeUser(0, 1.5);

        c->GetListOfPrimitives()->Add(hf, "");
        c->GetListOfPrimitives()->Add(&fGraphFeedbackDev, "LP");

        c = fFeedbackCmd->GetCanvas();
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.03);
        c->SetTopMargin(0.03);
        c->SetGrid();

        hf = DrawTimeFrame("Command temp delta [V]   ");
        hf->GetXaxis()->SetLabelSize(0.07);
        hf->GetYaxis()->SetLabelSize(0.07);
        hf->GetYaxis()->SetTitleSize(0.08);
        hf->GetYaxis()->SetTitleOffset(0.55);
        hf->GetXaxis()->SetTitle("");
        hf->GetYaxis()->SetRangeUser(-2, 2);

        c->GetListOfPrimitives()->Add(hf, "");
        c->GetListOfPrimitives()->Add(&fGraphFeedbackCmd, "LP");

        // --------------------------------------------------------------------------

        c = fRateScanCanv->GetCanvas();
        //c->SetBit(TCanvas::kNoContextMenu);
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.03);
        c->SetTopMargin(0.03);
        c->SetGrid();

        TH1F *h=new TH1F("Frame", "", 1, 0, 1);
        h->SetDirectory(0);
        h->SetBit(kCanDelete);
        h->SetStats(kFALSE);
        h->SetXTitle("Threshold [DAC]");
        h->SetYTitle("Rate [Hz]");
        h->GetXaxis()->CenterTitle();
	h->GetYaxis()->CenterTitle();
	h->GetXaxis()->SetLabelSize(0.025);
	h->GetYaxis()->SetLabelSize(0.025);
        h->GetYaxis()->SetTitleOffset(1.2);
        c->GetListOfPrimitives()->Add(h, "");

        fGraphRateScan[0].SetName("CameraRate");
        for (int i=0; i<40; i++)
        {
            fGraphRateScan[i+1].SetName("BoardRate");
            fGraphRateScan[i+1].SetMarkerStyle(kFullDotMedium);
        }
        for (int i=0; i<160; i++)
        {
            fGraphRateScan[i+41].SetName("PatchRate");
            fGraphRateScan[i+41].SetMarkerStyle(kFullDotMedium);
        }

        fGraphRateScan[0].SetLineColor(kBlue);
        fGraphRateScan[0].SetMarkerColor(kBlue);
        fGraphRateScan[0].SetMarkerStyle(kFullDotSmall);
        c->GetListOfPrimitives()->Add(&fGraphRateScan[0], "LP");

        // --------------------------------------------------------------------------

        c = fFtmRateCanv->GetCanvas();
        //c->SetBit(TCanvas::kNoContextMenu);
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.03);
        c->SetTopMargin(0.03);
        c->SetGrid();

	hf = DrawTimeFrame("Trigger rate [Hz]");
        hf->GetYaxis()->SetRangeUser(0, 1010);

        for (int i=0; i<160; i++)
        {
            fGraphPatchRate[i].SetName("PatchRate");
            //fGraphPatchRate[i].SetLineColor(kBlue);
            //fGraphPatchRate[i].SetMarkerColor(kBlue);
            fGraphPatchRate[i].SetMarkerStyle(kFullDotMedium);
        }
        for (int i=0; i<40; i++)
        {
            fGraphBoardRate[i].SetName("BoardRate");
            //fGraphBoardRate[i].SetLineColor(kBlue);
            //fGraphBoardRate[i].SetMarkerColor(kBlue);
            fGraphBoardRate[i].SetMarkerStyle(kFullDotMedium);
        }

        fGraphFtmRate.SetLineColor(kBlue);
        fGraphFtmRate.SetMarkerColor(kBlue);
        fGraphFtmRate.SetMarkerStyle(kFullDotSmall);

        c->GetListOfPrimitives()->Add(hf, "");
        c->GetListOfPrimitives()->Add(&fGraphFtmRate, "LP");

        /*
        TCanvas *c = fFtmTempCanv->GetCanvas();
        c->SetBit(TCanvas::kNoContextMenu);
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.03);
        c->SetTopMargin(0.03);
        c->cd();
        */
        //CreateTimeFrame("Temperature / �C");

        fGraphFtmTemp[0].SetMarkerStyle(kFullDotSmall);
        fGraphFtmTemp[1].SetMarkerStyle(kFullDotSmall);
        fGraphFtmTemp[2].SetMarkerStyle(kFullDotSmall);
        fGraphFtmTemp[3].SetMarkerStyle(kFullDotSmall);

        fGraphFtmTemp[1].SetLineColor(kBlue);
        fGraphFtmTemp[2].SetLineColor(kRed);
        fGraphFtmTemp[3].SetLineColor(kGreen);

        fGraphFtmTemp[1].SetMarkerColor(kBlue);
        fGraphFtmTemp[2].SetMarkerColor(kRed);
        fGraphFtmTemp[3].SetMarkerColor(kGreen);

        //fGraphFtmTemp[0].Draw("LP");
        //fGraphFtmTemp[1].Draw("LP");
        //fGraphFtmTemp[2].Draw("LP");
        //fGraphFtmTemp[3].Draw("LP");

        // --------------------------------------------------------------------------

        c = fAdcDataCanv->GetCanvas();
        //c->SetBit(TCanvas::kNoContextMenu);
        c->SetBorderMode(0);
        c->SetFrameBorderMode(0);
        c->SetFillColor(kWhite);
        c->SetRightMargin(0.10);
        c->SetGrid();
        //c->cd();
#endif

        // --------------------------------------------------------------------------
        fFeedbackDevCam->assignPixelMap(fPixelMap);
        fFeedbackDevCam->setAutoscaleLowerLimit((fFeedbackDevMin->minimum()+0.5*fFeedbackDevMin->singleStep()));
        fFeedbackDevCam->SetMin(fFeedbackDevMin->value());
        fFeedbackDevCam->SetMax(fFeedbackDevMax->value());
        fFeedbackDevCam->updateCamera();

        fFeedbackCmdCam->assignPixelMap(fPixelMap);
        fFeedbackCmdCam->setAutoscaleLowerLimit((fFeedbackCmdMin->minimum()+0.5*fFeedbackCmdMin->singleStep()));
        fFeedbackCmdCam->SetMin(fFeedbackCmdMin->value());
        fFeedbackCmdCam->SetMax(fFeedbackCmdMax->value());
        fFeedbackCmdCam->updateCamera();

        // --------------------------------------------------------------------------

        fBiasCamV->assignPixelMap(fPixelMap);
        fBiasCamV->setAutoscaleLowerLimit((fBiasVoltMin->minimum()+0.5*fBiasVoltMin->singleStep()));
        fBiasCamV->SetMin(fBiasVoltMin->value());
        fBiasCamV->SetMax(fBiasVoltMax->value());
        fBiasCamV->updateCamera();

        fBiasCamA->assignPixelMap(fPixelMap);
        fBiasCamA->setAutoscaleLowerLimit((fBiasCurrentMin->minimum()+0.5*fBiasCurrentMin->singleStep()));
        fBiasCamA->SetMin(fBiasCurrentMin->value());
        fBiasCamA->SetMax(fBiasCurrentMax->value());
        fBiasCamA->updateCamera();

        // --------------------------------------------------------------------------

        fRatesCanv->assignPixelMap(fPixelMap);
        fRatesCanv->setAutoscaleLowerLimit((fRatesMin->minimum()+0.5*fRatesMin->singleStep())*0.001);
        fRatesCanv->SetMin(fRatesMin->value());
        fRatesCanv->SetMax(fRatesMax->value());
        fRatesCanv->updateCamera();
        on_fPixelIdx_valueChanged(0);

        // --------------------------------------------------------------------------

        fRatesCanv->setTitle("Patch rates");
        fRatesCanv->setUnits("Hz");

        fBiasCamA->setTitle("BIAS current");
        fBiasCamA->setUnits("uA");

        fBiasCamV->setTitle("Applied BIAS voltage");
        fBiasCamV->setUnits("V");

        fEventCanv1->setTitle("Average (all slices)");
        fEventCanv2->setTitle("RMS (all slices)");
        fEventCanv3->setTitle("Maximum (all slices)");
        fEventCanv4->setTitle("Position of maximum (all slices)");

        fEventCanv1->setUnits("mV");
        fEventCanv2->setUnits("mV");
        fEventCanv3->setUnits("mV");
        fEventCanv4->setUnits("slice");

        // --------------------------------------------------------------------------

        fFeedbackDevCam->setTitle("Control deviation (Pulser amplitude voltage)");
        fFeedbackCmdCam->setTitle("Applied voltage change (BIAS voltage)");

        fFeedbackDevCam->setUnits("mV");
        fFeedbackCmdCam->setUnits("mV");

        // --------------------------------------------------------------------------

        QTimer::singleShot(1000, this, SLOT(slot_RootUpdate()));

        //widget->setMouseTracking(true);
        //widget->EnableSignalEvents(kMouseMoveEvent);

        fFtmRateCanv->setMouseTracking(true);
        fFtmRateCanv->EnableSignalEvents(kMouseMoveEvent);

        fAdcDataCanv->setMouseTracking(true);
        fAdcDataCanv->EnableSignalEvents(kMouseMoveEvent);

        fRatesCanv->setMouseTracking(true);
        fEventCanv1->setMouseTracking(true);
        fEventCanv2->setMouseTracking(true);
        fEventCanv3->setMouseTracking(true);
        fEventCanv4->setMouseTracking(true);

        fBiasCamV->setMouseTracking(true);
        fBiasCamA->setMouseTracking(true);

        fFeedbackDevCam->setMouseTracking(true);
        fFeedbackCmdCam->setMouseTracking(true);

        fEventCanv1->ShowPixelCursor(true);
        fEventCanv2->ShowPixelCursor(true);
        fEventCanv3->ShowPixelCursor(true);
        fEventCanv4->ShowPixelCursor(true);

        fEventCanv1->ShowPatchCursor(true);
        fEventCanv2->ShowPatchCursor(true);
        fEventCanv3->ShowPatchCursor(true);
        fEventCanv4->ShowPatchCursor(true);

        fFeedbackDevCam->ShowPixelCursor(true);
        fFeedbackCmdCam->ShowPixelCursor(true);

        fFeedbackDevCam->ShowPatchCursor(true);
        fFeedbackCmdCam->ShowPatchCursor(true);

        connect(fRatesCanv, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fEventCanv1, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fEventCanv2, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fEventCanv3, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fEventCanv4, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));

        connect(fBiasCamV, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fBiasCamA, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));

        connect(fFeedbackDevCam, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));
        connect(fFeedbackCmdCam, SIGNAL(signalPixelMoveOver(int)),
                this, SLOT(slot_CameraMouseMove(int)));

        connect(fRatesCanv, SIGNAL(signalPixelDoubleClick(int)),
                this, SLOT(slot_CameraDoubleClick(int)));
        connect(fRatesCanv, SIGNAL(signalCurrentPixel(int)),
                this, SLOT(slot_ChoosePixelThreshold(int)));
        connect(fBiasCamV, SIGNAL(signalCurrentPixel(int)),
                this, SLOT(slot_ChooseBiasChannel(int)));
        connect(fBiasCamA, SIGNAL(signalCurrentPixel(int)),
                this, SLOT(slot_ChooseBiasChannel(int)));

        connect(fFtmRateCanv, SIGNAL(     RootEventProcessed(TObject*, unsigned int, TCanvas*)),
                this,         SLOT  (slot_RootEventProcessed(TObject*, unsigned int, TCanvas*)));
        connect(fAdcDataCanv, SIGNAL(     RootEventProcessed(TObject*, unsigned int, TCanvas*)),
                this,         SLOT  (slot_RootEventProcessed(TObject*, unsigned int, TCanvas*)));
    }

    ~FactGui()
    {
        // Unsubscribe all services
        for (map<string,DimInfo*>::iterator i=fServices.begin();
             i!=fServices.end(); i++)
            delete i->second;

        // This is allocated as a chuck of chars
        delete [] reinterpret_cast<char*>(fEventData);
    }
};

#endif
