// **************************************************************************
/** @class FactFits

@brief FITS writter for the FACT project. 

The FactFits class is able to open, manage and update FITS files. 

The file columns should be given to the class before the file is openned. Once
a file has been created, the structure of its columns cannot be changed. Only
row can be added.

This class relies on the CCfits and CFitsIO packages.

*/
// **************************************************************************
#include "Fits.h"

#include "Time.h"
#include "Converter.h"
#include "MessageImp.h"

#include <sys/stat.h> //for file stats
#include <cstdio> // for file rename
#include <cerrno>

#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace CCfits;

// --------------------------------------------------------------------------
//
//! This gives a standard variable to the file writter. 
//! This variable should not be related to the DIM service being logged. 
//! @param desc the description of the variable to add
//! @param dataFormat the FITS data format corresponding to the variable to add.
//! @param dataPointer the memory location where the variable is stored
//! @param numDataBytes the number of bytes taken by the variable
//
void Fits::AddStandardColumn(const Description& desc, const string &dataFormat, void* dataPointer, long unsigned int numDataBytes)
{
    //check if entry already exist
    for (vector<Description>::const_iterator it=fStandardColDesc.begin(); it != fStandardColDesc.end(); it++)
        if (it->name == desc.name)
            return;

    fStandardColDesc.push_back(desc);
    fStandardFormats.push_back(dataFormat);
    fStandardPointers.push_back(dataPointer);
    fStandardNumBytes.push_back(numDataBytes);
}

// --------------------------------------------------------------------------
//
//! This gives the file writter access to the DIM data
//! @param desc a vector containing the description of all the columns to log
//! @param dataFormat a vector containing the FITS data format of all the columsn to log
//! @param dataPointer the memory location where the DIM data starts
//! @param numDataBytes the number of bytes taken by the DIM data. 
//! @param out Message object to use for propagating messages
//	
void Fits::InitDataColumns(const vector<Description> &desc, const vector<string>& dataFormat, MessageImp* out)
{
    fDataFormats = dataFormat;

    if ((desc.size() == 0) && (dataFormat.size() == 0))
    {
        fDataColDesc.clear();
        return;
    }

    //we will copy this information here. It duplicates the data, which is not great,
    // but it is the easiest way of doing it right now
    if (
        (desc.size() == dataFormat.size()+1) || // regular service
        (desc.size() == dataFormat.size()+2)    // service with ending string. skipped in fits
       )
    {
        //services have one (or two) more description than columns. skip the first entry while copying as it describes the table itself.

        fDataColDesc.clear();

        fTableDesc = desc[0].comment;
        if (fTableDesc.size() > 68)
        {
            out->Warn("Table description '" + fTableDesc + "' exceeds 68 chars... truncated.");
            fTableDesc = fTableDesc.substr(0,68);
        }

        for (unsigned int i=0; i<dataFormat.size(); i++)
        {
            string name = desc[i+1].name;
            if (name.length() > 68)
            {
                out->Warn("Column name '" + name + "' exceeds 68 chars... truncated.");
                name = name.substr(0, 68);
            }

            string comment = desc[i+1].comment;
            if (comment.length() + name.length() > 71)
            {
                out->Warn("Column '" + name + " / " + comment + "' exceeds 68 chars... truncated.");
                comment = comment.substr(0,68);
            }

            string unit = desc[i+1].unit;
            if (unit.length() > 68)
            {
                out->Warn("Unit '" + name + "' exceeds 68 chars... truncated.");
                unit = comment.substr(0,68);
            }

            const size_t p = fDataFormats[i].find_last_of('B');
            if ((boost::iequals(unit, "text") || boost::iequals(unit, "string")) && p!=string::npos)
            {
                out->Info("Column '" + name + "' detected to be an ascii string (FITS format 'A').");
                fDataFormats[i].replace(p, 1, "A");
            }

            fDataColDesc.push_back(Description(name, comment, unit));
        }
        return;
    }

    {//if we arrived here, this means that the columns descriptions could not be parsed
        ostringstream str;
        str << "Expected " << dataFormat.size() << " descriptions of columns, got " << (int)(desc.size())-1 << " for service: ";
        if (desc.size() > 0)
            str << desc[0].name;
        else
            str << "<unknown>";

        out->Warn(str.str());
    }

    fDataColDesc.clear();
 //   fDataColDesc.push_back(Description("service", "comment", "unit"));
    for (unsigned int i=0;i<dataFormat.size();i++)
    {
        ostringstream stt;
        stt << "Data" << i;
        fDataColDesc.push_back(Description(stt.str(), "", ""));
    }
}

// --------------------------------------------------------------------------
//
//! This opens the FITS file (after the columns have been passed)
//! @param fileName the filename with complete or relative path of the file to open
//! @param tableName the name of the table that will receive the logged data.
//! @param file a pointer to an existing FITS file. If NULL, file will be opened and managed internally
//! @param fitsCounter a pointer to the integer keeping track of the opened FITS files
//! @param out a pointer to the MessageImp that should be used to log errors
//! @param runNumber the runNumber for which this file is opened. 0 means nightly file.
//
bool Fits::Open(const string& fileName, const string& tableName, uint32_t* fitsCounter, MessageImp* out, int runNumber, FITS* file)
{
    fRunNumber = runNumber;
    fMess = out;
    fFileName = fileName;

    if (fFile)
    {
        fMess->Error(fileName+" already open...");
        return false;
    }

    fFile = new FitsFile(*fMess);

    if (file == NULL)
    {
        if (!fFile->OpenFile(fileName, true))
            return false;

        fNumOpenFitsFiles = fitsCounter;
        (*fNumOpenFitsFiles)++;
    }
    else
    {
        if (!fFile->SetFile(file))
            return false;
    }

    //concatenate the standard and data columns
    //do it the inneficient way first: its easier and faster to code.
    for (unsigned int i=0;i<fStandardColDesc.size();i++)
    {
        fFile->AddColumn(fStandardColDesc[i].name, fStandardFormats[i],
                         fStandardColDesc[i].unit);
    }

    for (unsigned int i=0; i<fDataColDesc.size(); i++)
    {
        string name = fDataColDesc[i].name;
        if (name.empty())
        {
            ostringstream stt;
            stt << "Data" << i;
            name = stt.str();
        }
//cout << endl << "#####adding column: " << name << " " << fDataFormats[i] << " " << fDataColDesc[i].unit << endl << endl;
        fFile->AddColumn(name, fDataFormats[i], fDataColDesc[i].unit);
    }

    try
    {
        if (!fFile->OpenNewTable(tableName, 100))
        {
            Close();
            //if the file already exist, then the column names must have changed
            //let's move the file and try to open it again.
            string fileNameWithoutFits = fFileName.substr(0, fileName.size()-4);
            int counter = 0;
            while (counter < 100)
            {
                ostringstream newFileName;
                newFileName << fileNameWithoutFits << counter << ".fits";
                ifstream testStream(newFileName.str().c_str());
                if (!testStream)
                {
                    if (rename(fFileName.c_str(), newFileName.str().c_str()))
                        return false;
                    break;
                }
                counter++;
            }
            if (counter == 100)
                return false;
            //now we open it again.
            fFile = new FitsFile(*fMess);
            if (file == NULL)
            {
                if (!fFile->OpenFile(fileName, true))
                    return false;
                fNumOpenFitsFiles = fitsCounter;
                (*fNumOpenFitsFiles)++;
            }
            else
            {
                if (!fFile->SetFile(file))
                    return false;
            }
            //YES, we must also redo that thing here...
            //concatenate the standard and data columns
            //do it the inneficient way first: its easier and faster to code.
            for (unsigned int i=0;i<fStandardColDesc.size();i++)
            {
                fFile->AddColumn(fStandardColDesc[i].name, fStandardFormats[i],
                                 fStandardColDesc[i].unit);
            }

            for (unsigned int i=0; i<fDataColDesc.size(); i++)
            {
                string name = fDataColDesc[i].name;
                if (name.empty())
                {
                    ostringstream stt;
                    stt << "Data" << i;
                    name = stt.str();
                }
        //cout << endl << "#####adding column: " << name << " " << fDataFormats[i] << " " << fDataColDesc[i].unit << endl << endl;
                fFile->AddColumn(name, fDataFormats[i], fDataColDesc[i].unit);
            }
            if (!fFile->OpenNewTable(tableName, 100))
            {
                Close();
                return false;
            }
        }

        fCopyBuffer.resize(fFile->GetDataSize());
//write header comments

        ostringstream str;
        for (unsigned int i=0;i<fStandardColDesc.size();i++)
        {
            str.str("");
            str << "TTYPE" << i+1;
            fFile->WriteKeyNT(str.str(), fStandardColDesc[i].name, fStandardColDesc[i].comment);
            str.str("");
            str << "TCOMM" << i+1;
            fFile->WriteKeyNT(str.str(), fStandardColDesc[i].comment, "");
        }

        for (unsigned int i=0; i<fDataColDesc.size(); i++)
        {
            string name = fDataColDesc[i].name;
            if (name.empty())
            {
                ostringstream stt;
                stt << "Data" << i;
                name = stt.str();
            }
            str.str("");
            str << "TTYPE" << i+fStandardColDesc.size()+1;
            fFile->WriteKeyNT(str.str(), name, fDataColDesc[i].comment);
            str.str("");
            str << "TCOMM" << i+fStandardColDesc.size()+1;
            fFile->WriteKeyNT(str.str(), fDataColDesc[i].comment, "");
        }

        fFile->WriteKeyNT("COMMENT", fTableDesc, "");

        if (fFile->GetNumRows() == 0)
        {//if new file, then write header keys -> reset fEndMjD used as flag
            fEndMjD = 0;
        }
        else
        {//file is beingn updated. Prevent from overriding header keys
            fEndMjD = Time().Mjd();
        }

        return fFile->GetNumRows()==0 ? WriteHeaderKeys() : true;
    }
    catch (const CCfits::FitsException &e)
    {
        cout << "Exception !" << endl;
        fMess->Error("Opening or creating table '"+tableName+"' in '"+fileName+"': "+e.message());

        fFile->fTable = NULL;
        Close();
        return false;
    }
}

// --------------------------------------------------------------------------
//
//! This writes the standard header 
//
bool Fits::WriteHeaderKeys()
{
    if (!fFile->fTable)
        return false;

    if (!fFile->WriteDefaultKeys("datalogger"))
        return false;

    if (!fFile->WriteKeyNT("TSTARTI",  0, "Time when first event received (integral part)")   ||
        !fFile->WriteKeyNT("TSTARTF",  0, "Time when first event received (fractional part)") ||
        !fFile->WriteKeyNT("TSTOPI",   0, "Time when last event received (integral part)")    ||
        !fFile->WriteKeyNT("TSTOPF",   0, "Time when last event received (fractional part)")  ||
        !fFile->WriteKeyNT("DATE-OBS", 0, "Time when first event received") ||
        !fFile->WriteKeyNT("DATE-END", 0, "Time when last event received") ||
        !fFile->WriteKeyNT("RUNID", fRunNumber, "Run number. 0 means not run file"))
        return false;

    return true;
}
void Fits::MoveFileToCorruptedFile()
{
    ostringstream corruptName;
    struct stat st;
    int append = 0;
    corruptName << fFileName << "corrupt" << append;
    while (!stat(corruptName.str().c_str(), &st))
    {
        append++;
        corruptName.str("");
        corruptName << fFileName << "corrupt" << append;
    }
    if (rename(fFileName.c_str(), corruptName.str().c_str()) != 0)
    {
        ostringstream str;
        str << "rename() failed for '" << fFileName << "': " << strerror(errno) << " [errno=" << errno << "]";
        fMess->Error(str);
        return;
    }

    fMess->Message("Renamed file " + fFileName + " to " + corruptName.str());

}
// --------------------------------------------------------------------------
//
//! This writes one line of data to the file.
//! @param conv the converter corresponding to the service being logged
//
bool Fits::Write(const Converter &conv, const void* data)
{
    //first copy the standard variables to the copy buffer
    int shift = 0;
    for (unsigned int i=0;i<fStandardNumBytes.size();i++)
    {
        const char *charSrc = reinterpret_cast<char*>(fStandardPointers[i]);
        reverse_copy(charSrc, charSrc+fStandardNumBytes[i], fCopyBuffer.data()+shift);
        shift += fStandardNumBytes[i];
    }
    try
    {
        //now take care of the DIM data. The Converter is here for that purpose
        conv.ToFits(fCopyBuffer.data()+shift, data, fCopyBuffer.size()-shift);
    }
    catch (const runtime_error &e)
    {
        ostringstream str;
        str << fFile->GetName() << ": " << e.what();
        fMess->Error(str);
        return false;
    }

    // This is not necessary, is it?
    // fFile->fTable->makeThisCurrent();
    if (!fFile->AddRow())
    {
        Close();
        MoveFileToCorruptedFile();
        return false;
    }
    if (!fFile->WriteData(fCopyBuffer))
    {
        Close();
        return false;
    }
    const double tm = *reinterpret_cast<double*>(fStandardPointers[0]);

    //the first standard variable is the current MjD
    if (fEndMjD==0)
    {
        // FIXME: Check error?
        fFile->WriteKeyNT("TSTARTI", uint32_t(floor(tm)),      "Time when first event received (integral part)");
        fFile->WriteKeyNT("TSTARTF", fmod(tm, 1),              "Time when first event received (fractional part)");
        fFile->WriteKeyNT("TSTOPI",  uint32_t(floor(fEndMjD)), "Time when last event received (integral part)");
        fFile->WriteKeyNT("TSTOPF",  fmod(fEndMjD, 1),         "Time when last event received (fractional part)");

        fFile->WriteKeyNT("DATE-OBS", Time(tm+40587).Iso(),
                          "Time when first event received");

        fFile->WriteKeyNT("DATE-END", Time(fEndMjD+40587).Iso(),
                          "Time when last event received");
    }

    fEndMjD = tm;

    return true;
}

// --------------------------------------------------------------------------
//
//! This closes the currently openned FITS file. 
//! it also updates the header to reflect the time of the last logged row
//	
void Fits::Close() 
{
    if (!fFile)
        return;
    if (fFile->IsOpen() && fFile->IsOwner())
    {
        // FIMXE: Check for error? (It is allowed that fFile is NULL)
        fFile->WriteKeyNT("TSTOPI", uint32_t(floor(fEndMjD)), "Time when last event received (integral part)");
        fFile->WriteKeyNT("TSTOPF", fmod(fEndMjD, 1),         "Time when last event received (fractional part)");

        fFile->WriteKeyNT("DATE-END", Time(fEndMjD+40587).Iso(),
                          "Time when last event received");
    }
    if (fFile->IsOwner())
    {
        if (fNumOpenFitsFiles != NULL)
            (*fNumOpenFitsFiles)--;
    }
    const string name = fFile->GetName();
    delete fFile;
    fFile = NULL;
    fMess->Info("Closed: "+name);
//    fMess = NULL;
}

void Fits::Flush()
{
    if (!fFile)
        return;

    fFile->Flush();
}
// --------------------------------------------------------------------------
//! Returns the size on the disk of the Fits file being written.
int Fits::GetWrittenSize() const
{
    if (!IsOpen())
        return 0;

    struct stat st;
    if (stat(fFile->GetName().c_str(), &st))
        return 0;

    return st.st_size;
}

/*
 To be done:
 - Check the check for column names in opennewtable
 - If Open return false we end in an infinite loop (at least if
   the dynamic cats to Bintable fails.

*/
