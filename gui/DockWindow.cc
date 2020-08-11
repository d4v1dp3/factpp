// **************************************************************************
/** @class DockWindow

@brief A main window which can be used to display a QDockWidget from a tab

*/
// **************************************************************************
#include "DockWindow.h"

#include <QDockWidget>
#include <QGridLayout>

#include <stdexcept>

using namespace std;

DockWindow::DockWindow(QDockWidget *d, const QString &name)
    : fDockWidget(d)
{
    QObject *w0 = d->parent();   // QWidget
    if (!w0)
        throw runtime_error("1st parent of QDockWidget is NULL");

    QObject *w1 = w0->parent();  // QWidget
    if (!w1)
        throw runtime_error("2nd parent of QDockWidget is NULL");

    QObject *w2 = w1->parent();  // QWidget
    if (!w2)
            throw runtime_error("3rd parent of QDockWidget is NULL");

    fTabWidget = dynamic_cast<QTabWidget*>(w2);
    if (!fTabWidget)
        throw runtime_error("3rd parent of QDockWidget is not a QTabWidget");

    setGeometry(d->geometry());
    addDockWidget(Qt::LeftDockWidgetArea, fDockWidget);
    setWindowTitle(name);

    // FIXME: ToolTip, WhatsThis

    show();
}

void DockWindow::closeEvent(QCloseEvent *)
{
    QWidget *w = new QWidget;

    QGridLayout *l = new QGridLayout(w);
    //layout->setObjectName(QString::fromUtf8("gridLayout_")+windowTitle());
    l->addWidget(fDockWidget, 0, 0, 1, 1);

    fTabWidget->addTab(w, windowTitle());
    fTabWidget->setTabsClosable(true);

    fDockWidget->setParent(w);
}
