#ifndef FACT_WindowLog
#define FACT_WindowLog

#include <map>
#include <mutex>
#include <vector>
#include <fstream>
#include <functional> // std::bind

#include <ncurses.h> // A_NORMAL etc

#include "../externals/Queue.h"

#include "WindowLogColor.h"

/// Stream manipulators to change the attributes of a WindowLog stream
enum WindowLogAttrs
{
    kReset      = -1,            ///< Reset all attributes
    kNormal     = A_NORMAL,      ///< Set attribute Normal
    kHighlight  = A_STANDOUT,    ///< Set attribute Highlight
    kUnderline  = A_UNDERLINE,   ///< Set attribute Underline
    kReverse    = A_REVERSE,     ///< Set attribute Reverse
    kBlink      = A_BLINK,       ///< Set attribute Blink
    kDim        = A_DIM,         ///< Set attribute Dim
    kBold       = A_BOLD,        ///< Set attribute Bold
    kProtect    = A_PROTECT,     ///< Set attribute Protect
    kInvisible  = A_INVIS,       ///< Set attribute Invisible
    kAltCharset = A_ALTCHARSET,  ///< Set attribute Alternative charset
};
/*
enum WindowLogManip
{
    kLogOn   = 1,
    kLogOff  = 2,
    kNullOn  = 3,
    kNullOff = 4,
};
*/
class WindowLog : public std::streambuf, public std::ostream
{
    friend std::ostream &operator<<(std::ostream &lout, WindowLogColor m);
    friend std::ostream &operator<<(std::ostream &lout, WindowLogAttrs m);
    //friend std::ostream &operator<<(std::ostream &lout, WindowLogManip m);
private:
    static const int fgBufferSize = 160;

    char        fBuffer;               ///
    char        fBase[fgBufferSize+1]; /// Buffer to store the data in
    char       *fPPtr;                 /// Pointer to present position in buffer
    const char *fEPtr;                 /// Pointer to end of buffer

    WINDOW     *fWindow;               /// Pointer to an ncurses Window

    std::vector<char>  fBacklog;       /// Backlog storage
    std::map<int, int> fAttributes;    /// Storage for attributes (backlog)

    std::ofstream fLogFile;    /// Stream for redirection to a log-file

    bool fIsNull;              /// Switch to toggle off physical output to the screen
    bool fEnableBacklog;       /// Switch to toggle storage in the backlog on or off

    std::mutex fMuxBacklog;    /// Mutex securing backlog access
    std::mutex fMuxFile;       /// Mutex securing file access
    std::mutex fMuxCout;       /// Mutex securing output to cout
    std::mutex fMuxWindow;     /// Mutex securing output to fWindow

    Queue<std::string> fQueueFile;

    static std::string GetAnsiAttr(int m);

    void AddAttr(int m);
    void AddColor(int m);

    bool WriteFile(const std::string &);
    void WriteBuffer();

    int sync();
    int overflow(int i); // i=EOF means not a real overflow

public:
    // --------------------------------------------------------------------------
    //
    //! Default constructor which initializes the streamer and sets the device
    //! which is used for the output
    //!
    //! Switch on backlog
    //! Switch on screen output
    //
    WindowLog() : std::ostream(this), fPPtr(fBase), fEPtr(fBase+fgBufferSize), fWindow(0), fIsNull(false), fEnableBacklog(true),
        fQueueFile(std::bind(&WindowLog::WriteFile, this, std::placeholders::_1))
    {
        //fLogFile.rdbuf()->pubsetbuf(0,0); // Switch off buffering
        setp(&fBuffer, &fBuffer+1);
        *this << '\0';
    }
    WindowLog(WindowLog const& log) : std::ios(), std::streambuf(), std::ostream((std::streambuf*)&log), fWindow(log.fWindow), fIsNull(false), fEnableBacklog(true),
        fQueueFile(std::bind(&WindowLog::WriteFile, this, std::placeholders::_1))
    {
        //fLogFile.rdbuf()->pubsetbuf(0,0); // Switch off buffering
    }
    ~WindowLog()
    {
        fQueueFile.wait(false);
    }

    /// Redirect the output to an ncurses WINDOW instead of cout
    void SetWindow(WINDOW *w) { fWindow=w; }

    /// Open a log-file
    bool OpenLogFile(const std::string &filename, bool append=false);

    /// Close a log-file
    void CloseLogFile();

    /// Display backlog
    void Display(bool empty=false);

    /// Empty backlog
    void EmptyBacklog();

    /// Get the current size of the backlog in bytes
    size_t GetSizeBacklog() const { return fBacklog.size(); }
    std::string GetSizeStr() const;

    /// Switch on or off any physical output to the screen (cout or fWindow)
    void SetNullOutput(bool n=true) { fIsNull=n; }
    bool GetNullOutput() const { return fIsNull; }

    /// Switch on or off any storage in the backlog
    void SetBacklog(bool n=true) { fEnableBacklog=n; }
    bool GetBacklog() const { return fEnableBacklog; }
};

std::ostream &operator<<(std::ostream &lout, WindowLogColor m);
std::ostream &operator<<(std::ostream &lout, WindowLogAttrs m);

#endif
