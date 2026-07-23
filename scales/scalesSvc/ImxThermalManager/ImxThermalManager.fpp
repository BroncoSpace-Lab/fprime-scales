module scalesSvc {
    @ ImxThermalManager to hold parameters and display IMX thermal data
    active component ImxThermalManager {

        @ Bind ThermalStateMachine to ImxThermalManager
        state machine instance thermalStateMachine: ThermalStateMachine

        @ asynchronous input port to handle incoming imx cpu temp
        async input port run: Svc.Sched

        @ output port to send imx thermal state to spacecraft state machine
        output port imxThermalStateOut: ThermalStateOut

        @ output port to send imx_cpu thermal readings to DataProducer
        output port cpuThermalReadOut: CpuThermalReadings
       
       @ telemetry channel for imx thermal state
        telemetry imx_thermal_state: ThermalStates \
            id 0x00
       
        @ telemetry channel for IMXCPUTEMP read
        telemetry imx_cpu_temp_read: ThermalReading \
            id 0x01

        # Default Parameter bounds for IMX_CPU States
        @ IMX_CPU_IDLE_LOW Parameter 
        param IMX_CPU_IDLE_LOW: F32 \
            default 10.0 \
            id 0x00 \ 
            set opcode 0x01 \   
            save opcode 0x02
        
        @ IMX_CPU_IDLE_HIGH Parameter 
        param IMX_CPU_IDLE_HIGH: F32 \
            default 60.0 \
            id 0x01 \ 
            set opcode 0x03 \
            save opcode 0x04

        @ IMX_CPU_WARN_LOW Parameter 
        param IMX_CPU_WARN_LOW: F32 \
            default -20.0 \
            id 0x02 \ 
            set opcode 0x05 \
            save opcode 0x06

        @ IMX_CPU_WARN_HIGH Parameter 
        param IMX_CPU_WARN_HIGH: F32 \
            default 80.0 \
            id 0x03 \ 
            set opcode 0x07 \
            save opcode 0x08
        
        @ IMX_CPU_FAULT_LOW Parameter 
        param IMX_CPU_FAULT_LOW: F32 \
            default -40.0 \
            id 0x04 \ 
            set opcode 0x09 \
            save opcode 0x10

        @ IMX_CPU_FAULT_HIGH Parameter 
        param IMX_CPU_FAULT_HIGH: F32 \
            default 100.0 \
            id 0x05 \ 
            set opcode 0x11 \
            save opcode 0x12
        
        event FAIL_TO_READ_TEMP(

        ) \
            severity warning high \
            id 0x01 \
            format "Failed to read temperature at IMX CPU from OSAL"

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