module scalesSvc{

  enum ThermalStates: U8 {
    IDLE = 1 @< System in IDLE mode
    WARN = 2 @< System in WARN mode
    FAULT = 3 @< System in FAULT mode
    NOT_USED = 0 @< Sensor is unavailable or should not be considered
  }

  struct ThermalReading {
    temperature: F32 @< Temperature in degrees Celsius
    tempState: ThermalStates @< State of the sensor
    sensorId: U8 @< ID of the thermal sensor
    location: string size 32 @< Description of sensor location
    timestamp: U32 @< Timestamp of reading
  }

}
