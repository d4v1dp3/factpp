#include "MessageDim.h"

#include "tools.h"

#include <iostream>

int main(int, const char **argv)
{
    // We could use putenv to make the Configure class change the value...
    setenv("DIM_DNS_NODE", "localhost", 0);

    // Get the name of the server we should subscribe to from the cmd line
    const std::string server = argv[1] ? argv[1] : "TIME";

    // Some info on the console
    std::cout << "Subscribing to " << server << "/MESSAGE...\n" << std::endl;

    // Create a message handler (default: redirects to stdout)
    MessageImp msg;

    // Subscribe to SERVER/MESSAGE and start output
    MessageDimRX msgrx(server, msg);

    // Just do nothing ;)
    while (1)
        usleep(1);

    return 0;
}

// **************************************************************************
/** @example log.cc

This is a simple example which subscribes to the message service of one
dedicated dim client. It can be used to remotely log its log-messages.

To redirect the output to a file use the shell redirection or the
tee-program.

The program is stopped by CTRL-C

*/
// **************************************************************************
