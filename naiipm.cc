/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <bitset>
#include <cstdio>
#include <regex>

#include "naiipm.h"

naiipm::naiipm():_interactive(false)
{
    // unit conversions
    _deci = 0.1;
    _milli = 0.001;

    // Map message to expected response
    _ipm_commands =
    {
        { "OFF",        "OK\n"},       // Turn Device OFF
        { "RESET",      "OK\n"},       // Turn Device ON (reset)
        { "SERNO?",     "^[0-9]{6}\n$"}, // Query Serial number (which changes)
        { "VER?",       "VER A022(L) 2018-11-13\n"}, // Query Firmware Ver
        { "TEST",       "OK\n"},       // Execute build-in self test
        { "BITRESULT?", "24\n"},       // Query self test result
        { "ADR",        ""},           // Device Address Selection
        { "MEASURE?",   "34\n"},       // Device Measurement
        { "STATUS?",    "12\n"},       // Device Status
        { "RECORD?",    "68\n"},       // Device Statistics
    };

    // Initialize the binary data map
    _ipm_data["BITRESULT?"] = _bitdata;   // Query self test result
    _ipm_data["MEASURE?"] = _measuredata; // Device Measurement
    _ipm_data["STATUS?"] = _statusdata;   // Device Status
    _ipm_data["RECORD?"] = _recorddata;   // Device Statistics

    _recordCount = 0;

}

naiipm::~naiipm()
{
}

//Initialize the iPM device. Returns a verified list of device addresses
// that may be shorter than the list passed in if some addresses did not
// pass verification.
bool naiipm::init(int fd)
{

    flush(fd);

    // Verify device existence at all addresses
    std::cout << "This ipm has " << numAddr() << " active addresses"
        << std::endl;
    for (int i=0; i < atoi(numAddr()); i++)
    {
        std::string msg;
        std::cout << "Info for address " << i << " is " << addr(i) << ","
            << scaleflag(i) << "," << procqueries(i) << "," << addrport(i)
            << std::endl;
        bool status = setActiveAddress(fd, i);
        // TBD: If setActiveAddress command failed (corruption) during init?

        // Turn Device OFF, wait > 100ms then turn ON to reset state
        msg = "OFF";
        if(not send_command(fd, msg)) { return false; }

        unsigned int microseconds = 110000;
        usleep(microseconds);  // Wait > 100ms

        msg = "RESET";
        if(not send_command(fd, msg)) { return false; }

        // Query Serial Number
        msg = "SERNO?";
        if(not send_command(fd, msg))
        {
            return false;
            // TBD: If SERNO query fails, remove address from list, but
            // continue with other addresses. decrease numAddr by 1 and
            // remove addr(i) from array. Log error.
        }
        // Query Firmware Version
        msg = "VER?";
        if(not send_command(fd, msg)) { return false; }

        // Execute build-in self test
        msg = "TEST";
        if(not send_command(fd, msg)) { return false; }
        msg = "BITRESULT?";
        if(not send_command(fd, msg)) { return false; }
        status = parseData(msg, i);
    }

    return true;
}

// Establish connection to iPM
int naiipm::open_port(const char *port)
{
    int fd; // file description for the serial port
    struct termios port_settings; // structure to store the port settings in

    fd = open(Port(), O_RDWR | O_NOCTTY | O_NONBLOCK); // read/write

    if (fd == -1) // if open is unsuccessful
    {
        std::cout << "open_port: Unable to open " << port << std::endl;
        exit(1);
    }
    else
    {
        // Confirm we are pointing to a serial device
        if (!isatty(fd))
        {
            std::cout << port << " is not a serial device" << std::endl;
            close_port(fd);
            exit(1);
        }

        // Get serial port configuration
        if (tcgetattr(fd, &port_settings) == -1)
        {
            std::cout << "Failed to get serial port config" << std::endl;
            exit(2);
        }

        // set bit input and output baud rate
        if (atoi(_baudRate) != 115200)
        {
            // TBD: default to 115200
            exit(1);
        }
        if (cfsetspeed(&port_settings, B115200) == -1)
        {
            std::cout << "Failed to set baud rate to 115200" << std::endl;
        }

        port_settings.c_cflag &= ~CRTSCTS; // turn off hardware flow control
        port_settings.c_cflag |= CS8; // 8n1 (8bit,no parity,1 stopbit)
        port_settings.c_cc[VTIME] = 1; // 0.1 second timeout

        // apply settings to the port
        if (tcsetattr(fd, TCSANOW, &port_settings) == -1)
        {
            std::cout << "Failed to set serial attributes" << std::endl;
        }

        std::cout << "Port " << port << " is open." << std::endl;
    }

    return(fd);
} //open_port

void naiipm::close_port(int fd)
{
    close(fd);
}

// Create UDP socket to send packets to nidas
void naiipm::open_udp(const char *ip, int port)
{
    //  AF_INET for IPv4/ AF_INET6 for IPv6
    //  SOCK_STREAM for TCP / SOCK_DGRAM for UDP
    _sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (_sock < 0)
    {
        std::cout << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    memset(&_servaddr, 0, sizeof(_servaddr));
    _servaddr.sin_family = AF_INET;
    _servaddr.sin_port = htons(port);
    _servaddr.sin_addr.s_addr = inet_addr(ip);
}

// Send a UDP message to nidas
void naiipm::send_udp(const char *buf)
{
    std::cout << "sending UDP string " << buf;
    if (sendto(_sock, (const char *)buf, strlen(buf), 0,
            (const struct sockaddr *) &_servaddr, sizeof(_servaddr)) == -1)
    {
        std::cout << "Sending packet to nidas returned error " << errno
            << std::endl;
    }
}

// Close UDP port
void naiipm::close_udp()
{
    close(_sock);
}

// Parse the addrInfo block from the command line
// Block contains addr,scaleflag,procqueries,port
void naiipm::parse_addrInfo(int i)
{
    char *addrinfo = addrInfo(i);
    if (Debug())
    {
        std::cout << "Parsing info block " << addrinfo << std::endl;
    }
    char *ptr = strtok(addrinfo, ",");
    setAddr(i, ptr);
    if (Debug())
    {
        std::cout << "addr: " << addr(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setScaleFlag(i, ptr);
    if (scaleflag(i) != 1 && scaleflag(i) != 0)  // only two valid values
    {
        std::cout << "scaleflag " << scaleflag(i) << " is not a valid value.";
        std::cout << " Set to 1 (scaling enabled)" << std::endl;
        char flag = '1';
        setScaleFlag(i, &flag);
    }
    if (Debug())
    {
        std::cout << "scaleflag: " << scaleflag(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setProcqueries(i, ptr);
    if (Debug())
    {
        std::cout << "procqueries: " << procqueries(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setAddrPort(i, ptr);
    if (Debug())
    {
        std::cout << "addrport: " << addrport(i) << std::endl;
    }
}

// Set active address
bool naiipm::setActiveAddress(int fd, int i)
{
    std::string msg = "ADR";
    char msgarg[8];

    snprintf(msgarg, 8, "%d", addr(i));
    if(not send_command(fd, msg, msgarg)) { return false; }

    return true;
}

// Set the recordPeriod per measureRate so can call RECORD only every
// recordFreq times that MEASURE / STATUS are called.
// measureRate is in hz; recordPeriod in minutes
// So if measureRate is 1 and recordPeriod is min, only send RECORD command
// after send 600 MEASURE/STATUS commands.
void naiipm::setRecordFreq()
{
     _recordFreq = (int)(atoi(_recordPeriod)*60.0) * atoi(_measureRate);
}


// Determine queries to send and process.
bool naiipm::loop(int fd)
{
    std::string msg;

    _recordCount++;

    for (int i=0; i < atoi(numAddr()); i++)
    {

        // ‘procqueries’ is an integer representation of 3-bit Boolean
        // field indicating whether query responses [RECORD,MEASURE,STATUS]
        // should be processed and variables included in a processed data
        // file.
        //     d’3 (b’011) indicates that MEASURE+STATUS are processed.
        //     d’5 (b’101) indicates that RECORD+STATUS are processed.
        int procq = procqueries(i);
        std::bitset<4> x('\0' + procq);
        if (Debug())
        {
            std::cout << ": [" << procq << "] " << '\0' + procq << " : " << x
                << std::endl;
        }

        if (setActiveAddress(fd, i))
        {
            // Per software requirements, MEASURE? Is queried first, followed
            // by STATUS?, followed by RECORD?
            std::bitset<4> m = x;
            if ((m &= 0b0010) == 2)  // MEASURE command requested
            {
                msg = "MEASURE?";
                if(not send_command(fd, msg)) { return false; }
                bool status = parseData(msg, i);
            }
            std::bitset<4> s = x;
            if ((s &= 0b0001) == 1)  // STATUS command requested
            {
                msg = "STATUS?";
                if(not send_command(fd, msg)) { return false; }
                bool status = parseData(msg, i);
            }
            std::bitset<4> r = x;
            if ((r &= 0b0100) == 4)  // RECORD command requested
            {
                if (_recordCount >= _recordFreq)
                {
                    msg = "RECORD?";
                    if(not send_command(fd, msg)) { return false; }
                    bool status = parseData(msg, i);
                    _recordCount = 0;
                }
            }
        }
    }

    return true;

}

// rate for STATUS and MEASURE is quicker than RECORD, so use that as the base.
// Rather than setting a timer to get responses at the exact interval requested,
// since this is housekeeping data and timing is not critical, set sleep so we
// get at least one response per requested time period. From test runs on
// Gigajoules, request for all three commands returns in ~0.2 seconds, so
// subtract that from requested rate.
void naiipm::sleep()
{
    // TBD: Will likely need to adjust this when the iPM is mounted on the
    // aircraft.
    _sleeptime = ((1000000 / atoi(_measureRate)) - 200000);  // usec
    usleep(_sleeptime);
}

void naiipm::setData(std::string cmd, int len)
{
    // free the previous binary data memory space
    // and update the map to point to the new space
    memcpy(_ipm_data[cmd], buffer, len);
}

// read response from iPM
void naiipm::get_response(int fd, int len, bool bin)
{
    int n = 0, r = 0;
    char c;
    int ret;
    fd_set set;
    buffer[0] = '\0';
    if (Debug())
    {
        std::cout << "Expected response length " << len << std::endl;
    }
    while (true)
    {
        // If iPM never returns expected number of bytes, timeout
        struct timeval timeout;
        FD_ZERO(&set);
        FD_SET(fd, &set);

        // During operation, the iPM timeout should be 100ms. When developing
        // using the Python emulator, this is too short, so add a second.
        int tout;
        if (Emulate())
        {
            tout = 1;  // Add a second to timeout when developing
        } else
        {
            tout = 0;  // Deployment mode - leave timeout at 100ms
        }
        timeout.tv_sec = tout;
        timeout.tv_usec = 100000;  // 100ms timeout

        int rv = select(fd + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("select()");
            break; /* an error occurred */
        }
        else if (rv == 0)
        {
            if (Debug())
            {
                std::cout << "timeout" << std::endl; /* a timeout occured */
            }
            break;
        }
        else
        {
            ret = read(fd, &c, 1);
        }

        if (ret > 0)  // successful read
        {
            std::bitset<8> x(c);
            unsigned int i = (unsigned char)c;
            //if (Debug())
            //{
            std::cout << n+1 << ": [" << c << "] " << std::dec << i << ",";
            std::cout << std::hex << i << " : " << x << std::dec << std::endl;
            //}
            buffer[n] = c;
            // linefeed is a valid value mid-binary query so only test
            // if NOT reading binary data
            if (c == '\n' && not bin) { // found linefeed
                break;
            }
            n++;
        } else if (ret != -1)  // read did not return timeout
        {
            std::cout << "unknown response " << c << std::endl;
        } else if (ret == -1)  // Resource temporarily unavailable
        {
            std::cout << "Read from iPM returned error " << strerror(errno)
                << std::endl;
        }


        // if receive len chars without an endline, return anyway
        // (handles binary data)
        if (n > (len - 1))
        {
            n--;  // decrement char count since never found linefeed
            break;
        }
    }

    buffer[n+1] = '\0'; // terminate the string

    if (not bin && n+1 != len)
    {
       if (len != 0) // Handle edge case - a hack, clean up later
       {
           // n and len should be the same for ascii data
           std::cout << "Didn't receive all expected chars: received " <<
               n+1 << ", expected " << len << std::endl;
       }
    }
}

// send command to iPM and verify response
bool naiipm::send_command(int fd, std::string msg, std::string msgarg)
{
    // Confirm command is in list of acceptable command
    if (not verify(msg)) {return false;}

    if (Debug())
    {
        std::cout << "Got message " << msg << std::endl;
    }
    // Find expected response for this message
    auto response = _ipm_commands.find(msg);
    std::string expected_response = response->second;
    if (Debug())
    {
        std::cout << "Expect response " << expected_response << std::endl;
    }

    // Send message to ipm
    if (msgarg != "")
    {
        msg.append(' ' + msgarg);
    }
    std::string sendmsg = msg + "\n";  // Add linefeed to end of command
    if (Debug())
    {
        std::cout << "Sending message " << sendmsg << std::endl;
        std::cout << "of length " << sendmsg.length() << std::endl;
    }
    write(fd, sendmsg.c_str(), sendmsg.length());
    if (tcdrain(fd) == -1)  // wait for write to complete
    {
        std::cout << errno << std::endl;
    }
    if (Debug())
    {
        std::cout << "Write completed" << std::endl;
    }

    if (msg == "SERNO?")  // Serial # changes frequently, so just check regex
    {
        // Since SERNO uses a regex, can't get expected response length by
        // checking length of expected response. So just hardcode as length of
        // 7.
        // Get response from ipm
        get_response(fd, 7, false);
        if (Debug())
        {
            std::cout << "Received " << buffer << std::endl;
        }
        std::string str = (std::string)buffer;
        std::regex r(expected_response);
        std::smatch m;
        if (not std::regex_match(str, m, r))
        {
            std::cout << "Device command " << msg << " did not return "
                << "expected response " << expected_response <<  std::endl;
            return false;  // command failed
        }
    }
    else
    {
        // Get response from ipm
        get_response(fd, int(expected_response.length()), false);
        if (Debug())
        {
            std::cout << "Received " << buffer << std::endl;
        }

        if(buffer != expected_response)
        {
            std::cout << "Device command " << msg << " did not return "
                << "expected response " << expected_response <<  std::endl;
            return false;  // command failed
        }
    }

    // Read binary part of response. Length of binary response was
    // returned as first response to query.
    if (_ipm_data.find(msg) != _ipm_data.end()) // cmd returns data
    {
        int binlen = std::stoi(buffer);
        if (Debug())
        {
            std::cout << "Now get " << binlen << " bytes" << std::endl;
        }
        get_response(fd, binlen, true);  // true indicates reading binary data
        setData(msg, binlen);
    }

    flush(fd);

    return true;  // command succeeded
}

// Flush serial port
void naiipm::flush(int fd)
{
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
        std::cout << "Flush returned error " << errno << std::endl;
    }

}

// List use options in interactive mode
void naiipm::printMenu()
{
    std::cout << "=========================================" << std::endl;
    std::cout << "Type one of the following iPM commands or" << std::endl;
    std::cout << "enter 'q' to quit" << std::endl;
    std::cout << "=========================================" << std::endl;
    for (auto msg : _ipm_commands) {
        std::cout << msg.first << std::endl;
    }
}

bool naiipm::verify(std::string cmd)
{
    // Confirm command is in list of acceptable command
    if (not _ipm_commands.count(cmd))
    {
        std::cout << "Command " << cmd << " is invalid. Please enter a " <<
            "valid command" << std::endl;
        return false;
    } else {
        return true;
    }
}

bool naiipm::readInput(int fd)
{
    std::string cmd = "";

    // Request user input
    std::cin >> (cmd);
    if (Debug())
    {
        std::cout << "User requested " << cmd << std::endl;
    }

    // Catch exit request
    if (cmd.compare("q") == 0)
    {
        if (Debug())
        {
            std::cout << "Exiting..." << std::endl;
        }
        return false;
    }

    // Confirm command is in list of acceptable command
    // If it is not, ask user to enter another command
    if (not verify(cmd)) {return true;}

    // If command is valid, send to ipm
    const char *cmdInput = cmd.c_str();
    if (cmd.compare("ADR") == 0)
    {
        // ADR is only command that requires a second component
        std::string addr = "";
        std::cout << "Which address would you like to activate (0-7)?"
            << std::endl;
        std::cin >> (addr);
        std::cout << "User requested " << cmd << " " << addr << std::endl;
        if (not send_command(fd, (char *)cmdInput, addr)) { return false; }
    } else {
        if (not send_command(fd, (char *)cmdInput)) { return false; }
    }

    // Check if command has binary data component. If so, parse it into
    // it's component variables.
    if (_ipm_data.find(cmd) != _ipm_data.end())  // found cmd in binary map
    {
        // Parse binary data
        parseData(cmd, 0);  // In interactive mode, only one address is used
    }

    return true;
}

bool naiipm::parseData(std::string cmd, int adr)
{
    if (Debug())
    {
        std::cout << '{' << cmd << '}' << std::endl;
        std::cout << "In parseData: Info for address " << adr << " is " << addr(adr) << ","
            << scaleflag(adr) << "," << procqueries(adr) << "," << addrport(adr)
            << std::endl;
    }
    // retrieve binary data
    char* data = getData(cmd); // data content

    // Create some pointers to access data of various lengths
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;

    // parse data
    if (cmd == "BITRESULT?") {
        parseBitresult(sp);
    }

    if (cmd == "RECORD?") {
        parseRecord(cp, sp, lp);
        if (Debug())
        {
            std::cout << record.TIME/60000 << " minutes since power-up"
                << std::endl;
        }
        if (scaleflag(adr) == 1) {
            snprintf(buffer, 255, "RECORD,%u,%u,%u,%u,%u,%u,%.2f,%.2f,%.2f,"
                "%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f,"
                "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                "%u\r\n",
                record.EVTYPE, record.OPSTATE, record.POWERCNT,
                record.TIME, record.TFLAG, record.CFLAG,
                record.VRMSMINA * _deci, record.VRMSMAXA * _deci,
                record.VRMSMINB * _deci, record.VRMSMAXB * _deci,
                record.VRMSMINC * _deci, record.VRMSMAXC * _deci,
                record.FREQMIN * _deci, record.FREQMAX * _deci,
                record.VDCMINA * _milli, record.THDMAXA * _milli,
                record.THDMINB * _milli, record.VDCMAXB * _milli,
                record.VDCMINC * _milli, record.VDCMAXC * _milli,
                record.THDMINA * _deci, record.THDMAXA * _deci,
                record.THDMINB * _deci, record.THDMAXB * _deci,
                record.THDMINC * _deci, record.THDMAXC * _deci,
                record.VPKMINA * _deci, record.VPKMAXA * _deci,
                record.VPKMINB * _deci, record.VPKMAXB * _deci,
                record.VPKMINC * _deci, record.VPKMAXC * _deci,
                record.CRC);
        } else {
            snprintf(buffer, 255, "RECORD,%02x,%02x,%08x,%08x,%08x,%08x,%04x,"
                "%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,"
                "%04x,%02x,%02x,%02x,%02x,%02x,%02x,%04x,%04x,%04x,%04x,%04x,"
                "%04x,%08x\r\n",
                record.EVTYPE, record.OPSTATE, record.POWERCNT, record.TIME,
                record.TFLAG, record.CFLAG, record.VRMSMINA, record.VRMSMAXA,
                record.VRMSMINB, record.VRMSMAXB, record.VRMSMINC,
                record.VRMSMAXC, record.FREQMIN, record.FREQMAX,
                record.VDCMINA, record.THDMAXA, record.THDMINB, record.VDCMAXB,
                record.VDCMINC, record.VDCMAXC, record.THDMINA, record.THDMAXA,
                record.THDMINB, record.THDMAXB, record.THDMINC, record.THDMAXC,
                record.VPKMINA, record.VPKMAXA, record.VPKMINB, record.VPKMAXB,
                record.VPKMINC, record.VPKMAXC, record.CRC);
        }
        send_udp(buffer);
    }

    if (cmd == "MEASURE?") {
        parseMeasure(cp, sp);
        if (scaleflag(adr) == 1) {
            snprintf(buffer, 255, "MEASURE,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                 "%.2f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u\r\n",
                 measure.FREQ * _deci, measure.TEMP * _deci,
                 measure.VRMSA * _deci, measure.VRMSB * _deci,
                 measure.VRMSC * _deci, measure.VPKA * _deci,
                 measure.VPKB * _deci, measure.VPKC * _deci,
                 measure.VDCA * _milli, measure.VDCB * _milli,
                 measure.VDCC * _milli, measure.PHA * _deci,
                 measure.PHB * _deci, measure.PHC * _deci,
                 measure.THDA * _deci, measure.THDB * _deci,
                 measure.THDC * _deci, measure.POWEROK);
        } else {
            snprintf(buffer, 255, "MEASURE,%04x,%04x,%04x,%04x,%04x,%04x,%04x,"
                 "%04x,%04x,%04x,%04x,%04x,%04x,%04x,%02x,%02x,%02x,%02x\r\n",
                 measure.FREQ, measure.TEMP,
                 measure.VRMSA, measure.VRMSB,
                 measure.VRMSC, measure.VPKA,
                 measure.VPKB, measure.VPKC,
                 measure.VDCA, measure.VDCB,
                 measure.VDCC, measure.PHA,
                 measure.PHB, measure.PHC,
                 measure.THDA, measure.THDB,
                 measure.THDC, measure.POWEROK);
        }
        send_udp(buffer);
    }

    if (cmd == "STATUS?") {
        parseStatus(cp, sp);
        if (scaleflag(adr) == 1) {
            snprintf(buffer, 255, "STATUS,%u,%u,%u,%u,%u\r\n", status.OPSTATE,
                status.POWEROK, status.TRIPFLAGS, status.CAUTIONFLAGS,
                status.BITSTAT);
        } else {
            snprintf(buffer, 255, "STATUS,%02x,%02x,%04x,%04x,%04x\r\n",
                status.OPSTATE, status.POWEROK, status.TRIPFLAGS,
                status.CAUTIONFLAGS, status.BITSTAT);
        }
        send_udp(buffer);
    }

    return true;
}

void naiipm::parseRecord(uint8_t *cp, uint16_t *sp, uint32_t *lp)
{
    record.EVTYPE = cp[0];    // Event Type
    // Event Type: 0 - Max Interval; 1 - Power Up; 2 - Power Down; 3 - Off;
    //             4 - Reset; 5 - Trip; 6 - Fail; 7 - Output On; 8 - Output Off

    record.OPSTATE = cp[1];    // Operating State
    // There is a typo in the programming manual. Power up count should
    // be 4 bytes and elapsed time should start at byte 6 and be 4 bytes
    // as coded here.
    record.POWERCNT = (((long)sp[2]) << 16) | sp[1];  // Power Up Count
    record.TIME = (((long)sp[4]) << 16) | sp[3]; // Power Up Time (1 ms)
    record.TFLAG = (((long)sp[6]) << 16) | sp[5];  // Trip Flag
    record.CFLAG = (((long)sp[8]) << 16) | sp[7];  // Caution Flag
    record.VRMSMINA = sp[9];   // Phase A Voltage Min (0.1 V rms)
    record.VRMSMAXA = sp[10];  // Phase A Voltage Max (0.1 V rms)
    record.VRMSMINB = sp[11];  // Phase B Voltage Min (0.1 V rms)
    record.VRMSMAXB = sp[12];  // Phase B Voltage Max (0.1 V rms)
    record.VRMSMINC = sp[13];  // Phase C Voltage Min (0.1 V rms)
    record.VRMSMAXC = sp[14];  // Phase C Voltage Max (0.1 V rms)
    record.FREQMIN = sp[15];   // Frequency Min (0.1 Hz)
    record.FREQMAX = sp[16];   // Frequency Max (0.1 Hz)
    record.VDCMINA = sp[17];   // Phase A DC Content Min (1 mV)
    record.VDCMAXA = sp[18];   // Phase A DC Content Max (1 mV)
    record.VDCMINB = sp[19];   // Phase B DC Content Min (1 mV)
    record.VDCMAXB = sp[20];   // Phase B DC Content Max (1 mV)
    record.VDCMINC = sp[21];   // Phase C DC Content Min (1 mV)
    record.VDCMAXC = sp[22];   // Phase C DC Content Max (1 mV)
    record.THDMINA = cp[46];   // Phase A Distortion Min (0.1 %)
    record.THDMAXA = cp[47];   // Phase A Distortion Max (0.1 %)
    record.THDMINB = cp[48];   // Phase B Distortion Min (0.1 %)
    record.THDMAXB = cp[49];   // Phase B Distortion Max (0.1 %)
    record.THDMINC = cp[50];   // Phase C Distortion Min (0.1 %)
    record.THDMAXC = cp[51];   // Phase C Distortion Max (0.1 %)
    record.VPKMINA = sp[26];   // Phase A Peak Voltage Min (0.1 V)
    record.VPKMAXA = sp[27];   // Phase A Peak Voltage Max (0.1 V)
    record.VPKMINB = sp[28];   // Phase B Peak Voltage Min (0.1 V)
    record.VPKMAXB = sp[29];   // Phase B Peak Voltage Max (0.1 V)
    record.VPKMINC = sp[30];   // Phase C Peak Voltage Min (0.1 V)
    record.VPKMAXC = sp[31];   // Phase C Peak Voltage Max (0.1 V)
    record.CRC = lp[16];       // CRC-32
}

void naiipm::parseBitresult(uint16_t *sp)
{
    // There is a typo in the programming manual. Byte should start at
    // zeroC
    float bitStatus = sp[0];
    float hREFV = sp[1];  // Half-Ref voltage (4.89mV)
    float VREFV = sp[2];  // VREF voltage (4.89mV)
    float FIVEV = sp[3];  // +5V voltage (9.78mV)
    float FIVEVA = sp[4]; // +5VA voltage (9.78mV)
    float RDV = sp[5];    // Relay Drive Voltage (53.76mV)
    // bytes 13-14 and 15-16 are reserved
    float ITVA = sp[8];   // Phase A input test voltage (4.89mV) - reserved
    float ITVB = sp[9];   // Phase B input test voltage (4.89mV) - reserved
    float ITVC = sp[10];  // Phase C input test voltage (4.89mV) - reserved
    float TEMP = sp[11] * 0.1;  // Temperature (0.1C)
    if (Debug())
    {
        std::cout << "iPM temperature (C) = " << TEMP << std::endl;
    }
}

void naiipm::parseMeasure(uint8_t *cp, uint16_t *sp)
{
    // There is a typo in the programming manual. Byte should start at
    // zero.
    measure.FREQ = sp[0];   // Frequency (0.1Hz)
    // bytes 3-4 are reserved
    measure.TEMP = sp[2];   // Temperature (0.1 C)
    measure.VRMSA = sp[3];  // Phase A voltage rms (0.1 V)
    measure.VRMSB = sp[4];  // Phase B voltage rms (0.1 V)
    measure.VRMSC = sp[5];  // Phase C voltage rms (0.1 V)
    measure.VPKA = sp[6];   // Phase A voltage peak (0.1 V)
    measure.VPKB = sp[7];   // Phase B voltage peak (0.1 V)
    measure.VPKC = sp[8];   // Phase C voltage peak (0.1 V)
    measure.VDCA = sp[9];   // Phase A DC component (1 mV DC)
    measure.VDCB = sp[10];  // Phase B DC component (1 mV DC)
    measure.VDCC = sp[11];  // Phase C DC component (1 mV DC)
    measure.PHA = sp[12];   // Phase A Phase Angle (0.1 deg) rel. to Phase A
    measure.PHB = sp[13];   // Phase B Phase Angle (0.1 deg) rel. to Phase B
    measure.PHC = sp[14];   // Phase C Phase Angle (0.1 deg) rel. to Phase C
    measure.THDA = cp[30];  // Phase A THD (0.1%)
    measure.THDB = cp[31];  // Phase B THD (0.1%)
    measure.THDC = cp[32];  // Phase C THD (0.1%)
    measure.POWEROK = cp[33];  // PowerOK (1 = power good, 0 = no good)
}

void naiipm::parseStatus(uint8_t *cp, uint16_t *sp)
{
    // There is a typo in the programming manual. Byte should start at
    // zero.
    status.OPSTATE = cp[0];    // Operating State
    // OpState: 0 - Off; 1 = reserved; 2 - reset; 3 - Tripped; 4 - Failed
    status.POWEROK = cp[1];    // PowerOK (1 - power good; 0 - no good)
    status.TRIPFLAGS = (((long)sp[2]) << 16) | sp[1];
    status.CAUTIONFLAGS = (((long)sp[4]) << 16) | sp[3];
    status.BITSTAT = sp[5];   // bitStatus
}
