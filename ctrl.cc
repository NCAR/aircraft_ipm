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
#include <cstring>
#ifdef __linux__
    #include <sys/io.h>
#endif

static naiipm ipm;
const char *acserver = "192.168.84.2";


void Usage()
{
    std::cout << "Usage:\n"
                 "\t-D device\t\tiPM connection device (Default:/dev/ttyS0)\n"
                 "\t-m measurerate\t\tSTATUS & MEASURE collection rate (hz)\n"
                 "\t-r recordperiod\t\tperiod of RECORD queries (minutes)\n"
                 "\t-b baudrate\t\tbaud rate (Default:115200)\n"
                 "\t-n num_addr\t\tnumber of active addresses on iPM\n"
                 "\t-# addr,procqueries,port\n"
                 "\t\t\t\t- addr is the iPM address; a number 0 to n-1\n"
                 "\t\t\t\t- procqueries is an integer representing a 3-bit\n"
                 "\t\t\t\t  boolean field indicating which query responses\n"
                 "\t\t\t\t  [RECORD,MEASURE,STATUS] should be processed, eg\n"
                 "\t\t\t\t  3 (b’011) requests MEASURE+STATUS\n"
                 "\t\t\t\t  5 (b’101) requests RECORD+STATUS\n"
                 "\t\t\t\t- port is the port to send the output UDP string\n"
                 "\t-i\t\t\trun in interactive mode (optional)\n"
                 "\t\t\t\t- When in interactive mode only -D and -b are\n"
                 "\t\t\t\t  required\n"
                 "\t\t\t\t- Inclusion of -a and -c will send a single\n"
                 "\t\t\t\t  command and exit.\n"
                 "\t-a\t\t\tset address (optional)\n"
                 "\t-c\t\t\tset command (optional)\n"
                 "\t-v\t\t\trun in verbose mode (optional)\n"
                 "\t-H\t\t\trun in hexadecimal output mode, comma-delimited;\n"
                 "\t\t\t\t  don't scale vars (optional)\n"
                 "\t-e\t\t\trun with emulator; longer timeout (optional)\n"
                 "\n"
                 "Examples:\n"
                 "\t./ipm_ctrl -i -a 2 -D /dev/ttyS0 -c RECORD?\n"
                 "\t    Interactive, Send a single RECORD? query to iPM "
                 "address 2 on /dev/ttyS0\n"
                 "\t./ipm_ctrl -i -a 1 -D /dev/ttyS0 -c MEASURE? -H\n"
                 "\t    Interactive, Send a single MEASURE? query to iPM "
                 "address 1 on /dev/ttyS0\n\t    and output hex values\n"
                 "\t./ipm_ctrl -i\n"
                 "\t    Interactive, Start menu-based control of iPM\n"
                 "\t./ipm_ctrl -m 1 -r 10 -n 2 -0 0,5,30101 -1 2,5,30102"
                 " -D /dev/ttyS0\n\t    Launch full application with bus "
                 "identification at\n\t    two addresses, initialization, "
                 "periodic data queries and transmission\n"
                 "\t    to network IP port.\n"
                 "\nRun as root to configure serial port and exit\n";
}


void processArgs(int argc, char *argv[])
{
    int opt;
    int errflag = 0, nopt = 0;
    bool D = false;
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
    while((opt = getopt(argc, argv, ":D:m:r:b:n:0:1:2:3:4:5:6:7:a:c:ivHe"))
           != -1)
    {
        nopt++;
        switch(opt)
        {
            case 'D':  // Device the iPM is connected to
                D = true;
                ipm.setDevice(optarg);
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
            case 'H': // Run in hexadecimal output mode, comma delimited
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

    if (errflag or (geteuid() != 0 and
        not i and (not nopt or not m or not r or not n)))
    {
        Usage();
        exit(1);
    }
}


int main(int argc, char * argv[])
{

    processArgs(argc, argv);

#ifdef __linux__
    // If on a linux machine and outb function exists, configure serial port
    // This command only works if this program is run with sudo. Since we do
    // not want to run with sudo in general, exit after this command is run.
    if ((geteuid() == 0) &&  // Running as root
        (not ioperm(0x1E9, 3, 1))) {  // serial port not configured
        std::cout << "Configuring serial port... " << std::endl;
        outb(0x00, 0x1E9);  // Note order is DATA, ADDRESS, but at command line
        outb(0x3E, 0x1EA);  // outb takes address data eg. sudo outb 0x1E9 0x00
        std::cout << " done." << std::endl;
        return 0;
    }
#endif

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

    int fd = ipm.open_port(ipm.Device());

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
        return 0;
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
