#include <stdlib.h>

extern const char *GetLocalIp();

int local_main(int argc, char **argv);

int main(int argc, char **argv)
{
    setenv("DIM_HOST_NODE", GetLocalIp(), 1);

    return local_main(argc, argv);
}

#define main local_main
#include "dim/src/dns.c"
