module scalesSvc {
    @ Component that will read temperature data from MCP sensors on the SCALES Merger Board
    active component McpManager {

        @Input port that will poll the sensors for temperature logging
        async input port McpRead: Svc.Sched

        @Output port that will send a request to the LinuxI2cdriver from the McpManager component
        output port McpWriteRead: Drv.I2cWriteRead
        # Note that since the command is a writeread, the output can technically act as an input and output

        telemetry mcp_imx: ThermalReading \
            id 0x00

        telemetry mcp_perif: ThermalReading \
            id 0x01
        
        telemetry mcp_jetson: ThermalReading \
            id 0x02


        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}