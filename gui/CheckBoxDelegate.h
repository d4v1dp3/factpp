#ifndef FACT_CheckBoxDelegate
#define FACT_CheckBoxDelegate

#include <QEvent>
#include <QStandardItem>

using namespace std;

class CheckBoxEvent : public QEvent
{
public:
    const QStandardItem &item;

    CheckBoxEvent(const QStandardItem &i)
        : QEvent((QEvent::Type)QEvent::registerEventType()),
    item(i) { }
};


#include <QStyledItemDelegate>

class CheckBoxDelegate : public QStyledItemDelegate
{
public:
    CheckBoxDelegate(QObject *p=0) : QStyledItemDelegate(p)
    {
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

    bool editorEvent(QEvent *evt, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index);
};

#endif
