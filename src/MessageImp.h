#ifndef FACT_MessageImp
#define FACT_MessageImp

#include <string>
#include <sstream>
#include <iostream>

class Time;

class MessageImp
{
public:
    /// Severity of a message
    enum Severity
    {
        kMessage = 10, ///< Just a message, usually obsolete
        kInfo    = 20, ///< An info telling something which can be interesting to know
        kWarn    = 30, ///< A warning, things that somehow might result in unexpected or unwanted bahaviour
        kError   = 40, ///< Error, something unexpected happened, but can still be handled by the program
        kAlarm   = 45, ///< Error, something unexpected happened, but needs user intervention (i.e. it needs a signal to the user)
        kFatal   = 50, ///< An error which cannot be handled at all happend, the only solution is program termination
        kComment = 90, ///< A comment which is always printed
        kDebug   = 99, ///< A message used for debugging only
    };

private:
    std::ostream &fOut; /// The ostream to which by default Write redirects its output
    uint32_t fLastMjd;  /// Mjd of last message

    int WriteImp(const Time &time, const std::string &txt, int qos=kMessage);

public:
    MessageImp(std::ostream &out=std::cout);
    virtual ~MessageImp() { }

    virtual void IndicateStateChange(const Time &, const std::string &) { }
    void StateChanged(const Time &time, const std::string &server, const std::string &msg, int state);
    virtual int Write(const Time &time, const std::string &txt, int qos=kMessage);

    int Update(const std::string &txt, int severity=kMessage);
    int Update(const char *txt, int severity=kMessage) { return Update(std::string(txt), severity); }
    int Update(const std::ostringstream &str, int severity=kMessage) { return Update(str.str(), severity); }
//    int Update(int qos, const char *fmt, ...);

    int Debug(const std::string &str)    { return Update(str, kDebug);   }
    int Message(const std::string &str)  { return Update(str, kMessage); }
    int Info(const std::string &str)     { return Update(str, kInfo);    }
    int Warn(const std::string &str)     { return Update(str, kWarn);    }
    int Error(const std::string &str)    { return Update(str, kError);   }
    int Alarm(const std::string &str)    { return Update(str, kAlarm);   }
    int Fatal(const std::string &str)    { return Update(str, kFatal);   }
    int Comment(const std::string &str)  { return Update(str, kComment); }

    int Debug(const char *txt)   { return Debug(std::string(txt));   }
    int Message(const char *txt) { return Message(std::string(txt)); }
    int Info(const char *txt)    { return Info(std::string(txt));    }
    int Warn(const char *txt)    { return Warn(std::string(txt));    }
    int Error(const char *txt)   { return Error(std::string(txt));   }
    int Alarm(const char *txt)   { return Alarm(std::string(txt));   }
    int Fatal(const char *txt)   { return Fatal(std::string(txt));   }
    int Comment(const char *txt) { return Comment(std::string(txt)); }

    int Debug(const std::ostringstream &str)   { return Debug(str.str());   }
    int Message(const std::ostringstream &str) { return Message(str.str()); }
    int Info(const std::ostringstream &str)    { return Info(str.str());    }
    int Warn(const std::ostringstream &str)    { return Warn(str.str());    }
    int Alarm(const std::ostringstream &str)   { return Alarm(str.str());   }
    int Error(const std::ostringstream &str)   { return Error(str.str());   }
    int Fatal(const std::ostringstream &str)   { return Fatal(str.str());   }
    int Comment(const std::ostringstream &str) { return Comment(str.str()); }

    std::ostream &operator()() const { return fOut; }
    std::ostream &Out() const { return fOut; }

    virtual bool MessageQueueEmpty() const { return true; }
};

#endif

// ***************************************************************************
/** @fn MessageImp::IndicateStateChange(const Time &time, const std::string &server)

This function is called to indicate a state change by StateChanged() to
derived classes.

@param time
   Time at which the state change happened

@param server
   Server which emitted the state change

**/
// ***************************************************************************
