#ifndef FACT_Service
#define FACT_Service

struct Service
{
    std::string name;
    std::string server;
    std::string service;
    std::string format;
    bool   iscmd;
};

inline bool operator<(const Service& left, const Service& right)
{
    return left.name < right.name;
}
#endif
