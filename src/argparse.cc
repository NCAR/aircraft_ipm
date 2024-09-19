/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include "argparse.h"

ipmCmd commands;

ipmArgparse::ipmArgparse():_interactive(false)
{
    // Set defaults
    const char *device = "/dev/ttyS0";
    setDevice(device);
    setScaleFlag(1);  // turn on scaling by default
    const char *baud = "57600";
    setBaud(baud);
    const char *naddr = "1";  // one address by default
    setNumAddr(naddr);
    const char *undefAddr = "-1";
    setAddress(undefAddr);
    setCmd("");
}

ipmArgparse::~ipmArgparse()
{
}

void ipmArgparse::Usage()
{
    std::cout <<
        "\nUsage:\n"
        "\t-D device\tiPM connection device (Default:/dev/ttyS0)\n"
        "\t-m measurerate\tSTATUS & MEASURE collection rate (hz)\n"
        "\t-r recordperiod\tperiod of RECORD queries (minutes)\n"
        "\t-b baudrate\tbaud rate (Default:57600)\n"
        "\t-n num_addr\tnumber of active addresses on iPM\n"
        "\t-# addr,procqueries,port\n"
        "\t\t\t  - addr is the iPM address; a number 0 to n-1\n"
        "\t\t\t  - procqueries is an integer representing a 3-bit\n"
        "\t\t\t  boolean field indicating which query responses\n"
        "\t\t\t  [RECORD,MEASURE,STATUS] should be processed, eg\n"
        "\t\t\t     3 (b’011) requests MEASURE+STATUS\n"
        "\t\t\t     5 (b’101) requests RECORD+STATUS\n"
        "\t\t\t  - port which to send the output UDP string\n"
        "\t-i \t\trun in interactive mode (optional)\n"
        "\t\t\t  - When in interactive mode only -D and -b are required\n"
        "\t\t\t  - Inclusion of -a and -c will send a single command\n"
        "\t\t\t  and exit.\n"
        "\t-a \t\tset address (optional)\n"
        "\t-c \t\tset command (optional)\n"
        "\t-v \t\trun in verbose mode (optional)\n"
        "\t-H \t\trun in hexadecimal output mode, comma-delimited\n"
        "\t\t\t  don't scale vars (optional)\n"
        "\t-e \t\trun with emulator; longer timeout (optional)\n"
        "\t-S \t\tConfigure serial port and exit. Must be run as\n"
        "\t\t\t  root\n"
        "\n"
        "Examples:\n"
        "\t./ipm_ctrl -i -a 2 -D /dev/ttyS0 -c RECORD?\n"
        "\t    Interactive, Send a single RECORD? query to iPM "
        "address 2 on\n\t     /dev/ttyS0\n"
        "\t./ipm_ctrl -i -a 1 -D /dev/ttyS0 -c MEASURE? -H\n"
        "\t    Interactive, Send a single MEASURE? query to iPM "
        "address 1 on \n\t    /dev/ttyS0 and output hex values\n"
        "\t./ipm_ctrl -i\n"
        "\t    Interactive, Start menu-based control of iPM\n"
        "\t./ipm_ctrl -m 1 -r 10 -n 2 -0 0,5,30101 -1 2,5,30102"
        " -D /dev/ttyS0\n\t    Launch full application with bus "
        "identification at two\n\t     addresses, initialization, "
        "periodic data queries and\n\t    transmission to network "
        "IP port.\n\n";
}

void ipmArgparse::process(int argc, char *argv[])
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
    while((opt = getopt(argc, argv, ":D:m:r:b:n:0:1:2:3:4:5:6:7:a:c:ivHeS"))
           != -1)
    {
        nopt++;
        switch(opt)
        {
            case 'D':  // Device the iPM is connected to
                D = true;
                setDevice(optarg);
                break;
            case 'm':  // STATUS & MEASURE collection rate (hz)
                m = true;
                setRate(optarg);
                break;
            case 'r':  // Period of RECORD queries (minutes)
                r = true;
                setPeriod(optarg);
                break;
            case 'b':  // Baud rate
                b = true;
                setBaud(optarg);
                break;
            case 'n':  // Number of addresses in use on iPM
                n = true;
                setNumAddr(optarg);
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                setAddrInfo(opt-'0', optarg);
                if (not parse_addrInfo(opt-'0'))
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
                    setAddress(optarg);
                }
                break;
            case 'c':
                if (commands.verify(optarg))
                {
                    setCmd(optarg);
                } else {
                    exit(1);
                }
                break;
            case 'i':  // Run in interactive (menu) mode
                setInteractive();
                i = true;
                break;
            case 'v': // Run in verbose mode
                setVerbose();
                break;
            case 'H': // Run in hexadecimal output mode, comma delimited
                setScaleFlag(0);  // Turn off scaling
            case 'e': // Run in emulator mode
                setEmulate();
                break;
            case 'S': // Configure serial port
                configureSerialPort();
                exit(0);
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
    if (nInfo != 0 and numAddr() != nInfo)
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

    // If running as root and included -S option, will exit before get here
    if ((geteuid() == 0)) // Running as root
    {
        std::cout << "\n**** Running as root. If you are trying to configure "
            "****\n**** serial ports, please use the -S option          ****\n"
            << std::endl;
        Usage();
        exit(1);
    }
    return;
}

// If on a linux machine and outb function exists, configure serial port
// This command only works if this program is run with sudo. Since we do
// not want to run with sudo in general, exit after this command is run.
void ipmArgparse::configureSerialPort()
{
#ifdef __linux__
    if ((geteuid() == 0) {  // Running as root
        if (not ioperm(0x1E9, 3, 1)) {  // serial port not configured
            std::cout << "Configuring serial port... " << std::endl;
            outb(0x00, 0x1E9);  // Here order is DATA, ADDRESS, but at command
            outb(0x3E, 0x1EA);  // line outb takes address data eg.
                                // sudo outb 0x1E9 0x00
            std::cout << " done." << std::endl;
        } else {
            std::cout << "Serial port already configured. Nothing to do"
              << std::endl;
        }
    } else {
        std::cout << "Must be root to configure serial port" << std::endl;
    }
#else
    std::cout << "serial port configuration only works on linux" << std::endl;
#endif
}

// Parse the addrInfo block from the command line
// Block contains addr,procqueries,port
bool ipmArgparse::parse_addrInfo(int i)
{
    char *addrinfo = addrInfo(i);
    // Validate address info block with simple comma count
    std::string s = (std::string)addrinfo;
    if (std::count(s.begin(), s.end(), ',') != 2)
    {
        return false;
    }
    if (Verbose())
    {
        std::cout << "Parsing info block " << addrinfo << std::endl;
    }
    char *ptr = strtok(addrinfo, ",");
    setAddr(i, ptr);
    if (Verbose())
    {
        std::cout << "addr: " << Addr(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setProcqueries(i, ptr);
    if (Verbose())
    {
        std::cout << "procqueries: " << Procqueries(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setAddrPort(i, ptr);
    if (Verbose())
    {
        std::cout << "addrport: " << Addrport(i) << std::endl;
    }

    return true;
}
