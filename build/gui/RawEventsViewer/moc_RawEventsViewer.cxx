/****************************************************************************
** Meta object code from reading C++ file 'RawEventsViewer.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../gui/RawEventsViewer/RawEventsViewer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RawEventsViewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RawDataViewer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   15,   14,   14, 0x05,
      51,   45,   14,   14, 0x05,
      75,   14,   14,   14, 0x05,
      97,   91,   14,   14, 0x05,
     121,   14,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
     145,   14,   14,   14, 0x0a,
     157,   14,   14,   14, 0x0a,
     170,   14,   14,   14, 0x0a,
     187,  182,   14,   14, 0x0a,
     205,   14,   14,   14, 0x0a,
     217,   14,   14,   14, 0x0a,
     233,   14,   14,   14, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RawDataViewer[] = {
    "RawDataViewer\0\0event\0signalCurrentEvent(int)\0"
    "slice\0signalCurrentSlice(int)\0"
    "newFileLoaded()\0pixel\0signalCurrentPixel(int)\0"
    "signalAutoScaleNeeded()\0plusEvent()\0"
    "minusEvent()\0readEvent()\0step\0"
    "setEventStep(int)\0nextSlice()\0"
    "previousSlice()\0setCurrentPixel(int)\0"
};

void RawDataViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        RawDataViewer *_t = static_cast<RawDataViewer *>(_o);
        switch (_id) {
        case 0: _t->signalCurrentEvent((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->signalCurrentSlice((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->newFileLoaded(); break;
        case 3: _t->signalCurrentPixel((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->signalAutoScaleNeeded(); break;
        case 5: _t->plusEvent(); break;
        case 6: _t->minusEvent(); break;
        case 7: _t->readEvent(); break;
        case 8: _t->setEventStep((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->nextSlice(); break;
        case 10: _t->previousSlice(); break;
        case 11: _t->setCurrentPixel((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RawDataViewer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RawDataViewer::staticMetaObject = {
    { &BasicGlCamera::staticMetaObject, qt_meta_stringdata_RawDataViewer,
      qt_meta_data_RawDataViewer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RawDataViewer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RawDataViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RawDataViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RawDataViewer))
        return static_cast<void*>(const_cast< RawDataViewer*>(this));
    return BasicGlCamera::qt_metacast(_clname);
}

int RawDataViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BasicGlCamera::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void RawDataViewer::signalCurrentEvent(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RawDataViewer::signalCurrentSlice(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RawDataViewer::newFileLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void RawDataViewer::signalCurrentPixel(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RawDataViewer::signalAutoScaleNeeded()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
static const uint qt_meta_data_UIConnector[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      54,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      18,   13,   12,   12, 0x0a,
      40,   13,   12,   12, 0x0a,
      67,   12,   12,   12, 0x0a,
      89,   83,   12,   12, 0x0a,
     120,   12,   12,   12, 0x0a,
     144,  138,   12,   12, 0x0a,
     178,  172,   12,   12, 0x0a,
     206,   12,   12,   12, 0x0a,
     235,   12,   12,   12, 0x0a,
     266,   12,   12,   12, 0x0a,
     298,   12,   12,   12, 0x0a,
     337,   12,   12,   12, 0x0a,
     378,   12,   12,   12, 0x0a,
     416,   12,   12,   12, 0x0a,
     461,   12,   12,   12, 0x0a,
     497,   12,   12,   12, 0x0a,
     533,   12,   12,   12, 0x0a,
     569,   12,   12,   12, 0x0a,
     605,   12,   12,   12, 0x0a,
     641,   12,   12,   12, 0x0a,
     675,   12,   12,   12, 0x0a,
     709,   12,   12,   12, 0x0a,
     743,   12,   12,   12, 0x0a,
     777,   12,   12,   12, 0x0a,
     811,   12,   12,   12, 0x0a,
     847,   12,   12,   12, 0x0a,
     883,   12,   12,   12, 0x0a,
     919,   12,   12,   12, 0x0a,
     955,   12,   12,   12, 0x0a,
     991,   12,   12,   12, 0x0a,
    1026,   12,   12,   12, 0x0a,
    1061,   12,   12,   12, 0x0a,
    1096,   12,   12,   12, 0x0a,
    1131,   12,   12,   12, 0x0a,
    1166,   12,   12,   12, 0x0a,
    1208,   12,   12,   12, 0x0a,
    1226,   12,   12,   12, 0x0a,
    1240,   12,   12,   12, 0x0a,
    1269,   12,   12,   12, 0x2a,
    1295,   12,   12,   12, 0x0a,
    1324,   12,   12,   12, 0x0a,
    1356,   12,   12,   12, 0x0a,
    1388,   12,   12,   12, 0x0a,
    1420,   12,   12,   12, 0x0a,
    1452,   12,   12,   12, 0x0a,
    1480,   12,   12,   12, 0x0a,
    1515,   12,   12,   12, 0x0a,
    1550,   12,   12,   12, 0x0a,
    1567,   12,   12,   12, 0x0a,
    1592, 1586,   12,   12, 0x0a,
    1632,   12,   12,   12, 0x0a,
    1672,   12,   12,   12, 0x0a,
    1712,   12,   12,   12, 0x0a,
    1741, 1736, 1732,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_UIConnector[] = {
    "UIConnector\0\0file\0fileSelected(QString)\0"
    "calibFileSelected(QString)\0newFileLoaded()\0"
    "value\0slicesPerSecondChanged(double)\0"
    "nextSlicePlease()\0slice\0"
    "currentSliceHasChanged(int)\0event\0"
    "currentEventHasChanged(int)\0"
    "on_playPauseButton_clicked()\0"
    "on_loadNewFileButton_clicked()\0"
    "on_loadDRSCalibButton_clicked()\0"
    "on_drawPatchCheckBox_stateChanged(int)\0"
    "on_drawImpulseCheckBox_stateChanged(int)\0"
    "on_drawBlurCheckBox_stateChanged(int)\0"
    "on_loopOverCurrentEventBox_stateChanged(int)\0"
    "on_colorRange0_valueChanged(double)\0"
    "on_colorRange1_valueChanged(double)\0"
    "on_colorRange2_valueChanged(double)\0"
    "on_colorRange3_valueChanged(double)\0"
    "on_colorRange4_valueChanged(double)\0"
    "on_redValue0_valueChanged(double)\0"
    "on_redValue1_valueChanged(double)\0"
    "on_redValue2_valueChanged(double)\0"
    "on_redValue3_valueChanged(double)\0"
    "on_redValue4_valueChanged(double)\0"
    "on_greenValue0_valueChanged(double)\0"
    "on_greenValue1_valueChanged(double)\0"
    "on_greenValue2_valueChanged(double)\0"
    "on_greenValue3_valueChanged(double)\0"
    "on_greenValue4_valueChanged(double)\0"
    "on_blueValue0_valueChanged(double)\0"
    "on_blueValue1_valueChanged(double)\0"
    "on_blueValue2_valueChanged(double)\0"
    "on_blueValue3_valueChanged(double)\0"
    "on_blueValue4_valueChanged(double)\0"
    "on_slicesPerSecValue_valueChanged(double)\0"
    "pixelChanged(int)\0cbpxChanged()\0"
    "on_HwIDBox_valueChanged(int)\0"
    "on_HwIDBox_valueChanged()\0"
    "on_SwIDBox_valueChanged(int)\0"
    "on_crateIDBox_valueChanged(int)\0"
    "on_boardIDBox_valueChanged(int)\0"
    "on_patchIDBox_valueChanged(int)\0"
    "on_pixelIDBox_valueChanged(int)\0"
    "on_autoScaleColor_clicked()\0"
    "on_entireCameraScale_toggled(bool)\0"
    "on_currentPixelScale_toggled(bool)\0"
    "slicesPlusPlus()\0slicesMinusMinus()\0"
    "state\0on_calibratedCheckBox_stateChanged(int)\0"
    "on_displayingSliceBox_valueChanged(int)\0"
    "on_displayingEventBox_valueChanged(int)\0"
    "displaySliceValue()\0int\0conf\0"
    "SetupConfiguration(Configuration&)\0"
};

void UIConnector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        UIConnector *_t = static_cast<UIConnector *>(_o);
        switch (_id) {
        case 0: _t->fileSelected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->calibFileSelected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->newFileLoaded(); break;
        case 3: _t->slicesPerSecondChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->nextSlicePlease(); break;
        case 5: _t->currentSliceHasChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->currentEventHasChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->on_playPauseButton_clicked(); break;
        case 8: _t->on_loadNewFileButton_clicked(); break;
        case 9: _t->on_loadDRSCalibButton_clicked(); break;
        case 10: _t->on_drawPatchCheckBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->on_drawImpulseCheckBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->on_drawBlurCheckBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->on_loopOverCurrentEventBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->on_colorRange0_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 15: _t->on_colorRange1_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 16: _t->on_colorRange2_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 17: _t->on_colorRange3_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 18: _t->on_colorRange4_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 19: _t->on_redValue0_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 20: _t->on_redValue1_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 21: _t->on_redValue2_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 22: _t->on_redValue3_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 23: _t->on_redValue4_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 24: _t->on_greenValue0_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 25: _t->on_greenValue1_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 26: _t->on_greenValue2_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 27: _t->on_greenValue3_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 28: _t->on_greenValue4_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 29: _t->on_blueValue0_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 30: _t->on_blueValue1_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 31: _t->on_blueValue2_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 32: _t->on_blueValue3_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 33: _t->on_blueValue4_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 34: _t->on_slicesPerSecValue_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 35: _t->pixelChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 36: _t->cbpxChanged(); break;
        case 37: _t->on_HwIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 38: _t->on_HwIDBox_valueChanged(); break;
        case 39: _t->on_SwIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 40: _t->on_crateIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 41: _t->on_boardIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 42: _t->on_patchIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 43: _t->on_pixelIDBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 44: _t->on_autoScaleColor_clicked(); break;
        case 45: _t->on_entireCameraScale_toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 46: _t->on_currentPixelScale_toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 47: _t->slicesPlusPlus(); break;
        case 48: _t->slicesMinusMinus(); break;
        case 49: _t->on_calibratedCheckBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 50: _t->on_displayingSliceBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 51: _t->on_displayingEventBox_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 52: _t->displaySliceValue(); break;
        case 53: { int _r = _t->SetupConfiguration((*reinterpret_cast< Configuration(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData UIConnector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject UIConnector::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_UIConnector,
      qt_meta_data_UIConnector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &UIConnector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *UIConnector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *UIConnector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_UIConnector))
        return static_cast<void*>(const_cast< UIConnector*>(this));
    if (!strcmp(_clname, "Ui::MainWindow"))
        return static_cast< Ui::MainWindow*>(const_cast< UIConnector*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int UIConnector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 54)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 54;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
