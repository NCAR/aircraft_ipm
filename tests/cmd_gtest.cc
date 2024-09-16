/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/
#include <gtest/gtest.h>
// sytem header includes must be before private public def
#define private public  // so can test private functions

namespace cmdtests
{
    #include "../src/argparse.cc"
    #include "../src/cmd.cc"
}

using namespace cmdtests;

class CmdTest : public ::testing::Test {
private:
    ipmArgparse _args;
    ipmCmd commands;

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

/********************************************************************
 ** Test verifying a command
 ********************************************************************
*/

TEST_F(CmdTest, ipmVerifyInvalid)
{
    testing::internal::CaptureStdout();

    // Invalid iPM query
    _args.setCmd("REC");
    commands.verify(_args.Cmd());
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "Command REC is invalid. Please enter a valid command\n");
}

TEST_F(CmdTest, ipmVerifyValid)
{
    testing::internal::CaptureStdout();

    // Invalid iPM query
    _args.setCmd("RECORD?");
    commands.verify(_args.Cmd());
    EXPECT_EQ(testing::internal::GetCapturedStdout(),
        "");
}
