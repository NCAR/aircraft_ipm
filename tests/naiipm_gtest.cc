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
        unsigned char record[] = {0, 2, 99, 0, 0, 0, 139, 68, 105, 4, 0, 0, 0,
            0, 0, 0, 0, 0, 209, 0, 155, 4, 209, 0, 155, 4, 0, 0, 0, 0, 69, 2,
            88, 2, 0, 0, 94, 0, 0, 0, 85, 0, 0, 0, 21, 0, 26, 113, 26, 113, 1,
            1, 4, 6, 90, 6, 4, 6, 83, 6, 0, 0, 24, 0, 19, 27, 124, 8 };
        memcpy(ipm.buffer, record, 68);
        ipm.setData("RECORD?", 68);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response strings
 ********************************************************************
*/
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
