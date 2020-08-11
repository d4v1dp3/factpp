#ifndef FACT_StateMachine
#define FACT_StateMachine

#include "StateMachineImp.h"

class StateMachine : public StateMachineImp
{
protected:
    EventImp *CreateEvent(const std::string &name, const std::string &fmt);

public:
    StateMachine(std::ostream &out, const std::string &name="") :
        StateMachineImp(out, name)
    {
    }

    bool ProcessCommand(const std::string &str, const char *ptr, size_t siz);
    bool ProcessCommand(const EventImp &evt);
    bool ProcessCommand(const std::string &str) { return ProcessCommand(str, 0, 0); }

};

#endif
