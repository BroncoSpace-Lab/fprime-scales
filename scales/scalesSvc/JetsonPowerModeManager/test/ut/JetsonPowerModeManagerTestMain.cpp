// ======================================================================
// \title  JetsonPowerModeManagerTestMain.cpp
// \brief  cpp file for JetsonPowerModeManager component test main function
// ======================================================================

#include "JetsonPowerModeManagerTester.hpp"

TEST(Nominal, PowerModeReceiveChangesModeWhenMismatched) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveChangesModeWhenMismatched();
}

TEST(Nominal, PowerModeReceiveReportsFailureWhenNvpmodelFails) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveReportsFailureWhenNvpmodelFails();
}

TEST(Nominal, PowerModeReceiveNoopWhenAlreadyInMode) {
    RecordProperty("requirement", "JPSM-002");
    scalesSvc::JetsonPowerModeManagerTester tester;
    tester.powerModeReceiveNoopWhenAlreadyInMode();
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
