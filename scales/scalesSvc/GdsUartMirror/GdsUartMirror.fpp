module scalesSvc {

  @ Mirrors the GDS-facing framed byte stream to a secondary UART link.
  @ This component sits between Svc.ComStub and the concrete byte-stream
  @ drivers. Downlink bytes are sent to the primary TCP driver and mirrored
  @ to the UART driver. Uplink bytes from either TCP or UART are forwarded
  @ into the same ComStub receive path.
  passive component GdsUartMirror {

    @ Primary/TCP driver ready notification
    sync input port primaryConnectedIn: Drv.ByteStreamReady

    @ UART mirror driver ready notification
    sync input port mirrorConnectedIn: Drv.ByteStreamReady

    @ Ready notification forwarded to ComStub
    output port connectedOut: Drv.ByteStreamReady

    @ Framed downlink data from ComStub
    sync input port sendIn: Drv.ByteStreamSend

    @ Framed downlink data to the primary/TCP driver
    output port primarySendOut: Drv.ByteStreamSend

    @ Mirrored framed downlink data to the UART driver
    output port mirrorSendOut: Drv.ByteStreamSend

    @ Receive data from the primary/TCP driver
    sync input port primaryReceiveIn: Drv.ByteStreamData

    @ Receive data from the UART mirror driver
    sync input port mirrorReceiveIn: Drv.ByteStreamData

    @ Forward receive data into ComStub
    output port receiveOut: Drv.ByteStreamData

    @ Buffer ownership returned by ComStub after receive processing
    sync input port receiveReturnIn: Fw.BufferSend

    @ Return primary/TCP receive buffers to the primary/TCP driver
    output port primaryReceiveReturnOut: Fw.BufferSend

    @ Return UART receive buffers to the UART driver
    output port mirrorReceiveReturnOut: Fw.BufferSend
  }
}
