#ifndef FACT_ReadlineColor
#define FACT_ReadlineColor

#include <ostream>

namespace ReadlineColor
{
    bool ExecuteShellCommand(std::ostream &out, const std::string &cmd);

    bool PrintBootMsg(std::ostream &out, const std::string &name, bool interactive=true);
    bool PrintAttributes(std::ostream &out);

    bool PrintGeneralHelp(std::ostream &out, const std::string &name);
    bool PrintCommands(std::ostream &out);
    bool PrintKeyBindings(std::ostream &out);

    bool Process(std::ostream &out, const std::string &str);
};

#endif
