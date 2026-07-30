// ======================================================================
// \title  JetsonPowerModeManagerTester.cpp
// \brief  cpp file for JetsonPowerModeManager component test harness implementation class
// ======================================================================

#include "JetsonPowerModeManagerTester.hpp"
#include <csignal>
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
    // Logged before the nvpmodel shell call (JPSM-010) so it reaches GDS
    // even if this process is torn down moments later by the reboot.
    ASSERT_EVENTS_JETSON_POWER_MODE_REBOOT_STARTED_SIZE(1);
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

void JetsonPowerModeManagerTester ::powerModeReceiveReportsFailureOnPackedNonzeroExit() {
    // Parity with jetsonPowerStateReceiveOffReportsFailureOnNonzeroExitNotNegativeOne:
    // a real std::system() failure returns a packed wait-status, not a bare
    // small int -- exit code 1 packs to 1 << 8 = 256 on Linux. This path
    // never had a test exercising a realistic packed exit before.
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = 1 << 8;

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(1);
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::MIN);
}

void JetsonPowerModeManagerTester ::powerModeReceiveTreatsSigtermAsLikelySuccess() {
    // nvpmodel -m <N> reboots the Jetson to apply the new mode, tearing down
    // this process's own systemd service the same way shutdown -h now does
    // -- the same SIGTERM-killed-by-collateral-teardown signature must be
    // treated as (likely) success here too, not a genuine nvpmodel failure.
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = SIGTERM;

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(0);
    ASSERT_from_powerModeSend_SIZE(0);
}

void JetsonPowerModeManagerTester ::powerModeReceiveNoopWhenAlreadyInMode() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::BALANCED);

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 0);
    ASSERT_EVENTS_JETSON_POWER_MODE_REBOOT_STARTED_SIZE(0);
    ASSERT_from_powerModeSend_SIZE(1);
    ASSERT_from_powerModeSend(0, scalesSvc::PowerModeID::BALANCED);
    ASSERT_TLM_CurrentPowerMode_SIZE(1);
    ASSERT_TLM_CurrentPowerMode(0, scalesSvc::PowerModeID::BALANCED);
}

void JetsonPowerModeManagerTester ::powerModeReceiveIgnoredWhileRebootPending() {
    // Defense-in-depth: a second mode-change request arriving in the narrow
    // window between nvpmodel being invoked and the reboot actually
    // severing the hub link/killing this process must not double-invoke
    // nvpmodel. The i.MX side should already prevent this in normal
    // operation (JetsonManager's own m_hasPendingCmd BUSY guard/hub-link-trust
    // gate) -- this protects against a locally-issued SET_POWER_MODE or a
    // narrow race overlapping a hub-driven change.
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();
    ASSERT_EQ(g_shellCallCount, 1);

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::EXTRA);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 1);  // unchanged -- second request was dropped
    ASSERT_EVENTS_POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING_SIZE(1);
}

void JetsonPowerModeManagerTester ::powerModeReceiveClearsRebootPendingOnGenuineFailure() {
    // On a GENUINE nvpmodel failure, no reboot is coming and this same
    // process instance keeps running -- m_rebootPending must be cleared, or
    // it would latch forever and permanently block all future mode changes.
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = 1 << 8;

    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();
    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(1);

    // A subsequent mismatched-mode request must NOT be blocked -- proves the
    // guard was actually cleared, not left latched by the failure above.
    g_mockShellExitCode = 0;
    this->invoke_to_powerModeReceive(0, scalesSvc::PowerModeID::EXTRA);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 2);
    ASSERT_EVENTS_POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING_SIZE(0);
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

void JetsonPowerModeManagerTester ::jetsonPowerStateReceiveOffReportsFailureOnNonzeroExitNotNegativeOne() {
    // A real std::system() failure (e.g. "sudo -n" refusing because no
    // NOPASSWD sudoers rule exists for this exact command, instead of
    // prompting) returns a packed wait-status, not a bare -1. Exit code 1
    // packs to 1 << 8 = 256 on Linux. The old "ret == -1" check missed this
    // entirely and treated it as success even though shutdown never ran.
    g_mockShellExitCode = 1 << 8;

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

void JetsonPowerModeManagerTester ::jetsonPowerStateReceiveOffTreatsSigtermAsSuccess() {
    // A raw wait-status of just the signal number (no WIFEXITED bit, no core
    // dump bit) means "killed by that signal" -- here, killed by SIGTERM
    // immediately after issuing `sudo -n /sbin/shutdown -h now`. This is the
    // expected signature of the real shutdown succeeding and tearing down
    // this process's own service cgroup as collateral, not a real failure,
    // and must NOT be reported as one (a false failure here would send a
    // wrong "still ON" correction to the i.MX side).
    g_mockShellExitCode = SIGTERM;

    this->invoke_to_jetsonPowerStateReceive(0, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_EVENTS_JETSON_POWER_STATE_CHANGE_FAILED_SIZE(0);
    // Only the original OFF acknowledgment -- no ON correction follows.
    ASSERT_from_jetsonPowerStateSend_SIZE(1);
    ASSERT_from_jetsonPowerStateSend(0, scalesSvc::JetsonPowerStateID::OFF);
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

void JetsonPowerModeManagerTester ::setPowerModeCmdReportsExecutionErrorOnNvpmodelFailure() {
    // Previously this handler discarded the shell command's exit status
    // entirely and always responded OK -- now it must check it, same as the
    // hub-driven powerModeReceive path.
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = 1 << 8;

    this->sendCmd_SET_POWER_MODE(0, 9, scalesSvc::PowerModeID::MAX);
    this->component.doDispatch();

    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::EXECUTION_ERROR);
}

void JetsonPowerModeManagerTester ::setPowerModeCmdTreatsSigtermAsSuccess() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);
    g_mockShellExitCode = SIGTERM;

    this->sendCmd_SET_POWER_MODE(0, 10, scalesSvc::PowerModeID::MAX);
    this->component.doDispatch();

    ASSERT_EVENTS_POWER_MODE_CHANGE_FAILED_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonPowerModeManagerTester ::setPowerModeCmdIgnoredWhileRebootPending() {
    g_mockPowerMode = static_cast<int>(scalesSvc::PowerModeID::MIN);

    this->sendCmd_SET_POWER_MODE(0, 11, scalesSvc::PowerModeID::MAX);
    this->component.doDispatch();
    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);

    // A second SET_POWER_MODE while the first's reboot is still pending
    // (m_rebootPending, left set by the success above) is rejected BUSY,
    // not run through nvpmodel a second time.
    this->sendCmd_SET_POWER_MODE(0, 12, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();
    ASSERT_EQ(g_shellCallCount, 1);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_EQ(this->cmdResponseHistory->at(1).response, Fw::CmdResponse::BUSY);
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

void JetsonPowerModeManagerTester ::setJetsonPowerStateCmdOffTreatsSigtermAsSuccess() {
    // Same SIGTERM-as-success reasoning as jetsonPowerStateReceiveOffTreatsSigtermAsSuccess,
    // for the local SET_JETSON_POWER_STATE command path. A plain "ret == 0"
    // check would wrongly report EXECUTION_ERROR for a shutdown that
    // actually worked.
    g_mockShellExitCode = SIGTERM;

    this->sendCmd_SET_JETSON_POWER_STATE(0, 8, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

// connectPorts()/initComponents() are auto-generated into
// JetsonPowerModeManagerTesterHelpers.cpp by UT_AUTO_HELPERS (see
// CMakeLists.txt) -- defining them again here would conflict at link time.

}  // namespace scalesSvc
