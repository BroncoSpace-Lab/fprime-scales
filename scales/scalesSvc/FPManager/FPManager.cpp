// ======================================================================
// \title  FPManager.cpp
// \brief  Fault Protection Manager for the SCALES system.
// ======================================================================

#include "scales/scalesSvc/FPManager/FPManager.hpp"

#include "Fw/Cmd/CmdPacket.hpp"

#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>

namespace scalesSvc {

namespace {
constexpr FwOpcodeType UNKNOWN_OPCODE = 0;
}

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
      m_safeModeHealthy(false),
      m_shutdownOutputsAsserted(false),
      m_lastPublishedState(FPManagerState::INIT),
      m_jetsonFaultSignalPending(false) {}

FPManager::~FPManager() {}

void FPManager::run_handler(FwIndexType portNum, U32 context) {
    this->fpStateMachine_sendSignal_tick();
}

void FPManager::fatalIn_handler(FwIndexType portNum, FwEventIdType Id) {
    // Emit and forward the fatal condition before cutting protected outputs.
    this->m_mode = FPManagerState::EMERGENCY;
    this->writeStateTelemetry();
    this->log_WARNING_HI_EMERGENCY_SHUTDOWN();
    this->fpStateMachine_sendSignal_fatal();
    this->fatalOut_out(0, Id);
    this->scalesSvc_FPStateMachine_action_SHUTDOWN(
        FPManagerComponentBase::SmId::fpStateMachine,
        scalesSvc_FPStateMachine::Signal::fatal);
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
    if (this->m_mode == FPManagerState::HPC &&
        !this->m_jetsonFaultSignalPending &&
        this->readingIsFault(reading)) {
        this->rememberFault("JETSON", reading);
        this->m_jetsonFaultSignalPending = true;
        this->fpStateMachine_sendSignal_jetson_fault();
    }
}

void FPManager::remoteJetsonCmdIn_handler(FwIndexType portNum,
                                          Fw::ComBuffer& data,
                                          U32 context) {
    const FwOpcodeType opcode = this->extractOpcode(data);
    if (this->m_jetsonPowerState != JetsonPowerStateID::ON) {
        this->log_WARNING_HI_REMOTE_JETSON_COMMAND_REJECTED(
            opcode, Fw::String("Jetson is not powered on"));
        this->remoteJetsonCmdResponseOut_out(
            0, opcode, context, Fw::CmdResponse::BUSY);
        return;
    }

    this->remoteJetsonCmdOut_out(0, data, context);
}

void FPManager::remoteJetsonCmdResponseIn_handler(
    FwIndexType portNum,
    FwOpcodeType opCode,
    U32 cmdSeq,
    const Fw::CmdResponse& response) {
    this->remoteJetsonCmdResponseOut_out(0, opCode, cmdSeq, response);
}

void FPManager::jetsonPowerStateIn_handler(FwIndexType portNum,
                                           const JetsonPowerStateID& stateNow) {
    this->m_jetsonPowerState = stateNow;
    if (stateNow == JetsonPowerStateID::OFF) {
        this->invalidateJetsonReadings();
    }
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
    this->m_shutdownOutputsAsserted = false;
    this->m_jetsonFaultSignalPending = false;
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
            this->m_jetsonPowerState = JetsonPowerStateID::OFF;
            this->invalidateJetsonReadings();
        }
        this->triggerPeripheralEmergencyShutdown(this->m_peripheralReading);
    } else if (jetsonFault) {
        this->rememberFault("JETSON", faultReading);
        this->m_jetsonFaultSignalPending = true;
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
    this->invalidateJetsonReadings();
    this->m_mode = FPManagerState::SAFE;
    this->m_safeModeHealthy = false;
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_confirmJetsonFaultAndPowerOff(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    ThermalReading faultReading;
    if (!this->findJetsonFault(faultReading)) {
        this->m_jetsonFaultSignalPending = false;
        this->fpStateMachine_sendSignal_failure();
        return;
    }
    this->rememberFault("JETSON", faultReading);
    this->reportReadingFault();
    this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    this->m_jetsonPowerState = JetsonPowerStateID::OFF;
    this->invalidateJetsonReadings();
    this->m_mode = FPManagerState::SAFE;
    this->m_jetsonFaultSignalPending = false;
    this->writeStateTelemetry();
    this->fpStateMachine_sendSignal_success();
}

void FPManager::scalesSvc_FPStateMachine_action_reportFault(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->m_mode == FPManagerState::FAULT) {
        return;
    }
    this->m_mode = FPManagerState::FAULT;
    this->reportReadingFault();
    this->writeStateTelemetry();
}

void FPManager::scalesSvc_FPStateMachine_action_faultModeHealthCheck(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->m_imxReadingValid && this->readingIsFault(this->m_imxReading)) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
        return;
    }
    ThermalReading jetsonFaultReading;
    if (this->findJetsonFault(jetsonFaultReading)) {
        this->rememberFault("JETSON", jetsonFaultReading);
        this->reportReadingFault();
        this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
        this->m_jetsonPowerState = JetsonPowerStateID::OFF;
        this->invalidateJetsonReadings();
        this->m_mode = FPManagerState::FAULT;
        this->writeStateTelemetry();
        return;
    }
    if (!this->m_peripheralReadingValid || this->readingIsFault(this->m_peripheralReading)) {
        return;
    }

    this->m_mode = FPManagerState::SAFE;
    this->m_safeModeHealthy = true;
    this->m_hasFault = false;
    this->m_faultSource = Fw::String("");
    this->writeStateTelemetry();
    this->fpStateMachine_sendSignal_healthy();
}

void FPManager::scalesSvc_FPStateMachine_action_SHUTDOWN(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->m_shutdownOutputsAsserted) {
        return;
    }
    if (this->m_mode != FPManagerState::EMERGENCY) {
        this->m_mode = FPManagerState::EMERGENCY;
        this->writeStateTelemetry();
    }
    this->m_shutdownOutputsAsserted = true;
    this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    this->m_jetsonPowerState = JetsonPowerStateID::OFF;
    this->invalidateJetsonReadings();
    this->peripheralPowerOff_out(0);
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

FwOpcodeType FPManager::extractOpcode(Fw::ComBuffer& data) const {
    Fw::CmdPacket cmdPkt;
    data.resetDeser();
    const Fw::SerializeStatus status = cmdPkt.deserializeFrom(data);
    const FwOpcodeType opcode =
        (status == Fw::FW_SERIALIZE_OK) ? cmdPkt.getOpCode() : UNKNOWN_OPCODE;
    data.resetDeser();
    return opcode;
}

void FPManager::invalidateJetsonReadings() {
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        this->m_jetsonReadingValid[i] = false;
    }
    this->m_jetsonFaultSignalPending = false;
    this->tlmWrite_JETSON_VALID_READING_COUNT(0);
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
    this->m_mode = FPManagerState::EMERGENCY;
    this->writeStateTelemetry();
    // This is the terminal thermal-fault path. Emit the operator-visible
    // emergency event after the state transition and before any fatal
    // forwarding or platform shutdown is initiated.
    this->log_WARNING_HI_EMERGENCY_SHUTDOWN();

#ifndef BUILD_UT
    // FatalHandler aborts this process and may not return. Schedule the
    // forced platform poweroff in a child so the request survives fatal
    // forwarding.
    const pid_t shutdownChild = ::fork();
    if (shutdownChild == 0) {
        ::sleep(1);
        ::execl("/sbin/poweroff", "poweroff", "-f", static_cast<char*>(nullptr));
        ::_exit(127);
    }
    if (shutdownChild < 0) {
        // Preserve the hardware-shutdown attempt if process creation fails.
        const int fallbackStatus =
            std::system("/bin/sh -c 'sleep 1; /sbin/poweroff -f' &");
        (void)fallbackStatus;
    }
#endif
    this->fpStateMachine_sendSignal_fatal();
    this->fatalOut_out(0, this->getIdBase() + EVENTID_EMERGENCY_SHUTDOWN);
    this->scalesSvc_FPStateMachine_action_SHUTDOWN(
        FPManagerComponentBase::SmId::fpStateMachine,
        scalesSvc_FPStateMachine::Signal::fatal);
}

void FPManager::triggerPeripheralEmergencyShutdown(const ThermalReading& reading) {
    this->m_safeModeHealthy = false;
    this->rememberFault("PERIPHERAL", reading);
    this->m_mode = FPManagerState::FAULT;
    this->reportReadingFault();
    this->writeStateTelemetry();
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
    if (this->m_mode != this->m_lastPublishedState) {
        this->log_ACTIVITY_HI_FP_STATE_CHANGED(this->m_lastPublishedState, this->m_mode);
        this->m_lastPublishedState = this->m_mode;
    }
    this->tlmWrite_FP_STATE(this->m_mode);
}

}  // namespace scalesSvc
