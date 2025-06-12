# components::PowerThermalManager

Component to manage and monitor power and thermal conditions of the system

## Usage Examples
The PowerThermalManager component monitors the thermal and power conditions of the spacecraft system. It tracks operational states, manages power transitions, and ensures the system operates within safe thermal and power limits.

### Diagrams
```
                   ┌──────────────────────┐
                   │                      │
   powerData  ───► │                      │ ────►  dataOut
                   │    Power Thermal     │
  thermalData ───► │    Manager           │ ────►  systemState
                   │                      │
  systemState ───► │                      │
                   │                      │
                   └──────────────────────┘
```

### Typical Usage
1. The component continuously receives power and thermal data from various sensors
2. It tracks the operational states of the system and handles power transitions accordingly
3. When the system changes state, the component adjusts power usage accordingly
4. The component monitors if thermal or power readings are outside of configured limits
5. If values exceed warning or critical thresholds, the component generates appropriate events
6. When values are out of range, the component notifies the System Resources component
7. The component provides comprehensive telemetry about the system's power and thermal status

## Class Diagram
```
```

## Port Descriptions
| Name | Description |
|---|---|
| powerData | Port for receiving power data |
| thermalData | Port for receiving thermal data |
| systemStateIn | Input port for receiving system state information |
| dataOut | Output port for all power and thermal telemetry data |
| systemResourcesOut | Output port for sending power/thermal status to SystemResources component |

## Component States
| Name | Description |
|---|---|
| SystemState.STANDBY | System is in standby mode with minimal power consumption |
| SystemState.NORMAL | System is in normal operational state |
| SystemState.SAFE | System is in safe mode due to potential issues |
| SystemState.CRITICAL | System is in critical mode due to severe conditions |
| SystemState.UNKNOWN | System state has not been determined or is transitioning |

## Sequence Diagrams
### Power/Thermal Monitoring Sequence
```
┌───────────┐     ┌────────────────────┐     ┌──────────────────┐
│  Sensors  │     │PowerThermalManager │     │ SystemResources  │
└─────┬─────┘     └──────────┬─────────┘     └────────┬─────────┘
      │                      │                        │
      │  Power/Thermal Data  │                        │
      │─────────────────────>│                        │
      │                      │                        │
      │                      │  Check against limits  │
      │                      │──────────┐             │
      │                      │          │             │
      │                      │<─────────┘             │
      │                      │                        │
      │                      │ If outside limits      │
      │                      │───────────────────────>│
      │                      │                        │
      │                      │  Trigger Events        │
      │                      │──────────┐             │
      │                      │          │             │
      │                      │<─────────┘             │
      │                      │                        │
```

### System State Change Sequence
```
┌───────────┐     ┌────────────────────┐     ┌──────────────────┐
│    SSM    │     │PowerThermalManager │     │ SystemResources  │
└─────┬─────┘     └──────────┬─────────┘     └────────┬─────────┘
      │                      │                        │
      │  System State Change │                        │
      │─────────────────────>│                        │
      │                      │                        │
      │                      │  Adjust Power          │
      │                      │──────────┐             │
      │                      │          │             │
      │                      │<─────────┘             │
      │                      │                        │
      │                      │ Power Transition Event │
      │                      │───────────────────────>│
      │                      │                        │
      │                      │ Updated Power/Thermal  │
      │                      │───────────────────────>│
      │                      │                        │
```

## Commands
| Name | Description |
|---|---|
| UPDATE_STATUS | Command to force a full power/thermal status update |
| SET_THERMAL_LIMITS | Command to set thermal limit thresholds (warning and critical levels) |
| SET_POWER_LIMITS | Command to set power limit thresholds (warning and critical levels) |

## Events
| Name | Description |
|---|---|
| THERMAL_WARNING_HIGH | Event indicating thermal upper warning threshold exceeded |
| THERMAL_WARNING_LOW | Event indicating thermal lower warning threshold exceeded |
| THERMAL_CRITICAL_HIGH | Event indicating thermal upper critical threshold exceeded |
| THERMAL_CRITICAL_LOW | Event indicating thermal lower critical threshold exceeded |
| POWER_WARNING_HIGH | Event indicating power upper warning threshold exceeded |
| POWER_WARNING_LOW | Event indicating power lower warning threshold exceeded |
| POWER_CRITICAL_HIGH | Event indicating power upper critical threshold exceeded |
| POWER_CRITICAL_LOW | Event indicating power lower critical threshold exceeded |
| LIMITS_UPDATED | Event indicating thermal or power limits have been updated |
| POWER_OUT_OF_RANGE | Event indicating power is outside normal operating range |
| THERMAL_OUT_OF_RANGE | Event indicating temperature is outside normal operating range |
| POWER_TRANSITION | Event indicating power has changed due to system state transition |

## Telemetry
| Name | Description |
|---|---|
| MaxTemperature | Maximum temperature currently detected across all sensors |
| MinTemperature | Minimum temperature currently detected across all sensors |
| AvgTemperature | Average temperature calculated from all thermal sensors |
| TotalPower | Total power consumption in watts across the system |
| ThermalUpperWarningThreshold | Current thermal upper warning threshold |
| ThermalLowerWarningThreshold | Current thermal lower warning threshold |
| ThermalUpperCriticalThreshold | Current thermal upper critical threshold |
| ThermalLowerCriticalThreshold | Current thermal lower critical threshold |
| PowerUpperWarningThreshold | Current power upper warning threshold |
| PowerLowerWarningThreshold | Current power lower warning threshold |
| PowerUpperCriticalThreshold | Current power upper critical threshold |
| PowerLowerCriticalThreshold | Current power lower critical threshold |
| CurrentSystemState | Current operational state of the system |

## Requirements
| Name | Description | Validation |
|---|---|---|
| PTM-001 | The component shall monitor thermal and power of the system | Unit Test |
| PTM-002 | The component shall track operational states and handle power transitions accordingly | Unit Test |
| PTM-003 | The power and thermal limits can be set to different values and persist across reboot cycles | Unit Test |
| PTM-004 | If power/thermal is out of range, the component will notify the System Resources | Unit Test |
| PTM-005 | The component shall send and receive power state changes | Unit Test |
| PTM-006 | The component shall receive power input | Unit Test |
| PTM-007 | The component shall receive thermal input | Unit Test |
| PTM-008 | The component shall receive system state changes | Unit Test |
| PRM-009 | The system shall output power and thermal data | Unit Test |
| PRM-010 | The component shall notify the System Resources if thermal/power are out of normal operating limits | Unit Test |

## Change Log
| Date | Description | Author |
|---|---|---|
| May 7, 2025 | Initial Draft | Janelle Tran |