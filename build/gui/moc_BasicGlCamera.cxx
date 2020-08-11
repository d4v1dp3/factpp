/****************************************************************************
** Meta object code from reading C++ file 'BasicGlCamera.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/BasicGlCamera.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BasicGlCamera.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_BasicGlCamera[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   15,   14,   14, 0x05,
      45,   15,   14,   14, 0x05,
      70,   15,   14,   14, 0x05,
      98,   14,   14,   14, 0x05,
     123,   14,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
     144,   14,   14,   14, 0x0a,
     168,   14,   14,   14, 0x0a,
     189,   14,   14,   14, 0x0a,
     216,   14,   14,   14, 0x0a,
     242,   14,   14,   14, 0x0a,
     271,   14,   14,   14, 0x0a,
     298,   14,   14,   14, 0x0a,
     323,   14,   14,   14, 0x0a,
     350,   14,   14,   14, 0x0a,
     378,   14,   14,   14, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_BasicGlCamera[] = {
    "BasicGlCamera\0\0pixel\0signalCurrentPixel(int)\0"
    "signalPixelMoveOver(int)\0"
    "signalPixelDoubleClick(int)\0"
    "colorPaletteHasChanged()\0signalUpdateCamera()\0"
    "linearScalePlease(bool)\0logScalePlease(bool)\0"
    "regularPalettePlease(bool)\0"
    "prettyPalettePlease(bool)\0"
    "greyScalePalettePlease(bool)\0"
    "glowingPalettePlease(bool)\0"
    "zeroRotationPlease(bool)\0"
    "plus90RotationPlease(bool)\0"
    "minus90RotationPlease(bool)\0timedUpdate()\0"
};

void BasicGlCamera::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        BasicGlCamera *_t = static_cast<BasicGlCamera *>(_o);
        switch (_id) {
        case 0: _t->signalCurrentPixel((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->signalPixelMoveOver((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->signalPixelDoubleClick((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->colorPaletteHasChanged(); break;
        case 4: _t->signalUpdateCamera(); break;
        case 5: _t->linearScalePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->logScalePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->regularPalettePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->prettyPalettePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->greyScalePalettePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->glowingPalettePlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->zeroRotationPlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->plus90RotationPlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->minus90RotationPlease((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->timedUpdate(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData BasicGlCamera::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject BasicGlCamera::staticMetaObject = {
    { &QGLWidget::staticMetaObject, qt_meta_stringdata_BasicGlCamera,
      qt_meta_data_BasicGlCamera, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &BasicGlCamera::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *BasicGlCamera::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *BasicGlCamera::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_BasicGlCamera))
        return static_cast<void*>(const_cast< BasicGlCamera*>(this));
    return QGLWidget::qt_metacast(_clname);
}

int BasicGlCamera::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void BasicGlCamera::signalCurrentPixel(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BasicGlCamera::signalPixelMoveOver(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void BasicGlCamera::signalPixelDoubleClick(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void BasicGlCamera::colorPaletteHasChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void BasicGlCamera::signalUpdateCamera()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
