#ifndef FACT_Database
#define FACT_Database

#include <exception>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <mysql++/mysql++.h>

#if !defined(__clang_major__) && defined(__GNUC__) &&  (__GNUC__ <= 4)
#include "tools.h" // to_string fix for ISDC
#endif

struct DatabaseName
{
    std::string user;
    std::string passwd;
    std::string server;
    std::string db;
    int port;
    char compression;

    DatabaseName(const std::string &database) : compression(0)
    {
        if (database.empty())
            return;

        static const boost::regex expr("(([[:word:].-]+)(:(.+))?@)?([[:word:].-]+)(:([[:digit:]]+))?(/([[:word:].-]+))(\\?compress=[01])?");

        boost::smatch what;
        if (!boost::regex_match(database, what, expr, boost::match_extra))
            throw std::runtime_error("Couldn't parse database URI '"+database+"'.");

        if (what.size()!=11)
            throw std::runtime_error("Error parsing database URI '"+database+"'.");

        user   = what[2];
        passwd = what[4];
        server = what[5];
        db     = what[9];

        compression = std::string(what[10])[10];

        try
        {
            port = stoi(std::string(what[7]));
        }
        catch (...)
        {
            port = 0;
        }
    }

    std::string uri() const
    {
        std::string rc;
        if (!user.empty())
            rc += user+"@";
        rc += server;
        if (port)
            rc += ":"+std::to_string(port);
        if (!db.empty())
            rc += "/"+db;
        return rc;
    }
};

class Database : public DatabaseName, public mysqlpp::Connection
{
public:
    Database(const std::string &desc) : DatabaseName(desc),
        mysqlpp::Connection()
    {
        if ((compression!='0' && boost::algorithm::to_lower_copy(server)!="localhost" && server!="127.0.0.1")||
            compression=='1')
            set_option(new mysqlpp::CompressOption());

        set_option(new mysqlpp::ReconnectOption(true));

        // Connect to the database
        if (!server.empty())
            reconnect();
    }

    void reconnect()
    {
        connect(db.c_str(), server.c_str(), user.c_str(), passwd.c_str(), port);
    }
};

#endif
