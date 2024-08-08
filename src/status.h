/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <arpa/inet.h>
#include <iostream>

#ifndef STATUS_H
#define STATUS_H
struct
{
    uint8_t OPSTATE;        // Operational State
    uint8_t POWEROK;        // Power OK
    uint32_t TRIPFLAGS;     // Power Trip Flags, performance exceeds limit
    uint32_t CAUTIONFLAGS;  // Power Caution Flags, marginal performance
    uint16_t BITSTAT;       // bitStatus
} status;

class ipmStatus
{
    public:

        ipmStatus();
        ~ipmStatus();

        /* Parse response to the STATUS command into component variables */
        void parse(uint8_t *cp, uint16_t *sp);
        /* Build a comma delimited string to send as a UDP packet to nidas */
        void createUDP(char *buffer, int scaleflag, int badData);
};

#endif /* STATUS_H */