module scalesSvc{
    struct PowerReading {
    voltage: F32 @< Voltage reading in volts
    current: F32 @< Current reading in amps
    power: F32 @< Power consumption in watts
    sourceId: U8 @< ID of the power source/sensor
    timestamp: U32 @< Timestamp of reading
  }
}