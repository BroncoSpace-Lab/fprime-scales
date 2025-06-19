# scalesSvc::JetsonPowerStateManager

Controls configuration, enabling state transitions and power cycle operations

## Usage Examples
The JetsonPowerStateManager component is used to control the power states of the Jetson device. It provides capabilities to turn the Jetson on/off using GPIO, set different power states (15W, 30W, 50W) by writing to a text file, and monitor the Jetson's status through ping mechanisms.

### Diagrams
```
                      ┌──────────────────────┐
                      │                      │
   pingReceive  ────► │                      │ ────►  pingSend
                      │    Jetson Power      │
powerStateReceive ──► │    State Manager     │ ────►  powerStateSend
                      │                      │
                      │                      │
                      └──────────────────────┘
```

### Typical Usage
1. The component is initialized with default power state configurations
2. The component can turn the Jetson on/off via GPIO signals
3. Power states can be changed (15W, 30W, 50W) by writing to a configuration text file on the Jetson
4. When the Jetson is awake, a ping is sent every 60 seconds to verify connectivity
5. The component monitors the Jetson's responsiveness and reports status through events and telemetry

## Class Diagram
```
```

## Port Descriptions
| Name | Description |
|---|---|
| pingReceive | Port for receiving ping requests to check if Jetson is awake |
| pingSend | Port for sending ping responses |
| powerStateReceive | Port for receiving power state change requests (e.g., 15W, 30W, 50W) |
| powerStateSend | Port for sending current power state information |
| schedIn | Port that receives the rate group "tick" for ping intervals |

## Component States
| Name | Description |
|---|---|
| PowerLevel.WATTS_15 | Jetson is in 15W power state |
| PowerLevel.WATTS_30 | Jetson is in 30W power state |
| PowerLevel.WATTS_50 | Jetson is in 50W power state |
| PowerLevel.UNKNOWN | Jetson power state is unknown or indeterminate |

## Sequence Diagrams
### Power State Change Sequence
```
┌─────┐          ┌────────────────────┐         ┌────────┐
│User │          │JetsonPowerManager  │         │ Jetson │
└──┬──┘          └──────────┬─────────┘         └───┬────┘
   │                        │                       │
   │ SET_POWER_STATE(state) │                       │
   │───────────────────────>│                       │
   │                        │                       │
   │                        │ Write to text file    │
   │                        │──────────────────────>│
   │                        │                       │
   │                        │ Update power state    │
   │                        │───────────┐           │
   │                        │           │           │
   │                        │<──────────┘           │
   │                        │                       │
   │    POWER_STATE_CHANGED │                       │
   │<───────────────────────│                       │
   │                        │                       │
```

### Ping Check Sequence
```
┌────────────────────┐         ┌────────┐
│JetsonPowerManager  │         │ Jetson │
└──────────┬─────────┘         └───┬────┘
           │                       │
           │      Ping Request     │
           │──────────────────────>│
           │                       │
           │      Ping Response    │
           │<──────────────────────│
           │                       │
           │ Update PingSuccessCount│
           │───────────┐           │
           │           │           │
           │<──────────┘           │
           │                       │
```

## Commands
| Name | Description |
|---|---|
| SET_POWER_STATE | Command to set the Jetson power state to a specific level (15W, 30W, or 50W) |
| GET_POWER_STATE | Command to request the current power state of the Jetson |
| CHECK_AWAKE | Command to check if the Jetson is awake and responsive |

## Events
| Name | Description |
|---|---|
| POWER_STATE_CHANGED | Event indicating power state change was successful |
| POWER_STATE_CHANGE_FAILED | Event indicating power state change failed |
| JETSON_AWAKE | Event indicating Jetson is awake and responding |
| JETSON_NOT_RESPONDING | Event indicating Jetson is not responding after multiple ping attempts |
| POWER_STATE_FILE_WRITE_SUCCESS | Event indicating successful write to power state file |
| POWER_STATE_FILE_WRITE_FAILED | Event indicating failed write to power state file |

## Telemetry
| Name | Description |
|---|---|
| CurrentPowerState | Current power state of Jetson (15W, 30W, 50W, or UNKNOWN) |
| PingSuccessCount | Number of successful ping operations |
| PingFailureCount | Number of failed ping operations |
| TimeSinceLastPing | Time since last successful ping (milliseconds) |

## Requirements
| Name | Description | Validation |
|---|---|---|
| JPSM-001 | The component shall turn the jetson on/off using GPIO | Unit Test |
| JPSM-002 | The component shall set power states of the Jetson by writing to a text file on the Jetson | Unit Test |
| JPSM-003 | The component shall ping its second instance every 60s when the Jetson is awake | Unit Test |
| JPSM-004 | The component shall send and receive health pings | Unit Test |
| JPSM-005 | The component shall send and receive power state changes | Unit Test |

## Change Log
| Date | Description |
|---|---|
| May 7, 2025 | Initial Draft |