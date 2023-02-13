/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <cstdlib>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>


/**
 * Class to initialize and control North Atlantic Industries
 * Intelligent Power Monitor (iPM)
 */
class naiipm
{
    public:
        naiipm();
        ~naiipm();

        void printMenu();
        bool readInput(int fd);

        int open_port(const char *port);
        void close_port(int fd);
        void open_udp(const char *ip, int port);
        void send_udp(const char *buffer);
        void close_udp();
        std::string get_response(int fd, int len);
        bool send_command(int fd, char *msg);

        const char* Port()      { return _port; }
        void setPort(const char port[])   { _port = port; }

        bool Interactive()    { return _interactive; };
        void setInteractive() {_interactive = true; }

    protected:
        const char* _port;
        bool _interactive = false;
        struct sockaddr_in servaddr;
        //  AF_INET for IPv4/ AF_INET6 for IPv6
        //  SOCK_STREAM for TCP / SOCK_DGRAM for UDP
        int sock = socket(AF_INET, SOCK_DGRAM, 0);

        // Map stores message, expected response
        std::map<std::string, std::string>ipm_commands = {
            { "OFF",        "OK\n"},       // Turn Devive OFF
            { "RESET",      "OK\n"},       // Turn Device ON (reset)
            { "SERNO?",     "203456-7\n"}, // Query Serial number
            { "VER?",       "VER 004 2022-11-21\n"}, // Query Firmware Ver
            { "TEST",       "OK\n"},       // Execute build-in self test
            { "BITRESULT?", "24\n"},       // Query self test result
            { "ADR",        ""},           // Device Address Selection
            { "MEASURE?",   "34\n"},       // Device Measurement
            { "STATUS?",    "12\n"},       // Device Status
            { "RECORD?",    "68\n"},       // Device Statistics
        };

};

