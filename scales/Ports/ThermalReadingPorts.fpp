module scalesSvc{
    
    @ Port for mcpManager thermal readings data
    port McpThermalReadings(
        obcThermalReading: ThermalReading @< Thermal Reading at OBC
        perifThermalReading: ThermalReading @< Thermal Reading at Peripheral
        jetsonThermalReading: ThermalReading @< Thermal Reading at Jetson
    )

    @ Port for ImxThermalManager thermal readings data
    port CpuThermalReadings(
        cpuThermalReading: ThermalReading @< Thermal Reading at the IMX CPU
    )

}