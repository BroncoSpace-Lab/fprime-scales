module scalesSvc {
    @ ImxThermalManager to hold parameters and display IMX thermal data
    active component ImxThermalManager {

        @ Bind ThermalStateMachine to ImxThermalManager
        state machine instance thermalStateMachine: ThermalStateMachine

        @ asynchronous input port to handle incoming imx cpu temp
        async input port run: Svc.Sched

        @ output port to send imx_cpu thermal readings to DataProducer
        output port cpuThermalReadOut: CpuThermalReadings

        @ output port to send the complete i.MX thermal reading to FPManager
        output port imxThermalReadingOut: ThermalReadingPort

        @ telemetry channel for IMXCPUTEMP read
        telemetry imx_cpu_temp_read: ThermalReading \
            id 0x01

        @ Currently active i.MX CPU IDLE/WARN/FAULT bounds (a rejected update
        @ never reaches this channel -- see IMX_CPU_BOUNDS parameter).
        telemetry IMX_CPU_BOUNDS: TempBounds id 0x10

        # Default bounds for the IMX_CPU thermal states
        @ IMX CPU IDLE/WARN/FAULT temperature bounds
        param IMX_CPU_BOUNDS: TempBounds \
            default { faultLow = -40.0, warnLow = -20.0, idleLow = 10.0, idleHigh = 60.0, warnHigh = 80.0, faultHigh = 100.0 } \
            id 0x00 \
            set opcode 0x01 \
            save opcode 0x02

        event FAIL_TO_READ_TEMP(

        ) \
            severity warning high \
            id 0x01 \
            format "Failed to read temperature at IMX CPU from OSAL"

        @ The IMX CPU's newly-set IDLE/WARN/FAULT bounds are not in a sane
        @ ascending order (FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <=
        @ WARN_HIGH <= FAULT_HIGH). The update is rejected and the last-known-
        @ good bounds stay in effect -- fires on every rejected attempt, not
        @ just the first, since misconfiguration never actually takes effect.
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
