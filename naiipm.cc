/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <fcntl.h>
#include <termios.h>
#include <bitset>

#include "naiipm.h"

naiipm::naiipm()
{
    // Initialize the binary data map
    char *bitdata = (char *)malloc(25 * sizeof(char));
    bitdata[0] = '\0';
    char *measuredata = (char *)malloc(35 * sizeof(char));
    measuredata[0] = '\0';
    char *statusdata = (char *)malloc(13 * sizeof(char));
    statusdata[0] = '\0';
    char *recorddata = (char *)malloc(69 * sizeof(char));
    recorddata[0] = '\0';
    ipm_data["BITRESULT?"] = bitdata;   // Query self test result
    ipm_data["MEASURE?"] = measuredata; // Device Measurement
    ipm_data["STATUS?"] = statusdata;   // Device Status
    ipm_data["RECORD?"] = recorddata;   // Device Statistics

}

naiipm::~naiipm()
{
    for (std::map<std::string, char *>::const_iterator it = ipm_data.begin();
            it !=ipm_data.end(); ++it)
    {
        free(it->second);
    }
}

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
        if (atoi(baud()) != 115200)
        {
            // log unknown speed
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
    if (sock < 0)
    {
        std::cout << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(30101);
    servaddr.sin_addr.s_addr = inet_addr(ip);
}

// Send a UDP message to nidas
void naiipm::send_udp(const char *buf)
{
    std::cout << "sending UDP string " << buf << std::endl;
    sendto(sock, (const char *)buf, strlen(buf), MSG_CONFIRM,
            (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

// Close UDP port
void naiipm::close_udp()
{
    close(sock);
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

    sprintf(msgarg, "%d", addr(i));
    if(not send_command(fd, msg, msgarg)) { return false; }

    return true;
}

// Determine queries to send and process.
bool naiipm::loop(int fd)
{
    std::string msg;

    for (int i=0; i < atoi(numAddr()); i++)
        {
            // ‘numphases’ (integer) indicates whether 1 phase, or 3-phases of
            // data are to be capture. Should only be =1 or =3
            int nphases = numphases(i);
            if (nphases != 1 && nphases != 3)
            {
                //log error
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
                    msg = "RECORD?";
                    if(not send_command(fd, msg)) { return false; }
                    bool status = parseData(msg, nphases);
                    std::cout << std::boolalpha << "Status is " << status
                        << std::endl;
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

    return true;

}

void naiipm::setData(std::string cmd, char *bitdata)
{
    // free the previous binary data memory space
    // and update the map to point to the new space
    free(ipm_data.find(cmd)->second);
    ipm_data[cmd] = bitdata;
}

// read response from iPM
char* naiipm::get_response(int fd, int len)
{
    int n = 0, r = 0;
    char *l = (char *)malloc((len+2) * sizeof(char));
    l[0] = '\0';
    std::cout << "len " << len << std::endl;
    while (true)
    {
        char c;
        int ret = read(fd, &c, 1);
        if (ret > 0)  // successful read
        {
            std::bitset<8> x(c);
            std::cout << n+1 << ": [" << c << "] " << int(c) << " : " << x
                << std::endl;
            if (c == '\0') {
                std::cout << "Found a null" << std::endl;
            }
            l[n] = c;
            if (c == '\n') { // found linefeed
                break;
            }
            n++;
        } else if (ret == '\0')  // time out after 5 tries
        {
            r++;
            std::cout << "null " << c << std::endl;
            if (r > 5)
            {
                break;
            }
        } else if (ret != -1)  // read did not return timeout
        {
            std::cout << "unknown response " << c << std::endl;
        }

        // if receive len chars without an endline, return anyway
	// (handles binary data)
        if (n > (len - 1))
        {
	    n--;  // decrement char count since never found linefeed
	    break;
	}

        // TBD: If iPM never returns expected number of bytes, timeout
        // Currently have RECORD? not returning enough data so can test
        // this.
    }

    l[n+1] = '\0'; // terminate the string

    return(l);
}

// send command to iPM and verify response
bool naiipm::send_command(int fd, std::string msg, std::string msgarg)
{
    // Confirm command is in list of acceptable command
    if (not verify(msg)) {return false;}

    std::cout << "Got message " << msg << std::endl;
    // Find expected response for this message
    auto response = ipm_commands.find(msg);
    std::string expected_response = response->second;
    // std::string expected_response = ipm_commands.find(msg)->second;
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
    // TBD: Need some sort of timeout here if instrument is hung and not
    // sending anything back that lets user know a power cycle might be
    // required.

    // Get response from ipm
    char * line = get_response(fd, int(expected_response.length()));
    std::cout << "Received " << line << std::endl;

    if(line != expected_response)
    {
        printf("Device command %s ", msg);
        std::cout << "did not return expected response "
            << expected_response <<  std::endl;
        free((char *)line);
        return false;  // command failed
    }

    // Read binary part of response. Length of binary response was
    // returned as first response to query.
    if (ipm_data.find(msg) != ipm_data.end()) // cmd returns data
    {
        int binlen = std::stoi(line);
        std::cout << "Now get " << binlen << " bytes" << std::endl;
        char* data = get_response(fd, binlen);
        setData(msg, data);
    }

    flush(fd);

    free((char *)line);
    return true;  // command succeeded
}

// Flush serial port
void naiipm::flush(int fd)
{
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
        std::cout << errno << std::endl;
    }

}

// List use options in interactive mode
void naiipm::printMenu()
{
    std::cout << "=========================================" << std::endl;
    std::cout << "Type one of the following iPM commands or" << std::endl;
    std::cout << "enter 'q' to quit" << std::endl;
    std::cout << "=========================================" << std::endl;
    for (auto msg : ipm_commands) {
        std::cout << msg.first << std::endl;
    }
}

bool naiipm::verify(std::string cmd)
{
    // Confirm command is in list of acceptable command
    if (not ipm_commands.count(cmd))
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
    if (ipm_data.find(cmd) != ipm_data.end())  // found cmd in binary map
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
    std::string datalen = ipm_commands.find(cmd)->second; // data length
    std::cout << '{' << datalen << '}' << std::endl;
    std::cout << '{' << cmd << '}' << std::endl;
    int dlen = stoi(datalen);
    char* data = getData(cmd); // data content

    // Create some pointers to access data of various lengths
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;

    std::cout << "readInput received " << data  << std::endl;

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
        sprintf(buffer,"RECORD,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f\r\n",
                record_1phase.FREQMAX * dH2H, record_1phase.FREQMIN * dH2H,
                record_1phase.VRMSMAXA * dV2V, record_1phase.VRMSMINA * dV2V,
                record_1phase.VPKMAXA * dV2V, record_1phase.VPKMINA * dV2V,
                record_1phase.VDCMAXA * mV2V, record_1phase.VDCMINA * mV2V,
                record_1phase.THDMAXA * dp2p, record_1phase.THDMINA * dp2p,
                (float)record_1phase.TIME, (float)record_1phase.EVTYPE
                );
        send_udp(buffer);
    }

    return true;
}
