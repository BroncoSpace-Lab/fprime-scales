module scalesSvc{
    struct StorageStatus {
    totalCapacityMB: U32 @< Total storage capacity in MB
    availableSpaceMB: U32 @< Available storage space in MB
    usedSpaceMB: U32 @< Used storage space in MB
    utilizationPercent: F32 @< Storage utilization percentage
    writeRateKBps: F32 @< Current write rate in KB/s
    readRateKBps: F32 @< Current read rate in KB/s
    healthStatus: string size 32 @< Storage health status description
    timestamp: U32 @< Timestamp of status update
  }
}