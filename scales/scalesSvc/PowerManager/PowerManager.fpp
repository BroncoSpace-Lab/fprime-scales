module scalesSvc {
    @ Component to manage and monitor system power distribution.
    active component PowerManager {

        ###############################################################################
        #                                 General Ports                               #
        ###############################################################################

        @ Port for sending power mode change requests to JetsonPowerModeManager
        output port reqPwrMode: PowerModeRecieve

        @ Port for receiving current power mode from JetsonPowerModeManager
        async input port currentPwrMode: PowerModeSend

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

        ###############################################################################
        #                                 Telemetry                                   #
        ###############################################################################

        @ Current power mode of the Jetson as reported by JetsonPowerModeManager
        telemetry JetsonPowerMode: PowerModeID

    }
}
