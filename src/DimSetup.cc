// **************************************************************************
/** @namespace Dim

@brief Namespace to host some global Dim helper functions

*/
// **************************************************************************
#include "Dim.h"

/*
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
*/
#include <boost/asio.hpp>

#include <iostream>

using namespace std;

// --------------------------------------------------------------------------
//
//! Tries to determine the local IP address with which we will connect
//! to the DIM dns.
//!
//! The local IP address is necessary because in some circumstances
//! Dim needs the IP address of the connecting machine (DIM_HOST_NODE)
//! for the clients to correctly connect to them. So what we would
//! need is the IP address over which the machine is reachable from
//! the client. Unfortunately, we have no access to the client
//! connection, hence, we have to find the best guess of an address
//! which is not our own machine and hope it is routed over the
//! standard ethernet interface over which other clients will connect.
//!
//! To not send random packages over the network we use a local
//! IP address. To make sure it is something which leaves the network
//! card and is not just our machine, we can use a broadcast address.
//! Consequently, the deafult has been chosen to be "192.168.0.255"
//!
//! @param dns
//!     Address of the Dim-dns
//!
//! @returns
//!     The IP Address through which the connection to the DNS will
//!     take place.
//!
//! @todo
//!     Implement a --host command line option (just in case)
//!
string Dim::GetLocalIp(const string &dns)
{
    using namespace boost::asio;
    using namespace boost::asio::ip;

    cout << "Trying to resolve local IP address..." << endl;

    boost::system::error_code ec;

    boost::asio::io_service io_service;

    udp::socket socket(io_service);

    udp::resolver resolver(io_service);
    udp::resolver::query query(dns, "0");
    udp::resolver::iterator iterator = resolver.resolve(query, ec);
    if (ec)
    {
        //cout << "WARNING - Failure in name-resolution of '" << dns << ":0': ";
        cout << "WARNING - Could not resolve local ip address: ";
        cout << ec.message() << " (" << ec << ")" << endl;
        return dns;
    }

    for (; iterator != udp::resolver::iterator(); ++iterator)
    {
        udp::endpoint endpoint = *iterator;
        socket.connect(endpoint, ec);
        if (ec)
        {
            cout << "WARNING - Could not resolve local ip address: ";
            cout << ec.message() << " (" << ec << ")" << endl;
            continue;
        }

        const string addr = socket.local_endpoint().address().to_v4().to_string();
        return addr;
    }

    return "localhost";

/*
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET; //AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    int rv;
    if ((rv = getaddrinfo(dns.c_str(), NULL, &hints, &servinfo)) != 0)
    {
        cout << "WARNING - getaddrinfo: " << gai_strerror(rv) << endl;
        return dns;
    }

    // loop through all the results and connect to the first we can
    for (p=servinfo; p; p=p->ai_next)
    {
        const int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock==-1)
            continue;

        if (connect(sock, p->ai_addr, p->ai_addrlen)==-1)
        {
            cout << "WARNING - connect: " << strerror(errno) << endl;
            close(sock);
            continue;
        }

        sockaddr_in name;
        socklen_t namelen = sizeof(name);
        if (getsockname(sock, (sockaddr*)&name, &namelen)==-1)
        {
            cout << "WARNING - getsockname: " << strerror(errno) << endl;
            close(sock);
            continue;
        }

        char buffer[16];
        if (!inet_ntop(AF_INET, &name.sin_addr, buffer, 16))
        {
            cout << "WARNING - inet_ntop: " << strerror(errno) << endl;
            close(sock);
            continue;
        }

        close(sock);

        freeaddrinfo(servinfo); // all done with this structure

        cout << "DIM_HOST_NODE=" << buffer << endl;
        return buffer;
    }

    freeaddrinfo(servinfo); // all done with this structure

    return dns;
*/
}

// --------------------------------------------------------------------------
//
//! Set the environment variable DIM_DNS_NODE to the given string and
//! DIM_HOST_NODE to the IP-address through which this machine connects
//! to the dns.
//!
//! @param dns
//!     Address of the Dim-dns
//!
void Dim::Setup(const std::string &dns, const std::string &host, const uint16_t &port)
{
    if (dns.empty())
    {
        setenv("DIM_DNS_NODE", "...", 1);
        //unsetenv("DIM_DNS_NODE");
        //unsetenv("DIM_HOST_NODE");
        return;
    }

    const string loc = host.empty() ? Dim::GetLocalIp(dns.c_str()) : host;

    setenv("DIM_DNS_NODE",  dns.c_str(), 1);
    setenv("DIM_HOST_NODE", loc.c_str(), 1);
    if (port>0)
        setenv("DIM_DNS_PORT", to_string(port).c_str(), 1);

    cout << "Setting DIM_DNS_NODE =" << dns;
    if (port>0)
        cout << ':' << port;
    cout << "\nSetting DIM_HOST_NODE=" << loc << endl;
}

extern "C"
{
    const char *GetLocalIp()
    {
        static string rc;
        rc = Dim::GetLocalIp();
        cout << "Setting DIM_HOST_NODE=" << rc << endl;
        return rc.c_str();
    }
}
