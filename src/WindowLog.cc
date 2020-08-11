// **************************************************************************
/** @class WindowLog

@brief A C++ ostream to an ncurses window supporting attributes and colors

@section References

 - <A HREF="http://www.gnu.org/software/ncurses">GNU Ncurses</A>

@todo
   improve docu


**/
// **************************************************************************
#include "WindowLog.h"

#include <sstream>
#include <iostream>
#include <algorithm>

#include <curses.h>

#include "tools.h"

using namespace std;

// --------------------------------------------------------------------------
//
//! Delete the contents of the backlog
//
void WindowLog::EmptyBacklog()
{
    fMuxBacklog.lock();
    fBacklog.clear();
    fMuxBacklog.unlock();
}

// --------------------------------------------------------------------------
//
//! Display the backlog. If fWindow is NULL then it is flushed to cout
//! otherwise to the window (including the colors and attributes)
//!
//! Access to cout, the backlog and the window is encapsulated in mutices.
//
void WindowLog::Display(bool empty)
{
    if (!fWindow)
    {
        fMuxBacklog.lock();
        fMuxCout.lock();

        cout.write(fBacklog.data(), fBacklog.size());
        cout.flush();

        if (empty)
            fBacklog.clear();

        fMuxCout.unlock();
        fMuxBacklog.unlock();
        return;
    }

    const int w = getmaxx(fWindow);

    fMuxBacklog.lock();
    fMuxWindow.lock();
    //vector<char>::iterator p0 = fBacklog.begin();

    int lines = 0;
    int x     = 0;

    for (unsigned int i=0; i<fBacklog.size(); i++)
    {
        if (fAttributes.find(i)!=fAttributes.end())
            fAttributes[i]==-1 ? wattrset(fWindow, 0) : wattron(fWindow, fAttributes[i]);

        if (fBacklog[i]=='\n')
        {
            // The attribute is added to the backlog in WriteBuffer
            //wattrset(fWindow, 0);
            lines += x/w + 1;
            x=0;
        }
        wprintw(fWindow, "%c", fBacklog[i]);
        x++;
    }

    if (empty)
        fBacklog.clear();

    fMuxWindow.unlock();
    fMuxBacklog.unlock();

    lines += x/w;
}

// --------------------------------------------------------------------------
//
//! Open a log-file into which any of the following output is written. If a
//! log-file is alraedy open it is closed.
//! before the new file is opened or an old one closed the current buffer
//! is flushed.
//!
//! @param filename
//!    The filename of the file to open
//!
//! @returns
//!    Whether the log-file stream is open or not
//
bool WindowLog::OpenLogFile(const string &filename, bool append)
{
    fMuxFile.lock();
    flush();

    if (fLogFile.is_open())
        fLogFile.close();

    fLogFile.open(filename, append ? ios::app|ios::out : ios::out);
    if (append)
        fLogFile << '\n';

    fMuxFile.unlock();

    return fLogFile.is_open();
}

// --------------------------------------------------------------------------
//
//! Close an open log-file
//
void WindowLog::CloseLogFile()
{
    fMuxFile.lock();
    fLogFile.close();
    fMuxFile.unlock();
}

bool WindowLog::WriteFile(const string &sout)
{
    fMuxFile.lock();
    fLogFile << sout;
    fLogFile.flush();
    fMuxFile.unlock();

    return true;
}

// --------------------------------------------------------------------------
//
//! This is the function which writes the stream physically to a device.
//! If you want to add a new device this must be done here.
//!
//! If the backlog is enabled, the contents are put into the backlog.
//! if fWindow is NULL the contents are flushed to cout, otherwise
//! to the window defined by fWindow.
//!
//! In addition the contents are flushed to the log-file if open.
//! If fIsNull is true any output on the screen (cout or fWindow) is
//! suppressed.
//!
//! @todo
//!    Truncate the backlog
//
void WindowLog::WriteBuffer()
{
    // Store the number of characters in the buffer which should be flushed
    const int len = fPPtr - fBase;

    // restart writing to the buffer at its first char
    fPPtr = fBase;

    // If the is nothing to output, we are done
    if (len<=0)
        return;

    // FIXME: Truncate backlog!

    // If fWindow is set, output everything to the window, otherwise
    // to cout
    if (!fIsNull)
    {
        if (fWindow)
        {
            fMuxWindow.lock();
            if (!fIsNull)
            {
                const string sout = string(fBase, len);
                wprintw(fWindow, "%s", sout.c_str());
            }
            // If the stream got flushed due to a line break
            // reset all attributes
            if (fBase[len-1]=='\n')
                wattrset(fWindow, 0);
            fMuxWindow.unlock();
        }
        else
        {
            fMuxCout.lock();
            cout.write(fBase, len);// << sout;
            // If the stream got flushed due to a line break
            // reset all attributes
            if (fBase[len-1]=='\n')
                cout << "\033[0m";
            cout.flush();
            fMuxCout.unlock();
        }
    }

    // Add the buffer to the backlog
    if (fEnableBacklog)
    {
        fMuxBacklog.lock();
        fBacklog.insert(fBacklog.end(), fBase, fBase+len);

        // If the stream got flushed due to a line break
        // add the reset of all attributes to the backlog
        if (fBase[len-1]=='\n')
        {
            if (!fWindow)
            {
                const char *reset = "\033[0m";
                fBacklog.insert(fBacklog.end(), reset, reset+4);

            }
            else
                fAttributes[fBacklog.size()] = -1;
        }
        fMuxBacklog.unlock();
    }

    fQueueFile.emplace(fBase, len);
    /*
    // Output everything also to the log-file
    fMuxFile.lock();
    fLogFile << sout;
    //fLogFile.flush();
    fMuxFile.unlock();
    */
    // If we are flushing because of an EOL, we reset also all attributes
}

// --------------------------------------------------------------------------
//
//! This is called to flush the buffer of the streaming devices
//
int WindowLog::sync()
{
    WriteBuffer();
    return 0;
}

// --------------------------------------------------------------------------
//
//! This function comes from streambuf and should output the buffer to
//! the device (flush, endl) or handle a buffer overflow (too many chars)
//! If a real overflow happens i contains the next chars which doesn't
//! fit into the buffer anymore.If the buffer is not really filled,
//! i is EOF(-1).
//
int WindowLog::overflow(int i) // i=EOF means not a real overflow
{
    *fPPtr++ = (char)i;

    if (fPPtr == fEPtr)
        WriteBuffer();

    return 0;
}

// --------------------------------------------------------------------------
//
//! Returns the size of the backlog buffer as string.
//!
//! @returns
//!     Size of the backlog as a string, e.g. "1k"
//
string WindowLog::GetSizeStr() const
{
    int s = GetSizeBacklog()/1000;
    if (s==0)
        return "0";

    char u = 'k';
    if (s>999)
    {
        s/=1000;
        u = 'M';
    }

    ostringstream str;
    str << s << u;
    return str.str();
}

// --------------------------------------------------------------------------
//
//! @returns
//!     the ANSI code corresponding to the attributes
//
string WindowLog::GetAnsiAttr(int m)
{
    if (m==kReset || m==kDefault)
        return "\033[0m";

    string rc;

    if ((m&COLOR_PAIR(kRed)    )==COLOR_PAIR(kRed)    )  rc += "\033[31m";
    if ((m&COLOR_PAIR(kGreen)  )==COLOR_PAIR(kGreen)  )  rc += "\033[32m";
    if ((m&COLOR_PAIR(kYellow) )==COLOR_PAIR(kYellow) )  rc += "\033[33m";
    if ((m&COLOR_PAIR(kBlue)   )==COLOR_PAIR(kBlue)   )  rc += "\033[34m";
    if ((m&COLOR_PAIR(kMagenta))==COLOR_PAIR(kMagenta))  rc += "\033[35m";
    if ((m&COLOR_PAIR(kCyan)   )==COLOR_PAIR(kCyan)   )  rc += "\033[36m";
    if ((m&COLOR_PAIR(kWhite)  )==COLOR_PAIR(kWhite)  )  rc += "\033[0m\033[1m";

    if ((m&kBold     )==kBold     )  rc += "\033[1m";
    if ((m&kDim      )==kDim      )  rc += "\033[2m";
    if ((m&kUnderline)==kUnderline)  rc += "\033[4m";
    if ((m&kBlink    )==kBlink    )  rc += "\033[5m";

    return rc;
}

// --------------------------------------------------------------------------
//
//! Add color to the stream according to the attribute. If fWindow is not
//! set this is an ANSI color code, otherwise the window's output
//! attributes are set.
//! It is also added to the backlog. Access to the backlog is encapsulated
//! into its mutex.
//
void WindowLog::AddColor(int m)
{
    const int col = COLOR_PAIR(m);

    if (!fWindow)
        // We don't have to flush here, because the attributes are simply
        // part of the stream
        *this << GetAnsiAttr(col);
    else
    {
        // Before we change the attributes we have to flush the screen
        // otherwise we would have to buffer them until we flush the
        // contents
        flush();
        wattron(fWindow, col);
    }

    fMuxBacklog.lock();
    fAttributes[fBacklog.size()] |= col;
    fMuxBacklog.unlock();
}

// --------------------------------------------------------------------------
//
//! Add attributes to the stream according to the attribute. If fWindow is
//! not set this is an ANSI code, otherwise the window's output
//! attributes are set.
//! It is also added to the backlog. Access to the backlog is encapsulated
//! into its mutex.
//
void WindowLog::AddAttr(int m)
{
    if (!fWindow)
        // We don't have to flush here, because the attributes are simply
        // part of the stream
        *this << GetAnsiAttr(m);
    else
    {
        // Before we change the attributes we have to flush the screen
        // otherwise we would have to buffer them until we flush the
        // contents
        flush();
        m==kReset ? wattrset(fWindow, 0) : wattron(fWindow, m);
    }

    fMuxBacklog.lock();
    m==kReset ?
        fAttributes[fBacklog.size()] = -1 :
        fAttributes[fBacklog.size()] |= m;
    fMuxBacklog.unlock();
}

// --------------------------------------------------------------------------
//
//!
//
std::ostream &operator<<(std::ostream &lout, WindowLogColor m)
{
    WindowLog *log=dynamic_cast<WindowLog*>(lout.rdbuf());
    if (log)
        log->AddColor(m);
    return lout;
}

// --------------------------------------------------------------------------
//
//!
//
std::ostream &operator<<(std::ostream &lout, WindowLogAttrs m)
{
    WindowLog *log=dynamic_cast<WindowLog*>(lout.rdbuf());
    if (log)
        log->AddAttr(m);
    return lout;
}
