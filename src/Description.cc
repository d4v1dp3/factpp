// **************************************************************************
/** @struct Description

@brief A struct which stores a name, a unit and a comment

To have proper descriptions of commands and services in the network
(and later proper informations in the FITS files) this struct provides
a simple storage for a name, a unit and a comment.

Assume you want to write a descriptive string for a command with three arguments.
It could look like this:

   COMMAND=Description|int[addr]:Address range (from - to)|val[byte]:Value to be set

Description is a general description of the command or service itself,
int and val are the names of the arguments (e.g. names of FITS columns),
addr and byte have the meaning of a unit (e.g. unit of FITS column)
and the text after the colon is a description of the arguments
(e.g. comment of a FITS column). The description must not contain a
line-break character \n.

Such a string can then be converted with SplitDescription into a vector
of Description objects, each containing the name, the unit and a
comment indivdually. The first object will contain COMMAND as name and
Description as comment. The unit will remain empty.

You can omit either the name, the unit or the comment or any
combination of them. The descriptions of the individual format strings
are separated by a vertical line. If you want to enclose the name into
[]-braces (e.g. marking an optional argument in a dim command), you
have add empty brackets for the units.

For a suggestion for rules for the names please have a look at:
http://heasarc.gsfc.nasa.gov/docs/heasarc/ofwg/docs/ofwg_recomm/r15.html

For units please see:
http://heasarc.gsfc.nasa.gov/docs/heasarc/ofwg/docs/general/ogip_93_001/ogip_93_001.html

*/
// **************************************************************************
#include "Description.h"

#include <sstream>

#include "tools.h"

using namespace std;
using namespace Tools;

// --------------------------------------------------------------------------
//
//! Construct a Description object
//!
//! @param n
//!     Name of the Description, e.g. "temp"
//!
//! @param c
//!     Descriptive text of the Description, e.g. "Temperature of the moon"
//!
//! @param u
//!     Unit of the Description, e.g. "K"
//
Description::Description(const string &n, const string &c, const string &u)
    : name(Trim(n)), comment(Trim(c)), unit(Trim(u))
{
}

// --------------------------------------------------------------------------
//
//! This function breaks down a descriptive string into its components.
//! For details see class reference.
//!
//! @param buffer
//!     string which should be broekn into pieces
//!
//! @returns
//!     A vector<Description> containing all the descriptions found.
//!     The first entry contains the Description members name and comment,
//!     corresponding to the service or command name the Description
//!     list is for and its corresponding description.
//
vector<Description> Description::SplitDescription(const string &buffer)
{
    const size_t p0 = buffer.find_first_of('=');

    const string svc  = buffer.substr(0, p0);
    const string desc = buffer.substr(p0+1);

    const size_t p = desc.find_first_of('|');

    // Extract a general description
    const string d = Trim(desc.substr(0, p));

    vector<Description> vec;
    vec.emplace_back(svc, d);

    if (p==string::npos)
        return vec;

    string buf;
    stringstream stream(desc.substr(p+1));
    while (getline(stream, buf, '|'))
    {
        if (buf.empty())
            continue;

        const size_t p1 = buf.find_first_of(':');

        const string comment = p1==string::npos ? "" : buf.substr(p1+1);
        if (p1!=string::npos)
            buf.erase(p1);

        const size_t p2 = buf.find_last_of('[');
        const size_t p3 = buf.find_last_of(']');

        const bool hasunit = p2<p3 && p2!=string::npos;

        const string unit = hasunit ? buf.substr(p2+1, p3-p2-1) : "";
        const string name = hasunit ? buf.substr(0, p2) : buf;

        vec.emplace_back(name, comment, unit);
    }

    return vec;
}


// --------------------------------------------------------------------------
//
//! Returns a string with an html formatted text containing the descriptions
//! as returned by SplitDescription
//!
//! @param vec
//!     vector of Description for the individual arguments. First
//!     element is the global description of the command or service.
//!
//! @returns
//!     string with html formatted text
//
string Description::GetHtmlDescription(const vector<Description> &vec)
{
    stringstream str;
    str << "<H3>" << vec[0].name << "</H3>";

    str << "Usage:";
    for (vector<Description>::const_iterator i=vec.begin()+1; i!=vec.end(); i++)
        str << "&nbsp;<font color='maroon'>&lt;" << i->name <<    "&gt;</font>";

    if (vec.size()==1)
        str << " &lt;no arguments&gt;";

    str << "<P>" << vec[0].comment << "<P>";

    str << "<table>";

    for (vector<Description>::const_iterator i=vec.begin()+1; i!=vec.end(); i++)
    {
        str << "<tr>"
            "<td><font color='maroon'>" << i->name <<     "</font>";

        if (i->unit.empty() && !i->comment.empty() && !i->name.empty())
            str << ':';

        str << "</td>";

        if (!i->unit.empty())
            str << "<td><font color='green'>[" << i->unit <<    "]</font>";

        if (!i->unit.empty() && !i->comment.empty())
            str << ':';

        str <<
            "</td>"
            "<td><font color='navy'>"   << i->comment <<  "</font></td>"
            "</tr>";
    }

    str << "</table>";

    return str.str();
}
