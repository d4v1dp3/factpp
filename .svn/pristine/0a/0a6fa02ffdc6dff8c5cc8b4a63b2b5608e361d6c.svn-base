#include "externals/PixelMap.h"

using namespace std;

#include <boost/regex.hpp>
#include <mysql++/mysql++.h>

void BiasMap::Retrieve(const std::string &database)
{
    static const boost::regex expr("(([[:word:].-]+)(:(.+))?@)?([[:word:].-]+)(:([[:digit:]]+))?(/([[:word:].-]+))");
    // 2: user
    // 4: pass
    // 5: server
    // 7: port
    // 9: db

    boost::smatch what;
    if (!boost::regex_match(database, what, expr, boost::match_extra))
        throw runtime_error("Couldn't parse '"+database+"'.");

    if (what.size()!=10)
        throw runtime_error("Error parsing '"+database+"'.");

    const string user   = what[2];
    const string passwd = what[4];
    const string server = what[5];
    const string db     = what[9];
    const int port      = atoi(string(what[7]).c_str());

    mysqlpp::Connection conn(db.c_str(), server.c_str(), user.c_str(), passwd.c_str(), port);

    const mysqlpp::StoreQueryResult res =
        conn.query("SELECT fPatchNumber, AVG(fVoltageNom), fOffset "
                   " FROM GapdVoltages "
                   " LEFT JOIN BiasOffsets USING(fPatchNumber) "
                   " GROUP BY fPatchNumber").store();

    clear();

    int l = 0;
    for (vector<mysqlpp::Row>::const_iterator v=res.begin(); v<res.end(); v++)
    {
        const int id = (*v)[0];

        if (id<0 || id>416)
        {
            ostringstream str;
            str << "Invalid channel id " << id << " received from database.";
            throw runtime_error(str.str());
        }

        BiasMapEntry entry;

        entry.hv_board   = id/32;
        entry.hv_channel = id%32;
        entry.Vnom       = (*v)[1];
        entry.Voff       = (*v)[2];

        (*this)[id] = entry;

        l++;
    }

    if (l!=416)
        throw runtime_error("Number of rows retrieved from the database does not match 416.");

    if (size()!=416)
        throw runtime_error("Number of entries retrived from database does not match 416.");
}
