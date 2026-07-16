// ======================================================================
// \title  GdsCommandAuthority.hpp
// \brief  Automatic TCP/UART GDS command authority selection
// ======================================================================

#ifndef scales_scalesSvc_GdsCommandAuthority_HPP
#define scales_scalesSvc_GdsCommandAuthority_HPP

#include "scales/scalesSvc/GdsCommandAuthority/GdsCommandAuthorityComponentAc.hpp"

namespace scalesSvc {

class GdsCommandAuthority final : public GdsCommandAuthorityComponentBase {
  public:
    explicit GdsCommandAuthority(const char* const compName);
    ~GdsCommandAuthority() override;

  private:
    static constexpr U32 TCP_FALLBACK_TICKS = 15;

    void tcpCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;
    void uartCmdIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void tcpCmdResponseIn_handler(FwIndexType portNum,
                                  FwOpcodeType opcode,
                                  U32 cmdSeq,
                                  const Fw::CmdResponse& response) override;
    void uartCmdResponseIn_handler(FwIndexType portNum,
                                   FwOpcodeType opcode,
                                   U32 cmdSeq,
                                   const Fw::CmdResponse& response) override;

    void tcpDrvReadyIn_handler(FwIndexType portNum) override;
    void tcpDrvReceiveIn_handler(FwIndexType portNum,
                                 Fw::Buffer& buffer,
                                 const Drv::ByteStreamStatus& status) override;
    void tcpDrvReceiveReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    Drv::ByteStreamStatus tcpDrvSendIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void setTcpConnected(bool connected);
    bool tcpReconnectDebounceElapsed();
    void writeTelemetry();

    GdsCommander m_commander;
    bool m_tcpConnected;
    U32 m_tcpDisconnectedTicks;
    Fw::Time m_tcpConnectedSince;
    bool m_tcpConnectedSinceValid;
    U32 m_tcpCommandsAccepted;
    U32 m_uartCommandsAccepted;
    U32 m_tcpCommandsRejected;
    U32 m_uartCommandsRejected;
};

}  // namespace scalesSvc

#endif
