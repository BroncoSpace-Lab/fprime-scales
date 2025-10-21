module scalesSvc{
    @ Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
    port PowerModeRecieve(modeReq: PowerModeID)

    @ Port for sending current power mode information
    port PowerModeSend(modeNow: PowerModeID)

    @ Port for receiving power data
    port PowerData(reading: PowerReading)
}