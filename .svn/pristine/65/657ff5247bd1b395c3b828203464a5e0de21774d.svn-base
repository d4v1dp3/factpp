#ifndef FACT_SpinBoxHex
#define FACT_SpinBoxHex

#include <QSpinBox>

#include <iostream>
class QRegExpValidator;

class SpinBoxHex : public QSpinBox
{
public:
    SpinBoxHex(QWidget *p=0) : QSpinBox(p)
    {
    }

protected:
    QValidator::State validate(QString &txt, int &/*pos*/) const
    {
        bool ok;
        txt.toInt(&ok, 16);

        return ok ? QValidator::Acceptable : QValidator::Invalid;
    }

    QString textFromValue(int val) const
    {
        return QString::number(val, 16).right(8).rightJustified(8, '0').toLower().insert(4, ':');
    }

    int valueFromText(const QString &txt) const
    {
        bool ok;
        return txt.toInt(&ok, 16);
    }
};

#endif

// **************************************************************************
/** @class SpinBoxHex

@brief A QSpinBox which displays the value as hex-value

*/
// **************************************************************************
