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

    @ Port for JetsonThermalManager thermal readings data 
    port JetsonThermalReadings(
        jetson_cpuThermalReading: ThermalReading @< Thermal Reading at the Jetson CPU
        jetson_gpuTheramlReading: ThermalReading @< Thermal Reading at the Jetson GPU
        jetson_cv0ThermalReading: ThermalReading @< Thermal Reading at the Jetson cv0 zone
        jetson_cv1ThermalReading: ThermalReading @< Thermal Reading at the Jetson cv1 zone
        jetson_cv2ThermalReading: ThermalReading @< Thermal Reading at the Jetson cv2 zone
        jetson_soc0ThermalReading: ThermalReading @< Thermal Reading at the Jetson soc0 zone
        jetson_soc1ThermalReading: ThermalReading @< Thermal Reading at the Jetson soc1 zone
        jetson_soc2ThermalReading: ThermalReading @< Thermal Reading at the Jetson soc2 zone
        jetson_tjThermalReading: ThermalReading @< Thermal Reading at the Jetson TJ zone
    )

}