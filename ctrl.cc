/*************************************************************************
 * Program to send commands to an Intelligent Power Monitor (iPM), receive
 * returned data and generate a UDP packet to be sent to nidas.
 *
 * IN DEVELOPMENT:
 *     Questions are marked "Question:"
 *     Incomplete items are marked "TBD"
 *
 *  2023, Copyright University Corporation for Atmospheric Research
 *************************************************************************
*/

#include "naiipm.h"

static naiipm ipm;

void processArgs(int argc, char *argv[])
{
    int opt;
    int errflag = 0, nopt = 0;
    bool p = false;
    bool s = false;
    bool m = false;
    bool r = false;
    bool b = false;
    bool n = false;
    char c = '9';
    int nInfo = 0;

    // Options between colons require an argument
    // Options after second colon do not.
    while((opt = getopt(argc, argv, ":p:s:m:r:b:n:0:1:2:3:4:5:6:7:i")) != -1)
    {
        nopt++;
        switch(opt)
        {
            case 'p':  // Port the iPM is connected to
                p = true;
                ipm.setPort(optarg);
                break;
            case 's':  // Port to send additional status messages
                s = true;
		// TBD: not yet implemented
                break;
            case 'm':  // STATUS & MEASURE collection rate (hz)
                m = true;
                ipm.setRate(optarg);
                break;
            case 'r':  // Period of RECORD queries (minutes)
                r = true;
                ipm.setPeriod(optarg);
                break;
            case 'b':  // Baud rate
                b = true;
                ipm.setBaud(optarg);
                break;
            case 'n':  // Number of addresses in use on iPM
                n = true;
                ipm.setNumAddr(optarg);
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                ipm.setAddrInfo(opt-'0', optarg);
                nInfo++;
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

    // Confirm that the number of addrinfo command line entries equals the
    // the numaddr number.
    if (nInfo != 0 and atoi(ipm.numAddr()) != nInfo)
    {
        std::cout << "-n option must match number of addresses given on " <<
            "command line" << std::endl;
        errflag++;
    }

    if (errflag or not nopt or not p or not m or not r or not b or not n)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t-p <port>\t\tport iPM is connected to" << std::endl;
        std::cout << "\t-s <port>\t\tport to send status msgs to" << std::endl; // ??? - look at specs!
        std::cout << "\t-m <measurerate>\tSTATUS & MEASURE collection rate "
            << " (hz)" << std::endl;
        std::cout << "\t-r <recordperiod>\tperiod of RECORD queries (minutes)"
            << std::endl;
        std::cout << "\t-b <baudrate>\t\tbaud rate" << std::endl;
        std::cout << "\t-n <num_addr>\t\tnumber of active addresses on iPM"
            << std::endl;
        std::cout << "\t-# <addr,numphases,procqueries,port>\n"
            "\t\t\t\tnumber 0 to n-1 followed by info block" << std::endl;
        std::cout << "\t-i\t\t\trun in interactive mode (optional)"
            << std::endl;
        exit(1);
    }
}

int main(int argc, char * argv[])
{
    processArgs(argc, argv);
    int fd = ipm.open_port(ipm.Port());

    // TBD: Hardcode acserver ip address at top of code
    // TBD: Get port from xml addr info packet.
    ipm.open_udp("192.168.84.2", 30101);

    bool status = true;
    if (ipm.Interactive())
    {
        while (status != 0)
        {
          ipm.printMenu();
          status = ipm.readInput(fd);
          std::cout << std::boolalpha << "Status is " << status << std::endl;
        }
    } else {
        if (ipm.init(fd))
        {
            std::cout << "Device successfully initialized" << std::endl;
        } else {
            std::cout << "Device failed to initialize" << std::endl;
            ipm.close_port(fd);
            exit(1);
        }

        // Cycle on requested commands
        while (true)
        {
            status = ipm.loop(fd);
        }
    }

    std::cout << "Exiting ipm_ctrl" << std::endl;

    // Close socket descriptor
    ipm.close_udp();

    ipm.close_port(fd);
    exit(1);
}
