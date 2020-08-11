#include "HeadersFTM.h"

#include <string.h>

#include <iomanip>

#include "Converter.h"

using namespace std;

void FTM::Header::print(std::ostream &out) const
{
    out << "State=" << std::dec << (fState&kFtmStates);
    switch (fState&kFtmStates)
    {
    case kFtmIdle:    out << " [idle]";    break;
    case kFtmConfig:  out << " [config]";  break;
    case kFtmRunning: out << " [running]"; break;
    case kFtmCalib:   out << " [calib]";   break;
    }

    out << "  Type=" << fType;
    switch (fType)
    {
    case kHeader:      out << " [header]";    break;
    case kStaticData:  out << " [static]";    break;
    case kDynamicData: out << " [dynamic]";   break;
    case kFtuList:     out << " [ftulist]";   break;
    case kErrorList:   out << " [errorlist]"; break;
    case kRegister:    out << " [register]";  break;
    }

    out << "  (len=" << fDataSize << ")";
    out << "  Id=0x" << std::hex << fBoardId;
    out << "  FW=" << fFirmwareId;
    out << "  TriggerCounter=" << std::dec << fTriggerCounter;
    out << "  TimeStamp=" << fTimeStamp;
    if (fState&kFtmLocked)
        out << "  [locked]";
    else
        out << "  [unlocked]";
    out << std::endl;
}

void FTM::FtuResponse::print(std::ostream &out) const
{
    out << std::hex << "Pings=" << ((fPingAddr>>8)&0x3);
    out << "  Addr=" << std::setw(2) << (fPingAddr&0x1f);
    out << "  DNA=" << std::setw(16) << fDNA;
    out << " ErrorCounter=" << std::dec << fErrorCounter << std::endl;
}

void FTM::FtuList::print(std::ostream &out) const
{
    out << "Number of boards responded: " << std::dec << fNumBoards << " (";
    out << fNumBoardsCrate[0] << ", ";
    out << fNumBoardsCrate[1] << ", ";
    out << fNumBoardsCrate[2] << ", ";
    out << fNumBoardsCrate[3] << ")" << std::endl;
    out << "Active boards: " << std::hex;
    out << std::setfill('0');
    out << std::setw(4) << fActiveFTU[0];
    out << std::setw(4) << fActiveFTU[1];
    out << std::setw(4) << fActiveFTU[2];
    out << std::setw(4) << fActiveFTU[3] << std::dec << std::endl;
    for (int c=0; c<4; c++)
        for (int b=0; b<10; b++)
        {
            out << ' ' << c << ':' << std::setfill('0') << std::setw(2) << b << ": ";
            out << fFTU[c][b];
        }
}

void FTM::DynamicDataBoard::print(std::ostream &out) const
{
    out << "Rate=" << std::setw(5) << fRateTotal << " (";
    out << std::setw(5) << fRatePatch[0] << ", ";
    out << std::setw(5) << fRatePatch[1] << ", ";
    out << std::setw(5) << fRatePatch[2] << ", ";
    out << std::setw(5) << fRatePatch[3] << ") ";
    out << "Overflow=" << fOverflow << " ";
    out << "CrcError=" << fCrcError << std::endl;
}

void FTM::DynamicData::print(std::ostream &out) const
{
    out << "OnTime=" << std::dec << fOnTimeCounter << " ";
    out << "Temp=(";
    out << fTempSensor[0] << ",";
    out << fTempSensor[1] << ",";
    out << fTempSensor[2] << ",";
    out << fTempSensor[3] << ")" << std::endl;

    for (int c=0; c<4; c++)
        for (int b=0; b<10; b++)
        {
            out << ' ' << c << ':' << std::setfill('0') << std::setw(2) << b << ": ";
            out << fBoard[c][b];
        }
}

void FTM::StaticDataBoard::print(std::ostream &out) const
{
    out << "Enable=( " << std::hex;
    for (int i=0; i<4; i++)
        out << std::setw(4) << fEnable[i] << " ";
    out << ")  " << std::dec;

    out << "DAC A=" << fDAC[0] << " ";
    out << "B=" << fDAC[1] << " ";
    out << "C=" << fDAC[2] << " ";
    out << "D=" << fDAC[3] << " ";
    out << "H=" << fDAC[4] << "  ";

    out << "Prescaling=" << fPrescaling << endl;
}

void FTM::StaticData::print(std::ostream &out) const
{
    out << std::hex;
    out << "General settings: ";
    if (IsEnabled(kTrigger))
        out << " Trigger";
    if (IsEnabled(kPedestal))
        out << " Pedestal";
    if (IsEnabled(kLPint))
        out << " LPint";
    if (IsEnabled(kLPext))
        out << " LPext";
    if (IsEnabled(kExt1))
        out << " Ext1";
    if (IsEnabled(kExt2))
        out << " Ext2";
    if (IsEnabled(kVeto))
        out << " Veto";
    if (IsEnabled(kClockConditioner))
        out << " ClockCond";
    out << " (" << fGeneralSettings << ")" << endl;
    out << "Status LEDs:       " << fStatusLEDs << endl;
    out << std::dec;
    out << "TriggerInterval:   " << fTriggerInterval << " ms" << endl;
    out << "TriggerSequence:   ";
    out <<  (fTriggerSequence     &0x1f) << ":";
    out << ((fTriggerSequence>> 5)&0x1f) << ":";
    out << ((fTriggerSequence>>10)&0x1f) << " (LPint:LPext:PED)" << endl;
    out << "Coinc. physics:    " << std::setw(2) << fMultiplicityPhysics << "/N  ";
    out << fWindowPhysics*4+8 << "ns" << endl;
    out << "Coinc. calib:      " << std::setw(2) << fMultiplicityCalib << "/N  ";
    out << fWindowCalib*4+8 << "ns" << endl;
    out << "Trigger delay:     " << fDelayTrigger*4+8 << "ns" << endl;
    out << "Time marker delay: " << fDelayTimeMarker*4+8 << "ns" << endl;
    out << "Dead time:         " << fDeadTime*4+8 << "ns" << endl;
    out << "Light pulser (int): " << dec << (int)fIntensityLPint;
    if (fEnableLPint&kGroup1)
        out << " + Group1";
    if (fEnableLPint&kGroup2)
        out << " + Group2";
    out << endl;
    out << "Light pulser (ext): " << dec << (int)fIntensityLPext;
    if (fEnableLPext&kGroup1)
        out << " + Group1";
    if (fEnableLPext&kGroup2)
        out << " + Group2";
    out << endl;
    out << "Clock conditioner:";
    out << std::hex << setfill('0');
    for (int i=0; i<8; i++)
        out << " " << setw(8) << fClockConditioner[i];
    out << endl;
    out << "Active FTUs:       ";
    out << fActiveFTU[0] << " ";
    out << fActiveFTU[1] << " ";
    out << fActiveFTU[2] << " ";
    out << fActiveFTU[3] << endl;
    out << std::dec;

    for (int c=0; c<4; c++)
        for (int b=0; b<10; b++)
        {
            out << ' ' << c << ':' << std::setfill('0') << std::setw(2) << b << ": ";
            out << fBoard[c][b];
        }
}

void FTM::Error::print(std::ostream &out) const
{
    out << dec;
    out << "ERROR: Num Calls   = " << fNumCalls;
    if (fNumCalls==0)
        out << " (too many)";
    out << endl;
    out << "       Delimiter   = " << (fDelimiter=='@'?"ok":"wrong") << endl;
    out << "       Path        = ";
    if (fSrcAddress==0xc0)
        out << "FTM(192)";
    else
        out << "FTU(" << (fSrcAddress &0x3) << ":" << (fSrcAddress >>2) << ")";
    out << " --> ";
    if (fDestAddress==0xc0)
        out << "FTM(192)";
    else
        out << "FTU(" << (fDestAddress&0x3) << ":" << (fDestAddress>>2) << ")";
    out << endl;
    out << "       FirmwareId  = " << hex << fFirmwareId << endl;
    out << "       Command     = " << hex << fCommand << endl;
    out << "       CRC counter = " << dec << fCrcErrorCounter << endl;
    out << "       CRC         = " << hex << fCrcCheckSum << endl;
    out << "       Data: " << Converter::GetHex<unsigned short>(fData, 0, false) << endl;
}
