#ifndef FACT_State
#define FACT_State

#include <string>
#include <vector>

#include "Time.h"

struct State
{
    int         index;    /// Index (e.g. 1)
    std::string name;     /// Name (e.g. 'Connected')
    std::string comment;  /// Description (e.g. 'Connection to hardware established.')
    Time        time;     /// Time of state change

    static std::vector<State> SplitStates(const std::string &buffer);

    static bool Compare(const State &i, const State &j) { return i.index<j.index; }

    State(int i=-256, const std::string &n="", const std::string &c="", const Time &t=Time(Time::none));
};

#endif
