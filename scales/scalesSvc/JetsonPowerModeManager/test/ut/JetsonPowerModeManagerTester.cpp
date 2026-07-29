// ======================================================================
// \title  JetsonPowerModeManagerTester.cpp
// \brief  cpp file for JetsonPowerModeManager component test harness implementation class
// ======================================================================

#include "JetsonPowerModeManagerTester.hpp"
#include <string>

namespace {
// configurePowerModeReader()/configureShellRunner() take plain function
// pointers (no captures allowed), so test-controlled behavior is threaded
// through file-scope state instead of lambda captures. This state is reset
// at the top of the Tester constructor so every TEST() starts clean.
int g_mockPowerMode = 4;  // 4 == "error" sentinel, matching get_nvp_mode()'s convention
int mockPowerModeReader() {
    return g_mockPowerMode;
}

std::string g_lastShellCommand;
int g_shellCallCount = 0;
int g_mockShellExitCode = 0;
int mockShellRunner(const char* cmd) {
    g_lastShellCommand = cmd;
    g_shellCallCount++;
    return g_mockShellExitCode;
}
}  // namespace

namespace scalesSvc {

JetsonPowerModeManagerTester ::JetsonPowerModeManagerTester()
    : JetsonPowerModeManagerGTestBase("JetsonPowerModeManagerTester", JetsonPowerModeManagerTester::MAX_HISTORY_SIZE),
      component("JetsonPowerModeManager") {
    g_mockPowerMode = 4;
    g_lastShellCommand.clear();
    g_shellCallCount = 0;
    g_mockShellExitCode = 0;

    this->initComponents();
    this->connectPorts();

    // Never let a test accidentally fall through to the real get_nvp_mode()/
    // std::system() -- both are configured to the mocks before any handler
    // can run, no exceptions.
    this->component.configurePowerModeReader(&mockPowerModeReader);
    this->component.configureShellRunner(&mockShellRunner);
}

JetsonPowerModeManagerTester ::~JetsonPowerModeManagerTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void JetsonPowerModeManagerTester ::powerModeReceiveChangesModeWhenMismatched() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EVENTS_POWER_MODE_REQUEST_RECEIVED_SIZE(1);
    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_NE(g_lastShellCommand.find("nvpmodel -m 2"), std::string::npos);
    // The mismatched-mode path does not report a mode itself when the
    // nvpmodel call succeeds -- the reboot confirmation happens later via
    // schedIn_handler.
    ASSERT_from_powerModeSend_SIZE(0);
    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(0);
}

void JetsonPowerModeManagerTester ::powerModeReceiveReportsFailureWhenNvpmodelFails() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = 1;

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(1);
    // The current (unchanged) mode is sent back so the i.MX's deferred
    // command times out instead of hanging forever.
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::MIN);
}

void JetsonPowerModeManagerTester ::powerModeReceiveNoopWhenAlreadyInMode() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::BALANCED);

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 0);
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::BALANCED);
    ASSERT_TLM_CurrentPowerMode_SIZE(1);
    ASSERT_TLM_CurrentPowerMode(0, scalesSvc::PowerModeID::BALANCED);
}

void JetsonPowerModeManagerTester ::jetsonPowerStateReceiveOnReportsOn() {
    this->invoke_to_jetsonPowerStateReceive(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    ASSERT_EVENTS_JETSON_POWER_STATE_REQUEST_RECEIVED_SIZE(1);
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_TLM_CurrentJetsonPowerState_SIZE(1);
    ASSERT_TLM_CurrentJetsonPowerState(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_EQ(g_shellCallCount, 0);
}

void JetsonPowerModeManagerTester ::jetsonPowerStateReceiveOffShutsDownGracefully() {
    g_mockShellExitCode = 0;

    this->invoke_to_jetsonPowerStateReceive(0, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    // OFF is acknowledged to the i.MX before the shell command runs.
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::OFF);
    ASSERT_TLM_CurrentJetsonPowerState_SIZE(1);
    ASSERT_TLM_CurrentJetsonPowerState(0, scalesSvc::JetsonPowerStateID::OFF);
    ASSERT_EVENTS_JETSON_SHUTDOWN_STARTED_SIZE(1);

    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_NE(g_lastShellCommand.find("shutdown -h now"), std::string::npos);
    ASSERT_EVENTS_JETSON_POWER_STATE_CHANGE_FAILED_SIZE(0);
    // A successful shutdown does not re-report ON afterward.
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
}

void JetsonPowerModeManagerTester ::jetsonPowerStateReceiveOffReportsFailureWhenShutdownFails() {
    g_mockShellExitCode = -1;

    this->invoke_to_jetsonPowerStateReceive(0, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_EVENTS_JETSON_POWER_STATE_CHANGE_FAILED_SIZE(1);
    // Shutdown failed -- the Jetson is still alive, so ON is reported again
    // after the initial OFF acknowledgment.
    ASSERT_from_jetsonPowerStateSend_SIZE(2);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::OFF);
    ASSERT_from_jetsonPowerStateSend(1, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_TLM_CurrentJetsonPowerState_SIZE(2);
    ASSERT_TLM_CurrentJetsonPowerState(1, scalesSvc::JetsonPowerStateID::ON);
}

void JetsonPowerModeManagerTester ::schedInReportsOnceAfterBoot() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::EXTRA);

    this->invoke_to_schedIn(0, 0);

    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::EXTRA);

    // A second tick must not repeat either report.
    this->invoke_to_schedIn(0, 0);
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_powerModeSend_SIZE(1);
}

void JetsonPowerModeManagerTester ::schedInSkipsModeReportOnReaderError() {
    g_mockPowerMode = 4;  // error sentinel

    this->invoke_to_schedIn(0, 0);

    // Power state is still reported once regardless...
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    // ...but the mode report is withheld until the reader stops erroring.
    ASSERT_from_powerModeSend_SIZE(0);

    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    this->invoke_to_schedIn(0, 0);
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::MIN);
    // Power state is not reported a second time.
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
}

void JetsonPowerModeManagerTester ::setPowerModeCmdChangesMode() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);

    this->sendCmd_SET_POWER_MODE(0, 1, scalesSvc::PowerModeID::MAX);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_NE(g_lastShellCommand.find("nvpmodel -m 0"), std::string::npos);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::setPowerModeCmdNoopWhenAlreadyInMode() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MAX);

    this->sendCmd_SET_POWER_MODE(0, 2, scalesSvc::PowerModeID::MAX);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::getPowerModeCmdReturnsCurrentMode() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::BALANCED);

    this->sendCmd_GET_POWER_MODE(0, 3);
    this->component.doDispatch();

    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::BALANCED);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::getPowerModeCmdValidationErrorOnReaderFailure() {
    g_mockPowerMode = 4;  // error sentinel

    this->sendCmd_GET_POWER_MODE(0, 4);
    this->component.doDispatch();

    ASSERT_from_powerModeSend_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::VALIDATION_ERROR);
}

void JetsonPowerModeManagerTester ::setJetsonPowerStateCmdOnReportsOn() {
    this->sendCmd_SET_JETSON_POWER_STATE(0, 5, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::setJetsonPowerStateCmdOffShutsDownGracefully() {
    g_mockShellExitCode = 0;

    this->sendCmd_SET_JETSON_POWER_STATE(0, 6, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 1);
    // Regression check: the command must complete with exactly ONE response,
    // not two -- an earlier version of this handler called cmdResponse_out()
    // unconditionally right after the OFF report AND again after checking
    // the shell result, which would assert in the real command dispatcher.
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::setJetsonPowerStateCmdOffReportsExecutionErrorOnFailure() {
    g_mockShellExitCode = 1;

    this->sendCmd_SET_JETSON_POWER_STATE(0, 7, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::EXECUTION_ERROR);
}

// connectPorts()/initComponents() are auto-generated into
// JetsonPowerModeManagerTesterHelpers.cpp by UT_AUTO_HELPERS (see
// CMakeLists.txt) -- defining them again here would conflict at link time.

}  // namespace scalesSvc
