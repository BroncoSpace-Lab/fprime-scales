module scalesSvc {

  @ The GDS connection currently authorized to issue commands
  enum GdsCommander : U8 {
    TCP = 0
    UART = 1
  }

  @ Selects exactly one of the TCP and UART GDS command streams.
  @ It also observes the TCP byte-stream interface so disconnects can be
  @ detected without changing the F Prime TCP driver.
  passive component GdsCommandAuthority {

    @ Routed command received from the TCP F Prime router
    guarded input port tcpCmdIn: Fw.Com

    @ Routed command received from the UART F Prime router
    guarded input port uartCmdIn: Fw.Com

    @ Accepted TCP command, kept separate to preserve response identity
    output port tcpCmdOut: Fw.Com

    @ Accepted UART command, kept separate to preserve response identity
    output port uartCmdOut: Fw.Com

    @ TCP command response from the command splitter
    sync input port tcpCmdResponseIn: Fw.CmdResponse

    @ UART command response from the command splitter
    sync input port uartCmdResponseIn: Fw.CmdResponse

    @ TCP command response returned to the TCP router
    output port tcpCmdResponseOut: Fw.CmdResponse

    @ UART command response returned to the UART router
    output port uartCmdResponseOut: Fw.CmdResponse

    @ Observe and pass through TCP driver connection-ready notifications
    guarded input port tcpDrvReadyIn: Drv.ByteStreamReady
    output port tcpDrvReadyOut: Drv.ByteStreamReady

    @ Observe and pass through TCP receive data/status
    guarded input port tcpDrvReceiveIn: Drv.ByteStreamData
    output port tcpDrvReceiveOut: Drv.ByteStreamData

    @ Pass TCP receive-buffer ownership back to the driver
    sync input port tcpDrvReceiveReturnIn: Fw.BufferSend
    output port tcpDrvReceiveReturnOut: Fw.BufferSend

    @ Observe and pass through TCP send data/status
    guarded input port tcpDrvSendIn: Drv.ByteStreamSend
    output port tcpDrvSendOut: Drv.ByteStreamSend

    @ One-Hz authority/debounce update
    guarded input port schedIn: Svc.Sched

    @ TCP connection has become ready
    event TcpGdsConnected severity activity high \
      format "TCP GDS connected; starting TCP priority debounce if needed"

    @ A TCP receive or send operation reported disconnection
    event TcpGdsDisconnected severity warning high \
      format "TCP GDS disconnected; starting 15-second fallback timer"

    @ TCP remained disconnected long enough to enable UART commanding
    event CommandAuthoritySwitchedToUart severity warning high \
      format "Command authority switched to UART fallback"

    @ TCP remained connected long enough to regain command authority
    event CommandAuthoritySwitchedToTcp severity activity high \
      format "Command authority switched back to TCP"

    @ A TCP command was rejected while UART fallback was active
    event TcpCommandRejectedUartActive severity warning low \
      format "TCP command rejected because UART fallback is active"

    @ A UART command was rejected while TCP was active
    event UartCommandRejectedTcpActive severity warning low \
      format "UART command rejected because TCP is active"

    telemetry CurrentCommander: GdsCommander
    telemetry TcpConnected: bool
    telemetry TcpDisconnectedSeconds: U32
    telemetry TcpCommandsAccepted: U32
    telemetry UartCommandsAccepted: U32
    telemetry TcpCommandsRejected: U32
    telemetry UartCommandsRejected: U32

    time get port timeCaller
    event port logOut
    text event port logTextOut
    telemetry port tlmOut
  }
}
