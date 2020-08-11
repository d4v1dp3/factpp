#ifndef FACT_Converter
#define FACT_Converter

#include <math.h>

#include <vector>
#include <iomanip>
#include <sstream>

#include <boost/any.hpp>

#include <stdexcept>

#include <iostream>

class Converter
{
public:
    typedef std::pair<const std::type_info *, int> Type;
    typedef std::pair<int, int>                    Offset;
    typedef std::pair<Type, Offset>                Format;
    typedef std::vector<Format>                    FormatList;

    struct O { };
    struct W { };

    static std::string Clean(std::string s);

private:
    std::ostream &wout;        /// ostream to which output is redirected

    const std::string fFormat; /// Original format string
    const FormatList  fList;   /// Compiled format description

    template <class T>
        T Get(std::stringstream &line) const;

    bool        GetBool(std::stringstream &line) const;
    std::string GetString(std::stringstream &line) const;
    std::string GetStringEol(std::stringstream &line) const;

    template<class T>
        void GetBinImp(std::vector<char> &v, const T &val) const;
    template<class T>
        void GetBinImp(std::vector<boost::any> &v, const T &val) const;

    void GetBinString(std::vector<char> &v, const std::string &val) const;
    void GetBinString(std::vector<boost::any> &v, const std::string &val) const;

    template<class T>
        std::string GetString(const char *&data) const;
    template<char>
        std::string GetString(const char* &ptr) const;

    template<class T>
        static Type GetType();
    template<class T>
        static Type GetVoid();

    template <class T>
        std::vector<T> Get(const std::string &str) const;
    template <class T>
        T Get(const void *d, size_t size) const;



    template<class T>
        void Add(std::string &str, const char* &ptr) const;
    void AddString(std::string &str, const char* &ptr) const;
    template<class T>
        void Add(std::vector<boost::any> &vec, const char* &ptr) const;
    void AddString(std::vector<boost::any> &vec, const char* &ptr) const;


public:
    Converter(std::ostream &out, const std::string &fmt, bool strict=true);
    Converter(const std::string &fmt, bool strict=true);

    /// @returns whether the interpreted format was valid but empty ("")
    bool empty() const { return fList.size()==1 && fList.back().first.second==0; }

    /// @returns whether the compilation was successfull
    bool valid() const { return !fList.empty() && fList.back().first.second==0; }

    /// @returns true if the compilation failed
    bool operator!() const { return !valid(); }

    const FormatList &GetList() const { return fList; }
    size_t GetSize() const { return fList.size()==0 ? 0 : fList.back().second.second; }

    static FormatList Compile(std::ostream &out, const std::string &fmt, bool strict=false);
    static FormatList Compile(const std::string &fmt, bool strict=false);

    std::string             GetString(const void *d, size_t size) const;
    std::vector<char>       GetVector(const void *d, size_t size) const;
    std::vector<boost::any> GetAny(const void *d, size_t size) const;

    std::vector<boost::any> GetAny(const std::string &str) const;
    std::vector<char>       GetVector(const std::string &str) const;

    std::vector<std::string> ToStrings(const void *src/*, size_t size*/) const;
    void ToFits(void* dest, const void* src, size_t size) const;

    std::vector<char> ToFits(const void* src, size_t size) const; 
    std::vector<std::string> GetFitsFormat() const;

    static std::string ToFormat(const std::vector<std::string> &fits);

    template<typename T>
        static std::string GetHex(const void *dat, size_t size, size_t col=0, bool prefix=true)
    {
        if (size%sizeof(T)!=0)
            throw std::runtime_error("GetHex: Total not dividable by typesize.");

        const T *ptr = reinterpret_cast<const T*>(dat);

        std::ostringstream text;
        text << std::hex;

        const size_t w = nearbyint(ceil(log2(size+1)))/4+1;

        for (size_t i=0; i<size/sizeof(T); i++)
        {
            if (prefix && col!=0 && i%col==0)
                text << std::setfill('0') << std::setw(w) << i << "| ";

            text << std::setfill('0') << std::setw(2*sizeof(T));
            text << (unsigned int)ptr[i] << ':';

            if (col!=0 && i%col==col-1)
                text << '\n';

        }

        return text.str();
    }

    template<typename T, typename S>
        static std::string GetHex(const S &s, size_t col=0, bool prefix=true)
    {
        return GetHex<T>(&s, sizeof(S), col, prefix);
    }

    void Print(std::ostream &out) const;
    void Print() const;

    static std::vector<std::string> Regex(const std::string &expr, const std::string &line);
};

#endif

// ***************************************************************************
/** @template GetHex(const void *dat, size_t size, size_t col, bool prefix)

Converts from a binary block into a hex representation.

@param dat
    Pointer to the data block

@param size
    Size of the data block (in bytes)

@param col
    Number of columns before new line (zero <default> to write a
    continous stream

@param prefix
    Boolean which defines whether each line should be prefixed with a counter,
    the default is true. It is ignored if col==0

@tparam T
    type to which the data should be converted. Most usefull types are
    unsigned byte, unsigned short, unsigned int, uint8_t, uint16_t, ...

@returns
    The string

**/
// ***************************************************************************
