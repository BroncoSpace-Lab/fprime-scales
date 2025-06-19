module scalesSvc{
    struct StorageData {
    componentId: U8 @< ID of the source/destination component
    dataType: string size 32 @< Type of data being stored/retrieved
    dataSize: U32 @< Size of data in bytes
    $priority: U8 @< Priority level of this data (0-255)
    timestamp: U32 @< Timestamp of data
    data: Fw.Buffer @< The actual data payload
  }
}