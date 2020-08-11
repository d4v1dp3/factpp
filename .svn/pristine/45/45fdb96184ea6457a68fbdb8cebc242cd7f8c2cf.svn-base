#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>

#include "Configuration.h"

#include "fits.h"
#include "huffman.h"

#include "Time.h"

using namespace std;

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("zfits");
    control.add_options()
        ("in",           var<string>()
#if BOOST_VERSION >= 104200
         ->required()
#endif
         )
        ("out",          var<string>(),             "")
        ("decompress,d", po_switch(),               "")
        ("force,f",      var<string>(),             "Force overwrite of output file")
        ;

    po::positional_options_description p;
    p.add("in",  1); // The 1st positional options
    p.add("out", 2); // The 2nd positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "zfits - A fits compressor\n"
        "\n"
        "\n"
        "Usage: zfits [-d] input.fits[.gz] [output.zf]\n";
    cout << endl;
}


string ReplaceEnd(const string &str, const string &expr, const string &repl)
{
    string out(str);

    const size_t p = out.rfind(expr);
    if (p==out.size()-expr.length())
        out.replace(p, expr.length(), repl);

    return out;
}


string ReplaceExt(const string &name, bool decomp)
{
    if (decomp)
        return ReplaceEnd(name, ".zfits", ".fits");

    string out = ReplaceEnd(name, ".fits",    ".zfits");
    return ReplaceEnd(out, ".fits.gz", ".zfits");
}

struct col_t : fits::Table::Column
{
    string name;
    void *ptr;
};

int Compress(const string &ifile, const string &ofile)
{
    // when to print some info on the screen (every f percent)
    float frac = 0.01;

    // open a fits file
    fits f(ifile);

    // open output file
    ofstream fout(ofile);

    // counters for total size and compressed size
    uint64_t tot = 0;
    uint64_t com = 0;

    // very simple timer
    double sec = 0;

    // Produce a lookup table with all informations about the
    // columns in the same order as they are in the file
    const fits::Table::Columns &cols= f.GetColumns();

    map<size_t, col_t> columns;

    size_t row_tot = 0;
    for (auto it=cols.begin(); it!=cols.end(); it++)
    {
        col_t c;

        c.offset = it->second.offset;
        c.size   = it->second.size;
        c.num    = it->second.num;
        c.name   = it->first;
        c.ptr    = f.SetPtrAddress(it->first);

        columns[c.offset] = c;

        row_tot += c.size*c.num;
    }

    // copy the header from the input to the output file
    // and prefix the output file as a compressed fits file
    string header;
    header.resize(f.tellg());

    f.seekg(0);
    f.read((char*)header.c_str(), header.size());

    char m[2];
    m[0] = static_cast<char>('z'+128);
    m[1] = static_cast<char>('f'+128);

    const size_t hlen = 0;

    size_t hs = header.size();

    fout.write(m, 2);                           // magic number
    fout.write((char*)&hlen, sizeof(size_t));   // length of possible header data (e.g. file version, compression algorithm)
    fout.write((char*)&hs,   sizeof(size_t));   // size of FITS header
    fout.write(header.c_str(), header.size());  // uncompressed FITS header

    tot += header.size();
    com += header.size()+2+2*sizeof(size_t);

    cout << fixed;

    Time start;

    // loop over all rows
    vector<char> cache(row_tot);
    while (f.GetNextRow())
    {
        // pointer to the start of the cache for the data of one row
        char *out = cache.data();

        // mask stroing which column have been compressed and which not
        vector<uint8_t> mask(cols.size()/8 + 1);

        // loop over all columns
        uint32_t icol = 0;
        for (auto it=columns.begin(); it!=columns.end(); it++, icol++)
        {
            // size of cell in bytes
            const size_t len_col = it->second.size * it->second.num;

            // get pointer to data
            int16_t *ptr = (int16_t*)it->second.ptr;

            // If the column is the data, preprocess the data
            /*
            if (it->second.name=="Data")
            {
                int16_t *end = ptr+1440*300-4-(1440*300)%2;
                int16_t *beg = ptr;

                while (end>=beg)
                {
                    const int16_t avg = (end[0] + end[1])/2;
                    end[2] -= avg;
                    end[3] -= avg;
                    end -=2;
                }
            }*/

            // do not try to compress less than 32bytes
            if (len_col>32 && it->second.size==2)
            {
                Time now;

                // perform 16bit hoffman (option for 8bit missing, skip 64bit)
                // (what to do with floats?)
                string buf;
                /*int len =*/ Huffman::Encode(buf, (uint16_t*)ptr, len_col/2);

                sec += Time().UnixTime()-now.UnixTime();

                // check if data was really compressed
                if (buf.size()<len_col)
                {
                    // copy compressed data into output cache
                    memcpy(out, buf.c_str(), buf.size());
                    out += buf.size();

                    // update mask
                    const uint64_t bit = (icol%8);
                    mask[icol/8] |= (1<<bit);

                    continue;
                }
            }

            // just copy the data if it has not been compressed
            memcpy(out, (char*)ptr, len_col);
            out += len_col;
        }

        // calcualte size of output buffer
        const size_t sz = out-cache.data();

        // update counters
        tot += row_tot;
        com += sz + mask.size();

        // write the compression mask and the (partly) copmpressed data stream
        fout.write((char*)mask.data(), mask.size());
        fout.write(cache.data(), sz);

      	 //if (sz2<0 || memcmp(data, dest3.data(), 432000*2)!=0)
         //   cout << "grrrr" << endl;

        const float proc = float(f.GetRow())/f.GetNumRows();
        if (proc>frac)
        {
            const double elep = Time().UnixTime()-start.UnixTime();
            cout << "\r" << setprecision(0) << setw(3) << 100*proc << "% [" << setprecision(1) << setw(5) << 100.*com/tot << "%] cpu:" << sec << "s in:" << tot/1000000/elep << "MB/s" << flush;
            frac += 0.01;
        }
    }

    const double elep = Time().UnixTime()-start.UnixTime();
    cout << setprecision(0) << "\r100% [" << setprecision(1) << setw(5) << 100.*com/tot << "%] cpu:"  << sec << "s in:" << tot/1000000/elep << "MB/s" << endl;

    return 0;
}

template<size_t N>
void revcpy(char *dest, const char *src, int num)
{
    const char *pend = src + num*N;
    for (const char *ptr = src; ptr<pend; ptr+=N, dest+=N)
        reverse_copy(ptr, ptr+N, dest);
}

int Decompress(const string &ifile, const string &ofile)
{
    // open a fits file
    ifstream fin(ifile);

    // open output file
    ofstream fout(ofile);

    // get and check magic number
    unsigned char m[2];
    fin.read((char*)m, 2);
    if (m[0]!='z'+128 || m[1]!='f'+128)
        throw runtime_error("File not a compressed fits file.");

    // get length of additional header information
    size_t hlen = 0;
    fin.read((char*)&hlen, sizeof(size_t));
    if (hlen>0)
        throw runtime_error("Only Version-zero files supported.");

    // get size of FITS header
    size_t hs = 0;
    fin.read((char*)&hs, sizeof(size_t));
    if (!fin)
        throw runtime_error("Could not access header size.");

    // copy the header from the input to the output file
    // and prefix the output file as a compressed fits file
    string header;
    header.resize(hs);

    fin.read((char*)header.c_str(), header.size());
    fout.write((char*)header.c_str(), header.size());
    if (!fin)
        throw runtime_error("Could not read full header");

    string templ("tmpXXXXXX");
    int fd = mkstemp((char*)templ.c_str());
    const ssize_t rc = write(fd, header.c_str(), header.size());
    close(fd);

    if (rc<0)
        throw runtime_error("Could not write to temporary file: "+string(strerror(errno)));

    // open the output file to get the header parsed
    fits info(templ);

    remove(templ.c_str());

    // get the maximum size of one row and a list
    // of all columns ordered by their offset
    size_t row_tot = 0;
    const fits::Table::Columns &cols = info.GetColumns();
    map<size_t, fits::Table::Column> columns;
    for (auto it=cols.begin(); it!=cols.end(); it++)
    {
        columns[it->second.offset] = it->second;
        row_tot += it->second.num*it->second.size;
    }

    // very simple timer
    double sec = 0;
    double frac = 0;

    size_t com = 2+hs+2*sizeof(size_t);
    size_t tot = hs;

    const size_t masklen = cols.size()/8+1;

    // loop over all rows
    vector<char> buf(row_tot+masklen);
    vector<char> swap(row_tot);
    uint32_t offset = 0;

    const uint64_t nrows = info.GetUInt("NAXIS2");

    Time start;

    cout << fixed;
    for (uint32_t irow=0; irow<nrows; irow++)
    {
        fin.read(buf.data()+offset, buf.size()-offset);

        const uint8_t *mask = reinterpret_cast<uint8_t*>(buf.data());
        offset = masklen;

        char *ptr = swap.data();

        uint32_t icol = 0;
        for (auto it=columns.begin(); it!= columns.end(); it++, icol++)
        {
            const size_t &num  = it->second.num;
            const size_t &size = it->second.size;

            if (mask[icol/8]&(1<<(icol%8)))
            {
                Time now;

                vector<uint16_t> out(num*size/2);
                int len = Huffman::Decode((uint8_t*)buf.data()+offset, buf.size()-offset, out);
                if (len<0)
                    throw runtime_error("Decoding failed.");

                sec += Time().UnixTime()-now.UnixTime();

                offset += len;

                revcpy<2>(ptr, (char*)out.data(), num);
            }
            else
            {
                switch (size)
                {
                case 1: memcpy   (ptr, buf.data()+offset, num*size); break;
                case 2: revcpy<2>(ptr, buf.data()+offset, num);      break;
                case 4: revcpy<4>(ptr, buf.data()+offset, num);      break;
                case 8: revcpy<8>(ptr, buf.data()+offset, num);      break;
                }

                offset += num*size;
            }

            ptr += num*size;
        }

        com += offset+masklen;
        tot += row_tot;

        fout.write((char*)swap.data(), swap.size());

        memmove(buf.data(), buf.data()+offset, buf.size()-offset);
        offset = buf.size()-offset;

        if (!fout)
            throw runtime_error("Error writing to output file");

        const float proc = float(irow)/nrows;
        if (proc>frac)
        {
            const double elep = Time().UnixTime()-start.UnixTime();
            cout << "\r" << setprecision(0) << setw(3) << 100*proc << "% [" << setprecision(1) << setw(5) << 100.*com/tot << "%] cpu:" << sec << "s out:" << tot/1000000/elep << "MB/s" << flush;
            frac += 0.01;
        }
    }

    const double elep = Time().UnixTime()-start.UnixTime();
    cout << setprecision(0) << "\r100% [" << setprecision(1) << setw(5) << 100.*com/tot << "%] cpu:" << sec << "s out:" << tot/1000000/elep << "MB/s" << endl;

    return 0;
}

int main(int argc, const char **argv)
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

    const bool decomp = conf.Get<bool>("decompress");

    const string ifile = conf.Get<string>("in");
    const string ofile = conf.Has("out") ? conf.Get<string>("out") : ReplaceExt(ifile, decomp);

    return decomp ? Decompress(ifile, ofile) : Compress(ifile, ofile);

    /*
    // reading and writing files which just contain the binary data
    // For simplicity I assume ROI=300

    ifstream finx( "20130117_082.fits.pu");
    ofstream foutx("20130117_082.fits.puz");

    while (1)
    {
        string str;
        str.resize(432000*2);

        finx.read((char*)str.c_str(), 432000*2);
        if (!finx)
            break;

        // Preprocess the data, e.g. subtract median pixelwise
        for (int i=0; i<1440; i++)
        {
            int16_t *chunk = (int16_t*)str.c_str()+i*300;
            sort(chunk, chunk+300);

            int16_t med = chunk[149];

            for (int j=0; j<300; j++)
                chunk[j] -= med;
        }

        // do huffman encoding on shorts
        string buf;
        int len = huffmans_encode(buf, (uint16_t*)str.c_str(), 432000);

        // if the result is smaller than the original data write
        // the result, otherwise the original data
        if (buf.size()<432000*2)
            foutx.write(buf.c_str(), buf.size());
        else
            foutx.write(str.c_str(), 432000);
    }

    return 0;
    */
}
