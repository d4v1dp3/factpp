/****************************************************************************
** Meta object code from reading C++ file 'Q3DCameraWidget.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/Q3DCameraWidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Q3DCameraWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Q3DCameraWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Q3DCameraWidget[] = {
    "Q3DCameraWidget\0\0timedUpdate()\0"
};

void Q3DCameraWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Q3DCameraWidget *_t = static_cast<Q3DCameraWidget *>(_o);
        switch (_id) {
        case 0: _t->timedUpdate(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData Q3DCameraWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Q3DCameraWidget::staticMetaObject = {
    { &BasicGlCamera::staticMetaObject, qt_meta_stringdata_Q3DCameraWidget,
      qt_meta_data_Q3DCameraWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Q3DCameraWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Q3DCameraWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Q3DCameraWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Q3DCameraWidget))
        return static_cast<void*>(const_cast< Q3DCameraWidget*>(this));
    return BasicGlCamera::qt_metacast(_clname);
}

int Q3DCameraWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BasicGlCamera::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
