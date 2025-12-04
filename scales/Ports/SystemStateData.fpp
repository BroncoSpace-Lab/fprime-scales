module scalesSvc{
    @ Input port for receiving spacecraft state information
    port SCALESstate(rxSpace: SystemStateData)

    @ Input port for receiving system state information
    port RequestedState(rxSystem: SystemStateData)
}