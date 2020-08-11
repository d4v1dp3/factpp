// **************************************************************************
/** @class HtmlDelegate

@brief A Qt-Delegate to display HTML text (QTextDocument) in a list

*/
// **************************************************************************
#include "HtmlDelegate.h"

#include <QPainter>
#include <QTextDocument>

void HtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTextDocument doc;
    doc.setPageSize(option.rect.size());
    doc.setHtml(index.data().toString());

    // === This can be used if a scrolling is needed ===
    // painter->save();
    // painter->translate(option.rect.topLeft());
    // QRect r(QPoint(0, 0), option.rect.size());
    // doc.drawContents(painter, r);
    // painter->restore();
    // drawFocus(painter, option, option.rect);

    doc.drawContents(painter, option.rect);
}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTextDocument doc;
    doc.setPageSize(option.rect.size());
    doc.setHtml(index.data().toString());
    return doc.size().toSize();
}
