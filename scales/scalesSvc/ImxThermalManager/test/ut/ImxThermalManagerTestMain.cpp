// ======================================================================
// \title  ImxThermalManagerTestMain.cpp
// \author lucal
// \brief  cpp file for ImxThermalManager component test main function
// ======================================================================

#include "ImxThermalManagerTester.hpp"

TEST(Nominal, ImxThermalManagerTesting) {
  RecordProperty("requirement", "ITM-001,ITM-002,ITM-004");
  scalesSvc::ImxThermalManagerTester tester;
  tester.ImxThermalManagerTesting();
}

TEST(Nominal, boundsUpdateGating) {
  RecordProperty("requirement", "ITM-003");
  scalesSvc::ImxThermalManagerTester tester;
  tester.boundsUpdateGating();
}

TEST(Nominal, highSideFaultGap) {
  RecordProperty("requirement", "ITM-002");
  scalesSvc::ImxThermalManagerTester tester;
  tester.highSideFaultGap();
}

TEST(Nominal, malformedTempFile) {
  RecordProperty("requirement", "ITM-001");
  scalesSvc::ImxThermalManagerTester tester;
  tester.malformedTempFile();
}

TEST(Nominal, parameterUpdatedCoverage) {
  RecordProperty("requirement", "ITM-003");
  scalesSvc::ImxThermalManagerTester tester;
  tester.parameterUpdatedCoverage();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
