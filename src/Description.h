#ifndef FACT_Description
#define FACT_Description

#include <string>
#include <vector>

struct Description
{
    std::string name;
    std::string comment;
    std::string unit;

    static std::vector<Description> SplitDescription(const std::string &buffer);
    static std::string GetHtmlDescription(const std::vector<Description> &vec);

    Description(const std::string &n, const std::string &c, const std::string &u="");
};

#endif
