module scalesSvc{

    struct PowerReading {
    voltage: F32 @< Voltage reading in volts
    current: F32 @< Current reading in amps
    power: F32 @< Power consumption in watts
    sourceId: U8 @< ID of the power source/sensor
    location: string size 32 @< Description of sensor location
    timestamp: U32 @< Timestamp of reading
  }

  enum PowerModeID: U8 {
    MAX = 0 @< Unlimited power mode
    MIN = 1 @< 15 Watts power mode
    BALANCED = 2 @< 30 Watts power mode
    EXTRA = 3 @< 50 Watts power mode
  }

  struct PowerMode {
    mode: PowerModeID @< Current power mode
  }

  enum JetsonPowerStateID: U8 {
    OFF = 0 @< Jetson is powered off
    ON = 1 @< Jetson is powered on
  }

  struct JetsonPowerState {
    jetsonState: JetsonPowerStateID @< Current power state of the Jetson
  }

}
