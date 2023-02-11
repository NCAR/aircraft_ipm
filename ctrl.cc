/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "naiipm.h"

static naiipm ipm;

void processArgs(int argc, char *argv[])
{
    int opt;
    int errflag = 0, nopt = 0;
    bool p = false;

    // Options between colons require an argument
    // Options after second colon do not.
    while((opt = getopt(argc, argv, ":p:i")) != -1)
    {
        nopt++;
        switch(opt)
        {
            case 'p':  // Port the iPM is connected to
                p = true;
                ipm.setPort(optarg);
                break;
            case 'i':  // Run in interactive (menu) mode
                ipm.setInteractive();
                break;
            case ':':
                std::cerr << "option -" << char(optopt) <<
                    " needs a value" << std::endl;
                errflag++;
                break;
            case '?':
                std::cerr << "unknown option: " << char(optopt) << std::endl;
                errflag++;
                break;
        }
    }
    std::cout << ipm.Port() << std::endl;
    if (errflag or not nopt or not p)
    {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "\t-p <port>\tport iPM is connected to" << std::endl;
        std::cerr << "\t-i\tRun in interactive mode" << std::endl;
        exit(1);
    }
}

bool init_device(int fd)
{
    const char* msg = "OFF";
    if(not ipm.send_command(fd, (char *)msg)) { return false; }

    sleep(.11);  // Wait > 100ms

    msg = "RESET";
    if(not ipm.send_command(fd, (char *)msg)) { return false; }

    return true;
}

int main(int argc, char * argv[])
{
    processArgs(argc, argv);
    int fd = ipm.open_port(ipm.Port());

    if (ipm.Interactive())
    {
        ipm.printMenu();
        ipm.readInput(fd);
    } else {
        if (init_device(fd))
        {
            std::cout << "Device successfully initialized" << std::endl;
            fprintf(stderr, "Device successfully initialized\n");
        } else {
            std::cout << "Device failed to initialize" << std::endl;
            fprintf(stderr, "Device failed to initialize\n");
        }
    }

    ipm.close_port(fd);
    exit(1);
}


