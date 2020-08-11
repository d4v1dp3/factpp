#ifndef FACT_FileEntry
#define FACT_FileEntry

#include <vector>
#include <string>
#include <iomanip>

namespace FileEntry
{
    enum BasicType_t
    {
        kNone = 0,
        kConst,
        kChar,
        kVarchar,
        kBool,
        kInt8,
        kUInt8,
        kFloat,
        kDouble,
        kInt16,
        kUInt16,
        kInt32,
        kUInt32,
        kInt64,
        kUInt64,
        kDecimal,
        kNumeric,
        kTime,
        kDate,
        kDateTime,
    };

    struct Type
    {
        BasicType_t type;
        std::string root;  // root basic type
        char branch;       // root branch
        char fits;         // fits type
        std::string sql;   // sql type
    };

    struct Types : std::vector<Type>
    {
        Types(std::initializer_list<Type> il, const allocator_type& alloc = allocator_type())
               : std::vector<Type>(il, alloc)
        {
        }

        // Note that the result is not unique!
        const std::vector<Type>::const_iterator root(const std::string &str) const
        {
            for (auto it=cbegin(); it!=cend(); it++)
                if (str==it->root)
                    return it;
            return cend();
        }

        // Note that the result is not unique!
        const std::vector<Type>::const_iterator type(const BasicType_t &t) const
        {
            for (auto it=cbegin(); it!=cend(); it++)
                if (t==it->type)
                    return it;
            return cend();
        }

        const std::vector<Type>::const_iterator fits(const char &f) const
        {
            for (auto it=cbegin(); it!=cend(); it++)
                if (f==it->fits)
                    return it;
            return cend();
        }

        const std::vector<Type>::const_iterator sql(const std::string &str) const
        {
            for (auto it=cbegin(); it!=cend(); it++)
                if (str==it->sql)
                    return it;
            return cend();
        }
    };

    static const Types LUT =
    {
        // type      root      branch  fits  sql
        { kChar,     "Char_t",    ' ', 'A', "CHAR"               },
        { kChar,     "Char_t",    ' ', 'a', "CHAR"               },
        { kVarchar,  "Char_t",    ' ', 'A', "VARCHAR"            },
        { kVarchar,  "Char_t",    ' ', 'a', "VARCHAR"            },
        { kBool,     "Bool_t",    'O', 'L', "BOOLEAN"            },
        { kBool,     "Bool_t",    'O', 'l', "BOOLEAN"            },
        { kInt8,     "Char_t",    'B', 'B', "TINYINT"            },
        { kUInt8,    "UInt8_t",   'b', 'b', "TINYINT UNSIGNED"   },
        { kInt16,    "Short_t",   'S', 'I', "SMALLINT"           },
        { kUInt16,   "UShort_t",  's', 'i', "SMALLINT UNSIGNED"  },
        { kInt32,    "Int_t",     'I', 'J', "INT"                },
        { kUInt32,   "UInt_t",    'i', 'j', "INT UNSIGNED"       },
        { kInt32,    "Int_t",     'I', 'J', "MEDIUMINT"          },
        { kUInt32,   "UInt_t",    'i', 'j', "MEDIUMINT UNSIGNED" },
        { kInt64,    "Long64_t",  'L', 'K', "BIGINT"             },
        { kUInt64,   "ULong64_t", 'l', 'k', "BIGINT UNSIGNED"    },
        { kFloat,    "Float_t",   'F', 'E', "FLOAT"              },
        { kFloat,    "Float_t",   'F', 'e', "FLOAT"              },
        { kDouble,   "Double_t",  'D', 'D', "DOUBLE"             },
        { kDouble,   "Double_t",  'D', 'd', "DOUBLE"             },
        { kTime,     "UInt32_t",  'i', 'j', "TIME"               },
        { kDate,     "UInt64_t",  'l', 'k', "DATE"               },
        { kDateTime, "UInt64_t",  'l', 'k', "DATETIME"           },
        { kDecimal,  "Double_t",  'D', 'D', "DECIMAL"            },
        { kDecimal,  "Double_t",  'D', 'd', "DECIMAL"            },
        { kNumeric,  "Double_t",  'D', 'D', "NUMERIC"            },
        { kNumeric,  "Double_t",  'D', 'd', "NUMERIC"            },
    };

    struct Container
    {
        static std::map<void*, size_t> counter;

        std::string branch; // branch name
        std::string column; // column name

        BasicType_t type;

        size_t num;
        void *ptr;

        Container() :  type(kNone), num(0), ptr(0)
        {
        }

        // Root File (do not mix with FitsFile!)
        Container(const std::string &b, const std::string &c,
                  const BasicType_t &t, const size_t &n=1)
            : branch(b), column(c), type(t), num(n), ptr(0)
        {
            switch (t)
            {
            case kInt8:   ptr = new int8_t[n];   break;
            case kUInt8:  ptr = new uint8_t[n];  break;
            case kBool:   ptr = new uint8_t[n];  break;
            case kFloat:  ptr = new float[n];    break;
            case kDecimal:
            case kNumeric:
            case kDouble: ptr = new double[n];   break;
            case kInt16:  ptr = new int16_t[n];  break;
            case kUInt16: ptr = new uint16_t[n]; break;
            case kInt32:  ptr = new int32_t[n];  break;
            case kTime:
            case kUInt32: ptr = new uint32_t[n]; break;
            case kInt64:  ptr = new int64_t[n];  break;
            case kDate:
            case kDateTime:
            case kUInt64: ptr = new uint64_t[n]; break;
            case kChar:
            case kVarchar:
            case kConst:
            case kNone:
                break;
            }
            // Indicate that this was allocated and how often
            counter[ptr]++;
        }

        // This is for rootifysql!
        Container(const std::string &b, const BasicType_t &t)
            : branch(b), type(t), num(1)
        {
            ptr = new double[1];
            // Indicate that this was allocated and how often
            counter[ptr]++;
        }

        Container(const std::string &c, const std::string &value)
            : branch(value), column(c), type(kConst), num(1), ptr(0)
        {
        }

        Container(const Container &c) : branch(c.branch), column(c.column),
            type(c.type), num(c.num), ptr(c.ptr)
        {
            // If this was not allocated in the constructor do not increate the counter
            if (counter[ptr])
                counter[ptr]++;
        }

        // FitsFile (do not mix with RootFile!)
        Container(const std::string &b, const std::string &c,
                  const BasicType_t &t, const size_t &n, void *_ptr)
            : branch(b), column(c), type(t), num(n), ptr(_ptr)
        {
        }

        ~Container()
        {
            // Count how often it gets deleted.
            counter[ptr]--;
            // Now it is time to delete it. Note that pointers which were
            // never allocated (counter==0) now are != 0
            if (counter[ptr]==0)
                ::operator delete[](ptr); // It seems root is deleting it already
        }

        std::string fmt(const size_t &index) const
        {
            std::ostringstream str;

            switch (type)
            {
            case kVarchar: str << std::string(reinterpret_cast<char*>(ptr), num).c_str(); break;
            case kFloat:   str << std::setprecision(8) << reinterpret_cast<float*>(ptr)[index];  break;
            case kDecimal:
            case kNumeric:
            case kDouble:  str << std::setprecision(16) << reinterpret_cast<double*>(ptr)[index]; break;
            case kBool:
            case kInt8:    str << int32_t(reinterpret_cast<int8_t*>(ptr)[index]); break;
            case kUInt8:   str << uint32_t(reinterpret_cast<uint8_t*>(ptr)[index]); break;
            case kInt16:   str << reinterpret_cast<int16_t*>(ptr)[index]; break;
            case kUInt16:  str << reinterpret_cast<uint16_t*>(ptr)[index]; break;
            case kInt32:   str << reinterpret_cast<int32_t*>(ptr)[index]; break;
            case kTime:
            case kUInt32:  str << reinterpret_cast<uint32_t*>(ptr)[index]; break;
            case kInt64:   str << reinterpret_cast<int64_t*>(ptr)[index]; break;
            case kDate:
            case kDateTime:
            case kUInt64:  str << reinterpret_cast<uint64_t*>(ptr)[index]; break;
            case kConst:   str << branch; break;
            case kChar:
            case kNone:
                break;
            }

            //if (str.str()=="nan" || str.str()=="-nan" || str.str()=="inf" || str.str()=="-inf")
            //    return "NULL";

            return str.str();
        }
    };

    std::map<void*, size_t> Container::counter;
}

#endif
