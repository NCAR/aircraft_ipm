/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions
#include "../src/status.cc"

class StatusTest : public ::testing::Test {
public:
        char buffer[1000];
private:
    ipmStatus _status;

    void SetUp()
    {
        // Set binary data to some actual data from the iPM
        unsigned char status[] = {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        memcpy(buffer, status, 12);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response string
 ********************************************************************
*/
TEST_F(StatusTest, Parse)
{
    // STATUS,2,1,0,0,0
    char *data = &buffer[0];
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    _status.parse(cp, sp);
    EXPECT_EQ(status.OPSTATE,2);
    EXPECT_EQ(status.POWEROK,1);
    EXPECT_EQ(status.TRIPFLAGS,0);
    EXPECT_EQ(status.CAUTIONFLAGS,0);
    EXPECT_EQ(status.BITSTAT,0);
}

/********************************************************************
 ** Test creating UDP packet
 ********************************************************************
*/
TEST_F(StatusTest, CreateUDP)
{
    std::string str;

    _status.createUDP(buffer, 1, 0);  // scaling turned on
    str = buffer;
    EXPECT_EQ(str,"STATUS,2,1,0,0,0,0\r\n");

    _status.createUDP(buffer, 0, 0);  // scaling turned off
    str = buffer;
    EXPECT_EQ(str,"STATUS,02,01,0000,0000,0000\r\n");
}
