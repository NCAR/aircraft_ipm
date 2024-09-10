/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <iomanip>
#include "record.h"

ipmRecord::ipmRecord()
{
    // unit conversions
    _deci = 0.1;
    _milli = 0.001;

    // generate CRC
    generateCRCTable();
}

ipmRecord::~ipmRecord()
{
}

void ipmRecord::parse(uint8_t *cp, uint16_t *sp, uint32_t *lp)
{
    record.EVTYPE = cp[0];    // Event Type
    // Event Type: 0 - Max Interval; 1 - Power Up; 2 - Power Down; 3 - Off;
    //             4 - Reset; 5 - Trip; 6 - Fail; 7 - Output On; 8 - Output Off

    record.OPSTATE = cp[1];    // Operating State
    // There is a typo in the programming manual. Power up count should
    // be 4 bytes and elapsed time should start at byte 6 and be 4 bytes
    // as coded here.
    record.POWERCNT = (((long)sp[2]) << 16) | sp[1];  // Power Up Count
    record.TIME = (((long)sp[4]) << 16) | sp[3]; // Power Up Time (1 ms)
    record.TFLAG = (((long)sp[6]) << 16) | sp[5];  // Trip Flag
    record.CFLAG = (((long)sp[8]) << 16) | sp[7];  // Caution Flag
    record.VRMSMINA = sp[9];   // Phase A Voltage Min (0.1 V rms)
    record.VRMSMAXA = sp[10];  // Phase A Voltage Max (0.1 V rms)
    record.VRMSMINB = sp[11];  // Phase B Voltage Min (0.1 V rms)
    record.VRMSMAXB = sp[12];  // Phase B Voltage Max (0.1 V rms)
    record.VRMSMINC = sp[13];  // Phase C Voltage Min (0.1 V rms)
    record.VRMSMAXC = sp[14];  // Phase C Voltage Max (0.1 V rms)
    record.FREQMIN = sp[15];   // Frequency Min (0.1 Hz)
    record.FREQMAX = sp[16];   // Frequency Max (0.1 Hz)
    record.VDCMINA = sp[17];   // Phase A DC Content Min (1 mV)
    record.VDCMAXA = sp[18];   // Phase A DC Content Max (1 mV)
    record.VDCMINB = sp[19];   // Phase B DC Content Min (1 mV)
    record.VDCMAXB = sp[20];   // Phase B DC Content Max (1 mV)
    record.VDCMINC = sp[21];   // Phase C DC Content Min (1 mV)
    record.VDCMAXC = sp[22];   // Phase C DC Content Max (1 mV)
    record.THDMINA = cp[46];   // Phase A Distortion Min (0.1 %)
    record.THDMAXA = cp[47];   // Phase A Distortion Max (0.1 %)
    record.THDMINB = cp[48];   // Phase B Distortion Min (0.1 %)
    record.THDMAXB = cp[49];   // Phase B Distortion Max (0.1 %)
    record.THDMINC = cp[50];   // Phase C Distortion Min (0.1 %)
    record.THDMAXC = cp[51];   // Phase C Distortion Max (0.1 %)
    record.VPKMINA = sp[26];   // Phase A Peak Voltage Min (0.1 V)
    record.VPKMAXA = sp[27];   // Phase A Peak Voltage Max (0.1 V)
    record.VPKMINB = sp[28];   // Phase B Peak Voltage Min (0.1 V)
    record.VPKMAXB = sp[29];   // Phase B Peak Voltage Max (0.1 V)
    record.VPKMINC = sp[30];   // Phase C Peak Voltage Min (0.1 V)
    record.VPKMAXC = sp[31];   // Phase C Peak Voltage Max (0.1 V)
    record.CRC = lp[16];       // CRC-32
}

void ipmRecord::createUDP(char *buffer, int scaleflag)
{
    if (scaleflag >= 1) {
        snprintf(buffer, 255, "RECORD,%u,%u,%u,%u,%u,%u,%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f,"
            "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
            "%u\r\n",
            record.EVTYPE, record.OPSTATE, record.POWERCNT,
            record.TIME, record.TFLAG, record.CFLAG,
            record.VRMSMINA * _deci, record.VRMSMAXA * _deci,
            record.VRMSMINB * _deci, record.VRMSMAXB * _deci,
            record.VRMSMINC * _deci, record.VRMSMAXC * _deci,
            record.FREQMIN * _deci, record.FREQMAX * _deci,
            record.VDCMINA * _milli, record.THDMAXA * _milli,
            record.THDMINB * _milli, record.VDCMAXB * _milli,
            record.VDCMINC * _milli, record.VDCMAXC * _milli,
            record.THDMINA * _deci, record.THDMAXA * _deci,
            record.THDMINB * _deci, record.THDMAXB * _deci,
            record.THDMINC * _deci, record.THDMAXC * _deci,
            record.VPKMINA * _deci, record.VPKMAXA * _deci,
            record.VPKMINB * _deci, record.VPKMAXB * _deci,
            record.VPKMINC * _deci, record.VPKMAXC * _deci,
            record.CRC);
    } else {
        snprintf(buffer, 255, "RECORD,%02x,%02x,%08x,%08x,%08x,%08x,%04x,"
            "%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,"
            "%04x,%02x,%02x,%02x,%02x,%02x,%02x,%04x,%04x,%04x,%04x,%04x,"
            "%04x,%08x\r\n",
            record.EVTYPE, record.OPSTATE, record.POWERCNT, record.TIME,
            record.TFLAG, record.CFLAG, record.VRMSMINA, record.VRMSMAXA,
            record.VRMSMINB, record.VRMSMAXB, record.VRMSMINC,
            record.VRMSMAXC, record.FREQMIN, record.FREQMAX,
            record.VDCMINA, record.THDMAXA, record.THDMINB, record.VDCMAXB,
            record.VDCMINC, record.VDCMAXC, record.THDMINA, record.THDMAXA,
            record.THDMINB, record.THDMAXB, record.THDMINC, record.THDMAXC,
            record.VPKMINA, record.VPKMAXA, record.VPKMINB, record.VPKMAXB,
            record.VPKMINC, record.VPKMAXC, record.CRC);
    }
}

float ipmRecord::getTimeSincePowerup()
{
    return record.TIME/60000;
}

/********************************************************************
 ** CRC validation
 ** Compare CRC to Reversed 0xEDB88320 at
 ** https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
 ** My crc calculation here match the online tool, but the CRC returned
 ** by the iPM does not. I tried both including and excluding the
 ** response length in the CRC but cannot match the returned value.
 ** Leaving the code here so that this can be investigated more later if
 ** desired.
 ********************************************************************
*/
void ipmRecord::generateCRCTable()
{
    uint32_t crc, poly;
    int i, j;

    poly = 0xEDB88320;
    for (i=0; i < 256; i++)
    {
        crc = i;
        for (j = 8; j > 0; j--)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
        _crcTable[i] = crc;
    }
}

uint32_t ipmRecord::calculateCRC32 (unsigned char *buf, int ByteCount)
{
    uint32_t crc;
    int i, j, ch;

    crc = 0xFFFFFFFF;
    for (i=0; i < ByteCount; i++)
    {
        ch = *buf++;
        crc = (crc>>8) ^ _crcTable[(crc ^ ch) & 0xFF];
    }
    return (crc ^ 0xFFFFFFFF);
}

/* Print a hex line of data suitable for copy/paste into the above online
   calculator.
*/
void ipmRecord::checkCRC(uint8_t *cp, uint32_t crc)
{
    for (int j=0;j<64;j++)
    {
        char c = cp[j];
        unsigned int i = (unsigned char)c;
        std::cout << std::setfill ('0') << std::setw(2) << std::hex << i;
    }
    std::cout << std::dec << std::endl;

    // Print out the CRC calculated here and the CRC from the data in hex
    // so can easily compare to output from online calculator.
    std::cout << std::hex << crc << std::dec << " : calculated CRC "
        << std::endl;
    std::cout << std::hex << record.CRC << std::dec << " : CRC from iPM"
        << std::endl;
}
