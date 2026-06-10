// ======================================================================
// \title  WatchdogManagerTestMain.cpp
// \author lucal
// \brief  cpp file for WatchdogManager component test main function
// ======================================================================

#include "WatchdogManagerTester.hpp"

TEST(Nominal, toDo) {
  scalesSvc::WatchdogManagerTester tester;
  tester.toDo();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
