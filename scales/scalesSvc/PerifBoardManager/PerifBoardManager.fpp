module scalesSvc {
    @ Component to automate power sequencing for external ethernet switch board integrated in the SCALES custom hardware stack
    active component PerifBoardManager {

        # One async command/port is required for active components
        # This should be overridden by the developers with a useful command/port
        @ input port tied to a rate group that keeps the GPIO toggling the ethernet switch load switch Enabled
        async input port perifBoardManager: Svc.Sched

        @ output port to send calls to the GPIO driver
        output port gpioSet: Drv.GpioWrite

        @ output port call to get GPIO state
        output port gpioGet: Drv.GpioRead

        @ telemetry channel to hold status for ethernet switch state
        telemetry perif_board_state: Drv.GpioStatus \
            id 0x00

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

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