//****************************************************************
/** @class FitsLoader

  @brief Load a given Fits file and table, and dump selected columns if requested.

  It derives from StateMachineDim. the first parent is here to enforce
  a state machine behaviour
  The possible states and transitions of the machine are:
  \dot
  digraph FitsLoader {
          node [shape=record, fontname=Helvetica, fontsize=10];
      e [label="Error" color="red"];
   r [label="Ready"]
   d [label="FileLoaded"]

  e -> r
  r -> e
  r -> d
  d -> r
   }
  \enddot
 */
 //****************************************************************
#include "Event.h"
#include "StateMachineDim.h"
#include "WindowLog.h"
#include "Configuration.h"
#include "LocalControl.h"
#include "Description.h"


#include <boost/bind.hpp>
#if BOOST_VERSION < 104400
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#undef BOOST_HAS_RVALUE_REFS
#endif
#endif
#include <boost/thread.hpp>

#include <iostream>

#include <CCfits/CCfits>

class FitsLoader : public StateMachineDim
{

public:
    enum
    {
        kSM_FileLoaded = 20,
    } localstates_t;

    FitsLoader(ostream& out);
    ~FitsLoader();

    ///Define command names
    static const char* fLoadFits;
    static const char* fUnloadFits;
    static const char* fListColumns;
    static const char* fDumpColumns;
    static const char* fClearDumpList;
    static const char* fDoDump;
    static const char* fConfigFileName;
    static const char* fConfigTableName;
    static const char* fConfigPrecName;
    static const char* fConfigFileOutName;

private:
    ///Name of the fits file to load
    string fFileName;
    ///Name of the table to load from the  file
    string fTableName;
    ///FITS pointer
    CCfits::FITS* fFile;
    ///Table pointer
    CCfits::Table* fTable;
    ///Precision of the ofstream. Used to output a given number of significant digits for floats or doubles
    int fStreamPrecision;
    ///Name of the output file
    string fFileOut;
    ///map between the column names and their CCfits objects
    map<string, CCfits::Column*> fColMap;
    ///List of the column names to be dumped
    vector<string> fDumpList;
    ///Transition from ready to fileLoaded.
    int LoadPlease();
    ///Transition from fileLoaded to ready
    int UnloadPlease();
    ///Lists the loaded column names
    int ListColumnsPlease(const Event&);
    ///Add a column name to the dump list
    int AddDumpColumnsPlease(const Event&);
    ///Clear the dump list
    int ClearDumpListPlease(const Event&);
    ///Perform the dumping, based on the current dump list
    int DoDumpPlease(const Event&);
    ///Set the name of the Fits file to be loaded
    int ConfigFileNamePlease(const Event&);
    ///Set the name of the table to be loaded
    int ConfigTableNamePlease(const Event&);
    ///Set the ofstream precision
    int SetOFStreamPrecisionPlease(const Event&);
    ///Set the name of the output file
    int SetFileOutPlease(const Event&);
    ///Calculate the buffer size required to read a row of the fits table, as well as the offsets to each column
    vector<int> CalculateBufferSize();
    ///Write a single row of the selected data
    void writeValuesFromFits(vector<int>& offsets,ofstream& targetFile, unsigned char* fitsBuffer);

public:
    ///Configures the fitsLoader from the config file and/or command arguments.
    void SetupConfig(Configuration& conf);
};

const char* FitsLoader::fLoadFits = "load";
const char* FitsLoader::fUnloadFits = "unload";
const char* FitsLoader::fListColumns = "list_columns";
const char* FitsLoader::fDumpColumns = "add_dump";
const char* FitsLoader::fClearDumpList = "clear_dump";
const char* FitsLoader::fDoDump = "dump";
const char* FitsLoader::fConfigFileName = "set_file";
const char* FitsLoader::fConfigTableName = "set_table";
const char* FitsLoader::fConfigPrecName = "set_prec";
const char* FitsLoader::fConfigFileOutName = "set_outfile";

// --------------------------------------------------------------------------
//
//! Set the name of the output file
//! @param evt
//!        the event transporting the file name
//
int FitsLoader::SetFileOutPlease(const Event& evt)
{
    fFileOut = evt.GetText();
    ostringstream str;
    str << "Output file is now " << fFileOut;
    Message(str);
    return 0;
}
// --------------------------------------------------------------------------
//
//! Set the precision of the ofstream. So that an appropriate number of significant digits are outputted.
//! @param evt
//!        the event transporting the precision
//
int FitsLoader::SetOFStreamPrecisionPlease(const Event& evt)
{
    fStreamPrecision = evt.GetInt();
    ostringstream str;
    str << "ofstream precision is now " << fStreamPrecision;
    Message(str);
    return 0;
}
// --------------------------------------------------------------------------
//
//! Writes a single row of the selected FITS data to the output file.
//! @param offsets
//!         a vector containing the offsets to the columns (in bytes)
//! @param targetFile
//!         the ofstream where to write to
//! @param fitsBuffer
//!         the memory were the row has been loaded by cfitsio
//
void FitsLoader::writeValuesFromFits(vector<int>& offsets,ofstream& targetFile, unsigned char* fitsBuffer)
{
    targetFile.precision(fStreamPrecision);
    map<string, CCfits::Column*>::iterator it;
   for (it=fColMap.begin(); it != fColMap.end(); it++)
    {
        bool found = false;
        for (vector<string>::iterator jt=fDumpList.begin(); jt != fDumpList.end(); jt++)
        {
            if (it->first == *jt)
            {
                found = true;
                break;
            }
        }
        if (!found)
            continue;
       int offset = offsets[it->second->index()-1];
       const char* charSrc = reinterpret_cast<char*>(&fitsBuffer[offset]);
        unsigned char copyBuffer[30];//max size of a single variable
        for (int width = 0; width<it->second->width(); width++)
        {
            switch (it->second->type())
            {
            case CCfits::Tbyte:
                targetFile << *charSrc;
                charSrc += sizeof(char);
            break;
            case CCfits::Tushort:
                targetFile << *reinterpret_cast<const unsigned short*>(charSrc);
                charSrc += sizeof(char);
            break;
            case CCfits::Tshort:
                targetFile << *reinterpret_cast<const short*>(charSrc);
                charSrc += sizeof(char);
            break;
            case CCfits::Tuint:
                reverse_copy(charSrc, charSrc+sizeof(unsigned int), copyBuffer);
                //warning suppressed in gcc4.0.2
                targetFile << *reinterpret_cast<unsigned int*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tint:
                reverse_copy(charSrc, charSrc+sizeof(int), copyBuffer);
                targetFile << *reinterpret_cast<int*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tulong:
                reverse_copy(charSrc, charSrc+sizeof(unsigned long), copyBuffer);
                targetFile << *reinterpret_cast<unsigned long*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tlong:
                reverse_copy(charSrc, charSrc+sizeof(long), copyBuffer);
                targetFile << *reinterpret_cast<long*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tlonglong:
                reverse_copy(charSrc, charSrc+sizeof(long long), copyBuffer);
                targetFile << *reinterpret_cast<long long*>(copyBuffer);
                charSrc += sizeof(long long);
            break;
            case CCfits::Tfloat:
                reverse_copy(charSrc, charSrc+sizeof(float), copyBuffer);
                targetFile << *reinterpret_cast<float*>(copyBuffer);
                charSrc += sizeof(float);
            break;
            case CCfits::Tdouble:
                reverse_copy(charSrc, charSrc+sizeof(double), copyBuffer);
                targetFile << *reinterpret_cast<double*>(copyBuffer);
                charSrc += sizeof(double);
            break;
            case CCfits::Tnull:
            case CCfits::Tbit:
            case CCfits::Tlogical:
            case CCfits::Tstring:
            case CCfits::Tcomplex:
            case CCfits::Tdblcomplex:
            case CCfits::VTbit:
            case CCfits::VTbyte:
            case CCfits::VTlogical:
            case CCfits::VTushort:
            case CCfits::VTshort:
            case CCfits::VTuint:
            case CCfits::VTint:
            case CCfits::VTulong:
            case CCfits::VTlong:
            case CCfits::VTlonglong:
            case CCfits::VTfloat:
            case CCfits::VTdouble:
            case CCfits::VTcomplex:
            case CCfits::VTdblcomplex:
                Error("Data type not implemented yet.");
                return;
            break;
            default:
                Error("THIS SHOULD NEVER BE REACHED");
                return;
            }//switch
            targetFile << " ";
        }//width loop
    }//iterator over the columns
    targetFile << endl;
}

// --------------------------------------------------------------------------
//
//! Calculates the required buffer size for reading one row of the current table.
//! Also calculates the offsets to all the columns
//
vector<int> FitsLoader::CalculateBufferSize()
{
    vector<int> result;
    map<int,int> sizes;
    int size = 0;

    for (map<string, CCfits::Column*>::iterator it=fColMap.begin(); it != fColMap.end(); it++)
    {
        int width = it->second->width();
        switch (it->second->type())
        {
        case CCfits::Tbyte:
        case CCfits::Tushort:
        case CCfits::Tshort:
            Message("short");
            sizes[it->second->index()] =  sizeof(char)*width;
        break;
        case CCfits::Tuint:
        case CCfits::Tint:
            Message("int");
            sizes[it->second->index()] =  sizeof(int)*width;
        break;
        case CCfits::Tulong:
        case CCfits::Tlong:
            Message("long");
            sizes[it->second->index()] = sizeof(int)*width;
        break;
        case CCfits::Tlonglong:
            Message("longlong");
            sizes[it->second->index()] =  sizeof(long long)*width;
        break;
        case CCfits::Tfloat:
            Message("float");
            sizes[it->second->index()] =  sizeof(float)*width;
        break;
        case CCfits::Tdouble:
            Message("double");
            sizes[it->second->index()] =  sizeof(double)*width;
        break;
        case CCfits::Tnull:
        case CCfits::Tbit:
        case CCfits::Tlogical:
        case CCfits::Tstring:
        case CCfits::Tcomplex:
        case CCfits::Tdblcomplex:
        case CCfits::VTbit:
        case CCfits::VTbyte:
        case CCfits::VTlogical:
        case CCfits::VTushort:
        case CCfits::VTshort:
        case CCfits::VTuint:
        case CCfits::VTint:
        case CCfits::VTulong:
        case CCfits::VTlong:
        case CCfits::VTlonglong:
        case CCfits::VTfloat:
        case CCfits::VTdouble:
        case CCfits::VTcomplex:
        case CCfits::VTdblcomplex:
            Error("Data type not implemented yet.");
            return vector<int>();
        break;
        default:
            Error("THIS SHOULD NEVER BE REACHED");
            return vector<int>();
        }
    }
    //calculate the offsets in the vector.
    int checkIndex = 1;
    for (map<int,int>::iterator it=sizes.begin(); it != sizes.end(); it++)
    {
        result.push_back(size);
        size += it->second;
        if (it->first != checkIndex)
        {
            ostringstream str;
            str << "Expected index " << checkIndex << " found " << it->first;
            Error(str);
        }
        checkIndex++;
    }
    result.push_back(size);
    return result;
}
// --------------------------------------------------------------------------
//
//! Constructor
//! @param out
//!        the ostream where to redirect the outputs
//
FitsLoader::FitsLoader(ostream& out) : StateMachineDim(out, "FITS_LOADER")
{
    //Add the existing states
    AddStateName(kSM_FileLoaded,  "FileLoaded", "A Fits file has been loaded");

    //Add the possible transitions
    AddEvent(kSM_FileLoaded, fLoadFits, kSM_Ready)
            (boost::bind(&FitsLoader::LoadPlease, this))
            ("Loads the given Fits file");
    AddEvent(kSM_Ready, fUnloadFits, kSM_FileLoaded)
            (boost::bind(&FitsLoader::UnloadPlease, this))
            ("Unloads the given Fits file");

    //Add the possible configurations
    AddEvent(fListColumns, "", kSM_FileLoaded)
            (boost::bind(&FitsLoader::ListColumnsPlease, this, _1))
            ("List the columns that were loaded from that file");
    AddEvent(fDumpColumns, "C", kSM_FileLoaded)
            (boost::bind(&FitsLoader::AddDumpColumnsPlease, this, _1))
            ("Add a given column to the dumping list");
    AddEvent(fClearDumpList, "", kSM_FileLoaded)
            (boost::bind(&FitsLoader::ClearDumpListPlease, this, _1))
            ("Clear the dumping list");
    AddEvent(fDoDump, "", kSM_FileLoaded)
            (boost::bind(&FitsLoader::DoDumpPlease, this, _1))
            ("Perform the dump of columns data, based on the to dump list");
    AddEvent(fConfigFileName, "C", kSM_Ready, kSM_FileLoaded)
            (boost::bind(&FitsLoader::ConfigFileNamePlease, this, _1))
            ("Gives the name of the Fits file to be loaded");
    AddEvent(fConfigTableName, "C", kSM_Ready, kSM_FileLoaded)
            (boost::bind(&FitsLoader::ConfigTableNamePlease, this, _1))
            ("Gives the name of the Table to be loaded");
    AddEvent(fConfigPrecName, "I", kSM_Ready, kSM_FileLoaded)
            (boost::bind(&FitsLoader::SetOFStreamPrecisionPlease, this, _1))
            ("Set the precision of the ofstream, i.e. the number of significant digits being outputted");
    AddEvent(fConfigFileOutName, "C", kSM_Ready, kSM_FileLoaded)
            (boost::bind(&FitsLoader::SetFileOutPlease, this, _1))
            ("Set the name of the outputted file.");

    fFile = NULL;
    fStreamPrecision = 20;

}
// --------------------------------------------------------------------------
//
//! Destructor
//
FitsLoader::~FitsLoader()
{
    if (fFile)
        delete fFile;
    fFile = NULL;
}
// --------------------------------------------------------------------------
//
//! Loads the fits file based on the current parameters
//
int FitsLoader::LoadPlease()
{
    ostringstream str;
    try
    {
        fFile = new CCfits::FITS(fFileName);
    }
    catch (CCfits::FitsException e)
     {
         str << "Could not open FITS file " << fFileName << " reason: " << e.message();
         Error(str);
         return kSM_Ready;
     }
    str.str("");
    const multimap< string, CCfits::ExtHDU * > extMap = fFile->extension();
    if (extMap.find(fTableName) == extMap.end())
    {
        str.str("");
        str << "Could not open table " << fTableName << ". Tables in file are: ";
        for (std::multimap<string, CCfits::ExtHDU*>::const_iterator it=extMap.begin(); it != extMap.end(); it++)
            str << it->first << " ";
        Error(str);
        return kSM_Ready;
    }
    else
        fTable = dynamic_cast<CCfits::Table*>(extMap.find(fTableName)->second);
    int numRows = fTable->rows();
    str.str("");
    str << "Loaded table has " << numRows << " rows";
    Message(str);

    fColMap = fTable->column();
    if (fDumpList.size() != 0)
    {
        bool should_clear = false;
        for (vector<string>::iterator it=fDumpList.begin(); it!= fDumpList.end(); it++)
        {
            if (fColMap.find(*it) == fColMap.end())
            {
                should_clear = true;
                Error("Config-given dump list contains invalid entry " + *it + " clearing the list");
            }
        }
        if (should_clear)
            fDumpList.clear();
    }
    return kSM_FileLoaded;
}
// --------------------------------------------------------------------------
//
//! Unloads the Fits file
//
int FitsLoader::UnloadPlease()
{
    if (fFile)
        delete fFile;
    else
        Error("Error: Fits file is  NULL while it should not have been");
    fFile = NULL;
    return kSM_Ready;
}
// --------------------------------------------------------------------------
//
//! List the columns that are in the loaded Fits table
//
int FitsLoader::ListColumnsPlease(const Event&)
{
    Message("Columns in the loaded table are:");
    map<string, CCfits::Column*>::iterator it;
    for (it=fColMap.begin(); it != fColMap.end(); it++)
        Message(it->first);
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Add a given column name  to the list of columns to dump
//! @param evt
//!        the event transporting the column name
//
int FitsLoader::AddDumpColumnsPlease(const Event& evt)
{
    string evtText(evt.GetText());
    //TODO check that this column indeed exist in the file
    if (fColMap.find(evtText) != fColMap.end())
        fDumpList.push_back(evtText);
    else
        Error("Could not find column " + evtText + " int table");
    Message("New dump list:");
    for (vector<string>::iterator it=fDumpList.begin(); it != fDumpList.end(); it++)
        Message(*it);
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Clear the list of columns to dump
//
int FitsLoader::ClearDumpListPlease(const Event&)
{
    fDumpList.clear();
    Message("Dump list is now empty");
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Perform the actual dump, based on the current parameters
//
int FitsLoader::DoDumpPlease(const Event&)
{
    fTable->makeThisCurrent();
    vector<int> offsets = CalculateBufferSize();
    int size = offsets[offsets.size()-1];
    offsets.pop_back();
    unsigned char* fitsBuffer = new unsigned char[size];

    ofstream targetFile(fFileOut);
    int status = 0;

    for (int i=1;i<=fTable->rows(); i++)
    {
        fits_read_tblbytes(fFile->fitsPointer(), i, 1, size, fitsBuffer, &status);
        if (status)
        {
            ostringstream str;
            str << "An error occurred while reading fits row #" << i << " error code: " << status;
            Error(str);
            str.str("");
            for (unsigned int j=0;j<offsets.size(); j++)
                str << offsets[j] << " ";
            Error(str);
        }
        writeValuesFromFits(offsets, targetFile, fitsBuffer);
    }
    delete[] fitsBuffer;
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Set the name of the intput Fits file
//! @param evt
//!        the event transporting the file name
//
int FitsLoader::ConfigFileNamePlease(const Event& evt)
{
    fFileName = string(evt.GetText());
    Message("New Fits file: " + fFileName);
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Set the name of the input table
//! @param evt
//!        the event transporting the table name
//
int FitsLoader::ConfigTableNamePlease(const Event& evt)
{
    fTableName = string(evt.GetText());
    Message("New Fits table: " + fTableName);
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! Retrieves the configuration parameters
//! @param conf
//!             the configuration object
//
void FitsLoader::SetupConfig(Configuration& conf)
{
    if (conf.Has("outfile"))
    {
        fFileOut = conf.Get<string>("outfile");
        Message("Output file is: " + fFileOut);
    }
    if (conf.Has("fitsfile"))
    {
        fFileName = conf.Get<string>("fitsfile");
        Message("Input fits is: " + fFileName);
    }
    if (conf.Has("tablename"))
    {
        fTableName = conf.Get<string>("tablename");
        Message("Input Table is: " + fTableName);
    }
    if (conf.Has("dump"))
    {
        fDumpList = conf.Get<vector<string>>("dump");
        Message("Dump list is:");
        for (vector<string>::iterator it=fDumpList.begin(); it != fDumpList.end(); it++)
            Message(*it);
    }
    if (conf.Has("precision"))
    {
        fStreamPrecision = conf.Get<int>("precision");

        ostringstream str;
        str << "OFStream precision is: " << fStreamPrecision;
        Message(str);
    }
}
void RunThread(FitsLoader* loader)
{
    loader->Run(true);
    Readline::Stop();
}
template<class T>
int RunShell(Configuration& conf)
{
    static T shell(conf.GetName().c_str(), conf.Get<int>("console")!=1);

    WindowLog& wout = shell.GetStreamOut();

    FitsLoader loader(wout);
    loader.SetupConfig(conf);
    shell.SetReceiver(loader);

    boost::thread t(boost::bind(RunThread, &loader));

    shell.Run();

    loader.Stop();

    t.join();

    return 0;
}
void PrintUsage()
{
    cout << "This is a usage. to be completed" << endl;
}
void PrintHelp()
{
    cout << "This is the help. I know, not so helpfull at the moment..." << endl;
}
void SetupConfiguration(Configuration& conf)
{
    po::options_description configp("Programm options");
    configp.add_options()
            ("console,c", var<int>(), "Use console (0=shell, 1=simple buffered, X=simple unbuffered)");

    po::options_description configs("Fits Loader options");
    configs.add_options()
            ("outfile,o", var<string>(), "Output file")
            ("fitsfile,f", var<string>(), "Input Fits file")
            ("tablename,t", var<string>(), "Input Table")
            ("dump,d", vars<string>(), "List of columns to dump")
            ("precision,p", var<int>(), "Precision of ofstream")
            ;
//    conf.AddEnv("dns", "DIM_DNS_NODE");

    conf.AddOptions(configp);
    conf.AddOptions(configs);
}
int main(int argc, const char** argv)
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return -1;

//    if (!conf.Has("console"))
//        return Run(conf);
    if (conf.Get<int>("console")==0)
        return RunShell<LocalShell>(conf);
    else
        return RunShell<LocalConsole>(conf);

    return 0;
}
