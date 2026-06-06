module scalesSvc {
    @ Device Manger to poll temperature data from on board MCP9808 temp sensors
    active component McpManager {

        @ Bind the ThermalStateMachine to McpManager
        state machine instance thermalStateMachine: ThermalStateMachine

        @ Output port allowing to connect to an I2c bus driver for writeRead operations to the mcp9808 temp sensors
        output port mcpWriteRead: Drv.I2cWriteRead

        @ Async scheduler input port to poll temp data from the sensors
        async input port run: Svc.Sched

        @ Telemetry to log imx_temp data
        telemetry IMX_TEMP: ThermalReading id 0

        @ Telemetry to log periferal temp data
        telemetry PERIPHERAL_TEMP: ThermalReading id 1

        @ Telemetry to log Jetson temp data
        telemetry JETSON_TEMP: ThermalReading id 2

        @ Telemetry to log the state of the manager
        telemetry MCP_THERMAL_STATE: string id 3

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