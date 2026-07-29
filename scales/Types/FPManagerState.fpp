module scalesSvc {

  @ Operator-visible FPManager protection state.
  enum FPManagerState: U8 {
    INIT = 0 @< FPManager has not completed its first state-machine tick
    SAFE = 1 @< Safe Mode; Jetson power-on is gated
    HPC = 2 @< HPC Mode; Jetson power-on requests are permitted
    FAULT = 3 @< Fault Mode; a non-recoverable protection fault was detected
    EMERGENCY = 4 @< Emergency Shutdown has latched; i.MX platform power is being cut
    EMERGENCY_REBOOT = 5 @< A component FATAL latched an FSW-process restart; i.MX platform power is NOT cut
  }

}
