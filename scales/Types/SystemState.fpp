  module scalesSvc{
    enum SystemState: U8 {
    STANDBY = 0 @< System in standby mode
    NORMAL = 1 @< System in normal operation
    SAFE = 2 @< System in safe mode
    CRITICAL = 3 @< System in critical mode
    UNKNOWN = 4 @< System state not determined
    }
  }
  