#include "Time.h"

#include <iostream>

using namespace std;
using namespace boost::posix_time;

int main(int, char **)
{
    // Print the local time in the default representation of cout
    cout << endl;
    cout << "Local Time:   " << Time(Time::local) << endl;

    // Print UTC in several different representations
    Time utc; 
    cout << "Universal CT: " << utc << endl;
    cout << "User defined: " << Time::fmt("%Y=%m=%d %H=%M=%S.%f") << utc << endl;
    cout << "SQL-format:   " << Time::sql << utc << endl;
    cout << "ISO-format:   " << Time::iso << utc << endl;
    cout << "Default:      " << Time::reset << utc << endl;
    cout << endl;

    // Copy the UTC into a stringstream and show it on the screen
    stringstream str;
    str << "stringstream: " << Time::sql << utc;
    cout << str.str() << endl;
    cout << endl;

    // Calculate the corresponsing MJD and shoud MJD and corresponding UTC
    const double mjd1 = utc.Mjd();
    cout << "Mjd:   " << Time::sql << utc << " (" << mjd1 << ")" << endl;

    // Set utc to the previously calculated MJD
    utc.Mjd(mjd1);

    // Re-calcualte MJD from this
    const double mjd2 = utc.Mjd();

    // Show the newly calculated MJD and time and the difference between both
    cout << "Mjd:   " << Time::sql << utc << " (" << mjd2 << ")" << endl;
    cout << "Diff:  " << mjd1 - mjd2 << endl;
    cout << endl;

    // Instantiate a Time object with an artificial time
    const Time bd(1974, 9, 9, 21, 59, 42, 123456);

    // Show it in two different representations
    cout << "Loc default:  " << Time::def << bd << endl;
    cout << "Standard:     " << Time::std << bd << endl;
    cout << endl;

    // Clear the stringstream contents
    str.str("");

    // Stream the time in its sql representation into the stringstream
    str << Time::ssql << bd;

    // Stream a time from the stringstream considering an sql representation
    // into a Time object
    Time tm;
    str >> Time::ssql >> tm;

    // Output stream and interpreted time
    cout << "Stream: " << str.str() << endl; 
    cout << "Time:   " << Time::ssql << tm << endl;
    cout << endl;

    // Print the individual elements of the date and the time
    cout << "Elements: ";
    cout << tm.Y() << " " << tm.M() << " " << tm.D() << " " ;
    cout << tm.h() << " " << tm.m() << " " << tm.s() << " " ;
    cout << tm.us() << endl;
    cout << endl;

    // Set and get a Time from a string
    const string s = "2042-12-24 12:42:42";

    Time tstr;
    tstr.SetFromStr(s);
    cout << "String:      " << s << endl;
    cout << "TimeFromStr: " << tstr.GetAsStr() << endl;
    cout << endl;

    // Calculate with times
    const Time t0;
    Time t1 = t0;
    cout << "T0  =          " << t0 << endl;
    cout << "T1  =          " << t1 << endl;
    t1 += hours(4242);
    cout << "T1 += 4242h:   " << t1 << endl;
    t1 += minutes(42);
    cout << "T1 += 42min:   " << t1 << endl;
    t1 += seconds(42);
    cout << "T1 += 42sec:   " << t1 << endl;
    cout << endl;

    cout << "T1 - T0 = " << t1-t0 << endl;

    const time_duration diff = t1-t0;
    cout << "T1 - T0 = " << diff.total_seconds() << "sec" << endl;
    cout << endl;

    return 0;
}

// **************************************************************************
/** @example time.cc

Example for the usage of the class Time

**/
// **************************************************************************
