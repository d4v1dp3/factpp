#include "HeadersFAD.h"

#include <string.h>

#include <iomanip>

using namespace std;

void FAD::EventHeader::print(std::ostream &out) const
{
    out << "Delimiter:  " << hex << fStartDelimiter;
    out << (fStartDelimiter==kDelimiterStart?" (ok)":" (WRONG)") << endl;
    out << " (Crate=" << dec << Crate() << ", Board=" << Board() << ", Version=" << (fVersion>>8) << "." << (fVersion&0xff) << ", DNA=" << hex << fDNA <<")" << endl;

    out << dec;
    out << "PkgLength:  " << fPackageLength << endl;

    out << "RunNumber:  " << fRunNumber << endl;
    out << "Time:       " << setprecision(3) << fixed << fTimeStamp/10000. << "s" << endl;
    out << "EvtCounter: " << fEventCounter << " of " << fNumTriggersToGenerate << endl;
    out << "Trigger:    Type=" << hex << fTriggerType << dec << " Counter=" << fTriggerCounter << " Crc=";
    if ((fTriggerCrc&0xff00)==0x0100)
        out << "<timeout>";
    else
        out << "0x" << hex << fTriggerCrc;
    out << " (" << hex << setw(4) << fTriggerCrc << "|" << setw(4) << fTriggerType << "|" << setw(8) << fTriggerCounter << ")" << dec << endl;
    out << "            N/40  = " << dec << GetTriggerLogic() << endl;
    out << "            TRG   =";

    if (IsTriggerPhys())
        out << " phys";
    if (HasTriggerPed())
        out << " ped";
    if (HasTriggerLPext())
        out << " LPext";
    if (HasTriggerLPint())
        out << " LPint";
    if (HasTIMsource())
        out << " TIM";
    if (HasTriggerExt1())
        out << " ext1";
    if (HasTriggerExt2())
        out << " ext2";
    out << endl;

    out << "            LPset = " << GetTriggerLPset() << endl;

    out << "RefClock:   " << dec << fFreqRefClock << " (approx. " << fFreqRefClock*2.048 <<  "MHz)" << endl;
    out << "PhaseShift: " << fAdcClockPhaseShift << endl;
    out << "Prescaler:  " << fTriggerGeneratorPrescaler << endl;

    out << "DAC:       " << dec;
    for (int i=0; i<kNumDac; i++)
        out << " " << fDac[i];
    out << endl;

    out << "Temp:      " << dec;
    for (int i=0; i<kNumTemp; i++)
        out << " " << GetTemp(i);
    out << endl;

    out << "Status=" << hex << fStatus << endl;
    // PllLock -> 1111
    out << "  RefClk locked (PLLLCK):    ";
    if ((PLLLCK()&15)==15)
        out << "all";
    else
        if (PLLLCK()==0)
            out << "none";
        else
            out
                << "0:" << ((PLLLCK()&1)?"yes":"no") << " "
                << "1:" << ((PLLLCK()&2)?"yes":"no") << " "
                << "2:" << ((PLLLCK()&4)?"yes":"no") << " "
                << "3:" << ((PLLLCK()&8)?"yes":"no") << endl;
//    if (IsRefClockTooHigh())
//        out << " (too high)";
    if (IsRefClockTooLow())
        out << " (too low)";
    out << endl;
    out << "  Domino wave (Denable):     " << (HasDenable()?"enabled":"disabled") << endl;
    out << "  DRS sampling (Dwrite):     " << (HasDwrite()?"enabled":"disabled") << endl;
    out << "  Dig.clock manager (DCM):   " << (IsDcmLocked()?"locked":"unlocked");
    out << " / " << (IsDcmReady()?"ready":"not ready") << endl;
    out << "  SPI Serial Clock (SCLK):   " << (HasSpiSclk()?"enabled":"disabled") << endl;
    out << "  Busy enabled:              ";
    if (HasBusyOn())
        out << "constantly enabled" << endl;
    else
        out << (HasBusyOff()?"constantly disabled":"normal") << endl;
    out << "  Trigger line enabled:      " << (HasTriggerEnabled()?"enabled":"disabled") << endl;
    out << "  Continous trigger enabled: " << (HasContTriggerEnabled()?"enabled":"disabled") << endl;
    out << "  Data transmission socket:  " << (IsInSock17Mode()?"Socket 1-7":"Sockets 0") << endl;
}

void FAD::ChannelHeader::print(std::ostream &out) const
{
    out << "Chip=" << dec << Chip() << " Ch=" << Channel() << ":";
    out << " StartCell=" << fStartCell;
    out << " ROI=" << fRegionOfInterest << endl;
}
