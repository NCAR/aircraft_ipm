/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <bitset>
#include <cstdio>
#include <regex>

#include "naiipm.h"
#include "src/measure.h"
#include "src/status.h"
#include "src/record.h"

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
    _badData = 0;

    // Set defaults
    setScaleFlag(1);  // turn on scaling by default
    const char *baud = "115200";
    setBaud(baud);
    const char *device = "/dev/ttyS0";
    setDevice(device);
    const char *naddr = "1";  // one address by default
    setNumAddr(naddr);
    const char *undefAddr = "-1";
    setAddress(undefAddr);
    setCmd("");
}

naiipm::~naiipm()
{
}

// If on a linux machine and outb function exists, configure serial port
// This command only works if this program is run with sudo. Since we do
// not want to run with sudo in general, exit after this command is run.
void naiipm::configureSerialPort()
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

//Initialize the iPM device. Returns a verified list of device addresses
// that may be shorter than the list passed in if some addresses did not
// pass verification.
bool naiipm::init(int fd)
{

    flush(fd);

    // Verify device existence at all addresses
    std::cout << "This ipm should have " << numAddr() << " active address(es)"
        << std::endl;
    for (int i=0; i < numAddr(); i++)
    {
        std::string msg;
        std::cout << "Info for address " << i << " is " << addr(i) << ","
            << procqueries(i) << "," << addrport(i)
            << std::endl;

	// When installed on the GV (as opposed to in the lab), on power up
	// the iPM frequently comes up in a hung state (returns nothing in
	// response to sent commands). The working theory is that there is some
	// junk on the port that is corrupting commands being sent. Attempt to
	// clear this by repeatedly sending the ADR and VER? commands up to 10
	// times.
	bool status;
	for (int j=0; j < 10; j++)
	{
            status = setActiveAddress(fd, addr(i));

            // Query Firmware Version
            msg = "VER?";
            if((status = send_command(fd, msg))) {  // success so stop iterating
		std::cout << "Took " << i << " ADR commands to clear iPM on" <<
		    " init" << std::endl;
	        break;
	    }

	    // Wait half a second and try again
            usleep(500000);  // 0.5 seconds
	}
        if(not status)
        {
            std::cout << "Unable to set active address to " << addr(i) <<
                ". Skipping adress " << addr(i) << "for this iteration" <<
                std::endl;
            continue;
        }

        // Turn Device OFF, wait > 100ms then turn ON to reset state
        msg = "OFF";
        if(not send_command(fd, msg)) {
            // OFF query failed, so remove address from active address list
            rmAddr(i);
            i--; // back up to where next addrinfo is now stored
            continue;
        }

        unsigned int microseconds = 110000;
        usleep(microseconds);  // Wait > 100ms

        msg = "RESET";
        if(not send_command(fd, msg)) { return false; }

        // Query Serial Number
        msg = "SERNO?";
        if(not send_command(fd, msg)) { return false; }
        // Query Firmware Version
        msg = "VER?";
        if(not send_command(fd, msg)) { return false; }

        // Execute build-in self test
        msg = "TEST";
        if(not send_command(fd, msg)) { return false; }
        msg = "BITRESULT?";
        if(not send_command(fd, msg)) { return false; }
        parseData(msg, i);
    }

    if (_numaddr == 0)
    {
        // if setting all active addresses fails on init, wait 5s and close
        // program so nidas can restart it.
        std::cout << "There are no active addresses available to select"
            << std::endl;
        usleep(5000000);  // 5 seconds
        return false;
    }

    return true;
}

// Remove address from active address list
// i is index of addrinfo string, not actual address to be removed
void naiipm::rmAddr(int i)
{
    std::cout << "Removing address " << addr(i) << " from active address list"
        << std::endl;
    for (int j=i; j<_numaddr; j++) {
        _addrinfo[j] = _addrinfo[j+1];
        _addr[j] = _addr[j+1];
        _procqueries[j] = _procqueries[j+1];
        _addrport[j] = _addrport[j+1];
    }
    _numaddr =  _numaddr - 1;
}

// Establish connection to iPM
int naiipm::open_port(const char *device)
{
    int fd; // file description for the serial port
    struct termios port_settings; // structure to store the port settings in

    fd = open(Device(), O_RDWR | O_NOCTTY | O_NONBLOCK); // read/write

    if (fd == -1) // if open is unsuccessful
    {
        std::cout << "open_port: Unable to open " << device << std::endl;
        exit(1);
    }
    else
    {
        // Confirm we are pointing to a serial device
        if (!isatty(fd))
        {
            std::cout << device << " is not a serial device" << std::endl;
            close_port(fd);
            exit(1);
        }

        // Get serial port configuration
        if (tcgetattr(fd, &port_settings) == -1)
        {
            std::cout << "Failed to get serial port config" << std::endl;
            exit(2);
        }

        // zero stuff out
        port_settings.c_iflag &= 0;
        port_settings.c_oflag &= 0;
        port_settings.c_lflag &= 0;
        cfsetspeed(&port_settings, 0);   // wipes out 8,11,12
        port_settings.c_cflag |= CREAD;  // turn on 8
        port_settings.c_cflag |= HUPCL;  // turn on 11
        port_settings.c_cflag |= CLOCAL; // turn on 12
        if (cfsetspeed(&port_settings, get_baud()) == -1)
        {
            std::cout << "Failed to set baud rate to 115200" << std::endl;
        }

        // |= turns on; &= ~ turns off
        port_settings.c_cflag &= ~CRTSCTS; // turn off hardware flow control
        // 8n1 (8bit,no parity,1 stopbit)
        port_settings.c_cflag |= CS8;                // turn on 8bit
        port_settings.c_cflag &= ~(PARENB | PARODD); // shut off parity
        port_settings.c_cflag &= ~CSTOPB;            // 1 stopbit
        port_settings.c_cc[VTIME] = 1; // .1 second timeout
        port_settings.c_iflag &=  ~(IXON | IXOFF | IXANY); // s/w flow ctrl off

        // apply settings to the port
        if (tcsetattr(fd, TCSANOW, &port_settings) == -1)
        {
            std::cout << "Failed to set serial attributes" << std::endl;
        }

        if (Verbose())
        {
            std::cout << "Port for device " << device << " is open."
                << std::endl;
        }
    }

    return(fd);
} //open_port

// Convert baud rate to value required by cfsetspeed command
uint_fast32_t naiipm::get_baud()
{
    switch (atoi(_baudRate)) {
        case 115200:
            return B115200;
        default:
            std::cout << "Unknown baud rate " << _baudRate << "If rate is valid"
                " please update get_baud() function" << std::endl;
            exit(1);
    }
}

void naiipm::close_port(int fd)
{
    close(fd);
}

// Create UDP sockets to send packets to nidas
// Each address needs it's own socket so it can send over it's own port.
void naiipm::open_udp(const char *ip)
{
    for (int i=0; i<numAddr(); i++)
    {
        //  AF_INET for IPv4/ AF_INET6 for IPv6
        //  SOCK_STREAM for TCP / SOCK_DGRAM for UDP
        _sock[i] = socket(AF_INET, SOCK_DGRAM, 0);

        if (_sock[i] < 0)
        {
            std::cout << "Socket creation failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        memset(&_servaddr[i], 0, sizeof(_servaddr[i]));
        _servaddr[i].sin_family = AF_INET;
        _servaddr[i].sin_port = htons(addrport(i));
        _servaddr[i].sin_addr.s_addr = inet_addr(ip);
    }

}

// Send a UDP message to nidas
void naiipm::send_udp(const char *buf, int i)
{
    std::cout << "sending to port " << addrport(i) << " UDP string " << buf;
    if (sendto(_sock[i], (const char *)buf, strlen(buf), 0,
            (const struct sockaddr *) &_servaddr[i], sizeof(_servaddr[i])) == -1)
    {
        std::cout << "Sending packet to nidas returned error " << errno
            << std::endl;
	exit(1);
    }
}

// Close UDP port
void naiipm::close_udp(int adr)
{
    if (adr == -1)
    {
        for (int i=0; i<numAddr(); i++)
        {
            close(_sock[i]);
        }
    } else {
        close(_sock[adr]);
    }
}

// Parse the addrInfo block from the command line
// Block contains addr,procqueries,port
bool naiipm::parse_addrInfo(int i)
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
        std::cout << "addr: " << addr(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setProcqueries(i, ptr);
    if (Verbose())
    {
        std::cout << "procqueries: " << procqueries(i) << std::endl;
    }

    ptr = strtok(NULL, ",");
    setAddrPort(i, ptr);
    if (Verbose())
    {
        std::cout << "addrport: " << addrport(i) << std::endl;
    }

    return true;
}

// Set active address
bool naiipm::setActiveAddress(int fd, int addr)
{
    std::string msg = "ADR";
    std::string msgarg = std::to_string(addr);

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

    for (int i=0; i < numAddr(); i++)
    {

        // ‘procqueries’ is an integer representation of 3-bit Boolean
        // field indicating whether query responses [RECORD,MEASURE,STATUS]
        // should be processed and variables included in a processed data
        // file.
        //     d’3 (b’011) indicates that MEASURE+STATUS are processed.
        //     d’5 (b’101) indicates that RECORD+STATUS are processed.
        int procq = procqueries(i);
        std::bitset<4> x('\0' + procq);
        if (Verbose())
        {
            std::cout << ": [" << procq << "] " << '\0' + procq << " : " << x
                << std::endl;
        }

        if (setActiveAddress(fd, addr(i)))
        {
            // Per software requirements, MEASURE? Is queried first, followed
            // by STATUS?, followed by RECORD?
            std::bitset<4> m = x;
            if ((m &= 0b0010) == 2)  // MEASURE command requested
            {
                msg = "MEASURE?";
                if(not send_command(fd, msg)) { return false; }
                parseData(msg, i);
            }
            std::bitset<4> s = x;
            if ((s &= 0b0001) == 1)  // STATUS command requested
            {
                msg = "STATUS?";
                if(not send_command(fd, msg)) { return false; }
                parseData(msg, i);
            }
            std::bitset<4> r = x;
            if ((r &= 0b0100) == 4)  // RECORD command requested
            {
                if (_recordCount >= _recordFreq)
                {
                    msg = "RECORD?";
                    if(not send_command(fd, msg)) { return false; }
                    parseData(msg, i);
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
    if (Verbose())
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
            if (len != 0)  // expected a response but didn't get one
            {
                // timeout
                trackBadData();
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
            if (Verbose())
            {
              // If c is not a printable character, for printing purposes
              // replace it with a null string terminator"
              char ch = c;
              if (not std::isprint(static_cast<unsigned char>(c))) {
                  ch = '\0';
              } else {
                  ch = c;
              }
              std::cout << n+1 << ": [" << ch << "] " << std::dec << i << ",";
              std::cout << std::hex << i << " : " << x << std::dec << std::endl;
            }
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
            return;
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

    if (not bin && (n+1 != len))
    {
       if (len != 0)  // For ADR command, expect len zero response, and when
       {              // get no response n+1 = 1, so will fail above check but
                    // ignore here. I am sure there is a better logic construct
                    // to catch this, but I am not coming up with it right now.
           if (n == 0) // Did not receive a response at all when expected
           {
               std::cout << "Didn't receive a response from the iPM."
                   << std::endl;
               std::cout << "Are you sure the selected address is active?"
                   << std::endl;
           } else {
               // n and len should be the same for ascii data
               std::cout << "Didn't receive all expected chars: received " <<
                   n+1 << ", expected " << len << " : " << buffer << std::endl;
               // data size error; increment bad data counter
               trackBadData();
           }
       }
    }
}

// If bad data is received (e.g. header error, size error, CRC error, query
// timeout) then the counter is incremented. If counter reaches 10 errors,
// application shall wait 5 seconds and then reinit. Log that we shut down for
// data error reasons.
void naiipm::trackBadData()
{
    _badData++;
    if (_badData == 10)
    {
        std::cout << "Found 10 data errors - shutting down and restarting"
            << std::endl;
        // Upon exit, nidas will wait for timeout given in XML (should be 5s)
        // and then will attempt to restart program.
        exit(1);
    }
}
// send a single command entered on the command line
void naiipm::singleCommand(int fd, std::string cmd, int addr)
{
    setActiveAddress(fd, addr);
    if (Verbose())
    {
        std::cout << "Sending command " << cmd << std::endl;
    }
    send_command(fd, cmd, "");
    parse_binary(cmd);
}

// send command to iPM and verify response
bool naiipm::send_command(int fd, std::string msg, std::string msgarg)
{
    // Confirm command is in list of acceptable command
    if (not verify(msg)) {return false;}

    if (Verbose())
    {
        std::cout << "Got message " << msg << std::endl;
    }

    // Find expected response for this message
    auto response = _ipm_commands.find(msg);
    std::string expected_response = response->second;

    if (Verbose())
    {
        std::cout << "Expect response " << expected_response << std::endl;
    }

    // Send message to ipm
    if (msgarg != "")
    {
        msg.append(' ' + msgarg);
    }
    std::string sendmsg = msg + "\n";  // Add linefeed to end of command
    if (Verbose())
    {
        std::cout << "Sending message " << sendmsg << std::endl;
        std::cout << "of length " << sendmsg.length() << std::endl;
    }
    write(fd, sendmsg.c_str(), sendmsg.length());
    if (tcdrain(fd) == -1)  // wait for write to complete
    {
        std::cout << errno << std::endl;
    }
    if (Verbose())
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
        if (Verbose())
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
        if (Verbose())
        {
            std::cout << "Received " << buffer << std::endl;
        }

        if(buffer != expected_response)
        {
            // header error so increment bad data counter
            trackBadData();
            std::cout << "Device command " << msg << " did not return "
                << "expected response " << expected_response << std::endl;
            return false;  // command failed
        }
    }

    // Read binary part of response. Length of binary response was
    // returned as first response to query.

    if (_ipm_data.find(msg) != _ipm_data.end()) // cmd returns data
    {
        int binlen = std::stoi(buffer);
        if (Verbose())
        {
            std::cout << "Now get " << binlen << " bytes" << std::endl;
        }
        get_response(fd, binlen, true);  // true indicates reading binary data
        setData(msg, binlen);
    }

    flush(fd);

    return true;  // command succeeded
}

// Determine which interactive mode user is requesting
// There are two options: either give an address and ipm query on the command
// line, receive a result and exit, or launch an interactive menu from which
// to select queries.
bool naiipm::setInteractiveMode(int fd)
{
    // If giving address and query on command line, ensure both exist
    // and are valid;
    // got -a but not -c
    if (atoi(Address()) != -1 and strcmp(Cmd(),"") == 0)
    {
        verify(Cmd());
        return false;
    }
    // got -c but not -a
    if (atoi(Address()) == -1 and strcmp(Cmd(),"") != 0)
    {
        std::cout << "Setting default address of 0" << std::endl;
        setAddress("0");
        return true;
    }

    // iPM command (-c) and address (-a) both given on command line
    // so send query to iPM
    if (atoi(Address()) != -1 and strcmp(Cmd(),"") != 0)
    {
        return true;
    }

    // didn't get -a or -c, so print menu and wait for user input
    bool status = true;
    while (status == true)
    {
        printMenu();
        status = readInput(fd);
    }

    return status;  // will be false if user requested to quit
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
    if (Verbose())
    {
        std::cout << "User requested " << cmd << std::endl;
    }

    // Catch exit request
    if (cmd.compare("q") == 0)
    {
        if (Verbose())
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

    parse_binary(cmd);

    return true;
}
void naiipm::parse_binary(std::string cmd)
{
    // Check if command has binary data component. If so, parse it into
    // it's component variables.
    if (_ipm_data.find(cmd) != _ipm_data.end())  // found cmd in binary map
    {
        // Parse binary data
        parseData(cmd, 0);  // In interactive mode, only one address is used
    }
}

void naiipm::parseData(std::string cmd, int adr)
{
    if (Verbose())
    {
        std::cout << '{' << cmd << '}' << std::endl;
        std::cout << "In parseData: Info for address " << adr << " is " << addr(adr) << ","
            << procqueries(adr) << "," << addrport(adr) << std::endl;
    }
    // retrieve binary data
    char* data = getData(cmd); // data content

    // Create some pointers to access data of various lengths
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;
    unsigned char *up = (unsigned char *)data;

    // parse data
    if (cmd == "BITRESULT?") {
        parseBitresult(sp);
    }

    if (cmd == "RECORD?") {
        ipmRecord _record;
        _record.parse(cp, sp, lp);

        // CRC validation doesn't currently work. See notes in src/record.cc
        // Leaving the code here so that this can be investigated more later
        // if desired.
        uint32_t crc = _record.calculateCRC32(&up[0], 64);
        //_record.checkCRC(cp, crc);
        // If CRC from the data and calculatedCRC don't match, increment bad
        // data counter:
        // trackBadData();

        if (Verbose())
        {
            std::cout << record.TIME/60000 << " minutes since power-up"
                << std::endl;
        }

        _record.createUDP(buffer, scaleflag());
    }

    if (cmd == "MEASURE?") {
        ipmMeasure _measure;
        _measure.parse(cp, sp);
        _measure.createUDP(buffer, scaleflag());
    }

    if (cmd == "STATUS?") {
        ipmStatus _status;
        _status.parse(cp, sp);
        _status.createUDP(buffer, scaleflag(), _badData);
    }

    if (Interactive())
    {
        std::cout << buffer << std::endl;
    } else
    {
        send_udp(buffer, adr);
    }

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
    if (Verbose())
    {
        std::cout << "iPM temperature (C) = " << TEMP << std::endl;
    }
}
