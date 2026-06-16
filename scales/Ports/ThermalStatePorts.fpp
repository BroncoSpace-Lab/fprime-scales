module scalesSvc{
    @ Input port for receiving theral state information
    port ThermalStateIn(rxThermalState: ThermalStates)

    @ Output port for sending thermal state information
    port ThermalStateOut(txThermalState: ThermalStates)
}