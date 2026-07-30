// ======================================================================
// \title  JetsonManagerTestMain.cpp
// \brief  cpp file for JetsonManager component test main function
// ======================================================================

#include "JetsonManagerTester.hpp"

TEST(JetsonManager, RequestPowerModeDeferredCompletion) {
    RecordProperty("requirement", "JM-004");
    scalesSvc::JetsonManagerTester tester;
    tester.requestPowerModeDeferredCompletion();
}

TEST(JetsonManager, RequestPowerModeTimeout) {
    RecordProperty("requirement", "JM-004");
    scalesSvc::JetsonManagerTester tester;
    tester.requestPowerModeTimeout();
}

TEST(JetsonManager, RequestPowerModeBusyWhilePending) {
    RecordProperty("requirement", "JM-012");
    scalesSvc::JetsonManagerTester tester;
    tester.requestPowerModeBusyWhilePending();
}

TEST(JetsonManager, RequestPowerModeRejectedWhenUnconfirmed) {
    RecordProperty("requirement", "JM-013");
    scalesSvc::JetsonManagerTester tester;
    tester.requestPowerModeRejectedWhenUnconfirmed();
}

TEST(JetsonManager, RequestPowerModeRejectedWhileAwaitingBootConfirmation) {
    RecordProperty("requirement", "JM-013");
    scalesSvc::JetsonManagerTester tester;
    tester.requestPowerModeRejectedWhileAwaitingBootConfirmation();
}

TEST(JetsonManager, RequestJetsonPowerStateOnDrivesGpioImmediately) {
    RecordProperty("requirement", "JM-001");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOnDrivesGpioImmediately();
}

TEST(JetsonManager, RequestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut) {
    RecordProperty("requirement", "JM-006");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut();
}

TEST(JetsonManager, RequestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown();
}

TEST(JetsonManager, RequestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout();
}

TEST(JetsonManager, RequestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff();
}

TEST(JetsonManager, RequestJetsonPowerStateOffAcceptedAfterRedundantOnCommand) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffAcceptedAfterRedundantOnCommand();
}

TEST(JetsonManager, RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation) {
    RecordProperty("requirement", "JM-011");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation();
}

TEST(JetsonManager, RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout) {
    RecordProperty("requirement", "JM-011");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout();
}

TEST(JetsonManager, RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower) {
    RecordProperty("requirement", "JM-005,JM-006");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower();
}

TEST(JetsonManager, RequestJetsonPowerStateOffConfirmedOffIsIdempotent) {
    RecordProperty("requirement", "JM-002,JM-006");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffConfirmedOffIsIdempotent();
}

TEST(JetsonManager, RequestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut) {
    RecordProperty("requirement", "JM-003");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut();
}

TEST(JetsonManager, RequestJetsonPowerStateRejectedByAuthorization) {
    RecordProperty("requirement", "JM-001");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateRejectedByAuthorization();
}

TEST(JetsonManager, RequestJetsonPowerStateBusyWhilePending) {
    RecordProperty("requirement", "JM-007");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateBusyWhilePending();
}

TEST(JetsonManager, FpJetsonPowerRequestInIgnoresOnAndActsOnOff) {
    RecordProperty("requirement", "JM-003");
    scalesSvc::JetsonManagerTester tester;
    tester.fpJetsonPowerRequestInIgnoresOnAndActsOnOff();
}

TEST(JetsonManager, FpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires) {
    RecordProperty("requirement", "JM-003,JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.fpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires();
}

TEST(JetsonManager, CurrentJetsonPwrStateIgnoredWithoutPendingCommand) {
    RecordProperty("requirement", "JM-008");
    scalesSvc::JetsonManagerTester tester;
    tester.currentJetsonPwrStateIgnoredWithoutPendingCommand();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
