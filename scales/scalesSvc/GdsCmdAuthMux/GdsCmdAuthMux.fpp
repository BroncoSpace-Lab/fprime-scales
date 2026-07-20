module scalesSvc {
    @ Gates command buffers from TCP and UART GDS paths so only the active
    @ command authority can forward commands downstream.
    active component GdsCmdAuthMux {

        @ Bind the command-authority state machine to this component.
        state machine instance gdsMuxStateMachine: GdsMuxStateMachine

        ##########################################################################
        # Command-buffer path
        ##########################################################################

        @ Command buffers received from the TCP GDS path.
        sync input port tcpCmdIn: Fw.Com

        @ Command buffers received from the UART GDS path.
        sync input port uartCmdIn: Fw.Com

        @ Command buffer forwarded from the currently-authorized GDS path.
        output port cmdOut: Fw.Com

        @ Command status returned by the downstream dispatcher or splitter.
        sync input port cmdResponseIn: Fw.CmdResponse

        @ Command status returned to the TCP GDS command source.
        output port tcpCmdResponseOut: Fw.CmdResponse

        @ Command status returned to the UART GDS command source.
        output port uartCmdResponseOut: Fw.CmdResponse

        ##########################################################################
        # State-machine driving inputs
        ##########################################################################

        @ Rate-group input for driving timeout and connection-state checks.
        async input port run: Svc.Sched

        @ TCP connection-state indication from the TCP GDS transport path.
        sync input port tcpGdsStatus: Fw.SuccessCondition

        ##########################################################################
        # Operator commands
        ##########################################################################

        @ Switch command authority back to TCP after TCP has recovered and stayed stable.
        async command SWITCH_TO_TCP opcode 0x00

        ##########################################################################
        # Events
        ##########################################################################

        @ Command authority switched from TCP to UART.
        event CommandAuthoritySwitchedToUart \
            severity activity high \
            id 0x00 \
            format "Command authority switched to UART GDS"

        @ TCP GDS recovered while UART holds command authority.
        event TcpGdsRecovered \
            severity activity high \
            id 0x01 \
            format "TCP GDS recovered; waiting for stable interval before TCP authority can be restored"

        @ TCP recovered and remained stable long enough to allow manual return.
        event TcpGdsStable \
            severity activity high \
            id 0x02 \
            format "TCP GDS is stable; TCP command authority can be restored by command"

        @ Command authority switched from UART to TCP.
        event CommandAuthoritySwitchedToTcp \
            severity activity high \
            id 0x03 \
            format "Command authority switched to TCP GDS"

        @ Command rejected because it arrived from the inactive GDS path.
        event CommandRejectedInactiveAuthority(
                                                source: string size 8 @< Rejected command source
                                              ) \
            severity warning high \
            id 0x04 \
            format "Rejected command from inactive {} GDS command path"

        @ TCP authority was retained because TCP recovered inside the grace period.
        event TcpRecoveredDuringGrace \
            severity activity low \
            id 0x05 \
            format "TCP GDS recovered during command-authority grace period"

        ##########################################################################
        # Telemetry
        ##########################################################################

        @ Current command authority: TCP or UART.
        telemetry CommandAuthority: scalesSvc.CommandAuthority id 0x00

        @ Whether TCP is stable enough to allow a commanded return to TCP authority.
        telemetry TcpReadyForAuthority: Fw.On id 0x01

        @ Count of TCP command buffers rejected while UART had authority.
        telemetry TcpCommandsRejected: U32 id 0x02

        @ Count of UART command buffers rejected while TCP had authority.
        telemetry UartCommandsRejected: U32 id 0x03

        ##########################################################################
        # Standard ports
        ##########################################################################

        @ Port for requesting the current time.
        time get port timeCaller

        @ Port for sending command registrations.
        command reg port cmdRegOut

        @ Port for receiving component commands.
        command recv port cmdIn

        @ Port for sending component command responses.
        command resp port cmdResponseOut

        @ Port for sending textual representation of events.
        text event port logTextOut

        @ Port for sending events to downlink.
        event port logOut

        @ Port for sending telemetry channels to downlink.
        telemetry port tlmOut

    }
}
