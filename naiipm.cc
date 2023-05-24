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
void naiipm::send_udp(const char *buffer)
{
    std::cout << "sending UDP string " << buffer << std::endl;
    sendto(sock, (const char *)buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
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
    // TBD: Need some sort of timeout here if instrument is hung and not sending
    // anything back that lets user know a power cycle might be required.

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
	    // retrieve binary data
        std::string datalen = ipm_commands.find(cmd)->second; // data length
        std::cout << '{' << datalen << '}' << std::endl;
        std::cout << '{' << cmd << '}' << std::endl;
        int dlen = stoi(datalen);
        char* data = ipm_data.find(cmd)->second; // data content
        std::cout << "readInput received " << data  << std::endl;

        if (cmd == "BITRESULT?") {
          short temperature = (((short)data[23]) << 8) | data[22];
          std::cout << "iPM temperature (C) = " << temperature*.1 << std::endl;
        }
    }

    return true;
}
