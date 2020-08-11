#include <iostream>

#include "EventImp.h"
#include "Configuration.h"
#include "StateMachineDim.h"
#include "LocalControl.h"

#include <boost/tokenizer.hpp>

#include <dbus/dbus-glib-lowlevel.h>

using namespace std;

class SkypeClient : public StateMachineDim
{
private:
    static const string fAuthorizationMsg;

    enum {
        kStateDisconnected = 1,
        kStateConnected = 2,
    };

    Time fLastConnect;

    DBusConnection *fBus;
    GMainLoop *fLoop;

    vector<string> fContacts;

    string fUser;

    bool fAllowRaw;

    uint64_t fLastReadMessage;

    string Contact(const string &id) const
    {
        if (id.size()==0)
            return "";

        if (id[0]!='#')
            return "";

        const size_t p = id.find_first_of('/');
        if (p==string::npos)
            return "";

        return id.substr(1, p-1);
    }



    static DBusHandlerResult NotifyHandler(DBusConnection *, DBusMessage *dbus_msg, void *user_data)
    {
        static_cast<SkypeClient*>(user_data)->HandleDBusMessage(dbus_msg);
        static_cast<SkypeClient*>(user_data)->Minimize();
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    int HandleMsg(const EventImp &evt)
    {
        if (evt.GetSize()==0)
            return GetCurrentState();

        for (auto it=fContacts.begin(); it!=fContacts.end(); it++)
            SendSkypeMessage(*it, evt.GetString());

        ostringstream msg;
        msg << evt.GetString() << " [" << fContacts.size() << "]";

        Info(msg);

        return GetCurrentState();
    }

    int HandleRaw(const EventImp &evt)
    {
        if (evt.GetSize()==0 || !fAllowRaw)
            return GetCurrentState();

        SendDBusMessage(evt.GetString());

        return GetCurrentState();
    }

    int HandleCall()
    {
        int cnt = 0;
        for (auto it=fContacts.begin(); it!=fContacts.end(); it++)
        {
            const string user = Contact(*it);
            if (user.empty())
                continue;

            SendDBusMessageNB("CALL "+user);

            cnt++;
        }

        ostringstream msg;
        msg << "CALLING [" << cnt << "/" << fContacts.size() << "]";

        Info(msg);

        return GetCurrentState();
    }

    int HandleSMS(const EventImp &/*sms*/)
    {
        /*
         -> CREATE SMS OUTGOING +0123456789
            <- SMS 821 STATUS COMPOSING
            <- SMS 821 PRICE 0
            <- SMS 821 TIMESTAMP 0
            <- SMS 821 PRICE_PRECISION 3
            <- SMS 821 PRICE_CURRENCY EUR
            <- SMS 821 STATUS COMPOSING
            <- SMS 821 TARGET_NUMBERS +0123456789
            <- SMS 821 PRICE -1
            <- SMS 821 TARGET_STATUSES +0123456789=TARGET_ANALYZING
            <- SMS 821 TARGET_STATUSES +0123456789=TARGET_ACCEPTABLE
            <- SMS 821 PRICE 78

         //-------------------------------------------------------------------
         // Now let's add two more target numbers (in addition to original)
         -> SET SMS 1702 TARGET_NUMBERS +37259877305, +37259877306, +37259877307
            <- SMS 1702 TARGET_NUMBERS +37259877305, +37259877306, +37259877307
            <- SMS 1702 TARGET_NUMBERS +37259877305, +37259877306, +37259877307
            <- SMS 1702 PRICE -1
            <- SMS 1702 TARGET_STATUSES +37259877305=TARGET_ACCEPTABLE, +37259877306=TARGET_ANALYZING, +37259877307=TARGET_ANALYZING
            <- SMS 1702 TARGET_STATUSES +37259877305=TARGET_ACCEPTABLE, +37259877306=TARGET_ACCEPTABLE, +37259877307=TARGET_ACCEPTABLE
            <- SMS 1702 TARGET_STATUSES +37259877305=TARGET_ACCEPTABLE, +37259877306=TARGET_ACCEPTABLE, +37259877307=TARGET_ACCEPTABLE
            <- SMS 1702 PRICE 234

            TARGET_ANALYZING
            TARGET_UNDEFINED
            TARGET_ACCEPTABLE
            TARGET_NOT_ROUTABLE
            TARGET_DELIVERY_PENDING
            TARGET_DELIVERY_SUCCESSFUL
            TARGET_DELIVERY_FAILED
            UNKNOWN

         // ----------------------------------------------------------------
         // This is how to set the message text property
         // Note that you will get two identical lines in response
         -> SET SMS 821 BODY "test 123 test 223 test 333"
            <- SMS 821 BODY "test 123 test 223 test 333"
            <- SMS 821 BODY "test 123 test 223 test 333"

         // ----------------------------------------------------------------
         // Now lets try to send the message
         -> ALTER SMS 821 SEND
            <- ALTER SMS 821 SEND
            <- SMS 821 STATUS SENDING_TO_SERVER
            <- SMS 821 TIMESTAMP 1174058095
            <- SMS 821 TARGET_STATUSES +0123456789=TARGET_ACCEPTABLE
            <- SMS 821 TARGET_STATUSES +0123456789=TARGET_DELIVERY_FAILED
            <- SMS 821 FAILUREREASON INSUFFICIENT_FUNDS
            <- SMS 821 STATUS FAILED
            <- SMS 821 IS_FAILED_UNSEEN TRUE

            STATUS
             RECEIVED                   the message has been received (but not tagged as read)
             READ                       the message has been tagged as read
             COMPOSING                  the message has been created but not yet sent
             SENDING_TO_SERVER          the message is in process of being sent to server
             SENT_TO_SERVER             the message has been sent to server
             DELIVERED                  server has confirmed that the message is sent out to recepient
             SOME_TARGETS_FAILED        server reports failure to deliver the message to one of the recepients within 24h
             FAILED                     the message has failed, possible reason may be found in FAILUREREASON property
             UNKNOWN                    message status is unknown

            FAILUREREASON
             MISC_ERROR                 indicates failure to supply a meaningful error message
             SERVER_CONNECT_FAILED      unable to connect to SMS server
             NO_SMS_CAPABILITY          recepient is unable to receive SMS messages
             INSUFFICIENT_FUNDS         insufficient Skype Credit to send an SMS message
             INVALID_CONFIRMATION_CODE  set when an erroneous code was submitted in a CONFIRMATION_CODE_SUBMIT message
             USER_BLOCKED               user is blocked from the server
             IP_BLOCKED                 user IP is blocked from the server
             NODE_BLOCKED               user p2p network node has been blocked from the server
             UNKNOWN                    default failure code
             NO_SENDERID_CAPABILITY     Set when a CONFIRMATION_CODE_REQUEST SMS message is sent with a mobile phone number containing country code of either USA, Taiwan or China. Setting reply-to number from Skype SMS’s to your mobile number is not supported in these countries. Added in Skype version 3.5 (protocol 8).

         // ----------------------------------------------------------------
         // As sending the message failed (not enough Skype credit),
         // lets delete the message
         -> DELETE SMS 821
            <- DELETE SMS 821
            */

        return GetCurrentState();
    }

    string SendDBusMessage(const string &cmd, bool display=true)
    {
        DBusMessage *send = GetDBusMessage(cmd);
        if (!send)
            return "";

        DBusError error;
        dbus_error_init(&error);

        // Send the message and wait for reply
        if (display)
            Info("TX: "+cmd);
        DBusMessage *reply=
            dbus_connection_send_with_reply_and_block(fBus, send,
                                                      -1,//DBUS_TIMEOUT_USE_DEFAULT,
                                                      &error);
        /*
        DBusPendingCall *pending = 0;
        if (!dbus_connection_send_with_reply (fBus, send,
                                              &pending, DBUS_TIMEOUT_USE_DEFAULT))
            return "";

        if (!pending)
            return "";

        bool = dbus_pending_call_get_completed(pending);
        //dbus_pending_call_block(pending);
        DBusMessage *reply = dbus_pending_call_steal_reply(pending);
        dbus_pending_call_unref(pending);
        */

        if (!reply)
        {
            Error("dbus_connection_send_with_reply_and_block: "+string(error.message));
            dbus_error_free(&error);
            return "";
        }

        // Get Skype's reply string
        const char *ack = 0;
        dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &ack, DBUS_TYPE_INVALID);

        if (display)
            Info("RX: "+string(ack));

        const string rc = ack;

        // Show no interest in the previously created messages.
        // DBus will delete a message if reference count drops to zero.
        dbus_message_unref(send);
        dbus_message_unref(reply);

        return rc;
    }

    DBusMessage *GetDBusMessage(const string &cmd)
    {
        // Create a message to be sent to Skype

        // Constructs a new message to invoke a method on a remote object.
        // Sets the service the message should be sent to "com.Skype.API"
        // Sets the object path the message should be sent to "/com/Skype"
        // Sets the interface to invoke method on to "com.Skype.API"
        // Sets the method to invoke to "Invoke"
        DBusMessage *send=
            dbus_message_new_method_call("com.Skype.API", "/com/Skype",
                                         "com.Skype.API", "Invoke");
        if (!send)
        {
            Error("dbus_message_new_method_call failed.");
            return NULL;
        }

        // Set the argument of the Invoke method
        // Sets arg to be an argument to be passed to the Invoke method.
        // It is an input argument. It has a type string.
        // There are no output arguments.
        const char *msg = cmd.c_str();
        dbus_message_append_args(send,
                                 DBUS_TYPE_STRING, &msg,
                                 DBUS_TYPE_INVALID);

        return send;
    }

    bool Minimize()
    {
        DBusMessage *send = GetDBusMessage("MINIMIZE");
        if (!send)
            return false;

        // Send the message and ignore the reply
        const bool rc = dbus_connection_send(fBus, send, NULL);

        // Show no interest in the previously created messages.
        // DBus will delete a message if reference count drops to zero.
        dbus_message_unref(send);

        return rc;
    }

    bool SendDBusMessageNB(const string &cmd)
    {
        DBusMessage *send = GetDBusMessage(cmd);
        if (!send)
            return false;

        // Send the message and ignore the reply
        Info("TX: "+cmd);
        const bool rc = dbus_connection_send(fBus, send, NULL);

        // Show no interest in the previously created messages.
        // DBus will delete a message if reference count drops to zero.
        dbus_message_unref(send);

        return rc;
    }

    bool SendSkypeMessage(const string &chat, const string &msg)
    {
        return SendDBusMessageNB("CHATMESSAGE "+chat+" "+msg);
/*
        // SendDBusMessage(bus, "CHAT CREATE "+user);

        const string rc = SendDBusMessage("CHATMESSAGE "+chat+" "+msg);

        const vector<string> vec = Split(rc);
        if (vec[0]=="ERROR")
        {
            auto it = find(fContacts.begin(), fContacts.end(), chat);
            if (it!=fContacts.end())
                fContacts.erase(it);
            return false;
        }

        return true;
        */
    }
    vector<string> Split(const string &msg)
    {
        using namespace boost;

        typedef char_separator<char> separator;
        const tokenizer<separator> tok(msg, separator(" "));

        vector<string> vec;
        for (auto it=tok.begin(); it!=tok.end(); it++)
            vec.push_back((*it)[0]==0?it->substr(1):*it);

        return vec;
    }

    void HandleDBusMessage(DBusMessage *dbus_msg)
    {
        if (GetCurrentState()!=kStateConnected)
            return;

        // CALL target1, target2, target3
        // SET CALL <id> STATUS FINISHED

        // Stores the argument passed to the Notify method
        // into notify_argument.

        char *notify_argument=0;
        dbus_message_get_args(dbus_msg, 0,
                              DBUS_TYPE_STRING, &notify_argument,
                              DBUS_TYPE_INVALID);

        Info("Notify: "+string(notify_argument));

        const vector<string> vec = Split(notify_argument);

        if (vec[0]=="CURRENTUSERHANDLE")
        {
            if (vec[1]!=fUser)
            {
                Error("Wrong user '"+vec[1]+"' logged in, '"+fUser+"' expected!");
                fNewState = kStateDisconnected;
                return;
            }
        }

        if (vec[0]=="CONNSTATUS")
        {
            // OFFLINE / CONNECTING / PAUSING / ONLINE
            if (vec[1]!="ONLINE")
            {
                Error("Connection status '"+vec[1]+"'");
                fNewState = kStateDisconnected;
                return;
            }
        }

        if (vec[0]=="USERSTATUS")
        {
            if (vec[1]!="ONLINE")
            {
                Info("Skype user not visible... setting online.");
                SendDBusMessageNB("SET USERSTATUS ONLINE");
            }
        }

       // USER rtlprmft RECEIVEDAUTHREQUEST Please allow me to see when you are online
 
        if (vec[0]=="USER")
        {
            if (vec[2]=="ONLINESTATUS")
            {
                if (vec[3]=="OFFLINE")
                {
                }
                Info("User '"+vec[1]+"' changed status to '"+vec[3]+"'");
            }

            // Answer authorization requests
            if (vec[2]=="RECEIVEDAUTHREQUEST")
                SendDBusMessageNB("SET USER "+vec[1]+" BUDDYSTATUS 2 "+fAuthorizationMsg);

            //if (vec[2]=="NROF_AUTHED_BUDDIES")
            //    cout << vec[1] << " --> " << vec[3];
        }

        if (vec[0]=="GROUP")
        {
            // 1: gorup id
            // 2: NROFUSERS
            // 3: n
        }

        if (vec[0]=="CHATMESSAGE")
        {
            if (vec[2]=="STATUS" && (vec[3]=="RECEIVED"|| vec[3]=="READ"))
            {
                const uint64_t last = stoll(vec[1]);

                // Check if message has already been processed: Sometimes
                // some messages are received twice as READ/READ
                if (last<=fLastReadMessage)
                    return;
                fLastReadMessage = last;

                string rc;

                rc=SendDBusMessage("GET CHATMESSAGE "+vec[1]+" CHATNAME");

                const string id = Split(rc)[3];

                rc=SendDBusMessage("GET CHATMESSAGE "+vec[1]+" BODY");

                const size_t p = rc.find(" BODY ");
                if (p==string::npos)
                {
                    cout<< "BODY TAG NOT FOUND|" << rc << "|" << endl;
                    return;
                }

                rc = Tools::Trim(rc.substr(rc.find(" BODY ")+6));

                if (rc=="start")
                {
                    auto it = find(fContacts.begin(), fContacts.end(), id);
                    if (it==fContacts.end())
                    {
                        SendSkypeMessage(id, "Successfully subscribed.");
                        fContacts.push_back(id);
                    }
                    else
                        SendSkypeMessage(id, "You are already subscribed.");

                    return;
                }
                if (rc=="stop")
                {
                    auto it = find(fContacts.begin(), fContacts.end(), id);
                    if (it!=fContacts.end())
                    {
                        SendSkypeMessage(id, "Successfully un-subscribed.");
                        fContacts.erase(it);
                    }
                    else
                        SendSkypeMessage(id, "You were not subscribed.");

                    return;
                }

                if (rc=="status")
                {
                    for (auto it=fContacts.begin(); it!=fContacts.end(); it++)
                    {
                        if (*it==vec[1])
                        {
                            SendSkypeMessage(id, "You are subscribed.");
                            return;
                        }
                    }
                    SendSkypeMessage(id, "You are not subscribed.");
                    return;
                }

                SendSkypeMessage(id, "SYNTAX ERROR\n\nAvailable commands:\nPlease use either 'start', 'stop' or 'status'");

            }
        }

        if (vec[0]=="CHAT")
        {
            const string id = vec[1];
            if (vec[2]=="ACTIVITY_TIMESTAMP")
            {
                //SendDBusMessage("CHAT CREATE "+Contact(vec[1]));
                //Info(vec[2]);
                // ALTER CHAT DISBAND
            }
            if (vec[2]=="MYROLE")
            {
            }
            if (vec[2]=="MEMBERS")
            {
            }
            if (vec[2]=="ACTIVEMEMBERS")
            {
            }
            if (vec[2]=="STATUS")
            {
                // vec[3]=="DIALOG")
            }
            if (vec[2]=="TIMESTAMP")
            {
            }
            if (vec[2]=="DIALOG_PARTNER")
            {
            }
            if (vec[2]=="FRIENDLYNAME")
            {
                // Notify: CHAT #maggiyy/$rtlprmft;da26ea52b3e70e65 FRIENDLYNAME Lamouette | noch ne message
            }
        }

        if (vec[0]=="CALL")
        {
            if (vec[2]=="STATUS" && vec[2]=="INPROGRESS")
                SendDBusMessageNB("SET CALL "+vec[1]+ "STATUS FINISHED");

            // CALL 1501 STATUS UNPLACED
            // CALL 1501 STATUS ROUTING
            // CALL 1501 STATUS RINGING
        }

    }

    int HandleDisconnect()
    {
        return kStateDisconnected;
    }

    int HandleConnect()
    {
        fLastConnect = Time();

        // Enable client connection to Skype
        if (SendDBusMessage("NAME FACT++")!="OK")
            return kStateDisconnected;

        // Negotiate protocol version
        if (SendDBusMessage("PROTOCOL 5")!="PROTOCOL 5")
            return kStateDisconnected;

        // Now we are connected: Minimize the window...
        SendDBusMessageNB("MINIMIZE");

        // ... and switch off the away message
        SendDBusMessageNB("SET AUTOAWAY OFF");

        // Check for unauthorized users and...
        const string rc = SendDBusMessage("SEARCH USERSWAITINGMYAUTHORIZATION");

        // ...authorize them
        vector<string> users = Split(rc);

        if (users[0]!="USERS")
        {
            Error("Unexpected answer received '"+rc+"'");
            return kStateDisconnected;
        }

        for (auto it=users.begin()+1; it!=users.end(); it++)
        {
            const size_t p = it->length()-1;
            if (it->at(p)==',')
                it->erase(p);

            SendDBusMessageNB("SET USER "+*it+" BUDDYSTATUS 2 "+fAuthorizationMsg);
        }

        return kStateConnected;
    }

    Time fLastPing;
    int fNewState;

    int Execute()
    {
        fNewState = -1;

        static GMainContext *context = g_main_loop_get_context(fLoop);
        g_main_context_iteration(context, FALSE);

        if (fNewState>0)
            return fNewState;

        const Time now;

        if (GetCurrentState()>kStateDisconnected)
        {
            if (now-fLastPing>boost::posix_time::seconds(15))
            {
                if (SendDBusMessage("PING", false)!="PONG")
                    return kStateDisconnected;

                fLastPing = now;
            }

            return GetCurrentState();
        }

        if (now-fLastConnect>boost::posix_time::minutes(1))
            return HandleConnect();

        return GetCurrentState();
    }

public:
    SkypeClient(ostream &lout) : StateMachineDim(lout, "SKYPE"),
        fLastConnect(Time()-boost::posix_time::minutes(5)), fLoop(0),
        fLastReadMessage(0)
    {
        AddStateName(kStateDisconnected, "Disonnected", "");
        AddStateName(kStateConnected,    "Connected",   "");

        AddEvent("MSG", "C", kStateConnected)
            (bind(&SkypeClient::HandleMsg, this, placeholders::_1))
            ("|msg[string]:message to be distributed");

        AddEvent("RAW", "C")
            (bind(&SkypeClient::HandleRaw, this, placeholders::_1))
            ("|msg[string]:send a raw message to the Skype API");

        AddEvent("CALL", "", kStateConnected)
            (bind(&SkypeClient::HandleCall, this))
            ("");

        AddEvent("CONNECT", kStateDisconnected)
            (bind(&SkypeClient::HandleConnect, this))
            ("");

        AddEvent("DISCONNECT", kStateConnected)
            (bind(&SkypeClient::HandleDisconnect, this))
            ("");

        fLoop = g_main_loop_new(NULL, FALSE);
    }
    ~SkypeClient()
    {
        g_main_loop_unref(fLoop);
    }

    int EvalOptions(Configuration &conf)
    {
        fUser = conf.Get<string>("user");
        fAllowRaw = conf.Get<bool>("allow-raw");

        // Get a connection to the session bus.
        DBusError error;
        dbus_error_init(&error);

        fBus = dbus_bus_get(DBUS_BUS_SESSION, &error);
        if (!fBus)
        {
            Error("dbus_bus_get failed: "+string(error.message));
            dbus_error_free(&error);
            return 1;
        }

        // Set up this connection to work in a GLib event loop.
        dbus_connection_setup_with_g_main(fBus, NULL);

        // Install notify handler to process Skype's notifications.
        // The Skype-to-client method call.
        DBusObjectPathVTable vtable;
        vtable.message_function = SkypeClient::NotifyHandler;

        // We will process messages with the object path "/com/Skype/Client".
        const dbus_bool_t check =
            dbus_connection_register_object_path(fBus, "/com/Skype/Client",
                                                 &vtable, this);
        if (!check)
        {
            Error("dbus_connection_register_object_path failed.");
            return 2;
        }

        return -1;
    }

    int Write(const Time &time, const string &txt, int severity=MessageImp::kMessage)
    {
        return MessageImp::Write(time, txt, severity);
    }
};

const string SkypeClient::fAuthorizationMsg =
    "This is an automatic client of the FACT project (www.fact-project.org). "
    "If you haven't tried to get in contact with this bot, feel free to block it. "
    "In case of problems or questions please contact system@fact-project.org.";


// -------------------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    const string n = conf.GetName()+".log";

    po::options_description config("Skype client options");
    config.add_options()
        ("user", var<string>("www.fact-project.org"), "If a user is given only connection to a skype with this user are accepted.")
        ("allow-raw", po_bool(false), "This allows sending raw messages to the SKype API (for debugging)")
        ;

    conf.AddOptions(config);
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
        "The skypeclient is a Dim to Skype interface.\n"
        "\n"
        "The default is that the program is started without user intercation. "
        "All actions are supposed to arrive as DimCommands. Using the -c "
        "option, a local shell can be initialized. With h or help a short "
        "help message about the usuage can be brought to the screen.\n"
        "\n"
        "Usage: skypeclient [OPTIONS]\n"
        "  or:  skypeclient [OPTIONS]\n";
    cout << endl;
}

void PrintHelp()
{
    /* Additional help text which is printed after the configuration
     options goes here */
}


#include "Main.h"

int main(int argc, const char *argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    Main::SetupConfiguration(conf);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    // No console access at all
    if (!conf.Has("console"))
        return Main::execute<LocalStream, SkypeClient>(conf);

    if (conf.Get<int>("console")==0)
        return Main::execute<LocalShell, SkypeClient>(conf);
    else
        return Main::execute<LocalConsole, SkypeClient>(conf);

    return 0;
}
