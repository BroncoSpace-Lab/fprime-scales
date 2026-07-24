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
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_TLM_FP_STATE_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(0).arg, FPManagerState::SAFE);
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

void FPManagerTester::imxFaultTriggersEmergencyShutdown() {
  this->initializeSafeMode();
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

void FPManagerTester::fatalShutdownForwardsAndLatches() {
  this->initializeSafeMode();
  this->invoke_to_fatalIn(0, 0x1234);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_fatalOut(0, static_cast<FwEventIdType>(0x1234));
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
  ASSERT_from_peripheralPowerOff_SIZE(1);

  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
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

}  // namespace scalesSvc
