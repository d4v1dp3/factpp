// **************************************************************************
/** @class FitsFile

@brief FITS writter for the FACT project. 

The FactFits class is able to open, manage and update FITS files. 

The file columns should be given to the class before the file is openned. Once
a file has been created, the structure of its columns cannot be changed. Only
row can be added.

This class relies on the CCfits and CFitsIO packages.

*/
// **************************************************************************
#include "FitsFile.h"

#ifdef HAVE_NOVA
#include "nova.h"
#endif

using namespace std;
using namespace CCfits;

bool FitsFile::WriteDefaultKeys(const string &prgname, float version)
{
    if (!fTable)
        return false;

    try
    {
        const Time now;
        WriteKey("TELESCOP", "FACT", "Telescope that acquired this data");
        WriteKey("PACKAGE",   PACKAGE_NAME, "Package name");
        WriteKey("VERSION",   PACKAGE_VERSION, "Package description");
        WriteKey("CREATOR",  prgname, "Program that wrote this file");
        WriteKey("EXTREL",   version, "Release Number");
        WriteKey("COMPILED",  __DATE__ " " __TIME__, "Compile time");
        WriteKey("REVISION",  REVISION, "SVN revision");
        WriteKey("ORIGIN",   "FACT", "Institution that wrote the file");
        WriteKey("DATE",     now.Iso(), "File creation date");
        WriteKey("NIGHT",    now.NightAsInt(), "Night as int");
#ifdef HAVE_NOVA
        WriteKey("OBSERVAT", Nova::LnLatPosn::preset(), "Observatory name (see nova.h)");
#endif
        WriteKey("TIMESYS",  "UTC", "Time system");
        WriteKey("TIMEUNIT", "d",   "Time given in days w.r.t. to MJDREF");
        WriteKey("MJDREF",   40587, "Store times in UNIX time (for convenience, seconds since 1970/1/1)");

        //WriteKey("CONTACT",   PACKAGE_BUGREPORT, "Current package maintainer");
        //WriteKey("URL",       PACKAGE_URL, "Current repositiory location");
    }
    catch (const CCfits::FitsException &e)
    {
        Error("CCfits::Table::addKey failed for '"+fTable->name()+"' in '"+fFile->name()+"': "+e.message());
        return false;
    }

    return true;
}

// --------------------------------------------------------------------------
//
//! Add a new column to the vectors storing the column data.
//! @param names the vector of string storing the columns names
//! @param types the vector of string storing the FITS data format
//! @param numElems the number of elements in this column
//! @param type the char describing the FITS data format
//! @param name the name of the particular column to be added.
//
void FitsFile::AddColumn(char type, const string &name, int numElems, const string &unit)
{
    fColNames.push_back(name);
    fColUnits.push_back(unit);

    ostringstream str;
    if (numElems != 1)
        str << numElems;

    switch (toupper(type))
    {
    case 'B': str << 'L'; break; // logical
    case 'C': str << 'B'; break; // byte
    case 'S': str << 'I'; break; // short
    case 'I': str << 'J'; break; // int
    case 'X': str << 'K'; break; // long long
    case 'F': str << 'E'; break; // float
    case 'D': str << 'D'; break; // double
    }

    fColTypes.push_back(str.str());
}

void FitsFile::AddColumn(const string &name, const string &format, const string &unit)
{
    fColNames.push_back(name);
    fColUnits.push_back(unit);
    fColTypes.push_back(format);
}

bool FitsFile::OpenFile(const string &filename, bool allow_open)
{
    if (fFile || fTable)
    {
        Error("FitsFile::OpenFile - File already open.");
        return false;
    }
    // fFileName = fileName;
    if (!allow_open && access(filename.c_str(), F_OK)==0)
    {
        Error("File '"+filename+"' already existing.");
        return false;
    }
    //create the FITS object
    try
    {
        fFile = new CCfits::FITS(filename, CCfits::RWmode::Write);
    }
    catch (CCfits::FitsException e)
    {
        Error("CCfits::FITS failed for '"+filename+"': "+e.message());
        return false;
    }
    /*
     "SIMPLE  =                    T / file does conform to FITS standard             "
     "BITPIX  =                    8 / number of bits per data pixel                  "
     "NAXIS   =                    0 / number of data axes                            "
     "EXTEND  =                    T / FITS dataset may contain extensions            "
     "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy"
     "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H "
     "END                                                                             ";
     */

    fIsOwner = true;

    return true;
}

void FitsFile::ResetColumns()
{
    fColNames.clear();
    fColTypes.clear();
    fColUnits.clear();
}

bool FitsFile::SetFile(CCfits::FITS *file)
{
    if (!file)
    {
        Error("Fits::SetFile failed: NULL argument.");
        return false;
    }

    if (fFile)
    {
        Error("Fits::SetFile failed: File already set.");
        return false;
    }

    fFile = file;
    fIsOwner = false;

    return true;
}

bool FitsFile::OpenTable(const string &tablename)
{
    if (!fFile)
    {
        Error("FitsFile::OpenTable - No file open.");
        return false;
    }
    if (fTable)
    {
        Error("FitsFile::OpenTable - Table already open.");
        return false;
    }

    //actually create the table
    CCfits::Table *table = 0;
    try
    {
        table = fFile->addTable(tablename, 0, fColNames, fColTypes, fColUnits);
    }
    catch (const CCfits::FitsException &e)
    {
        Error("CCfits::Table::addTable failed for '"+tablename+"' in '"+fFile->name()+"': "+e.message());
        return false;
    }

    if (table->rows() != 0)
    {
        Error("FITS table '"+tablename+"' created in '"+fFile->name()+"' on the fly looks non-empty.");
        return false;
    }

    // Set this as last - we use it for IsOpen()
    fTable = table;
    fNumRows = 0;

    return true;
}

// --------------------------------------------------------------------------
//
//! This looks for a suitable table in the fits file, i.e. that corresponds to the name and column names. (no format check yet)
//! @param tableName. the base table name to be obtained. If not suitable, numbers are appened to the name
//! @param allNames. the name of all columns
//! @param allDataTypes. the data types of all columns
//! @param allUnits. the units of the columns
//! @return a pointer to the newly retrieved/created table
//
bool FitsFile::OpenNewTable(const string &tableName, int maxtry)
{
    if (!fFile)
    {
        Error("FitsFile::OpenNewTable - No file open.");
        return false;
    }

    if (fTable)
    {
        Error("FitsFile::OpenNewTable - Table already open.");
        return false;
    }

    //first, let's check if the table already exist in the file
    fFile->read(vector<string>(1, tableName));

    // FIXME: Check for fFile and fTable
    const multimap<string, CCfits::ExtHDU *> &extMap = fFile->extension();

    for (int i=0; i<maxtry; i++)
    {
        //if (i==10)
        //    fMess->Warn("Already 10 different tables with different formats exist in this file. Please consider re-creating the file entirely (i.e. delete it please)");

        ostringstream str;
        str << tableName;
        if (i != 0)
            str << "-" << i;

        const string tname = str.str();

        const multimap<string,CCfits::ExtHDU*>::const_iterator it = extMap.find(tname);

        //current table name does not exist yet. return its associated fits table newly created
        if (it == extMap.end())
        {
            // What is this for?
            //for (multimap<string, CCfits::ExtHDU*>::const_iterator it=extMap.begin();
            //     it!= extMap.end(); it++)
            //    fMess->Debug(it->first);

            return OpenTable(tname);
        }

        CCfits::Table *table = dynamic_cast<CCfits::Table*>(it->second);

        // something wrong happened while getting the table pointer
        if (!table)
        {
            Error("HDU '"+tname+"' found in file, but it is not a proper CCfits::Table.");
            return false;
        }

        //now check that the table columns are the same
        //as the service columns
        table->makeThisCurrent();

        // FIXME: To be checked...
        /*
         const map<string, Column*> cMap = table->column();
         for (vector<string>::const_iterator ii=fFile->fColNames;
         ii!=fFile->fColNames.end(); ii++)
         if (cMap.find(*ii) == cMap.end())
         continue;
         */

        fNumRows = table->rows();

        // ----------- This is just a simple sanity check ----------

        // This is not necessary this is done already in
        // findSuitableTable (either directly or indirectly through OpenTable)
        // fFile->fTable->makeThisCurrent();

        //If the file already existed, then we must load its data to memory before writing to it.
        if (fNumRows>0)
        {
            CCfits::BinTable* bTable = dynamic_cast<CCfits::BinTable*>(table);
            if (!bTable)
            {
                Error("Table '"+tableName+"' found in '"+fFile->name()+"' is not a binary table.");
                return false;
            }

            //read the table binary data.
            vector<string> colName;
            bTable->readData(true, colName);

            // double check that the data was indeed read from the disk.
            // Go through the fTable instead as colName is empty (yes, it is !)
            const auto &cMap = table->column();

            //check that the existing columns are the same as the ones we want to write
            for (map<string, CCfits::Column*>::const_iterator mapIt = cMap.begin(); mapIt != cMap.end(); mapIt++)
            {
                bool found = false;
                for (unsigned int ii=0;ii<fColNames.size();ii++)
                {
                    if (mapIt->first == fColNames[ii])
                    {
                        found = true;
                        if (mapIt->second->format() != fColTypes[ii])
                        {
                            Error("Column "+fColNames[ii]+" has wrong format ("+fColTypes[ii]+" vs "+mapIt->second->format()+" in file)");
                            return false;
                        }
                    }
                }
                if (!found)
                {
                    Error("Column "+mapIt->first+" only exist in written file");
                    return false;
                }
            }
            //now we know that all the file's columns are requested. Let's do it the other way around
            for (unsigned int ii=0;ii<fColNames.size();ii++)
            {
                bool found = false;
                for (map<string, CCfits::Column*>::const_iterator mapIt = cMap.begin(); mapIt != cMap.end(); mapIt++)
                {
                    if (fColNames[ii] == mapIt->first)
                    {
                        found = true;
                        if (fColTypes[ii] != mapIt->second->format())
                        {
                            Error("Column "+fColNames[ii]+" has wrong format ("+fColTypes[ii]+" vs "+mapIt->second->format()+" in file)");
                            return false;
                        }
                    }
                }
                if (!found)
                {
                    Error("Column "+fColNames[ii]+" only exist in requested description");
                    return false;
                }
            }

            for (map<string,CCfits::Column*>::const_iterator cMapIt = cMap.begin();
                 cMapIt != cMap.end(); cMapIt++)
            {
                if (!cMapIt->second->isRead())
                {
                    Error("Reading column '"+cMapIt->first+"' back from '"+fFile->name()+"' failed.");
                    return false;
                }
            }
        }

        // Set this as last - we use it for IsOpen()
        fTable = table;

        return true;
    }

    ostringstream str;
    str << "FitsFile::OpenNewTable failed - more than " << maxtry << " tables tried." << endl;
    Error(str);

    return false;
}

bool FitsFile::AddRow()
{
    if (!fFile || !fTable)
    {
        Error("FitsFile::AddRow - No table open.");
        return false;
    }

    //insert a new row (1==number of rows to insert)
    int status(0);
    fits_insert_rows(fFile->fitsPointer(), fNumRows, 1, &status);

    // Status is also directly returned, but we need to give the
    // pointer anyway
    if (status)
    {
        char text[30];//max length of cfitsio error strings (from doc)
        fits_get_errstatus(status, text);

        ostringstream str;
        str << "Inserting row " << fNumRows << " failed in '"+fFile->name()+"': " << text << " (fits_insert_rows,rc=" << status << ")";
        Error(str);

        return false;
    }

    fNumRows++;
    fCursor = 1;

    return true;
}

bool FitsFile::WriteData(size_t &start, const void *ptr, size_t size)
{
    if (!fFile || !fTable)
    {
        Error("FitsFile::AddRow - No table open.");
        return false;
    }

    int status = 0;
    fits_write_tblbytes(fFile->fitsPointer(), fNumRows, start, size,
                        (unsigned char*)ptr, &status);

    // Status is also directly returned, but we need to give the
    // pointer anyway
    if (status)
    {
        char text[30];//max length of cfitsio error strings (from doc)
        fits_get_errstatus(status, text);

        ostringstream str;
        str << "Writing row " << fNumRows << " failed in '"+fFile->name()+"': " << text << " (file_write_tblbytes,rc=" << status << ")";
        Error(str);
    }

    start += size;
    return status==0;
}

void FitsFile::Close()
{
    if (!fFile)
        return;

    if (fIsOwner)
    {
        const string name = fFile->name();
        delete fFile;
    }

    //WARNING: do NOT delete the table as it gets deleted by the
    // fFile object
    fFile = NULL;
    fTable = NULL;
}

void FitsFile::Flush()
{
    if (!fFile)
        return;

    int status = 0;
    fits_flush_file(fFile->fitsPointer(), &status);

    if (status)
    {
        char text[30];
        fits_get_errstatus(status, text);

        ostringstream str;
        str << "Flushing file " << fFile->name() << " failed: " << text << " (fits_flush_file, rc=" << status << ")";
        Error(str);
    }
}
size_t FitsFile::GetDataSize() const
{
    size_t size = 0;

    for (vector<string>::const_iterator it=fColTypes.begin();
         it!=fColTypes.end(); it++)
    {
        size_t id=0;

        int n=1;
        try { n = stoi(*it, &id); }
        catch (const exception&) { }

        if (n==0)
            continue;

        switch ((*it)[id])
        {
        case 'L':
        case 'A': size += n*1; break; // ascii
        case 'B': size += n*1; break; // logical/byte
        case 'I': size += n*2; break; // short
        case 'J': size += n*4; break; // int
        case 'K': size += n*8; break; // long long
        case 'E': size += n*4; break; // float
        case 'D': size += n*8; break; // double
        default:
            throw runtime_error("FitsFile::GetDataSize - id not known.");
        }
    }

    return size;
}
