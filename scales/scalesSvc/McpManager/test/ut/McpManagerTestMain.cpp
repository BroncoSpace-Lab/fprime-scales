// ======================================================================
// \title  McpManagerTestMain.cpp
// \author bidat
// \brief  cpp file for McpManager component test main function
// ======================================================================

#include "McpManagerTester.hpp"

TEST(Nominal, mcpTest) {
  RecordProperty("requirement", "MCPM-001");
  scalesSvc::McpManagerTester tester;
  tester.mcpTest();
}

TEST(Nominal, boundsUpdateGating) {
  RecordProperty("requirement", "MCPM-002,MCPM-003");
  scalesSvc::McpManagerTester tester;
  tester.boundsUpdateGating();
}

TEST(Nominal, thermalStateEvaluation) {
  RecordProperty("requirement", "MCPM-002");
  scalesSvc::McpManagerTester tester;
  tester.thermalStateEvaluation();
}

TEST(Nominal, readFailureHandling) {
  RecordProperty("requirement", "MCPM-001,MCPM-004");
  scalesSvc::McpManagerTester tester;
  tester.readFailureHandling();
}

TEST(Nominal, parameterUpdatedSwitchCoverage) {
  RecordProperty("requirement", "MCPM-003");
  scalesSvc::McpManagerTester tester;
  tester.parameterUpdatedSwitchCoverage();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
