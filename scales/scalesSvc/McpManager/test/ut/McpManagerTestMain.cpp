// ======================================================================
// \title  McpManagerTestMain.cpp
// \author bidat
// \brief  cpp file for McpManager component test main function
// ======================================================================

#include "McpManagerTester.hpp"

TEST(Nominal, mcpTest) {
  scalesSvc::McpManagerTester tester;
  tester.mcpTest();
}

TEST(Nominal, boundsUpdateGating) {
  scalesSvc::McpManagerTester tester;
  tester.boundsUpdateGating();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
