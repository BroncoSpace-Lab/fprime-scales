module scalesSvc{
    @ Input port for receiving spacecraft state information
    port SpacecraftStateIn(rxSpace: SystemStateData)

    @ Input port for receiving system state information
    port SystemStateIn(rxSystem: SystemStateData)
}