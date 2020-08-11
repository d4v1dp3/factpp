#include "DataWriteRaw.h"

#include "HeadersFAD.h"
#include "EventBuilder.h"

using namespace std;

void DataWriteRaw::WriteBlockHeader(uint32_t type, uint32_t ver, uint32_t cnt, uint32_t len)
{
    const uint32_t val[4] = { type, ver, cnt, len };

    fOut.write(reinterpret_cast<const char*>(val), sizeof(val));
}

template<typename T>
void DataWriteRaw::WriteValue(const T &t)
{
    fOut.write(reinterpret_cast<const char*>(&t), sizeof(T));
}

bool DataWriteRaw::Open(const RUN_HEAD &h, const FAD::RunDescription &)
{
    const string name = FormFileName("bin");
    if (access(name.c_str(), F_OK)==0)
    {
        Error("File '"+name+"' already exists.");
        return false;
    }

    fFileName = name;

    errno = 0;
    fOut.open(name.c_str(), ios_base::out);
    if (!fOut)
    {
        ostringstream str;
        str << "ofstream::open() failed for '" << name << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);

        return false;
    }

    fCounter = 0;

    static uint32_t FACT = 0xFAC77e1e;

    fOut.write(reinterpret_cast<char*>(&FACT), 4);

    WriteBlockHeader(kIdentifier, 1, 0, 8);
    WriteValue(uint32_t(0));
    WriteValue(GetRunId());

    WriteBlockHeader(kRunHeader, 1, 0, sizeof(RUN_HEAD)-sizeof(PEVNT_HEADER*));
    fOut.write(reinterpret_cast<const char*>(&h), sizeof(RUN_HEAD)-sizeof(PEVNT_HEADER*));

    for (int i=0; i<40; i++)
    {
        WriteBlockHeader(kBoardHeader, 1, i, sizeof(PEVNT_HEADER));
        fOut.write(reinterpret_cast<const char*>(h.FADhead+i), sizeof(PEVNT_HEADER));
    }

    // FIXME: Split this
    const vector<char> block(sizeof(uint32_t)/*+sizeof(RUN_TAIL)*/);
    WriteBlockHeader(kRunSummary, 1, 0, block.size());

    fPosTail = fOut.tellp();
    fOut.write(block.data(), block.size());

    if (!fOut)
    {
        ostringstream str;
        str << "ofstream::write() failed for '" << name << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);

        return false;
    }

    return true;
}

bool DataWriteRaw::WriteEvt(const EVT_CTRL2 &evt)
{
    const EVENT &e = *evt.fEvent;

    const int sh = sizeof(EVENT)-2 + NPIX*e.Roi*2;

    WriteBlockHeader(kEvent, 1, fCounter++, sh);
    fOut.write(reinterpret_cast<const char*>(&e)+2, sh);
    return true;
}

bool DataWriteRaw::Close()
{
    WriteBlockHeader(kEndOfFile, 0, 0, 0);

    /*
    if (tail)
    {
        fOut.seekp(fPosTail);

        WriteValue(uint32_t(1));
        fOut.write(reinterpret_cast<const char*>(tail), sizeof(RUN_TAIL));
    }*/

    if (!fOut)
    {
        ostringstream str;

        str << "ofstream::write() failed for '" << GetFileName() << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);

        return false;
    }

    fOut.close();

    if (!fOut)
    {
        ostringstream str;
        str << "ofstream::close() failed for '" << GetFileName() << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);

        return false;
    }

    return true;
}
