#ifndef FACT_Time
#define FACT_Time

#include <boost/date_time/local_time/local_time.hpp>

// **************************************************************************
/** @class _time_format

@brief Helper to manipulate the input and output format of a time in a stream

This class represents a stream manipulator. It is used to change the input
or output format of a Time (or boost::posix_time) object to and from a
stream.

**/
// **************************************************************************
class _time_format
{
    friend std::ostream &operator<<(std::ostream &out, const _time_format &f);
    friend std::istream &operator>>(std::istream &in,  const _time_format &f);
private:
    const char *ptr; /// pointer given to the iostreams

public:
    /// initialize ptr with what should be passed to the iostreams
    _time_format(const char *txt) : ptr(txt) { }
    std::string str() const { return ptr; }
};

class Time : public boost::posix_time::ptime
{
public:
    /// A none-time, this can be used as a simple representation of an invalid time
    static const Time None;

    /// A stream manipulator to set the output/input format
    static const _time_format fmt(const char *txt=0);

    static const _time_format reset;   /// Remove the format description from the stream
    static const _time_format def;     /// set to format to the locale default
    static const _time_format std;     /// set to format to the iso standard
    static const _time_format sql;     /// set to format to the sql format
    static const _time_format ssql;    /// set to format to the sql format (without the fraction of seconds)
    static const _time_format iso;     /// set to format to the extended iso standard
    static const _time_format magic;   /// set to format to the MAGIC report format
    static const _time_format smagic;  /// set to format to the MAGIC report format (without the fraction of seconds)

    /// Enum used in the instantisation of the class to change the inititalisation value
    enum init_t
    {
        none,  ///< Do not initialize the time
        utc,   ///< Initialize with UTC
        local  ///< Initialize with local time
    };

public:
    /// Points to the famous 1/1/1970, the standard offset for unix times
    const static boost::gregorian::date fUnixOffset;

public:
    // Constructors
    Time(enum init_t type=utc);
    Time(const boost::date_time::special_values &val);
    Time(const time_t &tm, const suseconds_t &us);
    Time(const timeval &tm);
    Time(const ptime &pt) : boost::posix_time::ptime(pt) { }
    Time(short year, unsigned char month, unsigned char day,
         unsigned char h=0, unsigned char m=0, unsigned char s=0,
         unsigned int us=0);
    Time(double mjd) { Mjd(mjd); }
    Time(const std::string &str)
    {
        std::stringstream stream;
        stream << str;
        stream >> Time::iso >> *this;
    }

    // Convesion from and to a string
    std::string GetAsStr(const char *fmt="%Y-%m-%d %H:%M:%S") const;
    void SetFromStr(const std::string &str, const char *fmt="%Y-%m-%d %H:%M:%S");

    std::string Iso() const;

    // Conversion to and from MJD
    void Mjd(double mjd);
    double Mjd() const;
    double JD() const { return Mjd()+2400000.5; }

    // Check validity
    bool IsValid() const   { return *this != boost::date_time::not_special; }
    bool operator!() const { return *this == boost::date_time::not_special; }

    // Getter
    unsigned short Y() const  { return date().year(); }
    unsigned short M() const  { return date().month(); }
    unsigned short D() const  { return date().day(); }

    unsigned short h() const  { return time_of_day().hours(); }
    unsigned short m() const  { return time_of_day().minutes(); }
    unsigned short s() const  { return time_of_day().seconds(); }

    unsigned int   ms() const { return time_of_day().total_milliseconds()%1000; }
    unsigned int   us() const { return time_of_day().total_microseconds()%1000000; }

    double SecondsOfDay() const;

    time_t Time_t() const;
    double UnixTime() const;
    double UnixDate() const;
    double RootTime() const;
    uint64_t JavaDate() const { return IsValid() ? uint64_t(UnixTime()*1000) : 0; }

    std::string MinutesTo(const Time & = Time()) const;
    std::string SecondsTo(const Time & = Time()) const;

    Time GetPrevSunRise(double horizon, const std::string &obs="") const;
    Time GetNextSunRise(double horizon, const std::string &obs="") const;

    Time GetPrevSunRise(const std::string &obs="") const;
    Time GetNextSunRise(const std::string &obs="") const;

    Time GetPrevSunSet(double horizon, const std::string &obs="") const;
    Time GetNextSunSet(double horizon, const std::string &obs="") const;

    Time GetPrevSunSet(const std::string &obs="") const;
    Time GetNextSunSet(const std::string &obs="") const;

    uint32_t NightAsInt(const std::string &obs="") const;
};

std::ostream &operator<<(std::ostream &out, const _time_format &f);
std::istream &operator>>(std::istream &in,  const _time_format &f);

#endif
