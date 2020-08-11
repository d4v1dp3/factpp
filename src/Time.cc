// **************************************************************************
/** @class Time

@brief Adds some functionality to boost::posix_time::ptime for our needs

This is basically a wrapper around boost::posix_time::ptime which is made
to adapt the functionality to our needs. Time can store the current
data and time with a precision up to nanoseconds if provided by the
undrlaying system, otherwise microsecond precision is used.

It main purpose is to provide needed constructors and simplyfy the
conversion of dates and times from and to a string/stream.

Note that posix_time (as Posix times have) has a limited range. You cannot
use it for example for very early years of the last century.

@section Examples

 - An example can be found in \ref time.cc

@section References

 - <A HREF="http://www.boost.org/doc/libs/1_45_0/doc/html/date_time.html">BOOST++ date_time (V1.45.0)</A>

**/
// **************************************************************************
#include "Time.h"

using namespace std;
using namespace boost::posix_time;

#ifdef HAVE_NOVA
#include "externals/nova.h"
#endif

const boost::gregorian::date Time::fUnixOffset(1970, 1, 1);

const Time Time::None(Time::none);

// strftime
const _time_format Time::reset  = 0;
const _time_format Time::def    = "%c";
const _time_format Time::std    = "%x %X%F";
const _time_format Time::sql    = "%Y-%m-%d %H:%M:%S.%f";
const _time_format Time::ssql   = "%Y-%m-%d %H:%M:%S";
const _time_format Time::iso    = "%Y-%m-%dT%H:%M:%S%F%q";
const _time_format Time::magic  = "%Y %m %d %H %M %S %f";
const _time_format Time::smagic = "%Y %m %d %H %M %S";

// --------------------------------------------------------------------------
//
//! Construct a Time object with either UTC or local time, or without any
//! particular time.
//!
//! @param typ
//!    enum as defined in Time::init_t
//
Time::Time(enum init_t typ)
{
    switch (typ)
    {
    case utc:
        *this = microsec_clock::universal_time();
        break;
    case local:
        *this = microsec_clock::local_time();
        break;
    case none:
        break;
    }
}


// --------------------------------------------------------------------------
//
//! Construct a Time object with a date_time::special_value, e.g.
//!
//!  - neg_infin
//!  - pos_infin
//!  - not_a_date_time
//!  - max_date_time
//!  - min_date_time
//!
//!
//! @param val
//!    date_time::special_value
//
Time::Time(const boost::date_time::special_values &val) : ptime(val)
{
}

// --------------------------------------------------------------------------
//
//! Construct a Time object from seconds since 1970/1/1 and number of
//! milliseconds, as for example returned by gettimeofday()
//!
//! @param tm
//!    seconds since 1970/1/1
//!
//! @param millisec
//!    number of milliseconds
//
Time::Time(const time_t &tm, const suseconds_t &usec)
: ptime(fUnixOffset, time_duration(0, 0, tm, usec*pow(10, time_duration::num_fractional_digits()-6)))
{
}

// --------------------------------------------------------------------------
//
//! Construct a Time object from a struct timeval.
//!
//! @param tv
//!    struct timeval
//!
Time::Time(const timeval &tv)
: ptime(fUnixOffset, time_duration(0, 0, tv.tv_sec, tv.tv_usec*pow(10, time_duration::num_fractional_digits()-6)))
{
}

// --------------------------------------------------------------------------
//
//! Construct a Time from a date and time.
//!
//! @param year, month, day, hh, mm, ss, microsec
//!    A full date and time down to microsecond precision. From the end
//!    arguments can be omitted.
//!
Time::Time(short year, unsigned char month, unsigned char day,
           unsigned char hh, unsigned char mm, unsigned char ss, unsigned int microsec)
// Last argument is fractional_seconds ( correct with num_fractional_digits() )
: ptime(boost::gregorian::date(year, month, day),
        time_duration(hh, mm, ss, microsec*pow(10, time_duration::num_fractional_digits()-6)))
{
}

// --------------------------------------------------------------------------
//
//! Set the Time object to a given MJD. Note that this involves
//! conversion from double. So converting forth and back many many
//! times might results in drifts.
//!
//! @param mjd
//!    Modified Julian Date
//!
void Time::Mjd(double mjd)
{
    if (mjd > 2400000.5)
        mjd -= 2400000.5;

    // Convert MJD to ticks since offset
    mjd -= 40587;
    mjd *= 24*60*60*time_duration::ticks_per_second();

    *this = ptime(fUnixOffset, time_duration(0, 0, 0, mjd));
}

// --------------------------------------------------------------------------
//
//! @returns the seconds of the day including the fractional seconds.
//!
double Time::SecondsOfDay() const
{
    const time_duration tod = time_of_day();

    const double frac = double(tod.fractional_seconds())/time_duration::ticks_per_second();
    const double sec  = tod.total_seconds()+frac;

    return sec;
}

// --------------------------------------------------------------------------
//
//! Get the current MJD. Note that this involves
//! conversion to double. So converting forth and back many many
//! times might results in drifts.
//!
//! @returns
//!    Modified Julian Date
//!
double Time::Mjd() const
{
    return date().modjulian_day()+SecondsOfDay()/(24*60*60);

    /*
     const time_duration mjd = *this - ptime(fUnixOffset);
     const double sec = mjd.total_seconds()+mjd.fractional_seconds()/1e6;
     return sec/(24*60*60)+40587;
     */
}

// --------------------------------------------------------------------------
//
// @returns seconds since 1970/1/1
//
double Time::UnixTime() const
{
    return (date().modjulian_day()-40587)*24*60*60 + SecondsOfDay();
}

// --------------------------------------------------------------------------
//
// @returns days since 1970/1/1
//
double Time::UnixDate() const
{
    return (date().modjulian_day()-40587) + SecondsOfDay()/(24*60*60);
}

// --------------------------------------------------------------------------
//
// @returns seconds since 1970/1/1
//
time_t Time::Time_t() const
{
    return (date().modjulian_day()-40587)*24*60*60 + time_of_day().total_seconds();
}

// --------------------------------------------------------------------------
//
//! @returns the time in a format needed for root's TAxis
//!
double Time::RootTime() const
{
    return (date().modjulian_day()-49718)*24*60*60 + SecondsOfDay();
}

// --------------------------------------------------------------------------
//
//! Returns a string with the contents of the Time object formated
//! as defined in format.
//!
//! @param format
//!    format description of the string to be returned. For details
//!    see the boost documentation or the man page of strftime
//!
//! @returns
//!    A string with the time formatted as requested. Note some special
//!    strings might be returned in case the time is invalid.
//
string Time::GetAsStr(const char *format) const
{
    stringstream out;
    out << Time::fmt(format) << *this;
    return out.str();
}

// --------------------------------------------------------------------------
//
//! @returns
//!     a human readable string which complies with ISO 8601, in the
//!    "CCYY-MM-DDThh:mm:ss.f"
//
string Time::Iso() const
{
    stringstream out;
    out << Time::iso << *this;
    return out.str();
}

// --------------------------------------------------------------------------
//
//! Sets the time of the Time object to a time corresponding to
//! the one given as argument. It is evaluated according to the given
//! format.
//!
//! @param str
//!    The time as a string which should be converted to the Time object
//!
//! @param format
//!    format description of the string to be returned. For details
//!    see the boost documentation or the man page of strftime
//!
void Time::SetFromStr(const string &str, const char *format)
{
    // FIXME: exception handline
    stringstream stream;
    stream << str;
    stream >> Time::fmt(format) >> *this;
}

string Time::MinutesTo(const Time &time) const
{
    ostringstream str;
    if (time>*this)
        str << time-*this;
    else
        str << *this-time;
    return str.str().substr(0, 5);
}

string Time::SecondsTo(const Time &time) const
{
    ostringstream str;
    if (time>*this)
        str << time-*this;
    else
        str << *this-time;
    return str.str().substr(str.str().substr(0, 3)=="00:" ? 3 : 0, 5);
}

// --------------------------------------------------------------------------
//
//! @returns
//!     The time of the previous sun-rise, relative to given horizon in degree,
//!     for the observatory location given (see nova.h).
//!     If the sun is circumpolar, it simply return the intgeral fraction of
//!     the current MJD.
//!     If libnova was not compiled in, it will return the next noon.
//!
//!  @throws
//!     A runtime_error exception is thrown if the observatory location
//!     is not defined (see nova.h)
//
Time Time::GetPrevSunRise(double horizon, const string &obs) const
{
#ifdef HAVE_NOVA
    const Nova::LnLatPosn posn(obs);
    if (!posn.isValid())
        throw runtime_error("Observatory location '"+obs+"' unknown.");

    ln_rst_time sun_day;
    if (ln_get_solar_rst_horizon(JD()-0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd()));

    if (Time(sun_day.rise)<*this)
        return Time(sun_day.rise);

    if (ln_get_solar_rst_horizon(JD()-1.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd()));

    return Time(sun_day.rise);
#else
    return Time(floor(Mjd()-0.5)+0.5);
#endif
}

// --------------------------------------------------------------------------
//
//! @returns
//!     The time of the next sun-rise, relative to given horizon in degree,
//!     for the observatory location given (see nova.h).
//!     If the sun is circumpolar, it simply return the intgeral fraction of
//!     the current MJD+1.
//!     If libnova was not compiled in, it will return the next noon.
//!
//!  @throws
//!     A runtime_error exception is thrown if the observatory location
//!     is not defined (see nova.h)
//
Time Time::GetNextSunRise(double horizon, const string &obs) const
{
#ifdef HAVE_NOVA
    const Nova::LnLatPosn posn(obs);
    if (!posn.isValid())
        throw runtime_error("Observatory location '"+obs+"' unknown.");

    ln_rst_time sun_day;
    if (ln_get_solar_rst_horizon(JD()-0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd())+1);

    if (Time(sun_day.rise)>=*this)
        return Time(sun_day.rise);

    if (ln_get_solar_rst_horizon(JD()+0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd())+1);

    return Time(sun_day.rise);
#else
    return Time(floor(Mjd()+0.5)+0.5);
#endif
}

// --------------------------------------------------------------------------
//
//! Calls GetPrevSunRise(LN_SOLAR_STANDART_HORIZON)
//
Time Time::GetPrevSunRise(const string &obs) const
{
#ifdef HAVE_NOVA
    return GetPrevSunRise(LN_SOLAR_STANDART_HORIZON, obs);
#else
    return GetPrevSunRise(-0.8333, obs);
#endif
}

// --------------------------------------------------------------------------
//
//! Calls GetNextSunRise(LN_SOLAR_STANDART_HORIZON)
//
Time Time::GetNextSunRise(const string &obs) const
{
#ifdef HAVE_NOVA
    return GetNextSunRise(LN_SOLAR_STANDART_HORIZON, obs);
#else
    return GetNextSunRise(-0.8333, obs);
#endif
}

// --------------------------------------------------------------------------
//
//! @returns
//!     The time of the previous sun-set, relative to given horizon in degree,
//!     for the observatory location given (see nova.h).
//!     If the sun is circumpolar, it simply return the intgeral fraction of
//!     the current MJD.
//!     If libnova was not compiled in, it will return the next noon.
//!
//!  @throws
//!     A runtime_error exception is thrown if the observatory location
//!     is not defined (see nova.h)
//
Time Time::GetPrevSunSet(double horizon, const string &obs) const
{
#ifdef HAVE_NOVA
    const Nova::LnLatPosn posn(obs);
    if (!posn.isValid())
        throw runtime_error("Observatory location '"+obs+"' unknown.");

    ln_rst_time sun_day;
    if (ln_get_solar_rst_horizon(JD()-0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd()));

    if (Time(sun_day.set)<*this)
        return Time(sun_day.set);

    if (ln_get_solar_rst_horizon(JD()-1.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd()));

    return Time(sun_day.set);
#else
    return Time(floor(Mjd()-0.5)+0.5);
#endif
}

// --------------------------------------------------------------------------
//
//! @returns
//!     The time of the next sun-set, relative to given horizon in degree,
//!     for the observatory location given (see nova.h).
//!     If the sun is circumpolar, it simply return the intgeral fraction of
//!     the current MJD+1.
//!     If libnova was not compiled in, it will return the next noon.
//!
//!  @throws
//!     A runtime_error exception is thrown if the observatory location
//!     is not defined (see nova.h)
//
Time Time::GetNextSunSet(double horizon, const string &obs) const
{
#ifdef HAVE_NOVA
    const Nova::LnLatPosn posn(obs);
    if (!posn.isValid())
        throw runtime_error("Observatory location '"+obs+"' unknown.");

    ln_rst_time sun_day;
    if (ln_get_solar_rst_horizon(JD()-0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd())+1);

    if (Time(sun_day.set)>=*this)
        return Time(sun_day.set);

    if (ln_get_solar_rst_horizon(JD()+0.5, const_cast<Nova::LnLatPosn*>(&posn), horizon, &sun_day)==1)
        return Time(floor(Mjd())+1);

    return Time(sun_day.set);
#else
    return Time(floor(Mjd()+0.5)+0.5);
#endif
}

// --------------------------------------------------------------------------
//
//! Calls GetPrevSunSet(LN_SOLAR_STANDART_HORIZON)
//
Time Time::GetPrevSunSet(const string &obs) const
{
#ifdef HAVE_NOVA
    return GetPrevSunSet(LN_SOLAR_STANDART_HORIZON, obs);
#else
    return GetPrevSunSet(-0.8333, obs);
#endif
}

// --------------------------------------------------------------------------
//
//! Calls GetNextSunSet(LN_SOLAR_STANDART_HORIZON)
//
Time Time::GetNextSunSet(const string &obs) const
{
#ifdef HAVE_NOVA
    return GetNextSunSet(LN_SOLAR_STANDART_HORIZON, obs);
#else
    return GetNextSunSet(-0.8333, obs);
#endif
}

// --------------------------------------------------------------------------
//
//! @returns
//!     Returns an int corresponding to the current sun-cycle, that means
//!     the day of the last sun-rise w.r.t. this Time.
//!     YYYYMMDD, e.g. 20111224 for Christmas eve 2011
//!
//! @remark
//!     Before March 30th 2013, 12:00 noon was the reference and the
//!     returned value belonged to the day of sun-set within the
//!     24h period between two noon's.
//
uint32_t Time::NightAsInt(const string &obs) const
{
    const Time tm = GetPrevSunRise(obs);
    return tm.Y()*10000 + tm.M()*100 + tm.D();
}

// --------------------------------------------------------------------------
//
//! A stream manipulator which sets the streams Time output format
//! as defined in the argument.
//!
//! @param format
//!    format description of the manipulator be returned. For details
//!    see the boost documentation or the man page of strftime
//!
//! @returns
//!    a stream manipulator for the given format
//!
const _time_format Time::fmt(const char *format)
{
    return format;
}

// --------------------------------------------------------------------------
//
//! Sets the locale discription of the stream (the way how a time is
//! output) to the format defined by the given manipulator.
//!
//! Example:
//! \code
//!    Time t();
//!    cout << Time::fmt("%Y:%m:%d %H:%M:%S.%f") << t << endl;
//! \endcode
//!
//! @param out
//!    Reference to the stream
//!
//! @param f
//!    Time format described by a manipulator
//!
//! @returns
//!    A reference to the stream
//!
ostream &operator<<(ostream &out, const _time_format &f)
{
    const locale loc(locale::classic(),
                     f.ptr==0 ? 0 : new time_facet(f.ptr));

    out.imbue(loc);

    return out;
}

// --------------------------------------------------------------------------
//
//! Sets the locale discription of the stream (the way how a time is
//! input) to the format defined by the given manipulator.
//!
//! Example:
//! \code
//!    stringstream s;
//!    s << "09.09.1974 21:59";
//!
//!    Time t;
//!    s >> Time::fmt("%d.%m.%Y %H:%M") >> t;
//! \endcode
//!
//! @param in
//!    Reference to the stream
//!
//! @param f
//!    Time format described by a manipulator
//!
//! @returns
//!    A reference to the stream
//!
istream &operator>>(istream &in, const _time_format &f)
{
    const locale loc(locale::classic(),
                     f.ptr==0 ? 0 : new time_input_facet(f.ptr));

    in.imbue(loc);

    return in;
}
