// ======================================================================
// \title  JetsonThermalManagerTestMain.cpp
// \author lucal
// \brief  cpp file for JetsonThermalManager component test main function
// ======================================================================

#include "JetsonThermalManagerTester.hpp"

TEST(Nominal, JetsonThermalManagerUnitTester) {
  RecordProperty("requirement", "JTM-001,JTM-002,JTM-004");
  scalesSvc::JetsonThermalManagerTester tester;
  tester.JetsonThermalManagerUnitTester();
}

TEST(Nominal, boundsUpdateGating) {
  RecordProperty("requirement", "JTM-003");
  scalesSvc::JetsonThermalManagerTester tester;
  tester.boundsUpdateGating();
}

TEST(Nominal, parameterUpdatedCoverage) {
  RecordProperty("requirement", "JTM-003");
  scalesSvc::JetsonThermalManagerTester tester;
  tester.parameterUpdatedCoverage();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
