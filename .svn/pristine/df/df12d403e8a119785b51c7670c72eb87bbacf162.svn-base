// **************************************************************************
/** @class ReadlineWindow

@brief Helper to redirect readline's in- and output to an ncurses window

The idea of this class is to allow the use of the readline functionality
within a ncurses window. Therefore, several callback and hook function
of the readline libarary are redirected, which allow to redirect the
window's input stream to readline and the readline output to the
window.

After instantisation a pointer to a ncurses WINDOW must be given to the
object to 'bind' readline to this window.

In addition the color of the readline prompt can be set using the
SetPromptColor() member function. Note that the given color must
be the index of a COLOR_PAIR. For details on this see the ncurses
documentation

If ncurses will use more than one window on the screen it might
be necessary to redraw the screen before the cursor is displayed.
Therefore, the Refresh() function must be overwritten. It is called
before the cursor is put to its final location in the readline line
and after all update to the screen was performed.

Refresh() can be used to force a redisplay of the current input line
from a derived class. This might be necessary after changes top the
screen or window size.

@section References

 - <A HREF="http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html">GNU Readline</A>
 - <A HREF="http://www.gnu.org/software/ncurses">GNU Ncurses</A>

**/
// **************************************************************************
#include "Shell.h"

#include <sstream>
#include <iostream>
#include <string.h> // strlen

#include "tools.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Propagate prgname to the Readline constructor.
//! Initialize fPromtX and fPromtY to 0.
//! Set fWindow to 0. 
//!
//! @param prgname
//!    see Readline::Readline()
//!
//! @todo
//!    Maybe we should add sanity check for fWindow==0 in the functions?
//!
ReadlineWindow::ReadlineWindow(const char *prgname) : Readline(prgname),
    fWindow(0), fPromptX(0), fPromptY(0)
{
}

// --------------------------------------------------------------------------
//
//! Set the window in which the readline prompt and all readline output
//! is displayed. Sets the readline screen size (rl_set_screen_size)
//! accoring to the size of the window.
//!
//! Setup all necessry callback functions. To redirect readline input
//! and output properly.
//!
//! @param w
//!    Pointer to a ncurses WINDOW.
//
void ReadlineWindow::SetWindow(WINDOW *w)
{
    if (!w)
        return;

    // Get size of the window
    int width, height;
    getmaxyx(w, height, width);

    // Propagate the size to the readline library
    Resize(width, height);

    // Finally set the pointer to the panel in which we are supposed to
    // operate
    fWindow = w;
}


// --------------------------------------------------------------------------
//
//! Callback function for readlines rl_getc_function. Apart from redirecting
//! the input from the window to the readline libarary, it also checks if
//! the input forced scrolling of the window contents and in this case
//! adapt fPromptY.
//!
//! @return
//!    the character read by wgetch
//
/*
int ReadlineWindow::Getc(FILE *)
{
    /// ====   It seems this is obsolete because we will get teh scrolling
    //         from the continous call to event hook anyway

    // Get size of the window
    int lines, cols;
    getmaxyx(fWindow, lines, cols);

    // Get current cursor position
    int x0, y0, y1, x1;
    getyx(fWindow, y0, x0);

    // Read a character from stream in window
    const int c = wgetch(fWindow);

    // Get new cursor position
    getyx(fWindow, y1, x1);

    // Find out whether the last character initiated a scroll
    if (y0==lines-1 && y1==lines-1 && x1==0 && x0==cols-1)
        fPromptY--;

    // return character
    return c;
}
*/

// --------------------------------------------------------------------------
//
//! Store the current cursor position in fPromptX/Y
//
void ReadlineWindow::Startup()
{
    getyx(fWindow, fPromptY, fPromptX);
}

// --------------------------------------------------------------------------
//
//! Move the cursor to the position stored in fPromptX/Y which should
//! correspond to the beginning of the output line
//
void ReadlineWindow::RewindCursor() const
{
    wmove(fWindow, fPromptY, fPromptX);
}

// --------------------------------------------------------------------------
//
//! The event hook which is called regularly when a readline call is in
//! progress. We use this to synchronously upadte our prompt (mainly
//! the current cursor position) and refresh the screen, so that all
//! changes get displayed soon.
//!
//! By default, this will be called at most ten times a second if there
//! is no keyboard input.
//
void ReadlineWindow::EventHook(bool)
{
    Readline::EventHook();
    Redisplay();
    /*
     * This doesn't work if the contents of the line changes, e.g. when
     * the prompt is replaced

    // Refresh the screen
    Refresh();

    // Now move the cursor to its expected position
    int lines, cols;
    getmaxyx(fWindow, lines, cols);

    const int pos = fPromptY*cols + fPromptX + GetAbsCursor();
    wmove(fWindow, pos/cols, pos%cols);

    // Make the cursor movement visible on the screen
    wrefresh(fWindow);
    */

    // The lines above are just a simplified version of Redisplay()
    // which skips all the output.
}

// --------------------------------------------------------------------------
//
//! This basically implement displaying the whole line, starting with the
//! prompt and the rl_line_buffer. It also checks if displaying it
//! results in a scroll of the window and adapt fPromptY accordingly.
//!
//! The prompt is displayed in the color defined by fColor.
//!
//! Before the cursor position is finally set a screen refresh (Refresh())
//! is initiated to ensure that nothing afterwards will change the cursor
//! position. It might be necessary to overwrite this function.
//!
//! @todo fix docu
//
void ReadlineWindow::Redisplay()
{
    // Move to the beginning of the output
    wmove(fWindow, fPromptY, fPromptX);

    // Get site of the window
    int lines, cols;
    getmaxyx(fWindow, lines, cols);

    const string prompt = GetPrompt();
    const string buffer = GetBuffer();

    // Issue promt and redisplay text
    wattron(fWindow, fColor);
    wprintw(fWindow, "%s", prompt.c_str());
    wattroff(fWindow, fColor);
    wprintw(fWindow, "%s", buffer.c_str());

    // Clear everything after that
    wclrtobot(fWindow);

    // Calculate absolute position in window or beginning of output
    int xy = fPromptY*cols + fPromptX;

    // Calculate position of end of prompt
    xy += prompt.length();

    // Calculate position of cursor and end of output
    const int cur = xy + GetCursor();
    const int end = xy + buffer.size();

    // Calculate if the above output scrolled the window and by how many lines
    int scrolls = end/cols - lines + 1;

    if (scrolls<0)
        scrolls = 0;

    fPromptY -= scrolls;

    // new position
    const int px = cur%cols;
    const int py = scrolls>=1 ? cur/cols - scrolls : cur/cols;

    // Make sure that whatever happens while typing the correct
    // screen is shown (otherwise the top-panel disappears when
    // we scroll)
    Refresh();

    // Move the cursor to the cursor position
    wmove(fWindow, py, px);

    // Make changes visible on screen
    wrefresh(fWindow);
}

// --------------------------------------------------------------------------
//
//! Callback function to display the finally compiled list of completion
//! options. Adapts fPromtY for the number of scrolled lines.
//!
//! @param matches
//!    A list with the matches found. The first element contains
//!    what was completed. The length of the list is therefore
//!    num+1.
//!
//! @param num
//!    Number of possible completion entries in the list.
//!
//! @param max
//!    maximum length of the entries
//!
//! @todo
//!    Maybe we can use rl_outstream here if we find a way to redirect
//!    the stream to us instead of a file.
//
void ReadlineWindow::CompletionDisplay(char **matches, int num, int max)
{
    // Increase maximum size by two to get gaps in between the columns
    max += 2; // Two whitespaces in between the output

    // Get size of window
    int lines, cols;
    getmaxyx(fWindow, lines, cols);

    // Allow an empty space at the end of the lines for a '\n'
    cols--;

    // calculate the final number columns
    const int ncols = cols / max;

    // Compile a proper format string
    ostringstream fmt;
    fmt << "%-" << max << 's';

    // loop over all entries and display them
    int l=0;
    for (int i=0; i<num; i++)
    {
        // Check if we have to put a line-break
        if (i%ncols==0)
        {
            if ((max+0)*ncols < cols)
                wprintw(fWindow, "\n");
            l++;
        }

        // Display an entry
        wprintw(fWindow, fmt.str().c_str(), matches[i+1]);
    }

    // Display an empty line after the list
    if ((num-1)%ncols>0)
        wprintw(fWindow, "\n");
    wprintw(fWindow, "\n");

    // Get new cursor position
    int x, y;
    getyx(fWindow, y, x);

    // Clear everything behind the list
    wclrtobot(fWindow);

    // Adapt fPromptY for the number of scrolled lines if any.
    if (y==lines-1)
        fPromptY = lines-1;

    // Display anything
    wrefresh(fWindow);
}

// --------------------------------------------------------------------------
//
//! Overwrites Shutdown() of Readline. After Readline::Prompt has
//! returned a Redisplay() is forced to ensure a proper display of
//! everything. Finally, the display line is ended by a \n.
//!
//! @param buf
//!    The buffer returned by the readline call
//
void ReadlineWindow::Shutdown(const char *buf)
{
    // Move the cursor to the end of the total line entered by the user
    // (the user might have pressed return in the middle of the line)...
    int lines, cols;
    getmaxyx(fWindow, lines, cols);

    // Calculate absolute position in window or beginning of output
    // We can't take a pointer to the buffer because rl_end is not
    // valid anymore at the end of a readline call
    int xy = fPromptY*cols + fPromptX + GetPrompt().size() + strlen(buf);
    wmove(fWindow, xy/cols, xy%cols);

    // ...and output a newline. We have to do the manually.
    wprintw(fWindow, "\n");

    // refresh the screen
    wrefresh(fWindow);

    // This might have scrolled the window
    if (xy/cols==lines-1)
        fPromptY--;
}
