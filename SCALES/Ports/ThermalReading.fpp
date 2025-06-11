module SCALES{
    @ Port for receiving thermal data
    port thermalData(rxTemp: ThermalReading)
    # async input port thermalData: components.ThermalReading
}