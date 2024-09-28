/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <stdint.h>
#include <iostream>

#ifndef BITRESULT_H
#define BITRESULT_H

class ipmBitresult
{

private:

    struct
    {
        uint16_t bitStatus;
        uint16_t hREFV;  // Half-Ref voltage (4.89mV)
        uint16_t VREFV;  // VREF voltage (4.89mV)
        uint16_t FIVEV;  // +5V voltage (9.78mV)
        uint16_t FIVEVA; // +5VA voltage (9.78mV)
        uint16_t RDV;    // Relay Drive Voltage (53.76mV)
        // bytes 13-14 and 15-16 are reserved
        uint16_t ITVA;   // Phase A input test voltage (4.89mV) - reserved
        uint16_t ITVB;   // Phase B input test voltage (4.89mV) - reserved
        uint16_t ITVC;   // Phase C input test voltage (4.89mV) - reserved
        uint16_t TEMP;   // Temperature (0.1C)
    } bitresult;


public:

    //unit conversions
    float _deci;  // 0.1

    ipmBitresult();
    ~ipmBitresult();

    /* Parse response to the BITRESULT command into component variables */
    void parse(uint16_t *sp);
    /* Build a comma delimited string */
    void createUDP(char *buffer, int scaleflag);
    /* Return temperature scaled to degrees C */
    float getTemperature();
};

#endif /* BITRESULT_H */
