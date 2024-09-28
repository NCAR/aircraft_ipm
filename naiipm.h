/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <map>
#include <arpa/inet.h>
#include <iostream>

#ifndef NAIIPM_H
#define NAIIPM_H

#include "src/argparse.h"
#include "src/cmd.h"

extern ipmArgparse args;

/**
 * Class to initialize and control North Atlantic Industries
 * Intelligent Power Monitor (iPM)
 */
class naiipm
{
    public:
        naiipm();
        ~naiipm();

        int open_port();
        void close_port(int fd);

        void open_udp(const char *ip);
        void send_udp(const char *buffer, int adr);
        void close_udp(int adr);

        bool setInteractiveMode(int fd);
        void singleCommand(int fd);
        bool readInput(int fd);
        bool clear(int fd, int addr);
        bool init(int fd);
        bool loop(int fd);

        void setRecordFreq();
        void sleep();

    private:
        char buffer[1000];

        void parseData(std::string cmd, int addrIndex);
        void parseBitresult(uint16_t *sp);

        void get_response(int fd, int len, bool bin);
        void flush(int fd);
        virtual bool send_command(int fd, std::string msg, std::string msgarg = "");
        void parse_binary(std::string cmd);

        uint_fast32_t get_baud();

        int _recordCount;
        int _recordFreq;
        long _sleeptime;

        virtual bool setActiveAddress(int fd, int addr);
        void rmAddr(int i);

        void setData(std::string cmd, int binlen);
        char* getData(std::string msg)
            { return _ipm_data.find(msg)->second; }

        struct sockaddr_in _servaddr[8];
        int _sock[8];

        // Map message to expected response
        std::map<std::string, std::string>_ipm_commands;

        // Map message to data string
        char _bitdata[25];
        char _measuredata[35];
        char _statusdata[13];
        char _recorddata[69];
        typedef std::map<std::string, char*> IpmMap;
        IpmMap _ipm_data;

        // unit conversions
        float _deci;   // 0.1
        float _milli;  // 0.001

        // Bad data counter
        int _badData;

        void trackBadData();

};

#endif /* NAIIPM_H */
