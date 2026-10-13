// ======================================================================
// \title  InaManagerTestMain.cpp
// \brief  cpp file for InaManager component test main function
// ======================================================================

#include "InaManagerTester.hpp"

TEST(Nominal, nominalAllSensorsSucceed) {
  RecordProperty("requirement", "INAM-001");
  scalesSvc::InaManagerTester tester;
  tester.nominalAllSensorsSucceed();
}

TEST(Nominal, conversionHelpersDirect) {
  RecordProperty("requirement", "INAM-001");
  scalesSvc::InaManagerTester tester;
  tester.conversionHelpersDirect();
}

TEST(Nominal, timestampAdvancesAcrossTicks) {
  RecordProperty("requirement", "INAM-001");
  scalesSvc::InaManagerTester tester;
  tester.timestampAdvancesAcrossTicks();
}

TEST(Nominal, singleRegisterFailureStopsSubsequentReadsForThatSensor) {
  RecordProperty("requirement", "INAM-001,INA-002");
  scalesSvc::InaManagerTester tester;
  tester.singleRegisterFailureStopsSubsequentReadsForThatSensor();
}

TEST(Nominal, allSensorsFailStillForwardsToDataProducer) {
  RecordProperty("requirement", "INA-002");
  scalesSvc::InaManagerTester tester;
  tester.allSensorsFailStillForwardsToDataProducer();
}

TEST(Nominal, powerRegisterFailureAfterCurrentAndVoltageSucceed) {
  RecordProperty("requirement", "INAM-001,INA-002");
  scalesSvc::InaManagerTester tester;
  tester.powerRegisterFailureAfterCurrentAndVoltageSucceed();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
