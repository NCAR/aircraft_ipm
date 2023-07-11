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
        parse_addrInfo(i);
        std::cout << "Info for address " << i << " is " << addr(i)
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
        status = parseData(msg, 0);  // parse binary part of BITRESULT?
        std::cout << std::boolalpha << "Status is " << status << std::endl;


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
    std::cout << "sending UDP string " << buf << std::endl;
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
// Block contains addr,numphases,procqueries,port
void naiipm::parse_addrInfo(int i)
{
    char *addrinfo = addrInfo(i);
    std::cout << "Parsing info block " << addrinfo << std::endl;
    char *ptr = strtok(addrinfo, ",");
    setAddr(i, ptr);
    std::cout << "addr: " << addr(i) << std::endl;

    ptr = strtok(NULL, ",");
    setNumphases(i, ptr);
    std::cout << "numphases: " << numphases(i) << std::endl;

    ptr = strtok(NULL, ",");
    setProcqueries(i, ptr);
    std::cout << "procqueries: " << procqueries(i) << std::endl;

    ptr = strtok(NULL, ",");
    setAddrPort(i, ptr);
    std::cout << "addrport: " << addrport(i) << std::endl;
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

// Determine queries to send and process.
bool naiipm::loop(int fd)
{
    std::string msg;

    // Set the recordPeriod per measureRate so can call RECORD only every
    // recordFreq times through this loop.
    // measureRate is in hz; recordPeriod in minutes
    float recordFreq = (int)(atoi(_recordPeriod)*60.0)/atoi(_measureRate);
    _recordCount++;

    for (int i=0; i < atoi(numAddr()); i++)
        {
            // ‘numphases’ (integer) indicates whether 1 phase, or 3-phases of
            // data are to be capture. Should only be =1 or =3
            int nphases = numphases(i);
            if (nphases != 1 && nphases != 3)
            {
                //TBD: log numphases error
                exit(1);
            }

            // ‘procqueries’ is an integer representation of 3-bit Boolean
            // field indicating whether query responses [RECORD,MEASURE,STATUS]
            // should be processed and variables included in a processed data
            // file.
            //     d’3 (b’011) indicates that MEASURE+STATUS are processed.
            //     d’5 (b’101) indicates that RECORD+STATUS are processed.
            int procq = procqueries(i);
            std::bitset<4> x('\0' + procq);
            std::cout << ": [" << procq << "] " << '\0' + procq << " : " << x
                << std::endl;

            if (setActiveAddress(fd, i))
            {
                std::bitset<4> r = x;
                if ((r &= 0b0100) == 4)  // RECORD command requested
                {
	            if (_recordCount >= recordFreq)
		    {
                        msg = "RECORD?";
                        if(not send_command(fd, msg)) { return false; }
                        bool status = parseData(msg, nphases);
                        std::cout << std::boolalpha << "Status is " << status
                            << std::endl;
		        _recordCount = 0;
		    }
                }
                std::bitset<4> m = x;
                if ((m &= 0b0010) == 2)  // MEASURE command requested
                {
                    msg = "MEASURE?";
                    if(not send_command(fd, msg)) { return false; }
                    bool status = parseData(msg, nphases);
                    std::cout << std::boolalpha << "Status is " << status
                        << std::endl;
                }
                std::bitset<4> s = x;
                if ((s &= 0b0001) == 1)  // STATUS command requested
                {
                    msg = "STATUS?";
                    if(not send_command(fd, msg)) { return false; }
                    bool status = parseData(msg, nphases);
                    std::cout << std::boolalpha << "Status is " << status
                        << std::endl;
                }
            }
        }

        // rate for STATUS and MEASURE is quicker than RECORD, so use that
	// as the base. Rather than setting a timer, to get responses at the
	// exact interval requested, since this is housekeeping data and timing
	// is not critical, set sleep so we get at least one response per
	// requested time period. From test runs on Gigajoules, request for all
	// three commands returns in ~0.2 seconds, so subtract that from
	// requested rate.
	// TBD: Will likely need to adjust this when the iPM is mounted on the
	// aircraft.
	// Also fix so if measurerate is 5 in XML, that gets us 5 samples
	// per second.
        usleep((atoi(_measureRate) - 0.2) * 1000000);

    return true;

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
    std::cout << "len " << len << std::endl;
    while (true)
    {
	// If iPM never returns expected number of bytes, timeout
        struct timeval timeout;
        FD_ZERO(&set);
        FD_SET(fd, &set);

        timeout.tv_sec = 0;  // 100ms timeout
        timeout.tv_usec = 100000;

        int rv = select(fd + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("select()");
            break; /* an error occurred */
        }
        else if (rv == 0)
        {
            std::cout << "timeout" << std::endl; /* a timeout occured */
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
            std::cout << n+1 << ": [" << c << "] " << i << " : " << x
                << std::endl;
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
            std::cout << "Read from iPM returned error " <<strerror(errno)
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

    std::cout << "Got message " << msg << std::endl;
    // Find expected response for this message
    auto response = _ipm_commands.find(msg);
    std::string expected_response = response->second;
    std::cout << "Expect response " << expected_response << std::endl;

    // Send message to ipm
    if (msgarg != "")
    {
        msg.append(' ' + msgarg);
    }
    std::string sendmsg = msg + "\n";  // Add linefeed to end of command
    std::cout << "Sending message " << sendmsg << std::endl;
    std::cout << "of length " << sendmsg.length() << std::endl;
    write(fd, sendmsg.c_str(), sendmsg.length());
    if (tcdrain(fd) == -1)  // wait for write to complete
    {
        std::cout << errno << std::endl;
    }
    std::cout << "Write completed" << std::endl;

    // Get response from ipm
    get_response(fd, int(expected_response.length()), false);
    std::cout << "Received " << buffer << std::endl;

    if (msg == "SERNO?")  // Serial # changes frequently, so just check regex
    {
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
    else if(buffer != expected_response)
    {
        std::cout << "Device command " << msg << " did not return "
            << "expected response " << expected_response <<  std::endl;
        return false;  // command failed
    }

    // Read binary part of response. Length of binary response was
    // returned as first response to query.
    if (_ipm_data.find(msg) != _ipm_data.end()) // cmd returns data
    {
        int binlen = std::stoi(buffer);
        std::cout << "Now get " << binlen << " bytes" << std::endl;
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
    std::cout << "User requested " << cmd << std::endl;

    // Catch exit request
    if (cmd.compare("q") == 0)
    {
        std::cout << "Exiting..." << std::endl;
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
        // TBD: Determine desired funtionality re: 1- or 3-phase here
        parseData(cmd, 1); // Hardcode 1-phase in interactive mode for now
    }

    return true;
}

bool naiipm::parseData(std::string cmd, int nphases)
{
    // nphases can be 1 or 3. Any other value is ignored.
    // retrieve binary data
    std::cout << '{' << cmd << '}' << std::endl;
    char* data = getData(cmd); // data content

    // Create some pointers to access data of various lengths
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;

    // parse data
    if (cmd == "BITRESULT?") {
      short temperature = (((short)data[23] & 0xFF) << 8) | data[22] & 0xFF;
      std::cout << "iPM temperature (C) = " << temperature*.1 << std::endl;
    }

    if (cmd == "RECORD?" && nphases == 1) {
        record_1phase.FREQMAX = sp[16];
        record_1phase.FREQMIN = sp[15];
        record_1phase.VRMSMAXA = sp[10];
        record_1phase.VRMSMINA = sp[9];
        record_1phase.VPKMAXA = sp[27];
        record_1phase.VPKMINA = sp[26];
        record_1phase.VDCMAXA = sp[18];
        record_1phase.VDCMINA = sp[17];
        record_1phase.THDMAXA = cp[47];
        record_1phase.THDMINA = cp[46];
        // There is a typo in the programming manual. Power up count should
        // be 4 bytes and elapsed time should start at byte 6 and be 4 bytes
        // as coded here.
        record_1phase.TIME = (((long)sp[4]) << 16) | sp[3];
        std::cout << record_1phase.TIME/60000 << " minutes since power-up"
            << std::endl;
        record_1phase.EVTYPE = cp[0];
        snprintf(buffer, 255, "RECORD,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                "%.4f,%.4f,%.2f,%.2f,%.2f,%.2f\r\n",
                record_1phase.FREQMAX * _deci, record_1phase.FREQMIN * _deci,
                record_1phase.VRMSMAXA * _deci, record_1phase.VRMSMINA * _deci,
                record_1phase.VPKMAXA * _deci, record_1phase.VPKMINA * _deci,
                record_1phase.VDCMAXA * _milli, record_1phase.VDCMINA * _milli,
                record_1phase.THDMAXA * _deci, record_1phase.THDMINA * _deci,
                (float)record_1phase.TIME, (float)record_1phase.EVTYPE
                );
        send_udp(buffer);
    }

    if (cmd == "MEASURE?" && nphases == 1) {
        // There is a typo in the programming manual. Byte should start at
	// zero.
        measure_1phase.FREQ = sp[0];
        measure_1phase.VRMSA = sp[3];
        measure_1phase.VPKA = sp[6];
        measure_1phase.VDCA = sp[9];
        measure_1phase.PHA = sp[12];
        measure_1phase.THDA = cp[30];
        measure_1phase.POWEROK = cp[33];
        snprintf(buffer, 255, "MEASURE,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,"
                "%d\r\n",
                measure_1phase.FREQ * _deci, measure_1phase.VRMSA * _deci,
                measure_1phase.VPKA * _deci, measure_1phase.VDCA * _milli,
		measure_1phase.PHA * _deci, measure_1phase.THDA * _deci,
                (int)measure_1phase.POWEROK
                );
        send_udp(buffer);
    }

    // TBD: Implement parsing of 3-phase data.

    if (cmd == "STATUS?") {  // STATUS returns same UDP packet for both phases
	status.OPSTATE = cp[0];
	status.TRIPFLAGS = (((long)sp[2]) << 16) | sp[1];
	status.CAUTIONFLAGS = (((long)sp[4]) << 16) | sp[3];
        snprintf(buffer, 255, "STATUS,%d,%d,%d\r\n", status.OPSTATE,
		 status.TRIPFLAGS, status.CAUTIONFLAGS);
        send_udp(buffer);
    }

    return true;
}
