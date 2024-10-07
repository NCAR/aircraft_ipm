/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <iostream>
#include <unistd.h>
#include <algorithm>
#ifdef __linux__
    #include <sys/io.h>
#endif
#include <string.h>

#include "cmd.h"

#ifndef ARGPARSE_H
#define ARGPARSE_H

extern ipmCmd commands;

class ipmArgparse
{
    public:
        ipmArgparse();
        ~ipmArgparse();

        void setDevice(const char device[])   { _device = device; }
        const char* Device()                  { return _device; }

        const char* measureRate()         { return _measureRate; }
        void setRate(const char rate[])   { _measureRate = rate; }

        const char* recordPeriod()            { return _recordPeriod; };
        void setPeriod(const char period[])   { _recordPeriod = period; }

        void setBaud(const char baud[])   { _baudRate = baud; }
        const char* BaudRate()            { return _baudRate; }

        void setNumAddr(const char numaddr[])   { _numaddr = atoi(numaddr); }
        int numAddr()                           { return _numaddr; }
        void updateNumAddr(int naddr)           { _numaddr = naddr; }

        void setAddrInfo(int optopt, char addrinfo[])
            { _addrinfo[optopt] = addrinfo; }
        bool parse_addrInfo(int index);
        char* addrInfo(int index)      { return _addrinfo[index]; }
        void updateAddrInfo(int index, char* adinfo)
            { _addrinfo[index] = adinfo; }

        int Addr(int index)                  { return _addr[index]; }
        void setAddr(int index, char* ptr)   { _addr[index] = atoi(ptr); }
        void updateAddr(int index, int addr) { _addr[index] = addr; }

        int Procqueries(int index)   { return _procqueries[index]; }
        void setProcqueries(int index, char* ptr)
            { _procqueries[index] = atoi(ptr); }
        void updateProcqueries(int index, int procq)
            { _procqueries[index] = procq; }

        int Addrport(int index)   { return _addrport[index]; }
        void setAddrPort(int index, char* ptr)
            { _addrport[index] = atoi(ptr); }
        void updateAddrPort(int index, int adrp)
            { _addrport[index] = adrp; }

        void setAddress(const char address[]) {_address = address; }
        const char* Address()                 { return _address; }

        void setCmd(const char cmd[]) {_cmd = cmd; }
        const char* Cmd()             { return _cmd; }

        void setInteractive() { _interactive = true; }
        bool Interactive()    { return _interactive; };

        void setSilent(bool state) { _silent = state; }
        bool Silent()    { return _silent; }

        void setVerbose() { _verbose = true; }
        bool Verbose()    { return _verbose; };

        void setScaleFlag(int flag) { _scaleflag = flag; }
        int scaleflag()             { return _scaleflag; }

        void setEmulate() { _emulate = true; }
        bool Emulate()    { return _emulate; };

        void setDebug() { _debug = true; }
        bool Debug()    { return _debug; };

        void configureSerialPort();

        void Usage();
        void process(int argc, char *argv[]);

    private:
        const char* _device;
        const char* _measureRate;
        const char* _recordPeriod;
        const char* _baudRate;
        int _numaddr;
        char* _addrinfo[8];
        int _addr[8];
        int _procqueries[8];
        int _addrport[8];
        const char*  _address;
        const char* _cmd;
        bool _interactive;
        bool _silent = false;
        bool _verbose;
        int _scaleflag;
        bool _emulate;
        bool _debug;

};

#endif /* ARGPARSE_H */
