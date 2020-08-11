#ifndef FACT_DimWriteStatistics
#define FACT_DimWriteStatistics

#include <set>
#include <string>

// Keep these two together! Otheriwse it won't compile
#include <boost/bind.hpp>
#if BOOST_VERSION < 104400
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#undef BOOST_HAS_RVALUE_REFS
#endif
#endif
#include <boost/thread.hpp>

#include "MessageImp.h"
#include "DimDescriptionService.h"

class DimWriteStatistics
{
public:
    struct Stats
    {
        uint64_t freeSpace;
        uint64_t sizeWritten;
        uint64_t rateWritten;
        uint64_t timeElapsed;

        Stats() : freeSpace(0), sizeWritten(0), rateWritten(0), timeElapsed(0) { }
    };

private:
    MessageImp &fLog;

    DimDescribedService   fDimService;

    std::string           fCurrentFolder;   /// Current folder being watched for free space
    uint16_t              fUpdateInterval;  /// Duration, in millisecond between two service updates. 0 means no more updates
    size_t                fBaseSize;        /// Total base size of all opened files
    std::set<std::string> fOpenedFiles;     /// List of all opened files. set is used to easily check for entries

    /// Bool indicating if debug information should be printed
    bool fDebug;

    /// The data structure holding the stat data
    Stats fStats;

    /// The boost thread used to update the service
    boost::thread fThread;                  

    ///Main loop
    void UpdateService();

    ///Returns the free space on the disk of the folder being watched (fCurrentFolder)
    int64_t GetFreeSpace();

    ///Returns the size on disk of a given file
    int64_t GetFileSizeOnDisk(const std::string& file) { return GetFileSizeOnDisk(file, fLog); }

    int Write(const Time &t, const std::string &txt, int qos);

public:
    ///Constructor
    DimWriteStatistics(const std::string& serverName, MessageImp &log);

    ///Default destructor
    ~DimWriteStatistics();

    ///Configures that current folder where files are written to
    bool SetCurrentFolder(const std::string& folder);

    bool FileOpened(const std::string& fileName);

    void SetDebugMode(bool);
    void SetUpdateInterval(const int16_t millisec);

    const Stats &GetTotalSizeWritten() const { return fStats; }
    uint16_t GetUpdateInterval() const { return fUpdateInterval; }

    ///Returns the size on disk of a given file
    static int64_t GetFileSizeOnDisk(const std::string& file, MessageImp &imp);

    static bool DoesPathExist(std::string path, MessageImp &log);
};

#endif
