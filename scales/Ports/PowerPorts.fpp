module scalesSvc{
    @ Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
    port PowerModeReceive(modeReq: PowerModeID)

    @ Port for sending current power mode information
    port PowerModeSend(modeNow: PowerModeID)

    @ Port for receiving power mode change requests for the Jetson
    port JetsonPowerStateReceive(stateReq: JetsonPowerStateID)

    @ Port for receiving power data
    port PowerData(reading: PowerReading)

    @ Port for sending current power state of the Jetson
    port JetsonPowerStateSend(stateNow: JetsonPowerStateID)

    @ Synchronous authorization for a Jetson power command.
    port JetsonPowerStateAuthorize(stateReq: JetsonPowerStateID) -> Fw.Success

    @ Latched emergency power-off request for protected peripheral hardware.
    port EmergencyPowerOff
}
