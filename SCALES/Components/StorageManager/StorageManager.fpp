module components {

  # Defining types needed for this component
  
  # Storage status information
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
  
  # System state data from SpacecraftStateManager
  struct SystemStateData {
    state: SystemState @< Current spacecraft state
    timestamp: U32 @< Timestamp of state update
    availablePowerForStorage: F32 @< Available power allocation for storage operations (Watts)
    priorityLevel: U8 @< Current priority level for storage operations
    modeDescription: string size 64 @< Optional detailed mode description
  }
  
  # Enum for system states
  enum SystemState: U8 {
    STANDBY = 0 @< System in standby mode
    NORMAL = 1 @< System in normal operation
    SAFE = 2 @< System in safe mode
    CRITICAL = 3 @< System in critical mode
    UNKNOWN = 4 @< System state not determined
  }
  
  # Data structure for component data storage/retrieval
  struct StorageData {
    componentId: U8 @< ID of the source/destination component
    dataType: string size 32 @< Type of data being stored/retrieved
    dataSize: U32 @< Size of data in bytes
    priority: U8 @< Priority level of this data (0-255)
    timestamp: U32 @< Timestamp of data
    data: Fw.Buffer @< The actual data payload
  }
  
  # Structure for data transfer requests
  struct DataTransferRequest {
    sourceComponentId: U8 @< Source component ID
    destinationComponentId: U8 @< Destination component ID
    dataType: string size 32 @< Type of data to transfer
    priority: U8 @< Priority of transfer
    maxSizeMB: U32 @< Maximum size to transfer in MB
    timestamp: U32 @< Timestamp of request
  }

  @ Component to manage and monitor system storage resources
  active component StorageManager {
    
    ###############################################################################
    #                                 General Ports                               #
    ###############################################################################
    
    @ Array of input ports for receiving data from multiple components
    async input port dataInput: [10] components.StorageData
    
    @ Input port for receiving spacecraft state information
    async input port spacecraftStateIn: components.SystemStateData
    
    @ Output port for reporting storage status information
    output port storageStatus: components.StorageStatus
    
    @ Array of output ports for sending data to multiple components
    output port dataOutput: [10] components.StorageData
    
    ###############################################################################
    # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
    ###############################################################################
    @ Port for requesting the current time
    time get port timeCaller
    
    @ Port for sending command registrations
    command reg port cmdRegOut
    
    @ Port for receiving commands
    command recv port cmdIn
    
    @ Port for sending command responses
    command resp port cmdResponseOut
    
    @ Port for sending textual representation of events
    text event port logTextOut
    
    @ Port for sending events to downlink
    event port logOut
    
    @ Port for sending telemetry channels to downlink
    telemetry port tlmOut
    
    @ Port to return the value of a parameter
    param get port prmGetOut
    
    @ Port to set the value of a parameter
    param set port prmSetOut
    
    ###############################################################################
    #                                  Commands                                   #
    ###############################################################################
    
    @ Command to store data from a specific component
    async command STORE_DATA(
      componentId: U8 @< ID of the component storing data
      dataType: string size 32 @< Type of data being stored
      retention: U32 @< Retention period in days (0 = permanent)
    )
    
    @ Command to retrieve data for a specific component
    async command RETRIEVE_DATA(
      componentId: U8 @< ID of the component retrieving data
      dataType: string size 32 @< Type of data being retrieved
      maxItems: U16 @< Maximum number of items to retrieve (0 = all)
    )
    
    @ Command to delete data
    async command DELETE_DATA(
      componentId: U8 @< ID of the component whose data should be deleted
      dataType: string size 32 @< Type of data to delete
      olderThan: U32 @< Delete data older than this many days (0 = all)
    )
    
    @ Command to transfer data between components
    async command TRANSFER_DATA(
      sourceId: U8 @< Source component ID
      destId: U8 @< Destination component ID
      dataType: string size 32 @< Type of data to transfer
    )
    
    @ Command to sync data between components
    async command SYNC_DATA(
      componentId1: U8 @< First component ID
      componentId2: U8 @< Second component ID
      dataType: string size 32 @< Type of data to sync
      bidirectional: bool @< Whether sync should be bidirectional
    )
    
    @ Command to perform storage maintenance (defrag, integrity check)
    async command MAINTENANCE(
      level: U8 @< Maintenance level (0 = quick, 1 = standard, 2 = comprehensive)
    )
    
    @ Command to enable/disable automatic data management
    async command AUTO_MANAGE(
      enable: bool @< Whether to enable automatic data management
    )
    
    @ Command to get storage statistics for a specific component
    async command GET_COMPONENT_STATS(
      componentId: U8 @< Component ID to get stats for
    )
    
    ###############################################################################
    #                                   Events                                    #
    ###############################################################################
    
    @ Event indicating data successfully stored
    event DATA_STORED(
      componentId: U8 @< Component ID
      dataType: string size 32 @< Type of data stored
      sizeBytes: U32 @< Size of data stored in bytes
    ) severity activity high id 0 format "Data stored: {} bytes of {} from component {}"
    
    @ Event indicating data successfully retrieved
    event DATA_RETRIEVED(
      componentId: U8 @< Component ID
      dataType: string size 32 @< Type of data retrieved
      itemCount: U16 @< Number of items retrieved
    ) severity activity high id 1 format "Data retrieved: {} items of {} for component {}"
    
    @ Event indicating data successfully deleted
    event DATA_DELETED(
      componentId: U8 @< Component ID
      dataType: string size 32 @< Type of data deleted
      itemCount: U16 @< Number of items deleted
    ) severity activity high id 2 format "Data deleted: {} items of {} for component {}"
    
    @ Event indicating successful data transfer
    event DATA_TRANSFERRED(
      sourceId: U8 @< Source component ID
      destId: U8 @< Destination component ID
      dataType: string size 32 @< Type of data transferred
      sizeBytes: U32 @< Size of transferred data in bytes
    ) severity activity high id 3 format "Data transferred: {} bytes of {} from component {} to {}"
    
    @ Event indicating successful data sync
    event DATA_SYNCED(
      componentId1: U8 @< First component ID
      componentId2: U8 @< Second component ID
      dataType: string size 32 @< Type of data synced
      itemCount: U16 @< Number of items synced
    ) severity activity high id 4 format "Data synced: {} items of {} between components {} and {}"
    
    @ Event indicating storage approaching capacity
    event STORAGE_APPROACHING_CAPACITY(
      percentFull: F32 @< Percentage of storage used
      remainingMB: U32 @< Remaining storage in MB
    ) severity warning low id 5 format "Storage at {}% capacity, {}MB remaining"
    
    @ Event indicating storage critically full
    event STORAGE_CRITICALLY_FULL(
      percentFull: F32 @< Percentage of storage used
      remainingMB: U32 @< Remaining storage in MB
    ) severity warning high id 6 format "CRITICAL: Storage at {}% capacity, only {}MB remaining"
    
    @ Event indicating automatic data management action
    event AUTO_MANAGEMENT_ACTION(
      action: string size 32 @< Type of action taken
      description: string size 64 @< Description of action
    ) severity activity high id 7 format "Auto management: {} - {}"
    
    @ Event indicating storage health issue
    event STORAGE_HEALTH_ISSUE(
      issue: string size 64 @< Description of health issue
      severity: string size 16 @< Severity of issue
    ) severity warning high id 8 format "Storage health issue detected: {} ({})"
    
    ###############################################################################
    #                                 Telemetry                                   #
    ###############################################################################
    
    @ Total storage capacity in MB
    telemetry TotalStorageCapacity: U32
    
    @ Available storage space in MB
    telemetry AvailableStorage: U32
    
    @ Storage utilization percentage
    telemetry StorageUtilization: F32
    
    @ Current write rate in KB/s
    telemetry WriteRateKBps: F32
    
    @ Current read rate in KB/s
    telemetry ReadRateKBps: F32
    
    @ Number of data storage operations performed
    telemetry StoreOperations: U32
    
    @ Number of data retrieval operations performed
    telemetry RetrieveOperations: U32
    
    @ Number of data deletion operations performed
    telemetry DeleteOperations: U32
    
    @ Number of data transfer operations performed
    telemetry TransferOperations: U32
    
    @ Number of data sync operations performed
    telemetry SyncOperations: U32
    
    @ Automatic data management status
    telemetry AutoManagementEnabled: bool
    
    @ Storage health status indicator (0-100)
    telemetry StorageHealth: U8
  }
}