/*
 * factofits.h
 *
 *  Created on: Oct 16, 2013
 *      Author: lyard
 */

#ifndef FACTOFITS_H_
#define FACTOFITS_H_

#define GCC_VERSION (__GNUC__ * 10000  + __GNUC_MINOR__ * 100  + __GNUC_PATCHLEVEL__)

#include "zofits.h"
#include "DrsCalib.h"

class factofits : public zofits
{
    public:

        /// constructors
        factofits(uint32_t numTiles=DefaultMaxNumTiles(), uint32_t rowPerTile=DefaultNumRowsPerTile(),
                  uint32_t maxMem=DefaultMaxMemory())
            : zofits(numTiles, rowPerTile, maxMem)
        {
            fStartCellsOffset = -1;
            fDataOffset       = -1;
        }

        factofits(const char *fname, uint32_t numTiles=DefaultMaxNumTiles(),
                  uint32_t rowPerTile=DefaultNumRowsPerTile(), uint32_t maxMem=DefaultMaxMemory())
            : zofits(fname, numTiles, rowPerTile, maxMem)
        {
            fStartCellsOffset = -1;
            fDataOffset       = -1;
        }

        virtual ~factofits()
        {
        }

        /// whether or not a calibration was given to the file writer
        virtual bool IsOffsetCalibration()
        {
            return (fOffsetCalibration.size() != 0);
        }

        ///assign a given drs offset calibration
        void SetDrsCalibration(const std::vector<float> &calib)
        {
            VerifyCalibrationSize(calib.size());

            if (!IsOffsetCalibration())
                fOffsetCalibration.resize(1440*1024);

            for (uint32_t i=0; i<1440*1024; i++)
                fOffsetCalibration[i] = (int16_t)(calib[i]*4096/2000);
        }

        ///assign a given drs offset calibration
        void SetDrsCalibration(const std::vector<int16_t>& vec)
        {
            VerifyCalibrationSize(vec.size());

            if (!IsOffsetCalibration())
                fOffsetCalibration.resize(1440*1024);

            for (uint32_t i=0; i<1440*1024; i++)
                fOffsetCalibration[i] = vec[i];
        }

        ///assign a given drs offset calibration
        void SetDrsCalibration(const DrsCalibration& drs)
        {
            if (drs.fNumOffset==0)
                return;

            VerifyCalibrationSize(drs.fOffset.size());

            if (!IsOffsetCalibration())
                fOffsetCalibration.resize(1440*1024);

            for (uint32_t i=0; i<1024*1440; i++)
                fOffsetCalibration[i] = drs.fOffset[i]/drs.fNumOffset;
        }

        ///Overload of the super function
        bool WriteTableHeader(const char* name="DATA")
        {
            if (!zofits::WriteTableHeader(name))
                return false;

            if (!IsOffsetCalibration())
                return true;

            //retrieve the column storing the start cell offsets, if required.
            for (auto it=fRealColumns.cbegin(); it!=fRealColumns.cend(); it++)
            {
                if (it->col.name == "StartCellData")
                    fStartCellsOffset = it->col.offset;

                if (it->col.name == "Data")
                {
                    fNumSlices  = it->col.num;
                    fDataOffset = it->col.offset;
                    if (fNumSlices % 1440 != 0)
                    {
#ifdef __EXCEPTIONS
                        throw std::runtime_error("Number of data samples not a multiple of 1440.");
#else
                        gLog << ___warn___ << "WARNING - Number of data samples not a multiple of 1440. Doing it uncalibrated." << std::endl;
#endif
                        fOffsetCalibration.resize(0);
                    }
                    fNumSlices /= 1440;
                }
            }

            if (fStartCellsOffset < 0)
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("FACT Calibration requested, but \"StartCellData\" column not found.");
#else
                gLog << ___warn___ << "WARNING - FACT Calibration requested, but \"StartCellData\" column not found. Doing it uncalibrated." << std::endl;
#endif
                //throw away the calibration data
                fOffsetCalibration.resize(0);
            }

            if (fDataOffset < 0)
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("FACT Calibration requested, but \"Data\" column not found.");
#else
                gLog << ___warn___ << "WARNING - FACT Calibration requested, but \"Data\" column not found. Doing it uncalibrated." << std::endl;
#endif
                    //throw away the calibration data
                fOffsetCalibration.resize(0);
            }

            return true;
        }

        ///Uncompressed version of the DrsCalibration table
 /*       virtual bool WriteDrsOffsetsTable()
        {
            if (!IsOffsetCalibration())
                return false;

            ofits c;
            c.SetStr("XTENSION", "BINTABLE"            , "binary table extension");
            c.SetInt("BITPIX"  , 8                     , "8-bit bytes");
            c.SetInt("NAXIS"   , 2                     , "2-dimensional binary table");
            c.SetInt("NAXIS1"  , 1024*1440*2           , "width of table in bytes");
            c.SetInt("NAXIS2"  , 1                     , "number of rows in table");
            c.SetInt("PCOUNT"  , 0                     , "size of special data area");
            c.SetInt("GCOUNT"  , 1                     , "one data group (required keyword)");
            c.SetInt("TFIELDS" , 1                     , "number of fields in each row");
            c.SetStr("CHECKSUM", "0000000000000000"    , "Checksum for the whole HDU");
            c.SetStr("DATASUM" ,  "         0"         , "Checksum for the data block");
            c.SetStr("EXTNAME" , "ZDrsCellOffsets"     , "name of this binary table extension");
            c.SetStr("TTYPE1"  , "OffsetCalibration"   , "label for field   1");
            c.SetStr("TFORM1"  , "1474560I"            , "data format of field: 2-byte INTEGER");
            c.End();

            vector<char> swappedOffsets;
            swappedOffsets.resize(1024*1440*sizeof(int16_t));
            revcpy<sizeof(int16_t)>(swappedOffsets.data(), (char*)(fOffsetCalibration.data()), 1024*1440);

            Checksum datasum;
            datasum.add(swappedOffsets.data(), sizeof(int16_t)*1024*1440);

            std::ostringstream dataSumStr;
            dataSumStr << datasum.val();
            c.SetStr("DATASUM", dataSumStr.str());

            datasum += c.WriteHeader(*this);

            const off_t here_I_am = tellp();

            c.SetStr("CHECKSUM", datasum.str());
            c.WriteHeader(*this);

            seekp(here_I_am);

            write(swappedOffsets.data(), swappedOffsets.size());

            AlignTo2880Bytes();

            return good();
        }*/

        ///Actually write the drs calibration table
        virtual bool WriteDrsOffsetsTable()
        {
            if (!IsOffsetCalibration())
                return false;

            const uint32_t catalog_size = sizeof(int64_t)*2;

            ofits c;
            c.SetStr("XTENSION", "BINTABLE"            , "binary table extension");
            c.SetInt("BITPIX"  , 8                     , "8-bit bytes");
            c.SetInt("NAXIS"   , 2                     , "2-dimensional binary table");
            c.SetInt("NAXIS1"  , catalog_size           , "width of table in bytes");
            c.SetInt("NAXIS2"  , 1                     , "number of rows in table");
            c.SetInt("PCOUNT"  , 0                     , "size of special data area");
            c.SetInt("GCOUNT"  , 1                     , "one data group (required keyword)");
            c.SetInt("TFIELDS" , 1                     , "number of fields in each row");
            c.SetStr("CHECKSUM", "0000000000000000"    , "Checksum for the whole HDU");
            c.SetStr("DATASUM" ,  "         0"         , "Checksum for the data block");
            c.SetStr("EXTNAME" , "ZDrsCellOffsets"     , "name of this binary table extension");
            c.SetStr("TTYPE1"  , "OffsetCalibration"   , "label for field   1");
            c.SetStr("ZFORM1"  , "1474560I"            , "data format of field: 2-byte INTEGER");
            c.SetStr("TFORM1"  , "1QB"                 , "data format of variable length bytes");
            c.SetStr("ZCTYP1"  , "FACT"                , "Compression type FACT");

            c.SetBool( "ZTABLE",   true,            "Table is compressed");
            c.SetInt(  "ZNAXIS1",  1024*1440*2,    "Width of uncompressed rows");
            c.SetInt(  "ZNAXIS2",  1,               "Number of uncompressed rows");
            c.SetInt(  "ZPCOUNT",  0,               "");
            c.SetInt(  "ZHEAPPTR", catalog_size,               "");
            c.SetInt(  "ZTILELEN", 1, "Number of rows per tile");
            c.SetInt(  "THEAP",    catalog_size,               "");
            c.SetStr(  "RAWSUM",   "         0",    "Checksum of raw little endian data");
            c.SetFloat("ZRATIO",   0,               "Compression ratio");

            c.SetInt(  "ZSHRINK",  1,               "Catalog shrink factor");
            c.End();

            c.WriteHeader(*this);

            const off_t here_I_am = tellp();

            //go after the catalog to compress and write the table data
            seekp(here_I_am + catalog_size);

            //calculate RAWSUM
            Checksum rawsum;
            rawsum.add((char*)(fOffsetCalibration.data()), 1024*1440*sizeof(int16_t));
#if GCC_VERSION < 40603
            c.SetStr("RAWSUM", std::to_string((long long unsigned int)(rawsum.val())));
#else
            c.SetStr("RAWSUM", std::to_string(rawsum.val()));
#endif

            //compress data and calculate final, compressed size
            const uint32_t compressed_header_size = sizeof(FITS::TileHeader) + sizeof(FITS::BlockHeader) + 1*sizeof(uint16_t);
            std::vector<char> compressed_calib(1024*1440*2 + compressed_header_size + 8); //+8 for checksum;
            char* data_start = compressed_calib.data() + compressed_header_size;
            uint32_t compressed_size = compressHUFFMAN16(data_start, (char*)(fOffsetCalibration.data()), 1024*1440, 2, 1);;
            compressed_size += compressed_header_size;

            //Write tile header
            FITS::TileHeader th(0, 0);
            std::vector<uint16_t> seq(1, FITS::kFactHuffman16);
            FITS::Compression bh(seq, FITS::kOrderByRow);
            th.numRows = 1;
            th.size = compressed_size;
            bh.SetBlockSize(compressed_size-sizeof(FITS::TileHeader));
            memcpy(compressed_calib.data(), &(th), sizeof(FITS::TileHeader));
            bh.Memcpy(compressed_calib.data()+sizeof(FITS::TileHeader));

            //calculate resulting compressed datasum
            Checksum datasum;
            memset(compressed_calib.data()+compressed_size, 0, 8-compressed_size%8);
            datasum.add(compressed_calib.data(), compressed_size + 8-compressed_size%8);

            //write the catalog !
            seekp(here_I_am);

            std::vector<uint64_t> catalog(2,0);
            catalog[0] = compressed_size-sizeof(FITS::TileHeader);
            catalog[1] = sizeof(FITS::TileHeader);

            std::vector<char> swappedCatalog(catalog_size);
            revcpy<sizeof(int64_t)>(swappedCatalog.data(), (char*)(catalog.data()), 2);//catalog_size);
            datasum.add(swappedCatalog.data(), catalog_size);

            write(swappedCatalog.data(), catalog_size);

            //update relevant keywords
            c.SetFloat("ZRATIO", (float)(1024*1440*2)/(float)(compressed_size));
            c.SetInt("PCOUNT", compressed_size);// + catalog_size);


//cout << "DEBUG: compressed_size=" << compressed_size << " " << compressed_size%2880 << " " << catalog_size << endl;


#if GCC_VERSION < 40603
            c.SetStr("DATASUM", std::to_string((long long unsigned int)(datasum.val())));
#else
            c.SetStr("DATASUM", std::to_string(datasum.val()));
#endif

            datasum += c.WriteHeader(*this);

            c.SetStr("CHECKSUM", datasum.str());

            c.WriteHeader(*this);

            //write the compressed data
            seekp(here_I_am + catalog_size);
            write(compressed_calib.data(), compressed_size);

            AlignTo2880Bytes();

            return good();
        }

        ///Apply the drs offset calibration (overload of super-method)
        virtual void DrsOffsetCalibrate(char* target_location)
        {
            if (!IsOffsetCalibration())
                return;

            const int16_t* startCell = reinterpret_cast<int16_t*>(target_location + fStartCellsOffset);
            int16_t*       data      = reinterpret_cast<int16_t*>(target_location + fDataOffset);

            for (uint32_t ch=0; ch<1440; ch++)
            {
                if (startCell[ch] < 0)
                {
                    data += fNumSlices;
                    continue;
                }

                const int16_t modStart = startCell[ch]%1024;
                const int16_t *off     = fOffsetCalibration.data() + ch*1024;

                const int16_t* cal        = off+modStart;
                const int16_t* end_stride = data+fNumSlices;

                if (modStart+fNumSlices > 1024)
                {
                    while (cal < off+1024)
                        *data++ -= *cal++;
                    cal = off;
                }

                while (data<end_stride)
                    *data++ -= *cal++;
            }
        }

private:
        /// Checks if the size of the input calibration is ok
        bool VerifyCalibrationSize(uint32_t size)
        {
            if (size == 1440*1024)
                return true;

            std::ostringstream str;
            str << "Cannot load calibration with anything else than 1440 pixels and 1024 samples per pixel. Got a total size of " << size;
#ifdef __EXCEPTIONS
            throw std::runtime_error(str.str());
#else
            gLog << ___err___ << "ERROR - " << str.str() << std::endl;
            return false;
#endif
         }

        //Offsets calibration stuff.
        std::vector<int16_t> fOffsetCalibration;  ///< The calibration itself
        int32_t         fStartCellsOffset;   ///< Offset in bytes for the startcell data
        int32_t         fDataOffset;         ///< Offset in bytes for the data
        int32_t         fNumSlices;          ///< Number of samples per pixel per event

}; //class factofits

#endif /* FACTOFITS_H_ */
