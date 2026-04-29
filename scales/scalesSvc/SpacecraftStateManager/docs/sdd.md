# scalesSvc::SpacecraftStateManager

SCALES system state manager.

## Usage Examples
Add usage examples here

### Diagrams
Add diagrams here

### Typical Usage
And the typical usage of the component here

## Requirements

| Name | Description | Validation |
|---|---|---|
|SSM-001|The `SpacecraftStateManager` component shall monitor the state of the SCALES system.|---|
|SSM-002|The `SpacecraftStateManager` component shall manager the state of the SCALES system.|---|
|SSM-003|The `SpacecraftStateManager` component shall be able to switch states through ground commands.|---|
|SSM-004|The `SpacecraftStateManager` component shall be able to switch states through requests from other SCALES components.|---|

## Class Diagram
Add a class diagram here

## Port Descriptions
| Kind | Name | Description |
|---|---|---|
|output|StateNow|Current mode/state of the system.|
|async input|stateReq|Requested mode/state of the system, either from ground command or other SCALES component.|

## Component States
Add component states in the chart below
| Name | Description | Enter Mode Triggers | Exit Mode Triggers | IMX Status | Jetson Status |
|---|---|---|---|---|---|
| Safe Mode | Comms on, `scalesSvc::PowerManager` running but shouldn't turn anything on, can collect data/telemetry but should make no decisions.|Coming out of a FATAL. | Ground command|On, reduced power mode. Maintaining health-critical and comms operations only. | Off. |
| Nominal Mode | Jetson is on but idle to perform basic file management and uplink/downlink tasks with the IMX. |---| Command to switch to HPC mode, or a FATAL error. | On. | On, but idle. |
| Minimal HPC Mode | 15W power mode on the Jetson. Able to do basic tasks while minimizing power consumption. Default HPC mode. | IMX/Ground command | `scalesSvc::JetsonPowerModeManager` component power mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, 15W power mode |
| Balanced HPC Mode | 30W power mode on the Jetson. Can complete more computationally intensive tasks than in Minimal HPC Mode, but still managing power consumption. | IMX/Ground command | `scalesSvc::JetsonPowerModeManager` component power mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in 30W power mode. |
| Extra HPC Mode | 50W power mode on the Jetson. Most performance possible while capping power consumption on the Jetson. | IMX/Ground command | `scalesSvc::JetsonPowerModeManager` component power mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in 50W power mode. |
| Maximum HPC Mode | MAXN power mode on the Jetson. Absolute maximum in both power consumption and performance. Best for intense computations or operations. | IMX/Ground command | `scalesSvc::JetsonPowerModeManager` component power mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in MAXN power mode. |
| Thermal Recovery Mode | If the Jetson got too hot while in HPC mode, Thermal Recovery Mode is triggered. Turns off the Jetson and waits for temperature to stabilize again. | Jetson temperature too hot while in HPC mode. | Jetson temperature stabilized -> return to Nominal mode. | On | Off |

![State Machine Diagram](<fprime-scales-reference-State Machine.png>)

## Sequence Diagrams
Add sequence diagrams here

## Parameters
| Name | Description |
|---|---|
|CurrentState|Current system state.|
|ReqState|Requested state to change into.|

## Commands
| Name | Description |
|---|---|
|GET_SCALES_STATE|Command to return the current state of the SCALES system.|
|SET_SCALES_STATE|Command the change the state of the SCALES system.|

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