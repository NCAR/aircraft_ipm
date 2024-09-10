/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <stdint.h>
#include <iostream>

#ifndef RECORD_H
#define RECORD_H
class ipmRecord
{

private:

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


public:

    //unit conversions
    float _deci;  // 0.1
    float _milli;  // 0.001

    ipmRecord();
    ~ipmRecord();

    /* Parse response to the RECORD command into component variables */
    void parse(uint8_t *cp, uint16_t *sp, uint32_t *lp);
    /* Build a comma delimited string to send as a UDP packet to nidas */
    void createUDP(char *buffer, int scaleflag);
    /* Return time since power-up in minutes */
    float getTimeSincePowerup();

    // CRC validation
    uint32_t _crcTable[256];
    void generateCRCTable();
    uint32_t calculateCRC32(unsigned char *buf, int ByteCount);
    void checkCRC(uint8_t *cp, uint32_t crc);
};

#endif /* RECORD_H */
