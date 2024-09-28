/*************************************************************************
 * Program to send commands to an Intelligent Power Monitor (iPM), receive
 * returned data and generate a UDP packet to be sent to nidas.
 *
 * IN DEVELOPMENT:
 *     Questions are marked "Question:"
 *     Incomplete items are marked "TBD"
 *
 *  2024, Copyright University Corporation for Atmospheric Research
 *************************************************************************
*/

#include "naiipm.h"
#include "src/argparse.h"
#include <fstream>
#include <cstdio>
#include <cstring>

const char *acserver = "192.168.84.2";

int main(int argc, char * argv[])
{
    args.process(argc, argv);

    naiipm ipm;

    // set up logging to a timestamped file
    char time_buf[100];
    time_t now = time({});;
    strftime(time_buf, 100, "%Y%m%d_%H%M%S", gmtime(&now));

    std::string filename = "/var/log/ads/ipm_" + (std::string)time_buf + ".log";
    std::ofstream logfile(filename);
    auto oldbuf = std::cout.rdbuf( logfile.rdbuf());

    // If in interactive mode, don't log to a file
    for (int i=0; i< argc; ++i) {
        if (strcmp(argv[i],"-i") == 0) {
            // go back to writing to stdout
            std::cout << "Start logging" << std::endl;
            std::cout.rdbuf(oldbuf);
            std::remove(filename.c_str());  // remove logfile
        }
    }

    int fd = ipm.open_port();

    ipm.open_udp(acserver);

    bool status = true;
    if (args.Interactive())
    {
        bool mode  = ipm.setInteractiveMode(fd);
        // if mode is True, successfully configured a command line query,
        // so request that now. If false, just exit.
        if (mode == true)
        {
            int addr = atoi(args.Address());
            ipm.clear(fd, addr);  // Clear garbage off port
            ipm.singleCommand(fd);
        }
        return 0;
    } else {
        if (ipm.init(fd))
        {
            if (args.Verbose())
            {
                std::cout << "Device successfully initialized" << std::endl;
            }
        } else {
            std::cout << "Device failed to initialize" << std::endl;
            ipm.close_port(fd);
            return 1;
        }

        // Cycle on requested commands
        while (true)
        {
            ipm.setRecordFreq();
            status = ipm.loop(fd);
            ipm.sleep();
        }
    }

    std::cout << "Exiting ipm_ctrl" << std::endl;

    // Close socket descriptor
    ipm.close_udp(-1);

    ipm.close_port(fd);

    return 0;
}
