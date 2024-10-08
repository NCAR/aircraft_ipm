/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions
#include "../src/record.cc"

class RecordTest : public ::testing::Test {
public:
        char buffer[1000];
private:
    ipmRecord _record;

    void SetUp()
    {
        // Set binary data to some actual data from the iPM
        unsigned char record[] = {0, 2, 99, 0, 0, 0, 139, 68, 105, 4, 0, 0, 0,
            0, 0, 0, 0, 0, 209, 0, 155, 4, 209, 0, 155, 4, 0, 0, 0, 0, 69, 2,
            88, 2, 0, 0, 94, 0, 0, 0, 85, 0, 0, 0, 21, 0, 26, 113, 26, 113, 1,
            1, 4, 6, 90, 6, 4, 6, 83, 6, 0, 0, 24, 0, 19, 27, 124, 8 };
        memcpy(buffer, record, 68);
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test parsing response string
 ********************************************************************
*/
TEST_F(RecordTest, Parse)
{
    //RECORD,0,2,99,74007691,0,0,20.90,117.90,20.90,117.90,0.00,0.00,58.10,
    //    60.00,0.0000,0.0940,0.0000,0.0850,0.0000,0.0210,2.60,11.30,2.60,
    //    11.30,0.10,0.10,154.00,162.60,154.00,161.90,0.00,2.40,6931
    char *data = &buffer[0];
    uint8_t *cp = (uint8_t *)data;
    uint16_t *sp = (uint16_t *)data;
    uint32_t *lp = (uint32_t *)data;
    _record.parse(cp, sp, lp);
    EXPECT_EQ(_record.record.EVTYPE,0);
    EXPECT_EQ(_record.record.OPSTATE,2);
    EXPECT_EQ(_record.record.POWERCNT,99);
    EXPECT_EQ(_record.record.TIME,74007691);
    EXPECT_EQ(_record.record.TFLAG,0);
    EXPECT_EQ(_record.record.CFLAG,0);
    EXPECT_EQ(_record.record.VRMSMINA,209);
    EXPECT_EQ(_record.record.VRMSMAXA,1179);
    EXPECT_EQ(_record.record.VRMSMINB,209);
    EXPECT_EQ(_record.record.VRMSMAXB,1179);
    EXPECT_EQ(_record.record.VRMSMINC,0);
    EXPECT_EQ(_record.record.VRMSMAXC,0);
    EXPECT_EQ(_record.record.FREQMIN,581);
    EXPECT_EQ(_record.record.FREQMAX,600);
    EXPECT_EQ(_record.record.VDCMINA,0);
    EXPECT_EQ(_record.record.VDCMAXA,94);
    EXPECT_EQ(_record.record.VDCMINB,0);
    EXPECT_EQ(_record.record.VDCMAXB,85);
    EXPECT_EQ(_record.record.VDCMINC,0);
    EXPECT_EQ(_record.record.VDCMAXC,21);
    EXPECT_EQ(_record.record.THDMINA,26);
    EXPECT_EQ(_record.record.THDMAXA,113);
    EXPECT_EQ(_record.record.THDMINB,26);
    EXPECT_EQ(_record.record.THDMAXB,113);
    EXPECT_EQ(_record.record.THDMINC,1);
    EXPECT_EQ(_record.record.THDMAXC,1);
    EXPECT_EQ(_record.record.VPKMINA,1540);
    EXPECT_EQ(_record.record.VPKMAXA,1626);
    EXPECT_EQ(_record.record.VPKMINB,1540);
    EXPECT_EQ(_record.record.VPKMAXB,1619);
    EXPECT_EQ(_record.record.VPKMINC,0);
    EXPECT_EQ(_record.record.VPKMAXC,24);
    EXPECT_EQ(_record.record.CRC,142351123);
}

/********************************************************************
 ** Test creating UDP packet
 ********************************************************************
*/
TEST_F(RecordTest, CreateUDP)
{
    std::string str;

    _record.createUDP(buffer, 1);  // scaling turned on
    str = buffer;
    EXPECT_EQ(str,"RECORD,0,2,99,74007691,0,0,20.90,117.90,20.90,117.90,0.00,0.00,58.10,60.00,0.0000,0.0940,0.0000,0.0850,0.0000,0.0210,2.60,11.30,2.60,11.30,0.10,0.10,154.00,162.60,154.00,161.90,0.00,2.40,142351123\r\n");

    _record.createUDP(buffer, 0);  // scaling turned off
    str = buffer;
    EXPECT_EQ(str,"RECORD,00,02,00000063,0469448b,00000000,00000000,00d1,049b,00d1,049b,0000,0000,0245,0258,0000,005e,0000,0055,0000,0015,1a,71,1a,71,01,01,0604,065a,0604,0653,0000,0018,087c1b13\r\n");
}

/********************************************************************
 ** Test retrieving time since iPM powerup
 ********************************************************************
*/
TEST_F(RecordTest, GetTimeSincePowerup)
{
    EXPECT_FLOAT_EQ(_record.getTimeSincePowerup(), 1233);
}
