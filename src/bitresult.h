/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <stdint.h>

#ifndef BITRESULT_H
#define BITRESULT_H
struct
{
    float bitStatus;
    float hREFV;  // Half-Ref voltage (4.89mV)
    float VREFV;  // VREF voltage (4.89mV)
    float FIVEV;  // +5V voltage (9.78mV)
    float FIVEVA; // +5VA voltage (9.78mV)
    float RDV;    // Relay Drive Voltage (53.76mV)
    // bytes 13-14 and 15-16 are reserved
    float ITVA;   // Phase A input test voltage (4.89mV) - reserved
    float ITVB;   // Phase B input test voltage (4.89mV) - reserved
    float ITVC;   // Phase C input test voltage (4.89mV) - reserved
    float TEMP;   // Temperature (0.1C)
} bitresult;


class ipmBitresult
{
    public:

        //unit conversions
        float _deci;  // 0.1

        ipmBitresult();
        ~ipmBitresult();

        /* Parse response to the BITRESULT command into component variables */
        void parse(uint16_t *sp);
};

#endif /* BITRESULT_H */
