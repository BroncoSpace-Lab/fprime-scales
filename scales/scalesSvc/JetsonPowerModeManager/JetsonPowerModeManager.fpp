module scalesSvc {

  @ Controls configuration, enabling mode transitions and power cycle operations
  active component JetsonPowerModeManager {
    # One async command/port is required for active components
    
    ###############################################################################
    #                                 General Ports                               #
    ###############################################################################
    
    @ Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
    async input port powerModeRecieve: PowerModeReceive

    @ Port for sending current power mode information
    output port powerModeSend: PowerModeSend

    @ Port that receives the rate group tick
    sync input port schedIn: Svc.Sched
    
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
    #                                 Parameters                                  #
    ###############################################################################

    @ Requested power mode parameter
    param PWR_MODE_REQ: U8 default 0 id 0

    ###############################################################################
    #                                  Commands                                   #
    ###############################################################################
    
    @ Command to set the Jetson power mode
    async command SET_POWER_MODE(
      mode: PowerModeID @< Power mode to set (15W, 30W, or 50W)
    )

    @ Command to request current power mode
    async command GET_POWER_MODE
    
    ###############################################################################
    #                                   Events                                    #
    ###############################################################################
    
    @ Event emitted when a power mode change request arrives via the hub port (from IMX PowerManager)
    event POWER_MODE_REQUEST_RECEIVED(
      requested: PowerModeID @< The requested power mode
    ) severity activity high id 2 format "Jetson received power mode change request: {}"

    @ Event indicating power mode change successful
    event POWER_MODE_CHANGED(
      level: PowerModeID @< The new power mode
    ) severity activity high id 0 format "Jetson power mode changed to {}"

    @ Event indicating power mode change failed
    event POWER_MODE_CHANGE_FAILED(
      requested: PowerModeID @< The requested power mode
      reason: string size 64 @< Reason for failure
    ) severity warning high id 1 format "Failed to change Jetson power mode to {}: {}"
    
    ###############################################################################
    #                                 Telemetry                                   #
    ###############################################################################
    
    @ Current power mode of Jetson
    telemetry CurrentPowerMode: PowerModeID
  }
}