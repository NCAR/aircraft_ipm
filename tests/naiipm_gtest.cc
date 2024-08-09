/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions
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
 ** Test setting interactive mode
 ********************************************************************
*/
TEST_F(IpmTest, ipmSetInteractiveModeAnoC)
{

    testing::internal::CaptureStdout();
    int fd = 1;  // Set to stdout

    // got -a but not -c
    const char *addr = "2";
    ipm.setAddress(addr);
    ipm.setInteractiveMode(fd);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Command  is invalid. Please enter a valid command\n");
}

TEST_F(IpmTest, ipmSetInteractiveModeCnoA)
{
    testing::internal::CaptureStdout();
    int fd = 1;  // Set to stdout

    // got -c but not -a (so default to zero)
    ipm.setCmd("RECORD?");
    ipm.setInteractiveMode(fd);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Setting default address of 0\n");
}

TEST_F(IpmTest, ipmVerify)
{
    testing::internal::CaptureStdout();

    // Invalid iPM query
    ipm.setCmd("REC");
    ipm.verify(ipm.Cmd());
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Command REC is invalid. Please enter a valid command\n");
}

/********************************************************************
 ** Test removing inactive address
 ********************************************************************
*/
TEST_F(IpmTest, ipmRmAddr)
{
    testing::internal::CaptureStdout();
    char addrinfo[12];

    ipm.setNumAddr("4");
    strcpy(addrinfo, "0,1,30101");
    ipm.setAddrInfo(0, addrinfo);
    ipm.parse_addrInfo(0);
    strcpy(addrinfo, "2,5,30102");
    ipm.setAddrInfo(1, addrinfo);
    ipm.parse_addrInfo(1);
    strcpy(addrinfo, "5,7,30103");
    ipm.setAddrInfo(2, addrinfo);
    ipm.parse_addrInfo(2);
    strcpy(addrinfo, "6,7,30104");
    ipm.setAddrInfo(3, addrinfo);
    ipm.parse_addrInfo(3);

    EXPECT_EQ(ipm.numAddr(), 4);
    EXPECT_EQ(ipm.addr(0), 0);
    EXPECT_EQ(ipm.procqueries(0), 1);
    EXPECT_EQ(ipm.addrport(0), 30101);
    EXPECT_EQ(ipm.addr(1), 2);
    EXPECT_EQ(ipm.procqueries(1), 5);
    EXPECT_EQ(ipm.addrport(1), 30102);
    EXPECT_EQ(ipm.addr(2), 5);
    EXPECT_EQ(ipm.procqueries(2), 7);
    EXPECT_EQ(ipm.addrport(2), 30103);
    EXPECT_EQ(ipm.addr(3), 6);
    EXPECT_EQ(ipm.procqueries(3), 7);
    EXPECT_EQ(ipm.addrport(3), 30104);

    ipm.rmAddr(1);  // remove address at index 1
    EXPECT_EQ(ipm.numAddr(), 3);
    EXPECT_EQ(ipm.addr(0), 0);
    EXPECT_EQ(ipm.procqueries(0), 1);
    EXPECT_EQ(ipm.addrport(0), 30101);
    EXPECT_EQ(ipm.addr(1), 5);
    EXPECT_EQ(ipm.procqueries(1), 7);
    EXPECT_EQ(ipm.addrport(1), 30103);
    EXPECT_EQ(ipm.addr(2), 6);
    EXPECT_EQ(ipm.procqueries(2), 7);
    EXPECT_EQ(ipm.addrport(2), 30104);

    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Removing address 2 from active address list\n");

}

/********************************************************************
 ** Test parsing response strings
 ********************************************************************
*/
TEST_F(IpmTest, ipmParseData)
{

    ipm.open_udp("192.168.84.2");

    std::string str;
    char addrinfo[12];

    // do not scale
    strcpy(addrinfo, "0,5,30101");
    ipm.setScaleFlag(0);  // do not scale
    ipm.setAddrInfo(0, addrinfo);
    ipm.parse_addrInfo(0);

    testing::internal::CaptureStdout();
    ipm.parseData("MEASURE?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string MEASURE,0258,0205,048b,048b,0000,0604,05fc,0000,001c,001c,0009,0dc9,06c8,0707,1b,1b,01,01\r\n");

    testing::internal::CaptureStdout();
    ipm.parseData("STATUS?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string STATUS,02,01,0000,0000,0000\r\n");

    testing::internal::CaptureStdout();
    ipm.parseData("RECORD?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string RECORD,00,02,00000063,0469448b,00000000,00000000,00d1,049b,00d1,049b,0000,0000,0245,0258,0000,0071,001a,0055,0000,0015,1a,71,1a,71,01,01,0604,065a,0604,0653,0000,0018,087c1b13\r\n");

    // scale
    strcpy(addrinfo, "0,5,30101");
    ipm.setScaleFlag(1);  // scale
    ipm.setAddrInfo(0, addrinfo);
    ipm.parse_addrInfo(0);

    testing::internal::CaptureStdout();
    ipm.parseData("MEASURE?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string MEASURE,60.00,51.70,116.30,116.30,0.00,154.00,153.20,0.00,0.0280,0.0280,0.0090,352.90,173.60,179.90,2.70,2.70,0.10,1\r\n");

    testing::internal::CaptureStdout();
    ipm.parseData("STATUS?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string STATUS,2,1,0,0,0,0\r\n");

    testing::internal::CaptureStdout();
    ipm.parseData("RECORD?", 0);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "sending to port 30101 UDP string RECORD,0,2,99,74007691,0,0,20.90,117.90,20.90,117.90,0.00,0.00,58.10,60.00,0.0000,0.1130,0.0260,0.0850,0.0000,0.0210,2.60,11.30,2.60,11.30,0.10,0.10,154.00,162.60,154.00,161.90,0.00,2.40,142351123\r\n");

    ipm.close_udp(atoi(ipm.Address()));
}

TEST_F(IpmTest, ipmParseBitresult)
{
    char *data = ipm.getData("BITRESULT?");
    uint16_t *sp = (uint16_t *)data;
    ipm.parseBitresult(sp);
}

TEST_F(IpmTest, ipmParseAddrInfo)
{
    // Parse the addInfo block
    char addrinfo[12];
    strcpy(addrinfo, "0,1,5,30101");
    ipm.setAddrInfo(0, addrinfo);
    bool stat = ipm.parse_addrInfo(0);
    EXPECT_EQ(stat, 0);

    strcpy(addrinfo, "0,5,30101");
    ipm.setAddrInfo(0, addrinfo);
    stat = ipm.parse_addrInfo(0);
    EXPECT_EQ(stat, 1);
    EXPECT_EQ(ipm.addr(0), 0);
    EXPECT_EQ(ipm.procqueries(0), 5);
    EXPECT_EQ(ipm.addrport(0), 30101);
}

/********************************************************************
 ** Test implementation of measureRate (hz) and recordPeriod (minutes)
 ********************************************************************
*/
TEST_F(IpmTest, ipmSleep)
{
    ipm.setRate("1");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 800000);

    ipm.setRate("5");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 0);

    ipm.setRate("2");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 300000);
}

TEST_F(IpmTest, ipmSetRecordFreq)
{
    ipm.setRate("1");  // hz
    ipm.setPeriod("10");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 600);

    ipm.setRate("2");  // hz
    ipm.setPeriod("1");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 120);

    ipm.setRate("5");  // hz
    ipm.setPeriod("1");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 300);
}


/********************************************************************
 ** Test storing and retrieving command line args.  ipm_ctrl accepts
 ** the following command line arguments:
 **  -D <device>         device iPM is connected to
 **  -s <port>           port to send status msgs to
 **  -m <measurerate>    STATUS & MEASURE collection rate  (hz)
 **  -r <recordperiod>   period of RECORD queries (minutes)
 **  -b <baudrate>       baud rate
 **  -n <num_addr>       number of active addresses on iPM
 **  -# <addr,procqueries,port>
 **                      number 0 to n-1 followed by info block
 **  -i                  run in interactive mode (optional)
 **  -a <address>        set address (optional)
 **  -c <command>        set command (optional)
 **  -v                  run in verbose mode (optional)
 **  -H                  run in hexadecimal output mode, comma-delimited
 **                      don't scale vars (optional)
 **  -e                  run with emulator; longer timeout (optional)
 **
 ** 2023, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

TEST_F(IpmTest, ipmSetGetIpmPort)
{
    // device iPM is connected to
    ipm.setDevice("/dev/ttyS0");
    EXPECT_EQ(ipm.Device(), "/dev/ttyS0");
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
    EXPECT_EQ(ipm.numAddr(), 1);
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

TEST_F(IpmTest, ipmSetGetAddress)
{
    // address from command line in interactive mode
    ipm.setAddress("2");
    EXPECT_EQ(ipm.Address(), "2");
}

TEST_F(IpmTest, ipmSetGetCmd)
{
    // ipm cmd from command line in interactive mode
    ipm.setCmd("MEASURE?");
    EXPECT_EQ(ipm.Cmd(), "MEASURE?");
}

TEST_F(IpmTest, ipmSetGetVerbose)
{
    // Verbose mode
    ipm.setVerbose();
    EXPECT_EQ(ipm.Verbose(), true);
}

TEST_F(IpmTest, ipmSetGetEmulate)
{
    // Emulate mode
    ipm.setEmulate();
    EXPECT_EQ(ipm.Emulate(), true);
}
