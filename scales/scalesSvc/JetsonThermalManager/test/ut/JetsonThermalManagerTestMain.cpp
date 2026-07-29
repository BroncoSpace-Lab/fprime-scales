// ======================================================================
// \title  JetsonThermalManagerTestMain.cpp
// \author lucal
// \brief  cpp file for JetsonThermalManager component test main function
// ======================================================================

#include "JetsonThermalManagerTester.hpp"

TEST(Nominal, JetsonThermalManagerUnitTester) {
  scalesSvc::JetsonThermalManagerTester tester;
  tester.JetsonThermalManagerUnitTester();
}

TEST(Nominal, boundsUpdateGating) {
  scalesSvc::JetsonThermalManagerTester tester;
  tester.boundsUpdateGating();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
