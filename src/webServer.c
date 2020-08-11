#include <string.h>
#include <unistd.h>

int local_main(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc) {}

    char currwd[1024];
    strcpy(currwd, argv[0]);

    char *ptr = strrchr(currwd,'/');
    if (ptr)
        *ptr = '\0';

    strcpy(ptr, "/../dim/WebDID/");

    char *arg[2] = { currwd, argv[1] };
    return local_main(2, arg);
}

#define main local_main
#include "dim/src/webDid/webServer.c"
