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
    }

    void TearDown()
    {
    }
};

TEST_F(IpmTest, ipmSomething)
{
    // port to send status messages to
    // Not yet implemented
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
