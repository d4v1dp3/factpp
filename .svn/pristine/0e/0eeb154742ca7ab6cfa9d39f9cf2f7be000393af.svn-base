#ifndef FACT_Console
#define FACT_Console

#include "Readline.h"
#include "WindowLog.h"

class ConsoleStream : public Readline
{
private:
    WindowLog fLogO;

    void PrintReadlineError(const std::string &str);

public:
    ConsoleStream(const char *name);
    ~ConsoleStream();

    void SetNullOutput(bool null) { fLogO.SetNullOutput(null); }

    // I/O
    WindowLog &GetStreamOut() { return fLogO; }
    WindowLog &GetStreamIn()  { return fLogO; }

    const WindowLog &GetStreamOut() const { return fLogO; }
    const WindowLog &GetStreamIn()  const { return fLogO; }

    void Lock() { }
    void Run(const char * = 0);
    void Unlock() { }
};



class Console : public Readline
{
private:
    WindowLog fLogO;
    WindowLog fLogI;

    bool fContinous;

    void PrintReadlineError(const std::string &str);

public:
    Console(const char *name);
    ~Console();

    // Console
    void SetContinous(bool cont) { fContinous = cont; }
    bool IsContinous() const { return fContinous; }

    // I/O
    WindowLog &GetStreamOut() { return fLogO; }
    WindowLog &GetStreamIn()  { return fLogI; }

    const WindowLog &GetStreamOut() const { return fLogO; }
    const WindowLog &GetStreamIn()  const { return fLogI; }

    // Readline
    bool PrintGeneralHelp();
    bool PrintCommands();
    bool PrintKeyBindings();

    void Lock();
    bool Process(const std::string &str);
    void Unlock();

    std::string GetLinePrompt() const;

    void Startup();
    void EventHook(bool);
    void Run(const char * = 0);
};

#endif
