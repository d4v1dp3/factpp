// **************************************************************************
/** @class Shell

@brief Implementation of a console based user shell with an input and output window

Shell is based on the ReadlineWindow class. It creates two windows
(panels) using ncurses which are used for input and output. The output
window panel is displayed on top of the input panel, but can be hidden
for example by pressing F1.

The idea is that continous messages like logging messages do not interfere
with the input area, although one still has both diplayed in the same
console window.

To get a list of the command and functions supported by Shell
type 'h' or 'help' at a command line prompt.

The usage is quite simple. Instantiate an object of Shell with the
programname as an argument. For its meaning see the base class
documentation Readline::Readline(). The created input- and output-
stream can be accessed through GetStreamIn() and GetStreamOut()
whihc are both ostreams, one redirected to the input window and the
other one redirected to the output window. Especially, GetStreamIn()
should not be used while the Readline prompt is in progress, but can for
example be used to display errors about what was entered.

The recommanded way of usage is:

\code

   static Shell shell("myprog"); // readline will read the myprog.his history file

   while (1)
   {
        string txt = shell.Prompt("prompt> ");
        if (txt=="quit)
           break;

        // ... do something ...

        shell.AddHistory(txt);
   }

   // On destruction redline will write the current history to the file
   // By declaring the Shell static the correct terminal is restored even
   // the program got killed (not if killed with SIGABRT)

\endcode

If for some reason the terminal is not correctly restored type (maybe blindly)
<I>reset</I> in your console. This should restor everything back to normal.

@section References

 - <A HREF="http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html">GNU Readline</A>
 - <A HREF="http://www.gnu.org/software/ncurses">GNU Ncurses</A>


@todo
 - Introduce the possibility to scroll in both windows
 - Add redisplay functionality for both panels if the console was resized

*/
// **************************************************************************
#include "Shell.h"

#include <fstream>
#include <iostream>

#include <signal.h>    // SIGWINCH
#include <sys/wait.h>  // waitpid
#include <sys/ioctl.h> // ioctl

#include <panel.h>

#define FORK_ALLOWED

using namespace std;

Shell *Shell::This = 0;

// --------------------------------------------------------------------------
//
//! This initializes the Ncurses environment. Since the ncurses environment
//! is global only one instance of this class is allowed.
//!
//! The first 8 color pairs (COLOR_PAIR) are set to the first 8 color
//! with default background.
//!
//! The shells windows and panels are created. And their pointers are
//! propagated to the two associated WindowLog streams.
//!
//! Also some key bindings are initialized.
//
Shell::Shell(const char *prgname) : ReadlineWindow(prgname),
    fPanelHeight(13), fIsVisible(1)
{
    if (stdscr!=0)
    {
        endwin();
        cout << "ERROR - Only one instance of class Shell is allowed." << endl;
        exit(-1);
    }

    This = this;

    // ---------------------- Setup ncurses -------------------------

    initscr();		      // Start curses mode

    cbreak();		      // Line buffering disabled, Pass on
    noecho();                 // Switch off echo mode
    nonl();                   // Associate return with CR

    intrflush(stdscr, FALSE);
    keypad(stdscr, FALSE);    // Switch off keymapping for function keys

    start_color();            // Initialize ncurses colors
    use_default_colors();     // Assign terminal default colors to -1
    //assume_default_colors(-1, -1); // standard terminal colors assigned to pair 0

    // Setup colors
    for (int i=1; i<8; i++)
        init_pair(i, i, -1);  // -1: def background

    signal(SIGWINCH, HandleResizeImp);  // Attach HandleResize to SIGWINCH signal

    // ---------------------- Setup pansl --------------------------

    // Create the necessary windows
    WINDOW *wins[4];
    CreateWindows(wins);

    // Initialize the panels
    fPanelIn    = new_panel(wins[0]);
    fPanelFrame = new_panel(wins[1]);
    fPanelOut   = new_panel(wins[2]);

    win.SetWindow(wins[0]);
    wout.SetWindow(wins[2]);

    // Get the panels into the right order for startup
    ShowHide(1);

    // Setup Readline
    SetWindow(wins[0]);
    SetColorPrompt(COLOR_PAIR(COLOR_BLUE));

    // ------------------- Setup key bindings -----------------------
    BindKeySequence("\033OP",    rl_proc_F1);
    BindKeySequence("\033[1;5B", rl_scroll_top);
    BindKeySequence("\033[1;5A", rl_scroll_top);
    BindKeySequence("\033[1;3A", rl_scroll_bot);
    BindKeySequence("\033[1;3B", rl_scroll_bot);
    BindKeySequence("\033[5;3~", rl_top_inc);
    BindKeySequence("\033[6;3~", rl_top_dec);
    BindKeySequence("\033+",     rl_top_resize);
    BindKeySequence("\033-",     rl_top_resize);

    /*
     rl_bind_keyseq("\033\t",   rl_complete); // Meta-Tab
     rl_bind_keyseq("\033[1~",  home); // Home (console)
     rl_bind_keyseq("\033[H",   home); // Home (x)
     rl_bind_keyseq("\033[4~",  end); // End (console)
     rl_bind_keyseq("\033[F",   end); // End (x)
     rl_bind_keyseq("\033[A",   up); // Up
     rl_bind_keyseq("\033[B",   down); // Down
     rl_bind_keyseq("\033[[B",  accept); // F2 (console)
     rl_bind_keyseq("\033OQ",   accept); // F2 (x)
     rl_bind_keyseq("\033[21~", cancel); // F10
     */

    // Ctrl+dn:   \033[1;5B
    // Ctrl+up:   \033[1;5A
    // Alt+up:    \033[1;3A
    // Alt+dn:    \033[1;3B
    // Alt+pg up: \033[5;3~
    // Alt+pg dn: \033[6;3~
}

// --------------------------------------------------------------------------
//
//! Ends the ncurses environment by calling endwin().
//
Shell::~Shell()
{
    // Maybe not needed because the window is more or less valid until the
    // object is destructed anyway.
    //win.SetWindow(0);
    //wout.SetWindow(0);
    //SetWindow(0);

    endwin();
    cout << "The end." << endl;
}

// --------------------------------------------------------------------------
//
//! This function gets the windows into the expected order which is:
//!
//! @param v
//!    - \b 0:  Do not show the output panel
//!    - \b 1:  Show the output panel
//!    - \b -1: Toggle the visibility of the output panel
//!    - \b -2: Just update the panels, do not change their visibility
//
void Shell::ShowHide(int v)
{
    if (v>-2)
        fIsVisible = v==-1 ? !fIsVisible : v;

    if (fIsVisible)
    {
        show_panel(fPanelIn);
        show_panel(fPanelFrame);
        show_panel(fPanelOut);
    }
    else
    {
        show_panel(fPanelIn);
        hide_panel(fPanelFrame);
        hide_panel(fPanelOut);
    }

    update_panels();
    doupdate();
}


// --------------------------------------------------------------------------
//
//! Creates the windows to be used as panels and draws a frame around one
//!
//! @param w
//!    pointers to the three windows which have been created are returned
//!
//! @param all
//!   If all is false do not (re-)create the bottom or input-window
//
void Shell::CreateWindows(WINDOW *w[3], int all)
{
    int maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    const int separator = maxy-fPanelHeight;

    WINDOW *new_in    = all ? newwin(maxy,    maxx,   0, 0) : 0;
    WINDOW *new_frame = newwin(separator-1,   maxx,   0, 0);
    WINDOW *new_out   = newwin(separator-1-2, maxx-2, 1, 1);

    box(new_frame,     0,0);
    wmove(new_frame,   0, 1);
    waddch(new_frame,  ACS_RTEE);
    wprintw(new_frame, " F1 ");
    waddch(new_frame,  ACS_LTEE);

    scrollok(new_out, true);
    leaveok (new_out, true);

    if (new_in)
    {
        scrollok(new_in, true);  // Allow scrolling
        leaveok (new_in, false);  // Move the cursor with the output

        wmove(new_in, maxy-1, 0);
    }

    w[0] = new_in;
    w[1] = new_frame;
    w[2] = new_out;
}

// --------------------------------------------------------------------------
//
//! Key binding for F1. Toggles upper panel by calling ShowHide(-1)
//
int Shell::rl_proc_F1(int /*cnt*/, int /*key*/)
{
    This->ShowHide(-1); // toggle
    return 0;
}

int Shell::rl_scroll_top(int, int key)
{
    This->win << "Scroll " << key << endl;
    return 0;
}

int Shell::rl_scroll_bot(int, int key)
{
    This->win << "Scroll " << key << endl;
    return 0;
}

int Shell::rl_top_inc(int, int key)
{
    This->win << "Increase " << key << endl;
    return 0;
}

int Shell::rl_top_dec(int, int key)
{
    This->win << "Increase " << key << endl;
    return 0;
}

int Shell::rl_top_resize(int, int key)
{
    This->Resize(key=='+' ? This->fPanelHeight-1 : This->fPanelHeight+1);
    return 0;
}


// --------------------------------------------------------------------------
//
//! Signal handler for SIGWINCH, calls HandleResize
//
void Shell::HandleResizeImp(int)
{
    This->HandleResize();
}

// --------------------------------------------------------------------------
//
//! Signal handler for SIGWINCH. It resizes the terminal and all panels
//! according to the new terminal size and redisplay the backlog buffer
//! in both windows
//!
//! @todo
//!   Maybe there are more efficient ways than to display the whole buffers
//
void Shell::HandleResize()
{
    // Get the new terminal size
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    // propagate it to the terminal
    resize_term(w.ws_row, w.ws_col);

    // Store the pointer to the old windows
    WINDOW *w_in    = panel_window(fPanelIn);
    WINDOW *w_frame = panel_window(fPanelFrame);
    WINDOW *w_out   = panel_window(fPanelOut);

    // Create new windows
    WINDOW *wins[3];
    CreateWindows(wins);

    // Redirect the streams and the readline output to the new windows
    win.SetWindow(wins[0]);
    wout.SetWindow(wins[2]);

    SetWindow(wins[0]);

    // Replace windows in the panels
    replace_panel(fPanelIn,    wins[0]);
    replace_panel(fPanelFrame, wins[1]);
    replace_panel(fPanelOut,   wins[2]);

    // delete the old obsolete windows
    delwin(w_in);
    delwin(w_out);
    delwin(w_frame);

    // FIXME:  NEEDED also in Redisplay panel
    //Redisplay();

    // Redisplay their contents
    win.Display();
    wout.Display();
}

// --------------------------------------------------------------------------
//
//! This resized the top panel or output panel as requested by the argument.
//! The argument is the number of lines which are kept free for the input
//! panel below the top panel
//!
//! @returns
//!    always true
//
bool Shell::Resize(int h)
{
    // Get curretn terminal size
    int lines, cols;
    getmaxyx(stdscr, lines, cols);

    // Check if we are in a valid range
    if (h<1 || h>lines-5)
        return false;

    // Set new height for panel to be kept free
    fPanelHeight = h;

    // Store the pointers of the old windows associated with the panels
    // which should be resized
    WINDOW *w_frame = panel_window(fPanelFrame);
    WINDOW *w_out   = panel_window(fPanelOut);

    // Create new windows
    WINDOW *wins[3];
    CreateWindows(wins, false);

    // Redirect the output stream to the new window
    wout.SetWindow(wins[2]);

    // Replace the windows associated with the panels
    replace_panel(fPanelFrame, wins[1]);
    replace_panel(fPanelOut,   wins[2]);

    // delete the ols windows
    delwin(w_out);
    delwin(w_frame);

    // FIXME:  NEEDED also in Redisplay panel
    //Redisplay();

    // Redisplay the contents
    wout.Display();

    return true;
}

bool Shell::PrintKeyBindings()
{
    ReadlineColor::PrintKeyBindings(win);
    win << " " << kUnderline << "Special key bindings:" << endl << endl;;
    win << kBold << "   F1              " << kReset << "Toggle visibility of upper panel" << endl;
    win << endl;
    return true;
}

bool Shell::PrintGeneralHelp()
{
    ReadlineColor::PrintGeneralHelp(win, GetName());
    win << kBold << "   hide         " << kReset << "Hide upper panel." << endl;
    win << kBold << "   show         " << kReset << "Show upper panel." << endl;
    win << kBold << "   height <h>   " << kReset << "Set height of upper panel to h." << endl;
    win << endl;
    return true;
}

// --------------------------------------------------------------------------
//
//! Processes the command provided by the Shell-class.
//!
//! @returns
//!    whether a command was successfully processed or could not be found
//
bool Shell::Process(const string &str)
{
    // Implement readline commands:
    //   rl set     (rl_variable_bind(..))
    //   rl_read_init_file(filename)
    //  int rl_add_defun (const char *name, rl_command_func_t *function, int key)

    if (ReadlineColor::Process(win, str))
        return true;

    if (Readline::Process(str))
        return true;

    // ----------- ReadlineNcurses -----------

    if (string(str)=="hide")
    {
        ShowHide(0);
        return true;
    }
    if (string(str)=="show")
    {
        ShowHide(1);
        return true;
    }

    if (str.substr(0, 7)=="height ")
    {
        int h;
        sscanf(str.c_str()+7, "%d", &h);
        return Resize(h);
    }

    if (str=="d")
    {
        wout.Display();
        return true;
    }

    return false;
}

// --------------------------------------------------------------------------
//
//! Overwrites Shutdown. It's main purpose is to re-output
//! the prompt and the buffer using the WindowLog stream so that it is
//! buffered in its backlog.
//!
//! @param buf
//!    A pointer to the buffer returned by readline
//!
void Shell::Shutdown(const char *buf)
{
    ReadlineWindow::Shutdown(buf);

    // Now move the cursor to the start of the prompt
    RewindCursor();

    // Output the text ourself to get it into the backlog
    // buffer of win. We cannot use GetBuffer() because rl_end
    // is not updated finally.
    win << kBlue << GetPrompt() << kReset << buf << endl;
}
