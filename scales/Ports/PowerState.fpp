module scalesSvc{
    @ Port for receiving power state change requests (e.g., 15W, 30W, 50W)
    port PowerStateRecieve(recieve: PowerState)

    @ Port for sending current power state information
    port PowerStateSend(sendvar: PowerState)
}