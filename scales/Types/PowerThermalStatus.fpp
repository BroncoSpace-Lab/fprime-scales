module scalesSvc{
    struct PowerThermalStatus {
    powerReadings: PowerReading @< Power readings
    thermalReadings: ThermalReading @< Thermal readings
    systemRecommendation: SystemState @< Recommended system state based on power/thermal
    criticalFlag: bool @< Flag indicating if any readings are in critical range
    timestamp: U32 @< Timestamp of status update
  }
}
