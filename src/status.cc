/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include "status.h"

ipmStatus::ipmStatus()
{
}

ipmStatus::~ipmStatus()
{
}

void ipmStatus::parse(uint8_t *cp, uint16_t *sp)
{

    // There is a typo in the programming manual. Byte should start at
    // zero.
    status.OPSTATE = cp[0];    // Operating State
    // OpState: 0 - Off; 1 = reserved; 2 - reset; 3 - Tripped; 4 - Failed
    status.POWEROK = cp[1];    // PowerOK (1 - power good; 0 - no good)
    status.TRIPFLAGS = (((long)sp[2]) << 16) | sp[1];
    status.CAUTIONFLAGS = (((long)sp[4]) << 16) | sp[3];
    status.BITSTAT = sp[5];   // bitStatus
}

void ipmStatus::createUDP(char *buffer, int scaleflag, int badData)
{
    if (scaleflag >= 1) {
        snprintf(buffer, 255, "STATUS,%u,%u,%u,%u,%u,%d\r\n",
            status.OPSTATE, status.POWEROK, status.TRIPFLAGS,
            status.CAUTIONFLAGS, status.BITSTAT, badData);
    } else {
        snprintf(buffer, 255, "STATUS,%02x,%02x,%04x,%04x,%04x\r\n",
            status.OPSTATE, status.POWEROK, status.TRIPFLAGS,
            status.CAUTIONFLAGS, status.BITSTAT);
    }
}
