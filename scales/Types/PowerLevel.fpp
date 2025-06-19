module scalesSvc{
    enum PowerLevel: U8 {
    WATTS_15 = 0 @< 15 Watts power state
    WATTS_30 = 1 @< 30 Watts power state
    WATTS_50 = 2 @< 50 Watts power state
    UNKNOWN = 3 @< Power state not determined
  }
}