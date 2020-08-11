#ifndef FACT_ReadlineWindow
#define FACT_ReadlineWindow

#include "Readline.h"

typedef struct _win_st WINDOW;

class ReadlineWindow : public Readline
{
    WINDOW *fWindow; /// Pointer to the panel for the input stream

    int fPromptX;    /// When the readline call is issued the x position at which the output will start is stored here
    int fPromptY;    /// When the readline call is issued the y position at which the output will start is stored here

    int fColor;      /// Color index in which the prompt should be displayed

protected:
    // The implementations of the callback funtions
    //int  Getc(FILE *);
    void Startup();
    void Redisplay();
    void EventHook(bool = false);
    void CompletionDisplay(char **matches, int num, int max);

    // Refresh the display before setting the cursor position
    virtual void Refresh() { }

    // Callback after readline has returned
    void Shutdown(const char *buf);

public:
    ReadlineWindow(const char *prgname);

    // Initialization
    void SetWindow(WINDOW *w);
    void SetColorPrompt(int col) { fColor = col; }

    // Move cursor to start of last prompt
    void RewindCursor() const;
};

#endif
