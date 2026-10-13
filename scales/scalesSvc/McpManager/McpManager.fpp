module scalesSvc {
    @ Device Manger to poll temperature data from on board MCP9808 temp sensors
    active component McpManager {

        @ Bind the ThermalStateMachine to McpManager
        state machine instance mcp_thermalStateMachine: ThermalStateMachine

        ###############################################################################
        #                                 General Ports                               #
        ###############################################################################

        @ Output port allowing to connect to an I2c bus driver for writeRead operations to the mcp9808 temp sensors
        output port mcpWriteRead: Drv.I2cWriteRead

        @ Async scheduler input port to poll temp data from the sensors
        async input port run: Svc.Sched

        @ Output to send all of thermal readings to DataProducer
        output port mcpThermalReadOut: McpThermalReadings
        
        @ Complete readings for the i.MX, peripheral, and Jetson-board sensors.
        output port thermalReadingOut: ThermalReadingPort

        ###############################################################################
        #                    Telemetry + Parameters, grouped by subsystem             #
        ###############################################################################

        @ Telemetry to log imx_temp data
        telemetry IMX_TEMP: ThermalReading id 0

        @ Currently active i.MX IDLE/WARN/FAULT bounds (rejected updates never
        @ reach this channel -- see MCP_IMX_BOUNDS parameter).
        telemetry MCP_IMX_BOUNDS: TempBounds id 0x10

        @ i.MX sensor IDLE/WARN/FAULT temperature bounds
        param MCP_IMX_BOUNDS: TempBounds \
            default { faultLow = -40.0, warnLow = -20.0, idleLow = 10.0, idleHigh = 60.0, warnHigh = 80.0, faultHigh = 100.0 } \
            id 0x00 \
            set opcode 0x01 \
            save opcode 0x02


        @ Telemetry to log periferal temp data
        telemetry PERIPHERAL_TEMP: ThermalReading id 1

        @ Currently active peripheral IDLE/WARN/FAULT bounds (rejected updates
        @ never reach this channel -- see MCP_PERIPHERAL_BOUNDS parameter).
        telemetry MCP_PERIPHERAL_BOUNDS: TempBounds id 0x11

        @ Peripheral sensor IDLE/WARN/FAULT temperature bounds
        param MCP_PERIPHERAL_BOUNDS: TempBounds \
            default { faultLow = -40.0, warnLow = -20.0, idleLow = 10.0, idleHigh = 60.0, warnHigh = 80.0, faultHigh = 100.0 } \
            id 0x01 \
            set opcode 0x03 \
            save opcode 0x04


        @ Telemetry to log Jetson temp data
        telemetry JETSON_TEMP: ThermalReading id 2

        @ Currently active Jetson-board IDLE/WARN/FAULT bounds (rejected
        @ updates never reach this channel -- see MCP_JETSON_BOUNDS parameter).
        telemetry MCP_JETSON_BOUNDS: TempBounds id 0x12

        @ Jetson-board sensor IDLE/WARN/FAULT temperature bounds
        param MCP_JETSON_BOUNDS: TempBounds \
            default { faultLow = -40.0, warnLow = -20.0, idleLow = 10.0, idleHigh = 60.0, warnHigh = 80.0, faultHigh = 100.0 } \
            id 0x02 \
            set opcode 0x05 \
            save opcode 0x06


        ###############################################################################
        #                                 Events                                      #
        ###############################################################################
        event FAIL_TO_READ_TEMP_AT(
            location: string @< The location of the sensor that failed to read
        ) \
            severity warning high \
            id 0x00 \
            format "Failed to read temperature from sensor at location: {}"

        event FAIL_TO_READ_TEMP(

        ) \
            severity warning high \
            id 0x01 \
            format "Failed to read temperature from one or more sensors"

        @ A sensor's newly-set IDLE/WARN/FAULT bounds are not in a sane
        @ ascending order (FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <=
        @ WARN_HIGH <= FAULT_HIGH). The update is rejected and the sensor
        @ keeps using its last-known-good bounds -- fires on every rejected
        @ attempt, not just the first, since misconfiguration never actually
        @ takes effect.
        event THRESHOLDS_MISCONFIGURED(
            source: string size 32
            faultLow: F32
            warnLow: F32
            idleLow: F32
            idleHigh: F32
            warnHigh: F32
            faultHigh: F32
        ) \
            severity warning high \
            id 0x02 \
            format "{} temperature bounds rejected, not in ascending order: FAULT_LOW={} WARN_LOW={} IDLE_LOW={} IDLE_HIGH={} WARN_HIGH={} FAULT_HIGH={}"


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
