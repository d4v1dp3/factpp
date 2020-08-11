#ifndef FACT_FitsFile
#define FACT_FitsFile

#include <CCfits/CCfits>

#include "MessageImp.h"
#include "Time.h"

class FitsFile : public MessageImp
{
public:
    MessageImp &fMsg;

    std::vector<std::string> fColNames;
    std::vector<std::string> fColTypes;
    std::vector<std::string> fColUnits;

    CCfits::FITS*  fFile;        /// The pointer to the CCfits FITS file
    CCfits::Table* fTable;       /// The pointer to the CCfits binary table

    size_t fNumRows;             ///the number of rows that have been written already to the FITS file.
    size_t fCursor;

    bool fIsOwner;

    int Write(const Time &time, const std::string &txt, int qos)
    {
        return fMsg.Write(time, txt, qos);
    }

public:
    FitsFile(MessageImp &imp) :
        fMsg(imp), fFile(0), fTable(0), fNumRows(0), fCursor(0)
    {
    }
    ~FitsFile() { Close(); }

    bool WriteDefaultKeys(const string &prgname, float version=1.0);

    void AddColumn(char type, const string &name, int numElems=1, const string &unit="");
    void AddColumn(const string &name, const string &format, const string &unit="");
    void AddColumn(char type, const string &name, const string &unit)
    {
        AddColumn(type, name, 1, unit);
    }

    void ResetColumns();

    bool OpenFile(const string &filename, bool allow_open=false);
    bool SetFile(CCfits::FITS *file=0);
    bool OpenTable(const string &tablename);
    bool OpenNewTable(const string &tableName, int maxtry=1);

    template <typename T>
    void WriteKey(const string &name, const T &value, const string &comment)
    {
        if (fTable)
            fTable->addKey(name, value, comment);
    }

    template <typename T>
        bool WriteKeyNT(const string &name, const T &value, const string &comment)
    {
        if (!fTable)
            return false;

        try
        {
            fTable->addKey(name, value, comment);
        }
        catch (const CCfits::FitsException &e)
        {
            Error("CCfits::Table::addKey failed for '"+name+"' in '"+fFile->name()+'/'+fTable->name()+"': "+e.message());
            return false;
        }

        return true;
    }

    bool AddRow();
    bool WriteData(size_t &start, const void *ptr, size_t size);
    bool WriteData(const void *ptr, size_t size)
    {
        return WriteData(fCursor, ptr, size);
    }

    template<typename T>
        bool WriteData(const std::vector<T> &vec)
    {
        return WriteData(fCursor, vec.data(), vec.size()*sizeof(T));
    }

    void Close();

    void Flush();

    bool IsOpen() const { return fFile && fTable; }

    const std::vector<std::string> &GetColumnTypes() const { return fColTypes; }
    string GetName() const { return fFile ? fFile->name() : "<no file open>"; }
    bool IsOwner() const { return fIsOwner; }

    size_t GetDataSize() const;

    size_t GetNumRows() const { return fNumRows; }

};

#endif
