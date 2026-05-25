module scalesSvc {
    @ Manager to pull temperature zone data from the Jetson to forward to ThermalManager
    active component JetsonThermalManager {

         @ Synchronous input port to handle incoming jetson temp readings
        async input port jetsonTempRead: Svc.Sched

         @ telemetry channels for Jetson thermal data
        telemetry jetson_cpu_temp_read: ThermalReading \
            id 0x00
        telemetry jetson_gpu_temp_read: ThermalReading \
            id 0x01
        telemetry jetson_cv0_temp_read: ThermalReading \
            id 0x02
        telemetry jetson_cv1_temp_read: ThermalReading \   
            id 0x03
        telemetry jetson_cv2_temp_read: ThermalReading \   
            id 0x04
        telemetry jetson_soc0_temp_read: ThermalReading \
            id 0x05
        telemetry jetson_soc1_temp_read: ThermalReading \
            id 0x06
        telemetry jetson_soc2_temp_read: ThermalReading \
            id 0x07
        telemetry jetson_tj_temp_read: ThermalReading \
            id 0x08

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