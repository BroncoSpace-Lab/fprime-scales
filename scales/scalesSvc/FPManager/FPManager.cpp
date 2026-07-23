// ======================================================================
// \title  FPManager.cpp
// \brief  Fault Protection Manager for the SCALES system.
// ======================================================================

#include "scales/scalesSvc/FPManager/FPManager.hpp"

namespace scalesSvc {

FPManager::FPManager(const char* const compName)
    : FPManagerComponentBase(compName),
      m_mode(FPManagerState::INIT),
      m_imxReading(),
      m_peripheralReading(),
      m_jetsonReadings{},
      m_imxReadingValid(false),
      m_peripheralReadingValid(false),
      m_jetsonReadingValid{},
      m_jetsonPowerState(JetsonPowerStateID::OFF),
      m_faultReading(),
      m_faultSource(),
      m_hasFault(false),
      m_safeModeHealthy(false) {}

FPManager::~FPManager() {}

void FPManager::run_handler(FwIndexType portNum, U32 context) {
    this->fpStateMachine_sendSignal_tick();
}

void FPManager::fatalIn_handler(FwIndexType portNum, FwEventIdType Id) {
    // Execute the one-shot protection action before forwarding to FatalHandler.
    // FatalHandler may terminate the process before queued component work runs.
    this->scalesSvc_FPStateMachine_action_SHUTDOWN(
        FPManagerComponentBase::SmId::fpStateMachine,
        scalesSvc_FPStateMachine::Signal::fatal);
    this->fpStateMachine_sendSignal_fatal();
    this->fatalOut_out(0, Id);
}

void FPManager::imxThermalReadingIn_handler(FwIndexType portNum,
                                             const ThermalReading& reading) {
    this->m_imxReading = reading;
    this->m_imxReadingValid = reading.get_tempState() != ThermalStates::NOT_USED;
}

void FPManager::peripheralThermalReadingIn_handler(FwIndexType portNum,
                                                   const ThermalReading& reading) {
    this->m_peripheralReading = reading;
    this->m_peripheralReadingValid = reading.get_tempState() != ThermalStates::NOT_USED;
}

void FPManager::mcpThermalReadingIn_handler(FwIndexType portNum,
                                            const ThermalReading& reading) {
    switch (reading.get_sensorId()) {
        case 1:
            this->imxThermalReadingIn_handler(portNum, reading);
            break;
        case 2:
            this->peripheralThermalReadingIn_handler(portNum, reading);
            break;
        default:
            break;
    }
}

void FPManager::jetsonThermalReadingIn_handler(FwIndexType portNum,
                                               const ThermalReading& reading) {
    const U8 sensorId = reading.get_sensorId();
    if (sensorId >= JETSON_SENSOR_COUNT) {
        return;
    }
    this->m_jetsonReadings[sensorId] = reading;
    this->m_jetsonReadingValid[sensorId] = reading.get_tempState() != ThermalStates::NOT_USED;
    U8 validCount = 0;
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        if (this->m_jetsonReadingValid[i]) {
            validCount++;
        }
    }
    this->tlmWrite_JETSON_VALID_READING_COUNT(validCount);
}

void FPManager::jetsonPowerStateIn_handler(FwIndexType portNum,
                                           const JetsonPowerStateID& stateNow) {
    this->m_jetsonPowerState = stateNow;
}

Fw::Success FPManager::jetsonPowerAuthorizeIn_handler(
    FwIndexType portNum, const JetsonPowerStateID& stateReq) {
    if (stateReq.e != JetsonPowerStateID::ON && stateReq.e != JetsonPowerStateID::OFF) {
        this->log_WARNING_HI_JETSON_POWER_REQUEST_REJECTED(
            stateReq, Fw::String("Unsupported Jetson power state"));
        return Fw::Success::FAILURE;
    }
    if (stateReq.e == JetsonPowerStateID::ON && this->m_mode != FPManagerState::HPC) {
        this->log_WARNING_HI_JETSON_POWER_REQUEST_REJECTED(
            stateReq, Fw::String("Jetson ON requires HPC Mode"));
        return Fw::Success::FAILURE;
    }

    if (this->m_mode == FPManagerState::EMERGENCY) {
        this->log_WARNING_HI_JETSON_POWER_REQUEST_REJECTED(
            stateReq, Fw::String("Emergency Shutdown is latched"));
        return Fw::Success::FAILURE;
    }

    return Fw::Success::SUCCESS;
}

void FPManager::ENABLE_HPC_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_mode != FPManagerState::SAFE || !this->m_safeModeHealthy) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->fpStateMachine_sendSignal_hpcMode_en();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FPManager::DISABLE_HPC_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_mode == FPManagerState::SAFE) {
        this->writeStateTelemetry();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }

    if (this->m_mode != FPManagerState::HPC) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->fpStateMachine_sendSignal_hpcMode_dis();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FPManager::scalesSvc_FPStateMachine_action_initializeSafeMode(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->m_mode = FPManagerState::SAFE;
    this->m_hasFault = false;
    this->m_safeModeHealthy = false;
    this->m_faultSource = Fw::String("");
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_safeModeHealthCheck(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->m_mode = FPManagerState::SAFE;
    ThermalReading faultReading;
    if (this->m_imxReadingValid && this->readingIsFault(this->m_imxReading)) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
    } else if (this->m_peripheralReadingValid && this->readingIsFault(this->m_peripheralReading)) {
        this->triggerPeripheralEmergencyShutdown(this->m_peripheralReading);
    } else {
        this->m_safeModeHealthy = true;
        this->writeStateTelemetry();
        this->fpStateMachine_sendSignal_healthy();
    }
}

void FPManager::scalesSvc_FPStateMachine_action_hpcModeHealthCheck(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->m_mode = FPManagerState::HPC;
    ThermalReading faultReading;
    const bool imxFault = this->m_imxReadingValid && readingIsFault(this->m_imxReading);
    const bool peripheralFault =
        this->m_peripheralReadingValid && readingIsFault(this->m_peripheralReading);
    const bool jetsonFault = this->findJetsonFault(faultReading);

    if (imxFault) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
    } else if (peripheralFault) {
        if (jetsonFault) {
            this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
        }
        this->triggerPeripheralEmergencyShutdown(this->m_peripheralReading);
    } else if (jetsonFault) {
        this->rememberFault("JETSON", faultReading);
        this->fpStateMachine_sendSignal_jetson_fault();
    } else {
        this->writeStateTelemetry();
        this->fpStateMachine_sendSignal_healthy();
    }
}

void FPManager::scalesSvc_FPStateMachine_action_enableHpcMode(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->m_mode = FPManagerState::HPC;
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_disableHpcMode(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->m_jetsonPowerState == JetsonPowerStateID::ON) {
        this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    }
    this->m_jetsonPowerState = JetsonPowerStateID::OFF;
    this->m_mode = FPManagerState::SAFE;
    this->m_safeModeHealthy = false;
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_confirmJetsonFaultAndPowerOff(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    ThermalReading faultReading;
    if (!this->findJetsonFault(faultReading)) {
        this->fpStateMachine_sendSignal_failure();
        return;
    }
    this->rememberFault("JETSON", faultReading);
    this->reportReadingFault();
    this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    this->m_mode = FPManagerState::SAFE;
    this->writeStateTelemetry();
    this->fpStateMachine_sendSignal_success();
}

void FPManager::scalesSvc_FPStateMachine_action_reportFault(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->m_mode = FPManagerState::FAULT;
    this->reportReadingFault();
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_SHUTDOWN(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->m_mode == FPManagerState::EMERGENCY) {
        return;
    }
    this->m_mode = FPManagerState::EMERGENCY;
    this->log_WARNING_HI_EMERGENCY_SHUTDOWN();
    this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    this->peripheralPowerOff_out(0);
    this->writeStateTelemetry();
}

bool FPManager::readingIsFault(const ThermalReading& reading) const {
    return reading.get_tempState() == ThermalStates::FAULT;
}

bool FPManager::findJetsonFault(ThermalReading& faultReading) const {
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        if (this->m_jetsonReadingValid[i] && this->readingIsFault(this->m_jetsonReadings[i])) {
            faultReading = this->m_jetsonReadings[i];
            return true;
        }
    }
    return false;
}

void FPManager::rememberFault(const char* source, const ThermalReading& reading) {
    this->m_faultSource = Fw::String(source);
    this->m_faultReading = reading;
    this->m_hasFault = true;
}

void FPManager::triggerImxEmergencyShutdown(const ThermalReading& reading) {
    this->m_safeModeHealthy = false;
    this->rememberFault("IMX", reading);
    this->reportReadingFault();
    this->scalesSvc_FPStateMachine_action_SHUTDOWN(
        FPManagerComponentBase::SmId::fpStateMachine,
        scalesSvc_FPStateMachine::Signal::fatal);
    this->fpStateMachine_sendSignal_fatal();
    this->fatalOut_out(0, this->getIdBase() + EVENTID_EMERGENCY_SHUTDOWN);
}

void FPManager::triggerPeripheralEmergencyShutdown(const ThermalReading& reading) {
    this->m_safeModeHealthy = false;
    this->rememberFault("PERIPHERAL", reading);
    this->peripheralPowerOff_out(0);
    this->fpStateMachine_sendSignal_failure();
}

void FPManager::reportReadingFault() {
    if (!this->m_hasFault) {
        return;
    }
    this->log_WARNING_HI_FAULT_DETECTED(
        this->m_faultSource,
        this->m_faultReading.get_sensorId(),
        this->m_faultReading.get_temperature(),
        this->m_faultReading.get_tempState(),
        this->m_faultReading.get_location(),
        this->m_faultReading.get_timestamp());
}

void FPManager::writeStateTelemetry() {
    this->tlmWrite_FP_STATE(this->m_mode);
}

}  // namespace scalesSvc
