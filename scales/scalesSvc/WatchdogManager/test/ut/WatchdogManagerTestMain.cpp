// ======================================================================
// \title  WatchdogManagerTestMain.cpp
// \author lucal
// \brief  cpp file for WatchdogManager component test main function
// ======================================================================

#include "WatchdogManagerTester.hpp"

TEST(Nominal, WatchdogTester) {
  scalesSvc::WatchdogManagerTester tester;
  tester.WatchdogTester();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
