// ======================================================================
// \title  PerifBoardManagerTestMain.cpp
// \author luquito
// \brief  cpp file for PerifBoardManager component test main function
// ======================================================================

#include "PerifBoardManagerTester.hpp"

TEST(Nominal, testPerifBoardManager) {
  RecordProperty("requirement", "PBM-001,PBM-002,PBM-003");
  scalesSvc::PerifBoardManagerTester tester;
  tester.testPerifBoardManager();
}

TEST(Nominal, emergencyShutdownLatch) {
  RecordProperty("requirement", "PBM-005");
  scalesSvc::PerifBoardManagerTester tester;
  tester.emergencyShutdownLatch();
}

TEST(Nominal, configurableOffInterval) {
  RecordProperty("requirement", "PBM-003");
  scalesSvc::PerifBoardManagerTester tester;
  tester.configurableOffInterval();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
