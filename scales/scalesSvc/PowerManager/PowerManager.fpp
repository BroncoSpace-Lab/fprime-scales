module scalesSvc {
    @ Component to manage and monitor system power distribution.
    active component PowerManager {

        ###############################################################################
        #                                 General Ports                               #
        ###############################################################################

        @ Port for sending power mode change requests to JetsonPowerModeManager
        output port reqPwrMode: PowerModeReceive

        @ Port for receiving current power mode from JetsonPowerModeManager
        async input port currentPwrMode: PowerModeSend

        @ Port for sending Jetson Power on/off requests to JetsonPowerModeManager
        output port reqJetsonPwrState: JetsonPowerStateReceive

        @ Port for receiving current Jetson power state from JetsonPowerModeManager
        async input port currentJetsonPwrState: JetsonPowerStateSend

        @ Port for driving GPIO to control hardware power
        output port gpioSet: Drv.GpioWrite

        @ Port that receives the rate group tick
        sync input port schedIn: Svc.Sched

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

        @ Port to set the value of a parameter
        param set port prmSetOut

        ###############################################################################
        #                                  Commands                                   #
        ###############################################################################

        @ Command to request a power mode change on the Jetson
        async command REQUEST_POWER_MODE(
            mode: PowerModeID @< Requested power mode
        )

        @ Command to request a power state change (on/off) for the Jetson
        async command REQUEST_JETSON_POWER_STATE(
            jetsonState: JetsonPowerStateID @< Requested power state (on/off)
        )

        ###############################################################################
        #                                   Events                                    #
        ###############################################################################

        @ Event indicating a power mode change was requested
        event POWER_MODE_REQUESTED(
            mode: PowerModeID @< The requested power mode
        ) severity activity high id 0 format "PowerManager requested Jetson power mode {}"

        @ Event indicating the current power mode was received
        event POWER_MODE_RECEIVED(
            mode: PowerModeID @< The current power mode reported by Jetson
        ) severity activity low id 1 format "PowerManager received Jetson power mode {}"

        @ Event indicating a Jetson power state change was requested
        event JETSON_POWER_STATE_REQUESTED(
            jetsonState: JetsonPowerStateID @< The requested Jetson power state (on/off)
        ) severity activity high id 2 format "PowerManager requested Jetson power state change to {}"

        @ Event indicating the current Jetson power state was received
        event JETSON_POWER_STATE_RECEIVED(
            jetsonState: JetsonPowerStateID @< The current Jetson power state reported by Jetson
        ) severity activity low id 3 format "PowerManager received Jetson power state {}"

        @Event indicating a Jetson power commmand timed out
        event JETSON_POWER_STATE_TIMEOUT(
            jetsonState: JetsonPowerStateID @< The Jetson power state that timed out
        ) severity warning high id 4 format "PowerManager timed out waiting for Jetson power state response to command {}"

        ###############################################################################
        #                                 Telemetry                                   #
        ###############################################################################

        @ Current power mode of the Jetson as reported by JetsonPowerModeManager
        telemetry JetsonPowerMode: PowerModeID

        @ Current power state of the Jetson
        telemetry JetsonPowerState: JetsonPowerStateID

    }
}
