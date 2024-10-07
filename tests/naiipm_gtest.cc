/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions

#include "../naiipm.cc"
#include "../src/argparse.cc"
#include "../src/cmd.cc"

using ::testing::Return;

class MockNaiipm : public naiipm {
public:
    // Define methods to be mocked
    MOCK_METHOD(bool, send_command, (int fd, std::string msg,
        std::string msgarg), (override));
    MOCK_METHOD(bool, setActiveAddress, (int fd, int addr),
        (override));
};

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
 ** Test send_command function
 ********************************************************************
*/
TEST_F(IpmTest, ipmSendBadCommand)
{
    testing::internal::CaptureStdout();

    int fd = 1;  // Set to stdout
    EXPECT_EQ(ipm.send_command(fd, "BADCOMMAND"), false);

    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Command BADCOMMAND is invalid. Please enter a valid command\n");
}
/********************************************************************
 ** Test clear function (using mock)
 ********************************************************************
*/
TEST_F(IpmTest, ipmClear)
{
    int fd = 1;  // Set to stdout
    int addr = 2;
    std::string msg = "VER?";
    // Instantiate the mock naiipm class
    MockNaiipm mipm;
    // Define what the mocked functions will return
    EXPECT_CALL(mipm, setActiveAddress)
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mipm, send_command)
        .Times(1)
        .WillRepeatedly(Return(true));

    // Do the test, using the mocked instance, mipm
    testing::internal::CaptureStdout();

    bool status = mipm.clear(fd, addr);
    EXPECT_EQ(status, true);

    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Took 0 ADR commands to clear iPM on init\n");
}
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
    args.setAddress(addr);
    args.setCmd("");
    ipm.setInteractiveMode(fd);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Command  is invalid. Please enter a valid command\n");
}

TEST_F(IpmTest, ipmSetInteractiveModeCnoA)
{
    testing::internal::CaptureStdout();
    int fd = 1;  // Set to stdout

    // got -c but not -a (so default to zero)
    args.setCmd("RECORD?");
    args.setAddress("-1");
    ipm.setInteractiveMode(fd);
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Setting default address of 0\n");
}

/********************************************************************
 ** Test removing inactive address
 ********************************************************************
*/
TEST_F(IpmTest, ipmRmAddr)
{
    testing::internal::CaptureStdout();
    char addrinfo[12];

    args.setNumAddr("4");
    strcpy(addrinfo, "0,1,30101");
    args.setAddrInfo(0, addrinfo);
    args.parse_addrInfo(0);
    strcpy(addrinfo, "2,5,30102");
    args.setAddrInfo(1, addrinfo);
    args.parse_addrInfo(1);
    strcpy(addrinfo, "5,7,30103");
    args.setAddrInfo(2, addrinfo);
    args.parse_addrInfo(2);
    strcpy(addrinfo, "6,7,30104");
    args.setAddrInfo(3, addrinfo);
    args.parse_addrInfo(3);

    EXPECT_EQ(args.numAddr(), 4);
    EXPECT_EQ(args.Addr(0), 0);
    EXPECT_EQ(args.Procqueries(0), 1);
    EXPECT_EQ(args.Addrport(0), 30101);
    EXPECT_EQ(args.Addr(1), 2);
    EXPECT_EQ(args.Procqueries(1), 5);
    EXPECT_EQ(args.Addrport(1), 30102);
    EXPECT_EQ(args.Addr(2), 5);
    EXPECT_EQ(args.Procqueries(2), 7);
    EXPECT_EQ(args.Addrport(2), 30103);
    EXPECT_EQ(args.Addr(3), 6);
    EXPECT_EQ(args.Procqueries(3), 7);
    EXPECT_EQ(args.Addrport(3), 30104);

    ipm.rmAddr(1);  // remove address at index 1
    EXPECT_EQ(args.numAddr(), 3);
    EXPECT_EQ(args.Addr(0), 0);
    EXPECT_EQ(args.Procqueries(0), 1);
    EXPECT_EQ(args.Addrport(0), 30101);
    EXPECT_EQ(args.Addr(1), 5);
    EXPECT_EQ(args.Procqueries(1), 7);
    EXPECT_EQ(args.Addrport(1), 30103);
    EXPECT_EQ(args.Addr(2), 6);
    EXPECT_EQ(args.Procqueries(2), 7);
    EXPECT_EQ(args.Addrport(2), 30104);

    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Removing address 2 from active address list\n");

}

/********************************************************************
 ** Test parsing response strings
 ********************************************************************
*/
TEST_F(IpmTest, ipmParseDataNoScale)
{

    ipm.open_udp("192.168.84.2");

    std::string str;
    char addrinfo[12];

    // do not scale
    strcpy(addrinfo, "0,5,30101");
    args.setScaleFlag(0);  // do not scale
    args.setAddrInfo(0, addrinfo);
    args.parse_addrInfo(0);

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
        "sending to port 30101 UDP string RECORD,00,02,00000063,0469448b,00000000,00000000,00d1,049b,00d1,049b,0000,0000,0245,0258,0000,005e,0000,0055,0000,0015,1a,71,1a,71,01,01,0604,065a,0604,0653,0000,0018,087c1b13\r\n");
}

TEST_F(IpmTest, ipmParseDataScale)
{
    ipm.open_udp("192.168.84.2");

    std::string str;
    char addrinfo[12];

    // scale
    strcpy(addrinfo, "0,5,30101");
    args.setScaleFlag(1);  // scale
    args.setAddrInfo(0, addrinfo);
    args.parse_addrInfo(0);

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
        "sending to port 30101 UDP string RECORD,0,2,99,74007691,0,0,20.90,117.90,20.90,117.90,0.00,0.00,58.10,60.00,0.0000,0.0940,0.0000,0.0850,0.0000,0.0210,2.60,11.30,2.60,11.30,0.10,0.10,154.00,162.60,154.00,161.90,0.00,2.40,142351123\r\n");

    ipm.close_udp(atoi(args.Address()));
}

/********************************************************************
 ** Test implementation of measureRate (hz) and recordPeriod (minutes)
 ********************************************************************
*/
TEST_F(IpmTest, ipmSleep)
{
    args.setRate("1");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 800000);

    args.setRate("5");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 0);

    args.setRate("2");  // hz
    ipm.sleep();
    EXPECT_EQ(ipm._sleeptime, 300000);
}

TEST_F(IpmTest, ipmSetRecordFreq)
{
    args.setRate("1");  // hz
    args.setPeriod("10");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 600);

    args.setRate("2");  // hz
    args.setPeriod("1");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 120);

    args.setRate("5");  // hz
    args.setPeriod("1");  // minutes
    ipm.setRecordFreq();
    EXPECT_EQ(ipm._recordFreq, 300);
}
