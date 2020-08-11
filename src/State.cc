// **************************************************************************
/** @struct State

@brief A struct which stores an index, a comment and a name of a State

To have proper descriptions of states in the network, this struct provides
a simple storage for the properties of a state.

Assume you want to write a descriptive string for a state machine
with two states, it could look like this:

"1:Disconnected=Connection not established\n2:Connected=Connection established."

Such a string can then be converted with SplitStates into a vector
of State objects.

*/
// **************************************************************************
#include "State.h"

#include <sstream>
#include <algorithm>

#include "tools.h"

using namespace std;
using namespace Tools;

// --------------------------------------------------------------------------
//
//! Construct a Description object
//!
//! @param i
//!     Index of the state, e.g. 1
//!
//! @param n
//!     Name of the state, e.g. 'Connected'
//!
//! @param c
//!     Descriptive text of the state, e.g. "Connection to hardware established."
//
State::State(int i, const std::string &n, const std::string &c, const Time &t)
    : index(i), name(Trim(n)), comment(Trim(c)), time(t)
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
//!     A vector<State> containing all the states found.
//
vector<State> State::SplitStates(const string &buffer)
{
    vector<State> vec;

    string buf;
    stringstream stream(buffer);
    while (getline(stream, buf, '\n'))
    {
        if (buf.empty())
            continue;

        const size_t p1 = buf.find_first_of(':');
        const size_t p2 = buf.find_first_of('=');

        stringstream s(buf.substr(0, p1));

        int index;
        s >> index;

        const string name    = buf.substr(p1+1, p2-p1-1);
        const string comment = p2==string::npos ? "" : buf.substr(p2+1);

        vec.emplace_back(index, name, comment);
    }

    sort(vec.begin(), vec.end(), State::Compare);

    return vec;
}
