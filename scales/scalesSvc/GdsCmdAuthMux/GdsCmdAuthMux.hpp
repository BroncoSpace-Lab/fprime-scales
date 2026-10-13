// ======================================================================
// \title  GdsCmdAuthMux.hpp
// \author luquitolanzi
// \brief  hpp file for GdsCmdAuthMux component implementation class
// ======================================================================

#ifndef scalesSvc_GdsCmdAuthMux_HPP
#define scalesSvc_GdsCmdAuthMux_HPP

#include "Fw/Time/Time.hpp"
#include "scales/scalesSvc/GdsCmdAuthMux/GdsCmdAuthMuxComponentAc.hpp"

namespace scalesSvc {

class GdsCmdAuthMux final : public GdsCmdAuthMuxComponentBase {
  public:
    using TcpStatusPoller = bool (*)();

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct GdsCmdAuthMux object
    GdsCmdAuthMux(const char* const compName  //!< The component name
    );

    //! Destroy GdsCmdAuthMux object
    ~GdsCmdAuthMux();

    //! Configure a deployment-provided TCP connection-state poller
    void configureTcpStatusPoller(TcpStatusPoller poller);

  private:
    enum class CommandSource : U8 { NONE = 0, TCP = 1, UART = 2 };

    struct OutstandingCommand {
        bool active;
        FwOpcodeType opcode;
        U32 cmdSeq;
        CommandSource source;
    };

    // ----------------------------------------------------------------------
    // Private helper functions
    // ----------------------------------------------------------------------

    void updateTelemetry();

    bool tcpHasAuthority() const;

    bool uartHasAuthority() const;

    bool tcpReadyForCommandedReturn() const;

    bool elapsedAtLeast(const Fw::Time& start, U32 seconds, U32 useconds = 0) const;

    bool elapsedGreaterThan(const Fw::Time& start, U32 seconds, U32 useconds = 0) const;

    FwOpcodeType extractOpcode(Fw::ComBuffer& data) const;

    void rejectCommand(CommandSource source, Fw::ComBuffer& data, U32 context);

    void recordOutstandingCommand(CommandSource source, Fw::ComBuffer& data, U32 context);

    CommandSource findAndClearOutstandingCommand(FwOpcodeType opcode, U32 cmdSeq);

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for cmdResponseIn
    //!
    //! Command status returned by the downstream dispatcher or splitter.
    void cmdResponseIn_handler(FwIndexType portNum,             //!< The port number
                               FwOpcodeType opCode,             //!< Command Op Code
                               U32 cmdSeq,                      //!< Command Sequence
                               const Fw::CmdResponse& response  //!< The command response argument
                               ) override;

    //! Handler implementation for run
    //!
    //! Rate-group input for driving timeout and connection-state checks.
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    //! Handler implementation for tcpCmdIn
    //!
    //! Command buffers received from the TCP GDS path.
    void tcpCmdIn_handler(FwIndexType portNum,  //!< The port number
                          Fw::ComBuffer& data,  //!< Buffer containing packet data
                          U32 context           //!< Call context value; meaning chosen by user
                          ) override;

    //! Handler implementation for tcpGdsStatus
    //!
    //! TCP connection-state indication from the TCP GDS transport path.
    void tcpGdsStatus_handler(FwIndexType portNum,    //!< The port number
                              Fw::Success& condition  //!< Condition success/failure
                              ) override;

    //! Handler implementation for uartCmdIn
    //!
    //! Command buffers received from the UART GDS path.
    void uartCmdIn_handler(FwIndexType portNum,  //!< The port number
                           Fw::ComBuffer& data,  //!< Buffer containing packet data
                           U32 context           //!< Call context value; meaning chosen by user
                           ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SWITCH_TO_TCP
    //!
    //! Switch command authority back to TCP after TCP has recovered and stayed stable.
    void SWITCH_TO_TCP_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq            //!< The command sequence number
                                  ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action tcp_init of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Initialize TCP GDS as the command authority.
    void scalesSvc_GdsMuxStateMachine_action_tcp_init(SmId smId,  //!< The state machine id
                                                      scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action tcp_run of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Run TCP authority behavior and gate non-TCP command paths.
    void scalesSvc_GdsMuxStateMachine_action_tcp_run(SmId smId,  //!< The state machine id
                                                     scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                     ) override;

    //! Implementation for action uart_run of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Run UART authority behavior and gate non-UART command paths.
    void scalesSvc_GdsMuxStateMachine_action_uart_run(SmId smId,  //!< The state machine id
                                                      scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action start_tcp_down_grace of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Start the TCP-down grace timer.
    void scalesSvc_GdsMuxStateMachine_action_start_tcp_down_grace(
        SmId smId,                                   //!< The state machine id
        scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action monitor_tcp_down_grace of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Monitor the TCP-down grace timer.
    void scalesSvc_GdsMuxStateMachine_action_monitor_tcp_down_grace(
        SmId smId,                                   //!< The state machine id
        scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action switch_to_uart of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Switch command authority to UART and emit CommandAuthoritySwitchedToUart.
    void scalesSvc_GdsMuxStateMachine_action_switch_to_uart(SmId smId,  //!< The state machine id
                                                            scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                            ) override;

    //! Implementation for action emit_tcp_recovered of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Emit TcpGdsRecovered.
    void scalesSvc_GdsMuxStateMachine_action_emit_tcp_recovered(
        SmId smId,                                   //!< The state machine id
        scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action start_tcp_stable_timer of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Start the TCP stable timer.
    void scalesSvc_GdsMuxStateMachine_action_start_tcp_stable_timer(
        SmId smId,                                   //!< The state machine id
        scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action monitor_tcp_stable_timer of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Monitor the TCP stable timer.
    void scalesSvc_GdsMuxStateMachine_action_monitor_tcp_stable_timer(
        SmId smId,                                   //!< The state machine id
        scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action mark_tcp_ready of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Mark TCP stable and ready for user-selected authority.
    void scalesSvc_GdsMuxStateMachine_action_mark_tcp_ready(SmId smId,  //!< The state machine id
                                                            scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                            ) override;

    //! Implementation for action switch_to_tcp of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Switch command authority to TCP and emit CommandAuthoritySwitchedToTcp.
    void scalesSvc_GdsMuxStateMachine_action_switch_to_tcp(SmId smId,  //!< The state machine id
                                                           scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                           ) override;

    //! Implementation for action failure_eval of state machine scalesSvc_GdsMuxStateMachine
    //!
    //! Evaluate failure conditions and handle errors.
    void scalesSvc_GdsMuxStateMachine_action_failure_eval(SmId smId,  //!< The state machine id
                                                          scalesSvc_GdsMuxStateMachine::Signal signal  //!< The signal
                                                          ) override;

  private:
    TcpStatusPoller m_tcpStatusPoller;
    // The first transport sample must not be treated as an edge until the
    // state machine has completed its initial transition.
    bool m_tcpStatusInitialized;
    scalesSvc::CommandAuthority m_authority;
    CommandSource m_lastForwardedSource;
    OutstandingCommand m_outstandingCommands[16];
    bool m_tcpConnected;
    bool m_tcpReady;
    Fw::Time m_tcpDownStartTime;
    Fw::Time m_tcpStableStartTime;
    U32 m_tcpCommandsRejected;
    U32 m_uartCommandsRejected;
};

}  // namespace scalesSvc

#endif
