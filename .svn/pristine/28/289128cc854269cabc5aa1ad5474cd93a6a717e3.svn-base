// **************************************************************************
/** @file tools.cc

@todo
   - Resolve the dependancies with dim
   - Move code to a more appropriate place
   - put stuff in namespaces
*/
// **************************************************************************
#include "tools.h"

#include <stdarg.h>
#include <math.h>

#include <iomanip>
#include <sstream>
#include <math.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

string Tools::Format(const char *fmt, va_list &ap)
{
    int n=256;

    char *ret=0;
    while (1)
    {
        ret = new char[n+1];

        const int sz = vsnprintf(ret, n, fmt, ap);
        if (sz<=n)
            break;

        n *= 2;
        delete [] ret;
    };

    string str(ret);

    delete [] ret;

    return str;
}

string Tools::Form(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    string str = Format(fmt, ap);

    va_end(ap);

    return str;
}

// --------------------------------------------------------------------------
//
//! This is a static helper to remove leading and trailing whitespaces.
//!
//! @param buf
//!    a pointer to the char array from which the whitespaces should be
//!    removed
//!
//! @returns
//!    a std::string with the whitespaces removed from buf
//
string Tools::Trim(const string &str)
{
    // Trim Both leading and trailing spaces
    const size_t start = str.find_first_not_of(' '); // Find the first character position after excluding leading blank spaces
    const size_t end   = str.find_last_not_of(' ');  // Find the first character position from reverse af

    // if all spaces or empty return an empty string
    if (string::npos==start || string::npos==end)
        return string();

    return str.substr(start, end-start+1);
}

// --------------------------------------------------------------------------
//
//! This is a static helper to remove leading and trailing whitespaces and
//! if available leading and trailing quotes, can be either ' or "
//!
//! @param buf
//!    a pointer to the char array to be trimmed
//!
//! @returns
//!    a std::string with the content trimmed
//
string Tools::TrimQuotes(const string &str)
{
    string rc = Trim(str);
    if (rc.length()<2)
        return rc;

    const char b = rc[0];
    const char e = rc[rc.length()-1];

    if ((b=='\"' && e=='\"') || (b=='\'' && e=='\''))
        return rc.substr(1, rc.length()-2);

    return rc;
}

// --------------------------------------------------------------------------
//
//! This is a static helper to remove leading and trailing whitespaces.
//!
//! Usage example:
//!
//! \code
//!    string str = "     Dies ist ein test fuer einen ganz     langen Satz "
//!        "und ob er korrekt umgebrochen und formatiert wird. Alles "
//!        "nur ein simpler test aber trotzdem ganz wichtig.";
//!
//!    cout << setfill('-') << setw(40) << "+" << endl;
//!    while (1)
//!    {
//!        const string rc = Tools::Wrap(str, 40);
//!        if (rc.empty())
//!            break;
//!        cout << rc << endl;
//!    }
//! \endcode
//!
string Tools::Wrap(string &str, size_t width)
{
    const size_t pos = str.length()<width ? string::npos : str.find_last_of(' ', width);
    if (pos==string::npos)
    {
        const string rc = str;
        str = "";
        return rc;
    }

    const size_t indent = str.find_first_not_of(' ');

    const string rc = str.substr(0, pos);
    const size_t p2 = str.find_first_not_of(' ', pos+1);

    str = str.substr(0, indent) + str.substr(p2==string::npos ? pos+1 : p2);

    return rc;
}

string Tools::Scientific(uint64_t val)
{
    ostringstream rc;
    rc << setprecision(1) << fixed;

    if (val<1000)
    {
        rc << val << " ";
        return rc.str();
    }

    if (val<3000)
    {
        rc << val/1000. << " k";
        return rc.str();
    }

    if (val<1000000)
    {
        rc << val/1000 << " k";
        return rc.str();
    }

    if (val<3000000)
    {
        rc << val/1000000. << " M";
        return rc.str();
    }

    if (val<1000000000)
    {
        rc << val/1000000 << " M";
        return rc.str();
    }

    if (val<3000000000)
    {
        rc << val/1000000000. << " G";
        return rc.str();
    }

    if (val<1000000000000)
    {
        rc << val/1000000000 << " G";
        return rc.str();
    }

    if (val<3000000000000)
    {
        rc << val/1000000000000. << " T";
        return rc.str();
    }

    if (val<1000000000000000)
    {
        rc << val/1000000000000 << " T";
        return rc.str();
    }

    if (val<3000000000000000)
    {
        rc << val/1000000000000000. << " P";
        return rc.str();
    }

    rc << val/1000000000000000. << " P";
    return rc.str();
}

string Tools::Fractional(const double &val)
{
    ostringstream rc;
    rc << setprecision(1) << fixed;

    const auto abs = fabs(val);

    if (abs>1)
    {
        rc << val << " ";
        return rc.str();
    }

    if (abs>1e-3)
    {
        rc << val*1000 << " m";
        return rc.str();
    }

    if (abs>1e-6)
    {
        rc << val*1000000 << " u";
        return rc.str();
    }

    if (abs>1e-9)
    {
        rc << val*1000000000 << " n";
        return rc.str();
    }

    if (abs>1e-12)
    {
        rc << val*1000000000000. << " p";
        return rc.str();
    }

    rc << abs*1000000000000000. << " f";
    return rc.str();
}

// --------------------------------------------------------------------------
//
//! Splits a string into a filename and command line arguments, like:
//!
//!    file.txt arg1=argument1 arg2="argument 2" arg3="argument \"3\""
//!
//! 'file.txt' will be returned on opt, the arguments will be returned in
//! the returned map.
//!
//! If the returned file name is empty, an error has occured:
//!   If the map is also empty the file name was empty, if the map has
//!   one entry then for this entry the equal sign was missing.
//
//! allow==true allows for positional (empty) arguments, like in
//!    file.txt arg1 arg2="argument 2" arg3
//!
map<string,string> Tools::Split(string &opt, bool allow)
{
    using namespace boost;
    typedef escaped_list_separator<char> separator;

    const string data(opt);

    const tokenizer<separator> tok(data, separator("\\", " ", "\"'"));

    auto it=tok.begin();
    if (it==tok.end())
    {
        opt = "";
        return map<string,string>();
    }

    opt = string(*it).find_first_of('=')==string::npos ? *it++ : "";

    map<string,string> rc;

    int cnt=0;

    for (; it!=tok.end(); it++)
    {
        if (it->empty())
            continue;

        const size_t pos = it->find_first_of('=');
        if (pos==string::npos)
        {
            if (allow)
            {
                rc[to_string(cnt++)] = *it;
                continue;
            }

            opt = "";
            rc.clear();
            rc[*it] = "";
            return rc;
        }

        rc[it->substr(0, pos)] = it->substr(pos+1);
    }

    return rc;
}

vector<string> Tools::Split(const string &str, const string &delim)
{
    vector<string> rc;
    boost::split(rc, str, boost::is_any_of(delim));
    return rc;
}

// --------------------------------------------------------------------------
//
//! Returns the string with a comment (introduced by a #) stripped. The
//! comment mark can be escaped by either \# or "#"
//!
string Tools::Uncomment(const string &opt)
{
    using namespace boost;
    typedef escaped_list_separator<char> separator;

    const auto it = tokenizer<separator>(opt, separator("\\", "#", "\"'")).begin();

    const int charPos = it.base() - opt.begin();

    return charPos<1 ? "" : opt.substr(0, opt[charPos-1]=='#' ? charPos-1 : charPos);
}

// --------------------------------------------------------------------------
//
//! Wraps a given text into a vector of strings with not more than
//! max size lines
//!
vector<string> Tools::WordWrap(string text, const uint16_t &max)
{
    if (text.size()<max)
        return { text };

    vector<string> rc;
    while (1)
    {
        // Remove trailing white spaces
        const size_t st = text.find_first_not_of(' ');
        if (st==string::npos)
            break;

        text.erase(0, st);
        if (text.size()<max)
        {
            rc.push_back(text);
            break;
        }

        // Last white space - position to break
        const size_t ws = text.find_last_of(' ', max);
        if (ws==string::npos)
        {
            rc.push_back(text.substr(0, max));
            text.erase(0, max);
            continue;
        }

        // Previous non-whitespace, last position to keep
        const size_t ch = text.find_last_not_of(' ', ws);
        if (ch==string::npos) // found only white spaces
            continue;

        rc.push_back(text.substr(0, ch+1));
        text.erase(0, ws);
    }

    return rc;
}

#if !defined(__clang_major__) && defined(__GNUC__) &&  (__GNUC__ <= 4)

namespace std
{
    string to_string(const size_t &val)
    {
        return to_string((long long unsigned int)val);
    }

    string to_string(const int &val)
    {
        return to_string((long long int)val);
    }

    string to_string(const unsigned int &val)
    {
        return to_string((long long int)val);
    }
}
#endif
