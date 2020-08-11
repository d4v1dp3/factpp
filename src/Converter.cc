// **************************************************************************
/** @class Converter

@brief A compiler for the DIM data format string

The Converter class interprets arguments in a string accoring to the
given format definition and produces a corresponding memory block from it
which can be attached to an event later.

The format is given according to the Dim format description:

  The format parameter specifies the contents of the structure in the
  form T:N[;T:N]*[;T] where T is the item type: (I)nteger, (C)haracter,
  (L)ong, (S)hort, (F)loat, (D)ouble, X(tra long) and N is the
  number of such items. The type alone at the end means all following items
  are of the same type. Example: "I:3;F:2;C" means 3 Integers, 2 Floats and
  characters until the end. The format parameter is used for
  communicating between different platforms.

Note, that the strange notation T:N[;T:N]*[;T] is meant to be a regular
expression. An Xtra-long is a 'long long'.

Since Dim itself never really interpretes the format string, the programmer
is responsible to make sure that the delivered data and the interpretation
is consistent. Therefore the provided class can be of some help.

For example:

\code
   Converter c(cout, "I:1;F:2;I:2", );
   vector<char> v = c.GetVector("COMMAND 1 2.5 4.2 3 4");
\endcode

would produce a 20 byte data block with the integers 1, the floats
2.5 and 4.2, and the intergers 3 and 4, in this order.

The opposite direction is also possible

\code
   Converter c(cout, "I:1;F:2;I:2");
   cout << c.GetString(pointer, size) << endl;
 \endcode

Other conversion functions also exist.

To check if the compilation of the format string was successfull
the valid() member functio is provided.

The format parameter \b W(ord) is dedicated to this kind of conversion and
not understood by Dim. In addition there are \b O(ptions) which are like
Words but can be omitted. They should only be used at the end of the string.
Both can be encapsulated in quotationmarks '"'. Nested quotationmarks
are not supported. \b B(ool) is also special. It evaluates true/false,
yes/no, on/off, 1/0.

The non-DIM like format options can be switched on and off by using the
strict argument in the constructor. In general DimCommands can use these
options, but DimServices not.

@remark Note that all values are interpreted as signed, except the single
char (e.g. C:5)

*/
// **************************************************************************
#include "Converter.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include <cctype>    // std::tolower
#include <algorithm> // std::transform

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "tools.h"
#include "WindowLog.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! This function is supposed to remove all whitespaces from the format
//! string to allow easier regular expressions later.
//!
//! @param s
//!     string to be cleaned
//!
//! @returns
//!     string cleaned from whitespaces
//
std::string Converter::Clean(std::string s)
{
    while (1)
    {
        const size_t pos = s.find_last_of(' ');
        if (pos==string::npos)
            break;
        s.erase(pos, pos+1);
    }

    return s;
}

// --------------------------------------------------------------------------
//
//! This is just a simplification. For the time being it is used to output
//! the interpreted contents to the logging stream. Its main purpose
//! is to add the contents of val in a binary representation to the
//! vector v
//!
//! @tparam
//!     data type of the variable which should be added
//!
//! @param val
//!     reference to the data
//!
//! @param v
//!     vector<char> to which the binary copy should be added
//!
template <class T>
void Converter::GetBinImp(std::vector<char> &v, const T &val) const
{
    wout << " (" << val << ")";

    v.insert(v.end(),
             reinterpret_cast<const char*>(&val),
             reinterpret_cast<const char*>(&val+1));
}

// --------------------------------------------------------------------------
//
//! This is just a simplification. For the time being it is used to output
//! the interpreted contents to the logging stream. Its main purpose
//! is to add the contents of val as a boost::any object to the
//! vector v
//!
//! @tparam
//!     data type of the variable which should be added
//!
//! @param val
//!     reference to the data
//!
//! @param v
//!     vector<boost::any> to which the value should be added
//!
template <class T>
void Converter::GetBinImp(std::vector<boost::any> &v, const T &val) const
{
    wout << " (" << val << ")";

    v.push_back(val);
}

// --------------------------------------------------------------------------
//
//! This is just a simplification. For the time being it is used to output
//! the interpreted contents to the logging stream. Its main purpose
//! is to add the contents of the provided string at the end of the vector v.
//! vector v
//!
//! @param val
//!     reference to the string
//!
//! @param v
//!     vector<char> to which the value should be added
//!
void Converter::GetBinString(std::vector<char> &v, const string &val) const
{
    wout << " (" << val << ")";

    v.insert(v.end(), val.begin(), val.end()+1);
}

// --------------------------------------------------------------------------
//
//! This is just a simplification. For the time being it is used to output
//! the interpreted contents to the logging stream. Its main purpose
//! is to add the contents of the provided string at the end of the vector v.
//! vector v
//!
//! @param val
//!     reference to the string
//!
//! @param v
//!     vector<boost::any> to which the value should be added
//!
void Converter::GetBinString(std::vector<boost::any> &v, const string &val) const
{
    wout << " (" << val << ")";

    v.push_back(val);
    v.push_back('\n');
}

// --------------------------------------------------------------------------
//
//! Converts from the stringstream into the provided type.
//!
//! @param line
//!     reference to the stringstream from which the data should be
//!     interpreted
//!
//! @tparam
//!     Type of the data to be returned
//!
//! @returns
//!     The interpreted data
//!
template <class T>
T Converter::Get(std::stringstream &line) const
{
    char c;
    line >> c;
    if (!line)
        return T();

    if (c=='0')
    {
        if (line.peek()==-1)
        {
            line.clear(ios::eofbit);
            return 0;
        }

        if (line.peek()=='x')
        {
            line >> c;
            line >> hex;
        }
        else
        {
            line.unget();
            line >> oct;
        }
    }
    else
    {
        line.unget();
        line >> dec;
    }


    T val;
    line >> val;
    return val;
}

// --------------------------------------------------------------------------
//
//! Converts from the stringstream into bool. It allows to use lexical
//! boolean representations like yes/no, on/off, true/false and of
//! course 0/1. If the conversion fails the failbit is set.
//!
//! @param line
//!     reference to the stringstream from which the data should be
//!     interpreted
//!
//! @returns
//!     The boolean. 0 in case of failure
//!
bool Converter::GetBool(std::stringstream &line) const
{
    string buf;
    line >> buf;
    transform(buf.begin(), buf.end(), buf.begin(), ::tolower);

    if (buf=="yes" || buf=="true" || buf=="on" || buf=="1")
        return true;

    if (buf=="no" || buf=="false" || buf=="off" || buf=="0")
        return false;

    line.clear(ios::failbit);

    return false;
}

// --------------------------------------------------------------------------
//
//! Converts from the stringstream into a string. Leading whitespaces are
//! skipped. Everything up to the next whitespace is returned.
//! strings can be encapsulated into escape characters ("). Note, that
//! they cannot be nested.
//!
//! @param line
//!     reference to the stringstream from which the data should be
//!     interpreted
//!
//! @returns
//!     The string
//!
string Converter::GetString(std::stringstream &line) const
{
    while (line.peek()==' ')
        line.get();

    string buf;
    if (line.peek()=='\"')
    {
        line.get();
        getline(line, buf, '\"');
        if (line.peek()==-1)
            line.clear(ios::eofbit);
    }
    else
        line >> buf;

    return buf;
}

// --------------------------------------------------------------------------
//
//! Converts from the stringstream into a string. Leading whitespaces are
//! skipped. Everything until the end-of-line is returned. A trailing
//! \0 is added.
//!
//! @param line
//!     reference to the stringstream from which the data should be
//!     interpreted
//!
//! @returns
//!     The string
//!
string Converter::GetStringEol(stringstream &line) const
{
    line >> noskipws;

    const istream_iterator<char> eol; // end-of-line iterator
    const string text(istream_iterator<char>(line), eol);

    string str = Tools::Trim(text);
    if (str.length()>=2)
    {
        const char b = str[0];
        const char e = str[str.length()-1];

        if ((b=='\"' && e=='\"') || (b=='\'' && e=='\''))
        {
            typedef boost::escaped_list_separator<char> separator;
            const boost::tokenizer<separator> tok(str, separator("\\", " ", "\"'"));

            str = *tok.begin();
        }
    }

    return str + '\0';
}

// --------------------------------------------------------------------------
//
//! Converts from a binary block into a string. The type of the expected
//! value is defined by the template parameter.
//!
//! @param ptr
//!     A refrenece to the pointer of the binary representation to be
//!     interpreted. The pointer is incremented by the sizeof the type.
//!
//! @tparam T
//!     Expected type
//!
//! @returns
//!     The string
//!
template<class T>
string Converter::GetString(const char* &ptr) const
{
    const T &t = *reinterpret_cast<const T*>(ptr);

    ostringstream stream;
    stream << t;
    ptr += sizeof(T);

    return stream.str();
}

template<char>
string Converter::GetString(const char* &ptr) const
{
    ostringstream stream;
    stream << (int64_t)*ptr;
    ptr += 1;

    return stream.str();
}

// --------------------------------------------------------------------------
//
//! Convert the pointer using GetString into a string and add it (prefixed
//! by a whaitespace) to the given string.
//!
//! @param str
//!     Reference to the string to which the ptr should be added
//!
//! @param ptr
//!     Pointer to the binary representation. It will be incremented
//!     according to the sze of the template argument
//!
//! @tparam T
//!     Type as which the binary data should be interpreted
//!
template<class T>
void Converter::Add(string &str, const char* &ptr) const
{
    str += ' ' + GetString<T>(ptr);
}

// --------------------------------------------------------------------------
//
//! Convert the pointer into a boost::any object and add it to the
//! provided vector
//!
//! @param vec
//!     Vector to which the boost::any object should be added
//!
//! @param ptr
//!     Pointer to the binary representation. It will be incremented
//!     according to the size of the template argument
//!
//! @tparam T
//!     Type as which the binary data should be interpreted
//!
template<class T>
void Converter::Add(vector<boost::any> &vec, const char* &ptr) const
{
    vec.push_back(*reinterpret_cast<const T*>(ptr));
    ptr += sizeof(T);
}

// --------------------------------------------------------------------------
//
//! Add the string pointed to by ptr to the given string.
//!
//! @param str
//!     Reference to the string to which the ptr should be added
//!
//! @param ptr
//!     Pointer to the binary representation. It will be incremented
//!     according to the size of the template argument
//!
void Converter::AddString(string &str, const char* &ptr) const
{
    const string txt(ptr);
    str += ' '+txt;
    ptr += txt.length()+1;
}

// --------------------------------------------------------------------------
//
//! Add the string pointed to by ptr as boost::any to the provided vector
//!
//! @param vec
//!     Vector to which the boost::any object should be added
//!
//! @param ptr
//!     Pointer to the binary representation. It will be incremented
//!     according to the size of the template argument
//!
void Converter::AddString(vector<boost::any> &vec, const char* &ptr) const
{
    const string txt(ptr);
    vec.push_back(txt);
    ptr += txt.length()+1;
}

// --------------------------------------------------------------------------
//
//! Compiles the format string into fList. See Compile() for more details.
//!
//! @param out
//!     Output stream to which possible logging is redirected
//!
//! @param fmt
//!     Format to be compiled. For details see class reference
//!
//! @param strict
//!     Setting this to true allows non DIM options, whiel false
//!     will restrict the possible format strings to the ones also
//!     understood by DIM.
//!
Converter::Converter(std::ostream &out, const std::string &fmt, bool strict)
: wout(out), fFormat(Clean(fmt)), fList(Compile(out, fmt, strict))
{
}

// --------------------------------------------------------------------------
//
//! Compiles the format string into fList.
//!
//! Output by default is redirected to cout.
//!
//! @param fmt
//!     Format to be compiled. For details see class reference
//!
//! @param strict
//!     Setting this to true allows non DIM options, whiel false
//!     will restrict the possible format strings to the ones also
//!     understood by DIM.
//!
Converter::Converter(const std::string &fmt, bool strict)
: wout(cout), fFormat(Clean(fmt)), fList(Compile(fmt, strict))
{
}

// --------------------------------------------------------------------------
//
//! Converts the provided format string into a vector.
//!
//! @tparam T
//!     Kind of data to be returned. This can either be boost::any objects
//!     or a bnary data-block (char).
//!
//! @param str
//!     Data to be converted. For details see class reference
//!
//! @returns
//!    A vector of the given template type containing the arguments. In
//!    case of failure an empty vector is returned.
//!
//! @throws
//!    std::runtime_error if the conversion was not successfull
//!
template <class T>
vector<T> Converter::Get(const std::string &str) const
{
    if (!valid())
        throw runtime_error("Compiled format invalid!");

    // If the format is empty we are already done
    if (empty() && str.empty())
    {
        wout << endl;
        return vector<T>();
    }

    int arg = 0;
    stringstream line(str);

    vector<T> data;

    for (Converter::FormatList::const_iterator i=fList.begin(); i<fList.end()-1; i++)
    {
        if (*i->first.first == typeid(string))
        {
            GetBinString(data, GetStringEol(line));
            line.clear(ios::eofbit);
            continue;
        }

        // Get as many items from the input line as requested
        for (int j=0; j<i->second.first; j++)
        {
            switch (i->first.first->name()[0])
            {
            case 'b': GetBinImp(data, GetBool(line)); break;
            case 's': GetBinImp(data, Get<short>    (line)); break;
            case 'i': GetBinImp(data, Get<int>      (line)); break;
            case 'l': GetBinImp(data, Get<long>     (line)); break;
            case 'f': GetBinImp(data, Get<float>    (line)); break;
            case 'd': GetBinImp(data, Get<double>   (line)); break;
            case 'x': GetBinImp(data, Get<long long>(line)); break;
            case 'c':
                {
                    const unsigned short val = Get<unsigned short>(line);
                    if (val>255)
                        line.setstate(ios::failbit);
                    GetBinImp(data, static_cast<unsigned char>(val));
                }
                break;
            case 'N':
                GetBinString(data, GetString(line));
                if (*i->first.first == typeid(O))
                    line.clear(ios::goodbit|(line.rdstate()&ios::eofbit));
                break;
            default:
                // This should never happen!
                throw runtime_error("Format '"+string(i->first.first->name())+" not supported!");
            }

            arg++;
        }

        if (!line)
            break;
    }
    wout << endl;

    // Something wrong with the conversion (e.g. 5.5 for an int)
    if (line.fail() && !line.eof())
    {
        line.clear(); // This is necesasary to get a proper response from tellg()

        ostringstream err;
        err << "Error converting argument at " << arg << " [fmt=" << fFormat << "]!\n";
        err << line.str() << "\n";
        err << setw(int(line.tellg())) << " " << "^\n";
        throw runtime_error(err.str());
    }

    // Not enough arguments, we have not reached the end
    if (line.fail() && line.eof())
    {
        line.clear();

        ostringstream err;
        err << "Not enough arguments [fmt=" << fFormat << "]!\n";
        err << line.str() << "\n";
        err << setw(int(line.tellg())+1) << " " << "^\n";
        throw runtime_error(err.str());
    }

    // Too many arguments, we have not reached the end
    // Unfortunately, this can also mean that there is something
    // wrong with the last argument
    if (line.good() && !line.eof())
    {
        ostringstream err;
        err << "More arguments available than expected [fmt=" << fFormat << "]!\n";
        err << line.str() << "\n";
        err << setw(int(line.tellg())+1) << " " << "^\n";
        throw runtime_error(err.str());
    }

    return data;

}

std::vector<boost::any> Converter::GetAny(const std::string &str) const
{
    return Get<boost::any>(str);
}

std::vector<char> Converter::GetVector(const std::string &str) const
{
    return Get<char>(str);
}

// --------------------------------------------------------------------------
//
//! Converts the provided data block into a vector of boost::any or
//! a string.
//!
//! @tparam T
//!     Kind of data to be returned. This can either be boost::any objects
//!     or a string
//!
//! @returns
//!    A vector of the given template type containing the arguments. In
//!    case of failure an empty vector is returned.
//!
//! @throws
//!    std::runtime_error if the conversion was not successfull
//!
template<class T>
T Converter::Get(const void *dat, size_t size) const
{
    if (!valid())
        throw runtime_error("Compiled format invalid!");

    if (dat==0)
        throw runtime_error("Data pointer == NULL!");

    const char *ptr = reinterpret_cast<const char *>(dat);

    T text;
    for (Converter::FormatList::const_iterator i=fList.begin(); i<fList.end()-1; i++)
    {
        if (ptr-size>dat)
        {
            ostringstream err;
            err << "Format description [fmt=" << fFormat << "|size=" << GetSize() << "] exceeds available data size (" << size << ")";
            throw runtime_error(err.str());
        }

        if (*i->first.first == typeid(string))
        {
            if (size>0)
                AddString(text, ptr);
            if (ptr-size<=dat)
                return text;
            break;
        }

        // Get as many items from the input line as requested
        for (int j=0; j<i->second.first; j++)
        {
            switch (i->first.first->name()[0])
            {
            case 'b': Add<bool>     (text, ptr); break;
            case 'c': Add<char>     (text, ptr); break;
            case 's': Add<short>    (text, ptr); break;
            case 'i': Add<int>      (text, ptr); break;
            case 'l': Add<long>     (text, ptr); break;
            case 'f': Add<float>    (text, ptr); break;
            case 'd': Add<double>   (text, ptr); break;
            case 'x': Add<long long>(text, ptr); break;
            case 'N': AddString(text, ptr);      break;

            case 'v':
                // This should never happen!
                throw runtime_error("Type 'void' not supported!");
            default:
                throw runtime_error("TypeId '"+string(i->first.first->name())+"' not known!");
            }
        }
    }

    if (ptr-size!=dat)
    {
        ostringstream err;
        err << "Data block size (" << size << ") doesn't fit format description [fmt=" << fFormat << "|size=" << GetSize() <<"]";
        throw runtime_error(err.str());
    }

    return text;
}

std::vector<boost::any> Converter::GetAny(const void *dat, size_t size) const
{
    return Get<vector<boost::any>>(dat, size);
}

std::vector<char> Converter::GetVector(const void *dat, size_t size) const
{
    const string ref = GetString(dat, size);

    vector<char> data;
    data.insert(data.begin(), ref.begin()+1, ref.end());
    data.push_back(0);

    return data;
}

string Converter::GetString(const void *dat, size_t size) const
{
    const string s = Get<string>(dat, size);
    return s.empty() ? s : s.substr(1);
}

template<class T>
Converter::Type Converter::GetType()
{
    Type t;
    t.first  = &typeid(T);
    t.second = sizeof(T);
    return t;
}

template<class T>
Converter::Type Converter::GetVoid()
{
    Type t;
    t.first  = &typeid(T);
    t.second = 0;
    return t;
}

// --------------------------------------------------------------------------
//
//! static function to compile a format string.
//!
//! @param out
//!     Output stream to which possible logging is redirected
//!
//! @param fmt
//!     Format to be compiled. For details see class reference
//!
//! @param strict
//!     Setting this to true allows non DIM options, whiel false
//!     will restrict the possible format strings to the ones also
//!     understood by DIM.
//!
Converter::FormatList Converter::Compile(std::ostream &out, const std::string &fmt, bool strict)
{
    ostringstream text;

    // Access both, the data and the format through a stringstream
    stringstream stream(fmt);

    // For better performance we could use sregex
    static const boost::regex expr1("^([CSILFDXBOW])(:([1-9]+[0-9]*))?$");
    static const boost::regex expr2("^([CSILFDX])(:([1-9]+[0-9]*))?$");

    FormatList list;
    Format   format;

    // Tokenize the format
    string buffer;
    while (getline(stream, buffer, ';'))
    {
        boost::smatch what;
        if (!boost::regex_match(buffer, what, strict?expr2:expr1))
        {
            out << kRed << "Wrong format string '" << buffer << "'!" << endl;
            return FormatList();
        }

        const string t = what[1]; // type id
        const string n = what[3]; // counter

        const int cnt = n.empty() ? 0 : stoi(n);

        // if the :N part was not given assume 1
        format.second.first = cnt == 0 ? 1 : cnt;

        /*
        if (strict && t[0]=='C' && cnt>0)
        {
            out << kRed << "Dim doesn't support the format C with N>0!" << endl;
            return FormatList();
        }*/

        // Check if the format is just C (without a number)
        // That would mean that it is a \0 terminated string
        if (t[0]=='C' && cnt==0)
        {
            format.first = GetType<string>();
            list.push_back(format);
            format.second.second = 0; // end position not known
            break;
        }

        // Get as many items from the input line as requested
        switch (t[0])
        {
        case 'B':  format.first = GetType<bool>();      break;
        case 'C':  format.first = GetType<char>();      break;
        case 'S':  format.first = GetType<short>();     break;
        case 'I':  format.first = GetType<int>();       break;
        case 'L':  format.first = GetType<long>();      break;
        case 'F':  format.first = GetType<float>();     break;
        case 'D':  format.first = GetType<double>();    break;
        case 'X':  format.first = GetType<long long>(); break;
        case 'O':  format.first = GetVoid<O>();         break;
        case 'W':  format.first = GetVoid<W>();         break;
        default:
            // This should never happen!
            out << kRed << "Format '" << t[0] << " not known!" << endl;
            return list;
        }

        list.push_back(format);
        format.second.second += format.first.second * format.second.first;
    }

    format.first = GetVoid<void>();
    format.second.first = 0;

    list.push_back(format);

    return list;
}

// --------------------------------------------------------------------------
//
//! Same as Compile(ostream&,string&,bool) but cout is used as the default
//! output stream.
//!
//!
Converter::FormatList Converter::Compile(const std::string &fmt, bool strict)
{
    return Compile(cout, fmt, strict);
}

vector<string> Converter::Regex(const string &expr, const string &line)
{
    const boost::regex reg(expr);

    boost::smatch what;
    if (!boost::regex_match(line, what, reg, boost::match_extra))
        return vector<string>();

    vector<string> ret;
    for (unsigned int i=0; i<what.size(); i++)
        ret.push_back(what[i]);

    return ret;
}

// --------------------------------------------------------------------------
//
//! @param dest
//!    Array to which the destination data is written
//! @param src
//!    Array with the source data according to the format stored in the
//!    Converter
//! @param size
//!    size of the destination data in bytes
//!
void Converter::ToFits(void *dest, const void *src, size_t size) const
{
   // crawl through the src buffer and copy the data appropriately to the
   // destination buffer
   // Assumption: the string is always last. This way we
   // use the provided size to determine the number
   // of character to copy

   char       *charDest = static_cast<char*>(dest);
   const char *charSrc  = static_cast<const char*>(src);

   // We skip the last element 'v'
   for (Converter::FormatList::const_iterator i=fList.begin(); i!=fList.end()-1; i++)
   {
       /*
        // For speed reasons we don't do a check in the loop
       if (charDest-size>dest || charSrc-size>src)
       {
           ostringstream err;
           err << "Format description [fmt=" << fFormat << "] exceeds available data size (" << size << ")";
           throw runtime_error(err.str());
       }
       */

       // Skip strings (must be the last, so we could just skip it)
       const char type = i->first.first->name()[0];
       if (type=='S')
       {
           charSrc += strlen(charSrc)+1;
           continue;
       }

       const int s = i->first.second;      // size of element
       const int n = i->second.first;      // number of elements

       // Check if there are types with unknown sizes
       if (s==0 || n==0)
           throw runtime_error(string("Type '")+type+"' not supported converting to FITS.");

       // Let the compiler do some optimization
       switch (s)
       {
       case 1: memcpy(charDest, charSrc, s*n); charSrc+=s*n; charDest+=s*n; break;
       case 2: for (int j=0; j<n; j++) { reverse_copy(charSrc, charSrc+2, charDest); charSrc+=2; charDest+=2; } break;
       case 4: for (int j=0; j<n; j++) { reverse_copy(charSrc, charSrc+4, charDest); charSrc+=4; charDest+=4; } break;
       case 8: for (int j=0; j<n; j++) { reverse_copy(charSrc, charSrc+8, charDest); charSrc+=8; charDest+=8; } break;
       }
   }

   if (charDest-size!=dest/* || charSrc-size!=src*/)
   {
       ostringstream err;
       err << "ToFits - Data block size (" << size << ") doesn't fit format description [fmt=" << fFormat << "|size=" << GetSize() << "]";
       throw runtime_error(err.str());
   }
}

vector<string> Converter::ToStrings(const void *src/*, size_t size*/) const
{
   const char *charSrc = static_cast<const char*>(src);

   vector<string> rc;

   for (Converter::FormatList::const_iterator i=fList.begin(); i!=fList.end(); i++)
   {
       /*
       if (charSrc-size>src)
       {
           ostringstream err;
           err << "Format description [fmt=" << fFormat << "] exceeds available data size (" << size << ")";
           throw runtime_error(err.str());
       }*/

       const char type = i->first.first->name()[0];
       if (type=='v')
           break;

       if (type=='S' || type=='N')
       {
           const string str(charSrc);
           rc.push_back(str);
           charSrc += str.length()+1;
           continue;
       }

       // string types
       //if (string("bsilfdxc").find_first_of(type)==string::npos)
       //    throw runtime_error(string("Type '")+type+"' not supported converting to FITS.");

       const int s = i->first.second;      // size of element
       const int n = i->second.first;      // number of elements

       charSrc  += s*n;
   }

   return rc;

   /*
   if (charSrc-size!=src)
   {
       ostringstream err;
       err << "Data block size (" << size << ") doesn't fit format description [fmt=" << fFormat << "]";
       throw runtime_error(err.str());
   }*/
}

vector<char> Converter::ToFits(const void *src, size_t size) const
{
    vector<char> dest(size);
    ToFits(dest.data(), src, size);
    return dest;
}

string Converter::ToFormat(const vector<string> &fits)
{
    ostringstream str;
    for (vector<string>::const_iterator it=fits.begin(); it!=fits.end(); it++)
    {
        size_t id=0;
        int n;

        try
        {
            n = stoi(*it, &id);
        }
        catch (exception&)
        {
            n  = 1;
        }

        if (n==0)
            continue;

        switch ((*it)[id])
        {
        case 'A':
        case 'L': 
        case 'B': str << ";C:" << n; break;
        case 'J': str << ";I:" << n; break;
        case 'I': str << ";S:" << n; break;
        case 'K': str << ";X:" << n; break;
        case 'E': str << ";F:" << n; break;
        case 'D': str << ";D:" << n; break;
        default:
            throw runtime_error("ToFormat - id not known.");
        }
    }

    return str.str().substr(1);
}

vector<string> Converter::GetFitsFormat() const
{
    //we've got a nice structure describing the format of this service's messages.
    //Let's create the appropriate FITS columns
    vector<string> vec;
    for (FormatList::const_iterator it=fList.begin(); it!=fList.end(); it++)
    {
         ostringstream dataQualifier;
         dataQualifier << it->second.first;

         switch (it->first.first->name()[0])
         {
         case 'c': dataQualifier << 'B'; break;
         case 's': dataQualifier << 'I'; break;
         case 'i': dataQualifier << 'J'; break;
         case 'l': dataQualifier << 'J'; break;
         case 'f': dataQualifier << 'E'; break;
         case 'd': dataQualifier << 'D'; break;
         case 'x': dataQualifier << 'K'; break;
         case 'v':
         case 'S': //we skip the variable length strings
         case 'N':
             continue;

         default:
             throw runtime_error(string("GetFitsFormat - unknown FITS format [")+it->first.first->name()[0]+"]");
         };

         vec.push_back(dataQualifier.str());
    }

    return vec;
}

void Converter::Print(std::ostream &out) const
{
    for (FormatList::const_iterator i=fList.begin(); i!=fList.end(); i++)
    {
        out << "Type=" << i->first.first->name() << "[" << i->first.second << "]  ";
        out << "N=" << i->second.first << "  ";
        out << "offset=" << i->second.second << endl;
    }
}

void Converter::Print() const
{
    return Print(cout);
}
