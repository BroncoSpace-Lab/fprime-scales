module scalesSvc {
    @ Watchdog Manager for SCALES system.
    active component WatchdogManager {

        @ run handler
        async input port run: Svc.Sched

        @ gpio output port
        output port gpioWatchDog: Drv.GpioWrite

        @ telemetry channel to report the state of the watchdog
        telemetry WatchdogPet: scalesSvc.WatchdogStates

        @ parameter for watchdog pet interval, 1 second
        param watchdogPetInterval: U32 default 1

        @ event that updates everytime watchdog is pet
        event WatchdogState($state: Fw.On) \
            severity activity high \
            format "Watchdog is {}"


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