module scalesSvc{
    struct DataTransferRequest {
    sourceComponentId: U8 @< Source component ID
    destinationComponentId: U8 @< Destination component ID
    dataType: string size 32 @< Type of data to transfer
    $priority: U8 @< Priority of transfer
    maxSizeMB: U32 @< Maximum size to transfer in MB
    timestamp: U32 @< Timestamp of request
  }
}