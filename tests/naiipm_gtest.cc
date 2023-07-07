#include <gtest/gtest.h>
#include "../naiipm.cc"

class IpmTest : public ::testing::Test {
protected:
    naiipm ipm;

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

// Test storing and retrieving command line args
TEST_F(IpmTest, ipmArgs)
{
    ipm.setNumAddr("1");
    EXPECT_EQ(atoi(ipm.numAddr()), 1);

    ipm.setPort("/dev/ttyS0");
    EXPECT_EQ(ipm.Port(), "/dev/ttyS0");
}
