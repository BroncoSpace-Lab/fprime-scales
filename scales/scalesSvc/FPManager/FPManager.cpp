// ======================================================================
// \title  FPManager.cpp
// \brief  Fault Protection Manager for the SCALES system.
// ======================================================================

#include "scales/scalesSvc/FPManager/FPManager.hpp"

#include "Fw/Cmd/CmdPacket.hpp"

#include <cerrno>
#include <cstdlib>
#include <sys/reboot.h>
#include <sys/wait.h>
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
      m_platformPoweroffTriggered(false),
      m_lastPublishedState(FPManagerState::INIT),
      m_jetsonFaultSignalPending(false),
      m_imxWarnActive(false),
      m_peripheralWarnActive(false),
      m_jetsonWarnActive(false),
      m_sourceFaultStreak{},
      m_jetsonFaultStreak{},
      m_imxLastSource(SRC_IMX_LOCAL),
      m_peripheralLastSource(SRC_PERIF_MCP),
      m_activeFaultDebounceCount(FAULT_DEBOUNCE_DEFAULT),
      m_justBooted(true),
      m_paramValid(Fw::ParamValid::VALID),
      m_jetsonBootOutstanding(false) {}

FPManager::~FPManager() {}

void FPManager::run_handler(FwIndexType portNum, U32 context) {
    this->loadDebounceParameterOnFirstTick();
    // Republish the active threshold every tick (not just on boot/change) so
    // a GDS session that connects late still sees it -- mirrors
    // ImxThermalManager republishing IMX_CPU_BOUNDS in doEvaluate().
    this->tlmWrite_FAULT_DEBOUNCE_COUNT(this->m_activeFaultDebounceCount);
    this->fpStateMachine_sendSignal_tick();
}

void FPManager::fatalIn_handler(FwIndexType portNum, FwEventIdType Id) {
    // This is the generic "some other component logged a FATAL-severity
    // event" path -- NOT a platform-power condition. It intentionally does
    // NOT power off the i.MX or cut Jetson/peripheral power: that is
    // reserved exclusively for a confirmed i.MX thermal FAULT (see
    // triggerImxEmergencyShutdown). Log what failed (the only detail
    // Svc.FatalEvent carries is this raw numeric ID; resolve it against the
    // GDS event dictionary to identify the component), latch the distinct
    // EMERGENCY_REBOOT state via the state machine, and forward to the real
    // Svc.FatalHandler as always -- it aborts the process, and systemd
    // (Restart=on-failure) already respawns just the flight-software
    // process. No new reboot/poweroff mechanism is needed here.
    this->log_WARNING_HI_COMPONENT_FAILURE_DETECTED(Id);
    this->fpStateMachine_sendSignal_component_fatal();
    this->fatalOut_out(0, Id);
}

void FPManager::imxThermalReadingIn_handler(FwIndexType portNum,
                                             const ThermalReading& reading) {
    this->updateImxDomain(SRC_IMX_LOCAL, reading);
}

void FPManager::peripheralThermalReadingIn_handler(FwIndexType portNum,
                                                   const ThermalReading& reading) {
    this->updatePeripheralDomain(SRC_PERIF_LOCAL, reading);
}

void FPManager::mcpThermalReadingIn_handler(FwIndexType portNum,
                                            const ThermalReading& reading) {
    switch (reading.get_sensorId()) {
        case 1:
            this->updateImxDomain(SRC_IMX_MCP, reading);
            break;
        case 2:
            this->updatePeripheralDomain(SRC_PERIF_MCP, reading);
            break;
        default:
            break;
    }
}

void FPManager::updateImxDomain(FaultSource src, const ThermalReading& reading) {
    this->m_imxReading = reading;
    this->m_imxReadingValid = reading.get_tempState() != ThermalStates::NOT_USED;
    this->m_imxLastSource = src;
    this->updateFaultStreak(this->m_sourceFaultStreak[src], reading);
    this->updateWarnTracking("IMX", this->readingIsWarn(reading), this->m_imxWarnActive, reading);
}

void FPManager::updatePeripheralDomain(FaultSource src, const ThermalReading& reading) {
    this->m_peripheralReading = reading;
    this->m_peripheralReadingValid = reading.get_tempState() != ThermalStates::NOT_USED;
    this->m_peripheralLastSource = src;
    this->updateFaultStreak(this->m_sourceFaultStreak[src], reading);
    this->updateWarnTracking("PERIPHERAL", this->readingIsWarn(reading), this->m_peripheralWarnActive, reading);
}

void FPManager::updateFaultStreak(U32& streak, const ThermalReading& reading) {
    if (this->readingIsFault(reading)) {
        if (streak < FAULT_DEBOUNCE_MAX) {
            streak++;
        }
    } else {
        // IDLE, WARN, and NOT_USED (unavailable) all break the streak.
        streak = 0;
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
    this->updateFaultStreak(this->m_jetsonFaultStreak[sensorId], reading);
    U8 validCount = 0;
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        if (this->m_jetsonReadingValid[i]) {
            validCount++;
        }
    }
    this->tlmWrite_JETSON_VALID_READING_COUNT(validCount);

    ThermalReading jetsonWarnReading;
    const bool jetsonCurrentlyWarn = this->findJetsonWarn(jetsonWarnReading);
    this->updateWarnTracking("JETSON", jetsonCurrentlyWarn, this->m_jetsonWarnActive,
                              jetsonCurrentlyWarn ? jetsonWarnReading : reading);

    if (this->m_mode == FPManagerState::HPC &&
        !this->m_jetsonFaultSignalPending &&
        this->jetsonSensorFaultConfirmed(sensorId)) {
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
            portNum, opcode, context, Fw::CmdResponse::BUSY);
        return;
    }

    this->remoteJetsonCmdOut_out(portNum, data, context);
}

void FPManager::remoteJetsonCmdResponseIn_handler(
    FwIndexType portNum,
    FwOpcodeType opCode,
    U32 cmdSeq,
    const Fw::CmdResponse& response) {
    this->remoteJetsonCmdResponseOut_out(portNum, opCode, cmdSeq, response);
}

void FPManager::jetsonPowerStateIn_handler(FwIndexType portNum,
                                           const JetsonPowerStateID& stateNow) {
    // Any real report -- ON or OFF -- proves JetsonManager has heard back
    // from the Jetson, so the (cosmetic-only) boot-outstanding mirror is
    // satisfied. See the m_jetsonBootOutstanding comment in FPManager.hpp.
    this->m_jetsonBootOutstanding = false;
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

    if (stateReq.e == JetsonPowerStateID::ON) {
        // Mirrors JetsonManager's own "only arm on a genuine off->on
        // transition" nuance (see JetsonManager.hpp's boot-confirmation
        // guard comment) -- a redundant ON authorization to an
        // already-confirmed-on Jetson must not re-arm this.
        this->m_jetsonBootOutstanding = (this->m_jetsonPowerState != JetsonPowerStateID::ON);
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

    // Informational only -- picks the right operator-facing wording for
    // what's about to happen. Does not change what happens: beginDisableHpcMode
    // (below) unconditionally requests Jetson OFF regardless of this, and
    // JetsonManager's own m_awaitingBootConfirmation/m_deferredOffPending is
    // the actual source of truth for deferring/firing that request -- this
    // must never reject or block while booting.
    if (this->m_jetsonBootOutstanding) {
        this->log_ACTIVITY_HI_HPC_MODE_DISABLE_JETSON_BOOTING();
    } else if (this->m_jetsonPowerState == JetsonPowerStateID::ON) {
        this->log_ACTIVITY_HI_HPC_MODE_DISABLE_JETSON_OFF_REQUESTED();
    } else {
        this->log_ACTIVITY_HI_HPC_MODE_DISABLE_JETSON_ALREADY_OFF();
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
    if (this->imxFaultConfirmed()) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
    } else if (this->peripheralFaultConfirmed()) {
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
    const bool imxFault = this->imxFaultConfirmed();
    const bool peripheralFault = this->peripheralFaultConfirmed();
    const bool jetsonFault = this->findConfirmedJetsonFault(faultReading);

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

void FPManager::scalesSvc_FPStateMachine_action_beginDisableHpcMode(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
    this->m_mode = FPManagerState::SAFE;
    this->m_safeModeHealthy = false;
    this->writeStateTelemetry();
    // m_jetsonPowerState/invalidateJetsonReadings() are deliberately left to
    // jetsonPowerStateIn_handler, driven by JetsonManager's real confirmation
    // (graceful ack or its own bounded GPIO-cut fallback), not declared here
    // -- see disableHpcModeHealthCheck.
}

void FPManager::scalesSvc_FPStateMachine_action_disableHpcModeHealthCheck(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    if (this->imxFaultConfirmed()) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
        return;
    }
    if (this->peripheralFaultConfirmed()) {
        this->triggerPeripheralEmergencyShutdown(this->m_peripheralReading);
        return;
    }
    if (this->m_jetsonPowerState == JetsonPowerStateID::OFF) {
        this->fpStateMachine_sendSignal_success();
    }
}

void FPManager::scalesSvc_FPStateMachine_action_confirmJetsonFaultAndPowerOff(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    ThermalReading faultReading;
    if (!this->findConfirmedJetsonFault(faultReading)) {
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
    if (this->imxFaultConfirmed()) {
        this->triggerImxEmergencyShutdown(this->m_imxReading);
        return;
    }
    ThermalReading jetsonFaultReading;
    if (this->findConfirmedJetsonFault(jetsonFaultReading)) {
        this->rememberFault("JETSON", jetsonFaultReading);
        this->reportReadingFault();
        this->jetsonPowerRequestOut_out(0, JetsonPowerStateID::OFF);
        this->m_jetsonPowerState = JetsonPowerStateID::OFF;
        this->invalidateJetsonReadings();
        this->m_mode = FPManagerState::FAULT;
        this->writeStateTelemetry();
        return;
    }
    // Recovery gate -- deliberately NOT debounced. This must make FPManager
    // slower to act (above), never slower to stay safe: a single raw FAULT
    // reading (or an unavailable reading) keeps FPManager latched in FAULT
    // mode even if that source hasn't yet accumulated a full debounce streak.
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
    // Cut i.MX platform power last, for every Emergency Shutdown entry (not
    // just the i.MX-thermal-fault path), once the Jetson and peripheral
    // outputs above are already latched off. This call does not return on
    // success, so nothing here may depend on running after it.
    this->triggerPlatformPoweroff();
}

void FPManager::scalesSvc_FPStateMachine_action_REBOOT(
    SmId smId, scalesSvc_FPStateMachine::Signal signal) {
    // No hardware outputs, no platform poweroff, no Jetson/peripheral cuts --
    // this path restarts the flight-software process only (via the real
    // Svc.FatalHandler + systemd Restart=on-failure, triggered separately by
    // fatalIn_handler's fatalOut_out() forward).
    if (this->m_mode == FPManagerState::EMERGENCY) {
        // A real platform shutdown is already latched; never downgrade it.
        return;
    }
    if (this->m_mode != FPManagerState::EMERGENCY_REBOOT) {
        this->m_mode = FPManagerState::EMERGENCY_REBOOT;
        this->writeStateTelemetry();
    }
}

bool FPManager::readingIsFault(const ThermalReading& reading) const {
    return reading.get_tempState() == ThermalStates::FAULT;
}

bool FPManager::imxFaultConfirmed() const {
    return this->m_imxReadingValid && this->readingIsFault(this->m_imxReading) &&
           this->m_sourceFaultStreak[this->m_imxLastSource] >= this->m_activeFaultDebounceCount;
}

bool FPManager::peripheralFaultConfirmed() const {
    return this->m_peripheralReadingValid && this->readingIsFault(this->m_peripheralReading) &&
           this->m_sourceFaultStreak[this->m_peripheralLastSource] >= this->m_activeFaultDebounceCount;
}

bool FPManager::jetsonSensorFaultConfirmed(U8 sensorId) const {
    return sensorId < JETSON_SENSOR_COUNT && this->m_jetsonReadingValid[sensorId] &&
           this->readingIsFault(this->m_jetsonReadings[sensorId]) &&
           this->m_jetsonFaultStreak[sensorId] >= this->m_activeFaultDebounceCount;
}

bool FPManager::findConfirmedJetsonFault(ThermalReading& faultReading) const {
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        if (this->jetsonSensorFaultConfirmed(static_cast<U8>(i))) {
            faultReading = this->m_jetsonReadings[i];
            return true;
        }
    }
    return false;
}

bool FPManager::readingIsWarn(const ThermalReading& reading) const {
    return reading.get_tempState() == ThermalStates::WARN;
}

bool FPManager::findJetsonWarn(ThermalReading& warnReading) const {
    for (FwIndexType i = 0; i < JETSON_SENSOR_COUNT; i++) {
        if (this->m_jetsonReadingValid[i] && this->readingIsWarn(this->m_jetsonReadings[i])) {
            warnReading = this->m_jetsonReadings[i];
            return true;
        }
    }
    return false;
}

void FPManager::updateWarnTracking(const char* source, bool currentlyWarn,
                                   bool& warnActiveFlag, const ThermalReading& reading) {
    if (currentlyWarn && !warnActiveFlag) {
        warnActiveFlag = true;
        this->log_WARNING_LO_WARN_STATE_ENTERED(
            Fw::String(source), reading.get_sensorId(), reading.get_temperature(),
            reading.get_location(), reading.get_timestamp());
    } else if (!currentlyWarn && warnActiveFlag) {
        warnActiveFlag = false;
        this->log_ACTIVITY_HI_WARN_STATE_EXITED(
            Fw::String(source), reading.get_sensorId(), reading.get_temperature(),
            reading.get_location(), reading.get_timestamp());
    }
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
        // Otherwise a stale pre-power-off streak could trip immediately on
        // the very first reading after HPC re-entry.
        this->m_jetsonFaultStreak[i] = 0;
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
    // emergency event after the state transition.
    this->log_WARNING_HI_EMERGENCY_SHUTDOWN();
    // Trigger the platform poweroff directly and unconditionally, right
    // after the events/telemetry above -- see fatalIn_handler for why this
    // must not be conditioned on the state machine dispatch or on any other
    // output succeeding.
    this->triggerPlatformPoweroff();
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

void FPManager::triggerPlatformPoweroff() {
    if (this->m_platformPoweroffTriggered) {
        return;
    }
    this->m_platformPoweroffTriggered = true;
#ifndef BUILD_UT
    // Confirmed on hardware: "poweroff -f" itself immediately powers the
    // board off when run from a context systemd doesn't tear down with it.
    // The command was never the problem -- every previous attempt to run it
    // FROM FPManager (a forked child, or this process calling reboot(2)
    // directly) ran as a member of ImxDeployment.service's own cgroup, and
    // systemd's KillMode reaps everything left in that cgroup the instant
    // this process exits/aborts, which happens moments later via
    // FatalHandler -- before the poweroff work could finish.
    ::sync();

    // Fastest path: ask the kernel directly first. Cheap to attempt, and if
    // this process does hold CAP_SYS_BOOT it powers the board off
    // immediately with no process spawn at all. On success this does not
    // return.
    const int rebootStatus = ::reboot(RB_POWER_OFF);
    const int rebootErrno = errno;
    (void)rebootStatus;
    this->log_WARNING_HI_PLATFORM_POWEROFF_SYSCALL_FAILED(rebootErrno);

    // Robust fallback: hand "poweroff -f" to a brand-new transient unit that
    // systemd (PID 1) spawns and owns directly, in its own cgroup outside
    // ImxDeployment.service, so it runs to completion regardless of what
    // happens to this service afterward.
    const int status = std::system("systemd-run --no-block --collect -- /sbin/poweroff -f");
    if (status == -1) {
        // std::system() itself could not run a shell at all.
        this->log_WARNING_HI_PLATFORM_POWEROFF_FALLBACK_FAILED(-1);
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        const I32 exitCode = WIFEXITED(status) ? static_cast<I32>(WEXITSTATUS(status)) : -2;
        this->log_WARNING_HI_PLATFORM_POWEROFF_FALLBACK_FAILED(exitCode);
    }
#endif
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

void FPManager::parameterUpdated(FwPrmIdType id) {
    switch (id) {
        case PARAMID_FAULT_DEBOUNCE_COUNT:
            this->applyFaultDebounceCount(this->paramGet_FAULT_DEBOUNCE_COUNT(this->m_paramValid));
            break;
        default:
            break;
    }
}

void FPManager::applyFaultDebounceCount(U32 candidate) {
    if (candidate <= FAULT_DEBOUNCE_MAX) {
        this->m_activeFaultDebounceCount = candidate;
        this->tlmWrite_FAULT_DEBOUNCE_COUNT(this->m_activeFaultDebounceCount);
    } else {
        this->log_WARNING_HI_FAULT_DEBOUNCE_COUNT_REJECTED(
            candidate, FAULT_DEBOUNCE_MAX, this->m_activeFaultDebounceCount);
    }
}

void FPManager::loadDebounceParameterOnFirstTick() {
    if (!this->m_justBooted) {
        return;
    }
    this->m_justBooted = false;
    // Gate the saved/default value through the same validity check as a live
    // PRM_SET, so a bad value previously saved to non-volatile storage cannot
    // take effect at boot.
    this->applyFaultDebounceCount(this->paramGet_FAULT_DEBOUNCE_COUNT(this->m_paramValid));
}

}  // namespace scalesSvc
