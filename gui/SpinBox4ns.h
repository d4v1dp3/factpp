#ifndef FACT_SpinBox4ns
#define FACT_SpinBox4ns

#include <QSpinBox>

class SpinBox4ns : public QSpinBox
{
public:
    SpinBox4ns(QWidget *p) : QSpinBox(p)
    {
    }

    QValidator::State validate(QString &input, int &p) const
    {
        const QValidator::State rc = QSpinBox::validate(input, p);
        if (rc!=QValidator::Acceptable)
            return rc;

        const int pf = prefix().length();
        const int sf = suffix().length();
        const int len = input.length();

        return input.mid(pf, len-sf-pf).toUInt()%4==0 ? QValidator::Acceptable : QValidator::Intermediate;
    }

    void fixup(QString &input) const
    {
        const int pf = prefix().length();
        const int sf = suffix().length();
        const int len = input.length();

        const uint i = input.mid(pf, len-pf-sf).toUInt()+2;
        input = QString::number((i/4)*4);
    }
};

#endif

// **************************************************************************
/** @class SpinBox4ns

@brief A QSpinBox which only accepts values dividable by 4

*/
// **************************************************************************
