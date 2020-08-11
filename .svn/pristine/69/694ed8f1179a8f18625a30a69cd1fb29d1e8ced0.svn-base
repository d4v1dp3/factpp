#include <boost/array.hpp>

#include <string>    // std::string
#include <algorithm> // std::transform
#include <cctype>    // std::tolower

#include "FACT.h"
#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "ConnectionSSL.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Timers.h"
#include "Console.h"

#include "tools.h"

#include "HeadersTNGWeather.h"

#include <QtXml/QDomDocument>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace TNGWeather;


class ConnectionWeather : public ConnectionSSL
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

    virtual void UpdateWeather(const Time &, const DimWeather &)
    {
    }

    virtual void UpdateSeeing(const Time &, const DimSeeing &)
    {
    }

    virtual void UpdateDust(const Time &, const float &)
    {
    }

    string fRdfData;
    float  fDust;

protected:

    boost::array<char, 4096> fArray;

    Time fLastReport;
    Time fLastReception;

    Time fLastSeeing;

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host.");

            // 107: Transport endpoint is not connected (bs::error_code(107, bs::system_category))
            // 125: Operation canceled
            if (err && err!=ba::error::eof &&                     // Connection closed by remote host
                err!=ba::error::basic_errors::not_connected &&    // Connection closed by remote host
                err!=ba::error::basic_errors::operation_aborted)  // Connection closed by us
            {
                ostringstream str;
                str << "Reading from " << URL() << ": " << err.message() << " (" << err << ")";// << endl;
                Error(str);
            }
            PostClose(err!=ba::error::basic_errors::operation_aborted);

            fRdfData = "";
            return;
        }

        fRdfData += string(fArray.data(), bytes_received);

        const size_t end = fRdfData.find("\r\n\r\n");
        if (end==string::npos)
        {
            Out() << "Received data corrupted [1]." << endl;
            Out() << fRdfData << endl;
            return;
        }

        string data(fRdfData);
        data.erase(0, end+4);

        size_t pos = 0;
        while (1)
        {
            const size_t chunk = data.find("\r\n", pos);
            if (chunk==0 || chunk==string::npos)
            {
                StartReadReport();
                return;
            }

            size_t len = 0;
            stringstream val(data.substr(pos, chunk-pos));
            val >> hex >> len;

            data.erase(pos, chunk-pos+2);
            if (len==0)
                break;

            pos += len+2; // Count trailing \r\n of chunk
        }


        fLastReception = Time();
        fRdfData = "";
        PostClose(false);

        if (fIsVerbose)
        {
            Out() << "------------------------------------------------------" << endl;
            Out() << data << endl;
            Out() << "------------------------------------------------------" << endl;
        }

        QDomDocument doc;
        if (!doc.setContent(QString(data.data()), false))
        {
            Warn("Parsing of xml failed [0].");
            PostClose(false);
            return;
        }

        if (fIsVerbose)
            Out() << "Parsed:\n-------\n" << doc.toString().toStdString() << endl;

        const QDomElement root    = doc.documentElement();
        const QDomElement channel = root.firstChildElement("channel");
        const QDomElement item    = channel.firstChildElement("item");

        const QDomElement see   = item.firstChildElement("tngw:dimmSeeing");
        const QDomElement mjd   = item.firstChildElement("tngw:dimmSeeing.date");
        const QDomElement med   = item.firstChildElement("tngw:dimmSeeing.median");
        const QDomElement sdev  = item.firstChildElement("tngw:dimmSeeing.stdev");
        const QDomElement dust  = item.firstChildElement("tngw:dustTotal");
        const QDomElement trend = item.firstChildElement("tngw:trend");
        const QDomElement pres  = item.firstChildElement("tngw:airPressure");
        const QDomElement dew   = item.firstChildElement("tngw:dewPoint");
        const QDomElement wdir  = item.firstChildElement("tngw:windDirection");
        const QDomElement speed = item.firstChildElement("tngw:windSpeed");
        const QDomElement hum   = item.firstChildElement("tngw:hum");
        const QDomElement tmp   = item.firstChildElement("tngw:temperature");
        const QDomElement solar = item.firstChildElement("tngw:solarimeter");
        const QDomElement date  = item.firstChildElement("tngw:date");

        if (see.isNull()  || mjd.isNull()   || med.isNull()  || sdev.isNull() ||
            dust.isNull() || trend.isNull() || pres.isNull() || dew.isNull()  ||
            wdir.isNull() || speed.isNull() || hum.isNull()  || tmp.isNull()  ||
            solar.isNull()|| date.isNull())
        {
            Warn("Parsing of xml failed [1].");
            PostClose(false);
            return;
        }

        DimWeather w;
        w.fDustTotal     = dust .text().toFloat();
        w.fTempTrend     = trend.text().toFloat();
        w.fAirPressure   = pres .text().toFloat();
        w.fDewPoint      = dew  .text().toFloat();
        w.fWindDirection = wdir .text().toFloat();
        w.fWindSpeed     = speed.text().toFloat()*3.6;
        w.fHumidity      = hum  .text().toFloat();
        w.fTemperature   = tmp  .text().toFloat();
        w.fSolarimeter   = solar.text().toFloat();

        DimSeeing s;
        s.fSeeing        = see  .text().toFloat();
        s.fSeeingMed     = med  .text().toFloat();
        s.fSeeingStdev   = sdev .text().toFloat();

        const string dateObj = date.text().toStdString();
        const string dateSee = mjd .text().toStdString();

        Time timeObj(dateObj);
        Time timeSee(dateSee);
        if (!timeObj.IsValid())
        {
            struct tm tm;

            vector<char> buf(255);
            if (strptime(dateObj.c_str(), "%c", &tm))
                timeObj = Time(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                               tm.tm_hour,      tm.tm_min,   tm.tm_sec);
        }

        if (!timeSee.IsValid())
        {
            struct tm tm;

            vector<char> buf(255);
            if (strptime(dateSee.c_str(), "%c", &tm))
                timeSee = Time(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                               tm.tm_hour,      tm.tm_min,   tm.tm_sec);

            Warn("Seeing time invalid ["+dateObj+"]");
        }

        if (!timeObj.IsValid())
            throw runtime_error("object time invalid");

        if (timeObj!=fLastReport && fIsVerbose)
        {
            Out() << endl;
            Out() << "Date:           " << timeObj          << endl;
            Out() << "DustTotal:      " << w.fDustTotal     << " ugr/m^2" << endl;
            Out() << "AirPressure:    " << w.fAirPressure   << " mbar"    << endl;
            Out() << "DewPoint:       " << w.fDewPoint      << " deg C"   << endl;
            Out() << "WindDirection:  " << w.fWindDirection << " deg"     << endl;
            Out() << "WindSpeed:      " << w.fWindSpeed     << " m/s"     << endl;
            Out() << "Humidity:       " << w.fHumidity      << "%"        << endl;
            Out() << "Temperature:    " << w.fTemperature   << " deg C"   << endl;
            Out() << "TempTrend 24h:  " << w.fTempTrend     << " deg C"   << endl;
            Out() << "Solarimeter:    " << w.fSolarimeter   << " W/m^2"   << endl;
            Out() << endl;
            Out() << "Seeing:         " << s.fSeeing << " arcsec [" << timeSee << "]" << endl;
            Out() << "Seeing:         " << s.fSeeingMed << " +- " << s.fSeeingStdev << endl;
            Out() << endl;
        }

        fLastReport = timeObj;

        UpdateWeather(timeObj, w);

        if (timeSee.IsValid() && fLastSeeing!=timeSee)
        {
            UpdateSeeing(timeSee, s);
            fLastSeeing = timeSee;
        }

        if (fDust==w.fDustTotal)
            return;

        UpdateDust(timeObj, w.fDustTotal);
        fDust = w.fDustTotal;

        ostringstream out;
        out << setprecision(3) << "Dust: " << fDust << "ug/m^3 [" << timeObj << "]";
        Message(out);
    }

    void StartReadReport()
    {
        async_read_some(ba::buffer(fArray),
                        boost::bind(&ConnectionWeather::HandleRead, this,
                                    dummy::error, dummy::bytes_transferred));
    }

    boost::asio::deadline_timer fKeepAlive;

    void PostRequest()
    {
        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "User-Agent: FACT tngweather\r\n"
            "Accept: */*\r\n"
            "Host: "+URL()+"\r\n"
            "Connection: close\r\n"//Keep-Alive\r\n"
            "Content-Type: application/rss+xml\r\n"
            "Pragma: no-cache\r\n"
            "Cache-Control: no-cache\r\n"
            "Expires: 0\r\n"
            "Cache-Control: max-age=0\r\n"
            "\r\n";

        PostMessage(cmd);
    }

    void Request()
    {
        PostRequest();

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval));
        fKeepAlive.async_wait(boost::bind(&ConnectionWeather::HandleRequest,
                                          this, dummy::error));
    }

    void HandleRequest(const bs::error_code &error)
    {
        // 125: Operation canceled (bs::error_code(125, bs::system_category))
        if (error && error!=ba::error::basic_errors::operation_aborted)
        {
            ostringstream str;
            str << "Write timeout of " << URL() << ": " << error.message() << " (" << error << ")";// << endl;
            Error(str);

            PostClose(false);
            return;
        }

        if (IsClosed())
        {
            // For example: Here we could schedule a new accept if we
            // would not want to allow two connections at the same time.
            PostClose(true);
            return;
        }

        // Check whether the deadline has passed. We compare the deadline
        // against the current time since a new asynchronous operation
        // may have moved the deadline before this actor had a chance
        // to run.
        if (fKeepAlive.expires_at() > ba::deadline_timer::traits_type::now())
            return;

        Request();
    }


private:
    // This is called when a connection was established
    void ConnectionEstablished()
    {
        Request();
        StartReadReport();
    }

public:

    static const uint16_t kMaxAddr;

public:
    ConnectionWeather(ba::io_service& ioservice, MessageImp &imp) : ConnectionSSL(ioservice, imp()),
        fIsVerbose(true), fDust(-1),
        fLastReport(Time::none), fLastReception(Time::none), fLastSeeing(Time::none),
        fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
        ConnectionSSL::SetVerbose(b);
    }

    void SetInterval(uint16_t i)
    {
        fInterval = i;
    }

    void SetSite(const string &site)
    {
        fSite = site;
    }

    int GetState() const
    {
        if (fLastReport.IsValid() && fLastReport+boost::posix_time::seconds(fInterval*2)>Time())
            return 3; // receiving

        if (fLastReception.IsValid() && fLastReception+boost::posix_time::seconds(fInterval*2)>Time())
            return 2; // connected

        return 1; // Disconnected
    }
};

const uint16_t ConnectionWeather::kMaxAddr = 0xfff;

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimWeather : public ConnectionWeather
{
private:
    DimDescribedService fDimWeather;
    DimDescribedService fDimAtmosphere;
    DimDescribedService fDimSeeing;

    virtual void UpdateWeather(const Time &t, const DimWeather &data)
    {
        fDimWeather.setData(&data, sizeof(DimWeather));
        fDimWeather.Update(t);
    }

    virtual void UpdateDust(const Time &t, const float &dust)
    {
        fDimAtmosphere.setData(&dust, sizeof(float));
        fDimAtmosphere.Update(t);
    }

    virtual void UpdateSeeing(const Time &t, const DimSeeing &see)
    {
        fDimSeeing.setData(&see, sizeof(DimSeeing));
        fDimSeeing.Update(t);
    }

public:
    ConnectionDimWeather(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionWeather(ioservice, imp),
        fDimWeather("TNG_WEATHER/DATA", "F:1;F:1;F:1;F:1;F:1;F:1;F:1;F:1;F:1",
                     "|T[deg C]:Temperature"
                     "|DeltaT[deg C]:Temperature trend 24h"
                     "|T_dew[deg C]:Dew point"
                     "|H[%]:Humidity"
                     "|P[mbar]:Air pressure"
                     "|v[km/h]:Wind speed"
                     "|d[deg]:Wind direction (N-E)"
                     "|Dust[ug/m^3]:Dust (total)"
                     "|Solarimeter[W/m^2]:Solarimeter"),
        fDimAtmosphere("TNG_WEATHER/DUST", "F:1",
                       "|Dust[ug/m^3]:Dust (total)"),
        fDimSeeing("TNG_WEATHER/SEEING", "F:1;F:1;F:1",
                   "|Seeing[arcsec]:Seeing"
                   "|SeeingMed[arcsec]:Seeing Median"
                   "|SeeingStdev[arcsec]:Seeing Stdev")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineWeather : public StateMachineAsio<T>
{
private:
    S fWeather;

    bool CheckEventSize(size_t has, const char *name, size_t size)
    {
        if (has==size)
            return true;

        ostringstream msg;
        msg << name << " - Received event has " << has << " bytes, but expected " << size << ".";
        T::Fatal(msg);
        return false;
    }

    int SetVerbosity(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetVerbosity", 1))
            return T::kSM_FatalError;

        fWeather.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fWeather.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fWeather.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        poll();

        if (evt.GetBool())
            fWeather.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fWeather.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fWeather.GetState();
    }


public:
    StateMachineWeather(ostream &out=cout) :
        StateMachineAsio<T>(out, "TNG_WEATHER"), fWeather(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "NoConnection",
                     "No connection to web-server could be established recently");

        T::AddStateName(State::kConnected, "Invalid",
                     "Connection to webserver can be established, but received data is not recent or invalid");

        T::AddStateName(State::kReceiving, "Valid",
                     "Connection to webserver can be established, receint data received");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B")
            (bind(&StateMachineWeather::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineWeather::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineWeather::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        fWeather.SetVerbose(!conf.Get<bool>("quiet"));
        fWeather.SetInterval(conf.Get<uint16_t>("interval"));
        fWeather.SetDebugTx(conf.Get<bool>("debug-tx"));
        fWeather.SetSite(conf.Get<string>("url"));
        fWeather.SetEndpoint(conf.Get<string>("addr"));
        fWeather.StartConnect();

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineWeather<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("TNG weather control options");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("tngweb.tng.iac.es:443"),  "Network address of Cosy")
        ("url,u",  var<string>("/api/meteo/weather/feed.xml"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(300), "Interval between two updates on the server in seconds")
        ("debug-tx", po_bool(), "Enable debugging of ethernet transmission.")
        ;

    conf.AddOptions(control);
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
    cout <<
        "The tngweather is an interface to the TNG weather data.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: tngweather [-c type] [OPTIONS]\n"
        "  or:  tngweather [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
//    Main::PrintHelp<StateMachineFTM<StateMachine, ConnectionFTM>>();

    /* Additional help text which is printed after the configuration
     options goes here */

    /*
     cout << "bla bla bla" << endl << endl;
     cout << endl;
     cout << "Environment:" << endl;
     cout << "environment" << endl;
     cout << endl;
     cout << "Examples:" << endl;
     cout << "test exam" << endl;
     cout << endl;
     cout << "Files:" << endl;
     cout << "files" << endl;
     cout << endl;
     */
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    //try
    {
        // No console access at all
        if (!conf.Has("console"))
        {
            if (conf.Get<bool>("no-dim"))
                return RunShell<LocalStream, StateMachine, ConnectionWeather>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimWeather>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionWeather>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionWeather>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimWeather>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimWeather>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
