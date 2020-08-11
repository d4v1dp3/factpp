#include <boost/array.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <string>    // std::string
#include <algorithm> // std::transform
#include <cctype>    // std::tolower

#include "FACT.h"
#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Console.h"

#include "tools.h"

#include "HeadersGude.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace pt = boost::property_tree;
namespace dummy = ba::placeholders;

using namespace std;
using namespace Gude;

// ------------------------------------------------------------------------

class ConnectionGude : public Connection
{
    uint16_t fInterval;

    bool fIsVerbose;

    string fSite;

protected:

    Time fLastReport;
    Time fLastReception;

    boost::asio::streambuf fBuffer;
    string fData;

    array<float, 12*21> fReadings;
    array<string, 12>   fSensors;
    array<string, 21>   fUnits;

    virtual void UpdateGude(const array<float, 12*21> &)
    {
    }

    void ProcessAnswer(string s)
    {
        try
        {
            std::stringstream ss;
            ss << s;

            pt::ptree tree;
            pt::read_json(ss, tree);

            const auto &sensor_values = tree.get_child("sensor_values");

            for (const std::pair<std::string, pt::ptree> &obj : sensor_values)
            {
                switch (obj.second.get_child("type").get_value<uint32_t>())
                {
                case 8:

                    {
                        const auto &values = obj.second.get_child("values");

                        int isensor = 0;
                        for (const std::pair<std::string, pt::ptree> &sensor : values)
                        {
                            if (isensor==12)
                                throw runtime_error("Values from more sensors than expected!");

                            int ivalue = 0;
                            for (const std::pair<std::string, pt::ptree> &value : sensor.second)
                            {
                                if (ivalue==21)
                                    throw runtime_error("More values than expected!");

                                //std::stringstream ss2;
                                //pt::write_json(ss2, value.second.get_child("v").get_value<float>());

                                const auto idx = ivalue*12+isensor;
                                fReadings[idx] = value.second.get_child("v").get_value<float>();

                                ivalue++;
                            }
                            isensor++;
                        }
                    }
                default:
                    continue;

                }
            }



            const auto &outputs = tree.get_child("outputs");

            int isensor = 0;
            for (const std::pair<std::string, pt::ptree> &obj : outputs)
            {
                if (isensor==12)
                    throw runtime_error("More outputs than expected!");

                const string name = obj.second.get_child("name").get_value<string>();

                if (name!=fSensors[isensor])
                    Info("New name for sensor "+to_string(isensor)+": "+name);

                fSensors[isensor++] = name;
            }



            const auto &sensor_descr = tree.get_child("sensor_descr");

            for (const std::pair<std::string, pt::ptree> &obj : sensor_descr)
            {
                switch (obj.second.get_child("type").get_value<uint32_t>())
                {
                case 8:
                    {
                        const auto &fields = obj.second.get_child("fields");

                        int ifield = 0;
                        for (const std::pair<std::string, pt::ptree> &field : fields)
                        {
                            if (ifield==21)
                                throw runtime_error("More fields than expected!");

                            const string name = field.second.get_child("name").get_value<string>();
                            const string unit = field.second.get_child("unit").get_value<string>();

                            fUnits[ifield] = name;
                            if (!unit.empty())
                                fUnits[ifield] += " [" + unit + "]";

                            ifield++;
                        }
                    }
                default:
                    continue;

                }
            }

            UpdateGude(fReadings);

            fLastReport = Time();
        }
        catch (std::exception const& e)
        {
            Error(string("Parsing JSON failed: ")+e.what());
        }
    }

    void HandleRead(const boost::system::error_code& err, size_t bytes_received)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
            {
                // Does the message contain a header?
                const size_t p1 = fData.find("\r\n\r\n");
                if (p1!=string::npos)
                    ProcessAnswer(fData.substr(p1));
                else
                    Warn("Received message lacks a header!");
                fData = "";

                PostClose(false);

                return;
            }

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
            return;
        }

        fLastReception = Time();

        istream is(&fBuffer);

        string buffer;
        if (!getline(is, buffer, '\n'))
        {
            Fatal("Received message does not contain \\n... closing connection.");
            PostClose(false);
            return;
        }

        if (fIsVerbose)
            Out() << buffer << endl;

        fData += buffer;
        fData += '\n';

        StartReadLine();
    }

    void StartReadLine()
    {
        async_read_until(*this, fBuffer, '\n',
                             boost::bind(&ConnectionGude::HandleRead, this,
                                         dummy::error, dummy::bytes_transferred));
    }

    ba::deadline_timer fKeepAlive;

    void PostRequest()
    {
        const string cmd =
            "GET "+fSite+" HTTP/1.1\r\n"
            "\r\n";

        if (GetDebugTx())
            Out() << cmd << endl;

        PostMessage(cmd);
    }

    void Request()
    {
        PostRequest();

        fKeepAlive.expires_from_now(boost::posix_time::seconds(fInterval/2));
        fKeepAlive.async_wait(boost::bind(&ConnectionGude::HandleRequest,
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

        if (!is_open())
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
        StartReadLine();
    }

public:
    ConnectionGude(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(true), fLastReport(Time::none), fLastReception(Time::none), fKeepAlive(ioservice)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
        Connection::SetVerbose(b);
    }

    void SetInterval(uint16_t i)
    {
        fInterval = i;
    }

    void SetSite(const string &site)
    {
        fSite = site;
    }

    void Print(bool all=true)
    {
        for (int k=0; k<(all?21:6); k+=3)
        {
            Out() << setw(17) << ' ';
            for (int i=k; i<k+3; i++)
                Out() << setw(30) << fUnits[i];
            Out() << endl;

            for (int i=0; i<12; i++)
            {
                Out() << setw(17) << fSensors[i];
                for (int j=k; j<k+3; j++)
                    Out() << setw(30) << fReadings[i+j*12];
                Out() << endl;
            }

            Out() << endl;
        }
    }

    int GetState() const
    {
        if (fLastReport.IsValid() && fLastReport+boost::posix_time::seconds(fInterval*2)>Time())
            return 3;

        if (fLastReception.IsValid() && fLastReception+boost::posix_time::seconds(fInterval*2)>Time())
            return 2;

        return 1;

    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimGude : public ConnectionGude
{
private:

    DimDescribedService fDimGude;

    virtual void UpdateGude(const array<float, 12*21> &data)
    {
        fDimGude.setData(data.data(), sizeof(float)*data.size());
        fDimGude.Update();
    }

public:
    ConnectionDimGude(ba::io_service& ioservice, MessageImp &imp) :
        ConnectionGude(ioservice, imp),
        fDimGude("GUDE_CONTROL/DATA",
                 "F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12;F:12",
                 "|Voltage[V]"
                 "|Current[A]"
                 "|Frequency[Hz]"
                 "|PhaseIU[degree]"
                 "|ActivePower[W]"
                 "|ReactivePower[VAR]"
                 "|ApparentPower[VA]"
                 "|Powerfactor"
                 "|AbsActEnergyNonRes[kWh]"
                 "|AbsReactEnergyNonRes[kVARh]"
                 "|AbsActEnergyRes[kWh]"
                 "|AbsReactEnergyRes[kVARh]"
                 "|RelativeTime[s]"
                 "|FwdActEnergyNonRes[kWh]"
                 "|FwdReactEnergyNonRes[kVARh]"
                 "|FwdActEnergyRes[kWh]"
                 "|FwdReactEnergyRes[kVARh]"
                 "|RevActEnergyNonRes[kWh]"
                 "|RevReactEnergyNonRes[kVARh]"
                 "|RevActEnergyRes[kWh]"
                 "|RevReactEnergyRes[kVARh]")
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineGude : public StateMachineAsio<T>
{
private:
    S fGude;

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

        fGude.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int Print()
    {
        fGude.Print(false);
        return T::GetCurrentState();
    }

    int PrintAll()
    {
        fGude.Print(true);
        return T::GetCurrentState();
    }
/*
    int Disconnect()
    {
        // Close all connections
        fGude.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fGude.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fGude.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fGude.PostClose(true);

        return T::GetCurrentState();
    }
*/
    int Execute()
    {
        return fGude.GetState();
    }

public:
    StateMachineGude(ostream &out=cout) :
        StateMachineAsio<T>(out, "GUDE_CONTROL"), fGude(*this, *this)
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
            (bind(&StateMachineGude::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");

        T::AddEvent("PRINT", "")
            (bind(&StateMachineGude::Print, this))
            ("Print last readings");

        T::AddEvent("PRINT_ALL", "")
            (bind(&StateMachineGude::PrintAll, this))
            ("Print all readings");
/*
        // Conenction commands
        AddEvent("DISCONNECT")
            (bind(&StateMachineGude::Disconnect, this))
            ("disconnect from ethernet");

        AddEvent("RECONNECT", "O")
            (bind(&StateMachineGude::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");
*/
    }

    int EvalOptions(Configuration &conf)
    {
        fGude.SetVerbose(!conf.Get<bool>("quiet"));
        fGude.SetInterval(conf.Get<uint16_t>("interval"));
        fGude.SetDebugTx(conf.Get<bool>("debug-tx"));
        fGude.SetSite(conf.Get<string>("url"));
        fGude.SetEndpoint(conf.Get<string>("addr"));
        fGude.StartConnect();

        return -1;
    }
};



// ------------------------------------------------------------------------

#include "Main.h"


template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineGude<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Bias Crate temperature readout");
    control.add_options()
        ("no-dim,d",  po_switch(),    "Disable dim services")
        ("addr,a",  var<string>("10.0.100.238:80"),  "Network address of the hardware")
        ("url,u",  var<string>("/statusjsn.js?components=81921"),  "File name and path to load")
        ("quiet,q", po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("interval,i", var<uint16_t>(5), "Interval between two updates on the server in seconds")
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
        "The gudectrl is an interface to the Gude sensors.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: gudectrl [-c type] [OPTIONS]\n"
        "  or:  gudectrl [OPTIONS]\n";
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
                return RunShell<LocalStream, StateMachine, ConnectionGude>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimGude>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionGude>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionGude>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimGude>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimGude>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
