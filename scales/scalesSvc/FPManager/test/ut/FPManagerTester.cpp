#include "FPManagerTester.hpp"

#include "Fw/Cmd/CmdPacket.hpp"

namespace scalesSvc {

FPManagerTester::FPManagerTester()
    : FPManagerGTestBase("FPManagerTester", FPManagerTester::MAX_HISTORY_SIZE),
      component("FPManager") {
  this->initComponents();
  this->connectPorts();
}

FPManagerTester::~FPManagerTester() {
  this->component.deinit();
}

void FPManagerTester::drainStateMachine() {
  for (FwSizeType i = 0; i < 20; ++i) {
    const FPManagerComponentBase::MsgDispatchStatus status =
        this->dispatchCurrentMessages(this->component);
    if (status == FPManagerComponentBase::MSG_DISPATCH_EMPTY) {
      break;
    }
  }
}

ThermalReading FPManagerTester::reading(U8 sensorId, ThermalStates state,
                                        F32 temperature, const char* location,
                                        U32 timestamp) {
  ThermalReading result;
  result.set_sensorId(sensorId);
  result.set_tempState(state);
  result.set_temperature(temperature);
  result.set_location(Fw::String(location));
  result.set_timestamp(timestamp);
  return result;
}

Fw::ComBuffer FPManagerTester::commandBuffer(FwOpcodeType opcode) {
  Fw::ComBuffer data;
  Fw::CmdArgBuffer args;
  EXPECT_EQ(data.serializeFrom(
                static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
            Fw::FW_SERIALIZE_OK);
  EXPECT_EQ(data.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
  EXPECT_EQ(data.serializeFrom(args), Fw::FW_SERIALIZE_OK);
  return data;
}

void FPManagerTester::initializeSafeMode() {
  // Real topology init calls this once, before the rate group starts
  // ticking; paramGet_FAULT_DEBOUNCE_COUNT() only reads the local cache this
  // populates, it never invokes the port itself. Without this call the
  // cache is uninitialized, so loadDebounceParameterOnFirstTick() would read
  // garbage on the first run() tick below.
  this->component.loadParameters();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_TLM_FP_STATE_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(0).arg, FPManagerState::SAFE);
}

void FPManagerTester::setFaultDebounce(U32 n) {
  this->component.applyFaultDebounceCount(n);
}

void FPManagerTester::enterHpcMode() {
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->sendCmd_ENABLE_HPC_MODE(0, 0);
  this->drainStateMachine();
  ASSERT_CMD_RESPONSE_SIZE(1);
  ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void FPManagerTester::initializesSafeModeAndGatesJetsonOn() {
  this->initializeSafeMode();
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
  ASSERT_EVENTS_JETSON_POWER_REQUEST_REJECTED_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
}

void FPManagerTester::emitsStateTransitionEventsOnlyOnChange() {
  this->initializeSafeMode();
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(1);
  ASSERT_EVENTS_FP_STATE_CHANGED(0, FPManagerState::INIT, FPManagerState::SAFE);

  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(1);

  this->sendCmd_ENABLE_HPC_MODE(0, 0);
  this->drainStateMachine();
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);
  ASSERT_EVENTS_FP_STATE_CHANGED(1, FPManagerState::SAFE, FPManagerState::HPC);

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(3);
  ASSERT_EVENTS_FP_STATE_CHANGED(2, FPManagerState::HPC, FPManagerState::SAFE);
}

void FPManagerTester::entersHpcModeAndAcceptsJetsonOn() {
  this->initializeSafeMode();
  this->enterHpcMode();
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::SUCCESS);
  ASSERT_EVENTS_JETSON_POWER_REQUEST_REJECTED_SIZE(0);
}

void FPManagerTester::disablesHpcModeAndGatesJetsonOn() {
  this->initializeSafeMode();
  this->enterHpcMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_CMD_RESPONSE_SIZE(2);
  ASSERT_EQ(this->cmdResponseHistory->at(1).response, Fw::CmdResponse::OK);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_TLM_FP_STATE_SIZE(4);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(3).arg, FPManagerState::SAFE);

  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
  ASSERT_EVENTS_JETSON_POWER_REQUEST_REJECTED_SIZE(1);
}

void FPManagerTester::disableHpcModeWaitsForJetsonOffConfirmation() {
  this->initializeSafeMode();
  this->enterHpcMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(0, ThermalStates::IDLE, 40.0F, "CPU", 1));
  this->drainStateMachine();

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  // The cached Jetson reading is NOT yet invalidated -- FPManager has not
  // received a real OFF confirmation from JetsonManager yet.
  ASSERT_GT(this->tlmHistory_JETSON_VALID_READING_COUNT->size(), 0U);
  ASSERT_EQ(this->tlmHistory_JETSON_VALID_READING_COUNT
                ->at(this->tlmHistory_JETSON_VALID_READING_COUNT->size() - 1)
                .arg,
            1U);

  // Still waiting: a tick with no confirmation changes nothing, and
  // ENABLE_HPC_MODE stays rejected because Safe Mode isn't health-confirmed.
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->sendCmd_ENABLE_HPC_MODE(0, 2);
  this->drainStateMachine();
  ASSERT_EQ(this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1).response,
            Fw::CmdResponse::VALIDATION_ERROR);

  // The real confirmation arrives from JetsonManager (graceful ack or its
  // own bounded GPIO-cut fallback -- either way reported the same way here).
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::OFF);
  this->drainStateMachine();
  this->invoke_to_run(0, 0);  // tick #1: bare disablingHpc -> safeMode edge
  this->drainStateMachine();
  this->invoke_to_run(0, 0);  // tick #2: safeModeHealthCheck actually runs
  this->drainStateMachine();

  ASSERT_EQ(this->tlmHistory_JETSON_VALID_READING_COUNT
                ->at(this->tlmHistory_JETSON_VALID_READING_COUNT->size() - 1)
                .arg,
            0U);

  this->sendCmd_ENABLE_HPC_MODE(0, 3);
  this->drainStateMachine();
  ASSERT_EQ(this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1).response,
            Fw::CmdResponse::OK);
}

void FPManagerTester::disableHpcModeWithJetsonNeverToggledEmitsAlreadyOffEvent() {
  // HPC enabled, but the Jetson was never authorized/confirmed on -- OFF
  // must never be rejected/blocked, and DISABLE_HPC_MODE reports it as
  // "already off" (nothing to shut down), not a boot-in-progress wait.
  this->initializeSafeMode();
  this->enterHpcMode();

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_ALREADY_OFF_SIZE(1);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_OFF_REQUESTED_SIZE(0);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_BOOTING_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_EQ(this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1).response,
            Fw::CmdResponse::OK);
}

void FPManagerTester::disableHpcModeWhileJetsonBootingDefersAndEmitsBootingEvent() {
  // The Jetson was just authorized ON (a real ON command would have gone to
  // JetsonManager) but has not reported back yet -- DISABLE_HPC_MODE must
  // NOT reject or block on this. It proceeds exactly as always (requesting
  // OFF from JetsonManager unconditionally, which JetsonManager itself will
  // defer until boot is confirmed), but reports it as "Jetson booting" so
  // the operator knows why it may take longer than usual.
  this->initializeSafeMode();
  this->enterHpcMode();
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::SUCCESS);

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_BOOTING_SIZE(1);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_OFF_REQUESTED_SIZE(0);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_ALREADY_OFF_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_EQ(this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1).response,
            Fw::CmdResponse::OK);
}

void FPManagerTester::disableHpcModeAfterBootConfirmedEmitsOffRequestedEvent() {
  this->initializeSafeMode();
  this->enterHpcMode();
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::SUCCESS);
  // The real report arrives, confirming the Jetson booted.
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->drainStateMachine();

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_OFF_REQUESTED_SIZE(1);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_BOOTING_SIZE(0);
  ASSERT_EVENTS_HPC_MODE_DISABLE_JETSON_ALREADY_OFF_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_EQ(this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1).response,
            Fw::CmdResponse::OK);
}

void FPManagerTester::imxFaultDuringDisableHpcWaitStillTriggersEmergencyShutdown() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->enterHpcMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->drainStateMachine();

  this->sendCmd_DISABLE_HPC_MODE(0, 1);
  this->drainStateMachine();

  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::SAFE);

  // A real i.MX fault arrives while still waiting in disablingHpc -- it must
  // escalate immediately, not be delayed until Safe Mode is fully reached.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 99));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  // Fires twice total: once (unconditionally) from beginDisableHpcMode, and
  // once again from SHUTDOWN's own unconditional jetsonPowerRequestOut_out call.
  ASSERT_from_jetsonPowerRequestOut_SIZE(2);
  ASSERT_from_jetsonPowerRequestOut(1, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::EMERGENCY);
}

void FPManagerTester::imxFaultTriggersEmergencyShutdown() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 11));
  this->drainStateMachine();

  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "IMX", 1U, 101.0F,
                               ThermalStates::FAULT, "imx-cpu", 11U);
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_fatalOut(
      0, static_cast<FwEventIdType>(this->component.getIdBase() +
                                    FPManagerComponentBase::EVENTID_EMERGENCY_SHUTDOWN));
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::EMERGENCY);
}

void FPManagerTester::peripheralFaultPowersOffPeripheralOnly() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->drainStateMachine();

  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "PERIPHERAL", 2U, 88.0F,
                               ThermalStates::FAULT, "peripheral", 12U);
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
  ASSERT_from_fatalOut_SIZE(0);
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::FAULT);
}

void FPManagerTester::peripheralFaultRecoversToSafeMode() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::FAULT);

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::IDLE, 40.0F, "peripheral", 13));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_TLM_FP_STATE_SIZE(3);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(2).arg, FPManagerState::SAFE);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_from_fatalOut_SIZE(0);
}

void FPManagerTester::faultModeJetsonFaultRequestsOffAndStaysFault() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::IDLE, 40.0F, "peripheral", 13));
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 42));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(2);
  ASSERT_EVENTS_FAULT_DETECTED(1, "JETSON", 4U, 99.0F,
                               ThermalStates::FAULT, "gpu-cluster", 42U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_from_fatalOut_SIZE(0);
  ASSERT_TLM_FP_STATE_SIZE(3);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(2).arg, FPManagerState::FAULT);
}

void FPManagerTester::faultModeImxFaultOverridesJetsonAndPeripheral() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 14));
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 42));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(2);
  ASSERT_EVENTS_FAULT_DETECTED(1, "IMX", 1U, 101.0F,
                               ThermalStates::FAULT, "imx-cpu", 14U);
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(2);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_TLM_FP_STATE_SIZE(3);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(2).arg, FPManagerState::EMERGENCY);
}

void FPManagerTester::jetsonFaultReadingTriggersRecoveryInHpc() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->enterHpcMode();

  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 42));
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "JETSON", 4U, 99.0F,
                               ThermalStates::FAULT, "gpu-cluster", 42U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_TLM_FP_STATE_SIZE(4);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(3).arg, FPManagerState::SAFE);
}

void FPManagerTester::jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->enterHpcMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->drainStateMachine();

  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(0, ThermalStates::FAULT, 44.9F, "CPU", 455));
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "JETSON", 0U, 44.9F,
                               ThermalStates::FAULT, "CPU", 455U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_GT(this->tlmHistory_FP_STATE->size(), 0U);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::SAFE);
  ASSERT_GT(this->tlmHistory_JETSON_VALID_READING_COUNT->size(), 0U);
  ASSERT_EQ(this->tlmHistory_JETSON_VALID_READING_COUNT
                ->at(this->tlmHistory_JETSON_VALID_READING_COUNT->size() - 1)
                .arg,
            0U);

  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->sendCmd_ENABLE_HPC_MODE(0, 1);
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_CMD_RESPONSE_SIZE(2);
  ASSERT_EQ(this->cmdResponseHistory->at(1).response, Fw::CmdResponse::OK);
  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_GT(this->tlmHistory_FP_STATE->size(), 0U);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::HPC);
}

void FPManagerTester::attributesJetsonFaultAndReturnsSafe() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->enterHpcMode();

  for (U8 sensorId = 0; sensorId < 9; ++sensorId) {
    this->invoke_to_jetsonThermalReadingIn(
        0, this->reading(sensorId, ThermalStates::IDLE, 40.0F, "die", 10));
  }
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 42));
  this->dispatchCurrentMessages(this->component);

  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "JETSON", 4U, 99.0F,
                               ThermalStates::FAULT, "gpu-cluster", 42U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
}

void FPManagerTester::componentFatalRestartsFswWithoutPlatformShutdown() {
  this->initializeSafeMode();
  this->invoke_to_fatalIn(0, 0x1234);
  this->drainStateMachine();

  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED_SIZE(1);
  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED(0, static_cast<FwEventIdType>(0x1234));
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
  ASSERT_from_peripheralPowerOff_SIZE(0);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_fatalOut(0, static_cast<FwEventIdType>(0x1234));
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::EMERGENCY_REBOOT);

  // EMERGENCY_REBOOT is not HPC Mode, so a Jetson power-on request is still
  // rejected -- it must not be treated as newly safe just because FP_STATE
  // changed away from SAFE.
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
}

void FPManagerTester::emergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 11));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  // initializeSafeMode() already logged INIT->SAFE; this fault adds SAFE->EMERGENCY.
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);

  // A second, still-faulting i.MX reading arrives while already latched into
  // EMERGENCY. emergencyShutdown has no `on tick` handler, so the health
  // check never re-runs and the already-latched protected outputs must not
  // be re-asserted.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 102.0F, "imx-cpu", 13));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_peripheralPowerOff_SIZE(1);
  // Mode stays EMERGENCY, so no additional transition event is logged.
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);

  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
}

void FPManagerTester::repeatedComponentFatalsDoNotReassertOrRestate() {
  this->initializeSafeMode();
  this->invoke_to_fatalIn(0, 0x1234);
  this->drainStateMachine();

  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED_SIZE(1);
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::EMERGENCY_REBOOT);
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);

  // A second, unrelated component FATAL arrives (e.g. a different
  // FW_ASSERT). It must still be logged and forwarded every time, but must
  // not re-log a FP_STATE_CHANGED transition or touch any protected output,
  // since the state machine is already latched in emergencyReboot.
  this->invoke_to_fatalIn(0, 0x5678);
  this->drainStateMachine();

  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED_SIZE(2);
  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED(1, static_cast<FwEventIdType>(0x5678));
  ASSERT_from_fatalOut_SIZE(2);
  ASSERT_from_fatalOut(1, static_cast<FwEventIdType>(0x5678));
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
  ASSERT_from_peripheralPowerOff_SIZE(0);
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);
}

void FPManagerTester::componentFatalDoesNotDowngradeLatchedEmergencyState() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 11));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::EMERGENCY);

  // A component FATAL arriving after a real thermal shutdown has already
  // latched must never downgrade FP_STATE back to EMERGENCY_REBOOT -- that
  // would misrepresent an active platform poweroff as a mere FSW restart.
  this->invoke_to_fatalIn(0, 0x1234);
  this->drainStateMachine();

  ASSERT_EVENTS_COMPONENT_FAILURE_DETECTED_SIZE(1);
  ASSERT_from_fatalOut_SIZE(2);
  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::EMERGENCY);
  ASSERT_EVENTS_FP_STATE_CHANGED_SIZE(2);
}

void FPManagerTester::rejectsRemoteJetsonCommandWhenJetsonOff() {
  this->initializeSafeMode();

  const FwOpcodeType opcode = 0x10001311;
  Fw::ComBuffer cmd = this->commandBuffer(opcode);
  this->invoke_to_remoteJetsonCmdIn(0, cmd, 77);

  ASSERT_from_remoteJetsonCmdOut_SIZE(0);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut(0, opcode, 77, Fw::CmdResponse::BUSY);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED_SIZE(1);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED(
      0, opcode, "Jetson is not powered on");
}

void FPManagerTester::forwardsRemoteJetsonCommandWhenJetsonOn() {
  this->initializeSafeMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->drainStateMachine();

  const FwOpcodeType opcode = 0x10001311;
  Fw::ComBuffer cmd = this->commandBuffer(opcode);
  this->invoke_to_remoteJetsonCmdIn(0, cmd, 78);

  ASSERT_from_remoteJetsonCmdOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(0);
  const Fw::CmdResponse response = Fw::CmdResponse::OK;
  this->invoke_to_remoteJetsonCmdResponseIn(0, opcode, 78, response);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut(0, opcode, 78, response);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED_SIZE(0);
}

void FPManagerTester::rejectsSequencerRemoteJetsonCommandWhenJetsonOff() {
  this->initializeSafeMode();

  // Port index 1 is the CmdSequencer-originated path (imx_seqCmdSplitter),
  // as opposed to index 0's GDS-direct path (imx_cmdSplitter). Both must be
  // gated identically so a sequence targeting the Jetson cannot reach the
  // hub transport while the Jetson is powered off.
  const FwOpcodeType opcode = 0x10001311;
  Fw::ComBuffer cmd = this->commandBuffer(opcode);
  this->invoke_to_remoteJetsonCmdIn(1, cmd, 79);

  ASSERT_from_remoteJetsonCmdOut_SIZE(0);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut(0, opcode, 79, Fw::CmdResponse::BUSY);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED_SIZE(1);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED(
      0, opcode, "Jetson is not powered on");
}

void FPManagerTester::forwardsSequencerRemoteJetsonCommandWhenJetsonOn() {
  this->initializeSafeMode();
  this->invoke_to_jetsonPowerStateIn(0, JetsonPowerStateID::ON);
  this->drainStateMachine();

  const FwOpcodeType opcode = 0x10001311;
  Fw::ComBuffer cmd = this->commandBuffer(opcode);
  this->invoke_to_remoteJetsonCmdIn(1, cmd, 80);

  ASSERT_from_remoteJetsonCmdOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(0);
  const Fw::CmdResponse response = Fw::CmdResponse::OK;
  this->invoke_to_remoteJetsonCmdResponseIn(1, opcode, 80, response);
  ASSERT_from_remoteJetsonCmdResponseOut_SIZE(1);
  ASSERT_from_remoteJetsonCmdResponseOut(0, opcode, 80, response);
  ASSERT_EVENTS_REMOTE_JETSON_COMMAND_REJECTED_SIZE(0);
}

void FPManagerTester::imxWarnStateEntersAndExitsWithoutShutdown() {
  this->initializeSafeMode();

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::WARN, 75.0F, "imx-cpu", 20));
  this->drainStateMachine();

  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_ENTERED(0, "IMX", 1U, 75.0F, "imx-cpu", 20U);
  ASSERT_EVENTS_WARN_STATE_EXITED_SIZE(0);
  ASSERT_EVENTS_FAULT_DETECTED_SIZE(0);
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
  ASSERT_from_peripheralPowerOff_SIZE(0);
  ASSERT_from_fatalOut_SIZE(0);

  // A second WARN reading must not re-fire the entered event.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::WARN, 76.0F, "imx-cpu", 21));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);

  // Returning to IDLE exits WARN.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::IDLE, 40.0F, "imx-cpu", 22));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_EXITED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_EXITED(0, "IMX", 1U, 40.0F, "imx-cpu", 22U);
}

void FPManagerTester::peripheralWarnStateEntersAndExitsWithoutShutdown() {
  this->initializeSafeMode();

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::WARN, 70.0F, "peripheral", 30));
  this->drainStateMachine();

  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_ENTERED(0, "PERIPHERAL", 2U, 70.0F, "peripheral", 30U);
  ASSERT_EVENTS_FAULT_DETECTED_SIZE(0);
  ASSERT_from_peripheralPowerOff_SIZE(0);

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::IDLE, 40.0F, "peripheral", 31));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_EXITED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_EXITED(0, "PERIPHERAL", 2U, 40.0F, "peripheral", 31U);
}

void FPManagerTester::jetsonWarnStateAggregatesAcrossSensors() {
  this->initializeSafeMode();

  // Sensor 3 enters WARN: aggregate WARN entered.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(3, ThermalStates::WARN, 80.0F, "gpu", 40));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_ENTERED(0, "JETSON", 3U, 80.0F, "gpu", 40U);

  // Sensor 5 also enters WARN while sensor 3 is still WARN: no duplicate entered event.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(5, ThermalStates::WARN, 82.0F, "soc1", 41));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);

  // Sensor 3 clears, but sensor 5 is still WARN: aggregate stays WARN, no exit yet.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(3, ThermalStates::IDLE, 40.0F, "gpu", 42));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_EXITED_SIZE(0);

  // Sensor 5 clears too: aggregate exits WARN.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(5, ThermalStates::IDLE, 40.0F, "soc1", 43));
  this->drainStateMachine();
  ASSERT_EVENTS_WARN_STATE_EXITED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_EXITED(0, "JETSON", 5U, 40.0F, "soc1", 43U);
}

void FPManagerTester::imxFaultRequiresConsecutiveReadingsBeforeShutdown() {
  this->initializeSafeMode();
  // Default debounce count is 3.

  for (U32 i = 0; i < 2; i++) {
    this->invoke_to_imxThermalReadingIn(
        0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 10 + i));
    this->invoke_to_run(0, 0);
    this->drainStateMachine();
  }
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);
  // FP_STATE republishes every tick regardless of value, so only the last
  // entry (not the history size) indicates whether a transition happened.
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::SAFE);

  // Third consecutive FAULT reading reaches the threshold.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::EMERGENCY);
}

void FPManagerTester::imxFaultStreakResetsOnNonFaultReading() {
  this->initializeSafeMode();

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 10));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 11));
  // A healthy reading resets the streak back to zero.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::IDLE, 40.0F, "imx-cpu", 12));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 13));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 14));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  // Only 2 consecutive FAULT readings since the reset -- below threshold.
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 15));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
}

void FPManagerTester::imxFaultStreakResetsOnUnavailableReading() {
  this->initializeSafeMode();

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 10));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 11));
  // An unavailable/NOT_USED reading resets the streak just like a healthy
  // one -- consistent with how invalid readings are treated everywhere else.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::NOT_USED, 0.0F, "imx-cpu", 12));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 13));
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 14));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 15));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
}

void FPManagerTester::imxStreaksAreTrackedPerSource() {
  this->initializeSafeMode();

  // Alternate a faulting CPU-die reading (source SRC_IMX_LOCAL) with a
  // healthy MCP i.MX-sensor reading (source SRC_IMX_MCP). If the streak were
  // shared per domain instead of per source, each healthy MCP reading would
  // reset the shared counter and this sequence could never trip the
  // shutdown, silently masking a sustained real fault.
  for (U32 i = 0; i < 2; i++) {
    this->invoke_to_imxThermalReadingIn(
        0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 10 + i));
    this->invoke_to_mcpThermalReadingIn(
        0, this->reading(1, ThermalStates::IDLE, 45.0F, "OBC", 10 + i));
  }
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(0);

  // A third consecutive CPU-die FAULT reading (no MCP reading in between this
  // time) reaches the SRC_IMX_LOCAL streak's threshold and trips the
  // shutdown, since m_imxLastSource now points back at SRC_IMX_LOCAL.
  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::FAULT, 101.0F, "imx-cpu", 20));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
}

void FPManagerTester::peripheralFaultRequiresConsecutiveReadings() {
  this->initializeSafeMode();

  for (U32 i = 0; i < 2; i++) {
    this->invoke_to_peripheralThermalReadingIn(
        0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 10 + i));
    this->invoke_to_run(0, 0);
    this->drainStateMachine();
  }
  ASSERT_from_peripheralPowerOff_SIZE(0);
  // FP_STATE republishes every tick regardless of value, so only the last
  // entry (not the history size) indicates whether a transition happened.
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::SAFE);

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_from_peripheralPowerOff_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(this->tlmHistory_FP_STATE->size() - 1).arg,
            FPManagerState::FAULT);
}

void FPManagerTester::peripheralStreaksAreTrackedPerSource() {
  this->initializeSafeMode();

  // Same two-producer aliasing hazard as the i.MX domain: alternate a
  // faulting direct peripheral reading (SRC_PERIF_LOCAL) with a healthy MCP
  // peripheral-sensor reading (SRC_PERIF_MCP).
  for (U32 i = 0; i < 2; i++) {
    this->invoke_to_peripheralThermalReadingIn(
        0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 10 + i));
    this->invoke_to_mcpThermalReadingIn(
        0, this->reading(2, ThermalStates::IDLE, 40.0F, "peripheral-mcp", 10 + i));
  }
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_from_peripheralPowerOff_SIZE(0);

  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 20));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_from_peripheralPowerOff_SIZE(1);

  // An MCP sensorId this handler does not recognize (0 is the i.MX CPU-die
  // reading, routed elsewhere) is a no-op.
  this->invoke_to_mcpThermalReadingIn(
      0, this->reading(0, ThermalStates::FAULT, 200.0F, "unrelated", 99));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_from_peripheralPowerOff_SIZE(1);
}

void FPManagerTester::faultDebounceParameterUpdatedDispatchesCorrectly() {
  this->initializeSafeMode();
  this->clearHistory();

  // FPManagerComponentBase::PARAMID_FAULT_DEBOUNCE_COUNT dispatches into
  // applyFaultDebounceCount() via paramGet_FAULT_DEBOUNCE_COUNT();
  // faultDebounceCountParameterGatesAndPublishes calls applyFaultDebounceCount()
  // directly and never exercises this switch, so it's otherwise never
  // covered. Driving a real PRM_SET through cmdIn's dispatch queue would
  // just retest the same validation logic already covered there.
  this->component.parameterUpdated(FPManagerComponentBase::PARAMID_FAULT_DEBOUNCE_COUNT);
  ASSERT_GT(this->tlmHistory_FAULT_DEBOUNCE_COUNT->size(), 0U);
  ASSERT_EQ(this->tlmHistory_FAULT_DEBOUNCE_COUNT
                ->at(this->tlmHistory_FAULT_DEBOUNCE_COUNT->size() - 1)
                .arg,
            3U);

  // Unrecognized parameter ID: default case, no-op.
  const FwSizeType sizeBeforeUnknownId = this->tlmHistory_FAULT_DEBOUNCE_COUNT->size();
  this->component.parameterUpdated(0xDEAD);
  ASSERT_TLM_FAULT_DEBOUNCE_COUNT_SIZE(sizeBeforeUnknownId);
}

void FPManagerTester::jetsonZoneStreaksAreIndependent() {
  this->initializeSafeMode();
  this->enterHpcMode();

  // Two consecutive FAULT readings on zone 3 and zone 5 each -- below the
  // default threshold of 3, and each zone's counter must stay independent of
  // the other's.
  for (U32 i = 0; i < 2; i++) {
    this->invoke_to_jetsonThermalReadingIn(
        0, this->reading(3, ThermalStates::FAULT, 90.0F, "gpu", 10 + i));
    this->drainStateMachine();
    this->invoke_to_jetsonThermalReadingIn(
        0, this->reading(5, ThermalStates::FAULT, 92.0F, "soc1", 10 + i));
    this->drainStateMachine();
  }
  ASSERT_from_jetsonPowerRequestOut_SIZE(0);

  // A third consecutive FAULT reading on zone 3 only trips the fast path for
  // zone 3; zone 5's independent counter is still only at 2. The immediate
  // fast path sends the jetson_fault signal on the spot, but the actual
  // report/power-off happens on the next tick's confirmJetsonFaultAndPowerOff
  // action (same structure as jetsonFaultReadingTriggersRecoveryInHpc).
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(3, ThermalStates::FAULT, 90.0F, "gpu", 20));
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "JETSON", 3U, 90.0F, ThermalStates::FAULT, "gpu", 20U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
}

void FPManagerTester::jetsonImmediateFastPathHonorsDebounce() {
  this->initializeSafeMode();
  this->enterHpcMode();

  // Two consecutive FAULT readings on the same zone, with no invoke_to_run
  // in between -- the immediate fast path in jetsonThermalReadingIn_handler
  // must still honor the debounce threshold and not fire early.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 1));
  this->drainStateMachine();
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 2));
  this->drainStateMachine();

  ASSERT_from_jetsonPowerRequestOut_SIZE(0);
  ASSERT_EVENTS_FAULT_DETECTED_SIZE(0);

  // Third consecutive FAULT reading on the same zone reaches the threshold
  // and sends the jetson_fault signal immediately, with no intervening run
  // tick required to reach the debounce count itself. The actual report and
  // power-off request come from the next tick's confirmJetsonFaultAndPowerOff
  // action, same as jetsonFaultReadingTriggersRecoveryInHpc.
  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 3));
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
}

void FPManagerTester::jetsonPowerOffClearsFaultStreaks() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->enterHpcMode();

  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 1));
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);

  // Return to HPC Mode; invalidateJetsonReadings() must have cleared the
  // Jetson fault streaks along with the cached readings, so a single new
  // FAULT reading right after re-entry must not immediately act again once
  // the debounce threshold is raised back up.
  this->setFaultDebounce(3);
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->sendCmd_ENABLE_HPC_MODE(0, 1);
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  this->invoke_to_jetsonThermalReadingIn(
      0, this->reading(4, ThermalStates::FAULT, 99.0F, "gpu-cluster", 2));
  this->drainStateMachine();

  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
}

void FPManagerTester::warnTrackingIsNotDebounced() {
  this->initializeSafeMode();
  // A high debounce count must not delay WARN reporting -- only FAULT
  // escalation is gated by the debounce counter.
  this->setFaultDebounce(100);

  this->invoke_to_imxThermalReadingIn(
      0, this->reading(1, ThermalStates::WARN, 75.0F, "imx-cpu", 20));
  this->drainStateMachine();

  ASSERT_EVENTS_WARN_STATE_ENTERED_SIZE(1);
  ASSERT_EVENTS_WARN_STATE_ENTERED(0, "IMX", 1U, 75.0F, "imx-cpu", 20U);
}

void FPManagerTester::peripheralRecoveryIsNotDebounced() {
  this->initializeSafeMode();
  this->setFaultDebounce(1);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::FAULT, 88.0F, "peripheral", 12));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_TLM_FP_STATE_SIZE(2);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(1).arg, FPManagerState::FAULT);

  // Raise the debounce count -- if recovery were gated by it too, a single
  // healthy reading would not be enough to leave FAULT mode. The recovery
  // check in faultModeHealthCheck is deliberately left undebounced: debounce
  // must make FPManager slower to act, never slower to stay safe.
  this->setFaultDebounce(50);
  this->invoke_to_peripheralThermalReadingIn(
      0, this->reading(2, ThermalStates::IDLE, 40.0F, "peripheral", 13));
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_TLM_FP_STATE_SIZE(3);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(2).arg, FPManagerState::SAFE);
}

void FPManagerTester::faultDebounceCountParameterGatesAndPublishes() {
  this->initializeSafeMode();

  // A value within bounds is adopted and republished to telemetry.
  this->component.applyFaultDebounceCount(50);
  ASSERT_GT(this->tlmHistory_FAULT_DEBOUNCE_COUNT->size(), 0U);
  ASSERT_EQ(this->tlmHistory_FAULT_DEBOUNCE_COUNT
                ->at(this->tlmHistory_FAULT_DEBOUNCE_COUNT->size() - 1)
                .arg,
            50U);
  ASSERT_EVENTS_FAULT_DEBOUNCE_COUNT_REJECTED_SIZE(0);

  // A value above FAULT_DEBOUNCE_MAX (1000) is rejected; the previously
  // active value (50) stays in effect and is reported in the event.
  this->component.applyFaultDebounceCount(2000);
  ASSERT_EVENTS_FAULT_DEBOUNCE_COUNT_REJECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DEBOUNCE_COUNT_REJECTED(0, 2000U, 1000U, 50U);
  ASSERT_EQ(this->tlmHistory_FAULT_DEBOUNCE_COUNT
                ->at(this->tlmHistory_FAULT_DEBOUNCE_COUNT->size() - 1)
                .arg,
            50U);

  // Rejections fire every time, not just the first.
  this->component.applyFaultDebounceCount(5000);
  ASSERT_EVENTS_FAULT_DEBOUNCE_COUNT_REJECTED_SIZE(2);
  ASSERT_EVENTS_FAULT_DEBOUNCE_COUNT_REJECTED(1, 5000U, 1000U, 50U);
}

}  // namespace scalesSvc
