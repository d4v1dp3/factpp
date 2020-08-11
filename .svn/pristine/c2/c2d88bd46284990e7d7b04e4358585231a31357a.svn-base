#ifndef FACT_Fits
#define FACT_Fits

#include "Description.h"
#include "FitsFile.h"

class Converter;

using namespace std;

class Fits
{
private:
    FitsFile *fFile;
    string fFileName;

    ///Name of the "standard", i.e. data found in every fits file
    ///TODO make these variable static so that they are shared by every object.
    ///TODO add also a static boolean to initialize these only once
    vector<Description> fStandardColDesc;
    ///Format of the standard columns.
    vector<string> fStandardFormats;
    ///the pointers to the standard variables
    vector<void*> fStandardPointers;
    ///the number of bytes taken by each standard variable
    vector<int> fStandardNumBytes;

    ///the vector of data column names
    vector<Description> fDataColDesc;
    //Description of the table
    string fTableDesc;
    ///the data format of the data columns
    vector<string> fDataFormats;

    ///the copy buffer. Required to put the standard and data variable in contguous memory
    vector<char> fCopyBuffer;
    ///to keep track of the time of the latest written entry (to update the header when closing the file)
    double fEndMjD;
    ///Keep track of number of opened fits
    uint32_t* fNumOpenFitsFiles;
    ///were to log the errors
    MessageImp* fMess;

    ///Write the FITS header keys
    bool WriteHeaderKeys();
    //if a write error occurs
    void MoveFileToCorruptedFile();



public:
    ///current run number being logged
    int32_t fRunNumber;

    Fits() : fFile(NULL),
        fEndMjD(0.0),
        fNumOpenFitsFiles(NULL),
        fMess(NULL),
        fRunNumber(0)
    {}

    virtual ~Fits()
    {
        Close();
    }

    ///returns wether or not the file is currently open or not
    bool IsOpen() const { return fFile != NULL && fFile->IsOpen(); }

    ///Adds a column that exists in all FITS files
    void AddStandardColumn(const Description& desc, const string &dataFormat, void* dataPointer, long unsigned int numDataBytes);

    ///Adds columns specific to the service being logged.
    void InitDataColumns(const vector<Description> &desc, const vector<string>& dataFormat, MessageImp* out);

    ///Opens a FITS file
    bool Open(const string& fileName, const string& tableName,  uint32_t* fitsCounter, MessageImp* out, int runNumber, CCfits::FITS *file=0);//ostream& out);

    ///Write one line of data. Use the given converter.
    bool Write(const Converter &conv, const void* data);

    ///Close the currently opened file.
    void Close();

    ///Flush the currently opened file to disk.
    void Flush();

    ///Get the size currently written on the disk
    int GetWrittenSize() const;

    string GetName() const { return fFile ? fFile->GetName() : ""; }

};//Fits


#endif /*FITS_H_*/

// WriteToFITS vs Open/Close
