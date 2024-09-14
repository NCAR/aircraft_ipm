/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions
#include "../src/measure.cc"

class MeasureTest : public ::testing::Test {
public:
        char buffer[1000];
private:
    ipmMeasure _measure;

    void SetUp()
    {
        // Set binary data to some actual data from the iPM
        unsigned char measure[] = {88, 2, 0, 0, 5, 2, 139, 4, 139, 4, 0, 0, 4,
            6, 252, 5, 0, 0, 28, 0, 28, 0, 9, 0, 201, 13, 200, 6, 7, 7, 27, 27,
            1, 1};
        memcpy(buffer, measure, 34);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response string
 ********************************************************************
*/
TEST_F(MeasureTest, Parse)
{
    //MEASURE,60.00,51.70,116.30,116.30,0.00,154.00,153.20,0.00,0.0280,0.0280,
    //    0.0090,352.90,173.60,179.90,2.70,2.70,0.10,1
    char *data = &buffer[0];
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    _measure.parse(cp, sp);
    EXPECT_EQ(_measure.measure.FREQ,600);
    EXPECT_EQ(_measure.measure.TEMP,517);
    EXPECT_EQ(_measure.measure.VRMSA,1163);
    EXPECT_EQ(_measure.measure.VRMSB,1163);
    EXPECT_EQ(_measure.measure.VRMSC,0);
    EXPECT_EQ(_measure.measure.VPKA,1540);
    EXPECT_EQ(_measure.measure.VPKB,1532);
    EXPECT_EQ(_measure.measure.VPKC,0);
    EXPECT_EQ(_measure.measure.VDCA,28);
    EXPECT_EQ(_measure.measure.VDCB,28);
    EXPECT_EQ(_measure.measure.VDCC,9);
    EXPECT_EQ(_measure.measure.PHA,3529);
    EXPECT_EQ(_measure.measure.PHB,1736);
    EXPECT_EQ(_measure.measure.PHC,1799);
    EXPECT_EQ(_measure.measure.THDA,27);
    EXPECT_EQ(_measure.measure.THDB,27);
    EXPECT_EQ(_measure.measure.THDC,1);
    EXPECT_EQ(_measure.measure.POWEROK,1);
}

/********************************************************************
 ** Test creating UDP packet
 ********************************************************************
*/
TEST_F(MeasureTest, CreateUDP)
{
    std::string str;

    _measure.createUDP(buffer, 1);  // scaling turned on
    str = buffer;
    EXPECT_EQ(str,"MEASURE,60.00,51.70,116.30,116.30,0.00,154.00,153.20,0.00,0.0280,0.0280,0.0090,352.90,173.60,179.90,2.70,2.70,0.10,1\r\n");

    _measure.createUDP(buffer, 0);  // scaling turned off
    str = buffer;
    EXPECT_EQ(str,"MEASURE,0258,0205,048b,048b,0000,0604,05fc,0000,001c,001c,0009,0dc9,06c8,0707,1b,1b,01,01\r\n");
}
