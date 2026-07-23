#include "FPManagerTester.hpp"

TEST(FPManager, InitializesSafeModeAndGatesJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.initializesSafeModeAndGatesJetsonOn();
}

TEST(FPManager, EmitsStateTransitionEventsOnlyOnChange) {
  scalesSvc::FPManagerTester tester;
  tester.emitsStateTransitionEventsOnlyOnChange();
}

TEST(FPManager, EntersHpcModeAndAcceptsJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.entersHpcModeAndAcceptsJetsonOn();
}

TEST(FPManager, DisablesHpcModeAndGatesJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.disablesHpcModeAndGatesJetsonOn();
}

TEST(FPManager, ImxFaultTriggersEmergencyShutdown) {
  scalesSvc::FPManagerTester tester;
  tester.imxFaultTriggersEmergencyShutdown();
}

TEST(FPManager, PeripheralFaultPowersOffPeripheralOnly) {
  scalesSvc::FPManagerTester tester;
  tester.peripheralFaultPowersOffPeripheralOnly();
}

TEST(FPManager, PeripheralFaultRecoversToSafeMode) {
  scalesSvc::FPManagerTester tester;
  tester.peripheralFaultRecoversToSafeMode();
}

TEST(FPManager, FaultModeJetsonFaultRequestsOffAndStaysFault) {
  scalesSvc::FPManagerTester tester;
  tester.faultModeJetsonFaultRequestsOffAndStaysFault();
}

TEST(FPManager, FaultModeImxFaultOverridesJetsonAndPeripheral) {
  scalesSvc::FPManagerTester tester;
  tester.faultModeImxFaultOverridesJetsonAndPeripheral();
}

TEST(FPManager, JetsonFaultReadingTriggersRecoveryInHpc) {
  scalesSvc::FPManagerTester tester;
  tester.jetsonFaultReadingTriggersRecoveryInHpc();
}

TEST(FPManager, AttributesJetsonFaultAndReturnsSafe) {
  scalesSvc::FPManagerTester tester;
  tester.attributesJetsonFaultAndReturnsSafe();
}

TEST(FPManager, FatalShutdownForwardsAndLatches) {
  scalesSvc::FPManagerTester tester;
  tester.fatalShutdownForwardsAndLatches();
}

TEST(FPManager, RejectsRemoteJetsonCommandWhenJetsonOff) {
  scalesSvc::FPManagerTester tester;
  tester.rejectsRemoteJetsonCommandWhenJetsonOff();
}

TEST(FPManager, ForwardsRemoteJetsonCommandWhenJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.forwardsRemoteJetsonCommandWhenJetsonOn();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
