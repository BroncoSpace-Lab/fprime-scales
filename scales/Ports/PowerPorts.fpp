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

    @ Port Ina readings for DataProducer
    port InaPowerReadings(
        obcPowerReading: PowerReading @< Power Reading of OBC
        perifPowerReading: PowerReading @< Power Reading of Peripheral
        jetsonPowerReading: PowerReading @< Power Reading of Jetson
    )
}