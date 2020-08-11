/*
 * corruptFileFixer.cc
 *
 *  Takes a compressed fits file as an input, and copy the "good" part of it to a <filename>_recovered.fits.fz
 *  No tstart or tstop nor checksums are updated. For this the script fixHeaderKeys.sh should be used along with the ftools
 *
 *  How to compile me:
 *
 * setenv PATH /gpfs/fact/swdev/root_v5.32.00/bin:/usr/lib64/qt-3.3/bin:/usr/local/bin:/bin:/usr/bin:/opt/dell/srvadmin/bin
 *
 * setenv LD_LIBRARY_PATH /lib64:/home/isdc/lyard/FACT++/.libs:/gpfs/fact/swdev/root_v5.32.00/lib:/swdev_nfs/FACT++/.libs:
 *
 * g++ -o corruptZFitsFixer --std=c++0x src/corruptFileFixer.cc -I./externals -DHAVE_ZLIB
 *
 *
 *  Created on: Aug 23, 2016
 *      Author: lyard
 */

#include <iostream>
#include <fstream>
#include <list>

#include "factfits.h"

using namespace std;


    template<size_t N>
        void revcpy(char *dest, const char *src, int num)
    {
        const char *pend = src + num*N;
        for (const char *ptr = src; ptr<pend; ptr+=N, dest+=N)
            std::reverse_copy(ptr, ptr+N, dest);
    }

    string format_integer(unsigned long value)
    {
        ostringstream str;
        str << "          ";
        if (value < 10000000000) str << " ";
        if (value <  1000000000) str << " ";
        if (value <   100000000) str << " ";
        if (value <    10000000) str << " ";
        if (value <     1000000) str << " ";
        if (value <      100000) str << " ";
        if (value <       10000) str << " ";
        if (value <        1000) str << " ";
        if (value <         100) str << " ";
        if (value <          10) str << " ";
        str << value << " ";
        return str.str();
    }


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "Only one argument please: the input filename" << endl;
        return -1;
    }

    ifstream input(argv[1], ios::in | ios::binary);

    if (!input)
    {
        cout << "Impossible to open file named \"" << argv[1] << "\"" << endl;
        return -2;
    }

    //First look for the calibration table and make sure that it is complete otherwise there is nothing that we can do
    //read data by 80 chars chunks
    char*        fits_buffer  = new char[81];
    fits_buffer[80] = 0;
    unsigned int n_lines_read = 0;
    unsigned int num_start_tables = 0;
    unsigned int num_end_tables   = 0;
    streampos start_of_table_header = 0;

    //read fits "rows" until we arrive at the end of the data table header, i.e. 3 END and 2 XTENSION
    while (num_start_tables != 2 || num_end_tables != 3)
    {
        input.read(fits_buffer, 80);
        if (input.eof()) break;
        n_lines_read++;
        if (!strcmp(fits_buffer, "XTENSION= 'BINTABLE'           / binary table extension                         "))
        {
            num_start_tables ++;
            if (num_start_tables == 2)
                start_of_table_header = input.tellg() - streampos(80);
        }
        if (!strcmp(fits_buffer, "END                                                                             "))
            num_end_tables++;
        //look for relevant header keywords to be updated and remember their address in the file
        if (num_start_tables == 2)
        {//display the broken table header, for informatio
            //TODO make sure that this is a data run, otherwise constants used below will not work
            cout << fits_buffer << endl;
        }
    }

    if (num_start_tables != 2 || num_end_tables != 3)
    {
        cout << "Could not reach the end of the data table header: nothing could be recovered... sorry." << endl;
        return -2;
    }

    //we are now at the end of the data table header. Progress until the start of the data, i.e. move to 36 - (n_lines_read%36)
    int i_end = (36-(n_lines_read%36));

    for (int i=0;i<i_end; i++)
    {
        input.read(fits_buffer, 80);
        if (input.eof()) break;
        n_lines_read++;
    }

    streampos catalog_beginning     = input.tellg();
    size_t    catalog_reserved_size = 4800000;
    //we passed the header padding: skip the data and reserved catalog space
    input.seekg(catalog_beginning + (streampos)(catalog_reserved_size));
    n_lines_read += 60000;

    //get a list of valid tiles
    list<FITS::TileHeader> good_tiles;
    //remember where the data starts
    streampos data_beginning = input.tellg();
    streampos previous_tile_begin = input.tellg();
    //read the very first tile header
    input.read(fits_buffer, 80);

    if (memcmp(fits_buffer, "TILE", 4))
    {
        cout << "Compressed data does not start by string TILE: nothing could be recovered, sorry." << endl;
        return -4;
    }

    FITS::TileHeader* thead = reinterpret_cast<FITS::TileHeader*>(fits_buffer);
    FITS::TileHeader  previous_tile = *thead;
    //previous_tile has the previous tile header. Move forward, and remember the previous valid tile every time we find a new valid header
    while (!input.eof())
    {
        //skip previous tile
        input.seekg(previous_tile_begin + (streampos)(thead->size));
        streampos this_tile_start = input.tellg();
        input.read(fits_buffer, 80);
        if (input.eof())
            break;
        if (fits_buffer[0] != 'T' || fits_buffer[1] != 'I' || fits_buffer[2] != 'L' || fits_buffer[3] != 'E')
            break;

        //we've found a new valid tile header: remember the previous one
        good_tiles.push_back(previous_tile);
        previous_tile = *thead;
        previous_tile_begin = this_tile_start;
    }

    unsigned int num_tiles_recovered = good_tiles.size();
    //done.
    cout << "We have found " << num_tiles_recovered << " valid tiles. Recovering catalog now " << endl;

    std::vector<std::vector<std::pair<int64_t, int64_t> > > catalog;

    FITS::BlockHeader bhead;

    streamoff offset_in_heap = 0;
    streamoff valid_data = 0;
    unsigned int num_cols    = 9;
    input.close();
    input.open(argv[1], ios::in | ios::binary);
    input.seekg(data_beginning);
    for (unsigned int i=0;i<num_tiles_recovered;i++)
    {
        input.read(fits_buffer, sizeof(FITS::TileHeader));
        if (!input.good())
            break;
        if (memcmp(thead->id, "TILE", 4))
            break;

        catalog.emplace_back();
        offset_in_heap += sizeof(FITS::TileHeader);

        //skip through the columns
        for (unsigned int i=0;i<num_cols;i++)
        {
            input.read((char*)(&bhead), sizeof(FITS::BlockHeader));
            if (!input.good())
            {
                break;
            }
            catalog.back().emplace_back((int64_t)(bhead.size), offset_in_heap);
            offset_in_heap += bhead.size;
            input.seekg(data_beginning + offset_in_heap);
        }

        //at the very last, 0 size time-something column
        catalog.back().emplace_back(0,0);

        if (!input.good())
        {
            catalog.pop_back();
            break;
        }
        valid_data = offset_in_heap;
    }

    if (catalog.size() != num_tiles_recovered)
        cout << "Notice: some apparently OK tiles are in fact corrupted: could only recover " << catalog.size() << " tiles." << endl;

    string recovered_filename(argv[1]);
    recovered_filename = recovered_filename + ".recovered";

    cout << "Catalog recovered. Now writing " << recovered_filename << endl;

    ifstream test_input(recovered_filename.c_str(), ios::in | ios::binary);

    if (test_input)
    {
        cout << "Error: output file already exists. Aborting. " << endl;
        return -10;
    }

    ofstream output(recovered_filename.c_str(), ios::out | ios::binary);
    input.close();
    input.open(argv[1], ios::in | ios::binary);

    cout << "Writing calibration table...";
    cout.flush();
    //first skip the early section of the file: basic fits table, and calibration table
    while (input.tellg() != start_of_table_header)
    {
        input.read(fits_buffer, 80);
        output.write(fits_buffer, 80);
    }

    cout << "done." << endl << "Writing updated table header...";
    cout.flush();

    ostringstream updated_key;
    //calculate updated header keys
    unsigned long theap  = 160*num_tiles_recovered;
    unsigned long pcount = valid_data + catalog_reserved_size - theap;

    while (input.tellg() != catalog_beginning)
    {
        input.read(fits_buffer, 80);
        if (!memcmp(fits_buffer, "NAXIS2", 6))
        {
            updated_key.str("");
            updated_key << "NAXIS2  =" << format_integer(num_tiles_recovered) << "/ number of rows in table                        ";
            output.write(updated_key.str().c_str(), 80);
            continue;
        }
        if (!memcmp(fits_buffer, "PCOUNT", 6))
        {
            updated_key.str("");
            updated_key << "PCOUNT  =" << format_integer(pcount) << "/ size of special data area                      ";
            output.write(updated_key.str().c_str(), 80);
            continue;
        }
        if (!memcmp(fits_buffer, "ZNAXIS2", 7))
        {
            updated_key.str("");
            updated_key << "ZNAXIS2 =" << format_integer(num_tiles_recovered) << "/ Number of uncompressed rows                    ";
            output.write(updated_key.str().c_str(), 80);
            continue;
        }
        if (!memcmp(fits_buffer, "ZHEAPPTR", 8))
        {
            updated_key.str("");
            updated_key << "ZHEAPPTR=" << format_integer(catalog_reserved_size) << "                                                 ";
            output.write(updated_key.str().c_str(), 80);
            continue;
        }
        if (!memcmp(fits_buffer, "THEAP", 5))
        {
            updated_key.str("");
            updated_key << "THEAP   =" << format_integer(theap) << "                                                 ";
            output.write(updated_key.str().c_str(), 80);
            continue;
        }

        output.write(fits_buffer, 80);
    }
cout << "num tiles recovered: " << num_tiles_recovered << endl;
cout << "pcount: " << pcount << endl;
cout << "catalog_reserved_size: " << catalog_reserved_size << endl;
cout << "theap: " << theap << endl;
cout << "offset in heap: " << offset_in_heap << endl;
cout << "valid data:     " << valid_data << endl;
    cout << "done." << endl << "Writing updated catalog...";
    cout.flush();

    //write the catalog itself
    vector<char> swapped_catalog(catalog_reserved_size);
    unsigned int shift = 0;
    for (auto it=catalog.cbegin(); it!=catalog.cend(); it++)
    {
        revcpy<sizeof(uint64_t)>(swapped_catalog.data() + shift, (char*)(it->data()), (num_cols+1)*2);
        shift += 160;
    }

    if (catalog.size() < 30000)
        memset(swapped_catalog.data()+shift, 0, catalog_reserved_size - shift);

    output.write(swapped_catalog.data(), catalog_reserved_size);

    cout << "done." << endl << "Writing recovered data...";
    cout.flush();

    //write the actual data
    input.seekg(input.tellg() + (streampos)(catalog_reserved_size));
cout << "starting writing data at " << output.tellp() << endl;
    unsigned int actual_data_size = pcount + theap - catalog_reserved_size;
    unsigned int i=0;
    for (;i<actual_data_size-80;i+=80)
    {
        input.read(fits_buffer, 80);
        output.write(fits_buffer, 80);
    }

    input.read(fits_buffer, actual_data_size%80);
    output.write(fits_buffer, actual_data_size%80);
cout << "ended writing data at " << output.tellp() << endl;
    cout << "done." << endl << "Writing FITS padding...";
    cout.flush();
cout << "Current extra chars: " << output.tellp()%(80*36) << endl;
cout << "Will write " << 2880 - output.tellp()%(2880) << " filling bytes while at byte " << output.tellp() << endl;
    //eventually write the fits padding
    if (output.tellp()%(80*36) > 0)
    {
        std::vector<char> filler(2880-output.tellp()%(2880), 0);
        output.write(filler.data(), filler.size());
    }
    output.close();
    cout << "All done !." << endl;
    cout << "TSTOP is also most likely invalid, so:" << endl;
    cout << "      - run /swdev_nfs/FACT++/fitsdump <filename> -c UnixTimeUTC --minmax --nozero" << endl;
    cout << "      - update header key using e.g. fv" << endl;
    cout << "Checksums are now invalid: please run fchecksum <filename> update+ datasum+ " << endl;
    return 0;
}



