// ======================================================================
// \title  GdsCommandAuthority.cpp
// \brief  Automatic TCP/UART GDS command authority selection
// ======================================================================

#include "scales/scalesSvc/GdsCommandAuthority/GdsCommandAuthority.hpp"

namespace scalesSvc {

GdsCommandAuthority::GdsCommandAuthority(const char* const compName)
    : GdsCommandAuthorityComponentBase(compName),
      m_commander(GdsCommander::TCP),
      m_tcpConnected(false),
      m_tcpDisconnectedTicks(0),
      m_tcpConnectedSince(),
      m_tcpConnectedSinceValid(false),
      m_tcpCommandsAccepted(0),
      m_uartCommandsAccepted(0),
      m_tcpCommandsRejected(0),
      m_uartCommandsRejected(0) {}

GdsCommandAuthority::~GdsCommandAuthority() = default;

void GdsCommandAuthority::tcpCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    if (this->m_commander == GdsCommander::TCP) {
        ++this->m_tcpCommandsAccepted;
        this->tcpCmdOut_out(0, data, context);
    } else {
        ++this->m_tcpCommandsRejected;
        this->log_WARNING_LO_TcpCommandRejectedUartActive();
    }
}

void GdsCommandAuthority::uartCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    if (this->m_commander == GdsCommander::UART) {
        ++this->m_uartCommandsAccepted;
        this->uartCmdOut_out(0, data, context);
    } else {
        ++this->m_uartCommandsRejected;
        this->log_WARNING_LO_UartCommandRejectedTcpActive();
    }
}

void GdsCommandAuthority::tcpCmdResponseIn_handler(FwIndexType portNum,
                                                   FwOpcodeType opcode,
                                                   U32 cmdSeq,
                                                   const Fw::CmdResponse& response) {
    this->tcpCmdResponseOut_out(0, opcode, cmdSeq, response);
}

void GdsCommandAuthority::uartCmdResponseIn_handler(FwIndexType portNum,
                                                    FwOpcodeType opcode,
                                                    U32 cmdSeq,
                                                    const Fw::CmdResponse& response) {
    this->uartCmdResponseOut_out(0, opcode, cmdSeq, response);
}

void GdsCommandAuthority::tcpDrvReadyIn_handler(FwIndexType portNum) {
    this->setTcpConnected(true);
    this->tcpDrvReadyOut_out(0);
}

void GdsCommandAuthority::tcpDrvReceiveIn_handler(FwIndexType portNum,
                                                  Fw::Buffer& buffer,
                                                  const Drv::ByteStreamStatus& status) {
    if (status != Drv::ByteStreamStatus::OP_OK && status != Drv::ByteStreamStatus::RECV_NO_DATA) {
        this->setTcpConnected(false);
    }
    this->tcpDrvReceiveOut_out(0, buffer, status);
}

void GdsCommandAuthority::tcpDrvReceiveReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->tcpDrvReceiveReturnOut_out(0, buffer);
}

Drv::ByteStreamStatus GdsCommandAuthority::tcpDrvSendIn_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    const Drv::ByteStreamStatus status = this->tcpDrvSendOut_out(0, buffer);
    if (status != Drv::ByteStreamStatus::OP_OK && status != Drv::ByteStreamStatus::SEND_RETRY) {
        this->setTcpConnected(false);
    }
    return status;
}

void GdsCommandAuthority::schedIn_handler(FwIndexType portNum, U32 context) {
    if (this->m_tcpConnected) {
        this->m_tcpDisconnectedTicks = 0;
        if (this->m_commander == GdsCommander::UART) {
            if (this->tcpReconnectDebounceElapsed()) {
                this->m_commander = GdsCommander::TCP;
                this->log_ACTIVITY_HI_CommandAuthoritySwitchedToTcp();
            }
        }
    } else {
        if (this->m_tcpDisconnectedTicks < TCP_FALLBACK_TICKS) {
            ++this->m_tcpDisconnectedTicks;
        }
        if (this->m_commander == GdsCommander::TCP && this->m_tcpDisconnectedTicks >= TCP_FALLBACK_TICKS) {
            this->m_commander = GdsCommander::UART;
            this->log_WARNING_HI_CommandAuthoritySwitchedToUart();
        }
    }
    this->writeTelemetry();
}

void GdsCommandAuthority::setTcpConnected(bool connected) {
    if (connected == this->m_tcpConnected) {
        return;
    }

    this->m_tcpConnected = connected;
    this->m_tcpDisconnectedTicks = 0;
    if (connected) {
        this->m_tcpConnectedSince = this->getTime();
        this->m_tcpConnectedSinceValid = true;
        this->log_ACTIVITY_HI_TcpGdsConnected();
    } else {
        this->m_tcpConnectedSinceValid = false;
        this->log_WARNING_HI_TcpGdsDisconnected();
    }
    this->writeTelemetry();
}

bool GdsCommandAuthority::tcpReconnectDebounceElapsed() {
    const Fw::Time now = this->getTime();
    if (!this->m_tcpConnectedSinceValid || now.getTimeBase() != this->m_tcpConnectedSince.getTimeBase() ||
        now < this->m_tcpConnectedSince) {
        this->m_tcpConnectedSince = now;
        this->m_tcpConnectedSinceValid = true;
        return false;
    }

    const Fw::Time elapsed = Fw::Time::sub(now, this->m_tcpConnectedSince);
    return elapsed.getSeconds() >= 1;
}

void GdsCommandAuthority::writeTelemetry() {
    this->tlmWrite_CurrentCommander(this->m_commander);
    this->tlmWrite_TcpConnected(this->m_tcpConnected);
    this->tlmWrite_TcpDisconnectedSeconds(this->m_tcpDisconnectedTicks);
    this->tlmWrite_TcpCommandsAccepted(this->m_tcpCommandsAccepted);
    this->tlmWrite_UartCommandsAccepted(this->m_uartCommandsAccepted);
    this->tlmWrite_TcpCommandsRejected(this->m_tcpCommandsRejected);
    this->tlmWrite_UartCommandsRejected(this->m_uartCommandsRejected);
}

}  // namespace scalesSvc
