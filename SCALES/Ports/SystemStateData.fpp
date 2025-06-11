module SCALES{
    @ Input port for receiving spacecraft state information
    port spacecraftStateIn(rxSpace: SystemStateData)
    # async input port spacecraftStateIn: components.SystemStateData

    @ Input port for receiving system state information
    port systemStateIn(rxSystem: SystemStateData)
    # async input port systemStateIn: components.SystemStateData
}