//****************************************************************
/** @class DataLogger 
  
  @brief Logs all message and infos between the services
  
  This is the main logging class facility. 
  It derives from StateMachineDim and DimInfoHandler. the first parent is here to enforce 
  a state machine behaviour, while the second one is meant to make the dataLogger receive
  dim services to which it subscribed from.
  The possible states and transitions of the machine are:
  \dot
  // FIXME FIXME: Error states missing...
  digraph datalogger
  { 
     node [shape=record, fontname=Helvetica, fontsize=10];
  
     srt  [label="Start" style="rounded"]
     rdy  [label="Ready"]
     nop  [label="NightlyOpen"]
     wait [label="WaitingRun"]
     log  [label="Logging"]

     //e    [label="Error" color="red"];
     //c    [label="BadFolder" color="red"]
     
     
     cmd_start  [label="START"              shape="none" height="0"]
     cmd_stop   [label="STOP"               shape="none" height="0"]
     cmd_stopr  [label="STOP_RUN_LOGGING"   shape="none" height="0"]
     cmd_startr [label="START_RUN_LOGGING"  shape="none" height="0"]
     
     { rank=same; cmd_startr cmd_stopr }
     { rank=same; cmd_start  cmd_stop  }
     
  
     srt  -> rdy  
       
     rdy -> cmd_start   [ arrowhead="open" dir="both" arrowtail="tee" weight=10 ]
     cmd_start -> nop   

     nop  -> cmd_stop   [ arrowhead="none" dir="both" arrowtail="inv"  ]
     wait -> cmd_stop   [ arrowhead="none" dir="both" arrowtail="inv"  ]
     log  -> cmd_stop   [ arrowhead="none" dir="both" arrowtail="inv"  ]
     cmd_stop -> rdy    

     wait -> cmd_stopr  [ arrowhead="none" dir="both" arrowtail="inv"  ]
     log  -> cmd_stopr  [ arrowhead="none" dir="both" arrowtail="inv"  ]
     cmd_stopr -> nop   

     nop -> cmd_startr  [ arrowhead="none" dir="both" arrowtail="inv" weight=10 ]
     rdy -> cmd_startr  [ arrowhead="none" dir="both" arrowtail="inv" ]
     cmd_startr -> wait [ weight=10 ]


     wait -> log
     log  -> wait
  }
  \enddot

  For questions or bug report, please contact Etienne Lyard (etienne.lyard@unige.ch) or Thomas Bretz.
 */
 //****************************************************************
#include <unistd.h>      //for getting stat of opened files
//#include <sys/statvfs.h> //for getting disk free space
//#include <sys/stat.h>    //for getting files sizes
#include <fstream>
#include <functional>

#include <boost/filesystem.hpp>

#include "Dim.h"
#include "Event.h"
#include "StateMachineDim.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Converter.h"
#include "DimWriteStatistics.h"

#include "Description.h"

#ifdef HAVE_FITS
#include "Fits.h"
#endif

#include "DimState.h"

#ifdef HAVE_NOVA
#include "externals/nova.h"
#endif

//Dim structures
///distributes the number of opened subscriptions and fits files
struct NumSubAndFitsType {
    uint32_t numSubscriptions;
    uint32_t numOpenFits;
};
///distributes which files were opened.
struct OpenFileToDim {
    uint32_t code;
    char fileName[FILENAME_MAX];
};

///Run number record. Used to keep track of which run numbers are still active
struct RunNumberType {

    ///the actual run number
    int32_t runNumber;
    ///the time at which the run number was received
    Time time;
    ///default constructor
    RunNumberType()
    {
        runNumber = 0;
    }
    ///default destructor
    ~RunNumberType()
    {

    }
};

EventImp nullEventImp;
///Dim subscription type. Stores all the relevant info to handle a Dim subscription
struct SubscriptionType
{
#ifdef HAVE_FITS
    ///Nightly FITS output file
    Fits    nightlyFile;
#endif
    ///the server
    string server;
    ///the service
    string service;
    ///the converter for outputting the data according to the format
    shared_ptr<Converter> fConv;
    ///the original format string. So that we can check if format is changing over time
    string format;
    ///the current run number used by this subscription
    int32_t runNumber;
    ///time of the latest received event
    Time lastReceivedEvent;
    ///whether or not the fits buffer was allocated already
    bool fitsBufferAllocated;
    ///the actual dimInfo pointer (must be the last in the list to ensure
    /// that it is the first which is deleted -- and consequently none of
    /// the other members can still be in use in an infoHandler)
    //DIM_REPLACE
    //shared_ptr<DimStampedInfo> dimInfo;
    unsigned int index;
    ///counter to know if format has changed during operations
    unsigned int increment;

    ///Dim info constructor
    //DIM_REPLACE
//    SubscriptionType(DimStampedInfo* info=NULL)
    SubscriptionType()
    {
        fConv = shared_ptr<Converter>();
        runNumber = 0;
        lastReceivedEvent = Time::None;
        fitsBufferAllocated = false;
        // Should be the last instantiated to make sure that all other
        // variables which might be used are already initialized
        //DIM_REPLACE
        //dimInfo = shared_ptr<DimStampedInfo>(info);
        index = 0;
        increment = 0;
    }
    ///default destructor
    ~SubscriptionType()
    {
    }
};

class DataLogger : public StateMachineDim
//DIM_REPLACE
//, DimServiceInfoListImp
{
public:
    /// The list of existing states specific to the DataLogger
    enum
    {
        kSM_NightlyOpen     = 20,    ///< Nightly file openned and writing
        kSM_WaitingRun      = 30,    ///< waiting for the run number to open the run file
        kSM_Logging         = 40,    ///< both files openned and writing
        kSM_BadFolder       = 0x101, ///< the folder specified for Nightly logging does not exist or has bad permissions
        kSM_RunWriteError   = 0x103, ///< Denotes that an error occured while writing a run file (text or fits).
        kSM_DailyWriteError = 0x103, ///< Denots that an error occured while writing a daily file (text or fits).
    } localstates_t;
    
    DataLogger(ostream &out);
    ~DataLogger(); 

    int EvalOptions(Configuration& conf);

private:
    /************************************************
     * MEMBER VARIABLES
     ************************************************/
    /// ofstream for the NightlyLogfile
    ofstream fNightlyLogFile;
    /// Log stream to fNightlyLogFile
    MessageImp fNightlyLogImp;
    /// ofstream for the Nightly report file
//    ofstream fNightlyReportFile;
    /// base path of files
    string fFilePath;
    ///run numbers
    list<RunNumberType> fRunNumber;
    ///old run numbers time-out delay (in seconds)
    uint32_t fRunNumberTimeout;
    ///previous run number. to check if changed while logging
    int fPreviousRunNumber;
    ///Current Service Quality
    int fQuality;
    ///Modified Julian Date
    double fMjD;
    ///for obtaining the name of the existing services
//    ServiceList fServiceList;
    typedef map<const string, map<string, SubscriptionType> > SubscriptionsListType;
    ///All the services to which we have subscribed to, sorted by server name.
    SubscriptionsListType fServiceSubscriptions;
    ///full name of the nightly log file
    string fFullNightlyLogFileName;
    ///full name of the nightly report file
    string fFullNightlyReportFileName;
    ///variable to track when the statistic were last calculated
//    Time fPreviousStatsUpdateTime;
    Time fPreviousOldRunNumberCheck;
    ///boolean to know whether we should close and reopen daily files or not
    bool fDailyFileDayChangedAlready;

    DimWriteStatistics fFilesStats;

    ///map and mutex for storing services description
    map<string, vector<Description> > fServiceDescriptionList;
    mutex fMutex;
    int HandleDescriptions(DimDescriptions* desc);
    vector<Description> GetDescription(const string& server, const string& service);
private:
    /***************************************************
     * DIM INFO HANDLER
     ***************************************************/
    //overloading of DIM's infoHandler function
    int infoCallback(const EventImp& evt, unsigned int infoIndex);

//    Time GetSunRise(const Time &time=Time());

    /***************************************************
     * TRANSITION FUNCTIONS
     ***************************************************/
    ///Reporting method for the services info received
    void Report(const EventImp& evt, SubscriptionType& sub);

    ///Configuration of the nightly file path
    int ConfigureFilePath(const Event& evt);
    ///print the current state of the dataLogger
    int PrintState(const Event& evt);
    ///checks whether or not the current info being treated is a run number
    void CheckForRunNumber(const EventImp& evt, unsigned int index);
    /// start transition
    int Start();
    ///from waiting to logging transition
    //int StartRun();
    // from logging to waiting transition
    int StopRunLogging();
    ///stop and reset transition
    int GoToReady();
    ///from NightlyOpen to waiting transition
    int NightlyToWaitRun();
    ///from wait for run number to nightly open
    int BackToNightlyOpen();
#ifdef HAVE_FITS
    ///Open fits files
    void OpenFITSFiles(SubscriptionType& sub);
    ///Write data to FITS files
    void WriteToFITS(SubscriptionType& sub, const void* data);
    ///Allocate the buffers required for fits
    void AllocateFITSBuffers(SubscriptionType& sub);
#endif//has_fits

    /***************************************
     * DIM SERVICES PROVIDED BY THE DATA LOGGER
     ***************************************/
    ///monitoring notification loop
    void ServicesMonitoring();
    inline void NotifyOpenedFile(const string &name, int type, DimDescribedService* service);
    ///Service for opened files
    DimDescribedService* fOpenedNightlyFiles;
    DimDescribedService* fOpenedRunFiles;
    DimDescribedService* fNumSubAndFits;
    NumSubAndFitsType fNumSubAndFitsData;

    ///Service for broadcasting subscription status
    DimDescribedService* fCurrentSubscription;
    ///Number of seconds since the last update of the subscribed list
    int fCurrentSubscriptionUpdateRate;
    ///The last time in seconds of the day when the service was update
    Time fLastSubscriptionUpdate;
    ///update the service
    void updateSubscriptionList();
    ///set the duration between two updates. a zero or negative value disables the service updates
    int setSubscriptionListUpdateTimeLapse(const Event& evt);
    /***************************************************
     * DATA LOGGER's CONFIGURATION STUFF
     ***************************************************/
    ///black/white listing
    set<string> fBlackList;
    set<string> fWhiteList;
    ///list of services to be grouped
    set<string> fGrouping;
    ///configuration flags
    bool fDebugIsOn;
    bool fOpenedFilesIsOn;
    bool fNumSubAndFitsIsOn;
    //functions for controlling the services behavior
    int SetDebugOnOff(const Event& evt);
    int SetStatsPeriod(const Event& evt);
    int SetOpenedFilesOnOff(const Event& evt);
    int SetNumSubsAndFitsOnOff(const Event& evt);
    int SetRunTimeoutDelay(const Event& evt);

    ///boolean to prevent DIM update while desctructing the dataLogger
    bool fDestructing;    
    /***************************************************
     * UTILITIES
     ***************************************************/
    ///vectors to keep track of opened Fits files, for grouping purposes.
    map<string, vector<string> > fOpenedNightlyFits;
    ///creates a group fits file based on a list of files to be grouped
    void CreateFitsGrouping(map<string, vector<string> >& filesToGroup);

    bool OpenStreamImp(ofstream &stream, const string &filename, bool mightbeopen);
    bool OpenStream(shared_ptr<ofstream> stream, const string &filename);
    ///Open the relevant text files related to a particular run
//    int OpenRunFile(RunNumberType& run);
    ///add a new run number
    void AddNewRunNumber(int64_t newRun, Time time);
    std::vector<int64_t> previousRunNumbers;
    ///removes the oldest run number, and close the relevant files.
    void RemoveOldestRunNumber();
    ///retrieves the size of a file
    off_t GetFileSize(const string&);
    ///Get the digits of year, month and day for filenames and paths
//    void GetYearMonthDayForFiles(unsigned short& year, unsigned short& month, unsigned short& day);
    ///Appends the relevant year month day to a given path
//    void AppendYearMonthDaytoPath(string& path);
    ///Form the files path
    string CompileFileNameWithPath(const string &path, const string &service, const string & extension);
    ///Form the file names only
    string CompileFileName(const string& service, const string& extension, const uint32_t& night) const;
    ///Check whether service is in black and/or white list
    bool ShouldSubscribe(const string& server, const string& service);
    ///Subscribe to a given server and service
//    EventImp& SubscribeTo(const string& server, const string& service);
    ///Open a text file and checks for ofstream status
    bool OpenTextFile(ofstream& stream, const string& name);
    ///Checks if the input osftream is in error state, and if so close it.
    bool CheckForOfstreamError(ofstream& out, bool isDailyStream);
    ///Goes to Write error states
    void GoToRunWriteErrorState();
    void GoToNightlyWriteErrorState();
    ///Checks if a given path exist
    bool DoesPathExist(string path);
    ///Check if old run numbers can be trimmed, and if so, do it
    void TrimOldRunNumbers();
    ///Create a given directory
    bool CreateDirectory(const string &path);
    /***************************************************
    * INHERITED FROM DimServiceInfoList
    ***************************************************/
    ///Add a new server subscription
    void AddServer(const string& server);
    ///Add a new service subscription
    void AddService(const Service& svc);
    ///Remove a given service subscription
    //FIXME unused
    void RemoveService(const string, const string, bool);
    ///Remove all the services associated with a given server
    //FIXME unused
    void RemoveAllServices(const string&);
    ///pointer to the dim's subscription that should distribute the run numbers.
    //DIM_REPLACE
    //DimInfo* fRunNumberService;
    unsigned int fRunNumberService;
    /***************************************************
     * Overwritten from MessageImp
    ***************************************************/
    vector<string> backLogBuffer;
    bool shouldBackLog;
    bool fShouldAutoStart;
    bool fAutoStarted;

    //Current day variable. Used to close nightly files when night changes
    Time fCurrentDay;
    Time lastFlush;

    DimDnsServiceList fDimList;
    vector<DimDescriptions*> fServerDescriptionsList;

    //counter for keeping tracker of services
    unsigned int servicesCounter;
public:
    int Write(const Time &time, const std::string &txt, int qos=kMessage);

}; //DataLogger


/**
 * @brief the two methods below were copied from StateMachineDimControl.cc
 *
 */
int DataLogger::HandleDescriptions(DimDescriptions* desc)
{
    fMutex.lock();
    for (auto it=desc->descriptions.begin(); it != desc->descriptions.end(); it++) {
        if (fDebugIsOn)
        {
            Debug("Adding description for service: " + it->front().name);
        }
        fServiceDescriptionList[it->front().name].assign(it->begin(), it->end());
    }
    fMutex.unlock();

    return GetCurrentState();
}
/**
 *  UPDATE SUBSCRIPTION LIST. Updates the subscription list service if enough time has passed.
 *                            Otherwise does nothing
 */
void DataLogger::updateSubscriptionList()
{
    if (fCurrentSubscriptionUpdateRate <= 0) return;
    Time timeNow;
    //if less than the update rate time has passed, just return
    if (timeNow - fLastSubscriptionUpdate < boost::posix_time::seconds(fCurrentSubscriptionUpdateRate))
        return;
    //TODO remove me !
//    cout << "Updating subscription list with: " << endl;

    fLastSubscriptionUpdate = timeNow;

    //update service !
    ostringstream output;
    for (auto serverIt=fServiceSubscriptions.begin();serverIt!=fServiceSubscriptions.end(); serverIt++)
    {
        if (serverIt->first == "DATA_LOGGER")
            continue;
        for (auto serviceIt=serverIt->second.begin(); serviceIt!=serverIt->second.end(); serviceIt++)
        {
            output << serverIt->first << "/" << serviceIt->first << ",";
            if (serviceIt->second.lastReceivedEvent != Time::None)
                output << (timeNow - serviceIt->second.lastReceivedEvent).total_seconds();
            else
                output << "-1";
            output << "\n";
        }
    }
//TODO remove me !
//cout << output.str();
    fCurrentSubscription->setData(output.str().c_str(), output.str().size()+1);
    fCurrentSubscription->setQuality(0);
    fCurrentSubscription->Update();
}
int DataLogger::setSubscriptionListUpdateTimeLapse(const Event& evt)
{
    fCurrentSubscriptionUpdateRate = evt.GetInt();

    return GetCurrentState();
}
vector<Description> DataLogger::GetDescription(const string& server, const string& service)
{
    const lock_guard<mutex> guard(fMutex);
    const auto it = fServiceDescriptionList.find(server+"/"+service);
    return it==fServiceDescriptionList.end()?vector<Description>():it->second;
}
// --------------------------------------------------------------------------
//
//! Overwritten write function. This way we directly log the datalogger's messages, without going through dim's dns,
//! thus increasing robustness.
//! @param time: see MessageImp class param
//! @param txt: see MessageImp class param
//! @param qos: see MessageImp class param
//! @return see MessageImp class param
//
int DataLogger::Write(const Time&time, const std::string& txt, int qos)
{
    ostringstream ss;
    ss << "datalogger: " << txt;
    if (fNightlyLogFile.is_open())
    {
        fNightlyLogImp.Write(time, ss.str(), qos);
    }
    else if (shouldBackLog)
    {
             ostringstream str;
             MessageImp mimp(str);
             mimp.Write(time, ss.str(), qos);
             backLogBuffer.push_back(str.str());
         }
    return StateMachineDim::Write(time, ss.str(), qos);
}
// --------------------------------------------------------------------------
//
//! Check if a given path exists
//! @param path the path to be checked
//! @return whether or not the creation has been successfull
//
bool DataLogger::CreateDirectory(const string &path)
{
    try
    {
        boost::filesystem::create_directories(path);
        return true;
    }
    catch (const runtime_error &e)
    {
        Error(e.what());
        return false;
    }
}
// --------------------------------------------------------------------------
//
//! Check if a given path exists
//! @param path the path to be checked
//! @return whether or not the given path exists
//
bool DataLogger::DoesPathExist(string path)
{
    return DimWriteStatistics::DoesPathExist(path, *this);
}

void DataLogger::AddServer(const string& server)
{
    Info("Got request to add server " + server );
    if (server != "DIS_DNS")
    {
        for (auto it=fServerDescriptionsList.begin(); it != fServerDescriptionsList.end(); it++)
            if ((*it)->server == server)
            {
                if (fDebugIsOn)
                {
                    ostringstream str;
                    str << "Already got description for server " << server << ". Ignoring." << endl;
                    Debug(str.str());
                    return;
                }
            }
        DimDescriptions* d = new DimDescriptions(server);
        d->SetCallbackDescriptions(bind(&DataLogger::HandleDescriptions, this, d));
        d->Subscribe(*this);
        fServerDescriptionsList.push_back(d);
    }

}

// --------------------------------------------------------------------------
//
//! Add a new service subscription
//! @param server the server for which the subscription should be created
//! @param service the service for which the subscription should be created
//! @param isCmd whether this is a Dim Command or not. Commands are not logged
//
void DataLogger::AddService(const Service& svc)
{
    const string& serverr = svc.server;
    //FIX in order to get rid of the '+' that sometimes makes it all the way to me
    string server = serverr;
    if (server.size() > 0 && server[0] == '+')
    {
        server = server.substr(1);
        Warn("Got a service beginning with +. This is not supposed to happen");
    }
//    server = server.substr(1);

    const string& service = svc.service;
    const bool isCmd = svc.iscmd;

   //dataLogger does not subscribe to commands
    if (isCmd)
        return;

    Info("Got request to add service: "+server+"/"+service);

    //check the given subscription against black and white lists
    if (!ShouldSubscribe(server, service))
        return;

    map<string, SubscriptionType> &list = fServiceSubscriptions[server];

    if (list.find(service) != list.end())
    {
        if (list[service].format != svc.format)
        {
            if (list[service].nightlyFile.IsOpen())
            {
                string fileName = list[service].nightlyFile.GetName();
                if (fileName == "")
                {
                    Error("Something went wrong while dealing with new format of "+server+"/"+service+" file tagged as open but filename is empty. Aborting");
                    return;
                }
                list[service].nightlyFile.Close();
                list[service].increment++;
                Warn("Format of "+server+"/"+service+" has changed. Closing "+fileName);
/*                string fileNameWithoutFits = fileName.substr(0, fileName.size()-4);
                int counter=0;
                while (counter < 100)
                {
                    ostringstream newFileName;
                    newFileName << fileNameWithoutFits << counter << ".fits";
                    ifstream testStream(newFileName.str());
                    if (!testStream) //fileName available
                    {
                        rename(fileName.c_str(), newFileName.str().c_str());
                        break;
                    }
                    counter++;
                }
                if (counter==100)
                    Error("Could not rename "+fileName+" after 100 trials (because of format change). Aborting");
*/
                //reallocate the fits buffer...
                list[service].fitsBufferAllocated = false;
            }
            list[service].fConv = shared_ptr<Converter>(new Converter(Out(), svc.format));
            list[service].format = svc.format;
        }
        if (fDebugIsOn)
            Debug("Service " + server + "/" + service + " is already in the dataLogger's list... ignoring update.");
        return;
    }
    //DIM_REPLACE
//    list[service].dimInfo.reset(SubscribeTo(server, service));
    if (fDebugIsOn)
        Debug("Subscribing to service "+server+"/"+service);
    Subscribe(server + "/" + service)
        (bind(&DataLogger::infoCallback, this, placeholders::_1, servicesCounter));
    list[service].server  = server;
    list[service].service = service;
    list[service].format = svc.format;
    list[service].index = servicesCounter;
    fNumSubAndFitsData.numSubscriptions++;
    //check if this is the run numbers service
    if ((server == "FAD_CONTROL") && (service == "START_RUN"))
        fRunNumberService = servicesCounter;
    servicesCounter++;
    Info("Added subscription to " + server + "/" + service);
}
// --------------------------------------------------------------------------
//
//! Remove a given service subscription
//! @param server the server for which the subscription should be removed
//! @param service the service that should be removed
//! @param isCmd whether or not this is a command
//
void DataLogger::RemoveService(string server, string service, bool isCmd)
{

    Info("Got request to remove service: "+server+"/"+service);
    if (fDestructing)//this function is called by the super class, after the destructor has deleted its own subscriptions
        return;
//FIXME unused
    return;

    if (isCmd)
        return;

    if (fServiceSubscriptions.find(server) == fServiceSubscriptions.end())
    {
        Error("Request to remove service "+service+" from server "+server+", but service not found.");
        return;
    }

    if (fServiceSubscriptions[server].erase(service) != 1)
    {
        //check the given subscription against black and white lists
        if (!ShouldSubscribe(server, service))
            return;

        Error("Subscription "+server+"/"+service+" could not be removed as it is not present");
        return;
    }
    fNumSubAndFitsData.numSubscriptions--;

    if ((server == "FAD_CONTROL") && (service == "START_RUN"))
        fRunNumberService = 0;

    Info("Removed subscription to " + server + "/" + service);
}
// --------------------------------------------------------------------------
//
//! Remove all the services associated with a given server
//! @param server the server for which all the services should be removed
//
void DataLogger::RemoveAllServices(const string& server)
{
    Info("Got request for removing all services from: "+server);
    if (fServiceSubscriptions.find(server)==fServiceSubscriptions.end())
    {
        Warn("Request to remove all services, but corresponding server " + server + " not found.");
        return;
    }
//FIXME unused
    return;
    fNumSubAndFitsData.numSubscriptions -= fServiceSubscriptions[server].size();

    fServiceSubscriptions[server].clear();
    fServiceSubscriptions.erase(server);

    if (server == "FAD_CONTROL")
        fRunNumberService = 0;

    if (fDebugIsOn)
        Debug("Removed all subscriptions to " + server + "/");
}

// --------------------------------------------------------------------------
//
//! Checks if the given ofstream is in error state and if so, close it
//! @param out the ofstream that should be checked
//
bool DataLogger::CheckForOfstreamError(ofstream& out, bool isDailyStream)
{
    if (out.good())
        return true;

    Error("An error occured while writing to a text file. Closing it");
    if (out.is_open())
        out.close();
    if (isDailyStream)
        GoToNightlyWriteErrorState();
    else
        GoToRunWriteErrorState();

    return false;
}

bool DataLogger::OpenStreamImp(ofstream &stream, const string &filename, bool mightbeopen)
{
    if (stream.is_open())
    {
        if (!mightbeopen)
            Error(filename+" was already open when trying to open it.");
        return mightbeopen;
    }

    errno = 0;
    stream.open(filename.c_str(), ios_base::out | ios_base::app);
    if (!stream /*|| errno!=0*/)
    {
        ostringstream str;
        str << "ofstream::open() failed for '" << filename << "': " << strerror(errno) << " [errno=" << errno << "]";
        Error(str);
        return false;
    }

    if (!stream.is_open())
    {
        Error("File "+filename+" not open as it ought to be.");
        return false;
    }

    Info("Opened: "+filename);

    return true;
}

bool DataLogger::OpenStream(shared_ptr<ofstream> stream, const string &filename)
{
    return OpenStreamImp(*stream, filename, false);
}

// --------------------------------------------------------------------------
//
//! Open a text file and checks for error code
//! @param stream the ofstream for which the file should be opened
//! @name the file name
//
bool DataLogger::OpenTextFile(ofstream& stream, const string& name)
{
    return OpenStreamImp(stream, name, true);
}

// --------------------------------------------------------------------------
//
//! Create a new dim subscription to a given server and service
//! @param server the server name
//! @param service the service name
//
/*EventImp& DataLogger::SubscribeTo(const string& server, const string& service)
{

    //DIM_REPLACE
    //return new DimStampedInfo((server + "/" + service).c_str(), (void*)NULL, 0, this);
    EventImp& newSubscription = Subscribe(server + "/" + service);
    newSubscription.bind(&infoHandler, this, placeholders::_1);
    return newSubscription;
}*/
// --------------------------------------------------------------------------
//
//! Check whether a service should be subscribed to, based on the black/white list entries
//! @param server the server name associated with the service being checked
//! @param service the service name associated with the service being checked
//
bool DataLogger::ShouldSubscribe(const string& server, const string& service)
{
    if ((fBlackList.find(server + "/") != fBlackList.end()) ||
         (fBlackList.find(server + "/" + service) != fBlackList.end()) ||
         (fBlackList.find("/" + service) != fBlackList.end()))
		 {
		     if (fWhiteList.size()>0 &&
        		(fWhiteList.find(server + "/" + service) != fWhiteList.end()))
				{
					if (fDebugIsOn)
						Debug("White list saved service " + server + "/" + service + " from blacklisting");
        			return true;
				}
			if (fDebugIsOn)
				Debug("Blacklist banned service " + server + "/" + service);
		 	return false;
		}
    return true;
}
// --------------------------------------------------------------------------
//
//! Compiles a file name
//! @param path the base path where to put the file
//! @param time the time at which the file is created
//! @param service the service name, if any
//! @param extension the extension to add, if any
//
string DataLogger::CompileFileName(const string& service, const string& extension, const uint32_t& night) const
{
    ostringstream str;

    str << night;

    if (!service.empty())
        str << '.' << service;

    if (!extension.empty())
        str << "." << extension;

    return str.str();
}

string DataLogger::CompileFileNameWithPath(const string& path, const string& service, const string& extension)
{
    ostringstream str;

    const Time time;

    // calculate time suitable for naming files. Get a time between the next sun-rise (fCurrentDay)
    // and the previous sun-rise so that NightAsInt (which refers to the previous sun-rise)
    // provides reasonable results.

    // WARNING: This fails if the sun-rise is around midnight UTC!
    // It would be more appropriate to relate the "NightAsInt" to
    // the LOCAL time of sun-rise!
    const Time ftime = fCurrentDay-boost::posix_time::hours(12);

    //output it
    const uint32_t night = ftime.NightAsInt();
    str << path << Tools::Form("/%04d/%02d/%02d/", night/10000, (night/100)%100, night%100);

    //check if target directory exist
    if (!DoesPathExist(str.str()))
        CreateDirectory(str.str());

    str << '/' << CompileFileName(service, extension, night);//fCurrentDay);

    return str.str();


}

// --------------------------------------------------------------------------
//
//!retrieves the size on disk of a file
//! @param fileName the full file name for which the size on disk should be retrieved
//! @return the size of the file on disk, in bytes. 0 if the file does not exist or if an error occured
//
off_t DataLogger::GetFileSize(const string& fileName)
{
    return DimWriteStatistics::GetFileSizeOnDisk(fileName, *this);
}

// --------------------------------------------------------------------------
//
//! Removes the oldest run number and closes the fits files that should be closed
//! Also creates the fits grouping file
//
void DataLogger::RemoveOldestRunNumber()
{
    if (fDebugIsOn)
    {
        ostringstream str;
        str << "Removing run number " << fRunNumber.front().runNumber;
        Debug(str);
    }
    //remove the entry
    fRunNumber.pop_front();
}

// --------------------------------------------------------------------------
//
//! Default constructor. The name of the machine is given DATA_LOGGER
//! and the state is set to kSM_Ready at the end of the function.
//
//!Setup the allows states, configs and transitions for the data logger
//
DataLogger::DataLogger(ostream &out) : StateMachineDim(out, "DATA_LOGGER"),
fNightlyLogImp(fNightlyLogFile), fFilesStats("DATA_LOGGER", *this)
{
    shouldBackLog = true;

    servicesCounter=1;

    //initialize member data
    fFilePath = ".";

    fDimList.Subscribe(*this);
    fDimList.SetCallbackServerAdd(bind(&DataLogger::AddServer, this, placeholders::_1));
    fDimList.SetCallbackServiceAdd(bind(&DataLogger::AddService, this, placeholders::_1));

    //calculate time "centered" around noon instead of midnight
    const Time timeNow;
//    const Time nowMinusTwelve = timeNow-boost::posix_time::hours(12);
    //the "current day" is actually the next closing time of nightly files
    //the next closing time is 30 minutes after upcoming sunrise.
    //If we are within 30 minutes after sunrise, closing time is soon
    fCurrentDay = timeNow.GetNextSunRise();//GetSunRise(timeNow-boost::posix_time::minutes(30))+boost::posix_time::minutes(30);
    lastFlush = Time();

    //Give a name to this machine's specific states
    AddStateName(kSM_NightlyOpen,      "NightlyFileOpen",  "The summary files for the night are open.");
    AddStateName(kSM_WaitingRun,       "WaitForRun",       "The summary files for the night are open and we wait for a run to be started.");
    AddStateName(kSM_Logging,          "Logging",          "The summary files for the night and the files for a single run are open.");
    AddStateName(kSM_BadFolder,        "ErrInvalidFolder", "The folder for the files is not invalid.");
    AddStateName(kSM_DailyWriteError,  "ErrDailyWrite",    "An error occured while writing to a daily (and run) file.");
    AddStateName(kSM_RunWriteError,    "ErrRunWrite",      "An error occured while writing to a run file.");

    // Add the possible transitions for this machine
    AddEvent("START", kSM_Ready, kSM_BadFolder)
        (bind(&DataLogger::Start, this))
        ("Start the nightly logging. Nightly file location must be specified already");

    AddEvent("STOP", kSM_NightlyOpen, kSM_WaitingRun, kSM_Logging, kSM_DailyWriteError, kSM_RunWriteError)
        (bind(&DataLogger::GoToReady, this))
        ("Stop all data logging, close all files.");

    AddEvent("RESET", kSM_Error, kSM_BadFolder, kSM_DailyWriteError, kSM_RunWriteError)
        (bind(&DataLogger::GoToReady, this))
        ("Transition to exit error states. Closes the any open file.");

    AddEvent("START_RUN_LOGGING", /*kSM_Logging,*/ kSM_NightlyOpen, kSM_Ready)
        (bind(&DataLogger::NightlyToWaitRun, this))
        ("Go to waiting for run number state. In this state with any received run-number a new file is opened.");

    AddEvent("STOP_RUN_LOGGING", kSM_WaitingRun, kSM_Logging)
        (bind(&DataLogger::BackToNightlyOpen, this))
        ("Go from the wait for run to nightly open state.");

     // Provide a print command
     AddEvent("PRINT_INFO")
            (bind(&DataLogger::PrintState, this, placeholders::_1))
            ("Print information about the internal status of the data logger.");


     OpenFileToDim fToDim;
     fToDim.code = 0;
     fToDim.fileName[0] = '\0';

     fOpenedNightlyFiles = new DimDescribedService(GetName() + "/FILENAME_NIGHTLY", "I:1;C", fToDim,
                               "Path and base name used for the nightly files."
                               "|Type[int]:type of open files (1=log, 2=rep, 4=fits)"
                               "|Name[string]:path and base file name");

     fOpenedRunFiles = new DimDescribedService(GetName() + "/FILENAME_RUN", "I:1;C", fToDim,
                               "Path and base name used for the run files."
                               "|Type[int]:type of open files (1=log, 2=rep, 4=fits)"
                               "|Name[string]:path and base file name");

     fNumSubAndFitsData.numSubscriptions = 0;
     fNumSubAndFitsData.numOpenFits = 0;
     fNumSubAndFits = new DimDescribedService(GetName() + "/NUM_SUBS", "I:2", fNumSubAndFitsData,
                               "Num. open files + num. subscribed services"
                               "|NSubAndOpenFiles[int]:Num. of subs and open files");

     //services parameters
     fDebugIsOn         = false;
     fOpenedFilesIsOn   = true;
     fNumSubAndFitsIsOn = true;

     string emptyString="";
     //Subscription list service
     fCurrentSubscription = new DimDescribedService(GetName() + "/SUBSCRIPTIONS", "C", emptyString.c_str(),
                                     "List of all the services subscribed by datalogger, except the ones provided by itself."
                                     "|Liste[string]:list of logged services and the delay in seconds since last update");
     fCurrentSubscriptionUpdateRate = 60; //by default, 1 minute between each update
     fLastSubscriptionUpdate = timeNow;

     // provide services control commands
     AddEvent("SET_DEBUG_MODE", "B:1", kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun, kSM_Ready)
         (bind(&DataLogger::SetDebugOnOff, this, placeholders::_1))
         ("Switch debug mode on or off. Debug mode prints information about every service written to a file."
          "|Enable[bool]:Enable of disable debug mode (yes/no).");

     AddEvent("SET_STATISTICS_UPDATE_INTERVAL", "S:1", kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun, kSM_Ready)
         (bind(&DataLogger::SetStatsPeriod, this, placeholders::_1))
         ("Interval in which the data-logger statistics service (STATS) is updated."
          "|Interval[ms]:Value in milliseconds (<=0: no update).");

     AddEvent("ENABLE_FILENAME_SERVICES", "B:1", kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun, kSM_Ready)
         (bind(&DataLogger::SetOpenedFilesOnOff ,this, placeholders::_1))
         ("Switch service which distributes information about the open files on or off."
          "|Enable[bool]:Enable of disable filename services (yes/no).");

     AddEvent("ENABLE_NUMSUBS_SERVICE", "B:1", kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun, kSM_Ready)
         (bind(&DataLogger::SetNumSubsAndFitsOnOff, this, placeholders::_1))
         ("Switch the service which distributes information about the number of subscriptions and open files on or off."
          "|Enable[bool]:Enable of disable NUM_SUBS service (yes/no).");

     AddEvent("SET_RUN_TIMEOUT", "L:1", kSM_Ready, kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun)
         (bind(&DataLogger::SetRunTimeoutDelay, this, placeholders::_1))
         ("Set the timeout delay for old run numbers."
          "|timeout[min]:Time out in minutes after which files for expired runs are closed.");
     //Provide access to the duration between two updates of the service list
     AddEvent("SET_SERVICE_LIST_UPDATE_INTERVAL", "I:1", kSM_Ready, kSM_NightlyOpen, kSM_Logging, kSM_WaitingRun)
         (bind(&DataLogger::setSubscriptionListUpdateTimeLapse, this, placeholders::_1))
         ("Set the min interval between two services-list updates."
          "|duration[sec]:The interval between two updates, in seconds.");

     fDestructing = false;

     fPreviousOldRunNumberCheck = Time();

     fDailyFileDayChangedAlready = true;
     fRunNumberTimeout = 60000; //default run-timeout set to 1 minute
     fRunNumber.push_back(RunNumberType());
     fRunNumber.back().runNumber = -1;
     fRunNumber.back().time = Time();
     NotifyOpenedFile("", 0, fOpenedNightlyFiles);
     NotifyOpenedFile("", 0, fOpenedRunFiles);

     fRunNumberService = 0;

     fShouldAutoStart = false;
     fAutoStarted = false;


     if(fDebugIsOn)
     {
         Debug("DataLogger Init Done.");
     }
}

// --------------------------------------------------------------------------
//
//! Destructor
//
DataLogger::~DataLogger()
{
    if (fDebugIsOn)
        Debug("DataLogger destruction starts");    

    //this boolean should not be required anymore
    fDestructing = true;

    //now clear the services subscriptions
    dim_lock();
    fServiceSubscriptions.clear();
    dim_unlock();

    //clear any remaining run number (should remain only one)
    while (fRunNumber.size() > 0)
    {
         RemoveOldestRunNumber();
    }
    //go to the ready state. i.e. close all files, run-wise first
    GoToReady();

    Info("Will soon close the daily log file");

    delete fOpenedNightlyFiles;
    delete fOpenedRunFiles;
    delete fNumSubAndFits;
    delete fCurrentSubscription;

    if (fNightlyLogFile.is_open())//this file is the only one that has not been closed by GoToReady
    {
        fNightlyLogFile << endl;
        fNightlyLogFile.close();
    }
    if (!fNightlyLogFile.is_open())
        Info("Daily log file was closed indeed");
    else
        Warn("Seems like there was a problem while closing the daily log file.");
    for (auto it=fServerDescriptionsList.begin(); it!= fServerDescriptionsList.end(); it++)
        delete *it;

    if (fDebugIsOn)
        Debug("DataLogger desctruction ends");    
}

// --------------------------------------------------------------------------
//
//! checks if old run numbers should be trimmed and if so, do it
//
void DataLogger::TrimOldRunNumbers()
{
    const Time cTime = Time();

    if (cTime - fPreviousOldRunNumberCheck < boost::posix_time::milliseconds(fRunNumberTimeout))
        return;

    while (fRunNumber.size() > 1 && (cTime - fRunNumber.back().time) > boost::posix_time::milliseconds(fRunNumberTimeout))
    {
         RemoveOldestRunNumber();
    }
    fPreviousOldRunNumberCheck = cTime;
}
// --------------------------------------------------------------------------
//
//! Inherited from DimInfo. Handles all the Infos to which we subscribed, and log them
//
int DataLogger::infoCallback(const EventImp& evt, unsigned int subIndex)
{
//    if (fDebugIsOn)
//    {
//        ostringstream str;
//        str << "Got infoCallback called with service index= " << subIndex;
//        Debug(str.str());
//    }

    if ((GetCurrentState() == kSM_Ready) &&  (!fAutoStarted) && fShouldAutoStart)
    {
        fAutoStarted = true;
        SetCurrentState(Start(), "infoCallback");
//        SetCurrentState(NightlyToWaitRun());
    }
    else
    {
        if (GetCurrentState() > kSM_Ready)
            fAutoStarted = true;
    }


    //check if the service pointer corresponds to something that we subscribed to
    //this is a fix for a bug that provides bad Infos when a server starts
    bool found = false;
    SubscriptionsListType::iterator x;
    map<string, SubscriptionType>::iterator y;
    for (x=fServiceSubscriptions.begin(); x != fServiceSubscriptions.end(); x++)
    {//find current service is subscriptions
     //Edit: this should be useless now... remove it sometimes ?
        for (y=x->second.begin(); y!=x->second.end();y++)
            if (y->second.index == subIndex)
            {
                found = true;    
                break;
            }
        if (found)
            break;
    }

    if (!found && fDebugIsOn)
    {
        ostringstream str;
        str << "Service " << evt.GetName() << " not found in subscriptions" << endl;
        Debug(str.str());
    }
    if (!found)
        return GetCurrentState();


    if (evt.GetSize() == 0 && fDebugIsOn)
    {
        ostringstream str;
        str << "Got 0 size for " << evt.GetName() << endl;
        Debug(str.str());
    }
    if (evt.GetSize() == 0)
        return GetCurrentState();

    if (evt.GetFormat() == "" && fDebugIsOn)
    {
        ostringstream str;
        str << "Got no format for " << evt.GetName() << endl;
        Debug(str.str());
    }
    if (evt.GetFormat() == "")
        return GetCurrentState();

//    cout.precision(20);
//    cout << "Orig timestamp: " << Time(I->getTimestamp(), I->getTimestampMillisecs()*1000).Mjd() << endl;
    // FIXME: Here we have to check if we have received the
    //        service with the run-number.
    //        CheckForRunNumber(I); has been removed because we have to
    //        subscribe to this service anyway and hence we have the pointer
    //        (no need to check for the name)
    CheckForRunNumber(evt, subIndex);

    Report(evt, y->second);

    //remove old run numbers
    TrimOldRunNumbers();

    return GetCurrentState();
}

// --------------------------------------------------------------------------
//
//! Add a new active run number
//! @param newRun the new run number
//! @param time the time at which the new run number was issued
//
void DataLogger::AddNewRunNumber(int64_t newRun, Time time)
{

    if (newRun > 0xffffffff)
    {
        Error("New run number too large, out of range. Ignoring.");
        return;
    }
    for (std::vector<int64_t>::const_iterator it=previousRunNumbers.begin(); it != previousRunNumbers.end(); it++)
    {
        if (*it == newRun)
        {
            Error("Newly provided run number has already been used (or is still in use). Going to error state");
            SetCurrentState(kSM_BadFolder, "AddNewRunNumber");
            return;
        }
    }
    if (fDebugIsOn)
    {
        ostringstream str;
        str << "Adding new run number " << newRun << " issued at " << time;
        Debug(str);
    }
    //Add new run number to run number list
    fRunNumber.push_back(RunNumberType());
    fRunNumber.back().runNumber = int32_t(newRun);
    fRunNumber.back().time = time;

    if (fDebugIsOn)
    {
        ostringstream str;
        str << "The new run number is: " << fRunNumber.back().runNumber;
        Debug(str);
    }
    if (GetCurrentState() != kSM_Logging && GetCurrentState() != kSM_WaitingRun )
        return;

    if (newRun > 0 && GetCurrentState()  == kSM_WaitingRun)
        SetCurrentState(kSM_Logging, "AddNewRunNumber");
    if (newRun < 0 && GetCurrentState() == kSM_Logging)
        SetCurrentState(kSM_WaitingRun, "AddNewRunNumber");
}
// --------------------------------------------------------------------------
//
//! Checks whether or not the current info is a run number.
//! If so, then remember it. A run number is required to open the run-log file
//! @param I
//!        the current DimInfo
//
void DataLogger::CheckForRunNumber(const EventImp& evt, unsigned int index)
{
    if (index != fRunNumberService)
        return;
//    int64_t newRun = reinterpret_cast<const uint64_t*>(evt.GetData())[0];
    AddNewRunNumber(evt.GetXtra(), evt.GetTime());
}
// --------------------------------------------------------------------------
//
//! Get SunRise. Copied from drivectrl.cc
//! Used to know when to close and reopen files
//!
/*
Time DataLogger::GetSunRise(const Time &time)
{
#ifdef HAVE_NOVA
    const double lon = -(17.+53./60+26.525/3600);
    const double lat =   28.+45./60+42.462/3600;

    ln_lnlat_posn observer;
    observer.lng = lon;
    observer.lat = lat;

    // This caluclates the sun-rise of the next day after 12:00 noon
    ln_rst_time sun_day;
    if (ln_get_solar_rst(time.JD(), &observer, &sun_day)==1)
    {
        Fatal("GetSunRise reported the sun to be circumpolar!");
        return Time(Time::none);
    }

    if (Time(sun_day.rise)>=time)
        return Time(sun_day.rise);

    if (ln_get_solar_rst(time.JD()+0.5, &observer, &sun_day)==1)
    {
        Fatal("GetSunRise reported the sun to be circumpolar!");
        return Time(Time::none);
    }

    return Time(sun_day.rise);
#else
    return time;
#endif
}
*/
// --------------------------------------------------------------------------
//
//! write infos to log files.
//! @param I
//!     The current DimInfo 
//! @param sub
//!        The dataLogger's subscription corresponding to this DimInfo
//
void DataLogger::Report(const EventImp& evt, SubscriptionType& sub)
{
    const string fmt(evt.GetFormat());

    const bool isItaReport = fmt!="C";

    if (!fNightlyLogFile.is_open())
        return;

    if (fDebugIsOn && string(evt.GetName())!="DATA_LOGGER/MESSAGE")
    {
        ostringstream str;
        str << "Logging " << evt.GetName() << " [" << evt.GetFormat() << "] (" << evt.GetSize() << ")";
        Debug(str);
    }

    //
    // Check whether we should close and reopen daily text files or not
    // calculate time "centered" around noon instead of midnight
    // if number of days has changed, then files should be closed and reopenned.
    const Time timeNow;
//    const Time nowMinusTwelve = timeNow-boost::posix_time::hours(12);
//    int newDayNumber = (int)(nowMinusTwelve.Mjd());

    //also check if we should flush the nightly files
    if (lastFlush < timeNow-boost::posix_time::minutes(1))
    {
        lastFlush = timeNow;
        SubscriptionsListType::iterator x;
        map<string, SubscriptionType>::iterator y;
        for (x=fServiceSubscriptions.begin(); x != fServiceSubscriptions.end(); x++)
        {//find current service is subscriptions
            for (y=x->second.begin(); y!=x->second.end();y++)
                if (y->second.nightlyFile.IsOpen())
                {
                    y->second.nightlyFile.Flush();
                }
        }
        if (fDebugIsOn)
            Debug("Just flushed nightly fits files to the disk");
    }
    //check if we should close and reopen the nightly files
    if (timeNow > fCurrentDay)//GetSunRise(fCurrentDay)+boost::posix_time::minutes(30)) //if we went past 30 minutes after sunrise
    {
        //set the next closing time. If we are here, we have passed 30 minutes after sunrise.
        fCurrentDay = timeNow.GetNextSunRise();//GetSunRise(timeNow-boost::posix_time::minutes(30))+boost::posix_time::minutes(30);
        //crawl through the subcriptions and close any open nightly file
        SubscriptionsListType::iterator x;
        map<string, SubscriptionType>::iterator y;
        for (x=fServiceSubscriptions.begin(); x != fServiceSubscriptions.end(); x++)
        {//find current service is subscriptions
            for (y=x->second.begin(); y!=x->second.end();y++)
            {
                if (y->second.nightlyFile.IsOpen())
                {
                    y->second.nightlyFile.Close();
                }
                y->second.increment = 0;
            }
        }

        if (fDebugIsOn)
            Debug("Day have changed! Closing and reopening nightly files");

        fNightlyLogFile << endl;
        fNightlyLogFile.close();
//        fNightlyReportFile.close();

        Info("Closed: "+fFullNightlyLogFileName);
//        Info("Closed: "+fFullNightlyReportFileName);

        fFullNightlyLogFileName = CompileFileNameWithPath(fFilePath, "", "log");
        if (!OpenTextFile(fNightlyLogFile, fFullNightlyLogFileName))
        {
            GoToReady();
            SetCurrentState(kSM_BadFolder, "Report");
            return;
        }
        fNightlyLogFile << endl;

//        fFullNightlyReportFileName = CompileFileNameWithPath(fFilePath, "", "rep");
//        if (!OpenTextFile(fNightlyReportFile, fFullNightlyReportFileName))
//        {
//            GoToReady();
//            SetCurrentState(kSM_BadFolder, "Report");
//            return;
//        }
    }
    //create the converter for that service
    if (!sub.fConv)
    {
        sub.fConv = shared_ptr<Converter>(new Converter(Out(), evt.GetFormat()));
        if (!sub.fConv->valid())
        {
            ostringstream str;
            str << "Couldn't properly parse the format... service " << evt.GetName() << " ignored.";
            Error(str);
            return;    
        }
    }
    //construct the header
    ostringstream header;
    const Time cTime(evt.GetTime());
    fQuality = evt.GetQoS();

    //update subscription last received time
    sub.lastReceivedEvent = cTime;
    //update subscription list service if required
    updateSubscriptionList();

    fMjD = cTime.Mjd() ? cTime.Mjd()-40587 : 0;

    if (isItaReport)
    {
//DISABLED REPORT WRITING BY THOMAS REQUEST
        //write text header
/*        string serviceName = (sub.service == "MESSAGE") ? "" : "_"+sub.service;
        header << sub.server << serviceName << " " << fQuality << " ";
        header << evt.GetTime() << " ";

        string text;
        try
        {
            text = sub.fConv->GetString(evt.GetData(), evt.GetSize());
        }
        catch (const runtime_error &e)
        {
            ostringstream str;
            str << "Parsing service " << evt.GetName();
            str << " failed: " << e.what() << " removing the subscription to " << sub.server << "/" << sub.service;
            Warn(str);
            //remove this subscription from the list.
            //because these operators use references to elements, and because they're supposed here to erase these objects on the way, I'm not too sure... so duplicate the names !
            RemoveService(sub.server, sub.service, false);
            return;
        }

        if (text.empty())
        {
            ostringstream str;
            str << "Service " << evt.GetName() << " sent an empty string";
            Info(str);
            return;
        }
        //replace bizarre characters by white space
        replace(text.begin(), text.end(), '\n', '\\');
        replace_if(text.begin(), text.end(), ptr_fun<int, int>(&iscntrl), ' ');
        
        //write entry to Nightly report
        if (fNightlyReportFile.is_open())
        {
            fNightlyReportFile << header.str() << text << endl;
            if (!CheckForOfstreamError(fNightlyReportFile, true))
                return;
        }
*/
#ifdef HAVE_FITS
        //check if the last received event was before noon and if current one is after noon.
        //if so, close the file so that it gets reopened.
//        sub.lastReceivedEvent = cTime;
        if (!sub.nightlyFile.IsOpen())
            if (GetCurrentState() != kSM_Ready)
                OpenFITSFiles(sub);
        WriteToFITS(sub, evt.GetData());
#endif
    }
    else
    {//write entry to Nightly log
        vector<string> strings;
        try
        {
           strings = sub.fConv->ToStrings(evt.GetData());
        }
        catch (const runtime_error &e)
        {
            ostringstream str;
            str << "Parsing service " << evt.GetName();
            str << " failed: " << e.what() << " removing the subscription for now.";
            Error(str);
            //remove this subscription from the list.
            //because these operators use references to elements, and because they're supposed here to erase these objects on the way, I'm not too sure... so duplicate the names !
            RemoveService(sub.server, sub.service, false);
            return;
        }
        if (strings.size() > 1)
        {
            ostringstream err;
            err << "There was more than one string message in service " << evt.GetName() << " going to fatal error state";
            Error(err.str());
        }

        bool isMessage = (sub.service == "MESSAGE");
        ostringstream msg;
        string serviceName = isMessage ? "" : "_"+sub.service;
        msg << sub.server << serviceName;


        //in case of non messages message (i.e. binary data written to logs)
        //we override the quality before writing to .log, otherwise it will wrongly decorate the log entry
        //because fQuality is really the system state in some cases.
        //so save a backup of the original value before writing to fits
        int backup_quality = fQuality;

        //fix the quality of non message "messages"
        if (!isMessage)
        {
            msg << "[" << fQuality << "]";
            fQuality = kMessage;
        }

        //special case for alarm reset
        if (isMessage && (fQuality == kAlarm) && (strings[0] == ""))
        {
            fQuality = kInfo;
            strings[0] = "Alarm reset";
        }
        msg << ": " << strings[0];

        if (fNightlyLogFile.is_open())
        {
            fNightlyLogImp.Write(cTime, msg.str().c_str(), fQuality);
            if (!CheckForOfstreamError(fNightlyLogFile, true))
                return;
        }

        //in case we have overriden the fQuality before writing to log, restore the original value before writing to FITS
        if (!isMessage)
            fQuality = backup_quality;

//        sub.lastReceivedEvent = cTime;
        if (!sub.nightlyFile.IsOpen())
            if (GetCurrentState() != kSM_Ready)
                OpenFITSFiles(sub);
        WriteToFITS(sub, evt.GetData());
    }
}

// --------------------------------------------------------------------------
//
//! print the dataLogger's current state. invoked by the PRINT command
//! @param evt
//!        the current event. Not used by the method
//! @returns 
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::PrintState(const Event& )
{
    Message("------------------------------------------");
    Message("------- DATA LOGGER CURRENT STATE --------");
    Message("------------------------------------------");

    //print the path configuration
#if BOOST_VERSION < 104600
    Message("File path:    " + boost::filesystem::system_complete(boost::filesystem::path(fFilePath)).directory_string());
#else
    Message("File path:    " + boost::filesystem::system_complete(boost::filesystem::path(fFilePath)).parent_path().string());
#endif

    //print active run numbers
    ostringstream str;
    //timeout value
    str << "Timeout delay for old run numbers: " << fRunNumberTimeout << " ms";
    Message(str);
    str.str("");
    str << "Active Run Numbers:";
    for (list<RunNumberType>::const_iterator it=fRunNumber.begin(); it!=fRunNumber.end(); it++)
        str << " " << it->runNumber;
    if (fRunNumber.empty())
        str << " <none>";
    Message(str);

    //print all the open files. 
    Message("------------ OPEN FILES ----------------");
    if (fNightlyLogFile.is_open())
        Message("Nightly log-file:    "+fFullNightlyLogFileName);

//    if (fNightlyReportFile.is_open())
 //       Message("Nightly report-file: "+fFullNightlyReportFileName);

    const DimWriteStatistics::Stats statVar = fFilesStats.GetTotalSizeWritten();
 //   /*const bool statWarning =*/ calculateTotalSizeWritten(statVar, true);
#ifdef HAVE_FITS
    str.str("");
    str << "Number of open FITS files: " << fNumSubAndFitsData.numOpenFits;
    Message(str);
    // FIXME: Print list of open FITS files
#else
    Message("FITS output disabled at compilation");
#endif
    Message("----------------- STATS ------------------");
    if (fFilesStats.GetUpdateInterval()>0)
    {
        str.str("");
        str << "Statistics are updated every " << fFilesStats.GetUpdateInterval() << " ms";
        Message(str);
    }
    else
        Message("Statistics updates are currently disabled.");
    str.str("");
    str << "Total Size written: " << statVar.sizeWritten/1000 << " kB";
        Message(str);
    str.str("");
    str << "Disk free space:    " << statVar.freeSpace/1000000   << " MB";
    Message(str);

    Message("------------ DIM SUBSCRIPTIONS -----------");
    str.str("");
    str << "There are " << fNumSubAndFitsData.numSubscriptions << " active DIM subscriptions.";
    Message(str);
    for (map<const string, map<string, SubscriptionType> >::const_iterator it=fServiceSubscriptions.begin(); it!= fServiceSubscriptions.end();it++)
    {
        Message("Server "+it->first);
        for (map<string, SubscriptionType>::const_iterator it2=it->second.begin(); it2!=it->second.end(); it2++)
            Message(" -> "+it2->first);
    }
    Message("--------------- BLOCK LIST ---------------");
    for (set<string>::const_iterator it=fBlackList.begin(); it != fBlackList.end(); it++)
        Message(" -> "+*it);
    if (fBlackList.empty())
        Message(" <empty>");

    Message("--------------- ALLOW LIST ---------------");
    for (set<string>::const_iterator it=fWhiteList.begin(); it != fWhiteList.end(); it++)
        Message(" -> "+*it);
    if (fWhiteList.empty())
        Message(" <empty>");

    Message("-------------- GROUPING LIST -------------");
    Message("The following servers and/or services will");
    Message("be grouped into a single fits file:");
    for (set<string>::const_iterator it=fGrouping.begin(); it != fGrouping.end(); it++)
        Message(" -> "+*it);
    if (fGrouping.empty())
        Message(" <no grouping>");

    Message("------------------------------------------");
    Message("-------- END OF DATA LOGGER STATE --------");
    Message("------------------------------------------");

    return GetCurrentState();
}

// --------------------------------------------------------------------------
//
//! turn debug mode on and off
//! @param evt
//!        the current event. contains the instruction string: On, Off, on, off, ON, OFF, 0 or 1
//! @returns 
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::SetDebugOnOff(const Event& evt)
{
    const bool backupDebug = fDebugIsOn;

    fDebugIsOn = evt.GetBool();

    if (fDebugIsOn == backupDebug)
        Message("Debug mode was already in the requested state.");

    ostringstream str;
    str << "Debug mode is now " << fDebugIsOn;
    Message(str);

    fFilesStats.SetDebugMode(fDebugIsOn);

    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! set the statistics update period duration. 0 disables the statistics
//! @param evt
//!        the current event. contains the new duration.
//! @returns 
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::SetStatsPeriod(const Event& evt)
{
    fFilesStats.SetUpdateInterval(evt.GetShort());
    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! set the opened files service on or off. 
//! @param evt
//!        the current event. contains the instruction string. similar to setdebugonoff
//! @returns 
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::SetOpenedFilesOnOff(const Event& evt)
{
    const bool backupOpened = fOpenedFilesIsOn;

    fOpenedFilesIsOn = evt.GetBool();

    if (fOpenedFilesIsOn == backupOpened)
        Message("Opened files service mode was already in the requested state.");

    ostringstream str;
    str << "Opened files service mode is now " << fOpenedFilesIsOn;
    Message(str);

    return GetCurrentState();
}

// --------------------------------------------------------------------------
//
//! set the number of subscriptions and opened fits on and off
//! @param evt
//!        the current event. contains the instruction string. similar to setdebugonoff
//! @returns 
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::SetNumSubsAndFitsOnOff(const Event& evt)
{
    const bool backupSubs = fNumSubAndFitsIsOn;

    fNumSubAndFitsIsOn = evt.GetBool();

    if (fNumSubAndFitsIsOn == backupSubs)
        Message("Number of subscriptions service mode was already in the requested state");

    ostringstream str;
    str << "Number of subscriptions service mode is now " << fNumSubAndFitsIsOn;
    Message(str);

    return GetCurrentState();
}
// --------------------------------------------------------------------------
//
//! set the timeout delay for old run numbers
//! @param evt
//!        the current event. contains the timeout delay long value
//! @returns
//!        the new state. Which, in that case, is the current state
//!
int DataLogger::SetRunTimeoutDelay(const Event& evt)
{
    if (evt.GetUInt() == 0)
    {
        Error("Timeout delays for old run numbers must be greater than 0... ignored.");
        return GetCurrentState();
    }

    if (fRunNumberTimeout == evt.GetUInt())
        Message("New timeout for old run numbers is same value as previous one.");

    fRunNumberTimeout = evt.GetUInt();

    ostringstream str;
    str  << "Timeout delay for old run numbers is now " << fRunNumberTimeout << " ms";
    Message(str);

    return GetCurrentState();
}

// --------------------------------------------------------------------------
//
//! Notifies the DIM service that a particular file was opened
//! @ param name the base name of the opened file, i.e. without path nor extension. 
//!     WARNING: use string instead of string& because I pass values that do not convert to string&.
//!        this is not a problem though because file are not opened so often.
//! @ param type the type of the opened file. 0 = none open, 1 = log, 2 = text, 4 = fits
inline void DataLogger::NotifyOpenedFile(const string &name, int type, DimDescribedService* service)
{
    if (!fOpenedFilesIsOn)
        return;

    if (fDebugIsOn)
    {
        ostringstream str;
        str << "Updating " << service->getName() << " file '" << name << "' (type=" << type << ")";
        Debug(str);

        str.str("");
        str << "Num subscriptions: " << fNumSubAndFitsData.numSubscriptions << " Num open FITS files: " << fNumSubAndFitsData.numOpenFits;
        Debug(str);
    }

    if (name.size()+1 > FILENAME_MAX)
    {
        Error("Provided file name '" + name + "' is longer than allowed file name length.");
        return;
    }

    OpenFileToDim fToDim;
    fToDim.code = type;
    memcpy(fToDim.fileName, name.c_str(), name.size()+1);

    service->setData(reinterpret_cast<void*>(&fToDim), name.size()+1+sizeof(uint32_t));
    service->setQuality(0);
    service->Update();
}
// --------------------------------------------------------------------------
//
//! Implements the Start transition.
//! Concatenates the given path for the Nightly file and the filename itself (based on the day), 
//! and tries to open it.
//! @returns 
//!        kSM_NightlyOpen if success, kSM_BadFolder if failure
int DataLogger::Start()
{
    if (fDebugIsOn)
    {
        Debug("Starting...");    
    }
    fFullNightlyLogFileName = CompileFileNameWithPath(fFilePath, "", "log");
    bool nightlyLogOpen = fNightlyLogFile.is_open();
    if (!OpenTextFile(fNightlyLogFile, fFullNightlyLogFileName))
        return kSM_BadFolder;
    if (!nightlyLogOpen)
        fNightlyLogFile << endl;

//    fFullNightlyReportFileName = CompileFileNameWithPath(fFilePath, "", "rep");
//    if (!OpenTextFile(fNightlyReportFile, fFullNightlyReportFileName))
//    {
//        fNightlyLogFile.close();
//        Info("Closed: "+fFullNightlyReportFileName);
//        return kSM_BadFolder;
//    }

    fFilesStats.FileOpened(fFullNightlyLogFileName);
//    fFilesStats.FileOpened(fFullNightlyReportFileName);
    //notify that a new file has been opened.
    const string baseFileName = CompileFileNameWithPath(fFilePath, "", "");
    NotifyOpenedFile(baseFileName, 3, fOpenedNightlyFiles);

    fOpenedNightlyFits.clear();
    
    return kSM_NightlyOpen;     
}

#ifdef HAVE_FITS
// --------------------------------------------------------------------------
//
//! open if required a the FITS files corresponding to a given subscription
//! @param sub
//!     the current DimInfo subscription being examined
void DataLogger::OpenFITSFiles(SubscriptionType& sub)
{
    string serviceName(sub.server + "_" + sub.service);//evt.GetName());

    for (unsigned int i=0;i<serviceName.size(); i++)
    {
        if (serviceName[i] == '/')
        {
            serviceName[i] = '_';
            break;    
        }    
    }
    //we open the NightlyFile anyway, otherwise this function shouldn't have been called.
    if (!sub.nightlyFile.IsOpen())
    {
        string incrementedServiceName = serviceName;
        if (sub.increment != 0)
        {
            ostringstream str;
            str << "." << sub.increment;
            incrementedServiceName += str.str();
        }
        const string partialName = CompileFileNameWithPath(fFilePath, incrementedServiceName, "fits");

        const string fileNameOnly = partialName.substr(partialName.find_last_of('/')+1, partialName.size());
        if (!sub.fitsBufferAllocated)
            AllocateFITSBuffers(sub);
        //get the size of the file we're about to open
        if (fFilesStats.FileOpened(partialName))
            fOpenedNightlyFits[fileNameOnly].push_back(serviceName);

        if (!sub.nightlyFile.Open(partialName, serviceName, &fNumSubAndFitsData.numOpenFits, this, 0))
        {
            GoToRunWriteErrorState();
            return;
        }

        ostringstream str;
        str << "Opened: " << partialName << " (Nfits=" << fNumSubAndFitsData.numOpenFits << ")";
        Info(str);

        //notify the opening
        const string baseFileName = CompileFileNameWithPath(fFilePath, "", "");
        NotifyOpenedFile(baseFileName, 7, fOpenedNightlyFiles);
        if (fNumSubAndFitsIsOn)
            fNumSubAndFits->Update();
    }

}    
// --------------------------------------------------------------------------
//
//! Allocates the required memory for a given pair of fits files (nightly and run)
//! @param sub the subscription of interest.
//
void DataLogger::AllocateFITSBuffers(SubscriptionType& sub)
{
    //Init the time columns of the file
    Description dateDesc(string("Time"), string("Modified Julian Date"), string("MJD"));
    sub.nightlyFile.AddStandardColumn(dateDesc, "1D", &fMjD, sizeof(double));

    Description QoSDesc("QoS", "Quality of service", "");
    sub.nightlyFile.AddStandardColumn(QoSDesc, "1J", &fQuality, sizeof(int));

    // Compilation failed
    if (!sub.fConv->valid())
    {
        Error("Compilation of format string failed.");
        return;
    }

    //we've got a nice structure describing the format of this service's messages.
    //Let's create the appropriate FITS columns
    const vector<string> dataFormatsLocal = sub.fConv->GetFitsFormat();

    ostringstream str;
    str << "Initializing data columns for service " << sub.server << "/" << sub.service;
    Info(str);
    sub.nightlyFile.InitDataColumns(GetDescription(sub.server, sub.service), dataFormatsLocal, this);

    sub.fitsBufferAllocated = true;
}
// --------------------------------------------------------------------------
//
//! write a dimInfo data to its corresponding FITS files
//
//FIXME: DO I REALLY NEED THE EVENT IMP HERE ???
void DataLogger::WriteToFITS(SubscriptionType& sub, const void* data)
{
        //nightly File status (open or not) already checked
        if (sub.nightlyFile.IsOpen())
        {
            if (!sub.nightlyFile.Write(*sub.fConv.get(), data))
            {
                RemoveService(sub.server, sub.service, false);
                GoToNightlyWriteErrorState();
                return;
            }
         }
}
#endif //if has_fits
// --------------------------------------------------------------------------
//
//! Go to Run Write Error State
//      A write error has occurred. Checks what is the current state and take appropriate action
void DataLogger::GoToRunWriteErrorState()
{
    if ((GetCurrentState() != kSM_RunWriteError) &&
        (GetCurrentState() != kSM_DailyWriteError))
        SetCurrentState(kSM_RunWriteError, "GoToRunWriteErrorState");
}
// --------------------------------------------------------------------------
//
//! Go to Nightly Write Error State
//      A write error has occurred. Checks what is the current state and take appropriate action
void DataLogger::GoToNightlyWriteErrorState()
{
    if (GetCurrentState() != kSM_DailyWriteError)
        SetCurrentState(kSM_DailyWriteError, "GoToNightlyWriteErrorState");
}


#ifdef HAVE_FITS
// --------------------------------------------------------------------------
//
//! Create a fits group file with all the run-fits that were written (either daily or run)
//! @param filesToGroup a map of filenames mapping to table names to be grouped (i.e. a
//!        single file can contain several tables to group
//! @param runNumber the run number that should be used for grouping. 0 means nightly group
//
void DataLogger::CreateFitsGrouping(map<string, vector<string> > & filesToGroup)
{
    if (fDebugIsOn)
    {
        ostringstream str;
        str << "Creating fits group for nightly files";
        Debug(str);
    }
    //create the FITS group corresponding to the ending run.
    CCfits::FITS* groupFile;
    unsigned int numFilesToGroup = 0;
    unsigned int maxCharLength = 0;
    for (map<string, vector<string> >::const_iterator it=filesToGroup.begin(); it != filesToGroup.end(); it++)
    {
        //add the number of tables in this file to the total number to group
        numFilesToGroup += it->second.size();
        //check the length of all the strings to be written, to determine the max string length to write
        if (it->first.size() > maxCharLength)
            maxCharLength = it->first.size();
        for (vector<string>::const_iterator jt=it->second.begin(); jt != it->second.end(); jt++)
            if (jt->size() > maxCharLength)
                maxCharLength = jt->size();
    }

    if (fDebugIsOn)
    {
        ostringstream str;
        str << "There are " << numFilesToGroup << " tables to group";
        Debug(str);
    }
    if (numFilesToGroup <= 1)
    {
        filesToGroup.clear();
        return;
    }
    const string groupName = CompileFileNameWithPath(fFilePath, "", "fits");

    Info("Creating FITS group in: "+groupName);

    CCfits::Table* groupTable;

    try
    {
        groupFile = new CCfits::FITS(groupName, CCfits::RWmode::Write);
        //setup the column names
        ostringstream pathTypeName;
        pathTypeName << maxCharLength << "A";
        vector<string> names;
        vector<string> dataTypes;
        names.emplace_back("MEMBER_XTENSION");
        dataTypes.emplace_back("8A");
        names.emplace_back("MEMBER_URI_TYPE");
        dataTypes.emplace_back("3A");
        names.emplace_back("MEMBER_LOCATION");
        dataTypes.push_back(pathTypeName.str());
        names.emplace_back("MEMBER_NAME");
        dataTypes.push_back(pathTypeName.str());
        names.emplace_back("MEMBER_VERSION");
        dataTypes.emplace_back("1J");
        names.emplace_back("MEMBER_POSITION");
        dataTypes.emplace_back("1J");

        groupTable = groupFile->addTable("GROUPING", numFilesToGroup, names, dataTypes);
//TODO handle the case when the logger was stopped and restarted during the same day, i.e. the grouping file must be updated
     }
     catch (CCfits::FitsException e)
     {
         ostringstream str;
         str << "Creating FITS table GROUPING in " << groupName << ": " << e.message();
         Error(str);
         return;
     }
     try
     {
         groupTable->addKey("GRPNAME", "FACT_RAW_DATA", "Data from the FACT telescope");
     }
     catch (CCfits::FitsException e)
     {
         Error("CCfits::Table::addKey failed for 'GRPNAME' in '"+groupName+"-GROUPING': "+e.message());
         return;
     }
    //CCfits seems to be buggy somehow: can't use the column's function "write": it create a compilation error: maybe strings were not thought about.
    //use cfitsio routines instead
    groupTable->makeThisCurrent();
    //create appropriate buffer.
    const unsigned int n = 8 + 3 + 2*maxCharLength + 1 + 8; //+1 for trailling character

    vector<char> realBuffer(n);

    char *startOfExtension = realBuffer.data();
    char *startOfURI       = realBuffer.data()+8;
    char *startOfLocation  = realBuffer.data()+8+3;
    char *startOfName      = realBuffer.data()+8+3+maxCharLength;

    strcpy(startOfExtension, "BINTABLE");
    strcpy(startOfURI,       "URL");

    realBuffer[8+3+2*maxCharLength+3] = 1;
    realBuffer[8+3+2*maxCharLength+7] = 1;

    int i=1;
    for (map<string, vector<string> >::const_iterator it=filesToGroup.begin(); it!=filesToGroup.end(); it++)
        for (vector<string>::const_iterator jt=it->second.begin(); jt != it->second.end(); jt++, i++)
        {
            memset(startOfLocation, 0, 2*maxCharLength+1+8);

            strcpy(startOfLocation, it->first.c_str());
            strcpy(startOfName,     jt->c_str());

            if (fDebugIsOn)
            {
                ostringstream str;
                str << "Grouping " << it->first << " " << *jt;
                Debug(str);
            }

            int status = 0;
            fits_write_tblbytes(groupFile->fitsPointer(), i, 1, 8+3+2*maxCharLength +8,
                                reinterpret_cast<unsigned char*>(realBuffer.data()), &status);
            if (status)
            {
                char text[30];//max length of cfitsio error strings (from doc)
                fits_get_errstatus(status, text);
                ostringstream str;
                str << "Writing FITS row " << i << " in " << groupName << ": " << text << " (file_write_tblbytes, rc=" << status << ")";
                Error(str);
                GoToRunWriteErrorState();
                delete groupFile;
                return;
            }
        }

    filesToGroup.clear();
    delete groupFile;
}
#endif //HAVE_FITS

// --------------------------------------------------------------------------
//
//! Implements the StopRun transition.
//! Attempts to close the run file.
//! @returns
//!        kSM_WaitingRun if success, kSM_FatalError otherwise
int DataLogger::StopRunLogging()
{

    if (fDebugIsOn)
    {
        Debug("Stopping Run Logging...");    
    }

    if (fNumSubAndFitsIsOn)
        fNumSubAndFits->Update();

    while (fRunNumber.size() > 0)
    {
        RemoveOldestRunNumber();
    }
    return kSM_WaitingRun;
}
// --------------------------------------------------------------------------
//
//! Implements the Stop and Reset transitions.
//! Attempts to close any openned file.
//! @returns
//!     kSM_Ready
int DataLogger::GoToReady()
{
   if (fDebugIsOn)
   {
        Debug("Going to the Ready state...");
   }
   if (GetCurrentState() == kSM_Logging || GetCurrentState() == kSM_WaitingRun)
       StopRunLogging();

   //it may be that dim tries to write a dimInfo while we're closing files. Prevent that
   const string baseFileName = CompileFileNameWithPath(fFilePath, "", "");

//    if (fNightlyReportFile.is_open())
//    {
//        fNightlyReportFile.close();
//        Info("Closed: "+baseFileName+".rep");
//    }
#ifdef HAVE_FITS
    for (SubscriptionsListType::iterator i = fServiceSubscriptions.begin(); i != fServiceSubscriptions.end(); i++)
        for (map<string, SubscriptionType>::iterator j = i->second.begin(); j != i->second.end(); j++)
        {
            if (j->second.nightlyFile.IsOpen())
                j->second.nightlyFile.Close();
        }
#endif
    if (GetCurrentState() == kSM_Logging || 
        GetCurrentState() == kSM_WaitingRun || 
        GetCurrentState() == kSM_NightlyOpen)
    { 
        NotifyOpenedFile("", 0, fOpenedNightlyFiles);
        if (fNumSubAndFitsIsOn)
            fNumSubAndFits->Update();
    }
#ifdef HAVE_FITS
    CreateFitsGrouping(fOpenedNightlyFits);
#endif
    return kSM_Ready;
}

// --------------------------------------------------------------------------
//
//! Implements the transition towards kSM_WaitingRun
//! If current state is kSM_Ready, then tries to go to nightlyOpen state first.
//!    @returns
//!        kSM_WaitingRun or kSM_BadFolder
int DataLogger::NightlyToWaitRun()
{
    int cState = GetCurrentState();

    if (cState == kSM_Ready)
        cState = Start();

    if (cState != kSM_NightlyOpen)
        return GetCurrentState();

    if (fDebugIsOn)
    {
        Debug("Going to Wait Run Number state...");    
    }
    return kSM_WaitingRun;    
}
// --------------------------------------------------------------------------
//
//! Implements the transition from wait for run number to nightly open
//! Does nothing really.
//!    @returns
//!        kSM_WaitingRun
int DataLogger::BackToNightlyOpen()
{
    if (GetCurrentState()==kSM_Logging)
        StopRunLogging();

    if (fDebugIsOn)
    {
        Debug("Going to NightlyOpen state...");
    }
    return kSM_NightlyOpen;
}
// --------------------------------------------------------------------------
//
//! Setup Logger's configuration from a Configuration object
//! @param conf the configuration object that should be used
//!
int DataLogger::EvalOptions(Configuration& conf)
{
    fDebugIsOn = conf.Get<bool>("debug");
    fFilesStats.SetDebugMode(fDebugIsOn);

    //Set the block or allow list
    fBlackList.clear();
    fWhiteList.clear();

    //Adding entries that should ALWAYS be ignored
    fBlackList.insert("DATA_LOGGER/MESSAGE");
    fBlackList.insert("DATA_LOGGER/SUBSCRIPTIONS");
    fBlackList.insert("/SERVICE_LIST");
    fBlackList.insert("DIS_DNS/");

    //set the black list, white list and the goruping
    const vector<string> vec1 = conf.Vec<string>("block");
    const vector<string> vec2 = conf.Vec<string>("allow");
    const vector<string> vec3 = conf.Vec<string>("group");

    fBlackList.insert(vec1.begin(), vec1.end());
    fWhiteList.insert(vec2.begin(), vec2.end());
    fGrouping.insert( vec3.begin(), vec3.end());

    //set the old run numbers timeout delay
    if (conf.Has("run-timeout"))
    {
        const uint32_t timeout = conf.Get<uint32_t>("run-timeout");
        if (timeout == 0)
        {
            Error("Time out delay for old run numbers must not be 0.");
            return 1;
        }
        fRunNumberTimeout = timeout;
    }

    //configure the run files directory
    if (conf.Has("destination-folder"))
     {
         const string folder = conf.Get<string>("destination-folder");
         if (!fFilesStats.SetCurrentFolder(folder))
             return 2;

         fFilePath = folder;
         fFullNightlyLogFileName = CompileFileNameWithPath(fFilePath, "", "log");
         if (!OpenTextFile(fNightlyLogFile, fFullNightlyLogFileName))
             return 3;

         fNightlyLogFile << endl;
         NotifyOpenedFile(fFullNightlyLogFileName, 1, fOpenedNightlyFiles);
         for (vector<string>::iterator it=backLogBuffer.begin();it!=backLogBuffer.end();it++)
             fNightlyLogFile << *it;
     }

    shouldBackLog = false;
    backLogBuffer.clear();

    //configure the interval between statistics updates
    if (conf.Has("stats-interval"))
        fFilesStats.SetUpdateInterval(conf.Get<int16_t>("stats-interval"));

    //configure if the filenames service is on or off
    fOpenedFilesIsOn = !conf.Get<bool>("no-filename-service");

    //configure if the number of subscriptions and fits files is on or off.
    fNumSubAndFitsIsOn = !conf.Get<bool>("no-numsubs-service");
    //should we open the daily files at startup ?
    if (conf.Has("start-daily-files"))
        if (conf.Get<bool>("start-daily-files"))
        {
            fShouldAutoStart = true;
        }
    if (conf.Has("service-list-interval"))
        fCurrentSubscriptionUpdateRate = conf.Get<int32_t>("service-list-interval");

    Info("Preset observatory: "+Nova::LnLatPosn::preset()+" [PRESET_OBSERVATORY]");

    return -1;
}


#include "Main.h"

// --------------------------------------------------------------------------
template<class T>
int RunShell(Configuration &conf)
{
    return Main::execute<T, DataLogger>(conf);//, true);
}

/*
 Extract usage clause(s) [if any] for SYNOPSIS.
 Translators: "Usage" and "or" here are patterns (regular expressions) which
 are used to match the usage synopsis in program output.  An example from cp
 (GNU coreutils) which contains both strings:
  Usage: cp [OPTION]... [-T] SOURCE DEST
    or:  cp [OPTION]... SOURCE... DIRECTORY
    or:  cp [OPTION]... -t DIRECTORY SOURCE...
 */
void PrintUsage()
{
    cout << "\n"
        "The data logger connects to all available Dim services and "
        "writes them to ascii and fits files.\n"
        "\n"
        "The default is that the program is started without user interaction. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usage can be brought to the screen.\n"
        "\n"
        "Usage: datalogger [-c type] [OPTIONS]\n"
        "  or:  datalogger [OPTIONS]\n";
    cout << endl;

}
// --------------------------------------------------------------------------
void PrintHelp()
{
    /* Additional help text which is printed after the configuration
     options goes here */
    cout <<
        "\n"
        "If the allow list has any element, only the servers and/or services "
        "specified in the list will be used for subscription. The black list "
        "will disable service subscription and has higher priority than the "
        "allow list. If the allow list is not present by default all services "
        "will be subscribed."
        "\n"
        "For example, block=DIS_DNS/ will skip all the services offered by "
        "the DIS_DNS server, while block=/SERVICE_LIST will skip all the "
        "SERVICE_LIST services offered by any server and DIS_DNS/SERVICE_LIST "
        "will skip DIS_DNS/SERVICE_LIST.\n"
        << endl;

    Main::PrintHelp<DataLogger>();
}

// --------------------------------------------------------------------------
void SetupConfiguration(Configuration &conf)
{
    po::options_description configs("DataLogger options");
    configs.add_options()
        ("block,b",             vars<string>(),  "Black-list to block services")
        ("allow,a",             vars<string>(),  "White-list to only allow certain services")
        ("debug,d",             po_bool(),       "Debug mode. Print clear text of received service reports.")
        ("group,g",             vars<string>(),  "Grouping of services into a single run-Fits")
        ("run-timeout",         var<uint32_t>(), "Time out delay for old run numbers in milliseconds.")
        ("destination-folder",  var<string>(),   "Base path for the nightly and run files")
        ("stats-interval",      var<int16_t>(),  "Interval in milliseconds for write statistics update")
        ("no-filename-service", po_bool(),       "Disable update of filename service")
        ("no-numsubs-service",  po_bool(),       "Disable update of number-of-subscriptions service")
        ("start-daily-files",   po_bool(),       "Starts the logger in DailyFileOpen instead of Ready")
        ("service-list-interval", var<int32_t>(), "Interval between two updates of the service SUBSCRIPTIONS")
        ;

    conf.AddOptions(configs);
}
// --------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    {
        // No console access at all
        if (!conf.Has("console"))
            return RunShell<LocalStream>(conf);

        // Console access w/ and w/o Dim
        if (conf.Get<int>("console")==0)
            return RunShell<LocalShell>(conf);
        else
            return RunShell<LocalConsole>(conf);
    }


    return 0;
}
