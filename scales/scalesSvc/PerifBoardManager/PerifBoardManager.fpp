module scalesSvc {
    @ Component to automate power sequencing for external ethernet switch board integrated in the SCALES custom hardware stack
    active component PerifBoardManager {

        # One async command/port is required for active components
    
        @ input port to run the manager
        async input port run: Svc.Sched

        @ output port sending calls to the GPIO driver
        output port gpioSet: Drv.GpioWrite

        @ command to set the state of the gpio
        async command powerOn(
            highLow: Fw.On
        )   @< Sets the state of the GPIO to high

        @ event to report the state of the gpio
        event gpioOn($state: Fw.On) \
            severity activity high \
            format "Peripheral Board is {}"

        telemetry gpioState: Fw.Logic @< telemetry channel to report the state of the GPIO

        param offTimeSec: U32 default 2 @< Parameter to set the time to wait before re-powering on the board


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