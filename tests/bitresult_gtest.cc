/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions
#include "../src/bitresult.cc"

class BitresultTest : public ::testing::Test {
public:
        char buffer[1000];
private:
    ipmBitresult _bitresult;

    void SetUp()
    {
        // Set binary data to some actual data from the iPM
        unsigned char bitresult[] = {0,0,254,1,255,3,25,2,24,2,88,1,0,0,0,0,
           39,2,249,1,249,1,253,1};
        memcpy(buffer,bitresult,25);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response string
 ********************************************************************
*/
TEST_F(BitresultTest, Parse)
{
    char *data = &buffer[0];
    uint16_t *sp = (uint16_t *)data;
    _bitresult.parse(sp);
    EXPECT_EQ(_bitresult.bitresult.bitStatus, 0);
    EXPECT_EQ(_bitresult.bitresult.hREFV, 510);
    EXPECT_EQ(_bitresult.bitresult.VREFV, 1023);
    EXPECT_EQ(_bitresult.bitresult.FIVEV, 537);
    EXPECT_EQ(_bitresult.bitresult.FIVEVA, 536);
    EXPECT_EQ(_bitresult.bitresult.RDV, 344);
    EXPECT_EQ(_bitresult.bitresult.ITVA, 551);
    EXPECT_EQ(_bitresult.bitresult.ITVB, 505);
    EXPECT_EQ(_bitresult.bitresult.ITVC, 505);
    EXPECT_EQ(_bitresult.bitresult.TEMP,509);
}
