#ifndef FACT_HtmlDelegate
#define FACT_HtmlDelegate

#include <QStyledItemDelegate>

class HtmlDelegate : public QStyledItemDelegate
{
public:
    HtmlDelegate(QObject *p=0) : QStyledItemDelegate(p)
    {
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};

#endif
