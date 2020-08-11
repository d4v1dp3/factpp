#include "HeadersPower.h"

#include <string>
#include <iomanip>
#include <iostream>

#include <QString>
#include <QtXml/QDomNamedNodeMap>

#include "WindowLog.h"

using namespace std;
using namespace Power;

bool Status::Set(bool &rc, const QString &value)
{
    rc = value.toInt();
    return true;
}

bool Status::Set(const QDomNamedNodeMap &map)
{
    if (!map.contains("id") || !map.contains("title"))
        return false;

    QString item  = map.namedItem("id").nodeValue();
    QString value = map.namedItem("title").nodeValue();

    if (item==(QString("flow_meter")))
        return Set(fWaterFlowOk, value);

    if (item==(QString("level")))
        return Set(fWaterLevelOk, value);

    if (item==(QString("bias_power")))
        return Set(fPwrBiasOn, value);

    if (item==(QString("power_24v")))
        return Set(fPwr24VOn, value);

    if (item==(QString("pump")))
        return Set(fPwrPumpOn, value);

    if (item==(QString("drive_power")))
        return Set(fPwrDriveOn, value);

    if (item==(QString("drive_on")))
        return Set(fDriveMainSwitchOn, value);

    if (item==(QString("drive_enable")))
        return Set(fDriveFeedbackOn, value);

    return false;
}

void Status::Print(ostream &out, const char *title, const bool &val, const char *t, const char *f)
{
    out << setw(9) << title << " : ";
    if (val)
        out << kGreen << t << kReset << '\n';
    else
        out << kRed   << f << kReset << '\n';
}

void Status::Print(ostream &out)
{
    out << kReset << '\n';
    out << "------- WATER -------\n";
    Print(out, "level",    fWaterLevelOk, "ok", "low");
    Print(out, "flow",     fWaterFlowOk,  "ok", "low");
    out << "------- POWER -------\n";
    Print(out, "24V",      fPwr24VOn);
    Print(out, "pump",     fPwrPumpOn);
    Print(out, "bias",     fPwrBiasOn);
    Print(out, "drive",    fPwrDriveOn);
    out << "------- DRIVE -------\n";
    Print(out, "feedback", fDriveFeedbackOn,   "on", "off");
    Print(out, "main",     fDriveMainSwitchOn, "on", "off");
    out << "---------------------" << endl;
}
