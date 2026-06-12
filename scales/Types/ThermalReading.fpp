module scalesSvc{
    struct ThermalReading {
    temperature: F32 @< Temperature in degrees Celsius
    tempState: ThermalStates @< Thermal state of device
    sensorId: U8 @< ID of the thermal sensor
    location: string size 32 @< Description of sensor location
    timestamp: U32 @< Timestamp of reading
  }
   enum ThermalStates: U8 {
        IDLE = 1 @< System in IDLE mode
        WARN =  2@< System in WARN mode
        FAULT = 3 @< System in FAULT mode
    }
  
}