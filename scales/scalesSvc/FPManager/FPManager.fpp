module scalesSvc {
    @ Fault Protection Manager for the SCALES system.
    active component FPManager {

        @ Bind the FP protection state machine to FPManager.
        state machine instance fpStateMachine: FPStateMachine

        ##############################################################################
        #### State-machine and protection inputs/outputs #############################
        ##############################################################################

        @ Rate-group input that drives initialization and health checks.
        async input port run: Svc.Sched

        @ Immediate fatal announcement input for the terminal shutdown path.
        sync input port fatalIn: Svc.FatalEvent

        @ Forwards the fatal announcement to the standard process-level handler.
        output port fatalOut: Svc.FatalEvent

        @ Latest i.MX thermal reading.
        async input port imxThermalReadingIn: ThermalReadingPort

        @ Latest peripheral thermal reading.
        async input port peripheralThermalReadingIn: ThermalReadingPort

        @ Complete local MCP readings. Sensor IDs 1 and 2 represent i.MX and
        @ peripheral readings respectively; the Jetson-board reading is ignored
        @ here because Jetson die aggregation is handled by Jetson readings.
        async input port mcpThermalReadingIn: ThermalReadingPort

        @ Jetson thermal readings. All nine Jetson sensors share this input and are
        @ aggregated by sensorId inside FPManager.
        async input port jetsonThermalReadingIn: ThermalReadingPort

        @ Synchronous gate called by JetsonManager before it executes the command.
        sync input port jetsonPowerAuthorizeIn: JetsonPowerStateAuthorize

        @ Internal recovery and emergency power request sent to JetsonManager.
        output port jetsonPowerRequestOut: JetsonPowerStateReceive

        @ Emergency power-off request for the peripheral board.
        output port peripheralPowerOff: EmergencyPowerOff

        @ Current Jetson power state used by health checks.
        async input port jetsonPowerStateIn: JetsonPowerStateSend

        ##############################################################################
        #### Operator commands #######################################################
        ##############################################################################

        @ Enable HPC Mode. Jetson power-on requests are gated until this succeeds.
        async command ENABLE_HPC_MODE opcode 0x00

        @ Disable HPC Mode and return to Safe Mode. The Jetson is powered off.
        async command DISABLE_HPC_MODE opcode 0x01

        ##############################################################################
        #### Events and telemetry ####################################################
        ##############################################################################

        @ Jetson power request rejected because FPManager is not in HPC Mode.
        event JETSON_POWER_REQUEST_REJECTED(
            requested: JetsonPowerStateID
            reason: string size 64
        ) severity warning high id 0x00 \
            format "Jetson power request {} rejected by FPManager: {}"

        @ Thermal or power fault identified by FPManager.
        event FAULT_DETECTED(
            source: string size 32
            sensorId: U8
            temperature: F32
            thermalState: ThermalStates
            location: string size 32
            timestamp: U32
        ) severity warning high id 0x01 \
            format "Fault detected in {} sensor {} at {} C state {} location {} timestamp {}"

        @ High-priority emergency shutdown event.
        event EMERGENCY_SHUTDOWN severity warning high id 0x02 \
            format "FPManager emergency shutdown asserted"

        @ Current FP state for downlink and diagnostics.
        telemetry FP_STATE: FPManagerState id 0x00

        @ Number of Jetson thermal readings currently held by FPManager.
        telemetry JETSON_VALID_READING_COUNT: U8 id 0x01

        ##############################################################################
        #### Uncomment the following examples to start customizing your component ####
        ##############################################################################

        # @ Example async command
        # async command COMMAND_NAME(param_name: U32)

        # @ Example telemetry counter
        # telemetry ExampleCounter: U64

        # @ Example event
        # event ExampleStateEvent(example_state: Fw.On) severity activity high id 0 format "State set to {}"

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
