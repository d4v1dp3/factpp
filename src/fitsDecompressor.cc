/*
 * fitsCompressor.cc
 *
 *  Created on: May 7, 2013
 *      Author: lyard
 */


#include "Configuration.h"
#include "../externals/factfits.h"
#include "../externals/ofits.h"
#include "../externals/checksum.h"

#include <map>
#include <fstream>
#include <sstream>
#include <iostream>


using namespace std;

class CompressedFitsFile
{
public:
    class HeaderEntry
    {
        public:
            /**
             *  Default constructor
             */
            HeaderEntry(): _key(""),
                           _value(""),
                           _comment(""),
                           _fitsString("")
            {
            }

            /**
             * Regular constructor.
             * @param key the name of the keyword entry
             * @param value its value
             * @param comment an optionnal comment to be placed after the value
             */
            template<typename T>
            HeaderEntry(const string& k,
                        const T& val,
                        const string& comm) : _key(k),
                                              _value(""),
                                              _comment(comm),
                                              _fitsString("")
            {
                setValue(val);
            }

            /**
             * From fits.h
             */
            string Trim(const string &str, char c=' ')
            {
                // Trim Both leading and trailing spaces
                const size_t pstart = str.find_first_not_of(c); // Find the first character position after excluding leading blank spaces
                const size_t pend   = str.find_last_not_of(c);  // Find the first character position from reverse af

                // if all spaces or empty return an empty string
                if (string::npos==pstart || string::npos==pend)
                    return string();

                return str.substr(pstart, pend-pstart+1);
            }
            /**
             *  Constructor from the original fits entry
             */
            HeaderEntry(const string& line)
            {
                _fitsString = line.data();

                //parse the line
                _key = Trim(line.substr(0,8));
                //COMMENT and/or HISTORY values
                if (line.substr(8,2)!= "= ")
                {
                    _value = "";
                    _comment = Trim(line.substr(10));
                    return;
                }
                string next=line.substr(10);
                const size_t slash = next.find_first_of('/');
                _value = Trim(Trim(Trim(next.substr(0, slash)), '\''));
                _comment = Trim(next.substr(slash+1));
            }
            /**
             *  Alternative constroctor from the fits entry
             */
            HeaderEntry(const vector<char>& lineVec)
            {
                HeaderEntry(string(lineVec.data()));
            }
            /**
             *  Destructor
             */
            virtual ~HeaderEntry(){}

            const string& value()      const {return _value;}
            const string& key()        const {return _key;}
            const string& comment()    const {return _comment;}
            const string& fitsString() const {return _fitsString;}

            /**
             * Set a keyword value.
             * @param value The value to be set
             * @param update whether the value already exist or not. To be modified soon.
             */
            template<typename T>
            void setValue(const T& val)
            {
                ostringstream str;
                str << val;
                _value = str.str();
                buildFitsString();
            };
            /**
             *  Set the comment for a given entry
             */
            void setComment(const string& comm)
            {
                _comment = comm;
                buildFitsString();
            }

        private:
            /**
             *  Construct the FITS header string from the key, value and comment
             */
            void buildFitsString()
            {
                ostringstream str;
                unsigned int totSize = 0;

                // Tuncate the key if required
                if (_key.length() > 8)
                {
                    str << _key.substr(0, 8);
                    totSize += 8;
                }
                else
                {
                    str << _key;
                    totSize += _key.length();
                }

                // Append space if key is less than 8 chars long
                for (int i=totSize; i<8;i++)
                {
                    str << " ";
                    totSize++;
                }

                // Add separator
                str << "= ";
                totSize += 2;

                // Format value
                if (_value.length() < 20)
                    for (;totSize<30-_value.length();totSize++)
                        str << " ";

                if (_value.length() > 70)
                {
                    str << _value.substr(0,70);
                    totSize += 70;
                }
                else
                {
                    str << _value;
                    totSize += _value.size();
                }

                // If there is space remaining, add comment area
                if (totSize < 77)
                {
                    str << " / ";
                    totSize += 3;
                    if (totSize < 80)
                    {
                        unsigned int commentSize = 80 - totSize;
                        if (_comment.length() > commentSize)
                        {
                            str << _comment.substr(0,commentSize);
                            totSize += commentSize;
                        }
                        else
                        {
                            str << _comment;
                            totSize += _comment.length();
                        }
                    }
                }

                // If there is yet some free space, fill up the entry with spaces
                for (int i=totSize; i<80;i++)
                    str << " ";

                _fitsString = str.str();

                // Check for correct completion
                if (_fitsString.length() != 80)
                    cout << "Error |" << _fitsString << "| is not of length 80" << endl;
            }

            string _key;        ///< the key (name) of the header entry
            string _value;      ///< the value of the header entry
            string _comment;    ///< the comment associated to the header entry
            string _fitsString; ///< the string that will be written to the fits file
    };

    /**
     *  Supported compressions
     */
    typedef enum
    {
        UNCOMPRESSED,
        SMOOTHMAN
    } FitsCompression;

    /**
     *  Columns class
     */
    class ColumnEntry
    {
        public:
            /**
             *  Default constructor
             */
            ColumnEntry();

            /**
             *  Default destructor
             */
            virtual ~ColumnEntry(){}

            /**
             *  Constructor from values
             *  @param n the column name
             *  @param t the column type
             *  @param numof the number of entries in the column
             *  @param comp the compression type for this column
             */
            ColumnEntry(const string& n,
                        char          t,
                        int           numOf,
                        BlockHeader&   head,
                        vector<uint16_t>& seq) : _name(n),
                                                 _num(numOf),
                                                 _typeSize(0),
                                                 _offset(0),
                                                 _type(t),
                                                 _description(""),
                                                 _header(head),
                                                 _compSequence(seq)
            {
                switch (t)
                {
                    case 'L':
                    case 'A':
                    case 'B': _typeSize = 1; break;
                    case 'I': _typeSize = 2; break;
                    case 'J':
                    case 'E': _typeSize = 4; break;
                    case 'K':
                    case 'D': _typeSize = 8; break;
                    default:
                    cout << "Error: typename " << t << " missing in the current implementation" << endl;
                };

                ostringstream str;
                str << "data format of field: ";

                switch (t)
                {
                    case 'L': str << "1-byte BOOL"; break;
                    case 'A': str << "1-byte CHAR"; break;
                    case 'B': str << "BYTE"; break;
                    case 'I': str << "2-byte INTEGER"; break;
                    case 'J': str << "4-byte INTEGER"; break;
                    case 'K': str << "8-byte INTEGER"; break;
                    case 'E': str << "4-byte FLOAT"; break;
                    case 'D': str << "8-byte FLOAT"; break;
                }

                _description = str.str();
            }

            const string& name()                 const { return _name;};
            int           width()                const { return _num*_typeSize;};
            int           offset()               const { return _offset; };
            int           numElems()             const { return _num; };
            int           sizeOfElems()          const { return _typeSize;};
            void          setOffset(int off)           { _offset = off;};
            char          type()                 const { return _type;};
            string        getDescription()       const { return _description;}
            BlockHeader& getBlockHeader()  { return _header;}
            const vector<uint16_t>& getCompressionSequence() const { return _compSequence;}
            const char& getColumnOrdering() const { return _header.ordering;}


            string        getCompressionString() const
            {
                return "FACT";
             /*
                ostringstream str;
                for (uint32_t i=0;i<_compSequence.size();i++)
                switch (_compSequence[i])
                {
                    case FACT_RAW: if (str.str().size() == 0) str << "RAW"; break;
                    case FACT_SMOOTHING: str << "SMOOTHING "; break;
                    case FACT_HUFFMAN16: str << "HUFFMAN16 "; break;
                };
                return str.str();*/
            }

        private:

            string _name;          ///< name of the column
            int     _num;          ///< number of elements contained in one row of this column
            int    _typeSize;      ///< the number of bytes taken by one element
            int    _offset;        ///< the offset of the column, in bytes, from the beginning of one row
            char   _type;          ///< the type of the column, as specified by the fits documentation
            string _description;   ///< a description for the column. It will be placed in the header
            BlockHeader _header;
            vector<uint16_t> _compSequence;
    };

    public:
        ///@brief default constructor. Assigns a default number of rows and tiles
        CompressedFitsFile(uint32_t numTiles=100, uint32_t numRowsPerTile=100);

        ///@brief default destructor
        virtual ~CompressedFitsFile();

        ///@brief get the header of the file
        vector<HeaderEntry>& getHeaderEntries() { return _header;}

    protected:
        ///@brief protected function to allocate the intermediate buffers
        bool reallocateBuffers();

        //FITS related stuff
        vector<HeaderEntry>  _header;         ///< Header keys
        vector<ColumnEntry>  _columns;        ///< Columns in the file
        uint32_t             _numTiles;       ///< Number of tiles (i.e. groups of rows)
        uint32_t             _numRowsPerTile; ///< Number of rows per tile
        uint32_t             _totalNumRows;   ///< Total number of raws
        uint32_t             _rowWidth;       ///< Total number of bytes in one row
        bool                 _headerFlushed;  ///< Flag telling whether the header record is synchronized with the data on disk
        char*                _buffer;         ///< Memory buffer to store rows while they are not compressed
        Checksum             _checksum;       ///< Checksum for asserting the consistency of the data
        fstream              _file;           ///< The actual file streamer for accessing disk data

        //compression related stuff
        typedef pair<int64_t, int64_t> CatalogEntry;
        typedef vector<CatalogEntry>   CatalogRow;
        typedef vector<CatalogRow>     CatalogType;
        CatalogType _catalog;              ///< Catalog, i.e. the main table that points to the compressed data.
        uint64_t                 _heapPtr; ///< the address in the file of the heap area
        vector<char*> _transposedBuffer;   ///< Memory buffer to store rows while they are transposed
        vector<char*> _compressedBuffer;   ///< Memory buffer to store rows while they are compressed

        //thread related stuff
        uint32_t          _numThreads;    ///< The number of threads that will be used to compress
        uint32_t          _threadIndex;   ///< A variable to assign threads indices
        vector<pthread_t> _thread;        ///< The thread handler of the compressor
        vector<uint32_t>  _threadNumRows; ///< Total number of rows for thread to compress
        vector<uint32_t>  _threadStatus;  ///< Flag telling whether the buffer to be transposed (and compressed) is full or empty

        //thread states. Not all used, but they do not hurt
        static const uint32_t       _THREAD_WAIT_; ///< Thread doing nothing
        static const uint32_t   _THREAD_COMPRESS_; ///< Thread working, compressing
        static const uint32_t _THREAD_DECOMPRESS_; ///< Thread working, decompressing
        static const uint32_t      _THREAD_WRITE_; ///< Thread writing data to disk
        static const uint32_t       _THREAD_READ_; ///< Thread reading data from disk
        static const uint32_t       _THREAD_EXIT_; ///< Thread exiting

        static HeaderEntry _dummyHeaderEntry; ///< Dummy entry for returning if requested on is not found
};

class CompressedFitsWriter : public CompressedFitsFile
{
    public:
        ///@brief Default constructor. 100 tiles of 100 rows each are assigned by default
        CompressedFitsWriter(uint32_t numTiles=100, uint32_t numRowsPerTile=100);

        ///@brief default destructor
        virtual ~CompressedFitsWriter();

        ///@brief add one column to the file
        bool addColumn(const ColumnEntry& column);

        ///@brief sets a given header key
        bool setHeaderKey(const HeaderEntry&);

        bool changeHeaderKey(const string& origName, const string& newName);

        ///@brief open a new fits file
        bool open(const string& fileName, const string& tableName="Data");

        ///@brief close the opened file
        bool close();

        ///@brief write one row of data, already placed in bufferToWrite. Does the byte-swapping
        bool writeBinaryRow(const char* bufferToWrite);

        uint32_t getRowWidth();

        ///@brief assign a given (already loaded) drs calibration
        void setDrsCalib(int16_t* data);

        ///@brief set the number of worker threads compressing the data.
        bool setNumWorkingThreads(uint32_t num);

    private:
        ///@brief compresses one buffer of data, the one given by threadIndex
        uint64_t compressBuffer(uint32_t threadIndex);

        ///@brief writes an already compressed buffer to disk
        bool writeCompressedDataToDisk(uint32_t threadID, uint32_t sizeToWrite);

        ///@brief add the header checksum to the datasum
        void addHeaderChecksum(Checksum& checksum);

        ///@brief write the header. If closingFile is set to true, checksum is calculated
        void writeHeader(bool closingFile = false);

        ///@brief write the compressed data catalog. If closingFile is set to true, checksum is calculated
        void writeCatalog(bool closingFile=false);

        /// FIXME this was a bad idea. Move everything to the regular header
        vector<HeaderEntry> _defaultHeader;

        /// the main function compressing the data
        static void* threadFunction(void* context);

        /// Write the drs calibration to disk, if any
        void writeDrsCalib();

        /// Copy and transpose (or semi-transpose) one tile of data
        void copyTransposeTile(uint32_t index);

        /// Specific compression functions
        uint32_t compressUNCOMPRESSED(char* dest, const char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems);
        uint32_t      compressHUFFMAN(char* dest, const char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems);
        uint32_t    compressSMOOTHMAN(char* dest, char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems);
        uint32_t       applySMOOTHING(char* dest, char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems);

        int32_t         _checkOffset;  ///< offset to the data pointer to calculate the checksum
        int16_t*        _drsCalibData; ///< array of the Drs baseline mean
        int32_t         _threadLooper; ///< Which thread will deal with the upcoming bunch of data ?
        pthread_mutex_t _mutex;        ///< mutex for compressing threads


        static string _emptyBlock; ///< an empty block to be apened at the end of a file so that its length is a multiple of 2880
        static string _fitsHeader; ///< the default header to be written in every fits file
};

const uint32_t CompressedFitsFile::_THREAD_WAIT_      = 0;
const uint32_t CompressedFitsFile::_THREAD_COMPRESS_  = 1;
const uint32_t CompressedFitsFile::_THREAD_DECOMPRESS_= 2;
const uint32_t CompressedFitsFile::_THREAD_WRITE_     = 3;
const uint32_t CompressedFitsFile::_THREAD_READ_      = 4;
const uint32_t CompressedFitsFile::_THREAD_EXIT_      = 5;

template<>
void CompressedFitsFile::HeaderEntry::setValue(const string& v)
{
    string val = v;
    if (val.size() > 2 && val[0] == '\'')
    {
        size_t pos = val.find_last_of("'");
        if (pos != string::npos && pos != 0)
            val = val.substr(1, pos-1);
    }
    ostringstream str;

    str << "'" << val << "'";
    for (int i=str.str().length(); i<20;i++)
        str << " ";
    _value = str.str();
    buildFitsString();
}

/**
 * Default header to be written in all fits files
 */
string CompressedFitsWriter::_fitsHeader = "SIMPLE  =                    T / file does conform to FITS standard             "
                    "BITPIX  =                    8 / number of bits per data pixel                  "
                    "NAXIS   =                    0 / number of data axes                            "
                    "EXTEND  =                    T / FITS dataset may contain extensions            "
                    "CHECKSUM= '4AcB48bA4AbA45bA'   / Checksum for the whole HDU                     "
                    "DATASUM = '         0'         / Checksum for the data block                    "
                    "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy"
                    "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H "
                    "END                                                                             "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                ";

/**
 * Empty block to be appenned at the end of files, so that the length matches multiple of 2880 bytes
 *
 */
string CompressedFitsWriter::_emptyBlock = "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                "
                    "                                                                                ";

CompressedFitsFile::HeaderEntry CompressedFitsFile::_dummyHeaderEntry;
/****************************************************************
 *              SUPER CLASS DEFAULT CONSTRUCTOR
 ****************************************************************/
CompressedFitsFile::CompressedFitsFile(uint32_t numTiles,
                                       uint32_t numRowsPerTile) : _header(),
                                                                 _columns(),
                                                                 _numTiles(numTiles),
                                                                 _numRowsPerTile(numRowsPerTile),
                                                                 _totalNumRows(0),
                                                                 _rowWidth(0),
                                                                 _headerFlushed(false),
                                                                 _buffer(NULL),
                                                                 _checksum(0),
                                                                 _heapPtr(0),
                                                                 _transposedBuffer(1),
                                                                 _compressedBuffer(1),
                                                                 _numThreads(1),
                                                                 _threadIndex(0),
                                                                 _thread(1),
                                                                 _threadNumRows(1),
                                                                 _threadStatus(1)
{
    _catalog.resize(_numTiles);
    _transposedBuffer[0] = NULL;
    _compressedBuffer[0] = NULL;
    _threadStatus[0] = _THREAD_WAIT_;
    _threadNumRows[0] = 0;
}

/****************************************************************
 *              SUPER CLASS DEFAULT DESTRUCTOR
 ****************************************************************/
CompressedFitsFile::~CompressedFitsFile()
{
    if (_buffer != NULL)
    {
        _buffer = _buffer-4;
        delete[] _buffer;
        _buffer = NULL;
        for (uint32_t i=0;i<_numThreads;i++)
        {
            _compressedBuffer[i] = _compressedBuffer[i]-4;
            delete[] _transposedBuffer[i];
            delete[] _compressedBuffer[i];
            _transposedBuffer[i] = NULL;
            _compressedBuffer[i] = NULL;
        }
    }
    if (_file.is_open())
        _file.close();
}

/****************************************************************
 *              REALLOCATE BUFFER
 ****************************************************************/
bool CompressedFitsFile::reallocateBuffers()
{
    if (_buffer)
    {
        _buffer = _buffer - 4;
        delete[] _buffer;
        for (uint32_t i=0;i<_compressedBuffer.size();i++)
        {
            _compressedBuffer[i] = _compressedBuffer[i]-4;
            delete[] _transposedBuffer[i];
            delete[] _compressedBuffer[i];
        }
    }
    _buffer = new char[_rowWidth*_numRowsPerTile + 12];
    if (_buffer == NULL) return false;
    memset(_buffer, 0, 4);
    _buffer = _buffer + 4;
    if (_compressedBuffer.size() != _numThreads)
    {
        _transposedBuffer.resize(_numThreads);
        _compressedBuffer.resize(_numThreads);
    }
    for (uint32_t i=0;i<_numThreads;i++)
    {
        _transposedBuffer[i] = new char[_rowWidth*_numRowsPerTile];
        _compressedBuffer[i] = new char[_rowWidth*_numRowsPerTile + _columns.size() + sizeof(TileHeader) + 12]; //use a bit more memory for compression flags and checksumming
        if (_transposedBuffer[i] == NULL || _compressedBuffer[i] == NULL)
            return false;
        //shift the compressed buffer by 4 bytes, for checksum calculation
        memset(_compressedBuffer[i], 0, 4);
        _compressedBuffer[i] = _compressedBuffer[i]+4;
        //initialize the tile header
        TileHeader tileHeader;
        memcpy(_compressedBuffer[i], &tileHeader, sizeof(TileHeader));
    }
    return true;
}

/****************************************************************
 *                  DEFAULT WRITER CONSTRUCTOR
 ****************************************************************/
CompressedFitsWriter::CompressedFitsWriter(uint32_t numTiles,
                                           uint32_t numRowsPerTile) : CompressedFitsFile(numTiles, numRowsPerTile),
                                                                    _checkOffset(0),
                                                                    _drsCalibData(NULL),
                                                                    _threadLooper(0)
{
    _defaultHeader.push_back(HeaderEntry("XTENSION", "'BINTABLE'          ", "binary table extension"));
    _defaultHeader.push_back(HeaderEntry("BITPIX", 8, "8-bit bytes"));
    _defaultHeader.push_back(HeaderEntry("NAXIS", 2, "2-dimensional binary table"));
    _defaultHeader.push_back(HeaderEntry("NAXIS1", _rowWidth, "width of table in bytes"));
    _defaultHeader.push_back(HeaderEntry("NAXIS2", numTiles, "num of rows in table"));
    _defaultHeader.push_back(HeaderEntry("PCOUNT", 0, "size of special data area"));
    _defaultHeader.push_back(HeaderEntry("GCOUNT", 1, "one data group (required keyword)"));
    _defaultHeader.push_back(HeaderEntry("TFIELDS", _columns.size(), "number of fields in each row"));
    _defaultHeader.push_back(HeaderEntry("CHECKSUM", "'0000000000000000'  ", "Checksum for the whole HDU"));
    _defaultHeader.push_back(HeaderEntry("DATASUM",  "         0", "Checksum for the data block"));
    //compression stuff
    _defaultHeader.push_back(HeaderEntry("ZTABLE", "T", "Table is compressed"));
    _defaultHeader.push_back(HeaderEntry("ZNAXIS1", 0, "Width of uncompressed rows"));
    _defaultHeader.push_back(HeaderEntry("ZNAXIS2", 0, "Number of uncompressed rows"));
    _defaultHeader.push_back(HeaderEntry("ZPCOUNT", 0, ""));
    _defaultHeader.push_back(HeaderEntry("ZHEAPPTR", 0, ""));
    _defaultHeader.push_back(HeaderEntry("ZTILELEN", numRowsPerTile, "Number of rows per tile"));
    _defaultHeader.push_back(HeaderEntry("THEAP", 0, ""));

    pthread_mutex_init(&_mutex, NULL);
}

/****************************************************************
 *              DEFAULT DESTRUCTOR
 ****************************************************************/
CompressedFitsWriter::~CompressedFitsWriter()
{
    pthread_mutex_destroy(&_mutex);
}

/****************************************************************
 *              SET THE POINTER TO THE DRS CALIBRATION
 ****************************************************************/
void CompressedFitsWriter::setDrsCalib(int16_t* data)
{
    _drsCalibData = data;
}

/****************************************************************
 *                  SET NUM WORKING THREADS
 ****************************************************************/
bool CompressedFitsWriter::setNumWorkingThreads(uint32_t num)
{
    if (_file.is_open())
        return false;
    if (num < 1 || num > 64)
    {
        cout << "ERROR: num threads must be between 1 and 64. Ignoring" << endl;
        return false;
    }
    _numThreads = num;
    _transposedBuffer[0] = NULL;
    _compressedBuffer[0] = NULL;
    _threadStatus.resize(num);
    _thread.resize(num);
    _threadNumRows.resize(num);
    for (uint32_t i=0;i<num;i++)
    {
        _threadNumRows[i] = 0;
        _threadStatus[i] = _THREAD_WAIT_;
    }
    return reallocateBuffers();
}

/****************************************************************
 *              WRITE DRS CALIBRATION TO FILE
 ****************************************************************/
void CompressedFitsWriter::writeDrsCalib()
{
    //if file was not loaded, ignore
    if (_drsCalibData == NULL)
        return;
    uint64_t whereDidIStart = _file.tellp();
    vector<HeaderEntry> header;
    header.push_back(HeaderEntry("XTENSION", "'BINTABLE'          ", "binary table extension"));
    header.push_back(HeaderEntry("BITPIX"  , 8                     , "8-bit bytes"));
    header.push_back(HeaderEntry("NAXIS"   , 2                     , "2-dimensional binary table"));
    header.push_back(HeaderEntry("NAXIS1"  , 1024*1440*2           , "width of table in bytes"));
    header.push_back(HeaderEntry("NAXIS2"  , 1                     , "number of rows in table"));
    header.push_back(HeaderEntry("PCOUNT"  , 0                     , "size of special data area"));
    header.push_back(HeaderEntry("GCOUNT"  , 1                     , "one data group (required keyword)"));
    header.push_back(HeaderEntry("TFIELDS" , 1                     , "number of fields in each row"));
    header.push_back(HeaderEntry("CHECKSUM", "'0000000000000000'  ", "Checksum for the whole HDU"));
    header.push_back(HeaderEntry("DATASUM" ,  "         0"         , "Checksum for the data block"));
    header.push_back(HeaderEntry("EXTNAME" , "'ZDrsCellOffsets'    ", "name of this binary table extension"));
    header.push_back(HeaderEntry("TTYPE1"  , "'OffsetCalibration' ", "label for field   1"));
    header.push_back(HeaderEntry("TFORM1"  , "'1474560I'          ", "data format of field: 2-byte INTEGER"));

    for (uint32_t i=0;i<header.size();i++)
        _file.write(header[i].fitsString().c_str(), 80);
    //End the header
    _file.write("END                                                                             ", 80);
    long here = _file.tellp();
    if (here%2880)
        _file.write(_emptyBlock.c_str(), 2880 - here%2880);
    //now write the data itself
    int16_t* swappedBytes = new int16_t[1024];
    Checksum checksum;
    for (int32_t i=0;i<1440;i++)
    {
        memcpy(swappedBytes, &(_drsCalibData[i*1024]), 2048);
        for (int32_t j=0;j<2048;j+=2)
        {
            int8_t inter;
            inter = reinterpret_cast<int8_t*>(swappedBytes)[j];
            reinterpret_cast<int8_t*>(swappedBytes)[j] = reinterpret_cast<int8_t*>(swappedBytes)[j+1];
            reinterpret_cast<int8_t*>(swappedBytes)[j+1] = inter;
        }
        _file.write(reinterpret_cast<char*>(swappedBytes), 2048);
        checksum.add(reinterpret_cast<char*>(swappedBytes), 2048);
    }
    uint64_t whereDidIStop = _file.tellp();
    delete[] swappedBytes;
    //No need to pad the data, as (1440*1024*2)%2880==0

    //calculate the checksum from the header
    ostringstream str;
    str << checksum.val();
    header[9] = HeaderEntry("DATASUM", str.str(), "Checksum for the data block");
    for (vector<HeaderEntry>::iterator it=header.begin();it!=header.end(); it++)
        checksum.add(it->fitsString().c_str(), 80);
    string   end("END                                                                             ");
    string space("                                                                                ");
    checksum.add(end.c_str(), 80);
    int headerRowsLeft = 36 - (header.size() + 1)%36;
    for (int i=0;i<headerRowsLeft;i++)
        checksum.add(space.c_str(), 80);
    //udpate the checksum keyword
    header[8] = HeaderEntry("CHECKSUM", checksum.str(), "Checksum for the whole HDU");
    //and eventually re-write the header data
    _file.seekp(whereDidIStart);
    for (uint32_t i=0;i<header.size();i++)
        _file.write(header[i].fitsString().c_str(), 80);
    _file.seekp(whereDidIStop);
}

/****************************************************************
 *              ADD COLUMN
 ****************************************************************/
bool CompressedFitsWriter::addColumn(const ColumnEntry& column)
{
    if (_totalNumRows != 0)
    {
        cout << "Error: cannot add new columns once first row has been written" << endl;
        return false;
    }
    for (vector<ColumnEntry>::iterator it=_columns.begin(); it != _columns.end(); it++)
    {
        if (it->name() == column.name())
        {
            cout << "Warning: column already exist (" << column.name() << "). Ignoring" << endl;
            return false;
        }
    }
    _columns.push_back(column);
    _columns.back().setOffset(_rowWidth);
    _rowWidth += column.width();
    reallocateBuffers();

    ostringstream str, str2, str3;
    str << "TTYPE" << _columns.size();
    str2 << column.name();
    str3 << "label for field ";
    if (_columns.size() < 10) str3 << " ";
    if (_columns.size() < 100) str3 << " ";
    str3 << _columns.size();
    setHeaderKey(HeaderEntry(str.str(), str2.str(), str3.str()));
    str.str("");
    str2.str("");
    str3.str("");
    str << "TFORM" << _columns.size();
    str2 << "1QB";
    str3 << "data format of field " << _columns.size();
    setHeaderKey(HeaderEntry(str.str(), str2.str(), str3.str()));
    str.str("");
    str2.str("");
    str3.str("");
    str << "ZFORM" << _columns.size();
    str2 << column.numElems() << column.type();
    str3 << "Original format of field " << _columns.size();
    setHeaderKey(HeaderEntry(str.str(), str2.str(), str3.str()));
    str.str("");
    str2.str("");
    str3.str("");
    str << "ZCTYP" << _columns.size();
    str2 << column.getCompressionString();
    str3 << "Comp. Scheme of field " << _columns.size();
    setHeaderKey(HeaderEntry(str.str(), str2.str(), str3.str()));
    //resize the catalog vector accordingly
    for (uint32_t i=0;i<_numTiles;i++)
    {
        _catalog[i].resize(_columns.size());
        for (uint32_t j=0;j<_catalog[i].size();j++)
            _catalog[i][j] = make_pair(0,0);
    }
    return true;
}

/****************************************************************
 *                  SET HEADER KEY
 ****************************************************************/
bool CompressedFitsWriter::setHeaderKey(const HeaderEntry& entry)
{
    HeaderEntry ent = entry;
    for (vector<HeaderEntry>::iterator it=_header.begin(); it != _header.end(); it++)
    {
        if (it->key() == entry.key())
        {
            if (entry.comment() == "")
                ent.setComment(it->comment());
            (*it) = ent;
            _headerFlushed = false;
            return true;
        }
    }
    for (vector<HeaderEntry>::iterator it=_defaultHeader.begin(); it != _defaultHeader.end(); it++)
    {
        if (it->key() == entry.key())
        {
            if (entry.comment() == "")
                ent.setComment(it->comment());
            (*it) = ent;
            _headerFlushed = false;
            return true;
        }
    }
    if (_totalNumRows != 0)
    {
        cout << "Error: new header keys (" << entry.key() << ") must be set before the first row is written. Ignoring." << endl;
        return false;
    }
    _header.push_back(entry);
    _headerFlushed = false;
    return true;
}

bool CompressedFitsWriter::changeHeaderKey(const string& origName, const string& newName)
{
    for (vector<HeaderEntry>::iterator it=_header.begin(); it != _header.end(); it++)
    {
        if (it->key() == origName)
        {
            (*it) = HeaderEntry(newName, it->value(), it->comment());
            _headerFlushed = false;
            return true;
        }
    }
    for (vector<HeaderEntry>::iterator it=_defaultHeader.begin(); it != _defaultHeader.end(); it++)
    {
        if (it->key() == origName)
        {
            (*it) = HeaderEntry(newName, it->value(), it->comment());
            _headerFlushed = false;
            return true;
        }
    }
    return false;
}
/****************************************************************
 *              OPEN
 ****************************************************************/
bool CompressedFitsWriter::open(const string& fileName, const string& tableName)
{
     _file.open(fileName.c_str(), ios_base::out);
    if (!_file.is_open())
    {
        cout << "Error: Could not open the file (" << fileName << ")." << endl;
        return false;
    }
    _defaultHeader.push_back(HeaderEntry("EXTNAME", tableName, "name of this binary table extension"));
    _headerFlushed = false;
    _threadIndex = 0;
    //create requested number of threads
    for (uint32_t i=0;i<_numThreads;i++)
        pthread_create(&(_thread[i]), NULL, threadFunction, this);
    //wait for the threads to start
    while (_numThreads != _threadIndex)
        usleep(1000);
    //set the writing fence to the last thread
    _threadIndex = _numThreads-1;
    return (_file.good());
}

/****************************************************************
 *              WRITE HEADER
 ****************************************************************/
void CompressedFitsWriter::writeHeader(bool closingFile)
{
    if (_headerFlushed)
        return;
    if (!_file.is_open())
        return;

    long cPos = _file.tellp();

    _file.seekp(0);

    _file.write(_fitsHeader.c_str(), 2880);

    //Write the DRS calib table here !
    writeDrsCalib();

    //we are now at the beginning of the main table. Write its header
    for (vector<HeaderEntry>::iterator it=_defaultHeader.begin(); it != _defaultHeader.end(); it++)
        _file.write(it->fitsString().c_str(), 80);

    for (vector<HeaderEntry>::iterator it=_header.begin(); it != _header.end(); it++)
        _file.write(it->fitsString().c_str(), 80);

    _file.write("END                                                                             ", 80);
    long here = _file.tellp();
    if (here%2880)
        _file.write(_emptyBlock.c_str(), 2880 - here%2880);

    _headerFlushed = true;

    here = _file.tellp();

    if (here%2880)
        cout << "Error: seems that header did not finish at the end of a block." << endl;

    if (here > cPos && cPos != 0)
    {
        cout << "Error, entries were added after the first row was written. This is not supposed to happen." << endl;
        return;
    }

    here = _file.tellp();
    writeCatalog(closingFile);

    here = _file.tellp() - here;
    _heapPtr = here;

    if (cPos != 0)
        _file.seekp(cPos);
}

/****************************************************************
 *              WRITE CATALOG
 *  WARNING: writeCatalog is only meant to be used by writeHeader.
 *  external usage will most likely corrupt the file
 ****************************************************************/
void CompressedFitsWriter::writeCatalog(bool closingFile)
{
    uint32_t sizeWritten = 0;
    for (uint32_t i=0;i<_catalog.size();i++)
    {
        for (uint32_t j=0;j<_catalog[i].size();j++)
        {
            //swap the bytes
            int8_t swappedEntry[16];
            swappedEntry[0] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[7];
            swappedEntry[1] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[6];
            swappedEntry[2] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[5];
            swappedEntry[3] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[4];
            swappedEntry[4] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[3];
            swappedEntry[5] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[2];
            swappedEntry[6] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[1];
            swappedEntry[7] = reinterpret_cast<int8_t*>(&_catalog[i][j].first)[0];

            swappedEntry[8] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[7];
            swappedEntry[9] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[6];
            swappedEntry[10] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[5];
            swappedEntry[11] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[4];
            swappedEntry[12] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[3];
            swappedEntry[13] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[2];
            swappedEntry[14] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[1];
            swappedEntry[15] = reinterpret_cast<int8_t*>(&_catalog[i][j].second)[0];
            if (closingFile)
            {
                _checksum.add(reinterpret_cast<char*>(swappedEntry), 16);
            }
            _file.write(reinterpret_cast<char*>(&swappedEntry[0]), 2*sizeof(int64_t));
            sizeWritten += 2*sizeof(int64_t);
        }
    }

    //we do not reserve space for now because fverify does not like that.
    //TODO bug should be fixed in the new version. Install it on the cluster and restor space reservation
    return ;

    //write the padding so that the HEAP section starts at a 2880 bytes boundary
    if (sizeWritten % 2880 != 0)
    {
        vector<char> nullVec(2880 - sizeWritten%2880, 0);
        _file.write(nullVec.data(), 2880 - sizeWritten%2880);
    }
}

/****************************************************************
 *              ADD HEADER CHECKSUM
 ****************************************************************/
void CompressedFitsWriter::addHeaderChecksum(Checksum& checksum)
{
    for (vector<HeaderEntry>::iterator it=_defaultHeader.begin();it!=_defaultHeader.end(); it++)
        _checksum.add(it->fitsString().c_str(), 80);
    for (vector<HeaderEntry>::iterator it=_header.begin(); it != _header.end(); it++)
        _checksum.add(it->fitsString().c_str(), 80);
    string   end("END                                                                             ");
    string space("                                                                                ");
    checksum.add(end.c_str(), 80);
    int headerRowsLeft = 36 - (_defaultHeader.size() + _header.size() + 1)%36;
    for (int i=0;i<headerRowsLeft;i++)
        checksum.add(space.c_str(), 80);
}

/****************************************************************
 *                  CLOSE
 ****************************************************************/
bool CompressedFitsWriter::close()
{
    for (uint32_t i=0;i<_numThreads;i++)
        while (_threadStatus[i] != _THREAD_WAIT_)
            usleep(100000);
    for (uint32_t i=0;i<_numThreads;i++)
        _threadStatus[i] = _THREAD_EXIT_;
    for (uint32_t i=0;i<_numThreads;i++)
        pthread_join(_thread[i], NULL);
    //flush the rows that were not written yet
    if (_totalNumRows%_numRowsPerTile != 0)
    {
        copyTransposeTile(0);

        _threadNumRows[0] = _totalNumRows;
        uint32_t numBytes = compressBuffer(0);
        writeCompressedDataToDisk(0, numBytes);
    }
    //compression stuff
    setHeaderKey(HeaderEntry("ZNAXIS1", _rowWidth, "Width of uncompressed rows"));
    setHeaderKey(HeaderEntry("ZNAXIS2", _totalNumRows, "Number of uncompressed rows"));
    //TODO calculate the real offset from the main table to the start of the HEAP data area
    setHeaderKey(HeaderEntry("ZHEAPPTR", _heapPtr, ""));
    setHeaderKey(HeaderEntry("THEAP", _heapPtr, ""));

    //regular stuff
    if (_catalog.size() > 0)
    {
        setHeaderKey(HeaderEntry("NAXIS1", 2*sizeof(int64_t)*_catalog[0].size(), "width of table in bytes"));
        setHeaderKey(HeaderEntry("NAXIS2", _numTiles, ""));
        setHeaderKey(HeaderEntry("TFIELDS", _columns.size(), "number of fields in each row"));
        int64_t heapSize = 0;
        int64_t compressedOffset = 0;
        for (uint32_t i=0;i<_catalog.size();i++)
        {
            compressedOffset += sizeof(TileHeader);
            heapSize += sizeof(TileHeader);
            for (uint32_t j=0;j<_catalog[i].size();j++)
            {
                heapSize += _catalog[i][j].first;
//      cout << "heapSize: " << heapSize << endl;
                //set the catalog offsets to their actual values
                _catalog[i][j].second = compressedOffset;
                compressedOffset += _catalog[i][j].first;
                //special case if entry has zero length
                if (_catalog[i][j].first == 0) _catalog[i][j].second = 0;
            }
        }
        setHeaderKey(HeaderEntry("PCOUNT", heapSize, "size of special data area"));
    }
    else
    {
        setHeaderKey(HeaderEntry("NAXIS1", _columns.size()*2*sizeof(int64_t), "width of table in bytes"));
        setHeaderKey(HeaderEntry("NAXIS2", 0, ""));
        setHeaderKey(HeaderEntry("TFIELDS", _columns.size(), "number of fields in each row"));
        setHeaderKey(HeaderEntry("PCOUNT", 0, "size of special data area"));
        changeHeaderKey("THEAP", "ZHEAP");
    }
    ostringstream str;

    writeHeader(true);

    str.str("");
    str << _checksum.val();

    setHeaderKey(HeaderEntry("DATASUM", str.str(), ""));
    addHeaderChecksum(_checksum);
    setHeaderKey(HeaderEntry("CHECKSUM", _checksum.str(), ""));
    //update header value
    writeHeader();
    //update file length
    long here = _file.tellp();
    if (here%2880)
    {
        vector<char> nullVec(2880 - here%2880, 0);
        _file.write(nullVec.data(), 2880 - here%2880);
    }
    _file.close();
    return true;
}

/****************************************************************
 *                  COPY TRANSPOSE TILE
 ****************************************************************/
void CompressedFitsWriter::copyTransposeTile(uint32_t index)
{
    uint32_t thisRoundNumRows = (_totalNumRows%_numRowsPerTile) ? _totalNumRows%_numRowsPerTile : _numRowsPerTile;

    //copy the tile and transpose it
    uint32_t offset = 0;
    for (uint32_t i=0;i<_columns.size();i++)
    {
        switch (_columns[i].getColumnOrdering())//getCompression())
        {
            case FITS::kOrderByRow:
                for (uint32_t k=0;k<thisRoundNumRows;k++)
                {//regular, "semi-transposed" copy
                    memcpy(&(_transposedBuffer[index][offset]), &_buffer[k*_rowWidth + _columns[i].offset()], _columns[i].sizeOfElems()*_columns[i].numElems());
                    offset += _columns[i].sizeOfElems()*_columns[i].numElems();
                }
            break;

            case FITS::kOrderByCol :
                for (int j=0;j<_columns[i].numElems();j++)
                    for (uint32_t k=0;k<thisRoundNumRows;k++)
                    {//transposed copy
                        memcpy(&(_transposedBuffer[index][offset]), &_buffer[k*_rowWidth + _columns[i].offset() + _columns[i].sizeOfElems()*j], _columns[i].sizeOfElems());
                        offset += _columns[i].sizeOfElems();
                    }
            break;
            default:
                    cout << "Error: unknown column ordering: " << _columns[i].getColumnOrdering() << endl;

        };
    }
}

/****************************************************************
 *          WRITE BINARY ROW
 ****************************************************************/
bool CompressedFitsWriter::writeBinaryRow(const char* bufferToWrite)
{
    if (_totalNumRows == 0)
        writeHeader();

    memcpy(&_buffer[_rowWidth*(_totalNumRows%_numRowsPerTile)], bufferToWrite, _rowWidth);
    _totalNumRows++;
    if (_totalNumRows%_numRowsPerTile == 0)
    {
        //which is the next thread that we should use ?
        while (_threadStatus[_threadLooper] == _THREAD_COMPRESS_)
            usleep(100000);

        copyTransposeTile(_threadLooper);

        while (_threadStatus[_threadLooper] != _THREAD_WAIT_)
            usleep(100000);

        _threadNumRows[_threadLooper] = _totalNumRows;
        _threadStatus[_threadLooper] = _THREAD_COMPRESS_;
        _threadLooper = (_threadLooper+1)%_numThreads;
    }
    return _file.good();
}

uint32_t CompressedFitsWriter::getRowWidth()
{
    return _rowWidth;
}

/****************************************************************
 *                  COMPRESS BUFFER
 ****************************************************************/
uint32_t CompressedFitsWriter::compressUNCOMPRESSED(char* dest, const char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems)
{
    memcpy(dest, src, numRows*sizeOfElems*numRowElems);
    return numRows*sizeOfElems*numRowElems;
}

/****************************************************************
 *                  COMPRESS BUFFER
 ****************************************************************/
uint32_t CompressedFitsWriter::compressHUFFMAN(char* dest, const char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems)
{
    string huffmanOutput;
    uint32_t previousHuffmanSize = 0;
    if (numRows < 2)
    {//if we have less than 2 elems to compress, Huffman encoder does not work (and has no point). Just return larger size than uncompressed to trigger the raw storage.
        return numRows*sizeOfElems*numRowElems + 1000;
    }
    if (sizeOfElems < 2 )
    {
        cout << "Fatal ERROR: HUFMANN can only encode short or longer types" << endl;
        return 0;
    }
    uint32_t huffmanOffset = 0;
    for (uint32_t j=0;j<numRowElems;j++)
    {
        Huffman::Encode(huffmanOutput,
                        reinterpret_cast<const uint16_t*>(&src[j*sizeOfElems*numRows]),
                        numRows*(sizeOfElems/2));
        reinterpret_cast<uint32_t*>(&dest[huffmanOffset])[0] = huffmanOutput.size() - previousHuffmanSize;
        huffmanOffset += sizeof(uint32_t);
        previousHuffmanSize = huffmanOutput.size();
    }
    const size_t totalSize = huffmanOutput.size() + huffmanOffset;

    //only copy if not larger than not-compressed size
    if (totalSize < numRows*sizeOfElems*numRowElems)
        memcpy(&dest[huffmanOffset], huffmanOutput.data(), huffmanOutput.size());

    return totalSize;
}

/****************************************************************
 *                  COMPRESS BUFFER
 ****************************************************************/
uint32_t CompressedFitsWriter::compressSMOOTHMAN(char* dest, char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems)
{
    uint32_t colWidth = numRowElems;
    for (int j=colWidth*numRows-1;j>1;j--)
        reinterpret_cast<int16_t*>(src)[j] = reinterpret_cast<int16_t*>(src)[j] - (reinterpret_cast<int16_t*>(src)[j-1]+reinterpret_cast<int16_t*>(src)[j-2])/2;
    //call the huffman transposed
    return compressHUFFMAN(dest, src, numRowElems, sizeOfElems, numRows);
}

uint32_t CompressedFitsWriter::applySMOOTHING(char* , char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems)
{
    uint32_t colWidth = numRowElems;
    for (int j=colWidth*numRows-1;j>1;j--)
        reinterpret_cast<int16_t*>(src)[j] = reinterpret_cast<int16_t*>(src)[j] - (reinterpret_cast<int16_t*>(src)[j-1]+reinterpret_cast<int16_t*>(src)[j-2])/2;

    return numRows*sizeOfElems*numRowElems;
}
/****************************************************************
 *                  COMPRESS BUFFER
 ****************************************************************/
uint64_t CompressedFitsWriter::compressBuffer(uint32_t threadIndex)
{
    uint32_t thisRoundNumRows = (_threadNumRows[threadIndex]%_numRowsPerTile) ? _threadNumRows[threadIndex]%_numRowsPerTile : _numRowsPerTile;
    uint32_t offset=0;
    uint32_t currentCatalogRow = (_threadNumRows[threadIndex]-1)/_numRowsPerTile;
    uint64_t compressedOffset = sizeof(TileHeader); //skip the 'TILE' marker and tile size entry

    //now compress each column one by one by calling compression on arrays
    for (uint32_t i=0;i<_columns.size();i++)
    {
        _catalog[currentCatalogRow][i].second = compressedOffset;

        if (_columns[i].numElems() == 0) continue;

        BlockHeader& head = _columns[i].getBlockHeader();
        const vector<uint16_t>& sequence = _columns[i].getCompressionSequence();
        //set the default byte telling if uncompressed the compressed Flag
        uint64_t previousOffset = compressedOffset;
        //skip header data
        compressedOffset += sizeof(BlockHeader) + sizeof(uint16_t)*sequence.size();

        for (uint32_t j=0;j<sequence.size(); j++)
        {
            switch (sequence[j])
            {
                case FITS::kFactRaw:
//                    if (head.numProcs == 1)
                        compressedOffset += compressUNCOMPRESSED(&(_compressedBuffer[threadIndex][compressedOffset]), &(_transposedBuffer[threadIndex][offset]), thisRoundNumRows, _columns[i].sizeOfElems(), _columns[i].numElems());
                break;
                case FITS::kFactSmoothing:
                        applySMOOTHING(&(_compressedBuffer[threadIndex][compressedOffset]), &(_transposedBuffer[threadIndex][offset]), thisRoundNumRows, _columns[i].sizeOfElems(), _columns[i].numElems());
                break;
                case FITS::kFactHuffman16:
                    if (head.ordering == FITS::kOrderByCol)
                        compressedOffset += compressHUFFMAN(&(_compressedBuffer[threadIndex][compressedOffset]), &(_transposedBuffer[threadIndex][offset]), thisRoundNumRows, _columns[i].sizeOfElems(), _columns[i].numElems());
                    else
                        compressedOffset += compressHUFFMAN(&(_compressedBuffer[threadIndex][compressedOffset]), &(_transposedBuffer[threadIndex][offset]), _columns[i].numElems(), _columns[i].sizeOfElems(),  thisRoundNumRows);
                break;
                default:
                    cout << "ERROR: Unkown compression sequence entry: " << sequence[i] << endl;
                break;
            }
        }

        //check if compressed size is larger than uncompressed
        if (sequence[0] != FITS::kFactRaw &&
            compressedOffset - previousOffset > _columns[i].sizeOfElems()*_columns[i].numElems()*thisRoundNumRows+sizeof(BlockHeader)+sizeof(uint16_t)*sequence.size())
        {//if so set flag and redo it uncompressed
            cout << "REDOING UNCOMPRESSED" << endl;
            compressedOffset = previousOffset + sizeof(BlockHeader) + 1;
            compressedOffset += compressUNCOMPRESSED(&(_compressedBuffer[threadIndex][compressedOffset]), &(_transposedBuffer[threadIndex][offset]), thisRoundNumRows, _columns[i].sizeOfElems(), _columns[i].numElems());
            BlockHeader he;
            he.size = compressedOffset - previousOffset;
            he.numProcs = 1;
            he.ordering = FITS::kOrderByRow;
            memcpy(&(_compressedBuffer[threadIndex][previousOffset]), (char*)(&he), sizeof(BlockHeader));
            _compressedBuffer[threadIndex][previousOffset+sizeof(BlockHeader)] = FITS::kFactRaw;
            offset += thisRoundNumRows*_columns[i].sizeOfElems()*_columns[i].numElems();
           _catalog[currentCatalogRow][i].first = compressedOffset - _catalog[currentCatalogRow][i].second;
           continue;
        }
        head.size = compressedOffset - previousOffset;
        memcpy(&(_compressedBuffer[threadIndex][previousOffset]), (char*)(&head), sizeof(BlockHeader));
        memcpy(&(_compressedBuffer[threadIndex][previousOffset+sizeof(BlockHeader)]), sequence.data(), sizeof(uint16_t)*sequence.size());

         offset += thisRoundNumRows*_columns[i].sizeOfElems()*_columns[i].numElems();
        _catalog[currentCatalogRow][i].first = compressedOffset - _catalog[currentCatalogRow][i].second;
    }

    TileHeader tHead(thisRoundNumRows, compressedOffset);
    memcpy(_compressedBuffer[threadIndex], &tHead, sizeof(TileHeader));
    return compressedOffset;
}

/****************************************************************
 *              WRITE COMPRESS DATA TO DISK
 ****************************************************************/
bool CompressedFitsWriter::writeCompressedDataToDisk(uint32_t threadID, uint32_t sizeToWrite)
{
    char* checkSumPointer = _compressedBuffer[threadID];
    int32_t extraBytes = 0;
    uint32_t sizeToChecksum = sizeToWrite;
    if (_checkOffset != 0)
    {//should we extend the array to the left ?
        sizeToChecksum += _checkOffset;
        checkSumPointer -= _checkOffset;
        memset(checkSumPointer, 0, _checkOffset);
    }
    if (sizeToChecksum%4 != 0)
    {//should we extend the array to the right ?
        extraBytes = 4 - (sizeToChecksum%4);
        memset(checkSumPointer+sizeToChecksum, 0,extraBytes);
        sizeToChecksum += extraBytes;
    }
    //do the checksum
    _checksum.add(checkSumPointer, sizeToChecksum);
//    cout << endl << "Checksum: " << _checksum.val() << endl;
    _checkOffset = (4 - extraBytes)%4;
    //write data to disk
    _file.write(_compressedBuffer[threadID], sizeToWrite);
    return _file.good();
}

/****************************************************************
 *              WRITER THREAD LOOP
 ****************************************************************/
void* CompressedFitsWriter::threadFunction(void* context)
{
    CompressedFitsWriter* myself =static_cast<CompressedFitsWriter*>(context);

    uint32_t myID = 0;
    pthread_mutex_lock(&(myself->_mutex));
    myID = myself->_threadIndex++;
    pthread_mutex_unlock(&(myself->_mutex));
    uint32_t threadToWaitForBeforeWriting = (myID == 0) ? myself->_numThreads-1 : myID-1;

    while (myself->_threadStatus[myID] != _THREAD_EXIT_)
    {
        while (myself->_threadStatus[myID] == _THREAD_WAIT_)
            usleep(100000);
        if (myself->_threadStatus[myID] != _THREAD_COMPRESS_)
            continue;
        uint32_t numBytes = myself->compressBuffer(myID);
        myself->_threadStatus[myID] = _THREAD_WRITE_;

        //wait for the previous data to be written
        while (myself->_threadIndex != threadToWaitForBeforeWriting)
            usleep(1000);
        //do the actual writing to disk
        pthread_mutex_lock(&(myself->_mutex));
        myself->writeCompressedDataToDisk(myID, numBytes);
        myself->_threadIndex = myID;
        pthread_mutex_unlock(&(myself->_mutex));
        myself->_threadStatus[myID] = _THREAD_WAIT_;
    }
    return NULL;
}

/****************************************************************
 *                 PRINT USAGE
 ****************************************************************/
void printUsage()
{
    cout << endl;
    cout << "The FACT-Fits compressor reads an input Fits file from FACT"
            " and compresses it.\n It can use a drs calibration in order to"
            " improve the compression ratio. If so, the input calibration"
            " is embedded into the compressed file.\n"
            " By default, the Data column will be compressed using SMOOTHMAN (Thomas' algorithm)"
            " while other columns will be compressed with the AMPLITUDE coding (Veritas)"
            "Usage: Compressed_Fits_Test <inputFile>";
    cout << endl;
}

/****************************************************************
 *                  PRINT HELP
 ****************************************************************/
void printHelp()
{
    cout << endl;
    cout << "The inputFile is required. It must have fits in its filename and the compressed file will be written in the same folder. "
            "The fz extension will be added, replacing the .gz one if required \n"
            "If output is specified, then it will replace the automatically generated output filename\n"
            "If --drs, followed by a drs calib then it will be applied to the data before compressing\n"
            "rowPerTile can be used to tune how many rows are in each tile. Default is 100\n"
            "threads gives the number of threads to use. Cannot be less than the default (1)\n"
            "compression explicitely gives the compression scheme to use for a given column. The syntax is:\n"
            "<ColumnName>=<CompressionScheme> with <CompressionScheme> one of the following:\n"
            "UNCOMPRESSED\n"
            "AMPLITUDE\n"
            "HUFFMAN\n"
            "SMOOTHMAN\n"
            "INT_WAVELET\n"
            "\n"
            "--quiet removes any textual output, except error messages\n"
            "--verify makes the compressor check the compressed data. It will read it back, and compare the reconstructed CHECKSUM and DATASUM with the original file values."
            ;
    cout << endl << endl;
}

/****************************************************************
 *                  SETUP CONFIGURATION
 ****************************************************************/
void setupConfiguration(Configuration& conf)
{
    po::options_description configs("FitsCompressor options");
    configs.add_options()
            ("inputFile,i",   vars<string>(),      "Input file")
            ("drs,d",         var<string>(),       "Input drs calibration file")
            ("rowPerTile,r",  var<unsigned int>(), "Number of rows per tile. Default is 100")
            ("output,o",      var<string>(),       "Output file. If empty, .fz is appened to the original name")
            ("threads,t",     var<unsigned int>(), "Number of threads to use for compression")
            ("compression,c", vars<string>(),      "which compression to use for which column. Syntax <colName>=<compressionScheme>")
            ("quiet,q",       po_switch(),         "Should the program display any text at all ?")
            ("verify,v",      po_switch(),         "Should we verify the data that has been compressed ?")
            ;
    po::positional_options_description positional;
    positional.add("inputFile", -1);
    conf.AddOptions(configs);
    conf.SetArgumentPositions(positional);
}

/****************************************************************
 *                  MAIN
 ****************************************************************/
int main(int argc, const char** argv)
{
     Configuration conf(argv[0]);
     conf.SetPrintUsage(printUsage);
     setupConfiguration(conf);

     if (!conf.DoParse(argc, argv, printHelp))
         return -1;

     //initialize the file names to nothing.
     string fileNameIn = "";
     string fileNameOut = "";
     string drsFileName = "";
     uint32_t numRowsPerTile = 100;
     bool displayText=true;

    //parse configuration
    if (conf.Get<bool>("quiet")) displayText = false;
    const vector<string> inputFileNameVec = conf.Vec<string>("inputFile");
    if (inputFileNameVec.size() != 1)
    {
       cout << "Error: ";
       if (inputFileNameVec.size() == 0) cout << "no";
       else cout << inputFileNameVec.size();
       cout << " input file(s) given. Expected one. Aborting. Input:" << endl;;
       for (unsigned int i=0;i<inputFileNameVec.size(); i++)
           cout << inputFileNameVec[i] << endl;
       return -1;
    }

    //Assign the input filename
    fileNameIn = inputFileNameVec[0];

    //Check if we have a drs calib too
    if (conf.Has("drs")) drsFileName = conf.Get<string>("drs");

    //Should we verify the data ?
    bool verifyDataPlease = false;
    if (conf.Has("verify")) verifyDataPlease = conf.Get<bool>("verify");


    //should we use a specific output filename ?
    if (conf.Has("output"))
        fileNameOut = conf.Get<string>("output");
    else
    {
        size_t pos = fileNameIn.find(".fits.fz");
        if (pos == string::npos)
        {
            cout << "ERROR: input file does not seems ot be fits. Aborting." << endl;
            return -1;
        }
        fileNameOut = fileNameIn.substr(0, pos) + ".fits";
    }


    //should we use specific compression on some columns ?
    const vector<string> columnsCompression = conf.Vec<string>("compression");

    //split up values between column names and compression scheme
    vector<std::pair<string, string>> compressions;
        for (unsigned int i=0;i<columnsCompression.size();i++)
    {
        size_t pos = columnsCompression[i].find_first_of("=");
        if (pos == string::npos)
        {
            cout << "ERROR: Something wrong occured while parsing " << columnsCompression[i] << ". Aborting." << endl;
            return -1;
        }
        string comp = columnsCompression[i].substr(pos+1);
        if (comp != "UNCOMPRESSED" && comp != "AMPLITUDE" && comp != "HUFFMAN" &&
            comp != "SMOOTHMAN" && comp != "INT_WAVELET")
        {
            cout << "Unkown compression scheme requested (" << comp << "). Aborting." << endl;
            return -1;
        }
        compressions.push_back(make_pair(columnsCompression[i].substr(0, pos), comp));
    }

    //How many rows per tile should we use ?
    if (conf.Has("rowPerTile")) numRowsPerTile = conf.Get<unsigned int>("rowPerTile");

    //////////////////////////////////////////////////////////////////////////////////////
    //  Done reading configuration. Open relevant files
    //////////////////////////////////////////////////////////////////////////////////////


    //Open input's fits file
    factfits inFile(fileNameIn, "", "Events", false);

    if (!inFile.IsCompressedFITS())
    {
        cout << "ERROR: input file is NOT a compressed fits. Cannot be decompressed: Aborting." << endl;
        return -1;
    }

    //decide how many tiles should be put in the compressed file
    uint32_t originalNumRows = inFile.GetNumRows();
    uint32_t numTiles = (originalNumRows%numRowsPerTile) ? (originalNumRows/numRowsPerTile)+1 : originalNumRows/numRowsPerTile;
//    CompressedFitsWriter outFile(numTiles, numRowsPerTile);

    //should we use a specific number of threads for compressing ?
    unsigned int numThreads = 1;
    if (conf.Has("threads"))
    {
        numThreads = conf.Get<unsigned int>("threads");
//        outFile.setNumWorkingThreads(numThreads);
    }



    //Because the file to open MUST be given by the constructor, I must use a pointer instead
    factfits* drsFile = NULL;
    //try to open the Drs file. If any.
    if (drsFileName != "")
    {
        try
        {
            drsFile = new factfits(drsFileName);
        }
        catch (...)
        {
            cout << "Error: could not open " << drsFileName << " for calibration" << endl;
            return -1;
        }
    }

    if (displayText)
    {
        cout << endl;
        cout << "**********************" << endl;
        cout << "Will decompress from    : " << fileNameIn << endl;
        cout << "to                    : " << fileNameOut << endl;
        cout << "**********************" << endl;
        cout << endl;
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //  Done opening input files. Allocate memory and configure output file
    //////////////////////////////////////////////////////////////////////////////////////

    //allocate the buffer for temporary storage of each read/written row
    uint32_t rowWidth = inFile.GetUInt("ZNAXIS1");
    char* buffer = new char[rowWidth + 12];
    memset(buffer, 0, 4);
    buffer = buffer+4;

    //get the source columns
    const fits::Table::Columns& columns = inFile.GetColumns();
    const fits::Table::SortedColumns& sortedColumns = inFile.GetSortedColumns();
    if (displayText)
        cout << "Input file has " << columns.size() << " columns and " << inFile.GetNumRows() << " rows" << endl;


    //////////////////////////////////////////////////////////////////////////////////////
    //  Done configuring compression. Do the real job now !
    //////////////////////////////////////////////////////////////////////////////////////
    vector<void*>   readPointers;
    vector<int32_t> readOffsets;
    vector<int32_t> readElemSize;
    vector<int32_t> readNumElems;
    //Get table name for later use in case the compressed file is to be verified
    string tableName = inFile.GetStr("EXTNAME");


    //and the header of the compressed file
    const fits::Table::Keys& header2 = inFile.GetKeys();

    //get a non-compressed writer
    ofits reconstructedFile;

    //figure out its name: /dev/null unless otherwise specified
    string reconstructedName = fileNameOut;
    reconstructedFile.open(reconstructedName.c_str(), false);

    //reconstruct the original columns from the compressed file.
    string origChecksumStr;
    string origDatasum;

    //reset tablename value so that it is re-read from compressed table's header
    tableName = "";

    /************************************************************************************
     *  Reconstruction setup done. Rebuild original header
     ************************************************************************************/

    //re-tranlate the keys
    for (fits::Table::Keys::const_iterator it=header2.begin(); it!= header2.end(); it++)
    {
        string k = it->first;
        if (k == "XTENSION" || k == "BITPIX"  || k == "PCOUNT"   || k == "GCOUNT" ||
            k == "TFIELDS"  || k == "ZTABLE"  || k == "ZNAXIS1"  || k == "ZNAXIS2" ||
            k == "ZHEAPPTR" || k == "ZPCOUNT" || k == "ZTILELEN" || k == "THEAP" ||
            k == "CHECKSUM" || k == "DATASUM" || k == "FCTCPVER" || k == "ZHEAP")
        {
            continue;
        }

        if (k == "ZCHKSUM")
        {
            reconstructedFile.SetKeyComment("CHECKSUM", it->second.comment);
            origChecksumStr = it->second.value;
            continue;
        }
        if (k == "RAWSUM")
        {
            continue;
        }

        if (k == "ZDTASUM")
        {
            reconstructedFile.SetKeyComment("DATASUM",  it->second.comment);
            origDatasum = it->second.value;
            continue;
        }

        if (k == "EXTNAME")
        {
            tableName = it->second.value;
        }

        k = k.substr(0,5);

        if (k == "TTYPE")
        {//we have an original column name here.
         //manually deal with these in order to preserve the ordering (easier than re-constructing yet another list on the fly)
            continue;
        }

        if (k == "TFORM" || k == "NAXIS" || k == "ZCTYP" )
        {
            continue;
        }

        if (k == "ZFORM" || k == "ZTYPE")
        {
            string tmpKey = it->second.fitsString;
            tmpKey[0] = 'T';
            reconstructedFile.SetKeyFromFitsString(tmpKey);
            continue;
        }

        reconstructedFile.SetKeyFromFitsString(it->second.fitsString);
    }

    if (tableName == "")
    {
        cout << "Error: table name from file " << fileNameOut << " could not be found. Aborting" << endl;
        return -1;
    }

    //Restore the original columns
    for (uint32_t numCol=1; numCol<10000; numCol++)
    {
        ostringstream str;
        str << numCol;
        if (!inFile.HasKey("TTYPE"+str.str())) break;

        string ttype    = inFile.GetStr("TTYPE"+str.str());
        string tform    = inFile.GetStr("ZFORM"+str.str());
        char   type     = tform[tform.size()-1];
        string number   = tform.substr(0, tform.size()-1);
        int    numElems = atoi(number.c_str());

        if (number == "") numElems=1;

        reconstructedFile.AddColumn(numElems, type, ttype, "", "", false);
    }

    reconstructedFile.WriteTableHeader(tableName.c_str());

    /************************************************************************************
     *  Original header restored. Do the data
     ************************************************************************************/

    //set pointers to the readout data to later be able to gather it to "buffer".
    readPointers.clear();
    readOffsets.clear();
    readElemSize.clear();
    readNumElems.clear();
    for (fits::Table::Columns::const_iterator it=columns.begin(); it!= columns.end(); it++)
    {
        readPointers.push_back(inFile.SetPtrAddress(it->first));
        readOffsets.push_back(it->second.offset);
        readElemSize.push_back(it->second.size);
        readNumElems.push_back(it->second.num);
    }

    //do the actual reconstruction work
    uint32_t i=1;
    while (i<=inFile.GetNumRows() && inFile.GetNextRow())
    {
        int count=0;
        for (fits::Table::Columns::const_iterator it=columns.begin(); it!= columns.end();it++)
        {
            memcpy(&buffer[readOffsets[count]], readPointers[count], readElemSize[count]*readNumElems[count]);
            count++;
        }
        if (displayText) cout << "\r Row " << i << flush;
        reconstructedFile.WriteRow(buffer, rowWidth);
        if (!reconstructedFile.good())
        {
            cout << "ERROR: no space left on device (probably)" << endl;
            return -1;
        }
        i++;
    }

    if (displayText) cout << endl;

    //close reconstruction input and output
//    Do NOT close the verify file, otherwise data cannot be flushed to copy file
//    verifyFile.close();
    if (!inFile.IsFileOk())
        cout << "ERROR: file checksums seems wrong" << endl;

    if (!reconstructedFile.close())
    {
        cout << "ERROR: disk probably full..." <<endl;
        return -1;
    }

    //get original and reconstructed checksum and datasum
    std::pair<string, int> origChecksum = make_pair(origChecksumStr, atoi(origDatasum.c_str()));
    std::pair<string, int> newChecksum = reconstructedFile.GetChecksumData();

    //verify that no mistake was made
    if (origChecksum.second != newChecksum.second)
    {
        cout << "ERROR: datasums are NOT identical: " << (uint32_t)(origChecksum.second) << " vs " << (uint32_t)(newChecksum.second) << endl;
        return -1;
    }
    if (origChecksum.first != newChecksum.first)
    {
        cout << "WARNING: checksums are NOT Identical: " << origChecksum.first << " vs " << newChecksum.first << endl;
    }
    else
    {
        if (true) cout << "Ok" << endl;
    }

    buffer = buffer-4;
    delete[] buffer;
    return 0;
}

