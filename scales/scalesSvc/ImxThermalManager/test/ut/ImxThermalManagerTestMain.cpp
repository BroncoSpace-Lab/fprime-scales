// ======================================================================
// \title  ImxThermalManagerTestMain.cpp
// \author lucal
// \brief  cpp file for ImxThermalManager component test main function
// ======================================================================

#include "ImxThermalManagerTester.hpp"

TEST(Nominal, ImxThermalManagerTesting) {
  scalesSvc::ImxThermalManagerTester tester;
  tester.ImxThermalManagerTesting();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
