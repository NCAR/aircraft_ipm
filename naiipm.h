/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <map>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

/**
 * Data structures
 */

// 1phase
struct
{
    uint16_t FREQMAX;   // Frequency Max
    uint16_t FREQMIN;   // Frequency Min
    uint16_t VRMSMAXA;  // Phase A RMS Voltage Max
    uint16_t VRMSMINA;  // Phase A RMS Voltage Min
    uint16_t VPKMAXA;   // Phase A Peak Voltage Max
    uint16_t VPKMINA;   // Phase A Peak Voltage Min
    uint16_t VDCMAXA;   // Phase A Voltage, DC Coomponent Max
    uint16_t VDCMINA;   // Phase A Voltage, DC Coomponent Min
    uint8_t THDMAXA;    // Phase A Voltage THD Max
    uint8_t THDMINA;    // Phase A Voltage THD Min
    uint32_t TIME;      // Elapsed time since power-up (ms)
    uint8_t EVTYPE;     // Event Type
} record_1phase;



/**
 * Class to initialize and control North Atlantic Industries
 * Intelligent Power Monitor (iPM)
 */
class naiipm
{
    public:
        naiipm();
        ~naiipm();

        void setPort(const char port[])   { _port = port; }
        const char* Port()      { return _port; }
        int open_port(const char *port);
        void close_port(int fd);

        void open_udp(const char *ip, int port);
        void send_udp(const char *buffer);
        void close_udp();

        void setRate(const char rate[])   { _measureRate = rate; }
        void setPeriod(const char period[])   { _recordPeriod = period; }
        void setBaud(const char baud[])   { _baudRate = baud; }
        void setNumAddr(const char numaddr[])   { _numaddr = numaddr; }
        const char* numAddr()      { return _numaddr; }
        void setAddrInfo(int optopt, char addrinfo[])
            { _addrinfo[optopt] = addrinfo; }

        void setInteractive() { _interactive = true; }
        bool Interactive()    { return _interactive; };

        void printMenu();
        bool readInput(int fd);
        bool init(int fd);
        bool loop(int fd);

    private:
        char buffer[1000];

        bool parseData(std::string cmd, int nphases);

        void get_response(int fd, int len);
        void flush(int fd);
        bool verify(std::string cmd);
        bool send_command(int fd, std::string msg, std::string msgarg = "");

        const char* _measureRate;
        const char* rate()      { return _measureRate; }
        const char* _recordPeriod;
        const char* period()      { return _recordPeriod; }
        const char* _baudRate;
        const char* baud()      { return _baudRate; }

        char* _addrinfo[8];
        char* addrInfo(int index)      { return _addrinfo[index]; }
        void parse_addrInfo(int index);

        int _addr[8];
        int addr(int index)   { return _addr[index]; }
        void setAddr(int index, char* ptr)
            { _addr[index] = atoi(ptr); }
        bool setActiveAddress(int fd, int i);

        int _numphases[8];
        int numphases(int index)   { return _numphases[index]; }
        void setNumphases(int index, char* ptr)
            { _numphases[index] = atoi(ptr); }

        int _procqueries[8];
        int procqueries(int index)   { return _procqueries[index]; }
        void setProcqueries(int index, char* ptr)
            { _procqueries[index] = atoi(ptr); }

        int _addrport[8];
        int addrport(int index)   { return _addrport[index]; }
        void setAddrPort(int index, char* ptr)
            { _addrport[index] = atoi(ptr); }

        void setData(std::string cmd, int binlen);
        char* getData(std::string msg)
            { return _ipm_data.find(msg)->second; }

        const char* _port;
        const char* _numaddr;
        bool _interactive;
        struct sockaddr_in _servaddr;
        int _sock;

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
        float _dV2V; // 0.1 V to V
        float _dH2H; // 0.1 Hz to Hz
        float _mV2V;  // mV to V
        float _dp2p; // 0.1 % to %

};

