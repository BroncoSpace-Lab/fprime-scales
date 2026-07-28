module scalesSvc {
    @ Manager to pull temperature zone data from the Jetson to forward to ThermalManager
    active component JetsonThermalManager {

        @ Bind the ThermalStateMachine to JetsonThermalManager
        state machine instance jetson_thermalStateMachine: ThermalStateMachine

         @ Synchronous input port to handle incoming jetson temp readings
        async input port run: Svc.Sched

        @ Output the complete reading for each Jetson sensor to the FPManager.
        output port jetsonThermalReadingOut: ThermalReadingPort

        @ telemetry channel for Jetson CPU temp data
        telemetry jetson_cpu_temp_read: ThermalReading \
            id 0x00

        @ telemetry channel for Jetson GPU temp data
        telemetry jetson_gpu_temp_read: ThermalReading \
            id 0x01

        @ telemetry channel for Jetson CV0 temp data
        telemetry jetson_cv0_temp_read: ThermalReading \
            id 0x02

        @ telemetry channel for Jetson CV1 temp data
        telemetry jetson_cv1_temp_read: ThermalReading \
            id 0x03

        @ telemetry channel for Jetson CV2 temp data
        telemetry jetson_cv2_temp_read: ThermalReading \
            id 0x04

        @ telemetry channel for Jetson SOC0 temp data
        telemetry jetson_soc0_temp_read: ThermalReading \
            id 0x05

        @ telemetry channel for Jetson SOC1 temp data
        telemetry jetson_soc1_temp_read: ThermalReading \
            id 0x06

        # telemetry channel for Jetson SOC2 temp data
        telemetry jetson_soc2_temp_read: ThermalReading \
            id 0x07

        # telemetry channel for Jetson TJ temp data
        telemetry jetson_tj_temp_read: ThermalReading \
            id 0x08

        ###############################################################################
        # Bounds shared by all nine Jetson zones (they're all on one die)             #
        ###############################################################################

        @ Currently active Jetson IDLE/WARN/FAULT bounds, shared across all
        @ nine zones (a rejected update never reaches this channel -- see
        @ JETSON_BOUNDS parameter).
        telemetry JETSON_BOUNDS: TempBounds id 0x16

        @ Jetson IDLE/WARN/FAULT temperature bounds, shared across all nine
        @ on-die thermal zones
        param JETSON_BOUNDS: TempBounds \
            default { faultLow = -40.0, warnLow = -20.0, idleLow = 10.0, idleHigh = 60.0, warnHigh = 80.0, faultHigh = 100.0 } \
            id 0x00 \
            set opcode 0x01 \
            save opcode 0x02

        ###############################################################################
        #                                 Events                                      #
        ###############################################################################

        @ The Jetson's newly-set IDLE/WARN/FAULT bounds are not in a sane
        @ ascending order (FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <=
        @ WARN_HIGH <= FAULT_HIGH). The update is rejected and the last-known-
        @ good bounds stay in effect for all nine zones -- fires on every
        @ rejected attempt, not just the first, since misconfiguration never
        @ actually takes effect.
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
            id 0x00 \
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
