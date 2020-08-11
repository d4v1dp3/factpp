//*************************************************************************************
/** @class DimWriteStatistics

 @brief provides a statistics service telling the free space on disk and the total size written so far

*/
//*************************************************************************************
#include "DimWriteStatistics.h"

#include <sys/statvfs.h> //for getting disk free space
#include <sys/stat.h>    //for getting files sizes

#include <boost/filesystem.hpp>

#include "Time.h"

using namespace std;
using namespace boost::posix_time;

// --------------------------------------------------------------------------
//
//! Constructor with correct service name. The state machine using this object should give it
//! its own name as a parameter
//! @param serverName the name of the server which created this object
//
DimWriteStatistics::DimWriteStatistics(const string& server, MessageImp &log) :
    fLog(log),
    fDimService(server + "/STATS",  "X:1;X:1;X:1;X:1",
                "Statistics about size written"
                "|FreeSpace[bytes]:Free space on disk"
                "|Written[bytes]:Bytes written in total"
                "|Rate[bytes]:Bytes written since last update"
                "|Elapsed[ms]:Milliseconds elapsed since last update"),
    fCurrentFolder("."),
    fUpdateInterval(1000),
    fBaseSize(0),
    fDebug(false)
{
    fThread = boost::thread(boost::bind(&DimWriteStatistics::UpdateService, this));
}

// --------------------------------------------------------------------------
//
//! Destructor. Stop thread by setting fUpdateInterval to 0 and join the
//! thread.
//
DimWriteStatistics::~DimWriteStatistics()
{
    fUpdateInterval = 0;

    // This blocks for fPeriod duration, but maybe canceling the thread
    // could be more dangerous leaving Dim in an undefined state.
    fThread.interrupt();
}

int DimWriteStatistics::Write(const Time &t, const string &txt, int qos)
{
    return fLog.Write(t, txt, qos);
}

// --------------------------------------------------------------------------
//
//! Retrieves the free space of the current base path
//! @return the available free space on disk, in bytes
//
int64_t DimWriteStatistics::GetFreeSpace()
{
    struct statvfs vfs;
    if (statvfs(fCurrentFolder.c_str(), &vfs))
        return -1;

    return vfs.f_bsize*vfs.f_bavail;
}

// --------------------------------------------------------------------------
//
//! Retrieves the size on disk of a given file, in bytes
//! @param file the filename for which the size should be retrieved
//! @return the size of the file, in bytes
//
int64_t DimWriteStatistics::GetFileSizeOnDisk(const string& file, MessageImp &log)
{
     errno = 0;
     struct stat st;
     if (!stat(file.c_str(), &st))
         return st.st_size;

     //ignoring error #2: no such file or directory is not an error for new files
     if (errno == 0 || errno == 2)
         return 0;

     ostringstream str;
     str << "stat() failed for '" << file << "': " << strerror(errno) << " [errno=" << errno << "]";
     log.Error(str);

     return -1;
}

// --------------------------------------------------------------------------
//
//! Check if a given path exists
//! @param path the path to be checked
//! @return whether or not the given path exists
//
bool DimWriteStatistics::DoesPathExist(string path, MessageImp &log)
{
    namespace fs = boost::filesystem;

    if (path.empty())
        path = ".";

    const fs::path fullPath = fs::system_complete(fs::path(path));

    if (!fs::exists(fullPath))
       return false;

    if (!fs::is_directory(fullPath))
    {
        log.Error("Path given for checking '" + path + "' designate a file name. Please provide a path name only");
        return false;
    }

    if (access(path.c_str(), R_OK|W_OK|X_OK) != 0)
    {
        log.Error("Missing read, write or execute permissions on directory '" + path + "'");
        return false;
    }

    return true;
}

// --------------------------------------------------------------------------
//
//! Sets the current folder
//! @param folder the path to the folder
//
bool DimWriteStatistics::SetCurrentFolder(const string& folder)
{
    struct statvfs vfs;
    if (statvfs(folder.empty()?".":folder.c_str(), &vfs))
    {
        fLog.Error("statvfs() failed for '"+folder+"'... ignoring it.");
        return false;
    }

    fCurrentFolder = folder.empty()?".":folder;
    return true;
}

// --------------------------------------------------------------------------
//
//! Updates the service. This is the function executed by the thread
//
void DimWriteStatistics::UpdateService()
{
    Time previousTime;
    uint64_t previousSize = 0;

    while (1)
    {
        if (fUpdateInterval==0)
        {
            boost::this_thread::interruption_point();
            boost::this_thread::yield();
            continue;
        }

        Stats data;

        for (set<string>::const_iterator it = fOpenedFiles.begin(); it != fOpenedFiles.end(); it++)
            data.sizeWritten += GetFileSizeOnDisk(*it);
        data.sizeWritten -= fBaseSize;

        const Time cTime = Time();

        data.freeSpace   = GetFreeSpace();
        data.rateWritten = data.sizeWritten-previousSize;
        data.timeElapsed = (cTime - previousTime).total_milliseconds();

        previousSize = data.sizeWritten;
        previousTime = cTime;

        fDimService.setData(data);
        fDimService.Update(cTime);

        fStats = data;

        if (fDebug)
        {
            ostringstream str;
            str << "Written: " << fStats.sizeWritten/1000 << " kB; writing rate: ";
            str << fStats.rateWritten/fStats.timeElapsed << " kB/s; free space: ";
            str << fStats.freeSpace/1000000 << " MB";
            fLog.Debug(str);
        }

        boost::this_thread::sleep(milliseconds(fUpdateInterval));
    }
}
// --------------------------------------------------------------------------
//
//! Let the object know that a new file has been opened
//! @param fileName the full name of the file newly opened
//! @return whether this file could be stated or not
//
bool DimWriteStatistics::FileOpened(const string& fileName)
{
    if (fOpenedFiles.find(fileName) != fOpenedFiles.end())
        return false;

    //Add a newly opened file, and remember its original size
    const int64_t newSize = GetFileSizeOnDisk(fileName);
    if (newSize == -1)
        return false;

    fBaseSize += newSize;
    fOpenedFiles.insert(fileName);

    return true;
}
// --------------------------------------------------------------------------
//
//! Set the debug mode on and off
//! @param debug the new mode (true or false)
//
void DimWriteStatistics::SetDebugMode(bool debug)
{
    fDebug = debug;

    if (fDebug)
        fLog.Debug("Debug mode is now on.");
}
// --------------------------------------------------------------------------
//
//! Set the update of the service interval
//! @param duration the duration between two services update, in second
//
void DimWriteStatistics::SetUpdateInterval(const int16_t duration)
{
    if (!finite(duration))
    {
        fLog.Error("Provided update interval is not a valid float... discarding.");
        return;
    }
    if (uint16_t(duration) == fUpdateInterval)
    {
        fLog.Warn("Statistics update interval not modified. Supplied value already in use.");
        return;
    }

    if (duration <= 0)
        fLog.Message("Statistics are now OFF.");
    else
    {
        ostringstream str;
        str << "Statistics update interval is now " << duration << " seconds";
        fLog.Message(str);
    }

    fUpdateInterval = duration<0 ? 0 : duration;
}
