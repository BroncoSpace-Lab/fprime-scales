// ======================================================================
// \title  PerifBoardManagerTestMain.cpp
// \author luquito
// \brief  cpp file for PerifBoardManager component test main function
// ======================================================================

#include "PerifBoardManagerTester.hpp"

TEST(Nominal, testPerifBoardManager) {
  scalesSvc::PerifBoardManagerTester tester;
  tester.testPerifBoardManager();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
