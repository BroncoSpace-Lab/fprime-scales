module scalesSvc {
    @ Manager to pull temperature zone data from the Jetson to forward to ThermalManager
    active component JetsonThermalManager {

         @ Synchronous input port to handle incoming jetson temp readings
        async input port jetsonTempRead: Svc.Sched

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