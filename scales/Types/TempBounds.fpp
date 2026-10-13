module scalesSvc {

  @ Bundles the six IDLE/WARN/FAULT temperature thresholds for one thermal
  @ domain into a single struct, so they are configured and displayed as one
  @ parameter / one telemetry row (ascending left to right) instead of six.
  struct TempBounds {
    faultLow: F32  @< FAULT low temperature threshold
    warnLow: F32   @< WARNING low temperature threshold
    idleLow: F32   @< IDLE low temperature threshold
    idleHigh: F32  @< IDLE high temperature threshold
    warnHigh: F32  @< WARNING high temperature threshold
    faultHigh: F32 @< FAULT high temperature threshold
  }

}
