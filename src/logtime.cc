#include "MessageDim.h"

#include "tools.h"
#include "Time.h"

#include <iostream>

int main(int, const char **)
{
   // We could use putenv to make the Configure class change the value...
    setenv("DIM_DNS_NODE", "localhost", 0);

    // Start a DimServer called TIME
    DimServer::start("TIME");



    // Some info on the console
    std::cout << "Offering TIME/MESSAGE...\n" << std::endl;

    // Setup a DimService called TIME/MESSAGE
    MessageDimTX msg("TIME");
    while (1)
    {
        // Send current time
        msg.Message(Time().GetAsStr());

        // wait approximately one second
        usleep(1000000);

        //std::cout << DimServer::getClientName() << std::endl;
        //std::cout << DimServer::getClientId() << std::endl;
        //std::cout << DimServer::getDnsPort() << std::endl;
        std::cout << "con: " << dis_get_conn_id() << std::endl;

        char **ids = DimServer::getClientServices();

        while (*ids)
        {
            std::cout << *ids << std::endl;
            ids++;
        }
    }

    return 0;
}

// **************************************************************************
/** @example logtime.cc

This is a simple example how to log messages through the Dim network
using MessageDimTX. Here we are offering the time once a second.

The program is stopped by CTRL-C

*/
// **************************************************************************
