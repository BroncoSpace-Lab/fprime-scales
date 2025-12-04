# scalesSvc::PowerManager

The `scalesSvc::PowerManager` component parameters, tracks operational states, and responds to critical conditions to ensure system safety and stability. This component also manages the power for each board in the SCALES system.

## Usage Examples
Add usage examples here

## Requirements
Add requirements in the chart below
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
|---|---|

## Events
| Name | Description |
|---|---|
|---|---|

## Telemetry
| Name | Description |
|---|---|
|---|---|

## Unit Tests
Add unit test descriptions in the chart below
| Name | Description | Output | Coverage |
|---|---|---|---|
|---|---|---|---|

## Change Log
| Date | Description |
|---|---|
|---| Initial Draft |