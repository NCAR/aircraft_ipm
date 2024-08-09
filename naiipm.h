/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <map>
#include <arpa/inet.h>
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

        void setDevice(const char device[])   { _device = device; }
        const char* Device()      { return _device; }
        int open_port(const char *device);
        void close_port(int fd);

        void open_udp(const char *ip);
        void send_udp(const char *buffer, int adr);
        void close_udp(int adr);

        void setRate(const char rate[])   { _measureRate = rate; }
        void setPeriod(const char period[])   { _recordPeriod = period; }
        void setBaud(const char baud[])   { _baudRate = baud; }
        void setNumAddr(const char numaddr[])   { _numaddr = atoi(numaddr); }
        int numAddr()      { return _numaddr; }
        void setAddrInfo(int optopt, char addrinfo[])
            { _addrinfo[optopt] = addrinfo; }
        bool parse_addrInfo(int index);

        void setInteractive() { _interactive = true; }
        bool Interactive()    { return _interactive; };
        void setAddress(const char address[]) {_address = address; }
        const char* Address()        { return _address; }
        void setCmd(const char cmd[]) {_cmd = cmd; }
        const char* Cmd()        { return _cmd; }
        bool setInteractiveMode(int fd);

        void setVerbose() { _verbose = true; }
        bool Verbose()    { return _verbose; };

        void setEmulate() { _emulate = true; }
        bool Emulate()    { return _emulate; };

        void setScaleFlag(int flag)
            { _scaleflag = flag; }
        bool verify(std::string cmd);

        void singleCommand(int fd, std::string cmd, int addr);
        void printMenu();
        bool readInput(int fd);
        bool init(int fd);
        bool loop(int fd);

        void setRecordFreq();
        void sleep();

        void configureSerialPort();

    private:
        char buffer[1000];

        void parseData(std::string cmd, int addrIndex);
        void parseBitresult(uint16_t *sp);

        void get_response(int fd, int len, bool bin);
        void flush(int fd);
        bool send_command(int fd, std::string msg, std::string msgarg = "");
        void parse_binary(std::string cmd);

        const char* _measureRate;
        const char* _recordPeriod;
        const char* _baudRate;
	    uint_fast32_t get_baud();

        int _recordCount;
        int _recordFreq;
        long _sleeptime;

        char* _addrinfo[8];
        char* addrInfo(int index)      { return _addrinfo[index]; }

        int _addr[8];
        int addr(int index)   { return _addr[index]; }
        void setAddr(int index, char* ptr)
            { _addr[index] = atoi(ptr); }
        bool setActiveAddress(int fd, int addr);
        void rmAddr(int i);

        int _scaleflag;
        int scaleflag()   { return _scaleflag; }

        int _procqueries[8];
        int procqueries(int index)   { return _procqueries[index]; }
        void setProcqueries(int index, char* ptr)
            { _procqueries[index] = atoi(ptr); }

        int _addrport[8];
        void setAddrPort(int index, char* ptr)
            { _addrport[index] = atoi(ptr); }
        int addrport(int index)   { return _addrport[index]; }

        void setData(std::string cmd, int binlen);
        char* getData(std::string msg)
            { return _ipm_data.find(msg)->second; }

        const char* _device;
        int _numaddr;
        bool _interactive;
        const char*  _address;
        const char* _cmd;
        bool _verbose;
        bool _emulate;
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

