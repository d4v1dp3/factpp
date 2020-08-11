#ifndef FACT_DockWindow
#define FACT_DockWindow

#include <QMainWindow>

class QDockWidget;
class QTabWidget;
class QCloseEvent;

class DockWindow : public QMainWindow
{
    Q_OBJECT;

    QDockWidget  *fDockWidget;
    QTabWidget   *fTabWidget;

public:

    DockWindow(QDockWidget *d, const QString &name);

protected:
    void closeEvent(QCloseEvent *);
};

#endif
