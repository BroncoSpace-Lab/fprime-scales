// ======================================================================
// \title  GdsCmdAuthMux.cpp
// \author luquitolanzi
// \brief  cpp file for GdsCmdAuthMux component implementation class
// ======================================================================

#include "scales/scalesSvc/GdsCmdAuthMux/GdsCmdAuthMux.hpp"

#include "Fw/Cmd/CmdPacket.hpp"
#include "Fw/Types/String.hpp"

namespace scalesSvc {

namespace {
constexpr U32 TCP_DOWN_GRACE_SECONDS = 10;
constexpr U32 TCP_STABLE_SECONDS = 2;
constexpr FwOpcodeType UNKNOWN_OPCODE = 0;
}

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

GdsCmdAuthMux ::GdsCmdAuthMux(const char* const compName)
    : GdsCmdAuthMuxComponentBase(compName),
      m_tcpStatusPoller(nullptr),
      m_tcpStatusInitialized(false),
      m_authority(scalesSvc::CommandAuthority::TCP),
      m_lastForwardedSource(CommandSource::NONE),
      m_outstandingCommands{},
      m_tcpConnected(false),
      m_tcpReady(false),
      m_tcpDownStartTime(Fw::Time::zero()),
      m_tcpStableStartTime(Fw::Time::zero()),
      m_tcpCommandsRejected(0),
      m_uartCommandsRejected(0) {}

GdsCmdAuthMux ::~GdsCmdAuthMux() {}

void GdsCmdAuthMux ::configureTcpStatusPoller(TcpStatusPoller poller) {
    this->m_tcpStatusPoller = poller;
}

void GdsCmdAuthMux ::updateTelemetry() {
    this->tlmWrite_CommandAuthority(this->m_authority);
    this->tlmWrite_TcpReadyForAuthority(this->m_tcpReady ? Fw::On::ON : Fw::On::OFF);
    this->tlmWrite_TcpCommandsRejected(this->m_tcpCommandsRejected);
    this->tlmWrite_UartCommandsRejected(this->m_uartCommandsRejected);
}

bool GdsCmdAuthMux ::tcpHasAuthority() const {
    return this->m_authority == scalesSvc::CommandAuthority::TCP;
}

bool GdsCmdAuthMux ::uartHasAuthority() const {
    return this->m_authority == scalesSvc::CommandAuthority::UART;
}

bool GdsCmdAuthMux ::tcpReadyForCommandedReturn() const {
    return this->m_tcpReady && this->uartHasAuthority();
}

bool GdsCmdAuthMux ::elapsedAtLeast(const Fw::Time& start, U32 seconds, U32 useconds) const {
    const Fw::Time now = this->getTime();
    const Fw::TimeInterval elapsed(start, now);
    return elapsed >= Fw::TimeInterval(seconds, useconds);
}

bool GdsCmdAuthMux ::elapsedGreaterThan(const Fw::Time& start, U32 seconds, U32 useconds) const {
    const Fw::Time now = this->getTime();
    const Fw::TimeInterval elapsed(start, now);
    return elapsed > Fw::TimeInterval(seconds, useconds);
}

FwOpcodeType GdsCmdAuthMux ::extractOpcode(Fw::ComBuffer& data) const {
    Fw::CmdPacket cmdPkt;
    data.resetDeser();
    const Fw::SerializeStatus status = cmdPkt.deserializeFrom(data);
    return (status == Fw::FW_SERIALIZE_OK) ? cmdPkt.getOpCode() : UNKNOWN_OPCODE;
}

void GdsCmdAuthMux ::rejectCommand(CommandSource source, Fw::ComBuffer& data, U32 context) {
    const FwOpcodeType opcode = this->extractOpcode(data);
    if (source == CommandSource::TCP) {
        this->m_tcpCommandsRejected++;
        this->log_WARNING_HI_CommandRejectedInactiveAuthority(Fw::String("TCP"));
        this->tcpCmdResponseOut_out(0, opcode, context, Fw::CmdResponse::BUSY);
    } else if (source == CommandSource::UART) {
        this->m_uartCommandsRejected++;
        this->log_WARNING_HI_CommandRejectedInactiveAuthority(Fw::String("UART"));
        this->uartCmdResponseOut_out(0, opcode, context, Fw::CmdResponse::BUSY);
    }
    this->updateTelemetry();
}

void GdsCmdAuthMux ::recordOutstandingCommand(CommandSource source, Fw::ComBuffer& data, U32 context) {
    const FwOpcodeType opcode = this->extractOpcode(data);
    for (FwIndexType i = 0; i < FW_NUM_ARRAY_ELEMENTS(this->m_outstandingCommands); i++) {
        OutstandingCommand& entry = this->m_outstandingCommands[i];
        if (!entry.active) {
            entry.active = true;
            entry.opcode = opcode;
            entry.cmdSeq = context;
            entry.source = source;
            return;
        }
    }
    // If the table fills, keep the newest command routable by replacing the oldest slot.
    this->m_outstandingCommands[0].active = true;
    this->m_outstandingCommands[0].opcode = opcode;
    this->m_outstandingCommands[0].cmdSeq = context;
    this->m_outstandingCommands[0].source = source;
}

GdsCmdAuthMux::CommandSource GdsCmdAuthMux ::findAndClearOutstandingCommand(FwOpcodeType opcode, U32 cmdSeq) {
    for (FwIndexType i = 0; i < FW_NUM_ARRAY_ELEMENTS(this->m_outstandingCommands); i++) {
        OutstandingCommand& entry = this->m_outstandingCommands[i];
        if (entry.active && (entry.opcode == opcode) && (entry.cmdSeq == cmdSeq)) {
            const CommandSource source = entry.source;
            entry.active = false;
            entry.source = CommandSource::NONE;
            return source;
        }
    }
    return CommandSource::NONE;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void GdsCmdAuthMux ::cmdResponseIn_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) {
    CommandSource source = this->findAndClearOutstandingCommand(opCode, cmdSeq);
    if (source == CommandSource::NONE) {
        source = this->m_lastForwardedSource;
    }
    if (source == CommandSource::UART) {
        this->uartCmdResponseOut_out(0, opCode, cmdSeq, response);
    } else {
        this->tcpCmdResponseOut_out(0, opCode, cmdSeq, response);
    }
}

void GdsCmdAuthMux ::run_handler(FwIndexType portNum, U32 context) {
    this->gdsMuxStateMachine_sendSignal_tick();
    if (this->m_tcpStatusPoller != nullptr) {
        Fw::Success condition = this->m_tcpStatusPoller() ? Fw::Success::SUCCESS : Fw::Success::FAILURE;
        this->tcpGdsStatus_handler(0, condition);
    }
    this->updateTelemetry();
}

void GdsCmdAuthMux ::tcpCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    // Once TCP has recovered and passed the stability interval, allow the
    // recovery command itself through TCP. All other TCP commands remain
    // gated until authority has actually switched back.
    const bool tcpRecoveryCommand =
        this->uartHasAuthority() && this->tcpReadyForCommandedReturn() &&
        (this->extractOpcode(data) == this->getIdBase());

    if (this->tcpHasAuthority() || tcpRecoveryCommand) {
        this->m_lastForwardedSource = CommandSource::TCP;
        this->recordOutstandingCommand(CommandSource::TCP, data, context);
        this->cmdOut_out(0, data, context);
    } else {
        this->rejectCommand(CommandSource::TCP, data, context);
    }
}

void GdsCmdAuthMux ::tcpGdsStatus_handler(FwIndexType portNum, Fw::Success& condition) {
    const bool isConnected = condition == Fw::Success::SUCCESS;

    // Preserve the first sample until tcp_init has established the initial
    // authority state. This prevents a disconnected startup from skipping the
    // TCP-down grace-period transition.
    if (!this->m_tcpStatusInitialized) {
        this->m_tcpConnected = isConnected;
        return;
    }

    if (isConnected != this->m_tcpConnected) {
        this->m_tcpConnected = isConnected;
        if (this->m_tcpConnected) {
            // TCP authority is retained while the grace timer is active. The
            // recovery edge is observable here before the state transition.
            if (this->tcpHasAuthority()) {
                this->log_ACTIVITY_LO_TcpRecoveredDuringGrace();
            }
            this->gdsMuxStateMachine_sendSignal_tcp_gds_up();
        } else {
            this->m_tcpReady = false;
            this->gdsMuxStateMachine_sendSignal_tcp_gds_down();
        }
    }
}

void GdsCmdAuthMux ::uartCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    if (this->uartHasAuthority()) {
        this->m_lastForwardedSource = CommandSource::UART;
        this->recordOutstandingCommand(CommandSource::UART, data, context);
        this->cmdOut_out(0, data, context);
    } else {
        this->rejectCommand(CommandSource::UART, data, context);
    }
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void GdsCmdAuthMux ::SWITCH_TO_TCP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->tcpReadyForCommandedReturn()) {
        this->gdsMuxStateMachine_sendSignal_tcp_auth_set();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
    }
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_tcp_init(SmId smId,
                                                                  scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::TCP;
    this->m_tcpReady = false;
    this->m_lastForwardedSource = CommandSource::NONE;
    this->m_tcpStatusInitialized = true;
    this->updateTelemetry();
    this->gdsMuxStateMachine_sendSignal_success();
    if (!this->m_tcpConnected) {
        this->gdsMuxStateMachine_sendSignal_tcp_gds_down();
    }
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_tcp_run(SmId smId,
                                                                 scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::TCP;
    this->m_tcpReady = false;
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_uart_run(SmId smId,
                                                                  scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::UART;
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_start_tcp_down_grace(
    SmId smId,
    scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_tcpDownStartTime = this->getTime();
    this->m_tcpReady = false;
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_monitor_tcp_down_grace(
    SmId smId,
    scalesSvc_GdsMuxStateMachine::Signal signal) {
    if (this->m_tcpConnected) {
        this->gdsMuxStateMachine_sendSignal_tcp_gds_up();
    } else if (this->elapsedAtLeast(this->m_tcpDownStartTime, TCP_DOWN_GRACE_SECONDS)) {
        this->gdsMuxStateMachine_sendSignal_success();
    }
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_switch_to_uart(SmId smId,
                                                                        scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::UART;
    this->m_tcpReady = false;
    this->log_ACTIVITY_HI_CommandAuthoritySwitchedToUart();
    this->updateTelemetry();
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_emit_tcp_recovered(
    SmId smId,
    scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->log_ACTIVITY_HI_TcpGdsRecovered();
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_start_tcp_stable_timer(
    SmId smId,
    scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_tcpStableStartTime = this->getTime();
    this->m_tcpReady = false;
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_monitor_tcp_stable_timer(
    SmId smId,
    scalesSvc_GdsMuxStateMachine::Signal signal) {
    if (!this->m_tcpConnected) {
        this->gdsMuxStateMachine_sendSignal_tcp_gds_down();
    } else if (this->elapsedGreaterThan(this->m_tcpStableStartTime, TCP_STABLE_SECONDS)) {
        this->gdsMuxStateMachine_sendSignal_success();
    }
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_mark_tcp_ready(SmId smId,
                                                                        scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_tcpReady = true;
    this->log_ACTIVITY_HI_TcpGdsStable();
    this->updateTelemetry();
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_switch_to_tcp(SmId smId,
                                                                       scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::TCP;
    this->m_tcpReady = false;
    this->log_ACTIVITY_HI_CommandAuthoritySwitchedToTcp();
    this->updateTelemetry();
}

void GdsCmdAuthMux ::scalesSvc_GdsMuxStateMachine_action_failure_eval(SmId smId,
                                                                      scalesSvc_GdsMuxStateMachine::Signal signal) {
    this->m_authority = scalesSvc::CommandAuthority::TCP;
    this->m_tcpReady = false;
    this->updateTelemetry();
    this->gdsMuxStateMachine_sendSignal_success();
}

}  // namespace scalesSvc
