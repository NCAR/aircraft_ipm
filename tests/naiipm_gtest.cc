/********************************************************************
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#define private public  // so can test private functions
#include <gtest/gtest.h>
#include "../naiipm.cc"

class IpmTest : public ::testing::Test {
private:
    naiipm ipm;

    void SetUp()
    {
        // Set binary data to some actual data from the iPM
        unsigned char bitresult[] = {0,0,254,1,255,3,25,2,24,2,88,1,0,0,0,0,
           39,2,249,1,249,1,253,1};
        memcpy(ipm.buffer,bitresult,25);
        ipm.setData("BITRESULT?", 25);

        unsigned char record[] = {0, 2, 99, 0, 0, 0, 139, 68, 105, 4, 0, 0, 0,
            0, 0, 0, 0, 0, 209, 0, 155, 4, 209, 0, 155, 4, 0, 0, 0, 0, 69, 2,
            88, 2, 0, 0, 94, 0, 0, 0, 85, 0, 0, 0, 21, 0, 26, 113, 26, 113, 1,
            1, 4, 6, 90, 6, 4, 6, 83, 6, 0, 0, 24, 0, 19, 27, 124, 8 };
        memcpy(ipm.buffer, record, 68);
        ipm.setData("RECORD?", 68);

        unsigned char measure[] = {88, 2, 0, 0, 5, 2, 139, 4, 139, 4, 0, 0, 4,
            6, 252, 5, 0, 0, 28, 0, 28, 0, 9, 0, 201, 13, 200, 6, 7, 7, 27, 27,
            1, 1};
        memcpy(ipm.buffer, measure, 34);
        ipm.setData("MEASURE?", 34);

        unsigned char status[] = {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        memcpy(ipm.buffer, status, 12);
        ipm.setData("STATUS?", 12);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response strings
 ********************************************************************
*/
TEST_F(IpmTest, ipmParseBitresult)
{
    char *data = ipm.getData("BITRESULT?");
    uint16_t *sp = (uint16_t *)data;
    ipm.parseBitresult(sp);
}

TEST_F(IpmTest, ipmParseMeasure)
{
    //MEASURE,60.00,51.70,116.30,116.30,0.00,154.00,153.20,0.00,0.0280,0.0280,
    //    0.0090,352.90,173.60,179.90,2.70,2.70,0.10,1
    char *data = ipm.getData("MEASURE?");
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    ipm.parseMeasure(cp, sp);
    EXPECT_EQ(measure.FREQ,600);
    EXPECT_EQ(measure.TEMP,517);
    EXPECT_EQ(measure.VRMSA,1163);
    EXPECT_EQ(measure.VRMSB,1163);
    EXPECT_EQ(measure.VRMSC,0);
    EXPECT_EQ(measure.VPKA,1540);
    EXPECT_EQ(measure.VPKB,1532);
    EXPECT_EQ(measure.VPKC,0);
    EXPECT_EQ(measure.VDCA,28);
    EXPECT_EQ(measure.VDCB,28);
    EXPECT_EQ(measure.VDCC,9);
    EXPECT_EQ(measure.PHA,3529);
    EXPECT_EQ(measure.PHB,1736);
    EXPECT_EQ(measure.PHC,1799);
    EXPECT_EQ(measure.THDA,27);
    EXPECT_EQ(measure.THDB,27);
    EXPECT_EQ(measure.THDC,1);
    EXPECT_EQ(measure.POWEROK,1);
}
TEST_F(IpmTest, ipmParseStatus)
{
    // STATUS,2,1,0,0,0
    char *data = ipm.getData("STATUS?");
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    ipm.parseStatus(cp, sp);
    EXPECT_EQ(status.OPSTATE,2);
    EXPECT_EQ(status.POWEROK,1);
    EXPECT_EQ(status.TRIPFLAGS,0);
    EXPECT_EQ(status.CAUTIONFLAGS,0);
    EXPECT_EQ(status.BITSTAT,0);
}

TEST_F(IpmTest, ipmParseRecord)
{
    //RECORD,0,2,99,74007691,0,0,20.90,117.90,20.90,117.90,0.00,0.00,58.10,
    //    60.00,0.0000,0.1130,0.0260,0.0850,0.0000,0.0210,2.60,11.30,2.60,
    //    11.30,0.10,0.10,154.00,162.60,154.00,161.90,0.00,2.40,6931
    char *data = ipm.getData("RECORD?");
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;
    ipm.parseRecord(cp, sp, lp);
    EXPECT_EQ(record.EVTYPE,0);
    EXPECT_EQ(record.OPSTATE,2);
    EXPECT_EQ(record.POWERCNT,99);
    EXPECT_EQ(record.TIME,74007691);
    EXPECT_EQ(record.TFLAG,0);
    EXPECT_EQ(record.CFLAG,0);
    EXPECT_EQ(record.VRMSMINA,209);
    EXPECT_EQ(record.VRMSMAXA,1179);
    EXPECT_EQ(record.VRMSMINB,209);
    EXPECT_EQ(record.VRMSMAXB,1179);
    EXPECT_EQ(record.VRMSMINC,0);
    EXPECT_EQ(record.VRMSMAXC,0);
    EXPECT_EQ(record.FREQMIN,581);
    EXPECT_EQ(record.FREQMAX,600);
    EXPECT_EQ(record.VDCMINA,0);
    EXPECT_EQ(record.VDCMAXA,94);
    EXPECT_EQ(record.VDCMINB,0);
    EXPECT_EQ(record.VDCMAXB,85);
    EXPECT_EQ(record.VDCMINC,0);
    EXPECT_EQ(record.VDCMAXC,21);
    EXPECT_EQ(record.THDMINA,26);
    EXPECT_EQ(record.THDMAXA,113);
    EXPECT_EQ(record.THDMINB,26);
    EXPECT_EQ(record.THDMAXB,113);
    EXPECT_EQ(record.THDMINC,1);
    EXPECT_EQ(record.THDMAXC,1);
    EXPECT_EQ(record.VPKMINA,1540);
    EXPECT_EQ(record.VPKMAXA,1626);
    EXPECT_EQ(record.VPKMINB,1540);
    EXPECT_EQ(record.VPKMAXB,1619);
    EXPECT_EQ(record.VPKMINC,0);
    EXPECT_EQ(record.VPKMAXC,24);
    EXPECT_EQ(record.CRC,142351123);
}

TEST_F(IpmTest, ipmParseAddrInfo)
{
    // Parse the addInfo block
    char addrinfo[] = "0,1,5,30101";
    ipm.setAddrInfo(0, addrinfo);
    ipm.parse_addrInfo(0);
    EXPECT_EQ(ipm.addr(0), 0);
    EXPECT_EQ(ipm.numphases(0), 1);
    EXPECT_EQ(ipm.procqueries(0), 5);
    EXPECT_EQ(ipm.addrport(0), 30101);
}

/********************************************************************
 ** Test storing and retrieving command line args.  ipm_ctrl accepts
 ** the following command line arguments:
 **  -p <port>           port iPM is connected to
 **  -s <port>           port to send status msgs to
 **  -m <measurerate>    STATUS & MEASURE collection rate  (hz)
 **  -r <recordperiod>   period of RECORD queries (minutes)
 **  -b <baudrate>       baud rate
 **  -n <num_addr>       number of active addresses on iPM
 **  -# <addr,numphases,procqueries,port>
 **                      number 0 to n-1 followed by info block
 **  -i                  run in interactive mode (optional)
 **
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

TEST_F(IpmTest, ipmSetGetIpmPort)
{
    // port iPM is connected to
    ipm.setPort("/dev/ttyS0");
    EXPECT_EQ(ipm.Port(), "/dev/ttyS0");
}

TEST_F(IpmTest, ipmSetGetStatusPort)
{
    // port to send status messages to
    // Not yet implemented
}

TEST_F(IpmTest, ipmSetGetMeasureRate)
{
    // STATUS & MEASURE collection rate (hz)
    ipm.setRate("1");
    EXPECT_EQ(atoi(ipm._measureRate), 1);
}

TEST_F(IpmTest, ipmSetGetRecordPeriod)
{
    // period of RECORD queries (minutes)
    ipm.setPeriod("10");
    EXPECT_EQ(atoi(ipm._recordPeriod), 10);
}

TEST_F(IpmTest, ipmSetGetBaudRate)
{
    // baud rate
    ipm.setBaud("115200");
    EXPECT_EQ(atoi(ipm._baudRate), 115200);
}

TEST_F(IpmTest, ipmSetGetNumAddr)
{
    // number of active address on iPM
    ipm.setNumAddr("1");
    EXPECT_EQ(atoi(ipm.numAddr()), 1);
}

TEST_F(IpmTest, ipmSetGetAddrInfo)
{
    // address info block
    ipm.setAddrInfo(0, (char *)"0,1,5,30101");
    EXPECT_EQ(ipm._addrinfo[0], "0,1,5,30101");
}

TEST_F(IpmTest, ipmSetGetInteractive)
{
    // Interactive mode
    ipm.setInteractive();
    EXPECT_EQ(ipm.Interactive(), true);
}

TEST_F(IpmTest, ipmSetGetDebug)
{
    // Debug mode
    ipm.setDebug();
    EXPECT_EQ(ipm.Debug(), true);
}

TEST_F(IpmTest, ipmSetGetEmulate)
{
    // Emulate mode
    ipm.setEmulate();
    EXPECT_EQ(ipm.Emulate(), true);
}
