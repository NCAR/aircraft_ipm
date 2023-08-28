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
#include <fstream>
#include <cstdio>

static naiipm ipm;
const char *acserver = "192.168.84.2";

void processArgs(int argc, char *argv[])
{
    int opt;
    int errflag = 0, nopt = 0;
    bool p = false;
    bool m = false;
    bool r = false;
    bool b = false;
    bool n = false;
    bool i = false;
    int a = -1;
    std::string c = "";
    int nInfo = 0;


    // Options between colons require an argument
    // Options after last colon do not.
    while((opt = getopt(argc, argv, ":p:m:r:b:n:0:1:2:3:4:5:6:7:a:c:ivde"))
           != -1)
    {
        nopt++;
        switch(opt)
        {
            case 'p':  // Port the iPM is connected to
                p = true;
                ipm.setPort(optarg);
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
                if (not ipm.parse_addrInfo(opt-'0'))
                {
                    std::cout << optarg << " is not a valid address info block"
                        << std::endl;
                    exit(1);
                }

                nInfo++;
                break;
            case 'a':
                if (atoi(optarg) < 0 or atoi(optarg) > 7)  // verify
                {
                    std::cout << "Address " << optarg << " is invalid. "
                        "Please enter a valid address" << std::endl;
                    exit(1);
                } else {
                    ipm.setAddress(optarg);
                }
                break;
            case 'c':
                if (ipm.verify(optarg))
                {
                    ipm.setCmd(optarg);
                } else {
                    exit(1);
                }
                break;
            case 'i':  // Run in interactive (menu) mode
                ipm.setInteractive();
                i = true;
                break;
            case 'v': // Run in verbose mode
                ipm.setVerbose();
                break;
            case 'd': // Run in debug mode (output hex values)
                ipm.setScaleFlag(0);  // Turn off scaling
            case 'e': // Run in emulator mode
                ipm.setEmulate();
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
    if (nInfo != 0 and ipm.numAddr() != nInfo)
    {
        std::cout << "-n option must match number of addresses given on " <<
            "command line" << std::endl;
        errflag++;
    }

    if (errflag or  // not p or
        (not i and (not nopt or not m or not r or not n)))
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t-p <port>\t\tiPM connection port (Default:/dev/ttyS0)"
            << std::endl;
        std::cout << "\t-m <measurerate>\tSTATUS & MEASURE collection rate "
            << " (hz)" << std::endl;
        std::cout << "\t-r <recordperiod>\tperiod of RECORD queries (minutes)"
            << std::endl;
        std::cout << "\t-b <baudrate>\t\tbaud rate (Default:115200)"
            << std::endl;
        std::cout << "\t-n <num_addr>\t\tnumber of active addresses on iPM"
            << std::endl;
        std::cout << "\t-# <addr,procqueries,port>\n"
            "\t\t\t\tnumber 0 to n-1 followed by info block" << std::endl;
        std::cout << "\t-i\t\t\trun in interactive mode (optional)\n"
            "\t\t\t\t- When in interactive mode only -p and -b are\n"
            "\t\t\t\t  required\n"
            "\t\t\t\t- Inclusion of -a and -c will send a single\n"
            "\t\t\t\t  command and exit." << std::endl;
        std::cout << "\t-a\t\t\tset address (optional)" << std::endl;
        std::cout << "\t-c\t\t\tset command (optional)" << std::endl;
        std::cout << "\t-v\t\t\trun in verbose mode (optional)" << std::endl;
        std::cout << "\t-d\t\t\trun in debug mode; don't scale vars (optional)"
            << std::endl;
        std::cout << "\t-e\t\t\trun with emulator; longer timeout (optional)"
            << std::endl;
        exit(1);
    }
}

int main(int argc, char * argv[])
{

    // set up logging to a timestamped file
    char time_buf[100];
    time_t now = std::time({});;
    strftime(time_buf, 100, "%Y%m%d_%H%M%S", gmtime(&now));

    std::string filename = "ipm_" + (std::string)time_buf + ".log";
    std::ofstream logfile(filename);
    auto oldbuf = std::cout.rdbuf( logfile.rdbuf());

    // If in interactive mode, don't log to a file
    for (int i=0; i< argc; ++i) {
        if (strcmp(argv[i],"-i") == 0) {
            // go back to writing to stdout
            std::cout.rdbuf(oldbuf);
            std::remove(filename.c_str());  // remove logfile
        }
    }

    processArgs(argc, argv);
    int fd = ipm.open_port(ipm.Port());

    ipm.open_udp(acserver);

    bool status = true;
    if (ipm.Interactive())
    {
        bool mode  = ipm.setInteractiveMode(fd);
        // if  mode is True, successfully configured a command line query,
        // so request that now. If false, just exit.
        if (mode == true)
        {
            ipm.singleCommand(fd, ipm.Cmd(), atoi(ipm.Address()));
        }
        exit(1);
    } else {
        if (ipm.init(fd))
        {
            if (ipm.Verbose())
            {
                std::cout << "Device successfully initialized" << std::endl;
            }
        } else {
            std::cout << "Device failed to initialize" << std::endl;
            ipm.close_port(fd);
            exit(1);
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
    exit(1);
}
