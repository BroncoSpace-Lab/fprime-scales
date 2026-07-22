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
        # Telemetry to show the parameters of the Jetson's thermal zones  #
        ###############################################################################

        @ IDLE Low temperature threshold
        param JETSON_IDLE_LOW: F32 \
            default 10 \
            id 0x00 \ 
            set opcode 0x01 \
            save opcode 0x02

        @ IDLE High temperature threshold
        param JETSON_IDLE_HIGH: F32 \
            default 60 \
            id 0x01 \ 
            set opcode 0x03 \
            save opcode 0x04
        
        @ WARNING Low temperature threshold
        param JETSON_WARN_LOW: F32 \
            default -20 \
            id 0x02 \ 
            set opcode 0x05 \
            save opcode 0x06

        @ WARNING High temperature threshold
        param JETSON_WARN_HIGH: F32 \
            default 80 \   
            id 0x03 \ 
            set opcode 0x07 \
            save opcode 0x08
        
        @ FAULT Low temperature threshold
        param JETSON_FAULT_LOW: F32 \
            default -40 \
            id 0x04 \ 
            set opcode 0x09 \
            save opcode 0x10
        
        @ FAULT High temperature threshold
        param JETSON_FAULT_HIGH: F32 \
            default 100 \
            id 0x05 \ 
            set opcode 0x11 \
            save opcode 0x12
        
        @ Telmetry for IDLE state low threshold
        telemetry JETSON_IDLE_LOW: F32 id 0x16

        @ Telmetry for IDLE state high threshold
        telemetry JETSON_IDLE_HIGH: F32 id 0x17

        @ Telmetry for WARNING state low threshold
        telemetry JETSON_WARN_LOW: F32 id 0x18  

        @ Telmetry for WARNING state high threshold
        telemetry JETSON_WARN_HIGH: F32 id 0x19

        @ Telmetry for FAULT state low threshold
        telemetry JETSON_FAULT_LOW: F32 id 0x1a

        @ Telmetry for FAULT state high threshold
        telemetry JETSON_FAULT_HIGH: F32 id 0x1b

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
