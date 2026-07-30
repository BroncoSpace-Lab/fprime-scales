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

void JetsonManagerTester ::requestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut() {
    // Fresh component: m_jetsonPowerStateKnown is false and
    // m_currentJetsonPowerState defaults to OFF. reqJetsonPwrState_out() is
    // wired straight through GenericHub into imx_hubComStub.dataIn with no
    // queue/gate in between -- calling it while the Jetson (and therefore the
    // hub TCP link) has never been confirmed alive trips ComStub's
    // never-connected FW_ASSERT and kills the whole i.MX flight software.
    // An earlier version of this gate preferred the graceful path whenever
    // the state merely wasn't confirmed OFF, which reintroduced exactly that
    // crash on a fresh/rebooted i.MX with the Jetson actually off. The gate
    // must now require confirmed ON before ever attempting the hub call --
    // unconfirmed falls back to the same safe, idempotent direct GPIO cut as
    // confirmed-off.
    m_authorizeResult = Fw::Success::SUCCESS;
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();

    ASSERT_from_reqJetsonPwrState_SIZE(0);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateOffRejectedWhileBooting() {
    // Regression test for the exact bug this fixed: commanding ON completes
    // synchronously (GPIO high, immediate OK) but does NOT mean the Jetson
    // has actually booted. A commanded OFF sent before the Jetson's first
    // real report used to race the boot -- taking the graceful hub-routed
    // path against a link that might not exist yet, and getting stuck BUSY
    // for the full CMD_TIMEOUT_TICKS window. It must now be rejected outright.
    m_authorizeResult = Fw::Success::SUCCESS;

    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);

    // OFF sent before the Jetson has reported in must be rejected outright,
    // not raced -- no hub call, no GPIO cut, no lingering pending-command state.
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 2, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_from_reqJetsonPwrState_SIZE(0);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_EVENTS_JETSON_OFF_REJECTED_BOOTING_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_EQ(this->cmdResponseHistory->at(1).response, Fw::CmdResponse::BUSY);

    // The real boot report arrives -- OFF is no longer rejected on this basis,
    // and (Jetson now confirmed ON) takes the graceful path as normal.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 3, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_EVENTS_JETSON_OFF_REJECTED_BOOTING_SIZE(0);
    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_reqJetsonPwrState(0, scalesSvc::JetsonPowerStateID::OFF);
}

void JetsonManagerTester ::requestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout() {
    m_authorizeResult = Fw::Success::SUCCESS;

    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    // The Jetson never reports in. CMD_TIMEOUT_TICKS = 120 in JetsonManager.hpp;
    // the boot-confirmation guard must not clear before that many ticks.
    for (U32 i = 0; i < 119; i++) {
        this->invoke_to_schedIn(0, 0);
    }
    ASSERT_EVENTS_JETSON_BOOT_CONFIRMATION_TIMEOUT_SIZE(0);

    this->invoke_to_schedIn(0, 0);
    ASSERT_EVENTS_JETSON_BOOT_CONFIRMATION_TIMEOUT_SIZE(1);

    // OFF is no longer rejected on the "still booting" basis -- it falls
    // through to the normal confirmedOn-gated logic, which is unconfirmed
    // (no real report ever arrived) so it takes the safe direct GPIO cut.
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 2, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_EVENTS_JETSON_OFF_REJECTED_BOOTING_SIZE(0);
    ASSERT_from_reqJetsonPwrState_SIZE(0);
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::LOW);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void JetsonManagerTester ::requestJetsonPowerStateOffAcceptedAfterRedundantOnCommand() {
    // Regression test: a redundant ON command sent to an already-confirmed,
    // already-running Jetson must not re-arm the boot-confirmation guard --
    // otherwise nothing is left to clear it (the Jetson won't send another
    // unsolicited report just because it was told to turn on again), and
    // OFF stays rejected BUSY until the full CMD_TIMEOUT_TICKS window.
    m_authorizeResult = Fw::Success::SUCCESS;

    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 1, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();

    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

    // Redundant ON: Jetson is already confirmed on.
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 2, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);

    // OFF must be accepted -- not rejected as "still booting" -- and takes
    // the graceful path since the Jetson is confirmed on.
    this->sendCmd_REQUEST_JETSON_POWER_STATE(0, 3, scalesSvc::JetsonPowerStateID::OFF);
    this->component.doDispatch();
    ASSERT_EVENTS_JETSON_OFF_REJECTED_BOOTING_SIZE(0);
    ASSERT_from_reqJetsonPwrState_SIZE(1);
    ASSERT_from_reqJetsonPwrState(0, scalesSvc::JetsonPowerStateID::OFF);
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

    // Confirm ON first so the first OFF request below takes the graceful
    // (pending) path instead of completing synchronously via direct GPIO cut.
    this->invoke_to_currentJetsonPwrState(0, scalesSvc::JetsonPowerStateID::ON);
    this->component.doDispatch();
    this->clearHistory();

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
