module SCALES{
    @ Port for receiving power state change requests (e.g., 15W, 30W, 50W)
    port powerStateRecieve(recieve: PowerState)
    # async input port powerStateReceive: components.PowerState

    @ Port for sending current power state information
    port powerStateSend(sendvar: PowerState)
    # output port powerStateSend: components.PowerState
}