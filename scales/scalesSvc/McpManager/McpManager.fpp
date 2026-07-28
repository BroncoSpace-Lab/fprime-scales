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

        @ Complete readings for the i.MX, peripheral, and Jetson-board sensors.
        output port thermalReadingOut: ThermalReadingPort

        ###############################################################################
        #                                 Telemetry                                   #
        ###############################################################################

        @ Telemetry to log imx_temp data
        telemetry IMX_TEMP: ThermalReading id 0

        @ Telemetry to log periferal temp data
        telemetry PERIPHERAL_TEMP: ThermalReading id 1

        @ Telemetry to log Jetson temp data
        telemetry JETSON_TEMP: ThermalReading id 2


        @ Telmetry for i.MX IDLE state low threshold
        telemetry MCP_IMX_IDLE_LOW: F32 id 0x10

        @ Telmetry for i.MX IDLE state high threshold
        telemetry MCP_IMX_IDLE_HIGH: F32 id 0x11

        @ Telmetry for i.MX WARNING state low threshold
        telemetry MCP_IMX_WARN_LOW: F32 id 0x12

        @ Telmetry for i.MX WARNING state high threshold
        telemetry MCP_IMX_WARN_HIGH: F32 id 0x13

        @ Telmetry for i.MX FAULT state low threshold
        telemetry MCP_IMX_FAULT_LOW: F32 id 0x14

        @ Telmetry for i.MX FAULT state high threshold
        telemetry MCP_IMX_FAULT_HIGH: F32 id 0x15

        @ Telmetry for peripheral IDLE state low threshold
        telemetry MCP_PERIPHERAL_IDLE_LOW: F32 id 0x16

        @ Telmetry for peripheral IDLE state high threshold
        telemetry MCP_PERIPHERAL_IDLE_HIGH: F32 id 0x17

        @ Telmetry for peripheral WARNING state low threshold
        telemetry MCP_PERIPHERAL_WARN_LOW: F32 id 0x18

        @ Telmetry for peripheral WARNING state high threshold
        telemetry MCP_PERIPHERAL_WARN_HIGH: F32 id 0x19

        @ Telmetry for peripheral FAULT state low threshold
        telemetry MCP_PERIPHERAL_FAULT_LOW: F32 id 0x1A

        @ Telmetry for peripheral FAULT state high threshold
        telemetry MCP_PERIPHERAL_FAULT_HIGH: F32 id 0x1B

        @ Telmetry for Jetson-board IDLE state low threshold
        telemetry MCP_JETSON_IDLE_LOW: F32 id 0x1C

        @ Telmetry for Jetson-board IDLE state high threshold
        telemetry MCP_JETSON_IDLE_HIGH: F32 id 0x1D

        @ Telmetry for Jetson-board WARNING state low threshold
        telemetry MCP_JETSON_WARN_LOW: F32 id 0x1E

        @ Telmetry for Jetson-board WARNING state high threshold
        telemetry MCP_JETSON_WARN_HIGH: F32 id 0x1F

        @ Telmetry for Jetson-board FAULT state low threshold
        telemetry MCP_JETSON_FAULT_LOW: F32 id 0x20

        @ Telmetry for Jetson-board FAULT state high threshold
        telemetry MCP_JETSON_FAULT_HIGH: F32 id 0x21

        ###############################################################################
        #                                 Parameters                                  #
        ###############################################################################

        @ i.MX IDLE Low temperature threshold
        param MCP_IMX_IDLE_LOW: F32 \
            default 10 \
            id 0x00 \
            set opcode 0x01 \
            save opcode 0x02

        @ i.MX IDLE High temperature threshold
        param MCP_IMX_IDLE_HIGH: F32 \
            default 60 \
            id 0x01 \
            set opcode 0x03 \
            save opcode 0x04

        @ i.MX WARNING Low temperature threshold
        param MCP_IMX_WARN_LOW: F32 \
            default -20 \
            id 0x02 \
            set opcode 0x05 \
            save opcode 0x06

        @ i.MX WARNING High temperature threshold
        param MCP_IMX_WARN_HIGH: F32 \
            default 80 \
            id 0x03 \
            set opcode 0x07 \
            save opcode 0x08

        @ i.MX FAULT Low temperature threshold
        param MCP_IMX_FAULT_LOW: F32 \
            default -40 \
            id 0x04 \
            set opcode 0x09 \
            save opcode 0x0A

        @ i.MX FAULT High temperature threshold
        param MCP_IMX_FAULT_HIGH: F32 \
            default 100 \
            id 0x05 \
            set opcode 0x0B \
            save opcode 0x0C

        @ Peripheral IDLE Low temperature threshold
        param MCP_PERIPHERAL_IDLE_LOW: F32 \
            default 10 \
            id 0x06 \
            set opcode 0x0D \
            save opcode 0x0E

        @ Peripheral IDLE High temperature threshold
        param MCP_PERIPHERAL_IDLE_HIGH: F32 \
            default 60 \
            id 0x07 \
            set opcode 0x0F \
            save opcode 0x10

        @ Peripheral WARNING Low temperature threshold
        param MCP_PERIPHERAL_WARN_LOW: F32 \
            default -20 \
            id 0x08 \
            set opcode 0x11 \
            save opcode 0x12

        @ Peripheral WARNING High temperature threshold
        param MCP_PERIPHERAL_WARN_HIGH: F32 \
            default 80 \
            id 0x09 \
            set opcode 0x13 \
            save opcode 0x14

        @ Peripheral FAULT Low temperature threshold
        param MCP_PERIPHERAL_FAULT_LOW: F32 \
            default -40 \
            id 0x0A \
            set opcode 0x15 \
            save opcode 0x16

        @ Peripheral FAULT High temperature threshold
        param MCP_PERIPHERAL_FAULT_HIGH: F32 \
            default 100 \
            id 0x0B \
            set opcode 0x17 \
            save opcode 0x18

        @ Jetson-board IDLE Low temperature threshold
        param MCP_JETSON_IDLE_LOW: F32 \
            default 10 \
            id 0x0C \
            set opcode 0x19 \
            save opcode 0x1A

        @ Jetson-board IDLE High temperature threshold
        param MCP_JETSON_IDLE_HIGH: F32 \
            default 60 \
            id 0x0D \
            set opcode 0x1B \
            save opcode 0x1C

        @ Jetson-board WARNING Low temperature threshold
        param MCP_JETSON_WARN_LOW: F32 \
            default -20 \
            id 0x0E \
            set opcode 0x1D \
            save opcode 0x1E

        @ Jetson-board WARNING High temperature threshold
        param MCP_JETSON_WARN_HIGH: F32 \
            default 80 \
            id 0x0F \
            set opcode 0x1F \
            save opcode 0x20

        @ Jetson-board FAULT Low temperature threshold
        param MCP_JETSON_FAULT_LOW: F32 \
            default -40 \
            id 0x10 \
            set opcode 0x21 \
            save opcode 0x22

        @ Jetson-board FAULT High temperature threshold
        param MCP_JETSON_FAULT_HIGH: F32 \
            default 100 \
            id 0x11 \
            set opcode 0x23 \
            save opcode 0x24


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

        @ A sensor's IDLE/WARN/FAULT thresholds are not in a sane ascending
        @ order (FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH
        @ <= FAULT_HIGH). Readings for this sensor may be misclassified (e.g.
        @ reported as FAULT when WARN or IDLE was intended) until corrected.
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
            format "{} temperature thresholds are not in ascending order: FAULT_LOW={} WARN_LOW={} IDLE_LOW={} IDLE_HIGH={} WARN_HIGH={} FAULT_HIGH={}"


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
