module scalesSvc{
    @ Output port for all power and thermal telemetry data
    port ThermalDataOut(data: PowerThermalStatus)

    @ Output port for sending power/thermal status to SystemResources component
    port ThermalStatus(data: PowerThermalStatus)
}