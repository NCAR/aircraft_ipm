/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include "measure.h"

ipmMeasure::ipmMeasure()
{
    // unit conversions
    _deci = 0.1;
    _milli = 0.001;
}

ipmMeasure::~ipmMeasure()
{
}

void ipmMeasure::parseMeasure(uint8_t *cp, uint16_t *sp)
{
    // There is a typo in the programming manual. Byte should start at
    // zero.
    measure.FREQ = sp[0];   // Frequency (0.1Hz)
    // bytes 3-4 are reserved
    measure.TEMP = sp[2];   // Temperature (0.1 C)
    measure.VRMSA = sp[3];  // Phase A voltage rms (0.1 V)
    measure.VRMSB = sp[4];  // Phase B voltage rms (0.1 V)
    measure.VRMSC = sp[5];  // Phase C voltage rms (0.1 V)
    measure.VPKA = sp[6];   // Phase A voltage peak (0.1 V)
    measure.VPKB = sp[7];   // Phase B voltage peak (0.1 V)
    measure.VPKC = sp[8];   // Phase C voltage peak (0.1 V)
    measure.VDCA = sp[9];   // Phase A DC component (1 mV DC)
    measure.VDCB = sp[10];  // Phase B DC component (1 mV DC)
    measure.VDCC = sp[11];  // Phase C DC component (1 mV DC)
    measure.PHA = sp[12];   // Phase A Phase Angle (0.1 deg) rel. to Phase A
    measure.PHB = sp[13];   // Phase B Phase Angle (0.1 deg) rel. to Phase B
    measure.PHC = sp[14];   // Phase C Phase Angle (0.1 deg) rel. to Phase C
    measure.THDA = cp[30];  // Phase A THD (0.1%)
    measure.THDB = cp[31];  // Phase B THD (0.1%)
    measure.THDC = cp[32];  // Phase C THD (0.1%)
    measure.POWEROK = cp[33];  // PowerOK (1 = power good, 0 = no good)
}

void ipmMeasure::createMeasureLine(char *buffer, int scaleflag)
{
    if (scaleflag >= 1) {
        snprintf(buffer, 255, "MEASURE,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
             "%.2f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u\r\n",
             measure.FREQ * _deci, measure.TEMP * _deci,
             measure.VRMSA * _deci, measure.VRMSB * _deci,
             measure.VRMSC * _deci, measure.VPKA * _deci,
             measure.VPKB * _deci, measure.VPKC * _deci,
             measure.VDCA * _milli, measure.VDCB * _milli,
             measure.VDCC * _milli, measure.PHA * _deci,
             measure.PHB * _deci, measure.PHC * _deci,
             measure.THDA * _deci, measure.THDB * _deci,
             measure.THDC * _deci, measure.POWEROK);
    } else {
        snprintf(buffer, 255, "MEASURE,%04x,%04x,%04x,%04x,%04x,%04x,%04x,"
             "%04x,%04x,%04x,%04x,%04x,%04x,%04x,%02x,%02x,%02x,%02x\r\n",
             measure.FREQ, measure.TEMP,
             measure.VRMSA, measure.VRMSB,
             measure.VRMSC, measure.VPKA,
             measure.VPKB, measure.VPKC,
             measure.VDCA, measure.VDCB,
             measure.VDCC, measure.PHA,
             measure.PHB, measure.PHC,
             measure.THDA, measure.THDB,
             measure.THDC, measure.POWEROK);
    }
}
