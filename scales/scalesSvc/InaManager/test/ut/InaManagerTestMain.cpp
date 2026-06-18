// ======================================================================
// \title  InaManagerTestMain.cpp
// \author jacob
// \brief  cpp file for InaManager component test main function
// ======================================================================

#include "InaManagerTester.hpp"

TEST(InaManager, NominalReadAllSensors) {
  scalesSvc::InaManagerTester tester;
  tester.testNominalReadAllSensors();
}

TEST(InaManager, I2cFailureSkipsFailedSensorAndLogsEvent) {
  scalesSvc::InaManagerTester tester;
  tester.testI2cFailureSkipsFailedSensorAndLogsEvent();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
