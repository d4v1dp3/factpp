// **************************************************************************
/** @class CheckBoxDelegate

@brief A delegate which displays an arrow if there are sub items and raises an event if the checkbox is checked

*/
// **************************************************************************
#include "CheckBoxDelegate.h"

#include <QPainter>
#include <QApplication>

void CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //---  QColumnViewDelegate
    const bool reverse = (option.direction == Qt::RightToLeft);
    const int  width   = (option.rect.height() * 2) / 3;


    // Modify the options to give us room to add an arrow
    QStyleOptionViewItemV4 opt = option;
    if (reverse)
        opt.rect.adjust(width,0,0,0);
    else
        opt.rect.adjust(0,0,-width,0);

    if (!(index.model()->flags(index) & Qt::ItemIsEnabled))
    {
        opt.showDecorationSelected = true;
        opt.state |= QStyle::State_Selected;
    }


    QStyledItemDelegate::paint(painter, opt, index);


    if (reverse)
        opt.rect = QRect(option.rect.x(), option.rect.y(),
                         width, option.rect.height());
    else
        opt.rect = QRect(option.rect.x() + option.rect.width() - width, option.rect.y(),
                         width, option.rect.height());

    // Draw >
    if (index.model()->hasChildren(index))
    {
        const QWidget *view = opt.widget;

        QStyle *style = view ? view->style() : qApp->style();
        style->drawPrimitive(QStyle::PE_IndicatorColumnViewArrow, &opt,
                             painter, view);
    }
}


bool CheckBoxDelegate::editorEvent(QEvent *evt, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                   const QModelIndex &index)
{
    QStandardItemModel *it = dynamic_cast<QStandardItemModel*>(model);
    if (!it)
        return QStyledItemDelegate::editorEvent(evt, model, option, index);

    const QStandardItem *item = it->itemFromIndex(index);
    if (!item)
        return QStyledItemDelegate::editorEvent(evt, model, option, index);

    const Qt::CheckState before = item->checkState();

    const bool rc = QStyledItemDelegate::editorEvent(evt, model, option, index);

    const Qt::CheckState after = item->checkState();

    if (before!=after)
        QApplication::sendEvent(it->parent(), new CheckBoxEvent(*item));

    return rc;
}

// **************************************************************************
/** @class CheckBoxEvent

@brief An event posted by the CheckBoxDelegate if the CheckBox is used

*/
// **************************************************************************
