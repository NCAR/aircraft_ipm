#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE naiipm test suite
#include <boost/test/included/unit_test.hpp>

#include <string.h>
#include "../naiipm.cc"

static naiipm ipm;

BOOST_AUTO_TEST_SUITE(ipmTests)

// Test storing and retrieving command line args
BOOST_AUTO_TEST_CASE(testSetGetNumAddr)
{
    ipm.setNumAddr("1");
    BOOST_CHECK( atoi(ipm.numAddr())==1 );
}

BOOST_AUTO_TEST_CASE(testSetGetIpmPort)
{
    ipm.setPort("/dev/ttyS0");
    BOOST_CHECK ( ((std::string)ipm.Port()).compare("/dev/ttyS0")==0 );

}

BOOST_AUTO_TEST_SUITE_END()
