#ifndef MARS_ofits
#define MARS_ofits

#include "FITS.h"
#include "fits.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "n/a"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "n/a"
#endif

#ifndef REVISION
#define REVISION "n/a"
#endif

#ifdef __CINT__
#define off_t size_t
#endif

// Sloppy:  allow / <--- left
//          allow all characters (see specs for what is possible)

// units: m kg s rad sr K A mol cd Hz J W V N Pa C Ohm S F Wb T Hlm lx


class ofits : public std::ostream
{
protected:
    std::filebuf fFilebuf;

public:
    struct Key
    {
        std::string key;
        bool        delim;
        std::string value;
        std::string comment;
        std::string fitsString;

        off_t offset;   // offset in file

        bool changed;   // For closing the file

        Key() : delim(false), offset(0), changed(true) { }
        Key(const std::string &s) : delim(false), fitsString(s), offset(0), changed(true) { }

        std::string Trim(const std::string &str)
        {
            // Trim Both leading and trailing spaces
            const size_t first = str.find_first_not_of(' '); // Find the first character position after excluding leading blank spaces
            const size_t last  = str.find_last_not_of(' ');  // Find the first character position from reverse af

            // if all spaces or empty return an empty string
            if (std::string::npos==first || std::string::npos==last)
                return std::string();

            return str.substr(first, last-first+1);
        }

        bool FormatKey()
        {
            key = Trim(key);
            if (key.empty())
            {
#ifdef __EXCEPTIONS
                throw std::runtime_error("Key name empty.");
#else
                gLog << ___err___ << "ERROR - Key name empty." << std::endl;
                return false;
#endif
            }
            if (key.size()>8)
            {
                std::ostringstream sout;
                sout << "Key '" << key << "' exceeds 8 bytes.";
#ifdef __EXCEPTIONS
                throw std::runtime_error(sout.str());
#else
                gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
                return false;
#endif
            }

            //transform(key.begin(), key.end(), key.begin(), toupper);
#if GCC_VERSION < 40603
            for (std::string::const_iterator c=key.begin(); c<key.end(); c++)
#else
            for (std::string::const_iterator c=key.cbegin(); c<key.cend(); c++)
#endif
                if ((*c<'A' || *c>'Z') && (*c<'0' || *c>'9') && *c!='-' && *c!='_')
                {
                    std::ostringstream sout;
                    sout << "Invalid character '" << *c << "' found in key '" << key << "'";
#ifdef __EXCEPTIONS
                    throw std::runtime_error(sout.str());
#else
                    gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
                    return false;
#endif
                }

            return true;
        }

        bool FormatComment()
        {
            comment = Trim(comment);

#if GCC_VERSION < 40603
            for (std::string::const_iterator c=key.begin(); c<key.end(); c++)
#else
            for (std::string::const_iterator c=key.cbegin(); c<key.cend(); c++)
#endif
                if (*c<32 || *c>126)
                {
                    std::ostringstream sout;
                    sout << "Invalid character '" << *c << "' [" << int(*c) << "] found in comment '" << comment << "'";
#ifdef __EXCEPTIONS
                    throw std::runtime_error(sout.str());
#else
                    gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
                    return false;
#endif
                }

            return true;
        }

        bool check(bool trim=false)
        {
            if (!FormatKey())
                return false;

            if (!FormatComment())
                return false;

            size_t sz = CalcSize();
            if (sz<=80)
                return true;

            if (!trim)
            {
                std::ostringstream sout;
                sout << "Size " << sz << " of entry for key '" << key << "' exceeds 80 characters.";
#ifdef __EXCEPTIONS
                throw std::runtime_error(sout.str());
#else
                gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
#endif
                return false;
            }

            //looks like something went wrong. Maybe entry is too long ?
            //try to remove the comment
            comment = "";

            sz = CalcSize();
            if (sz<=80)
            {
#ifndef __EXCEPTIONS
                std::ostringstream sout;
                sout << "Size " << sz << " of entry for key '" << key << "' exceeds 80 characters... removed comment.";
                gLog << ___warn___ << "WARNING - " << sout.str() << std::endl;
#endif
                return true;
            }

            std::ostringstream sout;
            sout << "Size " << sz << " of entry for key '" << key << "' exceeds 80 characters even without comment.";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        size_t CalcSize() const
        {
            if (!delim)
                return 10+comment.size();

            return 10 + (value.size()<20?20:value.size()) + 3 + comment.size();
        }

        std::string Compile()
        {
            if (!fitsString.empty())
                return fitsString;

            std::ostringstream sout;
            sout << std::left << std::setw(8) << key;

            if (!delim)
            {
                sout << "  " << comment;
                return sout.str();
            }

            sout << "= ";
            sout << (!value.empty() && value[0]=='\''?std::left:std::right);
            sout << std::setw(20) << value << std::left;

            if (!comment.empty())
                sout << " / " << comment;

            return sout.str();
        }

        Checksum checksum;

        void Out(std::ostream &fout)
        {
            if (!changed)
                return;

            std::string str = Compile();
            str.insert(str.end(), 80-str.size(), ' ');

            if (offset==0)
                offset = fout.tellp();

            //cout << "Write[" << offset << "]: " << key << "/" << value << endl;

            fout.seekp(offset);
            fout << str;

            checksum.reset();
            checksum.add(str.c_str(), 80);

            changed = false;
        }
        /*
        void Out(ostream &out)
        {
            std::string str = Compile();

            str.insert(str.end(), 80-str.size(), ' ');

            out << str;
            changed = false;
        }*/
    };

private:
    std::vector<Key> fKeys;

    std::vector<Key>::iterator findkey(const std::string &key)
    {
        for (auto it=fKeys.begin(); it!=fKeys.end(); it++)
            if (key==it->key)
                return it;

        return fKeys.end();
    }

    bool Set(const std::string &key="", bool delim=false, const std::string &value="", const std::string &comment="")
    {
        // If no delimit add the row no matter if it alread exists
        if (delim)
        {
            // if the row already exists: update it
            auto it = findkey(key);
            if (it!=fKeys.end())
            {
                it->value   = value;
                it->changed = true;
                return true;
            }
        }

        if (fTable.num_rows>0)
        {
            std::ostringstream sout;
            sout << "No new header key can be defined, rows were already written to the file... ignoring new key '" << key << "'";
#ifdef __EXCEPTIONS
                throw std::runtime_error(sout.str());
#else
                gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
                return false;
#endif
        }

        Key entry;

        entry.key     = key;
        entry.delim   = delim;
        entry.value   = value;
        entry.comment = comment;

        if (!entry.check(fCommentTrimming))
            return false;

        fKeys.emplace_back(entry);
        return true;
    }

protected:
    struct Table
    {
        off_t offset;

        size_t bytes_per_row;
        size_t num_rows;
        size_t num_cols;

        struct Column
        {
            std::string name;
            size_t offset;
            size_t num;
            size_t size;
            char   type;
        };

        std::vector<Column> cols;

        Table() : offset(0), bytes_per_row(0), num_rows(0), num_cols(0)
        {
        }
    };


    Table fTable;

    std::vector<char> fOutputBuffer;

    std::vector<Table::Column>::const_iterator findcol(const std::string &name)
    {
        for (auto it=fTable.cols.cbegin(); it!=fTable.cols.cend(); it++)
            if (name==it->name)
                return it;

        return fTable.cols.cend();
    }

    Checksum fDataSum;
    Checksum fHeaderSum;

    bool fCommentTrimming;
    bool fManualExtName;

public:
    ofits()
        : std::ostream(), fFilebuf(), fCommentTrimming(false), fManualExtName(false)
    {
        init(&fFilebuf);
    }

    ofits(const char *fname)
        : std::ostream(), fFilebuf(), fCommentTrimming(false), fManualExtName(false)
    {
        init(&fFilebuf);
        open(fname);
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    ofits(const std::string &fname)
        : std::ostream(), fCommentTrimming(false), fManualExtName(false)
    {
        init(&fFilebuf);
        open(fname);
    }
#endif

    virtual ~ofits()
    {
        if (is_open())
            close();
    }
/*
    filebuf *rdbuf() const
    {
        return const_cast<filebuf*>(&fFilebuf);
    }
*/
    bool is_open()
    {
        return fFilebuf.is_open();
    }

    bool is_open() const
    {
        return fFilebuf.is_open();
    }

    virtual void open(const char *filename, bool addEXTNAMEKey=true)
    {
        fDataSum  = 0;
        fHeaderSum = 0;

        fTable = Table();
        fKeys.clear();

        SetStr("XTENSION", "BINTABLE", "binary table extension");
        SetInt("BITPIX",  8, "8-bit bytes");
        SetInt("NAXIS",   2, "2-dimensional binary table");
        SetInt("NAXIS1",  0, "width of table in bytes");
        SetInt("NAXIS2",  0, "number of rows in table");
        SetInt("PCOUNT",  0, "size of special data area");
        SetInt("GCOUNT",  1, "one data group (required keyword)");
        SetInt("TFIELDS", 0, "number of fields in each row");
        if (addEXTNAMEKey)
            SetStr("EXTNAME", "", "name of extension table");
        else
            fManualExtName = true;
        SetStr("CHECKSUM", "0000000000000000", "Checksum for the whole HDU");
        SetStr("DATASUM",  "         0", "Checksum for the data block");

        if (!fFilebuf.open(filename, ios_base::out|ios_base::trunc))
            setstate(ios_base::failbit);
        else
            clear();
    }

    virtual void open(const std::string &filename, bool addEXTNAMEKey=true)
    {
        open(filename.c_str(), addEXTNAMEKey);
    }

    void AllowCommentsTrimming(bool allow)
    {
        fCommentTrimming = allow;
    }

    //Etienne: required to enable 1 to 1 reconstruction of files
    bool SetKeyComment(const std::string& key, const std::string& comment)
    {
        auto it = findkey(key);
        if (it==fKeys.end())
            return false;

        it->comment = comment;
        it->changed = true;
        return true;
    }
        /* tbretz, I removed that, because it does not comply
         with the FITS standard, it omits all checks... it
         neither checks if a row already exists, not
         checks the chcracter set
    bool SetKeyFromFitsString(const std::string& fitsString)
    {
        if (fTable.num_rows>0)
        {
            std::ostringstream sout;
            sout << "No new header key can be defined, rows were already written to the file... ignoring new key '" << fitsString << "'";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        Key entry;
        entry.fitsString = fitsString;
        entry.changed = true;
        fKeys.push_back(entry);
        return true;
}*/


    bool CopyKeys(const fits &fin, bool update=false)
    {
        if (fTable.num_rows>0)
        {
#ifdef __EXCEPTIONS
            throw std::runtime_error("No header keys can be copied, rows were already written to the file... ignoring CopyKeys().");
#else
            gLog << ___err___ << "ERROR - No header key can be copyied, rows were already written to the file... ignoring CopyKeys()." << std::endl;
            return false;
#endif
        }

        const auto &keys = fin.GetKeys();

        // We can assume that the keys are all valid
        for (auto it=keys.cbegin(); it!=keys.cend(); it++)
        {
            const std::string &key = it->first;

            if (FITS::IsReservedKeyWord(key))
                continue;

            const auto &entry = it->second;

            // If no delimit add the row no matter if it alread exists
            if (entry.fitsString[9]=='=')
            {
                // if the row already exists: remove it
                const auto it2 = findkey(key);
                if (it2!=fKeys.end())
                {
                    if (!update)
                        continue;

                    fKeys.erase(it2);
                }
            }

            fKeys.emplace_back(entry.fitsString);
        }

        return true;
    }


    bool SetRaw(const std::string &key, const std::string &val, const std::string &comment)
    {
        return Set(key, true, val, comment);
    }

    bool SetBool(const std::string &key, bool b, const std::string &comment="")
    {
        return Set(key, true, b?"T":"F", comment);
    }

    bool AddEmpty(const std::string &key, const std::string &comment="")
    {
        return Set(key, true, "", comment);
    }

    bool SetStr(const std::string &key, std::string s, const std::string &comment="")
    {
        for (uint i=0; i<s.length(); i++)
            if (s[i]=='\'')
                s.insert(i++, "\'");

        return Set(key, true, "'"+s+"'", comment);
    }

    bool SetInt(const std::string &key, int64_t i, const std::string &comment="")
    {
        std::ostringstream sout;
        sout << i;

        return Set(key, true, sout.str(), comment);
    }

    bool SetFloat(const std::string &key, double f, int p, const std::string &comment="")
    {
        std::ostringstream sout;

        if (p<0)
            sout << std::setprecision(-p) << fixed;
        if (p>0)
            sout << std::setprecision(p);
        if (p==0)
            sout << std::setprecision(f>1e-100 && f<1e100 ? 15 : 14);

        sout << f;

        std::string str = sout.str();

        replace(str.begin(), str.end(), 'e', 'E');

        if (str.find_first_of('E')==std::string::npos && str.find_first_of('.')==std::string::npos)
            str += ".";

        return Set(key, true, str, comment);
    }

    bool SetFloat(const std::string &key, double f, const std::string &comment="")
    {
        return SetFloat(key, f, 0, comment);
    }

    bool SetHex(const std::string &key, uint64_t i, const std::string &comment="")
    {
        std::ostringstream sout;
        sout << std::hex << "0x" << i;
        return SetStr(key, sout.str(), comment);
    }

    bool AddComment(const std::string &comment)
    {
        return Set("COMMENT", false, "", comment);
    }

    bool AddHistory(const std::string &comment)
    {
        return Set("HISTORY", false, "", comment);
    }

    void End()
    {
        Set("END");
        while (fKeys.size()%36!=0)
            fKeys.emplace_back();
    }

    //ETIENNE to be able to restore the file 1 to 1, I must restore the header keys myself
    virtual bool AddColumn(uint32_t cnt, char typechar, const std::string &name, const std::string &unit, const std::string &comment="", bool addHeaderKeys=true)
    {
        if (tellp()<0)
        {
            std::ostringstream sout;
            sout << "File not open... ignoring column '" << name << "'";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        if (tellp()>0)
        {
            std::ostringstream sout;
            sout << "Header already written, no new column can be defined... ignoring column '" << name << "'";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        if (findcol(name)!=fTable.cols.cend())
        {
            std::ostringstream sout;
            sout << "A column with the name '" << name << "' already exists.";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        typechar = toupper(typechar);

        static const std::string allow("LABIJKEDQ");
#if GCC_VERSION < 40603
        if (std::find(allow.begin(), allow.end(), typechar)==allow.end())
#else
        if (std::find(allow.cbegin(), allow.cend(), typechar)==allow.end())
#endif
        {
            std::ostringstream sout;
            sout << "Column type '" << typechar << "' not supported.";
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        std::ostringstream type;
        type << cnt;
        if (typechar=='Q')
            type << "QB";
        else
            type << typechar;

        fTable.num_cols++;

        if (addHeaderKeys)
        {
#if GCC_VERSION < 40603
            const std::string nc = std::to_string((long long int)(fTable.num_cols));
#else
            const std::string nc = std::to_string(fTable.num_cols);
#endif
            SetStr("TFORM"+nc, type.str(), "format of "+name+" "+FITS::CommentFromType(typechar));
            SetStr("TTYPE"+nc, name, comment);
            if (!unit.empty())
                SetStr("TUNIT"+nc, unit, "unit of "+name);
        }

        const size_t size = FITS::SizeFromType(typechar);

        Table::Column col;

        col.name   = name;
        col.type   = typechar;
        col.num    = cnt;
        col.size   = size;
        col.offset = fTable.bytes_per_row;

        fTable.cols.emplace_back(col);

        fTable.bytes_per_row += size*cnt;

        // Align to four bytes
        fOutputBuffer.resize(fTable.bytes_per_row + 4-fTable.bytes_per_row%4);

        return true;
    }

    virtual bool AddColumn(const FITS::Compression&, uint32_t cnt, char typechar, const std::string& name, const std::string& unit,  const std::string& comment="", bool addHeaderKeys=true)
    {
        return AddColumn(cnt, typechar, name, unit, comment, addHeaderKeys);
    }

    bool AddColumnShort(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'I', name, unit, comment); }
    bool AddColumnInt(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'J', name, unit, comment); }
    bool AddColumnLong(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'K', name, unit, comment); }
    bool AddColumnFloat(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'E', name, unit, comment); }
    bool AddColumnDouble(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'D', name, unit, comment); }
    bool AddColumnChar(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'A', name, unit, comment); }
    bool AddColumnByte(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'B', name, unit, comment); }
    bool AddColumnBool(uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(cnt, 'L', name, unit, comment); }

    bool AddColumnShort(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'I', name, unit, comment); }
    bool AddColumnInt(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'J', name, unit, comment); }
    bool AddColumnLong(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'K', name, unit, comment); }
    bool AddColumnFloat(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'E', name, unit, comment); }
    bool AddColumnDouble(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'D', name, unit, comment); }
    bool AddColumnChar(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'A', name, unit, comment); }
    bool AddColumnByte(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'B', name, unit, comment); }
    bool AddColumnBool(const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(1, 'L', name, unit, comment); }

    bool AddColumnShort(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'I', name, unit, comment); }
    bool AddColumnInt(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'J', name, unit, comment); }
    bool AddColumnLong(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'K', name, unit, comment); }
    bool AddColumnFloat(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'E', name, unit, comment); }
    bool AddColumnDouble(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'D', name, unit, comment); }
    bool AddColumnChar(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'A', name, unit, comment); }
    bool AddColumnByte(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'B', name, unit, comment); }
    bool AddColumnBool(const FITS::Compression &comp, uint32_t cnt, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, cnt, 'L', name, unit, comment); }

    bool AddColumnShort(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'I', name, unit, comment); }
    bool AddColumnInt(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'J', name, unit, comment); }
    bool AddColumnLong(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'K', name, unit, comment); }
    bool AddColumnFloat(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'E', name, unit, comment); }
    bool AddColumnDouble(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'D', name, unit, comment); }
    bool AddColumnChar(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'A', name, unit, comment); }
    bool AddColumnByte(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'B', name, unit, comment); }
    bool AddColumnBool(const FITS::Compression &comp, const std::string &name, const std::string &unit="", const std::string &comment="")
    { return AddColumn(comp, 1, 'L', name, unit, comment); }

    /*
    bool AddKey(string key, double d, const std::string &comment)
    {
        ostringstream out;
        out << d;

        std::string s = out.str();

        replace(s.begin(), s.end(), "e", "E");

        return AddKey(key, s, comment);
        }*/


    Checksum WriteHeader(std::ostream &fout)
    {
        Checksum sum;
        uint32_t count=0;
        for (auto it=fKeys.begin(); it!=fKeys.end(); it++)
        {
            it->Out(fout);
            sum += it->checksum;
            count++;
        }
        fout.flush();

        return sum;
    }

    Checksum WriteHeader()
    {
        return WriteHeader(*this);
    }

    void FlushHeader()
    {
        const off_t pos = tellp();
        WriteHeader();
        seekp(pos);
    }

    Checksum WriteFitsHeader()
    {
        ofits h;

        h.SetBool("SIMPLE", true, "file does conform to FITS standard");
        h.SetInt ("BITPIX",    8, "number of bits per data pixel");
        h.SetInt ("NAXIS",     0, "number of data axes");
        h.SetBool("EXTEND", true, "FITS dataset may contain extensions");
        h.SetStr ("CHECKSUM","0000000000000000", "Checksum for the whole HDU");
        h.SetStr ("DATASUM", "         0", "Checksum for the data block");
        h.AddComment("FITS (Flexible Image Transport System) format is defined in 'Astronomy");
        h.AddComment("and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H");
        h.End();

        const Checksum sum = h.WriteHeader(*this);

        h.SetStr("CHECKSUM", sum.str());

        const size_t offset = tellp();
        h.WriteHeader(*this);
        seekp(offset);

        return sum;
    }

    virtual bool WriteDrsOffsetsTable ()
    {
        return true;
    }

    virtual bool WriteCatalog()
    {
        return true;
    }

    virtual bool WriteTableHeader(const char *name="DATA")
    {
        if (tellp()>0)
        {
#ifdef __EXCEPTIONS
            throw std::runtime_error("File not empty anymore.");
#else
            gLog << ___err___ << "ERROR - File not empty anymore." << std::endl;
            return false;
#endif
        }

        fHeaderSum = WriteFitsHeader();

        WriteDrsOffsetsTable();

        if (!fManualExtName)
            SetStr("EXTNAME", name);
        SetInt("NAXIS1",  fTable.bytes_per_row);
        SetInt("TFIELDS", fTable.cols.size());

        End();

        WriteHeader();

        WriteCatalog();

        return good();
    }

    template<size_t N>
        void revcpy(char *dest, const char *src, int num)
    {
        const char *pend = src + num*N;
        for (const char *ptr = src; ptr<pend; ptr+=N, dest+=N)
            std::reverse_copy(ptr, ptr+N, dest);
    }

    virtual uint32_t GetBytesPerRow() const { return fTable.bytes_per_row; }

    virtual bool WriteRow(const void *ptr, size_t cnt, bool byte_swap=true)
    {
        // FIXME: Make sure that header was already written
        //        or write header now!
        if (cnt!=fTable.bytes_per_row)
        {
            std::ostringstream sout;
            sout << "WriteRow - Size " << cnt << " does not match expected size " << fTable.bytes_per_row;
#ifdef __EXCEPTIONS
            throw std::runtime_error(sout.str());
#else
            gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
            return false;
#endif
        }

        // For the checksum we need everything to be correctly aligned
        const uint8_t offset = fTable.offset%4;

        char *buffer = fOutputBuffer.data() + offset;

        auto ib = fOutputBuffer.begin();
        auto ie = fOutputBuffer.rbegin();
        *ib++ = 0;
        *ib++ = 0;
        *ib++ = 0;
        *ib   = 0;

        *ie++ = 0;
        *ie++ = 0;
        *ie++ = 0;
        *ie   = 0;

        if (!byte_swap)
            memcpy(buffer, ptr, cnt);
        else
        {
            for (auto it=fTable.cols.cbegin(); it!=fTable.cols.cend(); it++)
            {
                const char *src  = reinterpret_cast<const char*>(ptr) + it->offset;
                char       *dest = buffer + it->offset;

                // Let the compiler do some optimization by
                // knowing the we only have 1, 2, 4 and 8
                switch (it->size)
                {
                case 1: memcpy   (dest, src, it->num*it->size); break;
                case 2: revcpy<2>(dest, src, it->num);          break;
                case 4: revcpy<4>(dest, src, it->num);          break;
                case 8: revcpy<8>(dest, src, it->num);          break;
                }
            }
        }

        write(buffer, cnt);
        fDataSum.add(fOutputBuffer);

        fTable.num_rows++;
        fTable.offset += cnt;
        return good();
    }

    template<typename N>
    bool WriteRow(const std::vector<N> &vec)
    {
        return WriteRow(vec.data(), vec.size()*sizeof(N));
    }

    // Flushes the number of rows to the header on disk
    virtual void FlushNumRows()
    {
        SetInt("NAXIS2", fTable.num_rows);
        FlushHeader();
    }

    size_t GetNumRows() const { return fTable.num_rows; }

    void AlignTo2880Bytes()
    {
        if (tellp()%(80*36)>0)
        {
            std::vector<char> filler(80*36-tellp()%(80*36));
            write(filler.data(), filler.size());
        }
    }

    Checksum UpdateHeaderChecksum()
    {
        std::ostringstream dataSumStr;
        dataSumStr << fDataSum.val();
        SetStr("DATASUM", dataSumStr.str());

        const Checksum sum = WriteHeader();

        //sum += headersum;

        SetStr("CHECKSUM", (sum+fDataSum).str());

        return WriteHeader();
    }
    virtual bool close()
    {
        if (tellp()<0)
            return false;

        AlignTo2880Bytes();

        // We don't have to jump back to the end of the file
        SetInt("NAXIS2", fTable.num_rows);

        const Checksum chk = UpdateHeaderChecksum();

        if (!fFilebuf.close())
            setstate(ios_base::failbit);

        if ((chk+fDataSum).valid())
            return true;

        std::ostringstream sout;
        sout << "Checksum (" << std::hex << chk.val() << ") invalid.";
#ifdef __EXCEPTIONS
        throw std::runtime_error(sout.str());
#else
        gLog << ___err___ << "ERROR - " << sout.str() << std::endl;
        return false;
#endif
    }

    std::pair<std::string, int> GetChecksumData()
    {
        std::string datasum;
        std::string checksum;
        //cannot directly use the Get methods, because they are only in fits.h
        for (std::vector<Key>::const_iterator it=fKeys.cbegin(); it!= fKeys.cend(); it++)
        {
            if (it->key == "CHECKSUM") checksum = it->value;
            if (it->key == "DATASUM") datasum = it->value;
        }
        if (checksum[0] == '\'')
            checksum = checksum.substr(1,checksum.size()-2);
        if (datasum[0] == '\'')
            datasum = datasum.substr(1, datasum.size()-2);
        return std::make_pair(checksum, atoi(datasum.c_str()));
    }

    void SetDefaultKeys()
    {
        SetStr("TELESCOP", "FACT", "Telescope that acquired this data");
        SetStr("CREATOR", typeid(*this).name(), "Class that wrote this file");
        SetFloat("EXTREL", 1.0, "Release Number");
        SetStr("COMPILED", __DATE__" " __TIME__, "Compile time");
        SetStr("ORIGIN", "FACT", "Institution that wrote the file");
        SetStr("TIMESYS", "UTC", "Time system");
        SetStr("TIMEUNIT", "d", "Time given in days w.r.t. to MJDREF");
        SetInt("MJDREF", 40587, "MJD to UNIX time (seconds since 1970/1/1)");
        SetStr("PACKAGE", PACKAGE_NAME, "Package name");
        SetStr("VERSION", PACKAGE_VERSION, "Package description");
        SetStr("REVISION", REVISION, "SVN revision");

        const time_t t0 = time(NULL);
        const struct tm *tmp1 = gmtime(&t0);

        std::string str(19, '\0');
        if (tmp1 && strftime(const_cast<char*>(str.data()), 20, "%Y-%m-%dT%H:%M:%S", tmp1))
            SetStr("DATE", str, "File creation date");
    }
};

#if 0
#include "fits.h"

int main()
{
    using namespace std;

    ofits h2("delme.fits");

    h2.SetInt("KEY1", 1, "comment 1");
    h2.AddColumnInt(2, "testcol1", "counts", "My comment");
    h2.AddColumnInt("testcol2", "counts", "My comment");
    //h2.AddColumnInt("testcol2", "counts", "My comment");
    h2.SetInt("KEY2", 2, "comment 2");

    /*
     AddFloat("X0",           0.000123456, "number of fields in each row");
     AddFloat("X1",           0, "number of fields in each row");
     AddFloat("X2",      12345, "number of fields in each row");
     AddFloat("X3",      123456.67890, "number of fields in each row");
     AddFloat("X4", 1234567890123456789.12345678901234567890, "number of fields in each row");
     AddFloat("X5", 1234567890.1234567890e20, "number of fields in each row");
     AddFloat("X6", 1234567890.1234567890e-20, "number of fields in each row");
     AddFloat("XB", 1234567890.1234567890e-111, "number of fields in each row");
     AddFloat("X7", 1e-5, "number of fields in each row");
     AddFloat("X8", 1e-6, "number of fields in each row");
     //AddStr("12345678", "123456789012345678", "12345678901234567890123456789012345678901234567");
     */
    // -

    h2.WriteTableHeader("TABLE_NAME");

    for (int i=0; i<10; i++)
    {
        int j[3] = { i+10, i*10, i*100 };
        h2.WriteRow(j, 3*sizeof(i));
    }

    //h2.AddColumnInt("testcol2", "counts", "My comment");
    //h2.SetInt("KEY3", 2, "comment 2");
    h2.SetInt("KEY2", 2, "comment 2xxx");
    h2.SetInt("KEY1", 11);

    h2.close();

    cout << "---" << endl;

    fits f("delme.fits");
    if (!f)
        throw std::runtime_error("xxx");

    cout << "Header is valid: " << f.IsHeaderOk() << endl;

    while (f.GetNextRow());

    cout << "File   is valid: " << f.IsFileOk() << endl;

    cout << "---" << endl;

    return 0;
}
#endif

#endif
