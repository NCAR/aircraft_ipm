/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <fcntl.h>
#include <termios.h>

#include "naiipm.h"

naiipm::naiipm()
{
}

naiipm::~naiipm()
{
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

        port_settings.c_lflag &= ~ICANON; // ensure non-canonical
        port_settings.c_cflag |= CS8; // 8n1 (8bit,no parity,1 stopbit)
        port_settings.c_cc[VTIME] = 1; // 0.1 second timeout

        // apply settings to the port
        if (tcsetattr(fd, TCSANOW, &port_settings) == -1)
        {
            std::cout << "Failed to set serial attributes" << std::endl;
        }

        std::cout << "port " << port << " is open." << std::endl;
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

// read response from iPM
std::string naiipm::get_response(int fd, int len)
{
    int n = 0, r = 0;
    char line[len+1] = "";  // space for string terminating char
    char *l = &line[0];
    while (true)
    {
        char c;
        int ret = read(fd, &c, 1);
        if (ret > 0)  // successful read
        {
            std::cout << c << std::endl;
            l[n] = c;
            if (c == '\n') {
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
         if (n > (len - 1)) { break;}
    }
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
        std::cout << errno << std::endl;
    }

    l[n+1] = '\0'; // terminate the string

    return(line);
}

// send command to iPM and verify response
bool naiipm::send_command(int fd, std::string msg, std::string msgarg)
{
    std::cout << "Got message " << msg << std::endl;
    // Find expected response for this message
    auto response = ipm_commands.find(msg);
    std::string expected_response = response->second;
    std::cout << "Expect response '" << expected_response << "'" << std::endl;

    // Send message to ipm
    if (msgarg != "")
    {
        msg.append(' ' + msgarg);
    }
    std::cout << "Sending message " << msg << std::endl;
    write(fd, msg.c_str(), msg.length());
    if (tcdrain(fd) == -1)  // wait for write to complete
    {
        std::cout << errno << std::endl;
    }
    std::cout << "write completed" << std::endl;

    // Get response from ipm
    std::string line = get_response(fd, int(expected_response.length()));
    std::cout << "Received " << line << std::endl;

    if(line == expected_response)
    {
        return true;  // command succeeded
    } else {
        printf("Device command %s ", msg);
        std::cout << "did not return expected response "
            << expected_response <<  std::endl;
        return false;  // command failed
    }
}

// List use options in interactive mode
void naiipm::printMenu()
{
    std::cout << "=========================================" << std::endl;
    std::cout << "Type one of the following commands" << std::endl;
    std::cout << "=========================================" << std::endl;
    for (auto msg : ipm_commands) {
        std::cout << msg.first << std::endl;
    }
}

bool naiipm::readInput(int fd)
{
    std::string cmd = "";
    std::cin >> (cmd);

    // Request user input
    const char *cmdInput = cmd.c_str();
    std::cout << "User requested " << cmd << std::endl;

    // Confirm command is in list of acceptable command
    if (not ipm_commands.count(cmd))
    {
        std::cout << "Command " << cmd << " is invalid" << std::endl;
        return false;
    }

    // If command is valid, send to ipm
    if (not send_command(fd, (char *)cmdInput)) { return false; }

    return true;
}
