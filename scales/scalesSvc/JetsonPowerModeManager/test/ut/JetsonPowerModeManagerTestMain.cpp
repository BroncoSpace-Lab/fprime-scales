// ======================================================================
// \title  JetsonPowerModeManagerTestMain.cpp
// \brief  cpp file for JetsonPowerModeManager component test main function
// ======================================================================

#include "JetsonPowerModeManagerTester.hpp"

TEST(Nominal, PowerModeReceiveChangesModeWhenMismatched) {
    RecordProperty("requirement", "JPSM-002,JPSM-010");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveChangesModeWhenMismatched();
}

TEST(Nominal, PowerModeReceiveReportsFailureWhenNvpmodelFails) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveReportsFailureWhenNvpmodelFails();
}

TEST(Nominal, PowerModeReceiveReportsFailureOnPackedNonzeroExit) {
    RecordProperty("requirement", "JPSM-002,JPSM-009");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveReportsFailureOnPackedNonzeroExit();
}

TEST(Nominal, PowerModeReceiveTreatsSigtermAsLikelySuccess) {
    RecordProperty("requirement", "JPSM-009");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveTreatsSigtermAsLikelySuccess();
}

TEST(Nominal, PowerModeReceiveNoopWhenAlreadyInMode) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveNoopWhenAlreadyInMode();
}

TEST(Nominal, PowerModeReceiveIgnoredWhileRebootPending) {
    RecordProperty("requirement", "JPSM-011");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveIgnoredWhileRebootPending();
}

TEST(Nominal, PowerModeReceiveClearsRebootPendingOnGenuineFailure) {
    RecordProperty("requirement", "JPSM-011");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveClearsRebootPendingOnGenuineFailure();
}

TEST(Nominal, JetsonPowerStateReceiveOnReportsOn) {
    RecordProperty("requirement", "JPSM-005");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.jetsonPowerStateReceiveOnReportsOn();
}

TEST(Nominal, JetsonPowerStateReceiveOffShutsDownGracefully) {
    RecordProperty("requirement", "JPSM-004");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.jetsonPowerStateReceiveOffShutsDownGracefully();
}

TEST(Nominal, JetsonPowerStateReceiveOffReportsFailureWhenShutdownFails) {
    RecordProperty("requirement", "JPSM-004");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.jetsonPowerStateReceiveOffReportsFailureWhenShutdownFails();
}

TEST(Nominal, JetsonPowerStateReceiveOffReportsFailureOnNonzeroExitNotNegativeOne) {
    RecordProperty("requirement", "JPSM-004");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.jetsonPowerStateReceiveOffReportsFailureOnNonzeroExitNotNegativeOne();
}

TEST(Nominal, JetsonPowerStateReceiveOffTreatsSigtermAsSuccess) {
    RecordProperty("requirement", "JPSM-008");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.jetsonPowerStateReceiveOffTreatsSigtermAsSuccess();
}

TEST(Nominal, SchedInReportsOnceAfterBoot) {
    RecordProperty("requirement", "JPSM-001,JPSM-003");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.schedInReportsOnceAfterBoot();
}

TEST(Nominal, SchedInSkipsModeReportOnReaderError) {
    RecordProperty("requirement", "JPSM-001");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.schedInSkipsModeReportOnReaderError();
}

TEST(Nominal, SetPowerModeCmdChangesMode) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setPowerModeCmdChangesMode();
}

TEST(Nominal, SetPowerModeCmdNoopWhenAlreadyInMode) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setPowerModeCmdNoopWhenAlreadyInMode();
}

TEST(Nominal, SetPowerModeCmdReportsExecutionErrorOnNvpmodelFailure) {
    RecordProperty("requirement", "JPSM-012");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setPowerModeCmdReportsExecutionErrorOnNvpmodelFailure();
}

TEST(Nominal, SetPowerModeCmdTreatsSigtermAsSuccess) {
    RecordProperty("requirement", "JPSM-009,JPSM-012");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setPowerModeCmdTreatsSigtermAsSuccess();
}

TEST(Nominal, SetPowerModeCmdIgnoredWhileRebootPending) {
    RecordProperty("requirement", "JPSM-011");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setPowerModeCmdIgnoredWhileRebootPending();
}

TEST(Nominal, GetPowerModeCmdReturnsCurrentMode) {
    RecordProperty("requirement", "JPSM-001");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.getPowerModeCmdReturnsCurrentMode();
}

TEST(Nominal, GetPowerModeCmdValidationErrorOnReaderFailure) {
    RecordProperty("requirement", "JPSM-001,JPSM-006");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.getPowerModeCmdValidationErrorOnReaderFailure();
}

TEST(Nominal, SetJetsonPowerStateCmdOnReportsOn) {
    RecordProperty("requirement", "JPSM-005");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setJetsonPowerStateCmdOnReportsOn();
}

TEST(Nominal, SetJetsonPowerStateCmdOffShutsDownGracefully) {
    RecordProperty("requirement", "JPSM-004,JPSM-006");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setJetsonPowerStateCmdOffShutsDownGracefully();
}

TEST(Nominal, SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure) {
    RecordProperty("requirement", "JPSM-004,JPSM-006");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setJetsonPowerStateCmdOffReportsExecutionErrorOnFailure();
}

TEST(Nominal, SetJetsonPowerStateCmdOffTreatsSigtermAsSuccess) {
    RecordProperty("requirement", "JPSM-008");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.setJetsonPowerStateCmdOffTreatsSigtermAsSuccess();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
