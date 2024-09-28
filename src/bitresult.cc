/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include "bitresult.h"

ipmBitresult::ipmBitresult()
{
    // unit conversions
    _deci = 0.1;
}

ipmBitresult::~ipmBitresult()
{
}

void ipmBitresult::parse(uint16_t *sp)
{

    // There is a typo in the programming manual. Byte should start at
    // zero.
    bitresult.bitStatus = sp[0];
    bitresult.hREFV = sp[1];  // Half-Ref voltage (4.89mV)
    bitresult.VREFV = sp[2];  // VREF voltage (4.89mV)
    bitresult.FIVEV = sp[3];  // +5V voltage (9.78mV)
    bitresult.FIVEVA = sp[4]; // +5VA voltage (9.78mV)
    bitresult.RDV = sp[5];    // Relay Drive Voltage (53.76mV)
    // bytes 13-14 and 15-16 are reserved
    bitresult.ITVA = sp[8];   // Phase A input test voltage (4.89mV) - reserved
    bitresult.ITVB = sp[9];   // Phase B input test voltage (4.89mV) - reserved
    bitresult.ITVC = sp[10];  // Phase C input test voltage (4.89mV) - reserved
    bitresult.TEMP = sp[11];  // Temperature (0.1C)
}

void ipmBitresult::createUDP(char *buffer, int scaleflag)
{
    if (scaleflag >= 1) {
        snprintf(buffer, 255, "BITRESULT,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
             "%.2f,%.2f,%.2f\r\n", (float) bitresult.bitStatus,
             (float) bitresult.hREFV, (float) bitresult.VREFV,
             (float) bitresult.FIVEV, (float) bitresult.FIVEVA,
             (float) bitresult.RDV, (float) bitresult.ITVA,
             (float) bitresult.ITVB, (float) bitresult.ITVC,
             bitresult.TEMP * _deci);
             ;
    } else {
        snprintf(buffer, 255, "BITRESULT,%04x,%04x,%04x,%04x,%04x,%04x,%04x,"
             "%04x,%04x,%04x\r\n", bitresult.bitStatus,
             bitresult.hREFV, bitresult.VREFV, bitresult.FIVEV,
             bitresult.FIVEVA, bitresult.RDV, bitresult.ITVA,
             bitresult.ITVB, bitresult.ITVC, bitresult.TEMP);
    }
}

float ipmBitresult::getTemperature()
{
    return bitresult.TEMP * _deci;
}
