module scalesSvc{
    enum ThermalStates: U8 {
        IDLE = 0 @< System in IDLE mode
        WARN = 1 @< System in WARN mode
        FAULT = 2 @< System in FAULT mode
    }
  }
  