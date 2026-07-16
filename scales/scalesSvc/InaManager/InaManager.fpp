module scalesSvc {
    @ Manager for INA260 current, voltage, and power sensor.
    active component InaManager {
        
        # ----------------------------------------------------------------------
        # General ports
        # ----------------------------------------------------------------------

        @ Port for performing I2C write/read transactions with the INA260 sensor
        output port busWriteRead : Drv.I2cWriteRead

        @ Input port for sending data each tick
        async input port run: Svc.Sched

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Telemetry channel for INA260 Jetson subsystem using struct PowerReading
        telemetry INA260_Jetson : PowerReading id 0

        @ Telemetry channel for INA260 OBC subsystem using struct PowerReading
        telemetry INA260_OBC : PowerReading id 1

        @ Telemetry channel for INA260 Peripheral subsystem using struct PowerReading
        telemetry INA260_Peripheral : PowerReading id 2

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ Event for failed INA260 read
        event I2cReadFailed(register_address: U8, status: I32) severity warning high \
            format "INA260 I2C read failed for register 0x{} with status {}"

        event FAIL_TO_READ_TEMP_AT(
            location: string @< The location of the sensor that failed to read
        ) \
            severity warning high \
            format "Failed to read temperature from sensor at location: {}"

        event SensorReadComplete(current_mA: F32, voltage_mV: F32, power_mW: F32) severity activity high \
            format "INA260 read complete: current {} mA, voltage {} mV, power {} mW"
            
        # @ Example port: receiving calls from the rate group
        # sync input port run: Svc.Sched

        # @ Example parameter
        # param PARAMETER_NAME: U32

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