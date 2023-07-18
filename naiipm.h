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

struct
{
    uint8_t EVTYPE;     // Event Type
    uint8_t OPSTATE;    // Operating State
    uint32_t POWERCNT;  // Power Up Count
    uint32_t TIME;      // Elapsed time since power-up (ms)
    uint32_t TFLAG;     // Trip Flag
    uint32_t CFLAG;     // Caution Flag
    uint16_t VRMSMINA;  // Phase A RMS Voltage Min
    uint16_t VRMSMAXA;  // Phase A RMS Voltage Max
    uint16_t VRMSMINB;  // Phase B RMS Voltage Min
    uint16_t VRMSMAXB;  // Phase B RMS Voltage Max
    uint16_t VRMSMINC;  // Phase C RMS Voltage Min
    uint16_t VRMSMAXC;  // Phase C RMS Voltage Max
    uint16_t FREQMIN;   // Frequency Min
    uint16_t FREQMAX;   // Frequency Max
    uint16_t VDCMINA;   // Phase A Voltage, DC Coomponent Min
    uint16_t VDCMAXA;   // Phase A Voltage, DC Coomponent Max
    uint16_t VDCMINB;   // Phase B Voltage, DC Coomponent Min
    uint16_t VDCMAXB;   // Phase B Voltage, DC Coomponent Max
    uint16_t VDCMINC;   // Phase C Voltage, DC Coomponent Min
    uint16_t VDCMAXC;   // Phase C Voltage, DC Coomponent Max
    uint8_t THDMINA;    // Phase A Voltage THD Min
    uint8_t THDMAXA;    // Phase A Voltage THD Max
    uint8_t THDMINB;    // Phase B Voltage THD Min
    uint8_t THDMAXB;    // Phase B Voltage THD Max
    uint8_t THDMINC;    // Phase C Voltage THD Min
    uint8_t THDMAXC;    // Phase C Voltage THD Max
    uint16_t VPKMINA;   // Phase A Peak Voltage Min
    uint16_t VPKMAXA;   // Phase A Peak Voltage Max
    uint16_t VPKMINB;   // Phase B Peak Voltage Min
    uint16_t VPKMAXB;   // Phase B Peak Voltage Max
    uint16_t VPKMINC;   // Phase C Peak Voltage Min
    uint16_t VPKMAXC;   // Phase C Peak Voltage Max
    uint32_t CRC;       // CRC-32
} record;

struct
{
    uint16_t FREQ;      // AC Power Frequency
    uint16_t TEMP;      // Temperature
    uint16_t VRMSA;     // Phase A RMS AC Voltage
    uint16_t VRMSB;     // Phase B RMS AC Voltage
    uint16_t VRMSC;     // Phase C RMS AC Voltage
    uint16_t VPKA;      // Phase A Peak AC Voltage
    uint16_t VPKB;      // Phase B Peak AC Voltage
    uint16_t VPKC;      // Phase C Peak AC Voltage
    uint16_t VDCA;      // Phase A Voltage, DC Component
    uint16_t VDCB;      // Phase B Voltage, DC Component
    uint16_t VDCC;      // Phase C Voltage, DC Component
    uint16_t PHA;       // Phase A Voltage, AC Phase Angle
    uint16_t PHB;       // Phase B Voltage, AC Phase Angle
    uint16_t PHC;       // Phase C Voltage, AC Phase Angle
    uint8_t THDA;       // Phase A Voltage THD
    uint8_t THDB;       // Phase B Voltage THD
    uint8_t THDC;       // Phase C Voltage THD
    uint8_t POWEROK;    // Power OK, All phases
} measure;

struct
{
    uint8_t OPSTATE;        // Operational State
    uint8_t POWEROK;        // Power OK
    uint32_t TRIPFLAGS;     // Power Trip Flags, performance exceeds limit
    uint32_t CAUTIONFLAGS;  // Power Caution Flags, marginal performance
    uint16_t BITSTAT;       // bitStatus
} status;


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
        void parseBitresult(uint16_t *sp);
        void parseMeasure(uint8_t *cp, uint16_t *sp);
        void scaleMeasure();
        void parseStatus(uint8_t *cp, uint16_t *sp);
        void parseRecord(uint8_t *cp, uint16_t *sp, uint32_t *lp);
        void scaleRecord();

        void get_response(int fd, int len, bool bin);
        void flush(int fd);
        bool verify(std::string cmd);
        bool send_command(int fd, std::string msg, std::string msgarg = "");

        const char* _measureRate;
        const char* _recordPeriod;
        const char* _baudRate;

        int _recordCount;

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
        float _deci;   // 0.1
        float _milli;  // 0.001
};

