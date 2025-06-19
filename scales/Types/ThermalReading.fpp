module scalesSvc{
    struct ThermalReading {
    temperature: F32 @< Temperature in degrees Celsius
    sensorId: U8 @< ID of the thermal sensor
    location: string size 32 @< Description of sensor location
    timestamp: U32 @< Timestamp of reading
  }
}