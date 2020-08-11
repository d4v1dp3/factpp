#ifndef FACT_StateMachineDimControl
#define FACT_StateMachineDimControl

#include <set>

#include "DimState.h"
#include "StateMachineDim.h"

class Configuration;

class StateMachineDimControl : public StateMachineDim
{
    std::mutex fMutex;

    std::vector<DimDescriptions*> fDimDescriptionsList;

    std::set<std::string> fServerList;
    std::set<Service> fServiceList;
    std::map<std::string, std::vector<std::string>> fCommandList;
    std::map<std::string, State> fCurrentStateList;
    std::map<std::pair<std::string, int32_t>, std::pair<std::string, std::string>> fStateDescriptionList;
    std::map<std::string, std::vector<Description>> fServiceDescriptionList;

    std::function<void(const std::string &, const State&)> fStateCallback;

    DimVersion fDim;
    DimDnsServiceList fDimList;

    bool fDebug;

    std::string fUser;
    std::string fScriptUser;

    /// Default arguments provided to every java script
    std::string fArgumentsJS;

    std::function<int(const EventImp &)> fInterruptHandler;

    std::string Line(const std::string &txt, char fill);

public:
    static bool fIsServer;

    int ChangeState(int qos, const Time &time, int scriptdepth, std::string scriptfile, std::string user);
    int ChangeState(int state);

    int StartScript(const EventImp &imp, const std::string &cmd);
    int StopScript(const EventImp &imp);
    int InterruptScript(const EventImp &imp);

    int HandleStateChange(const std::string &server, DimDescriptions *state);
    int HandleDescriptions(DimDescriptions *state);
    int HandleStates(const std::string &server, DimDescriptions *state);
    int HandleServerAdd(const std::string &server);
    int HandleServerRemove(const std::string &server);
    int HandleAddService(const Service &svc);

    bool HasServer(const std::string &server);

    std::vector<std::string> GetServerList();
    std::vector<std::string> GetCommandList(const std::string &server);
    std::vector<std::string> GetCommandList();
    std::vector<Description> GetDescription(const std::string &service);
    std::vector<State>       GetStates(const std::string &server);
    std::set<Service>        GetServiceList();

    std::string              GetArguments() const { return fArgumentsJS; }

    int PrintStates(std::ostream &out, const std::string &serv="");
    int PrintDescription(std::ostream &out, bool iscmd, const std::string &serv="", const std::string &service="");

    State GetServerState(const std::string &server);

    bool SendDimCommand(const std::string &server, std::string str, std::ostream &lout);

    void SetStateCallback(const std::function<void(const std::string &, const State &)> &func) { fStateCallback = func; }

    void SetInterruptHandler(const std::function<int(const EventImp &)> &func=std::function<int(const EventImp &)>()) { fInterruptHandler = func; }

    void Stop(int code=0);

public:
    StateMachineDimControl(std::ostream &out=std::cout);
    ~StateMachineDimControl();

    int EvalOptions(Configuration &conf);
};

#endif
