/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include "naiipm.h"

static naiipm ipm;

void processArgs(int argc, char *argv[])
{
    int opt;
    int errflag = 0, nopt = 0;
    bool p = false;
    bool m = false;
    bool r = false;
    bool b = false;
    bool n = false;
    char c = '9';
    int nInfo = 0;

    // Options between colons require an argument
    // Options after second colon do not.
    while((opt = getopt(argc, argv, ":p:m:r:b:n:0:1:2:3:4:5:6:7:i")) != -1)
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
                break;
            case 'r':  // Period of RECORD queries (mintes)
                r = true;
                break;
            case 'b':  // Baud rate
                b = true;
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
    if (atoi(ipm.numAddr()) != nInfo)
    {
        std::cout << "-n option must match number of addresses given on " <<
            "command line" << std::endl;
        errflag++;
    }

    if (errflag or not nopt or not p or not m or not r or not b or not n)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t-p <port>\tport iPM is connected to" << std::endl;
        std::cout << "\t-m <measurerate>\tSTATUS & MEASURE collection rate "
            << " (hz)" << std::endl;
        std::cout << "\t-r <recordperiod>\tPeriod of RECORD queries (minutes)"
            << std::endl;
        std::cout << "\t-b <baudrate>\tBaud rate" << std::endl;
        std::cout << "\t-n <num_addr>\tnumber of active addresses on iPM"
            << std::endl;
        std::cout << "\t-#\tNumber 0 to n-1 followed by info block " <<
            "(-# addr, numphases, procqueries, port" << std::endl;
        std::cout << "\t-i\tRun in interactive mode" << std::endl;
        exit(1);
    }
}

bool init_device(int fd)
{
    // Turn Device OFF, wait > 100ms then turn ON to reset state
    std::string msg = "OFF";
    char msgarg[8];
    char *addrinfo;
    if(not ipm.send_command(fd, msg)) { return false; }

    sleep(.11);  // Wait > 100ms

    msg = "RESET";
    if(not ipm.send_command(fd, msg)) { return false; }

    // Verify device existence at all addresses
    std::cout << "This ipm has " << ipm.numAddr() << " active addresses"
        << std::endl;
    for (int i=0; i < atoi(ipm.numAddr()); i++)
    {
        ipm.parse_addrInfo(i);
        std::cout << "Info for address " << i << " is " << ipm.addr(i)
            << std::endl;
        msg = "ADR";
        sprintf(msgarg, "%d", ipm.addr(i));
        std::cout << "Attempting to send message " << msg << " " << msgarg
            << std::endl;
        if(not ipm.send_command(fd, msg, msgarg)) { return false; }

        // Query Serial Number
        msg = "SERNO?";
        // TBD: If SERNO query fails, remove address from list, but continue
        // with other addresses
        if(not ipm.send_command(fd, msg)) { return false; }

        // Query Firmware Version
        msg = "Ver?";
        if(not ipm.send_command(fd, msg)) { return false; }

        // Execute build-in self test
        msg = "TEST";
        if(not ipm.send_command(fd, msg)) { return false; }
        msg = "BITRESULT?";
        if(not ipm.send_command(fd, msg)) { return false; }
        //TBD: read in 24 bytes of data from test

    }

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

    // TBD: Get ip address and port from xml.
    ipm.open_udp("172.16.47.154", 30101);

    // Send a message to the server
    const char *buffer = "MEASURE,20230201T00:00:00,11,11,99,99,99,99,99\r\n";
    ipm.send_udp(buffer);

    // Close socket descriptor
    ipm.close_udp();

    ipm.close_port(fd);
    exit(1);
}


