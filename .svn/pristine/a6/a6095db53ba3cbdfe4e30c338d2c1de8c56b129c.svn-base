#ifndef FACT_Shell
#define FACT_Shell

#include "ReadlineWindow.h"
#include "ReadlineColor.h"
#include "WindowLog.h"

class WindowLog;

typedef struct panel PANEL;

class Shell : public ReadlineWindow
{
protected:
    static Shell *This; /// pointer to our glocal object to get the static member functions into scope

    WindowLog win;//(&fPanelIn);    // FIXME: Ref
    WindowLog wout;//(&fPanelOut);  // FIXME: Ref

private:
    PANEL *fPanelIn;    /// Pointer to the panel for the input stream
    PANEL *fPanelFrame; /// Pointer to the panel for the frame around the output
    PANEL *fPanelOut;   /// Pointer to the panel for the output stream

    int fPanelHeight;   /// Space between the bottom of the screen and the output panel
    int fIsVisible;     /// Flag whether the output panel is visible or not (for toggle operations)

    // Callback function for key presses
    static int rl_proc_F1(int cnt, int key);
    static int rl_scroll_top(int cnt, int key);
    static int rl_scroll_bot(int cnt, int key);
    static int rl_top_inc(int cnt, int key);
    static int rl_top_dec(int cnt, int key);
    static int rl_top_resize(int cnt, int key);

    /// Static member function used as callback for a signal which is
    /// emitted by the system if the size of the console window has changed
    static void HandleResizeImp(int dummy);

    /// Non static member function called by HandleResize
    void HandleResize();

    /// Helper for the constructor and window resizing to create the windows and panels
    void CreateWindows(WINDOW *w[3], int all=true);

    // Action after readline finished
    void Shutdown(const char *);

public:
    Shell(const char *prgname);
    ~Shell();

    bool Resize(int h);
    void ShowHide(int v);
    void Refresh() { ShowHide(-2); }

    bool PrintCommands() { return ReadlineColor::PrintCommands(win); }
    bool PrintGeneralHelp();
    bool PrintKeyBindings();

    bool Process(const std::string &str);

    void Lock() { }
    void Run(const char * = "")
    {
        ReadlineColor::PrintBootMsg(win, GetName());
        ReadlineWindow::Run();
    }
    void Unlock() { }

    WindowLog &GetStreamOut() { return wout; }
    WindowLog &GetStreamIn() { return win; }
    const WindowLog &GetStreamOut() const { return wout; }
    const WindowLog &GetStreamIn() const { return win; }
};

#endif
