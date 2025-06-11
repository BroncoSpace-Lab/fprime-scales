module SCALES{
    @ Output port for all power and thermal telemetry data
    port dataOut(data: PowerThermalStatus)
    # output port dataOut: components.PowerThermalStatus

    @ Output port for sending power/thermal status to SystemResources component
    port systemResourcesOut(hotcold: PowerThermalStatus)
    # output port systemResourcesOut: components.PowerThermalStatus
}