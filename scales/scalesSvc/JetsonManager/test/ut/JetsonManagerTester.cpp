// ======================================================================
// \title  JetsonManagerTester.cpp
// \brief  cpp file for JetsonManager component test harness implementation class
// ======================================================================

#include "JetsonManagerTester.hpp"

namespace scalesSvc {

JetsonManagerTester ::JetsonManagerTester()
    : JetsonManagerGTestBase("JetsonManagerTester", JetsonManagerTester::MAX_HISTORY_SIZE),
      component("JetsonManager"),
      m_authorizeResult(Fw::Success::SUCCESS) {
    this->initComponents();
    this->connectPorts();
}

JetsonManagerTester ::~JetsonManagerTester() {
    this->component.deinit();
}

Fw::Success JetsonManagerTester ::from_fpJetsonPowerAuthorize_handler(FwIndexType portNum,
                                                                       const scalesSvc::JetsonPowerStateID& stateReq) {
    // Call the generated base implementation first so this call is still
    // recorded in the usual from-port history, then override only the
    // return value so tests can control FPManager's authorization result.
    static_cast<void>(JetsonManagerTesterBase::from_fpJetsonPowerAuthorize_handler(portNum, stateReq));
    return m_authorizeResult;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void JetsonManagerTester ::requestPowerModeDeferredCompletion() {
    this->sendCmd_REQUEST_POWER_MODE(0, 1, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();

    // The command must NOT complete immediately -- it stays open until the
    // Jetson reports back the matching mode after rebooting.
    ASSERT_CMD_RESPONSE_SIZE(0);
    ASSERT_from_reqPwrMode_SIZE(1);
    ASSERT_from_reqPwrMode(0, scalesSvc::PowerModeID::BALANCED);
    ASSERT_EVENTS_POWER_MODE_REQUESTED_SIZE(1);

    // A mismatched report must not complete the command.
    this->invoke_to_currentPwrMode(0, scalesSvc::PowerModeID::MIN);
    this->component.doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(0);

    // The matching report completes it with OK.
    this->invoke_to_currentPwrMode(0, scalesSvc::PowerModeID::BALANCED);
    this->component.doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestPowerModeTimeout() {
    this->sendCmd_REQUEST_POWER_MODE(0, 2, scalesSvc::PowerModeID::EXTRA);
    this->component.doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(0);

    // CMD_TIMEOUT_TICKS = 120 in JetsonManager.hpp; the command must not
    // complete before that many ticks have elapsed.
    for (U32 i = 0; i < 119; i++) {
        this->invoke_to_schedIn(0, 0);
    }
    ASSERT_CMD_RESPONSE_SIZE(0);

    this->invoke_to_schedIn(0, 0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::EXECUTION_ERROR);
}

void JetsonManagerTester ::requestJetsonPowerStateOnDrivesGpioImmediately() {
    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    ASSERT_from_fpJetsonPowerAuthorize_SIZE(1);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH);
    ASSERT_TLM_JetsonPowerState_SIZE(1);
    ASSERT_TLM_JetsonPowerState(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_from_fpJetsonPowerStateOut_SIZE(1);
    ASSERT_from_fpJetsonPowerStateOut(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateOffUnconfirmedPrefersGraceful() {
    // Fresh component: m_jetsonPowerStateKnown is false and
    // m_currentJetsonPowerState defaults to OFF. Before the fix for this
    // exact bug, that unconfirmed default was trusted as fact and OFF was
    // treated as an idempotent no-op GPIO cut -- skipping the graceful
    // Jetson-side shutdown request even if the Jetson was actually alive.
    // It must now prefer the graceful path whenever the state isn't
    // confirmed off.
    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_reqJetsonPwrState(0, scalesSvc::JetsonPowerStateID::OFF);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(0);
}

void JetsonManagerTester ::requestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower() {
    this->component.loadParameters();

    // Confirm ON via a real status report first.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 5, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(0);

    // Jetson acknowledges OFF -- GPIO must not be cut yet, the delay window
    // starts now.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(0);

    // JETSON_POWER_OFF_DELAY_TICKS defaults to 15 in JetsonManager.fpp.
    for (U32 i = 0; i < 14; i++) {
        this->invoke_to_schedIn(0, 0);
    }
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(0);

    this->invoke_to_schedIn(0, 0);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateOffConfirmedOffIsIdempotent() {
    this->component.loadParameters();

    // Confirm OFF via a real status report first.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    this->clearHistory();

    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 2, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    // Confirmed-off must complete synchronously with a direct GPIO cut, not
    // attempt the graceful Jetson-side request.
    ASSERT_from_reqJetsonPwrState_SIZE(0);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut() {
    this->component.loadParameters();

    // Confirm ON so the graceful path is taken, then never acknowledge it.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 3, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_gpioSet_SIZE(0);

    // CMD_TIMEOUT_TICKS = 120; no acknowledgment ever arrives.
    for (U32 i = 0; i < 119; i++) {
        this->invoke_to_schedIn(0, 0);
    }
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_EVENTS_JETSON_POWER_STATE_TIMEOUT_SIZE(0);

    this->invoke_to_schedIn(0, 0);
    ASSERT_EVENTS_JETSON_POWER_STATE_TIMEOUT_SIZE(1);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateRejectedByAuthorization() {
    m_authorizeResult = Fw::Success::FAILURE;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 4, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    ASSERT_from_fpJetsonPowerAuthorize_SIZE(1);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::VALIDATION_ERROR);
}

void JetsonManagerTester ::requestJetsonPowerStateBusyWhilePending() {
    this->component.loadParameters();

    // First OFF request while unconfirmed takes the graceful (pending) path.
    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 10, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(0);

    // A second request while the first is still outstanding is rejected BUSY.
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 11, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::BUSY);
}

void JetsonManagerTester ::fpJetsonPowerRequestInIgnoresOnAndActsOnOff() {
    this->component.loadParameters();

    // ON requests on this internal path are ignored entirely -- only
    // FPManager-driven OFF (recovery/emergency/HPC-disable) is acted on.
    this->invoke_to_fpJetsonPowerRequestIn(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_from_reqJetsonPwrState_SIZE(0);

    // Confirm ON, then request OFF: the graceful path is preferred when the
    // Jetson is known ON, same as the GDS-facing command.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    this->invoke_to_fpJetsonPowerRequestIn(0, scalesSvc::JetsonPowerStateID::OFF);
    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_gpioSet_SIZE(0);
}

void JetsonManagerTester ::currentJetsonPwrStateIgnoredWithoutPendingCommand() {
    // A status report with no outstanding power command still updates the
    // cached state and telemetry, but takes no further action.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    ASSERT_TLM_JetsonPowerState_SIZE(1);
    ASSERT_TLM_JetsonPowerState(0, scalesSvc::JetsonPowerStateID::ON);
    ASSERT_from_gpioSet_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(0);
}

// connectPorts()/initComponents() are auto-generated into
// JetsonManagerTesterHelpers.cpp by UT_AUTO_HELPERS (see CMakeLists.txt) --
// defining them again here would conflict at link time.

}  // namespace scalesSvc
