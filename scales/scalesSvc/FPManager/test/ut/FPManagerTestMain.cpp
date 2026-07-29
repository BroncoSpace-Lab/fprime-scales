#include "FPManagerTester.hpp"

TEST(FPManager, InitializesSafeModeAndGatesJetsonOn) {
  RecordProperty("requirement", "FP-001,FP-002,FP-003");
  scalesSvc::FPManagerTester tester;
  tester.initializesSafeModeAndGatesJetsonOn();
}

TEST(FPManager, EmitsStateTransitionEventsOnlyOnChange) {
  RecordProperty("requirement", "FP-012");
  scalesSvc::FPManagerTester tester;
  tester.emitsStateTransitionEventsOnlyOnChange();
}

TEST(FPManager, EntersHpcModeAndAcceptsJetsonOn) {
  RecordProperty("requirement", "FP-003");
  scalesSvc::FPManagerTester tester;
  tester.entersHpcModeAndAcceptsJetsonOn();
}

TEST(FPManager, DisablesHpcModeAndGatesJetsonOn) {
  RecordProperty("requirement", "FP-002,FP-003,FP-009");
  scalesSvc::FPManagerTester tester;
  tester.disablesHpcModeAndGatesJetsonOn();
}

TEST(FPManager, DisableHpcModeWaitsForJetsonOffConfirmation) {
  RecordProperty("requirement", "FP-009");
  scalesSvc::FPManagerTester tester;
  tester.disableHpcModeWaitsForJetsonOffConfirmation();
}

TEST(FPManager, ImxFaultDuringDisableHpcWaitStillTriggersEmergencyShutdown) {
  RecordProperty("requirement", "FP-009,FP-007");
  scalesSvc::FPManagerTester tester;
  tester.imxFaultDuringDisableHpcWaitStillTriggersEmergencyShutdown();
}

TEST(FPManager, ImxFaultTriggersEmergencyShutdown) {
  RecordProperty("requirement", "FP-006,FP-007,FP-008");
  scalesSvc::FPManagerTester tester;
  tester.imxFaultTriggersEmergencyShutdown();
}

TEST(FPManager, PeripheralFaultPowersOffPeripheralOnly) {
  RecordProperty("requirement", "FP-006");
  scalesSvc::FPManagerTester tester;
  tester.peripheralFaultPowersOffPeripheralOnly();
}

TEST(FPManager, PeripheralFaultRecoversToSafeMode) {
  RecordProperty("requirement", "FP-006,FP-010");
  scalesSvc::FPManagerTester tester;
  tester.peripheralFaultRecoversToSafeMode();
}

TEST(FPManager, FaultModeJetsonFaultRequestsOffAndStaysFault) {
  RecordProperty("requirement", "FP-010,FP-011");
  scalesSvc::FPManagerTester tester;
  tester.faultModeJetsonFaultRequestsOffAndStaysFault();
}

TEST(FPManager, FaultModeImxFaultOverridesJetsonAndPeripheral) {
  RecordProperty("requirement", "FP-007,FP-010");
  scalesSvc::FPManagerTester tester;
  tester.faultModeImxFaultOverridesJetsonAndPeripheral();
}

TEST(FPManager, JetsonFaultReadingTriggersRecoveryInHpc) {
  RecordProperty("requirement", "FP-004,FP-005");
  scalesSvc::FPManagerTester tester;
  tester.jetsonFaultReadingTriggersRecoveryInHpc();
}

TEST(FPManager, JetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry) {
  RecordProperty("requirement", "FP-004,FP-005");
  scalesSvc::FPManagerTester tester;
  tester.jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry();
}

TEST(FPManager, AttributesJetsonFaultAndReturnsSafe) {
  RecordProperty("requirement", "FP-004,FP-005");
  scalesSvc::FPManagerTester tester;
  tester.attributesJetsonFaultAndReturnsSafe();
}

TEST(FPManager, ComponentFatalRestartsFswWithoutPlatformShutdown) {
  RecordProperty("requirement", "FP-017");
  scalesSvc::FPManagerTester tester;
  tester.componentFatalRestartsFswWithoutPlatformShutdown();
}

TEST(FPManager, EmergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals) {
  RecordProperty("requirement", "FP-008,FP-014");
  scalesSvc::FPManagerTester tester;
  tester.emergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals();
}

TEST(FPManager, RepeatedComponentFatalsDoNotReassertOrRestate) {
  RecordProperty("requirement", "FP-017,FP-014");
  scalesSvc::FPManagerTester tester;
  tester.repeatedComponentFatalsDoNotReassertOrRestate();
}

TEST(FPManager, ComponentFatalDoesNotDowngradeLatchedEmergencyState) {
  RecordProperty("requirement", "FP-020");
  scalesSvc::FPManagerTester tester;
  tester.componentFatalDoesNotDowngradeLatchedEmergencyState();
}

TEST(FPManager, ImxFaultRequiresConsecutiveReadingsBeforeShutdown) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.imxFaultRequiresConsecutiveReadingsBeforeShutdown();
}

TEST(FPManager, ImxFaultStreakResetsOnNonFaultReading) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.imxFaultStreakResetsOnNonFaultReading();
}

TEST(FPManager, ImxFaultStreakResetsOnUnavailableReading) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.imxFaultStreakResetsOnUnavailableReading();
}

TEST(FPManager, ImxStreaksAreTrackedPerSource) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.imxStreaksAreTrackedPerSource();
}

TEST(FPManager, PeripheralFaultRequiresConsecutiveReadings) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.peripheralFaultRequiresConsecutiveReadings();
}

TEST(FPManager, PeripheralStreaksAreTrackedPerSource) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.peripheralStreaksAreTrackedPerSource();
}

TEST(FPManager, FaultDebounceParameterUpdatedDispatchesCorrectly) {
  RecordProperty("requirement", "FP-019");
  scalesSvc::FPManagerTester tester;
  tester.faultDebounceParameterUpdatedDispatchesCorrectly();
}

TEST(FPManager, JetsonZoneStreaksAreIndependent) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.jetsonZoneStreaksAreIndependent();
}

TEST(FPManager, JetsonImmediateFastPathHonorsDebounce) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.jetsonImmediateFastPathHonorsDebounce();
}

TEST(FPManager, JetsonPowerOffClearsFaultStreaks) {
  RecordProperty("requirement", "FP-018");
  scalesSvc::FPManagerTester tester;
  tester.jetsonPowerOffClearsFaultStreaks();
}

TEST(FPManager, WarnTrackingIsNotDebounced) {
  RecordProperty("requirement", "FP-016,FP-018");
  scalesSvc::FPManagerTester tester;
  tester.warnTrackingIsNotDebounced();
}

TEST(FPManager, PeripheralRecoveryIsNotDebounced) {
  RecordProperty("requirement", "FP-010,FP-018");
  scalesSvc::FPManagerTester tester;
  tester.peripheralRecoveryIsNotDebounced();
}

TEST(FPManager, FaultDebounceCountParameterGatesAndPublishes) {
  RecordProperty("requirement", "FP-019");
  scalesSvc::FPManagerTester tester;
  tester.faultDebounceCountParameterGatesAndPublishes();
}

TEST(FPManager, RejectsRemoteJetsonCommandWhenJetsonOff) {
  RecordProperty("requirement", "FP-013");
  scalesSvc::FPManagerTester tester;
  tester.rejectsRemoteJetsonCommandWhenJetsonOff();
}

TEST(FPManager, ForwardsRemoteJetsonCommandWhenJetsonOn) {
  RecordProperty("requirement", "FP-013");
  scalesSvc::FPManagerTester tester;
  tester.forwardsRemoteJetsonCommandWhenJetsonOn();
}

TEST(FPManager, RejectsSequencerRemoteJetsonCommandWhenJetsonOff) {
  RecordProperty("requirement", "FP-013,FP-015");
  scalesSvc::FPManagerTester tester;
  tester.rejectsSequencerRemoteJetsonCommandWhenJetsonOff();
}

TEST(FPManager, ForwardsSequencerRemoteJetsonCommandWhenJetsonOn) {
  RecordProperty("requirement", "FP-013,FP-015");
  scalesSvc::FPManagerTester tester;
  tester.forwardsSequencerRemoteJetsonCommandWhenJetsonOn();
}

TEST(FPManager, ImxWarnStateEntersAndExitsWithoutShutdown) {
  RecordProperty("requirement", "FP-016");
  scalesSvc::FPManagerTester tester;
  tester.imxWarnStateEntersAndExitsWithoutShutdown();
}

TEST(FPManager, PeripheralWarnStateEntersAndExitsWithoutShutdown) {
  RecordProperty("requirement", "FP-016");
  scalesSvc::FPManagerTester tester;
  tester.peripheralWarnStateEntersAndExitsWithoutShutdown();
}

TEST(FPManager, JetsonWarnStateAggregatesAcrossSensors) {
  RecordProperty("requirement", "FP-016");
  scalesSvc::FPManagerTester tester;
  tester.jetsonWarnStateAggregatesAcrossSensors();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
