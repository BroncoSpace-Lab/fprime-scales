module scalesSvc {
    @ Producer of data products for thermal and power readings
    active component DataProducer {
        
        ###############################################################################
        #                                 General Ports                               #
        ###############################################################################

        @ Port run on rate groups
        sync input port run: Svc.Sched

        @ Input port to receive McpManager thermal readings
        async input port McpThermalReadingIn: McpThermalReadings

        @ Input port to receive ImxThermalManager thermal readings
        async input port cpuThermalReadIn: CpuThermalReadings

        @ Input port to receive InaManager power readings
        async input port inaPowerReadIn: InaPowerReadings

        ###############################################################################
        #                                 DATA PRODUCTS                               #
        ###############################################################################

        @ Data product for getting temperature record from the MCP9808 temp sensors
        product record ImxTemperatureRecord: ThermalReading id 0
        product record JetsonTemperatureRecord: ThermalReading id 1
        product record PeripheralTemperatureRecord: ThermalReading id 2

        @ Data product for getting temperature record from the ImxThermalManager
        product record CpuTemperatureRecord: ThermalReading id 3

        @ Data Product for getting power record for from the InaManager
        product record ObcPowerRecord: PowerReading id 13
        product record PerifPowerRecord: PowerReading id 14
        product record JetsonPowerRecord: PowerReading id 15

        @ Data prodcut container containing temperature records
        product container McpTemperatureContainer id 0 default priority 10
        product container CpuTemperatureContainer id 1 default priority 10
        product container InaPowerContainer id 3 default priority 10

        @ Port to ask buffer manager to allocate memory for the container synchronously
        product get port productGetOut  

        @ Port to send the filled container to the data product writer
        product send port productSendOut

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}