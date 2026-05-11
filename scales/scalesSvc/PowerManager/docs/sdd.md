# scalesSvc::PowerManager

The `scalesSvc::PowerManager` monitors system parameters, tracks operational states, and responds to critical conditions to ensure system safety and stability. This component also manages the power for each board in the SCALES system.

## Usage Examples

The `scalesSvc::PowerManager` component will interface with the `scalesSvc::JetsonPowerModeManager` to oversee the monitoring and changing of power modes on the Jetson. Since the Jetson must reset after a new power mode has been requested, `scalesSvc::PowerModeManager` will request the power mode change, and monitor the Jetson until it wakes up to validate the successful changing of the power mode.

## Requirements

| Name | Description | Validation |
|---|---|---|
| PM-001 | The component shall monitor power levels and consumption across the SCALES system. |---|
| PM-002 | The component shall issue a fault when predefined critical thresholds are exceeded. |---|
| PM-003 | The component shall request power mode changes from `scalesSvc::JetsonPowerModeManager` and monitor the succes or failure of that request. |---|
| PM-005 | The component shall be able to control power to hardware in the SCALES system, turning hardware on and off. |---|

### Diagrams
Add diagrams here

### Typical Usage
And the typical usage of the component here

## Class Diagram
Add a class diagram here

## Port Descriptions
| Kind | Name | Description |
|---|---|---|
| output | reqPwrMode | Port for sending power mode change requests to JetsonPowerModeManager |
| async input | currentPwrMode | Port for receiving current power mode from JetsonPowerModeManager |
| async input | powerData | Port for receiving power data |
| async input | systemStateIn | Input port for receiving system state information |
| output | pwrTlm | Output port for all power telemetry data |
| output | gpioSet | Port for driving the GPIO of the Jetson HIGH to turn it on |

## Component States
Add component states in the chart below

State machine will either be implemented here or in `scalesSvc::SpacecraftStateManager`. See the [SpacecraftstateManager sdd](https://github.com/BroncoSpace-Lab/fprime-scales/tree/kellydev/scales/scalesSvc/SpacecraftStateManager) for more information on the state machine.

| Name | Description |
|---|---|
| Nominal | IMX is on without limitations and Jetson is idle, not using its GPU to reduce power consumption. |
| HPC | High Performance Computing (HPC) mode is when the Jetson is on and using its GPU, defaulting into Minimal (15W) power mode. |

These power states will be further defined as we continue power testing on the IMX and the Jetson with our custom hardware configuration.

## Sequence Diagrams
Add sequence diagrams here

## Parameters
| Name | Description |
|---|---|
|---|---|

## Commands
| Name | Description |
|---|---|
| REQUEST_POWER_MODE | Command to request a power mode change from the Jetson. This command will send a request to change power modes using the `reqPwrMode` port, which will trigger `scalesSvc::JetsonPowerModeManager` to change modes. |

## Events
| Name | Description |
|---|---|
| POWER_MODE_REQUESTED | Event indicating a Jetson power mode change was requested. |
| POWER_MODE_RECEIVED | Event indicating the current power mode was received. |

## Telemetry
| Name | Description |
|---|---|
| JetsonPowerMode | Current power mode of the Jetson as reported by `scalesSvc::JetsonPowerModeManager` |

## Unit Tests
Add unit test descriptions in the chart below
| Name | Description | Output | Coverage |
|---|---|---|---|
|---|---|---|---|

## Change Log
| Date | Description |
|---|---|
| 4/27/2026 | JetsonPowerModeManager compatibility implementation. |