#include <arpa/inet.h>
#include <iostream>

#ifndef MEASURE_H
#define MEASURE_H
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


class ipmMeasure
{
    public:

        //unit conversions
        float _deci;  // 0.1
        float _milli;  // 0.001

        ipmMeasure();
        ~ipmMeasure();
        void parseMeasure(uint8_t *cp, uint16_t *sp);
        void createMeasureLine(char *buffer, int scaleflag);
};

#endif /* MEASURE_H */
