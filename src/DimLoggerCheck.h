#ifndef FACT_DimLoggerCheck
#define FACT_DimLoggerCheck

#include <set>
#include <string>
#include <functional>

#include "tools.h"
#include "EventImp.h"
#include "DimDescriptionService.h"

class DimLoggerCheck
{
   //typedef std::function<int(const EventImp &)> callback;
    typedef std::function<int(const std::map<const std::string, const int> &)> callback;

    callback fCallback;

    const std::string name;
    std::map<const std::string, const int> list;

    virtual int Handler(const EventImp &evt)
    {
        using namespace std;

        const set<string>   vserv = DimDescribedService::GetServices();
        const vector<string> ltxt = Tools::Split(evt.GetString(), "\n");

        map<string, int> vsubs;
        for (auto it=ltxt.begin(); it!=ltxt.end(); it++)
        {
            const vector<string> col = Tools::Split(*it, ",");

            if (col.size()==2 && col[0].substr(0, name.size()+1)==name+"/")
                vsubs[col[0]] = atoi(col[1].c_str());
        }

        list.clear();
        for (auto it=vserv.begin(); it!=vserv.end(); it++)
        {
            const auto is = vsubs.find(*it);
            list.insert(make_pair(it->substr(name.size()+1), is==vsubs.end() ? -2 : is->second));
        }

        list.erase("STATE_LIST");

        return fCallback ? fCallback(list) : StateMachineImp::kSM_KeepState;
    }

public:
    DimLoggerCheck() { }
    DimLoggerCheck(const std::string &n) : name(n)
    {
    }
    virtual ~DimLoggerCheck()
    {
    }

    virtual void Subscribe(StateMachineImp &imp)
    {
        imp.Subscribe("DATA_LOGGER/SUBSCRIPTIONS")
            (imp.Wrap(std::bind(&DimLoggerCheck::Handler, this, std::placeholders::_1)));
    }

    void SetCallback(const callback &cb)
    {
        fCallback = cb;
    }

    const std::map<const std::string, const int> &GetList() const { return list; }
};

#endif
