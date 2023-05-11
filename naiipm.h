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
        char* get_response(int fd, int len);
        void flush(int fd);
        bool verify(std::string cmd);
        bool send_command(int fd, std::string msg, std::string msgarg = "");

        const char* Port()      { return _port; }
        void setPort(const char port[])   { _port = port; }

        const char* numAddr()      { return _numaddr; }
        void setNumAddr(const char numaddr[])   { _numaddr = numaddr; }

        char* addrInfo(int index)      { return _addrinfo[index]; }
        void setAddrInfo(int optopt, char addrinfo[])
            { _addrinfo[optopt] = addrinfo; }
        void parse_addrInfo(int index);

        int addr(int index)   { return _addr[index]; }
        void setAddr(int index, char* ptr)
            { _addr[index] = atoi(ptr); }

        int numphases(int index)   { return _numphases[index]; }
        void setNumphases(int index, char* ptr)
            { _numphases[index] = atoi(ptr); }

        int procqueries(int index)   { return _procqueries[index]; }
        void setProcqueries(int index, char* ptr)
            { _procqueries[index] = atoi(ptr); }

        int addrport(int index)   { return _addrport[index]; }
        void setAddrPort(int index, char* ptr)
            { _addrport[index] = atoi(ptr); }

        bool Interactive()    { return _interactive; };
        void setInteractive() { _interactive = true; }

        char* getData(std::string msg)
            { return ipm_data.find(msg)->second; }

        void setData(std::string cmd, char * bitdata)
            {ipm_data[cmd] = bitdata;}

        void initData();

    protected:
        const char* _port;
        const char* _numaddr;
        char* _addrinfo[8];
        int _addr[8];
        int _numphases[8];
        int _procqueries[8];
        int _addrport[8];
        bool _interactive = false;
        struct sockaddr_in servaddr;
        //  AF_INET for IPv4/ AF_INET6 for IPv6
        //  SOCK_STREAM for TCP / SOCK_DGRAM for UDP
        int sock = socket(AF_INET, SOCK_DGRAM, 0);

        // Map message to expected response
        std::map<std::string, std::string>ipm_commands = {
            { "OFF",        "OK\n"},       // Turn Device OFF
            { "RESET",      "OK\n"},       // Turn Device ON (reset)
            { "SERNO?",     "200728\n"}, // Query Serial number
            { "VER?",       "VER A022(L) 2018-11-13\n"}, // Query Firmware Ver
            { "TEST",       "OK\n"},       // Execute build-in self test
            { "BITRESULT?", "24\n"},       // Query self test result
            { "ADR",        ""},           // Device Address Selection
            { "MEASURE?",   "34\n"},       // Device Measurement
            { "STATUS?",    "12\n"},       // Device Status
            { "RECORD?",    "68\n"},       // Device Statistics
        };

        // Map message to data string
        typedef std::map<std::string, char*> IpmMap;
        typedef std::pair<std::string, char*> IpmPair;
        IpmMap ipm_data;

};

