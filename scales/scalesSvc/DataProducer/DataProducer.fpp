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

        ###############################################################################
        #                                 DATA PRODUCTS                               #
        ###############################################################################

        @ Data product for getting temperature record from the MCP9808 temp sensors
        product record ImxTemperatureRecord: ThermalReading id 0
        product record JetsonTemperatureRecord: ThermalReading id 1
        product record PeripheralTemperatureRecord: ThermalReading id 2

        @ Data prodcut container containing temperature records
        product container McpTemperatureContainer id 0 default priority 10

        @ Port to ask buffer manager to allocate memory for the container synchronously
        product get port mcpProductGetOut  

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