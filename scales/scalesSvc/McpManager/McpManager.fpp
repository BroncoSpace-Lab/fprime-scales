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


        @ Telmetry for IDLE state low threshold
        telemetry MCP_IDLE_LOW: F32 id 0x10

        @ Telmetry for IDLE state high threshold
        telemetry MCP_IDLE_HIGH: F32 id 0x11

        @ Telmetry for WARNING state low threshold
        telemetry MCP_WARN_LOW: F32 id 0x12

        @ Telmetry for WARNING state high threshold
        telemetry MCP_WARN_HIGH: F32 id 0x13

        @ Telmetry for FAULT state low threshold
        telemetry MCP_FAULT_LOW: F32 id 0x14

        @ Telmetry for FAULT state high threshold
        telemetry MCP_FAULT_HIGH: F32 id 0x15

        ###############################################################################
        #                                 Parameters                                  #
        ###############################################################################

        @ IDLE Low temperature threshold
        param MCP_IDLE_LOW: F32 \
            default 10 \
            id 0x00 \ 
            set opcode 0x01 \
            save opcode 0x02

        @ IDLE High temperature threshold
        param MCP_IDLE_HIGH: F32 \
            default 60 \
            id 0x01 \ 
            set opcode 0x03 \
            save opcode 0x04
        
        @ WARNING Low temperature threshold
        param MCP_WARN_LOW: F32 \
            default -20 \
            id 0x02 \ 
            set opcode 0x05 \
            save opcode 0x06

        @ WARNING High temperature threshold
        param MCP_WARN_HIGH: F32 \
            default 80 \   
            id 0x03 \ 
            set opcode 0x07 \
            save opcode 0x08
        
        @ FAULT Low temperature threshold
        param MCP_FAULT_LOW: F32 \
            default -40 \
            id 0x04 \ 
            set opcode 0x09 \
            save opcode 0x10
        
        @ FAULT High temperature threshold
        param MCP_FAULT_HIGH: F32 \
            default 100 \
            id 0x05 \ 
            set opcode 0x11 \
            save opcode 0x12
        

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
