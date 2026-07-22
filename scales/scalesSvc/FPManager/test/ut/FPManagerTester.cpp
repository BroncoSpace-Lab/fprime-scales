#include "FPManagerTester.hpp"

namespace scalesSvc {

FPManagerTester::FPManagerTester()
    : FPManagerGTestBase("FPManagerTester", FPManagerTester::MAX_HISTORY_SIZE),
      component("FPManager") {
  this->initComponents();
  this->connectPorts();
}

FPManagerTester::~FPManagerTester() {}

void FPManagerTester::drainStateMachine() {
  for (FwSizeType i = 0; i < 20; ++i) {
    this->component.doDispatch();
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

void FPManagerTester::initializeSafeMode() {
  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  ASSERT_TLM_FP_STATE_SIZE(1);
  ASSERT_EQ(this->tlmHistory_FP_STATE->at(0).arg, 1U);
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
  ASSERT_from_jetsonPowerRequestOut_SIZE(1);
  ASSERT_from_jetsonPowerRequestOut(0, JetsonPowerStateID::OFF);
}

void FPManagerTester::entersHpcModeAndAcceptsJetsonOn() {
  this->initializeSafeMode();
  this->enterHpcMode();
  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::SUCCESS);
  ASSERT_EVENTS_JETSON_POWER_REQUEST_REJECTED_SIZE(0);
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
  this->component.doDispatch();

  this->invoke_to_run(0, 0);
  this->drainStateMachine();
  this->invoke_to_run(0, 0);
  this->drainStateMachine();

  ASSERT_EVENTS_FAULT_DETECTED_SIZE(1);
  ASSERT_EVENTS_FAULT_DETECTED(0, "Jetson", 4U, 99.0F,
                               ThermalStates::FAULT, "gpu-cluster", 42U);
  ASSERT_from_jetsonPowerRequestOut_SIZE(2);
  ASSERT_from_jetsonPowerRequestOut(1, JetsonPowerStateID::OFF);
}

void FPManagerTester::fatalShutdownForwardsAndLatches() {
  this->initializeSafeMode();
  this->invoke_to_fatalIn(0, 0x1234);
  this->drainStateMachine();

  ASSERT_EVENTS_EMERGENCY_SHUTDOWN_SIZE(1);
  ASSERT_from_fatalOut_SIZE(1);
  ASSERT_from_fatalOut(0, static_cast<FwEventIdType>(0x1234));
  ASSERT_from_jetsonPowerRequestOut_SIZE(2);
  ASSERT_from_peripheralPowerOff_SIZE(1);

  const Fw::Success result =
      this->invoke_to_jetsonPowerAuthorizeIn(0, JetsonPowerStateID::ON);
  ASSERT_EQ(result, Fw::Success::FAILURE);
}

}  // namespace scalesSvc
