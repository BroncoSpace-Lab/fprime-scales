module scalesSvc{
    struct SystemStateData {
    $state: SystemState @< Current spacecraft state
    timestamp: U32 @< Timestamp of state update
    availablePowerForStorage: F32 @< Available power allocation for storage operations (Watts)
    priorityLevel: U8 @< Current priority level for storage operations
    modeDescription: string size 64 @< Optional detailed mode description
  }
}