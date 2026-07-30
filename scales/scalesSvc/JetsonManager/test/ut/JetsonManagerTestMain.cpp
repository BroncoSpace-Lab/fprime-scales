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

TEST(JetsonManager, RequestJetsonPowerStateOffRejectedWhileBooting) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffRejectedWhileBooting();
}

TEST(JetsonManager, RequestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout) {
    RecordProperty("requirement", "JM-009");
    scalesSvc::JetsonManagerTester tester;
    tester.requestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout();
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

TEST(JetsonManager, CurrentJetsonPwrStateIgnoredWithoutPendingCommand) {
    RecordProperty("requirement", "JM-008");
    scalesSvc::JetsonManagerTester tester;
    tester.currentJetsonPwrStateIgnoredWithoutPendingCommand();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
