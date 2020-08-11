#ifndef FACT_zofits
#define FACT_zofits

/*
 * zofits.h
 *
 *  FACT native compressed FITS writer
 *      Author: lyard
 */
#include "ofits.h"
#include "huffman.h"
#include "Queue.h"
#include "MemoryManager.h"

#ifdef HAVE_BOOST_THREAD
#include <boost/thread.hpp>
#else
#include <unistd.h>
#endif

#if defined(__CINT__) || !defined(__EXCEPTIONS)
namespace std
{
    typedef void* exception_ptr;
};
#endif

class zofits : public ofits
{
#ifdef __MARS__ // Needed by CINT to access the structures
public:
#endif
        /// Overriding of the begin() operator to get the smallest item in the list instead of the true begin
        template<class S>
        struct QueueMin : std::list<S>
        {
            typename std::list<S>::iterator begin()
            {
                return min_element(std::list<S>::begin(), std::list<S>::end());
            }
        };

#ifdef __CINT__
        // CINT doesn't like the packed attribute...
        // Therefore we give another hint of the size of the structure
        struct CatalogEntry { char dummy[16]; };
#else
        //catalog types
        struct CatalogEntry
        {
            CatalogEntry(int64_t f=0, int64_t s=0) : first(f), second(s) { }
            int64_t first;   ///< Size of this column in the tile
            int64_t second;  ///< offset of this column in the tile, from the start of the heap area
        } __attribute__((__packed__));
#endif

        typedef std::vector<CatalogEntry> CatalogRow;
        typedef std::list<CatalogRow>     CatalogType;

        /// Parameters required to write a tile to disk
        struct WriteTarget
        {
            bool operator < (const WriteTarget& other) const
            {
                return tile_num < other.tile_num;
            }

            WriteTarget() { }
            WriteTarget(const WriteTarget &t, uint32_t sz) : tile_num(t.tile_num), size(sz), data(t.data) { }

            uint32_t              tile_num; ///< Tile index of the data (to make sure that they are written in the correct order)
            uint32_t              size;     ///< Size to write
            std::shared_ptr<char> data;     ///< Memory block to write
        };


        /// Parameters required to compress a tile of data
        struct CompressionTarget
        {
            CompressionTarget(CatalogRow& r) : catalog_entry(r)
            {}

            CatalogRow&           catalog_entry;   ///< Reference to the catalog entry to deal with
            std::shared_ptr<char> src;             ///< Original data
            std::shared_ptr<char> transposed_src;  ///< Transposed data
            WriteTarget           target;          ///< Compressed data
            uint32_t              num_rows;        ///< Number of rows to compress
         };

public:
        /// static setter for the default number of threads to use. -1 means all available physical cores
        static uint32_t DefaultNumThreads(const uint32_t &_n=-2) { static uint32_t n=0; if (int32_t(_n)>-2) n=_n; return n; }
        static uint32_t DefaultMaxMemory(const uint32_t &_n=0) { static uint32_t n=1000000; if (_n>0) n=_n; return n; }
        static uint32_t DefaultMaxNumTiles(const uint32_t &_n=0) { static uint32_t n=1000; if (_n>0) n=_n; return n; }
        static uint32_t DefaultNumRowsPerTile(const uint32_t &_n=0) { static uint32_t n=100; if (_n>0) n=_n; return n; }

        /// constructors
        /// @param numTiles how many data groups should be pre-reserved ?
        /// @param rowPerTile how many rows will be grouped together in a single tile
        /// @param maxUsableMem how many bytes of memory can be used by the compression buffers
        zofits(uint32_t numTiles    = DefaultMaxNumTiles(),
               uint32_t rowPerTile  = DefaultNumRowsPerTile(),
               uint32_t maxUsableMem= DefaultMaxMemory()) : ofits(),
            fMemPool(0, size_t(maxUsableMem)*1000),
            fWriteToDiskQueue(std::bind(&zofits::WriteBufferToDisk, this, std::placeholders::_1), false)
        {
            InitMemberVariables(numTiles, rowPerTile, size_t(maxUsableMem)*1000);
            SetNumThreads(DefaultNumThreads());
        }

        /// @param fname the target filename
        /// @param numTiles how many data groups should be pre-reserved ?
        /// @param rowPerTile how many rows will be grouped together in a single tile
        /// @param maxUsableMem how many bytes of memory can be used by the compression buffers
        zofits(const char* fname,
               uint32_t numTiles    = DefaultMaxNumTiles(),
               uint32_t rowPerTile  = DefaultNumRowsPerTile(),
               uint32_t maxUsableMem= DefaultMaxMemory()) : ofits(),
            fMemPool(0, size_t(maxUsableMem)*1000),
            fWriteToDiskQueue(std::bind(&zofits::WriteBufferToDisk, this, std::placeholders::_1), false)
        {
            InitMemberVariables(numTiles, rowPerTile, size_t(maxUsableMem)*1000);
            SetNumThreads(DefaultNumThreads());
            open(fname);
        }

        zofits(const std::string &fname,
               uint32_t numTiles    = DefaultMaxNumTiles(),
               uint32_t rowPerTile  = DefaultNumRowsPerTile(),
               uint32_t maxUsableMem= DefaultMaxMemory()) : ofits(),
            fMemPool(0, size_t(maxUsableMem)*1000),
            fWriteToDiskQueue(std::bind(&zofits::WriteBufferToDisk, this, std::placeholders::_1), false)
        {
            InitMemberVariables(numTiles, rowPerTile, size_t(maxUsableMem)*1000);
            SetNumThreads(DefaultNumThreads());
            open(fname);
        }

        //initialization of member variables
        /// @param nt number of tiles
        /// @param rpt number of rows per tile
        /// @param maxUsableMem max amount of RAM to be used by the compression buffers
        void InitMemberVariables(const uint32_t nt=0, const uint32_t rpt=0, const uint64_t maxUsableMem=0)
        {
            fCheckOffset = 0;
            fNumQueues   = 0;

            fNumTiles       = nt==0 ? 1 : nt;
            fNumRowsPerTile = rpt;

            fRealRowWidth     = 0;
            fCatalogOffset    = 0;
            fCatalogSize      = 0;

            fMaxUsableMem = maxUsableMem;
#ifdef __EXCEPTIONS
            fThreadsException = std::exception_ptr();
#endif
            fErrno = 0;
        }

        /// write the header of the binary table
        /// @param name the name of the table to be created
        /// @return the state of the file
        virtual bool WriteTableHeader(const char* name="DATA")
        {
            reallocateBuffers();

            SetInt("ZNAXIS1", fRealRowWidth);

            ofits::WriteTableHeader(name);

            fCompressionQueues.front().setPromptExecution(fNumQueues==0);
            fWriteToDiskQueue.setPromptExecution(fNumQueues==0);

            if (fNumQueues != 0)
            {
                //start the compression queues
                for (auto it=fCompressionQueues.begin(); it!= fCompressionQueues.end(); it++)
                    it->start();

                //start the disk writer
                fWriteToDiskQueue.start();
            }

            //mark that no tile has been written so far
            fLatestWrittenTile = -1;

            //no wiring error (in the writing of the data) has occured so far
            fErrno = 0;

            return good();
        }

        /// open a new file.
        /// @param filename the name of the file
        /// @param Whether or not the name of the extension should be added or not
        void open(const char* filename, bool addEXTNAMEKey=true)
        {
            ofits::open(filename, addEXTNAMEKey);

            //add compression-related header entries
            SetBool( "ZTABLE",   true,            "Table is compressed");
            SetInt(  "ZNAXIS1",  0,               "Width of uncompressed rows");
            SetInt(  "ZNAXIS2",  0,               "Number of uncompressed rows");
            SetInt(  "ZPCOUNT",  0,               "");
            SetInt(  "ZHEAPPTR", 0,               "");
            SetInt(  "ZTILELEN", fNumRowsPerTile, "Number of rows per tile");
            SetInt(  "THEAP",    0,               "");
            SetStr(  "RAWSUM",   "         0",    "Checksum of raw little endian data");
            SetFloat("ZRATIO",   0,               "Compression ratio");
            SetInt(  "ZSHRINK",  1,               "Catalog shrink factor");

            fCatalogSize   = 0;
            fRealRowWidth  = 0;
            fCatalogOffset = 0;
            fCatalogSize   = 0;
            fCheckOffset   = 0;

            fRealColumns.clear();
            fCatalog.clear();
            fCatalogSum.reset();
            fRawSum.reset();
        }

        void open(const std::string &filename, bool addEXTNAMEKey=true)
        {
            open(filename.c_str(), addEXTNAMEKey);
        }

        /// Super method. does nothing as zofits does not know about DrsOffsets
        /// @return the state of the file
        virtual bool WriteDrsOffsetsTable()
        {
            return good();
        }

        /// Returns the number of bytes per uncompressed row
        /// @return number of bytes per uncompressed row
        uint32_t GetBytesPerRow() const
        {
            return fRealRowWidth;
        }

        /// Write the data catalog
        /// @return the state of the file
        bool WriteCatalog()
        {
            const uint32_t one_catalog_row_size = fTable.num_cols*2*sizeof(uint64_t);
            const uint32_t total_catalog_size   = fNumTiles*one_catalog_row_size;

            // swap the catalog bytes before writing
            std::vector<char> swapped_catalog(total_catalog_size);

            uint32_t shift = 0;
            for (auto it=fCatalog.cbegin(); it!=fCatalog.cend(); it++)
            {
                revcpy<sizeof(uint64_t)>(swapped_catalog.data() + shift, (char*)(it->data()), fTable.num_cols*2);
                shift += one_catalog_row_size;
            }

            if (fCatalogSize < fNumTiles)
                memset(swapped_catalog.data()+shift, 0, total_catalog_size-shift);

            // first time writing ? remember where we are
            if (fCatalogOffset == 0)
                fCatalogOffset = tellp();

            // remember where we came from
            const off_t where_are_we = tellp();

            // write to disk
            seekp(fCatalogOffset);
            write(swapped_catalog.data(), total_catalog_size);

            if (where_are_we != fCatalogOffset)
                seekp(where_are_we);

            // udpate checksum
            fCatalogSum.reset();
            fCatalogSum.add(swapped_catalog.data(), total_catalog_size);

            return good();
        }

        /// Applies the DrsOffsets calibration to the data. Does nothing as zofits knows nothing about drsoffsets.
        virtual void DrsOffsetCalibrate(char* )
        {

        }

        CatalogRow& AddOneCatalogRow()
        {
            // add one row to the catalog
            fCatalog.emplace_back(CatalogRow());
            fCatalog.back().resize(fTable.num_cols);
            for (auto it=fCatalog.back().begin(); it != fCatalog.back().end(); it++)
                *it = CatalogEntry(0,0);

            fCatalogSize++;

            return fCatalog.back();
        }

        /// write one row of data
        /// Note, in a multi-threaded environment (NumThreads>0), the return code should be checked rather
        /// than the badbit() of the stream (it might have been set by a thread before the errno has been set)
        /// errno will then contain the correct error number of the last error which happened during writing.
        /// @param ptr the source buffer
        /// @param the number of bytes to write
        /// @return the state of the file. WARNING: with multithreading, this will most likely be the state of the file before the data is actually written
        bool WriteRow(const void* ptr, size_t cnt, bool = true)
        {
            if (cnt != fRealRowWidth)
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("Wrong size of row given to WriteRow");
#else
                gLog << ___err___ << "ERROR - Wrong size of row given to WriteRow" << std::endl;
                return false;
#endif
            }

#ifdef __EXCEPTIONS
            //check if something hapenned while the compression threads were working
            //if so, re-throw the exception that was generated
            if (fThreadsException != std::exception_ptr())
                std::rethrow_exception(fThreadsException);
#endif

            //copy current row to pool or rows waiting for compression
            char* target_location = fSmartBuffer.get() + fRealRowWidth*(fTable.num_rows%fNumRowsPerTile);
            memcpy(target_location, ptr, fRealRowWidth);

            //for now, make an extra copy of the data, for RAWSUM checksuming.
            //Ideally this should be moved to the threads
            //However, because the RAWSUM must be calculated before the tile is transposed, I am not sure whether
            //one extra memcpy per row written is worse than 100 rows checksumed when the tile is full....
            const uint32_t rawOffset = (fTable.num_rows*fRealRowWidth)%4;
            char* buffer = fRawSumBuffer.data() + rawOffset;
            auto ib = fRawSumBuffer.begin();
            auto ie = fRawSumBuffer.rbegin();
            *ib++ = 0;
            *ib++ = 0;
            *ib++ = 0;
            *ib   = 0;

            *ie++ = 0;
            *ie++ = 0;
            *ie++ = 0;
            *ie   = 0;

            memcpy(buffer, ptr, fRealRowWidth);

            fRawSum.add(fRawSumBuffer, false);

            fTable.num_rows++;

            if (fTable.num_rows % fNumRowsPerTile != 0)
            {
                errno = fErrno;
                return errno==0;
            }

            // use the least occupied queue
            const auto imin = std::min_element(fCompressionQueues.begin(), fCompressionQueues.end());

            if (!imin->emplace(InitNextCompression()))
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("The compression queues are not started. Did you close the file before writing this row?");
#else
                gLog << ___err___ << "The compression queues are not started. Did you close the file before writing this row?" << std::endl;
                errno = 0;
                return false;
#endif
            }

            errno = fErrno;
            return errno==0;
        }

        /// update the real number of rows
        void FlushNumRows()
        {
            SetInt("NAXIS2", (fTable.num_rows + fNumRowsPerTile-1)/fNumRowsPerTile);
            SetInt("ZNAXIS2", fTable.num_rows);
            FlushHeader();
        }

        /// Setup the environment to compress yet another tile of data
        /// @param target the struct where to host the produced parameters
        CompressionTarget InitNextCompression()
        {
            CompressionTarget target(AddOneCatalogRow());

            //fill up compression target
            target.src            = fSmartBuffer;
            target.transposed_src = fMemPool.malloc();
            target.num_rows       = fTable.num_rows;

            //fill up write to disk target
            WriteTarget &write_target = target.target;
            write_target.tile_num = (fTable.num_rows-1)/fNumRowsPerTile;
            write_target.size     = 0;
            write_target.data     = fMemPool.malloc();

            //get a new buffer to host the incoming data
            fSmartBuffer = fMemPool.malloc();

            return target;
        }

        /// Shrinks a catalog that is too long to fit into the reserved space at the beginning of the file.
        uint32_t ShrinkCatalog()
        {
            //add empty row to get either the target number of rows, or a multiple of the allowed size
            for (uint32_t i=0;i<fCatalogSize%fNumTiles;i++)
                AddOneCatalogRow();

            //did we write more rows than what the catalog could host ?
            if (fCatalogSize <= fNumTiles) // nothing to do
                return 1;

            //always exact as extra rows were added just above
            const uint32_t shrink_factor = fCatalogSize / fNumTiles; 

            //shrink the catalog !
            uint32_t entry_id = 1;
            auto it = fCatalog.begin();
            it++;
            for (; it != fCatalog.end(); it++)
            {
                if (entry_id >= fNumTiles)
                    break;

                const uint32_t target_id = entry_id*shrink_factor;

                auto jt = it;
                for (uint32_t i=0; i<target_id-entry_id; i++)
                    jt++;

                *it = *jt;

                entry_id++;
            }

            const uint32_t num_tiles_to_remove = fCatalogSize-fNumTiles;

            //remove the too many entries
            for (uint32_t i=0;i<num_tiles_to_remove;i++)
            {
                fCatalog.pop_back();
                fCatalogSize--;
            }

            //update header keywords
            fNumRowsPerTile *= shrink_factor;

            SetInt("ZTILELEN", fNumRowsPerTile);
            SetInt("ZSHRINK",  shrink_factor);

            return shrink_factor;
        }

        /// close an open file.
        /// @return the state of the file
        bool close()
        {
            // stop compression and write threads
            for (auto it=fCompressionQueues.begin(); it != fCompressionQueues.end(); it++)
                it->wait();

            fWriteToDiskQueue.wait();

            if (tellp() < 0)
                return false;

#ifdef __EXCEPTIONS
            //check if something hapenned while the compression threads were working
            //if so, re-throw the exception that was generated
            if (fThreadsException != std::exception_ptr())
                std::rethrow_exception(fThreadsException);
#endif

            //write the last tile of data (if any)
            if (fErrno==0 && fTable.num_rows%fNumRowsPerTile!=0)
            {
                fWriteToDiskQueue.enablePromptExecution();
                fCompressionQueues.front().enablePromptExecution();
                fCompressionQueues.front().emplace(InitNextCompression());
            }

            AlignTo2880Bytes();

            int64_t heap_size = 0;
            int64_t compressed_offset = 0;
            for (auto it=fCatalog.begin(); it!= fCatalog.end(); it++)
            {
                compressed_offset += sizeof(FITS::TileHeader);
                heap_size         += sizeof(FITS::TileHeader);
                for (uint32_t j=0; j<it->size(); j++)
                {
                    heap_size += (*it)[j].first;
                    (*it)[j].second = compressed_offset;
                    compressed_offset += (*it)[j].first;
                    if ((*it)[j].first == 0)
                        (*it)[j].second = 0;
                }
            }

            const uint32_t shrink_factor = ShrinkCatalog();

            //update header keywords
            SetInt("ZNAXIS1", fRealRowWidth);
            SetInt("ZNAXIS2", fTable.num_rows);

            SetInt("ZHEAPPTR", fCatalogSize*fTable.num_cols*sizeof(uint64_t)*2);

            const uint32_t total_num_tiles_written = (fTable.num_rows + fNumRowsPerTile-1)/fNumRowsPerTile;
            const uint32_t total_catalog_width     = 2*sizeof(int64_t)*fTable.num_cols;

            SetInt("THEAP",  total_num_tiles_written*total_catalog_width);
            SetInt("NAXIS1", total_catalog_width);
            SetInt("NAXIS2", total_num_tiles_written);
            SetStr("RAWSUM", std::to_string((long long int)(fRawSum.val())));

            const float compression_ratio = (float)(fRealRowWidth*fTable.num_rows)/(float)heap_size;
            SetFloat("ZRATIO", compression_ratio);

            //add to the heap size the size of the gap between the catalog and the actual heap
            heap_size += (fCatalogSize - total_num_tiles_written)*fTable.num_cols*sizeof(uint64_t)*2;

            SetInt("PCOUNT", heap_size, "size of special data area");

            //Just for updating the fCatalogSum value
            WriteCatalog();

            fDataSum += fCatalogSum;

            const Checksum checksm = UpdateHeaderChecksum();

            if (!fFilebuf.close())
                setstate(ios_base::failbit);

            fSmartBuffer = std::shared_ptr<char>();

            //restore the number of rows per tile in case the catalog has been shrinked
            if (shrink_factor != 1)
                fNumRowsPerTile /= shrink_factor;

            if ((checksm+fDataSum).valid())
                return true;

            std::ostringstream sout;
            sout << "Checksum (" << std::hex << checksm.val() << ") invalid.";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        /// Overload of the ofits method. Just calls the zofits specific one with default, uncompressed options for this column
        bool AddColumn(uint32_t cnt, char typechar, const std::string& name, const std::string& unit,
                       const std::string& comment="", bool addHeaderKeys=true)
        {
            return AddColumn(FITS::kFactRaw, cnt, typechar, name, unit, comment, addHeaderKeys);
        }

        /// Overload of the simplified compressed version
        bool AddColumn(const FITS::Compression &comp, uint32_t cnt, char typechar, const std::string& name,
                       const std::string& unit, const std::string& comment="", bool addHeaderKeys=true)
        {
            if (!ofits::AddColumn(1, 'Q', name, unit, comment, addHeaderKeys))
                return false;

            const size_t size = FITS::SizeFromType(typechar);

            Table::Column col;
            col.name   = name;
            col.type   = typechar;
            col.num    = cnt;
            col.size   = size;
            col.offset = fRealRowWidth;

            fRealRowWidth += size*cnt;

            fRealColumns.emplace_back(col, comp);

            SetStr("ZFORM"+std::to_string((long long int)(fRealColumns.size())), std::to_string((long long int)(cnt))+typechar, "format of "+name+" "+FITS::CommentFromType(typechar));
            SetStr("ZCTYP"+std::to_string((long long int)(fRealColumns.size())), "FACT", "Compression type: FACT");

            return true;
        }

        /// Get and set the actual number of threads for this object
        int32_t GetNumThreads() const { return fNumQueues; }
        bool SetNumThreads(uint32_t num)
        {
            if (tellp()>0)
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("Number of threads cannot be changed in the middle of writing a file");
#else
                gLog << ___err___ << "ERROR - Number of threads cannot be changed in the middle of writing a file" << std::endl;
#endif
                return false;
            }

            //get number of physically available threads
#ifdef HAVE_BOOST_THREAD
            unsigned int num_available_cores = boost::thread::hardware_concurrency();
#else
            unsigned int num_available_cores = std::thread::hardware_concurrency();
            if (num_available_cores == 0)
                num_available_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
            // could not detect number of available cores from system properties...
            if (num_available_cores == 0)
                num_available_cores = 1;

            // leave one core for the main thread and one for the writing
            if (num > num_available_cores)
                num = num_available_cores>2 ? num_available_cores-2 : 1;

            fCompressionQueues.resize(num<1?1:num, Queue<CompressionTarget>(std::bind(&zofits::CompressBuffer, this, std::placeholders::_1), false));
            fNumQueues = num;

            return true;
        }

        uint32_t GetNumTiles() const { return fNumTiles; }
        void SetNumTiles(uint32_t num) { fNumTiles=num; }

protected:

        /// Allocates the required objects.
        void reallocateBuffers()
        {
            size_t total_block_head_size = 0;
            for (auto it=fRealColumns.begin(); it!=fRealColumns.end(); it++)
                total_block_head_size += it->block_head.getSizeOnDisk();

            const size_t chunk_size = fRealRowWidth*fNumRowsPerTile + total_block_head_size + sizeof(FITS::TileHeader) + 8; //+8 for checksuming;
            fMemPool.setChunkSize(chunk_size);
            fSmartBuffer = fMemPool.malloc();
            fRawSumBuffer.resize(fRealRowWidth + 4-fRealRowWidth%4); //for checksuming
        }

        /// Actually does the writing to disk (and checksuming)
        /// @param src the buffer to write
        /// @param sizeToWrite how many bytes should be written
        /// @return the state of the file
        bool writeCompressedDataToDisk(char* src, const uint32_t sizeToWrite)
        {
            char* checkSumPointer = src+4;
            int32_t extraBytes = 0;
            uint32_t sizeToChecksum = sizeToWrite;

            //should we extend the array to the left ?
            if (fCheckOffset != 0)
            {
                sizeToChecksum  += fCheckOffset;
                checkSumPointer -= fCheckOffset;
                memset(checkSumPointer, 0, fCheckOffset);
            }

            //should we extend the array to the right ?
            if (sizeToChecksum%4 != 0)
            {
                extraBytes = 4 - (sizeToChecksum%4);
                memset(checkSumPointer+sizeToChecksum, 0, extraBytes);
                sizeToChecksum += extraBytes;
            }

            //do the checksum
            fDataSum.add(checkSumPointer, sizeToChecksum);

            fCheckOffset = (4 - extraBytes)%4;

            //write data to disk
            write(src+4, sizeToWrite);

            return good();
        }

        /// Compress a given buffer based on the target. This is the method executed by the threads
        /// @param target the struct hosting the parameters of the compression
        /// @return number of bytes of the compressed data, or always 1 when used by the Queues
        bool CompressBuffer(const CompressionTarget& target)
        {
            //Can't get this to work in the thread. Printed the adresses, and they seem to be correct.
            //Really do not understand what's wrong...
            //calibrate data if required
            const uint32_t thisRoundNumRows  = (target.num_rows%fNumRowsPerTile) ? target.num_rows%fNumRowsPerTile : fNumRowsPerTile;
            for (uint32_t i=0;i<thisRoundNumRows;i++)
            {
                char* target_location = target.src.get() + fRealRowWidth*i;
                DrsOffsetCalibrate(target_location);
            }
#ifdef __EXCEPTIONS
            try
            {
#endif
                //transpose the original data
                copyTransposeTile(target.src.get(), target.transposed_src.get(), target.num_rows);

                //compress the buffer
                const uint64_t compressed_size = compressBuffer(target.target.data.get(), target.transposed_src.get(), target.num_rows, target.catalog_entry);

                //post the result to the writing queue
                //get a copy so that it becomes non-const
                fWriteToDiskQueue.emplace(target.target, compressed_size);

#ifdef __EXCEPTIONS
            }
            catch (...)
            {
                fThreadsException = std::current_exception();
                if (fNumQueues == 0)
                    std::rethrow_exception(fThreadsException);
            }
#endif

            return true;
        }

        /// Write one compressed tile to disk. This is the method executed by the writing thread
        /// @param target the struct hosting the write parameters
        bool WriteBufferToDisk(const WriteTarget& target)
        {
            //is this the tile we're supposed to write ?
            if (target.tile_num != (uint32_t)(fLatestWrittenTile+1))
                return false;

            fLatestWrittenTile++;

#ifdef __EXCEPTIONS
            try
            {
#endif
                //could not write the data to disk
                if (!writeCompressedDataToDisk(target.data.get(), target.size))
                    fErrno = errno;
#ifdef __EXCEPTIONS
            }
            catch (...)
            {
                fThreadsException = std::current_exception();
                if (fNumQueues == 0)
                    std::rethrow_exception(fThreadsException);
            }
#endif
            return true;
        }

        /// Compress a given buffer based on its source and destination
        //src cannot be const, as applySMOOTHING is done in place
        /// @param dest the buffer hosting the compressed data
        /// @param src the buffer hosting the transposed data
        /// @param num_rows the number of uncompressed rows in the transposed buffer
        /// @param the number of bytes of the compressed data
        uint64_t compressBuffer(char* dest, char* src, uint32_t num_rows, CatalogRow& catalog_row)
        {
            const uint32_t thisRoundNumRows = (num_rows%fNumRowsPerTile) ? num_rows%fNumRowsPerTile : fNumRowsPerTile;
            uint32_t       offset           = 0;

            //skip the checksum reserved area
            dest += 4;

            //skip the 'TILE' marker and tile size entry
            uint64_t compressedOffset = sizeof(FITS::TileHeader);

            //now compress each column one by one by calling compression on arrays
            for (uint32_t i=0;i<fRealColumns.size();i++)
            {
                catalog_row[i].second = compressedOffset;

                if (fRealColumns[i].col.num == 0)
                    continue;

                FITS::Compression& head = fRealColumns[i].block_head;

                //set the default byte telling if uncompressed the compressed Flag
                const uint64_t previousOffset = compressedOffset;

                //skip header data
                compressedOffset += head.getSizeOnDisk();

                for (uint32_t j=0;j<head.getNumProcs();j++)//sequence.size(); j++)
                {
                    switch (head.getProc(j))
                    {
                    case FITS::kFactRaw:
                        compressedOffset += compressUNCOMPRESSED(dest + compressedOffset, src  + offset, thisRoundNumRows*fRealColumns[i].col.size*fRealColumns[i].col.num);
                        break;

                    case FITS::kFactSmoothing:
                        applySMOOTHING(src + offset, thisRoundNumRows*fRealColumns[i].col.num);
                        break;

                    case FITS::kFactHuffman16:
                        if (head.getOrdering() == FITS::kOrderByCol)
                            compressedOffset += compressHUFFMAN16(dest + compressedOffset, src  + offset, thisRoundNumRows, fRealColumns[i].col.size, fRealColumns[i].col.num);
                        else
                            compressedOffset += compressHUFFMAN16(dest + compressedOffset, src  + offset, fRealColumns[i].col.num, fRealColumns[i].col.size, thisRoundNumRows);
                        break;
                    }
                }

                //check if compressed size is larger than uncompressed
                //if so set flag and redo it uncompressed
                if ((head.getProc(0) != FITS::kFactRaw) && (compressedOffset - previousOffset > fRealColumns[i].col.size*fRealColumns[i].col.num*thisRoundNumRows+head.getSizeOnDisk()))// && two)
                {
                    //de-smooth !
                    if (head.getProc(0) == FITS::kFactSmoothing)
                        UnApplySMOOTHING(src+offset, fRealColumns[i].col.num*thisRoundNumRows);

                    FITS::Compression he;

                    compressedOffset = previousOffset + he.getSizeOnDisk();
                    compressedOffset += compressUNCOMPRESSED(dest + compressedOffset, src + offset, thisRoundNumRows*fRealColumns[i].col.size*fRealColumns[i].col.num);

                    he.SetBlockSize(compressedOffset - previousOffset);
                    he.Memcpy(dest+previousOffset);

                    offset += thisRoundNumRows*fRealColumns[i].col.size*fRealColumns[i].col.num;

                    catalog_row[i].first = compressedOffset - catalog_row[i].second;
                    continue;
                }

                head.SetBlockSize(compressedOffset - previousOffset);
                head.Memcpy(dest + previousOffset);

                offset += thisRoundNumRows*fRealColumns[i].col.size*fRealColumns[i].col.num;
                catalog_row[i].first = compressedOffset - catalog_row[i].second;
            }

            const FITS::TileHeader tile_head(thisRoundNumRows, compressedOffset);
            memcpy(dest, &tile_head, sizeof(FITS::TileHeader));

            return compressedOffset;
        }

        /// Transpose a tile to a new buffer
        /// @param src buffer hosting the regular, row-ordered data
        /// @param dest the target buffer that will receive the transposed data
        void copyTransposeTile(const char* src, char* dest, uint32_t num_rows)
        {
            const uint32_t thisRoundNumRows = (num_rows%fNumRowsPerTile) ? num_rows%fNumRowsPerTile : fNumRowsPerTile;

            //copy the tile and transpose it
            for (uint32_t i=0;i<fRealColumns.size();i++)
            {
                switch (fRealColumns[i].block_head.getOrdering())
                {
                case FITS::kOrderByRow:
                    //regular, "semi-transposed" copy
                    for (uint32_t k=0;k<thisRoundNumRows;k++)
                    {
                        memcpy(dest, src+k*fRealRowWidth+fRealColumns[i].col.offset, fRealColumns[i].col.size*fRealColumns[i].col.num);
                        dest += fRealColumns[i].col.size*fRealColumns[i].col.num;
                    }
                    break;

                case FITS::kOrderByCol:
                    //transposed copy
                    for (uint32_t j=0;j<fRealColumns[i].col.num;j++)
                        for (uint32_t k=0;k<thisRoundNumRows;k++)
                        {
                            memcpy(dest, src+k*fRealRowWidth+fRealColumns[i].col.offset+fRealColumns[i].col.size*j, fRealColumns[i].col.size);
                            dest += fRealColumns[i].col.size;
                        }
                    break;
                };
            }
        }

        /// Specific compression functions
        /// @param dest the target buffer
        /// @param src the source buffer
        /// @param size number of bytes to copy
        /// @return number of bytes written
        uint32_t compressUNCOMPRESSED(char* dest, const char* src, uint32_t size)
        {
            memcpy(dest, src, size);
            return size;
        }

        /// Do huffman encoding
        /// @param dest the buffer that will receive the compressed data
        /// @param src the buffer hosting the transposed data
        /// @param numRows number of rows of data in the transposed buffer
        /// @param sizeOfElems size in bytes of one data elements
        /// @param numRowElems number of elements on each row
        /// @return number of bytes written
        uint32_t compressHUFFMAN16(char* dest, const char* src, uint32_t numRows, uint32_t sizeOfElems, uint32_t numRowElems)
        {
            std::string huffmanOutput;
            uint32_t previousHuffmanSize = 0;

            //if we have less than 2 elems to compress, Huffman encoder does not work (and has no point). Just return larger size than uncompressed to trigger the raw storage.
            if (numRows < 2)
                return numRows*sizeOfElems*numRowElems + 1000;

            if (sizeOfElems < 2 )
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("HUFMANN16 can only encode columns with 16-bit or longer types");
#else
                gLog << ___err___ << "ERROR - HUFMANN16 can only encode columns with 16-bit or longer types" << std::endl;
                return 0;
#endif
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

        /// Applies Thomas' DRS4 smoothing
        /// @param data where to apply it
        /// @param numElems how many elements of type int16_t are stored in the buffer
        /// @return number of bytes modified
        uint32_t applySMOOTHING(char* data, uint32_t numElems)
        {
            int16_t* short_data = reinterpret_cast<int16_t*>(data);
            for (int j=numElems-1;j>1;j--)
                short_data[j] = short_data[j] - (short_data[j-1]+short_data[j-2])/2;

            return numElems*sizeof(int16_t);
        }

        /// Apply the inverse transform of the integer smoothing
        /// @param data where to apply it
        /// @param numElems how many elements of type int16_t are stored in the buffer
        /// @return number of bytes modified
        uint32_t UnApplySMOOTHING(char* data, uint32_t numElems)
        {
            int16_t* short_data = reinterpret_cast<int16_t*>(data);
            for (uint32_t j=2;j<numElems;j++)
                short_data[j] = short_data[j] + (short_data[j-1]+short_data[j-2])/2;

            return numElems*sizeof(uint16_t);
        }



        //thread related stuff
        MemoryManager   fMemPool;           ///< Actual memory manager, providing memory for the compression buffers
        int32_t         fNumQueues;         ///< Current number of threads that will be used by this object
        uint64_t        fMaxUsableMem;      ///< Maximum number of bytes that can be allocated by the memory manager
        int32_t         fLatestWrittenTile; ///< Index of the last tile written to disk (for correct ordering while using several threads)

        std::vector<Queue<CompressionTarget>>     fCompressionQueues;  ///< Processing queues (=threads)
        Queue<WriteTarget, QueueMin<WriteTarget>> fWriteToDiskQueue;   ///< Writing queue (=thread)

        // catalog related stuff
        CatalogType fCatalog;               ///< Catalog for this file
        uint32_t    fCatalogSize;           ///< Actual catalog size (.size() is slow on large lists)
        uint32_t    fNumTiles;              ///< Number of pre-reserved tiles
        uint32_t    fNumRowsPerTile;        ///< Number of rows per tile
        off_t       fCatalogOffset;         ///< Offset of the catalog from the beginning of the file

        // checksum related stuff
        Checksum fCatalogSum;    ///< Checksum of the catalog
        Checksum fRawSum;        ///< Raw sum (specific to FACT)
        int32_t  fCheckOffset;   ///< offset to the data pointer to calculate the checksum

        // data layout related stuff
        /// Regular columns augmented with compression informations

    public: // Public for the root dictionary
        struct CompressedColumn
        {
            CompressedColumn(const Table::Column& c, const FITS::Compression& h) : col(c),
                block_head(h)
            {}
#ifdef __MARS__ // Needed for the compilation ofthe dictionary
            CompressedColumn() { }
#endif
            Table::Column col;             ///< the regular column entry
            FITS::Compression block_head;  ///< the compression data associated with that column
        };

    protected:
        std::vector<CompressedColumn> fRealColumns;     ///< Vector hosting the columns of the file
        uint32_t                      fRealRowWidth;    ///< Width in bytes of one uncompressed row
        std::shared_ptr<char>         fSmartBuffer;     ///< Smart pointer to the buffer where the incoming rows are written
        std::vector<char>             fRawSumBuffer;    ///< buffer used for checksuming the incoming data, before compression

        std::exception_ptr fThreadsException; ///< exception pointer to store exceptions coming from the threads
        int                fErrno;            ///< propagate errno to main thread
};

#endif
