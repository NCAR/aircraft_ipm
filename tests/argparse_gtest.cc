/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions

namespace argtests
{
    #include "../src/argparse.cc"
    #include "../src/cmd.cc"
}

using namespace argtests;

class ArgTest : public ::testing::Test {
private:

    ipmArgparse _args;
    void SetUp()
    {
    }

    void TearDown()
    {
    }
};


/********************************************************************
 ** Test parsing addrinfo block
 ********************************************************************
*/
TEST_F(ArgTest, ParseAddrInfo)
{
    // Parse the addInfo block
    char addrinfo[12];
    strcpy(addrinfo, "0,1,5,30101");
    _args.setAddrInfo(0, addrinfo);
    bool stat = _args.parse_addrInfo(0);
    EXPECT_EQ(stat, 0);

    strcpy(addrinfo, "0,5,30101");
    _args.setAddrInfo(0, addrinfo);
    stat = _args.parse_addrInfo(0);
    EXPECT_EQ(stat, 1);
    EXPECT_EQ(_args.Addr(0), 0);
    EXPECT_EQ(_args.Procqueries(0), 5);
    EXPECT_EQ(_args.Addrport(0), 30101);
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
 ********************************************************************
*/

TEST_F(ArgTest, SetGetIpmPort)
{
    // device iPM is connected to
    _args.setDevice("/dev/ttyS0");
    EXPECT_EQ(_args.Device(), "/dev/ttyS0");
}

TEST_F(ArgTest, SetGetStatusPort)
{
    // port to send status messages to
    // Not yet implemented
}

TEST_F(ArgTest, SetGetMeasureRate)
{
    // STATUS & MEASURE collection rate (hz)
    _args.setRate("1");
    EXPECT_EQ(atoi(_args._measureRate), 1);
}

TEST_F(ArgTest, SetGetRecordPeriod)
{
    // period of RECORD queries (minutes)
    _args.setPeriod("10");
    EXPECT_EQ(atoi(_args._recordPeriod), 10);
}

TEST_F(ArgTest, SetGetBaudRate)
{
    // baud rate
    _args.setBaud("115200");
    EXPECT_EQ(atoi(_args._baudRate), 115200);
}

TEST_F(ArgTest, SetGetNumAddr)
{
    // number of active address on iPM
    _args.setNumAddr("1");
    EXPECT_EQ(_args.numAddr(), 1);
}

TEST_F(ArgTest, SetGetAddrInfo)
{
    // address info block
    _args.setAddrInfo(0, (char *)"0,1,5,30101");
    EXPECT_EQ(_args._addrinfo[0], "0,1,5,30101");
}

TEST_F(ArgTest, SetGetInteractive)
{
    // Interactive mode
    _args.setInteractive();
    EXPECT_EQ(_args.Interactive(), true);
}

TEST_F(ArgTest, SetGetAddress)
{
    // address from command line in interactive mode
    _args.setAddress("2");
    EXPECT_EQ(_args.Address(), "2");
}

TEST_F(ArgTest, SetGetCmd)
{
    // ipm cmd from command line in interactive mode
    _args.setCmd("MEASURE?");
    EXPECT_EQ(_args.Cmd(), "MEASURE?");
}

TEST_F(ArgTest, SetGetVerbose)
{
    // Verbose mode
    _args.setVerbose();
    EXPECT_EQ(_args.Verbose(), true);
}

TEST_F(ArgTest, SetGetEmulate)
{
    // Emulate mode
    _args.setEmulate();
    EXPECT_EQ(_args.Emulate(), true);
}
