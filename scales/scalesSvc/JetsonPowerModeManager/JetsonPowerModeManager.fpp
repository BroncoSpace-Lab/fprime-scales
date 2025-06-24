module scalesSvc {

  @ Controls configuration, enabling mode transitions and power cycle operations
  active component JetsonPowerModeManager {
    # One async command/port is required for active components
    
    ###############################################################################
    #                                 General Ports                               #
    ###############################################################################
    
    @ Port for receiving ping requests to check if Jetson is awake
    async input port pingReceive: Svc.Ping
    
    @ Port for sending ping responses 
    output port pingSend: Svc.Ping
    
    @ Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
    async input port powerStateRecieve: PowerModeRecieve
    
    @ Port for sending current power mode information
    output port powerStateSend: PowerModeSend

    @ Port that receives the rate group "tick" for ping intervals
    async input port schedIn: Svc.Sched
    
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
    
    @Port to set the value of a parameter
    param set port prmSetOut
    
    ###############################################################################
    #                                  Commands                                   #
    ###############################################################################
    
    @ Command to set the Jetson power mode
    async command SET_POWER_MODE(
      $state: PowerModeID @< Power mode to set (15W, 30W, or 50W)
    )
    
    @ Command to request current power mode
    async command GET_POWER_MODE
    
    @ Command to check if Jetson is awake
    async command CHECK_AWAKE
    
    ###############################################################################
    #                                   Events                                    #
    ###############################################################################
    
    @ Event indicating power mode change successful
    event POWER_MODE_CHANGED(
      level: PowerModeID @< The new power mode
    ) severity activity high id 0 format "Jetson power mode changed to {}"
    
    @ Event indicating power mode change failed
    event POWER_STATE_CHANGE_FAILED(
      requested: PowerMode @< The requested power mode
      reason: string size 64 @< Reason for failure
    ) severity warning high id 1 format "Failed to change Jetson power mode to {}: {}"
    
    @ Event indicating Jetson is awake
    event JETSON_AWAKE severity activity high id 2 format "Jetson is awake and responding"
    
    @ Event indicating Jetson is not responding
    event JETSON_NOT_RESPONDING(
      attempts: U32 @< Number of ping attempts
    ) severity warning high id 3 format "Jetson not responding after {} ping attempts"

    @ Event indicating successful write to power mode file
    event POWER_MODE_FILE_WRITE_SUCCESS(
      path: string size 128 @< Path to the file written
    ) severity activity high id 4 format "Successfully wrote power mode to file: {}"

    @ Event indicating failed write to power mode file
    event POWER_STATE_FILE_WRITE_FAILED(
      path: string size 128 @< Path to the file that failed to write
    ) severity warning high id 5 format "Failed to write power mode to file: {}"
    
    ###############################################################################
    #                                 Telemetry                                   #
    ###############################################################################
    
    @ Current power mode of Jetson
    telemetry CurrentPowerState: PowerMode
    
    @ Number of successful ping operations
    telemetry PingSuccessCount: U32
    
    @ Number of failed ping operations
    telemetry PingFailureCount: U32
    
    @ Time since last successful ping (milliseconds)
    telemetry TimeSinceLastPing: U32
  }
}