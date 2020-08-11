/*
 * TempViewer.cc
 *
 *  Created on: Aug 26, 2011
 *      Author: lyard
 */
#include "Q3DCameraWidget.h"
#include <qapplication.h>
#include <qlayout.h>

#include "dic.hxx"

class TemperatureSub : public DimClient
{
    DimStampedInfo info;
    Q3DCameraWidget* view;
    int numC;

public:
    TemperatureSub() : info("FSC_CONTROL/TEMPERATURE", (void*)NULL, 0, this), view(NULL)
    {numC = 0;}
    void setViewer(Q3DCameraWidget* v) { view = v;}
    void infoHandler()
    {
        DimInfo* I = getInfo();
        if (!(I==&info))
        {
            cout << "Hum, I'm getting info from subsciptions to which I didn\'t subscribe... weird" << endl;
            return;
        }
        float* values = (float*)(I->getData());
        if (I->getSize() != 60*sizeof(float))
        {
            cout << "wrong size: " << I->getSize() << endl;
            return;
        }
        if (view)// && numC > 2)
            view->updateData(values);
        numC++;
    }
};
void do3DView(int argc, char** argv)
{
    QApplication a(argc, argv);

    Q3DCameraWidget* view = new Q3DCameraWidget();
    TemperatureSub sub;

    QWidget window;
    QHBoxLayout* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(view);
//    layout->setMouseTracking(true);
//    window.setMouseTracking(true);
//    view->setMouseTracking(true);
    window.resize(600,600);
    window.show();

    sub.setViewer(view);

    a.exec();

}

int main(int argc, char** argv)
{
    do3DView(argc, argv);

}
