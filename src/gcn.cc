#include <functional>
#include <boost/algorithm/string/join.hpp>

#include "Database.h"

#include "Dim.h"
#include "Event.h"
#include "Shell.h"
#include "StateMachineDim.h"
#include "StateMachineAsio.h"
#include "Connection.h"
#include "LocalControl.h"
#include "Configuration.h"
#include "Console.h"
#include "Converter.h"

#include "tools.h"
#include "../externals/nova.h"

#include "HeadersGCN.h"
#include "HeadersToO.h"

#include <QtXml/QDomDocument>

namespace ba    = boost::asio;
namespace bs    = boost::system;
namespace dummy = ba::placeholders;

using namespace std;
using namespace GCN;

// ------------------------------------------------------------------------

class ConnectionGCN : public Connection
{
private:
    map<uint16_t, GCN::PaketType_t> fTypes;

    vector<string> fEndPoints;
    int fEndPoint;

    bool fIsVerbose;
    bool fDebugRx;

    string fUri;

    uint32_t     fRxSize;
    vector<char> fRxData;

    Time fLastKeepAlive;

    QString GetParamValue(const QDomElement &what, const string &name)
    {
        const QDomNodeList param = what.elementsByTagName("Param");
        for (int i=0; i<param.count(); i++)
        {
            const QDomElement elem = param.at(i).toElement();
            if (elem.attribute("name").toStdString()==name)
                return elem.attribute("value");
        }

        return "";
    }

    GCN::PaketPtr GetType(const QDomElement &what)
    {
        const auto value = GetParamValue(what, "Packet_Type");
        if (value.isEmpty())
            return GCN::PaketTypes.end();

        const uint16_t val = value.toUInt();

        const auto it = GCN::PaketTypes.find(val);

        if (it==GCN::PaketTypes.end())
            Warn("Unknown paket type "+to_string(val)+".");

        return it;

    }

    int ProcessXml(const QDomElement &root)
    {
        if (root.isNull())
            return -255;

        const string role = root.attribute("role", "").toStdString();
        const string trn  = root.tagName().toStdString();

        // A full description can be found at http://voevent.dc3.com/schema/default.html

        if (trn=="trn:Transport")
        {
            if (role=="iamalive")
            {
                const QDomElement orig = root.firstChildElement("Origin");
                const QDomElement time = root.firstChildElement("TimeStamp");
                if (orig.isNull() || time.isNull())
                    return -254;

                fLastKeepAlive = Time(time.text().toStdString());

                if (fIsVerbose)
                {
                    Out() << Time().GetAsStr() << " ----- " << trn << " [" << role << "] -----" << endl;
                    Out() << " " << time.tagName().toStdString() << " = " << fLastKeepAlive.GetAsStr() << '\n';
                    Out() << " " << orig.tagName().toStdString() << " = " << orig.text().toStdString() << '\n';
                    Out() << endl;
                }

                return true;
            }

            return false;
        }

        ofstream fout("gcn.stream", ios::app);
        fout << "------------------------------------------------------------------------------\n" << fRxData.data() << endl;

        if (trn=="voe:VOEvent")
        {
            // WHAT: http://gcn.gsfc.nasa.gov/tech_describe.html
            const QDomElement who  = root.firstChildElement("Who");
            const QDomElement what = root.firstChildElement("What");
            const QDomElement when = root.firstChildElement("WhereWhen");
            //const QDomElement how  = root.firstChildElement("How");
            //const QDomElement why  = root.firstChildElement("Why");
            //const QDomElement cite = root.firstChildElement("Citations");
            //const QDomElement desc = root.firstChildElement("Description");
            //const QDomElement ref  = root.firstChildElement("Reference");
            if (who.isNull() || what.isNull() || when.isNull())
                return -253;

            const auto ptype = GetType(what);

            const QDomElement date   = who.firstChildElement("Date");
            const QDomElement author = who.firstChildElement("Author");
            const QDomElement sname  = author.firstChildElement("shortName");
            const QDomElement desc   = what.firstChildElement("Description");

            const QDomElement obsdat = when.firstChildElement("ObsDataLocation");
            const QDomElement obsloc = obsdat.firstChildElement("ObservationLocation");
            const QDomElement coord  = obsloc.firstChildElement("AstroCoords");

            const QDomElement time   = coord.firstChildElement("Time").firstChildElement("TimeInstant").firstChildElement("ISOTime");
            const QDomElement pos2d  = coord.firstChildElement("Position2D");
            const QDomElement name1  = pos2d.firstChildElement("Name1");
            const QDomElement name2  = pos2d.firstChildElement("Name2");
            const QDomElement val2   = pos2d.firstChildElement("Value2");
            const QDomElement c1     = val2.firstChildElement("C1");
            const QDomElement c2     = val2.firstChildElement("C2");
            const QDomElement errad  = pos2d.firstChildElement("Error2Radius");

            const auto &id = ptype->first;

            if (id==0)
            {
                Warn("Packet ID = 0");
                return -252;
            }

            // Gravitational wave event
            const bool is_gw = id==150 || id==151 || id==152 || id==153 || id==164;

            // No shortName
            const bool no_sn = id==171 || id==173 || id==174;

            // Required keywords
            vector<string> missing;
            if (date.isNull())
                missing.emplace_back("Date");
            if (author.isNull())
                missing.emplace_back("Author");
            if (sname.isNull() && !is_gw && !no_sn)
                missing.emplace_back("shortName");
            if (obsdat.isNull())
                missing.emplace_back("ObsDataLocation");
            if (obsloc.isNull())
                missing.emplace_back("ObservationLocation");
            if (coord.isNull())
                missing.emplace_back("AstroCoords");
            if (time.isNull())
                missing.emplace_back("Time/TimeInstant/ISOTime");
            if (pos2d.isNull() && !is_gw)
                missing.emplace_back("Position2D");
            if (name1.isNull() && !is_gw)
                missing.emplace_back("Name1");
            if (name1.isNull() && !is_gw)
                missing.emplace_back("Name2");
            if (val2.isNull() && !is_gw)
                missing.emplace_back("Value2");
            if (c1.isNull() && !is_gw)
                missing.emplace_back("C1");
            if (c2.isNull() && !is_gw)
                missing.emplace_back("C2");
            if (errad.isNull() && !is_gw)
                missing.emplace_back("Error2Radius");

            if (!missing.empty())
            {
                Warn("Missing elements: "+boost::algorithm::join(missing, ", "));
                return -id;
            }

            //  59/31: Konus LC / IPN raw         [observation]
            //   110:  Fermi GBM (ART)            [observation]  (Initial)       // Stop data taking
            //   111:  Fermi GBM (FLT)            [observation]  (after ~2s)     // Start pointing/run
            //   112:  Fermi GBM (GND)            [observation]  (after 2-20s)   // Refine pointing
            //   115:  Fermi GBM position         [observation]  (final ~hours)
            //
            //    51:  Intergal pointdir              [utility]
            //    83:  Swift pointdir                 [utility]
            //   129:  Fermi pointdir                 [utility]
            //
            //     2:  Test coord             (      1)  [test]
            //    44:  HETE test              ( 41- 43)  [test]
            //    52:  Integral SPIACS                   [test]
            //    53:  Integral Wakeup                   [test]
            //    54:  Integral refined                  [test]
            //    55:  Integral Offline                  [test]
            //    56:  Integral Weak                     [test]
            //    82:  BAT   GRB pos test     (     61)  [test]
            //   109:  AGILE GRB pos test     (100-103)  [test]
            //   119:  Fermi GRB pos test     (111-113)  [test]
            //   124:  Fermi LAT pos upd test (120-122)  [test]
            //   136:  MAXI coord test        (    134)  [test]
            //
            // Integral: RA=1.2343, Dec=2.3456
            //

            /*
             54
             ==
             <Group name="Test_mpos" >
               <Param name="Test_Notice"              value="true" />
             </Group>


             82
             ==
             <Group name="Solution_Status" >
               <Param name="Test_Submission"          value="false" />
             </Group>


             115
             ===
             2013-07-20 19:04:13: TIME = 2013-07-20 02:46:40

             <Group name="Trigger_ID" >
               <Param name="Test_Submission"       value="false" />
             </Group>
             */

            // ----------------------------- Create Name ----------------------

            const auto &paket = ptype->second;

            string name;
            bool prefix = true;

            switch (id)
            {
            // case 60:
            // case 61:
            // case 62:
            // 
            // case 110:
            // case 111:
            // case 112:
            // case 115:
            //     name = GetParamValue(what, "TRIGGER_NUM").toStdString();
            //     break;
            // 
            // case 123:
            //     name = GetParamValue(what, "REF_NUM").toStdString();
            //     break;

            case 125:
                name = GetParamValue(what, "SourceName").toStdString();
                prefix = false;
                break;

            case 140:
                name = GetParamValue(what, "TrigID").toStdString();
                if (name[0]=='-')
                    name.erase(name.begin());
                break;

            case 157:
            case 158:
                {
                    const string event_id = GetParamValue(what, "event_id").toStdString();
                    const string run_id   = GetParamValue(what, "run_id").toStdString();
                    name = event_id+"_"+run_id;
                    break;
                }

            case 171:
            case 173:
            case 174:
                {
                    const string run_id   = GetParamValue(what, "run_id").toStdString();
                    const string event_id = GetParamValue(what, "event_id").toStdString();
                    name = run_id+"_"+event_id;
                    break;
                }

                // Missing ID
                // case 51:
                // case 83:
                // case 119:
                // case 129:
            default:
                name = GetParamValue(what, "TrigID").toStdString();
            }

            if (name.empty() || name=="0")
            {
                Warn("Missing ID... cannot create default source name... using date instead.");
                name = Time().GetAsStr("%Y%m%d_%H%M%S");
                prefix = true;
            }

            if (prefix)
                name.insert(0, paket.instrument+"#");

            // ----------------------------------------------------------------

            const string unit = pos2d.attribute("unit").toStdString();

            const double ra  = c1.text().toDouble();
            const double dec = c2.text().toDouble();
            const double err = errad.text().toDouble();

            const string n1 = name1.text().toStdString();
            const string n2 = name2.text().toStdString();

            const bool has_coordinates = n1=="RA" && n2=="Dec" && unit=="deg" && ra!=0 && dec!=0;

            const std::set<int16_t> typelist =
            {
                51,  // INTEGRAL_POINTDIR

                53,  // INTEGRAL_WAKEUP
                54,  // INTEGRAL_REFINED
                55,  // INTEGRAL_OFFLINE

                // 56, // INTEGRAL_WEAK
                // 59, // KONUS_LC

                60,  // SWIFT_BAT_GRB_ALERT
                61,  // SWIFT_BAT_GRB_POS_ACK
                62,  // SWIFT_BAT_GRB_POS_NACK

                83,  // SWIFT_POINTDIR

                97,  // SWIFT_BAT_QL_POS

                100, // AGILE_GRB_WAKEUP
                101, // AGILE_GRB_GROUND
                102, // AGILE_GRB_REFINED

                // 110, // FERMI_GBM_ALERT [0/0]
                111, // FERMI_GBM_FLT_POS
                112, // FERMI_GBM_GND_POS
                115, // FERMI_GBM_FIN_POS

                123, // FERMI_LAT_TRANS
                125, // FERMI_LAT_MONITOR

                // 134, // MAXI_UNKNOWN
                // 135, // MAXI_KNOWN
                // 136, // MAXI_TEST

                157, // AMON_ICECUBE_COINC
                158, // AMON_ICECUBE_HESE

                169, // AMON_ICECUBE_EHE
                171, // HAWC_BURST_MONITOR
                173, // ICECUBE_GOLD
                174, // ICECUBE_BRONZE
            };

            const bool integral_test = role=="test" && id>=53 && id<=56;

            const bool valid_id = typelist.find(id)!=typelist.end();

            if (valid_id && has_coordinates && !integral_test)
            {
                const ToO::DataGRB data =
                {
                    .type   = id,
                    .ra     = ra,
                    .dec    = dec,
                    .err    = err,
                };

                vector<char> dim(sizeof(ToO::DataGRB) + name.size() + 1);

                memcpy(dim.data(),                      &data,        sizeof(ToO::DataGRB));
                memcpy(dim.data()+sizeof(ToO::DataGRB), name.c_str(), name.size());

                Dim::SendCommandNB("SCHEDULER/GCN", dim);
                Info("Sent ToO '"+name+"' ["+role+"]");
            }

            Out() << Time(date.text().toStdString()).GetAsStr() << " ----- " << sname.text().toStdString() << " [" << role << "]\n";
            if (!desc.isNull())
                Out() << "[" << desc.text().toStdString()  << "]\n";
            Out() << name << " [" << paket.name << ":" << id << "]: " << paket.description << endl;
            Out() << left;
            Out() << "  " << setw(5) << "TIME" << "= " << Time(time.text().toStdString()).GetAsStr() << '\n';
            Out() << "  " << setw(5) << n1     << "= " << ra  << unit << '\n';
            Out() << "  " << setw(5) << n2     << "= " << dec << unit << '\n';
            Out() << "  " << setw(5) << "ERR"  << "= " << err << unit << '\n';
            Out() << endl;

            if (role=="observation")
            {
                return true;
            }

            if (role=="test")
            {
                return true;
            }

            if (role=="retraction")
            {
                return true;
            }

            if (role=="utility")
            {
                return true;
            }

            return false;
        }

        Out() << Time().GetAsStr() << " ----- " << trn << " [" << role << "] -----" << endl;

        return false;
    }

    void HandleReceivedData(const bs::error_code& err, size_t bytes_received, int type)
    {
        // Do not schedule a new read if the connection failed.
        if (bytes_received==0 || err)
        {
            if (err==ba::error::eof)
                Warn("Connection closed by remote host (GCN).");

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

        if (type==0)
        {
            fRxSize = ntohl(fRxSize);
            fRxData.assign(fRxSize+1, 0);
            ba::async_read(*this, ba::buffer(fRxData, fRxSize),
                             boost::bind(&ConnectionGCN::HandleReceivedData, this,
                                         dummy::error, dummy::bytes_transferred, 1));
            return;
        }

        if (fDebugRx)
        {
            Out() << "------------------------------------------------------\n";
            Out() << fRxData.data() << '\n';
            Out() << "------------------------------------------------------" << endl;
        }

        QDomDocument doc;
        if (!doc.setContent(QString(fRxData.data()), false))
        {
            Warn("Parsing of xml failed [0].");
            Out() << "------------------------------------------------------\n";
            Out() << fRxData.data() << '\n';
            Out() << "------------------------------------------------------" << endl;
            PostClose(true);
            return;
        }

        if (fDebugRx)
            Out() << "Parsed:\n-------\n" << doc.toString().toStdString() << endl;

        const int rc = ProcessXml(doc.documentElement());
        if (rc<0)
        {
            Warn("Parsing of xml failed [1].");
            Out() << "------------------------------------------------------\n";
            Out() << fRxData.data() << '\n';
            Out() << "------------------------------------------------------" << endl;
            //PostClose(false);

            try
            {
                auto db = Database(fUri);

                // Make an entry in the alter databse to issue a call
                const string query =
                    "INSERT FlareAlerts.FlareTriggers SET\n"
                    " fTriggerInserted=Now(),\n"
                    " fNight="+to_string(Time().NightAsInt())+",\n"
                    " fPacketTypeKey="+to_string(-rc)+",\n"
                    " fTriggerType=8";

                auto q = db.query(query);
                q.execute();
                if (!q.info().empty())
                    Info(q.info());
            }
            catch (const mysqlpp::ConnectionFailed &e)
            {
                Error(e.what());
            }
        }

        if (!rc)
        {
            Out() << "------------------------------------------------------\n";
            Out() << doc.toString().toStdString() << '\n';
            Out() << "------------------------------------------------------" << endl;
        }

        StartRead();
    }

    void StartRead()
    {
        ba::async_read(*this, ba::buffer(&fRxSize, 4),
                       boost::bind(&ConnectionGCN::HandleReceivedData, this,
                                   dummy::error, dummy::bytes_transferred, 0));
    }

    // This is called when a connection was established
    void ConnectionEstablished()
    {
        StartRead();
    }

public:
    ConnectionGCN(ba::io_service& ioservice, MessageImp &imp) : Connection(ioservice, imp()),
        fIsVerbose(false), fDebugRx(false), fLastKeepAlive(Time::none)
    {
        SetLogStream(&imp);
    }

    void SetVerbose(bool b)
    {
        fIsVerbose = b;
    }

    void SetDebugRx(bool b)
    {
        fDebugRx = b;
        Connection::SetVerbose(b);
    }

    void SetURI(const string &uri)
    {
        fUri = uri;
    }

    void SetEndPoints(const vector<string> &v)
    {
        fEndPoints = v;
        fEndPoint = 0;
    }

    void StartConnect()
    {
        if (fEndPoints.size()>0)
            SetEndpoint(fEndPoints[fEndPoint++%fEndPoints.size()]);
        Connection::StartConnect();
    }

    bool IsValid()
    {
        return fLastKeepAlive.IsValid() ? Time()-fLastKeepAlive<boost::posix_time::minutes(2) : false;
    }
};

// ------------------------------------------------------------------------

#include "DimDescriptionService.h"

class ConnectionDimGCN : public ConnectionGCN
{
private:

public:
    ConnectionDimGCN(ba::io_service& ioservice, MessageImp &imp) : ConnectionGCN(ioservice, imp)
    {
    }
};

// ------------------------------------------------------------------------

template <class T, class S>
class StateMachineGCN : public StateMachineAsio<T>
{
private:
    S fGCN;

    int Disconnect()
    {
        // Close all connections
        fGCN.PostClose(false);

        return T::GetCurrentState();
    }

    int Reconnect(const EventImp &evt)
    {
        // Close all connections to supress the warning in SetEndpoint
        fGCN.PostClose(false);

        // Now wait until all connection have been closed and
        // all pending handlers have been processed
        ba::io_service::poll();

        if (evt.GetBool())
            fGCN.SetEndpoint(evt.GetString());

        // Now we can reopen the connection
        fGCN.PostClose(true);

        return T::GetCurrentState();
    }

    int Execute()
    {
        if (!fGCN.IsConnected())
            return State::kDisconnected;

        return fGCN.IsValid() ? State::kValid : State::kConnected;
    }

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

        fGCN.SetVerbose(evt.GetBool());

        return T::GetCurrentState();
    }

    int SetDebugRx(const EventImp &evt)
    {
        if (!CheckEventSize(evt.GetSize(), "SetDebugRx", 1))
            return T::kSM_FatalError;

        fGCN.SetDebugRx(evt.GetBool());

        return T::GetCurrentState();
    }

public:
    StateMachineGCN(ostream &out=cout) :
        StateMachineAsio<T>(out, "GCN"), fGCN(*this, *this)
    {
        // State names
        T::AddStateName(State::kDisconnected, "Disconnected",
                     "No connection to GCN.");
        T::AddStateName(State::kConnected, "Connected",
                     "Connection to GCN established.");
        T::AddStateName(State::kValid, "Valid",
                     "Connection valid (keep alive received within past 2min)");

        // Verbosity commands
        T::AddEvent("SET_VERBOSE", "B:1")
            (bind(&StateMachineGCN::SetVerbosity, this, placeholders::_1))
            ("set verbosity state"
             "|verbosity[bool]:disable or enable verbosity for received data (yes/no), except dynamic data");
        T::AddEvent("SET_DEBUG_RX", "B:1")
            (bind(&StateMachineGCN::SetDebugRx, this, placeholders::_1))
            ("Set debux-rx state"
             "|debug[bool]:dump received text and parsed text to console (yes/no)");


        // Conenction commands
        T::AddEvent("DISCONNECT", State::kConnected)
            (bind(&StateMachineGCN::Disconnect, this))
            ("disconnect from ethernet");
        T::AddEvent("RECONNECT", "O", State::kDisconnected, State::kConnected)
            (bind(&StateMachineGCN::Reconnect, this, placeholders::_1))
            ("(Re)connect ethernet connection to FTM, a new address can be given"
             "|[host][string]:new ethernet address in the form <host:port>");

        fGCN.StartConnect();
    }

    void SetEndpoint(const string &url)
    {
        vector<string> v;
        v.push_back(url);
        fGCN.SetEndPoints(v);
    }

    int EvalOptions(Configuration &conf)
    {
        fGCN.SetVerbose(!conf.Get<bool>("quiet"));
        fGCN.SetEndPoints(conf.Vec<string>("addr"));

        fGCN.SetURI(conf.Get<string>("schedule-database"));

        return -1;
    }
};

// ------------------------------------------------------------------------

#include "Main.h"

template<class T, class S, class R>
int RunShell(Configuration &conf)
{
    return Main::execute<T, StateMachineGCN<S, R>>(conf);
}

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("FTM control options");
    control.add_options()
        ("no-dim",        po_bool(),  "Disable dim services")
        ("addr,a",        vars<string>(),  "Network addresses of GCN server")
        ("quiet,q",       po_bool(true),  "Disable printing contents of all received messages (except dynamic data) in clear text.")
        ("schedule-database", var<string>(), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
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
        "The gcn reads and evaluates alerts from the GCN network.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: gcn [-c type] [OPTIONS]\n"
        "  or:  gcn [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    Main::PrintHelp<StateMachineGCN<StateMachine, ConnectionGCN>>();

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
                return RunShell<LocalStream, StateMachine, ConnectionGCN>(conf);
            else
                return RunShell<LocalStream, StateMachineDim, ConnectionDimGCN>(conf);
        }
        // Cosole access w/ and w/o Dim
        if (conf.Get<bool>("no-dim"))
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachine, ConnectionGCN>(conf);
            else
                return RunShell<LocalConsole, StateMachine, ConnectionGCN>(conf);
        }
        else
        {
            if (conf.Get<int>("console")==0)
                return RunShell<LocalShell, StateMachineDim, ConnectionDimGCN>(conf);
            else
                return RunShell<LocalConsole, StateMachineDim, ConnectionDimGCN>(conf);
        }
    }
    /*catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }*/

    return 0;
}
