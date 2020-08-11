// **************************************************************************
/** @class Readline

@brief C++ wrapper for GNU's readline library

This class is meant as a C++ wrapper around GNU's readline library.
Note that because readline uses a global namespace only one instance
of this class can exist at a time. Instantiating a second object after
a first one was deleted might show unexpected results.

When the object is instantiated readline's history is read from a file.
At destruction the history in memory is copied back to that file.
The history file will be truncated to fMaxLines.

By overloading the Readline class the function used for auto-completion
can be overwritten.

Simple example:

\code

   Readline rl("MyProg"); // will read the history from "MyProg.his"
   while (1)
   {
        string txt = rl.Prompt("prompt> ");
        if (txt=="quit)
           break;

        // ... do something ...

        rl.AddHistory(txt);
   }

   // On destruction the history will be written to the file

\endcode

Simpler example (you need to implement the Process() function)

\code

   Readline rl("MyProg"); // will read the history from "MyProg.his"
   rl.Run("prompt> ");

   // On destruction the history will be written to the file

\endcode

@section References

 - <A HREF="http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html">GNU Readline</A>
 - <A HREF="http://www.rendezvousalpha.com/f/bash_d596c169?fn=1670">GNU Readline (src code)</A>

 */
// **************************************************************************
#include "Readline.h"

#include <sstream>
#include <fstream>
#include <iostream>

#include <sys/ioctl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

#include "tools.h"
#include "Time.h"

namespace fs = boost::filesystem;

using namespace std;

Readline   *Readline::This   =  0;
bool        Readline::fStopScript = false;
int         Readline::fScriptDepth = 0;
std::string Readline::fScript;
std::string Readline::fExternalInput;

// --------------------------------------------------------------------------
//
//! Construct a Readline object. The constructor reads the history from a
//! history file. The filename is compiled by adding ".his" to the
//! supplied argument. The name oif the history file is stored in fName.
//!
//! Since readline has a global namespace, the creation of only one
//! Readline instance is allowed.
//!
//! The provided program name is supplied to readline by means of
//! rl_readline_name.
//!
//! Readlines default callback frunction for completions is redirected
//! to CompletionImp which in turn will call Completion, which can be
//! overwritten by the user.
//!
//! Bind some default key sequences like Page-up/-down for searching forward
//! and backward in history.
//!
//! @param prgname
//!    The prefix of the history filename. Usually the program name, which
//!    can be initialized by argv[0].
//
Readline::Readline(const char *prgname) :
    fMaxLines(500), fLine(0), fSection(-4), fLabel(-1), fCompletion(0)
{
    if (This)
    {
        cout << "ERROR - Readline can only be instatiated once!" << endl;
        exit(-1);
    }

    This = this;

    // Alternative completion function
    rl_attempted_completion_function = rl_ncurses_completion_function;

    // Program name
#if BOOST_VERSION < 104600
    static const string fname = boost::filesystem::path(prgname).filename();
#else
    static const string fname = boost::filesystem::path(prgname).filename().string();
#endif
    rl_readline_name = fname.c_str();

    // Compile filename for history file
    fName = string(prgname)+".his";

    // Read history file
    read_history(fName.c_str());
    //if (read_history(fName.c_str()))
    //    cout << "WARNING - Reading " << fName << ": " << strerror(errno) << endl;

    fCommandLog.open(string(prgname)+".evt");

    // Setup the readline callback which are needed to redirect
    // the otuput properly to our ncurses panel
    rl_getc_function                   = rl_ncurses_getc;
    rl_startup_hook                    = rl_ncurses_startup;
    rl_redisplay_function              = rl_ncurses_redisplay;
    rl_event_hook                      = rl_ncurses_event_hook;
    rl_completion_display_matches_hook = rl_ncurses_completion_display;

    // Bind delete, page up, page down
    rl_bind_keyseq("\e[1~",  rl_named_function("beginning-of-line"));
    rl_bind_keyseq("\e[3~",  rl_named_function("delete-char"));
    rl_bind_keyseq("\e[4~",  rl_named_function("end-of-line"));
    rl_bind_keyseq("\e[5~",  rl_named_function("history-search-backward"));
    rl_bind_keyseq("\e[6~",  rl_named_function("history-search-forward"));
    rl_bind_keyseq("\033[1;3F", rl_named_function("kill-line"));
    rl_bind_keyseq("\033[1;5D", rl_named_function("backward-word"));
    rl_bind_keyseq("\033[1;5C", rl_named_function("forward-word"));
    rl_bind_key(25, rl_named_function("kill-whole-line"));

    //for (int i=0; i<10; i++) cout << (int)getchar() << endl;
}

// --------------------------------------------------------------------------
//
//! Writes the current history to the file with the name stored in fName.
//! In addition the written file is truncated to fMaxLines to keep the
//! file of a reasonable size. The number of lines fMaxLines can be set
//! by SetMaxLines before the destructor is called. Setting fMaxLines
//! to 0 or a negative value switches automatic truncation off.
//
Readline::~Readline()
{
    // Write current history to file
    if (write_history(fName.c_str()))
        cout << "WARNING - Write " << fName.c_str() << ": " << strerror(errno) << endl;

    // Truncate file
    if (fMaxLines>0 && history_truncate_file(fName.c_str(), fMaxLines))
        cout << "WARNING - Truncate " << fName.c_str() << ": " << strerror(errno) << endl;
}

// --------------------------------------------------------------------------
//
//! This wraps the given readline function such that the output can be
//! redirected from thr rl_outstream to the given C++ ostream.
//!
//! @param out
//!    The stream to which the output should be redirected.
//!
//! @param function
//!    Takes a function of type bool(*)() as argument
//!
//! @returns
//!    The return value of the function
//
bool Readline::RedirectionWrapper(ostream &out, bool (*function)())
{
    FILE *save = SetStreamOut(tmpfile());
    const bool rc = function();
    FILE *file = SetStreamOut(save);

    const bool empty = ftell(file)==0;

    rewind(file);

    if (empty)
    {
        out << " <empty>" << endl;
        fclose(file);
        return rc;
    }

    while (1)
    {
        const int c = getc(file);
        if (feof(file))
            break;
        out << (char)c;
    }
    out << endl;

    fclose(file);

    return rc;
}

// --------------------------------------------------------------------------
//
//! Redirected from rl_getc_function, calls Getc
//
int Readline::rl_ncurses_getc(FILE *f)
{
    return This->Getc(f);
}

// --------------------------------------------------------------------------
//
//! Redirected from rl_startup_hook, calls Startup.
//! A function called just before readline prints the first prompt.
//
int Readline::rl_ncurses_startup()
{
    This->Startup();
    return 0; // What is this for?
}

// --------------------------------------------------------------------------
//
//! Redirected from rl_redisplay_function, calls Redisplay.
//! Readline will call indirectly to update the display with the current
//! contents of the editing buffer. 
//
void Readline::rl_ncurses_redisplay()
{
    This->Redisplay();
}

// --------------------------------------------------------------------------
//
//! Redirected from rl_event_hook, calls Update().
//! A function called periodically when readline is waiting for
//! terminal input.
//!
int Readline::rl_ncurses_event_hook()
{
    This->EventHook();
    return 0;
}

// --------------------------------------------------------------------------
//
//! Redirected from rl_completion_display_matches_hook,
//! calls CompletionDisplayImp
//!
//! A function to be called when completing a word would normally display
//! the list of possible matches. This function is called in lieu of
//! Readline displaying the list. It takes three arguments:
//! (char **matches, int num_matches, int max_length) where matches is
//! the array of matching strings, num_matches is the number of strings
//! in that array, and max_length is the length of the longest string in
//! that array. Readline provides a convenience function,
//! rl_display_match_list, that takes care of doing the display to
//! Readline's output stream. 
//
void Readline::rl_ncurses_completion_display(char **matches, int num, int max)
{
    This->CompletionDisplay(matches, num, max);
}

char **Readline::rl_ncurses_completion_function(const char *text, int start, int end)
{
    return This->Completion(text, start, end);
}

// --------------------------------------------------------------------------
//
//! Calls the default rl_getc function.
//
int  Readline::Getc(FILE *f)
{
    return rl_getc(f);
}

// --------------------------------------------------------------------------
//
//! Default: Do nothing.
//
void Readline::Startup()
{
}

// --------------------------------------------------------------------------
//
//! The default is to redisplay the prompt which is gotten from
//! GetUpdatePrompt(). If GetUpdatePrompt() returns an empty string the
//! prompt is kept untouched. This can be used to keep a prompt updated
//! with some information (e.g. time) just by overwriting GetUpdatePrompt()
//!
void Readline::EventHook(bool newline)
{
    const string cpy = fExternalInput;
    fExternalInput = "";

    if (!cpy.empty())
    {
        rl_replace_line(cpy.c_str(), 1);
        rl_done = 1;
    }

    string p = GetUpdatePrompt();
    if (p.empty())
        p = rl_prompt;

    if (newline)
        rl_on_new_line();

    if (rl_prompt==p && !newline)
        return;

    UpdatePrompt(p);

    int w, h;
    rl_get_screen_size(&h, &w);
    cout << '\r' << string(w+1, ' ') << '\r';
    rl_forced_update_display();
}

// --------------------------------------------------------------------------
//
//! Called from Prompt and PromptEOF after readline has returned. It is
//! meant as the opposite of Startup (called after readline finsihes)
//! The default is to do nothing.
//!
//! @param buf
//!    A pointer to the buffer returned by readline
//
void Readline::Shutdown(const char *)
{
}

// --------------------------------------------------------------------------
//
//! Default: call rl_redisplay()
//

void Readline::Redisplay()
{
    static int W=-1, H=-1;

    int w, h;
    rl_get_screen_size(&h, &w);
    if (W==w && h==H)
    {
        rl_redisplay();
        return;
    }

    cout << '\r' << string(w+1, ' ') << '\r';

    W=w;
    H=h;

    rl_forced_update_display();
}

// --------------------------------------------------------------------------
//
//! Default: call rl_completion_display_matches()
//
void Readline::CompletionDisplay(char **matches, int num, int max)
{
    rl_display_match_list(matches, num, max);
    rl_forced_update_display();
}

// --------------------------------------------------------------------------
//
//! This is a static helper for the compilation of a completion-list.
//! It compares the two inputs (str and txt) to a maximum of the size of
//! txt. If they match, memory is allocated with malloc and a pointer to
//! the null-terminated version of str is returned.
//!
//! @param str
//!    A reference to the string which is checked (e.g. "Makefile.am")
//!
//! @param txt
//!    A reference to the part of the string the user has already typed,
//!    e.g. "Makef"
//!
//! @returns
//!    A pointer to memory allocated with malloc containing the string str
//
char *Readline::Compare(const string &str, const string &txt)
{
    /*return strncmp(str.c_str(), txt.c_str(), txt.length())==0 ? */
    return strncasecmp(str.c_str(), txt.c_str(), txt.length())==0 ?
        strndup(str.c_str(), str.length()) : 0;
}

char **Readline::CompletionMatches(const char *text, char *(*func)(const char*, int))
{
    return rl_completion_matches(text, func);
}

// --------------------------------------------------------------------------
//
//! The given vector should be a reference to a vector of strings
//! containing all possible matches. The actual match-making is then
//! done in Complete(const char *, int)
//!
//! The pointer fCompletion is redirected to the vector for the run time
//! of the function, but restored afterwards. So by this you can set a
//! default completion list in case Complete is not called or Completion
//! not overloaded.
//!
//! @param v
//!    reference to a vector of strings with all possible matches
//!
//! @param text
//!    the text which should be matched (it is just propagated to
//!    Readline::Completion)
//!
char **Readline::Complete(const vector<string> &v, const char *text)
{
    const vector<string> *save = fCompletion;

    fCompletion = &v;
    char **rc = rl_completion_matches(const_cast<char*>(text), CompleteImp);
    fCompletion = save;

    return rc;
}

// --------------------------------------------------------------------------
//
//! If fCompletion==0 the default is to call readline's
//! rl_filename_completion_function. Otherwise the contents of fCompletion
//! are returned. To change fCompletion either initialize it via
//! SetCompletion() (in this case you must ensure the life time of the
//! object) or call
//!    Complete(const vector<string>&, const char*)
//! from
//!    Completion(const char * int, int)
//!
//! This is the so called generator function, the readline manual says
//! about this function:
//!
//!   The generator function is called repeatedly from
//!   rl_completion_matches(), returning a string each time. The arguments
//!   to the generator function are text and state. text is the partial word
//!   to be completed. state is zero the first time the function is called,
//!   allowing the generator to perform any necessary initialization, and a
//!   positive non-zero integer for each subsequent call. The generator
//!   function returns (char *)NULL to inform rl_completion_matches() that
//!   there are no more possibilities left. Usually the generator function
//!   computes the list of possible completions when state is zero, and
//!   returns them one at a time on subsequent calls. Each string the
//!   generator function returns as a match must be allocated with malloc();
//!   Readline frees the strings when it has finished with them.
//
char *Readline::Complete(const char* text, int state)
{
    if (fCompletion==0)
        return rl_filename_completion_function(text, state);

    static vector<string>::const_iterator pos;
    if (state==0)
        pos = fCompletion->begin();

    while (pos!=fCompletion->end())
    {
        char *rc = Compare(*pos++, text);
        if (rc)
            return rc;
    }

    return 0;
}

// --------------------------------------------------------------------------
//
//! Calls Complete()
//
char *Readline::CompleteImp(const char* text, int state)
{
    return This->Complete(text, state);
}

// --------------------------------------------------------------------------
//
//! The readline manual says about this function:
//!
//!   A  pointer to an alternative function to create matches. The
//!   function is called with text, start, and end. start and end are
//!   indices in rl_line_buffer saying what the boundaries of text are.
//!   If this function exists and returns NULL, or if this variable is
//!   set to NULL, then rl_complete() will call the value of
//!   rl_completion_entry_function to generate matches, otherwise the
//!   array of strings returned will be used.
//!
//! This function is virtual and can be overwritten. It defaults to
//! a call to rl_completion_matches with CompleteImp as an argument
//! which defaults to filename completion, but can also be overwritten.
//!
//! It is suggested that you call
//!    Complete(const vector<string>&, const char*)
//! from here.
//!
//! @param text
//!    A pointer to a char array conatining the text which should be
//!    completed. The text is null-terminated.
//!
//! @param start
//!    The start index within readline's line buffer rl_line_buffer,
//!    at which the text starts which presumably should be completed.
//!
//! @param end
//!    The end index within readline's line buffer rl_line_buffer,
//!    at which the text ends which presumably should be completed.
//!
//! @returns
//!    An array of strings which were allocated with malloc and which
//!    will be freed by readline with the possible matches.
//
char **Readline::Completion(const char *text, int /*start*/, int /*end*/)
{
    // To do filename completion call
    return rl_completion_matches((char*)text, CompleteImp);
}

// --------------------------------------------------------------------------
//
//! Adds the given string to the history buffer of readline's history by
//! calling add_history. 
//!
//! @param str
//!    A reference to a string which should be added to readline's
//!    history.
//!
//! @param skip
//!    If skip is 1 and str matches the last added entry in the history,
//!    the entry is skipped. If skip==2, all entries matching str are
//!    removed from the history before the new entry is added as last one.
//!    <skip==2 is the default>
//
void Readline::AddToHistory(const string &str, int skip)
{
    if (skip==1 && fLastLine==str)
        return;

    if (str.empty())
        return;

    int p = -1;
    while (skip==2)
    {
        p = history_search_pos(str.c_str(), 0, p+1);
        if (p<0)
            break;

        HIST_ENTRY *e = remove_history(p--);

        free(e->line);
        free(e);
    }

    add_history(str.c_str());
    fLastLine = str;
}

// --------------------------------------------------------------------------
//
//! @returns
//!     a string containing [{fLine}]
//
string Readline::GetLinePrompt() const
{
    ostringstream str;
    str << '[' << fLine << ']';
    return str.str();
}

// --------------------------------------------------------------------------
//
//! Calls rl_set_prompt. This can be used from readline's callback function
//! to change the prompt while a call to the readline function is in
//! progress.
//!
//! @param prompt
//!     The new prompt to be shown
//
void Readline::UpdatePrompt(const string &prompt) const
{
    rl_set_prompt(prompt.c_str());
}

// --------------------------------------------------------------------------
//
//! This function is used to bind a key sequence via a call to
//! rl_bind_keyseq.
//!
//! Readline's manual says about this function:
//!
//!   Bind the key sequence represented by the string keyseq to the
//!   function function, beginning in the current keymap. This makes
//!   new keymaps as necessary. The return value is non-zero if keyseq
//!   is invalid.
//!
//! Key sequences are escaped sequences of characters read from an input
//! stream when a special key is pressed. This is necessary because
//! there are usually more keys and possible combinations than ascii codes.
//!
//! Possible key sequences are for example:
//!   "\033OP"       F1
//!   "\033[1;5A"    Ctrl+up
//!   "\033[1;5B"    Ctrl+down
//!   "\033[1;3A"    Alt+up
//!   "\033[1;3B"    Alt+down
//!   "\033[5;3~"    Alt+page up
//!   "\033[6;3~"    Alt+page down
//!   "\033+"        Alt++
//!   "\033-"        Alt+-
//!   "\033\t"       Alt+tab
//!   "\033[1~"      Alt+tab
//!
//! @param seq
//!     The key sequence to be bound
//!
//! @param func
//!     A function of type "int func(int, int)
//
void Readline::BindKeySequence(const char *seq, int (*func)(int, int))
{
    rl_bind_keyseq(seq, func);
}

// --------------------------------------------------------------------------
//
//! Calls rl_variable_dumper(1)
//!
//!   Print the readline variable names and their current values
//!   to rl_outstream. If readable is non-zero, the list is formatted
//!   in such a way that it can be made part of an inputrc file and
//!   re-read.
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
bool Readline::DumpVariables()
{
    rl_variable_dumper(1);
    return true;
}

// --------------------------------------------------------------------------
//
//! Calls rl_function_dumper(1)
//!
//!   Print the readline function names and the key sequences currently
//!   bound to them to rl_outstream. If readable is non-zero, the list
//!   is formatted in such a way that it can be made part of an inputrc
//!   file and re-read.
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
bool Readline::DumpFunctions()
{
    rl_function_dumper(1);
    return true;
}

// --------------------------------------------------------------------------
//
//! Calls rl_list_funmap_names()
//!
//!    Print the names of all bindable Readline functions to rl_outstream.
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
bool Readline::DumpFunmap()
{
    rl_list_funmap_names();
    return true;
}

// --------------------------------------------------------------------------
//
//! Sets rl_outstream (the stdio stream to which Readline performs output)
//! to the new stream.
//!
//! @param f
//!    The new stdio stream to which readline should perform its output
//!
//! @return
//!    The old stream to which readline was performing output
//
FILE *Readline::SetStreamOut(FILE *f)
{
    FILE *rc = rl_outstream;
    rl_outstream = f;
    return rc;
}

// --------------------------------------------------------------------------
//
//! Sets rl_instream (the stdio stream from which Readline reads input)
//! to the new stream.
//!
//! @param f
//!    The new stdio stream from which readline should read its input
//!
//! @return
//!    The old stream from which readline was reading it input
//
FILE *Readline::SetStreamIn(FILE *f)
{
    FILE *rc = rl_instream;
    rl_instream = f;
    return rc;
}

// --------------------------------------------------------------------------
//
//! return rl_display_prompt (the prompt which should currently be
//! displayed on the screen) while a readline command is in progress
//
string Readline::GetPrompt()
{
    return rl_display_prompt;
}

// --------------------------------------------------------------------------
//
//! return rl_line_buffer (the current input line which should currently be
//! displayed on the screen) while a readline command is in progress
//!
//! The length of the current line buffer (rl_end) is available as
//! GetLineBuffer().size()
//!
//! Note that after readline has returned the contents of rl_end might
//! not reflect the correct buffer length anymore, hence, the returned buffer
//! might be truncated.
//
string Readline::GetBuffer()
{
    return string(rl_line_buffer, rl_end);
}

// --------------------------------------------------------------------------
//
//! return rl_point (the current cursor position within the line buffer)
//
int Readline::GetCursor()
{
    return rl_point;
}

// --------------------------------------------------------------------------
//
//! return strlen(rl_display_prompt) + rl_point
//
int Readline::GetAbsCursor()
{
    return strlen(rl_display_prompt) + rl_point;
}

// --------------------------------------------------------------------------
//
//! return rl_end (the current total length of the line buffer)
//! Note that after readline has returned the contents of rl_end might
//! not reflect the correct buffer length anymore.
//
int Readline::GetBufferLength()
{
    return rl_end;
}

// --------------------------------------------------------------------------
//
//! return the length of the prompt plus the length of the line buffer
//
int Readline::GetLineLength()
{
    return strlen(rl_display_prompt) + rl_end;
}

// --------------------------------------------------------------------------
//
//! Calls: Function: void rl_resize_terminal()
//! Update Readline's internal screen size by reading values from the kernel.
//
void Readline::Resize()
{
    rl_resize_terminal();
}

// --------------------------------------------------------------------------
//
//! Calls: Function: void rl_set_screen_size (int rows, int cols)
//! Set Readline's idea of the terminal size to rows rows and cols columns.
//!
//! @param width
//!    Number of columns
//!
//! @param height
//!    Number of rows
//
void Readline::Resize(int width, int height)
{
    rl_set_screen_size(height, width);
}

// --------------------------------------------------------------------------
//
//! Get the number of cols readline assumes the screen size to be
//
int Readline::GetCols() const
{
    int rows, cols;
    rl_get_screen_size(&rows, &cols);
    return cols;
}

// --------------------------------------------------------------------------
//
//! Get the number of rows readline assumes the screen size to be
//
int Readline::GetRows() const
{
    int rows, cols;
    rl_get_screen_size(&rows, &cols);
    return rows;
}

// --------------------------------------------------------------------------
//
//! Return a list of pointer to the history contents
//
vector<const char*> Readline::GetHistory() const
{
    HIST_ENTRY **next = history_list();

    vector<const char*> v;

    for (; *next; next++)
        v.push_back((*next)->line);

    return v;
}

// --------------------------------------------------------------------------
//
//! Clear readline history (calls clear_history())
//!
//! @returns
//!     always true
//
bool Readline::ClearHistory()
{
    clear_history();
    return true;
}

// --------------------------------------------------------------------------
//
//! Displays the current history on rl_outstream
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
bool Readline::DumpHistory()
{
    HIST_ENTRY **next = history_list();

    if (!next)
        return true;

    for (; *next; next++)
        fprintf(rl_outstream, "%s\n", (*next)->line);

    return true;
}

// --------------------------------------------------------------------------
//
//! Execute a shell command through a pipe. Write stdout to rl_outstream
//!
//! @param cmd
//!     Command to be executed
//!
//! @returns
//!     always true
//
bool Readline::ExecuteShellCommand(const string &cmd)
{
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        fprintf(rl_outstream, "ERROR - Could not create pipe '%s': %m\n", cmd.c_str());
        return true;
    }

    while (1)
    {
        char buf[1024];

        const size_t sz = fread(buf, 1, 1024, pipe);

        fwrite(buf, 1, sz, rl_outstream);

        if (feof(pipe) || ferror(pipe))
            break;
    }

    if (ferror(pipe))
        fprintf(rl_outstream, "ERROR - Reading from pipe '%s': %m\n", cmd.c_str());

    pclose(pipe);

    fprintf(rl_outstream, "\n");

    return true;
}

// --------------------------------------------------------------------------
//
//! Print the available commands. This is intended for being overwritten
//! by deriving classes.
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
//
bool Readline::PrintCommands()
{
    fprintf(rl_outstream, "\n");
    fprintf(rl_outstream, " Commands:\n");
    fprintf(rl_outstream, "   No application specific commands defined.\n");
    fprintf(rl_outstream, "\n");
    return true;
}

// --------------------------------------------------------------------------
//
//! Print a general help message. This is intended for being overwritten
//! by deriving classes.
//!
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
//
bool Readline::PrintGeneralHelp()
{
    fprintf(rl_outstream, "\n");
    fprintf(rl_outstream, " General help:\n");
    fprintf(rl_outstream, "   h,help       Print this help message\n");
    fprintf(rl_outstream, "   clear        Clear history buffer\n");
    fprintf(rl_outstream, "   lh,history   Dump the history buffer to the screen\n");
    fprintf(rl_outstream, "   v,variables  Dump readline variables\n");
    fprintf(rl_outstream, "   f,functions  Dump readline functions\n");
    fprintf(rl_outstream, "   m,funmap     Dump readline funmap\n");
    fprintf(rl_outstream, "   c,commands   Dump available commands\n");
    fprintf(rl_outstream, "   k,keylist    Dump key bindings\n");
    fprintf(rl_outstream, "   .! command   Execute a shell command\n");
    fprintf(rl_outstream, "   .w n         Sleep n milliseconds\n");
    fprintf(rl_outstream, "   .x file ..   Execute a script of commands (+optional argumnets)\n");
    fprintf(rl_outstream, "   .x file:N .. Execute a script of commands, start at label N\n");
    fprintf(rl_outstream, "   .j N         Forward jump to label N\n");
    fprintf(rl_outstream, "   .lt f0 f1 N  If float f0 lower than float f1, jump to label N\n");
    fprintf(rl_outstream, "   .gt f0 f1 N  If float f0 greater than float f1, jump to label N\n");
    fprintf(rl_outstream, "   .eq i0 i1 N  If int i0 equal int i1, jump to label N\n");
    fprintf(rl_outstream, "   : N          Defines a label (N=number)\n");
    fprintf(rl_outstream, "   # comment    Ignored\n");
    fprintf(rl_outstream, "   .q,quit      Quit\n");
    fprintf(rl_outstream, "\n");
    fprintf(rl_outstream, " The command history is automatically loaded and saves to\n");
    fprintf(rl_outstream, " and from %s.\n", GetName().c_str());
    fprintf(rl_outstream, "\n");
    return true;
}

// --------------------------------------------------------------------------
//
//! Print a help text about key bindings. This is intended for being
//! overwritten by deriving classes.
//!
//!
//! rl_outstream can be redirected using SetStreamOut()
//!
//! @returns
//!     always true
//
//
bool Readline::PrintKeyBindings()
{
    fprintf(rl_outstream, "\n");
    fprintf(rl_outstream, " Key bindings:\n");
    fprintf(rl_outstream, "   Page-up         Search backward in history\n");
    fprintf(rl_outstream, "   Page-dn         Search forward in history\n");
    fprintf(rl_outstream, "   Ctrl-left       One word backward\n");
    fprintf(rl_outstream, "   Ctrl-right      One word forward\n");
    fprintf(rl_outstream, "   Home            Beginning of line\n");
    fprintf(rl_outstream, "   End             End of line\n");
    fprintf(rl_outstream, "   Ctrl-d          Quit\n");
    fprintf(rl_outstream, "   Ctrl-y          Delete line\n");
    fprintf(rl_outstream, "   Alt-end/Ctrl-k  Delete until the end of the line\n");
    fprintf(rl_outstream, "   F1              Toggle visibility of upper panel\n");
    fprintf(rl_outstream, "\n");
    fprintf(rl_outstream, " Default key-bindings are identical with your bash.\n");
    fprintf(rl_outstream, "\n");
    return true;
}

// --------------------------------------------------------------------------
//
//!
//
bool Readline::PreProcess(const string &str)
{
    // ----------- Labels -------------

    if (str[0]==':')
    {
        try
        {
            fSection = stoi(str.substr(1));
            SetSection(fSection);

            if (fLabel!=fSection)
                return true;
        }
        catch (const logic_error &e)
        {
            fCommandLog << "# ERROR[" << fScriptDepth << "] - Inavlid label '" << str.substr(1) << "'" << endl;
            fLabel = -2;
            return true;
        }

        fLabel=-1;
        return false;
    }

    if (fLabel>=0)
    {
        fCommandLog << "# SKIP[" << fScriptDepth << "]: " << fLabel << " - " << str << endl;
        return true;
    }

    if (str.substr(0, 3)==".j ")
    {
        fLabel = atoi(str.substr(3).c_str());
        return false;
    }

    return Process(str);

}

// --------------------------------------------------------------------------
//
//!
//
bool Readline::Process(const string &str)
{
    // ----------- Common commands -------------

    if (str.substr(0, 3)==".w ")
    {
         usleep(stoi(str.substr(3))*1000);
         return true;
    }

    if (str.substr(0, 3)==".x ")
    {
        string opt(str.substr(3));

        map<string,string> data = Tools::Split(opt);
        if (opt.size()==0)
        {
            if (data.size()==0)
                PrintReadlineError("Filename missing.");
            else
                PrintReadlineError("Equal sign missing in argument '"+data.begin()->first+"'");

            return true;
        }

        const string save  = fScript;
        const int save_sec = fSection;
        Execute(opt, data);
        fScript = save;
        if (save_sec!=-4)
        {
            fSection = save_sec;
            SetSection(save_sec);
        }

        return true;
    }

    if (str.substr(0, 2)==".!")
    {
         ExecuteShellCommand(str.substr(2));
         return true;
    }

    if (str.substr(0, 4)==".gt ")
    {
        istringstream in(str.substr(4));

        float v0, v1;
        int label;

        in >> v0 >> v1 >> label;
        if (in.fail())
        {
            PrintReadlineError("Couldn't parse '"+str+"'");
            fLabel = -2;
            return true;
        }

        if (v0 > v1)
            fLabel = label;

        return true;
    }

    if (str.substr(0, 4)==".lt ")
    {
        istringstream in(str.substr(4));

        float v0, v1;
        int label;

        in >> v0 >> v1 >> label;
        if (in.fail())
        {
            PrintReadlineError("Couldn't parse '"+str+"'");
            fLabel = -2;
            return true;
        }

        if (v0 < v1)
           fLabel = label;

        return true;
    }

    if (str.substr(0, 4)==".eq ")
    {
        istringstream in(str.substr(4));

        int v0, v1, label;

        in >> v0 >> v1 >> label;
        if (in.fail())
        {
            PrintReadlineError("Couldn't parse '"+str+"'");
            fLabel = -2;
            return true;
        }

        if (v0==v1)
            fLabel = label;

        return true;
    }


    // ----------- Readline static -------------

    if (str=="clear")
        return ClearHistory();

    if (str=="lh" || str=="history")
        return DumpHistory();

    if (str=="v" || str=="variables")
        return DumpVariables();

    if (str=="f" || str=="functions")
        return DumpFunctions();

    if (str=="m" || str=="funmap")
        return DumpFunmap();

    // ---------- Readline virtual -------------

    if (str=="h" || str=="help")
        return PrintGeneralHelp();

    if (str=="c" || str=="commands")
        return PrintCommands();

    if (str=="k" || str=="keylist")
        return PrintKeyBindings();

    return false;
}

// --------------------------------------------------------------------------
//
//! This function is a wrapper around the call to readline. It encapsultes
//! the return buffer into a std::string and deletes the memory allocated
//! by readline. Furthermore, it removes leading and trailing whitespaces
//! before return the result. The result is returned in the given
//! argument containing the prompt. Before the function returns Shutdown()
//! is called (as opposed to Startup when readline starts)
//!
//! @param str
//!    The prompt which is to be shown by the readline libarary. it is
//!    directly given to the call to readline. The result of the
//!    readline call is returned in this string.
//!
//! @returns
//!    true if the call succeeded as usual, false if EOF was detected
//!    by the readline call.
//
bool Readline::PromptEOF(string &str)
{
    char *buf = readline(str.c_str());
    Shutdown(buf);

    // Happens when EOF is encountered
    if (!buf)
        return false;

    str = Tools::Trim(buf);

    free(buf);

    return true;
}

// --------------------------------------------------------------------------
//
//! This function is a wrapper around the call to readline. It encapsultes
//! the return buffer into a std::string and deletes the memory allocated
//! by readline. Furthermore, it removes leading and trailing whitespaces
//! before return the result. Before the function returns Shutdown() is
//! called (as opposed to Startup when readline starts)
//!
//! @param prompt
//!    The prompt which is to be shown by the readline libarary. it is
//!    directly given to the call to readline.
//!
//! @returns
//!    The result of the readline call
//
string Readline::Prompt(const string &prompt)
{
    char *buf = readline(prompt.c_str());

    Shutdown(buf ? buf : "");

    const string str = !buf || (rl_done && rl_pending_input==4)
        ? ".q" : Tools::Trim(buf);

    free(buf);

    return str;
}

// --------------------------------------------------------------------------
//
//! Writes the current history to the file defined by fName and
//! replaces the history by the data from the given file.
//!
//! @param fname
//!    Name of the history file to read
//!
void Readline::StaticPushHistory(const string &fname="")
{
    fs::path his = fs::path(This->fName).parent_path();
    his /= fname;

    write_history(This->fName.c_str());
    stifle_history(0);
    unstifle_history();
    read_history(his.string().c_str());
}

// --------------------------------------------------------------------------
//
//! Writes the current history to the file with the given name
//! and replaces the history by the file defined by fName.
//!
//! @param fname
//!    Name of the history file to write (it will be truncated to 1000 lines)
//!
void Readline::StaticPopHistory(const string &fname="")
{
    fs::path his = fs::path(This->fName).parent_path();
    his /= fname;

    write_history(his.string().c_str());
    history_truncate_file(his.string().c_str(), 1000);

    stifle_history(0);
    unstifle_history();
    read_history(This->fName.c_str());
}

// --------------------------------------------------------------------------
//
//! Just calls readline and thus allows to just prompt for something.
//! Adds everything to the history except '.q'
//!
//! @param prompt
//!    Prompt to be displayed
//!
//! @return
//!    String entered by the user ('.q' is Ctrl-d is pressed)
//!
string Readline::StaticPrompt(const string &prompt)
{
    char *buf = readline(prompt.c_str());
    if (!buf)
        return ".q";

    const string str(buf);
    if (Tools::Trim(str)!=".q" && !Tools::Trim(str).empty())
        if (history_length==0 || history_search_pos(str.c_str(), -1, history_length-1)!=history_length-1)
            add_history(buf);

    free(buf);

    return str;
}

// --------------------------------------------------------------------------
//
//! Process a single line. All lines are added to the history, but only
//! accepted lines are written to the command log. In this case fLine is
//! increased by one.
//! Comment (starting with #) are removed from the command-line. A # can be
//! escaped with quotation marks "#"
//
void Readline::ProcessLine(const string &str)
{
    try
    {
        const string cmd = Tools::Uncomment(str);

        if (!cmd.empty())
        {
            const bool rc = PreProcess(cmd);

            AddToHistory(cmd);

            if (rc)
                return;

            fLine++;
        }

        fCommandLog << str << endl;
    }
    catch (const exception &e)
    {
        PrintReadlineError("Couldn't parse: "+str+" ["+string(e.what())+"]");
    }
}

// --------------------------------------------------------------------------
//
//! This implements a loop over readline calls. A prompt to issue can be
//! given. If it is NULL, the prompt is retrieved from GetUpdatePrompt().
//! It is updated regularly by means of calls to GetUpdatePrompt() from
//! EventHook(). If Prompt() returns with "quit" or ".q" the loop is
//! exited. If ".qqq" is entered exit(-1) is called. In case of ".qqqqqq"
//! abort(). Both ways to exit the program are not recommended. Empty
//! inputs are ignored. After that Process() with the returned string
//! is called. If Process returns true the input is not counted and not
//! added to the history, otherwise the line counter is increased
//! and the input is added to the history.
//!
//! @param prompt
//!    The prompt to be issued or NULL if GetUPdatePrompt should be used
//!    instead.
//!
void Readline::Run(const char *prompt)
{
    fLine = 0;
    while (1)
    {
        // Before we start we have to make sure that the
        // screen looks like and is ordered like expected.
        const string str = Prompt(prompt?prompt:GetUpdatePrompt());
        if (str.empty())
            continue;

        if (str=="quit" || str==".q")
            break;

        if (str==".qqq")
            exit(128);

        if (str==".qqqqqq")
            abort();

        ProcessLine(str);
    }
}

// --------------------------------------------------------------------------
//
//! Executes commands read from an ascii file as they were typed in
//! the console. Empty lines and lines beginning with # are ignored.
//!
//! @param fname
//!    Filename of file to read
//!
//! @param args
//!    Arguments to be passed to the script. A search and replace
//!    will be done for ${arg}
//!
//! @returns
//!    -1 if the file couldn't be read and the number of commands for which
//!    Process() was callled otherwise
//!
int Readline::Execute(const string &fname, const map<string,string> &args)
{
    // this could do the same:
    //    rl_instream = fopen(str.c_str(), "r");

    if (IsStopped())
        return 0;

    string name = Tools::Trim(fname);
    fScript = name;

    fSection = -3;
    SetSection(-3);
    fLabel = -1;

    const size_t p = name.find_last_of(':');
    if (p!=string::npos)
    {
        fLabel = atoi(name.substr(p+1).c_str());
        name = name.substr(0, p);
    }

    ifstream fin(name.c_str());
    if (!fin)
    {
        fSection = -4;
        SetSection(-4);
        return -1;
    }

    if (fScriptDepth++==0)
        fStopScript = false;

    fCommandLog << "# " << Time() << " - " << name << " (START[" << fScriptDepth<< "]";
    if (fLabel>=0)
        fCommandLog << ':' << fLabel;
    fCommandLog << ")" << endl;

    fSection = -1;
    SetSection(-1);

    int rc = 0;

    string buffer;
    while (getline(fin, buffer, '\n') && !fStopScript)
    {
        buffer = Tools::Trim(buffer);
        if (buffer.empty())
            continue;

        rc++;

        if (buffer=="quit" || buffer==".q")
        {
            Stop();
            break;
        }

        // find and replace arguments
        for (auto it=args.begin(); it!=args.end(); it++)
        {
            const string find = "${"+it->first+"}";
            for (size_t pos=0; (pos=buffer.find(find, pos))!=string::npos; pos+=find.length())
                buffer.replace(pos, find.size(), it->second);
        }

        // process line
        ProcessLine(buffer);
    }

    fCommandLog << "# " << Time() << " - " << name << " (FINISHED[" << fScriptDepth<< "])" << endl;

    if (--fScriptDepth==0)
        fStopScript = false;

    fLabel = -1;
    fSection = -4;
    SetSection(-4);

    return rc;
}

// --------------------------------------------------------------------------
//
//! Stops the readline execution. It fakes the end of an editing by
//! setting rl_done to 1. To distinguish this from a normal edit,
//! rl_pending_input is set to EOT.
//!
void Readline::Stop()
{
    rl_done          = 1;
    rl_pending_input = 4; // EOT (end of transmission, ctrl-d)
}

// --------------------------------------------------------------------------
//
//! @returns
//!    the status of rl_done and rl_pending_input. If rl_done==1 and
//!    rl_pending_input==4 true is returned, false otherwise.
//!    This can be used to check if Stop() was called. If readline is
//!    not in operation.
//!
bool Readline::IsStopped() const
{
    return rl_done==1 && rl_pending_input==4;
};

void Readline::PrintReadlineError(const std::string &str)
{
    fprintf(rl_outstream, "%s\n", str.c_str());
}
