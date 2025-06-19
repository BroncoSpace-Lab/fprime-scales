module scalesSvc{
    struct PowerState {
    level: PowerLevel @< Current power level
    timestamp: U32 @< Timestamp of state
    textFilePath: string size 128 @< Path to power state config file
  }
}