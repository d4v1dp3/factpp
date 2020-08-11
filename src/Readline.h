#ifndef FACT_Readline
#define FACT_Readline

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class Readline
{
public:
    static bool RedirectionWrapper(std::ostream &out, bool (*function)());

protected:
    /// A pointer to the unique instance of Readline to let the static
    /// functions used as callback for readline call member function
    /// with an object. This makes overwriting easier.
    static Readline *This;

private:
    std::string fName;     /// Filename for the history file compiled in the constructor
    int fMaxLines;         /// Maximum number of lines in the history file

    std::ofstream fCommandLog;

    std::string fLastLine; /// Last line adde to history

    int fLine;
    int fSection;
    int fLabel;
    static int  fScriptDepth;
    static bool fStopScript;

    static std::string fExternalInput;

    /// Static member function which are used to adapt readline to ncurses
    static int    rl_ncurses_getc(FILE *);
    static int    rl_ncurses_startup();
    static void   rl_ncurses_redisplay();
    static int    rl_ncurses_event_hook();
    static void   rl_ncurses_completion_display(char **matches, int num, int max);
    static char **rl_ncurses_completion_function(const char *text, int start, int end);
    static char   *CompleteImp(const char* text, int state);


protected:
    static std::string fScript;

    /// The non static implementations of the callback funtions above
    virtual int  Getc(FILE *);
    virtual void Startup();
    virtual void EventHook(bool newline=false);
    virtual void Shutdown(const char *buf);
    virtual void Redisplay();
    virtual void CompletionDisplay(char **matches, int num, int max);

    /// Functions dealing with auto completion
    virtual char *Complete(const char* text, int state);
    virtual char **Completion(const char *text, int start, int end);

    /// Pointer to a list of possible matched for auto-completion
    const std::vector<std::string> *fCompletion; 
    void SetCompletion(const std::vector<std::string> *v) { fCompletion = v; }
    char **Complete(const std::vector<std::string> &v, const char *text);

    ///
    virtual void SetSection(int) { }
    virtual void PrintReadlineError(const std::string &str);

public:
    Readline(const char *prgname);
    virtual ~Readline();

    // Access to readline
    void BindKeySequence(const char *seq, int (*func)(int, int));

    static  bool DumpVariables();
    static  bool DumpFunctions();
    static  bool DumpFunmap();
    static  bool DumpHistory();

    virtual bool PrintGeneralHelp();
    virtual bool PrintCommands();
    virtual bool PrintKeyBindings();

    // History functions
    std::string GetName() const { return fName; }

    void AddToHistory(const std::string &str, int skip=2);
    static bool ClearHistory();
    std::vector<const char*> GetHistory() const;

    void SetMaxSize(int lines) { fMaxLines = lines; }

    // Prompting
    void UpdatePrompt(const std::string &prompt) const;
    void UpdatePrompt() const { UpdatePrompt(GetUpdatePrompt()); }

    virtual bool PreProcess(const std::string &str);
    virtual bool Process(const std::string &str);
    virtual std::string GetUpdatePrompt() const { return ""; }
    virtual bool PromptEOF(std::string &str);
    virtual std::string Prompt(const std::string &prompt);
    virtual void Run(const char *prompt=0);
    static  void Stop();
    virtual bool ExecuteShellCommand(const std::string &cmd);
    int          Execute(const std::string &fname, const std::map<std::string,std::string> &args=std::map<std::string,std::string>());
    bool         IsStopped() const;
    void         ProcessLine(const std::string &str);
    void         SetLabel(int l) { fLabel = l; }
    static void  StopScript() { fStopScript = true; }
    static bool  IsScriptStopped() { return fStopScript; }
    static int   GetScriptDepth() { return fScriptDepth; }
    static void  SetScriptDepth(unsigned int d) { fScriptDepth=d; }
    static void  SetExternalInput(const std::string &inp) { fExternalInput = inp; }

    static std::string GetScript() { return fScript; }
    static std::string GetExternalInput() { return fExternalInput; }

    int GetLine() const { return fLine; }
    virtual std::string GetLinePrompt() const;

    // Helper
    static char  *Compare(const std::string &str, const std::string &txt);
    static char **CompletionMatches(const char *text, char *(*func)(const char*, int));

    // I/O Streams
    static FILE *SetStreamOut(FILE *f);
    static FILE *SetStreamIn(FILE *f);

    // Other global readline variables
    static std::string GetPrompt();
    static std::string GetBuffer();
    static int GetAbsCursor();
    static int GetCursor();
    static int GetBufferLength();
    static int GetLineLength();

    // Screen size
    static void Resize();
    static void Resize(int w, int h);
    int GetCols() const;
    int GetRows() const;

    static Readline *Instance() { return This; }

    static void        StaticPushHistory(const std::string &fname);
    static std::string StaticPrompt(const std::string &prompt);
    static void        StaticPopHistory(const std::string &fname);
};

#endif
