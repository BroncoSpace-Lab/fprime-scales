# scalesSvc::FPManager

Fault Protection Manager for the SCALES system. FPManager owns the system-level
flight-processor protection state machine and gates Jetson power requests based
on that state.

This document records the implemented component design, interfaces, state
transitions, and verification status.

## Design Summary

FPManager receives complete `ThermalReading` values from the thermal managers.
It evaluates the `tempState` field, while retaining the complete reading so a
fault report can identify the sensor (`sensorId`, `location`, and timestamp)
that caused the transition.

Jetson readings are aggregated in FPManager because the sensors share the same
die. A Jetson fault is asserted when any valid Jetson reading has
`ThermalStates.FAULT`; JetsonThermalManager is not responsible for this
system-level aggregation.

The Jetson is required to be powered off in Safe Mode. A Jetson power-on
request is accepted only in HPC Mode. Power-off requests remain available for
protection and recovery actions.

`$fatal` is an emergency override. FPManager performs the one-shot protection
action before forwarding the fatal event to the standard fatal handler. The
action emits the high-priority shutdown event, synchronously requests Jetson
and peripheral power removal, and latches `emergencyShutdown`. It does not
transition through `faultMode` and has no recovery transition; the standard
fatal handler then performs the process/i.MX shutdown and the satellite power
cycle resets the system.

## Usage Examples

The intended lifecycle is:

```text
initial -> init --first tick--> safeMode --hpcMode_en--> hpcMode
                                  |                    |
                                  |                    +--Jetson fault--> jetsonFaultRecovery
                                  +--fault------------> faultMode
```

Any state can receive `$fatal` and enter `emergencyShutdown`.

### Diagrams

The state machine is defined in `FPStateMachine.fpp`. The Jetson fault-recovery
and emergency-shutdown sequences below describe the component-level behavior.

### Typical Usage

1. On the first scheduler tick, initialize FPManager in Safe Mode and request
   Jetson power OFF.
2. In Safe Mode, evaluate i.MX and peripheral readings and keep Jetson power
   off.
3. After a Safe Mode health check passes, `ENABLE_HPC_MODE` may transition to
   HPC Mode. HPC Mode evaluates i.MX, peripheral, and Jetson readings on each
   tick; Jetson ON commands remain gated by the resulting state.
4. If only the Jetson is faulty, record the offending full reading, power off
   the Jetson, report the cause, and return to Safe Mode.
5. If i.MX or peripheral health fails, enter Fault Mode and report the stored
   reading that caused the failure.

## Implementation Progress

- [x] Define initialization, Safe Mode, HPC Mode, Jetson fault recovery, Fault
  Mode, and terminal Emergency Shutdown states.
- [x] Define the first-tick Safe Mode initialization behavior.
- [x] Define Jetson aggregate-fault behavior using full thermal readings.
- [x] Define the Jetson power-on gating boundary at HPC Mode.
- [x] Add full thermal-reading input ports to `FPManager`.
- [x] Add cached reading storage and Jetson sensor aggregation.
- [x] Add gated Jetson power request handling and command rejection events.
- [x] Add fault-cause events/telemetry for the stored `ThermalReading`.
- [x] Instantiate and drive `FPStateMachine` from `FPManager`.
- [x] Wire local i.MX and MCP thermal readings into FPManager.
- [x] Add the upstream fatal-announcement input to the terminal shutdown path.
- [x] Transport the nine Jetson readings across hub serial channel 4 into FPManager.
- [x] Keep `REQUEST_JETSON_POWER_STATE` in JetsonManager while synchronously
  authorizing it through FPManager before GPIO or hub activity.
- [x] Make FP recovery and emergency Jetson power-off requests synchronous so
  protection GPIO actions are not left behind in a queue.
- [x] Make peripheral emergency power-off synchronous and latched.
- [x] Add FPManager unit tests for Safe Mode, HPC gating, Jetson recovery, and
  fatal shutdown. Runtime execution requires an ARM64 target or emulator.

## Component Relationships

The deployment topology connects thermal managers to FPManager, JetsonManager
to the synchronous power authorization gate, and FPManager protection outputs
to JetsonManager and PerifBoardManager. The authoritative port wiring is in
`ImxDeployment/Top/topology.fpp`.

## Port Descriptions
| Name | Description |
|---|---|
| Thermal reading inputs | Full `ThermalReading` values from i.MX, MCP/local peripheral, and Jetson sources. Jetson input is multi-reading and aggregated by sensor ID. |
| Jetson power authorization | Synchronous gate called by JetsonManager before executing `REQUEST_JETSON_POWER_STATE`. |
| Internal power output | Synchronous OFF request to JetsonManager for recovery and emergency protection; it drives the Jetson GPIO LOW immediately. |
| Peripheral emergency output | Synchronous, latched OFF request that holds the peripheral board power down. |
| Rate-group tick | Drives initialization and periodic health checks. |

## Component States
| Name | Description |
|---|---|
| `init` | Startup state. The first tick initializes Safe Mode. |
| `safeMode` | Jetson off; i.MX and peripheral health are checked. |
| `hpcMode` | HPC enabled; i.MX, peripheral, and aggregate Jetson thermal health are checked. Jetson ON commands are authorized only here. |
| `jetsonFaultRecovery` | Confirms and reports a Jetson fault, powers off Jetson, then returns to Safe Mode. |
| `faultMode` | Reports non-recoverable i.MX/peripheral or general protection faults. |
| `emergencyShutdown` | Terminal state for `$fatal`; performs the one-shot shutdown before fatal handling is forwarded. |

## Sequence Diagrams

### Jetson Fault Recovery

```text
HPC tick -> FPManager: evaluate all ThermalReading values
FPManager -> FPStateMachine: jetson_fault
FPStateMachine -> FPManager: confirmJetsonFaultAndPowerOff
FPManager -> Jetson power manager: OFF
FPManager -> GDS: report sensorId/location/timestamp
FPManager -> FPStateMachine: success
FPStateMachine -> safeMode: enter
```

### Emergency Shutdown

```text
FatalAnnounce -> FPManager: fatalIn
FPManager: emit EMERGENCY_SHUTDOWN
FPManager -> JetsonManager: synchronous OFF / GPIO LOW
FPManager -> PerifBoardManager: synchronous latched OFF
FPManager -> FatalHandler: forward fatal event
FatalHandler -> i.MX process: terminate
Satellite power cycle -> system: reset
```

## Parameters
| Name | Description |
|---|---|
|---|---|

## Commands
| Name | Description |
|---|---|
| HPC mode enable | Requests transition from Safe Mode to HPC Mode. The request is accepted only after Safe Mode health checks pass. |
| Jetson power request | Requests Jetson power changes. ON is gated to HPC Mode; OFF is always permitted. |

## Events
| Name | Description |
|---|---|
| Fault detected | Reports the failing subsystem and, for thermal faults, the complete source reading. |
| Emergency shutdown | High-priority warning emitted when `$fatal` causes the terminal shutdown action. |

## Telemetry
| Name | Description |
|---|---|
| `FP_STATE` | Current FPManager state value. |
| `JETSON_VALID_READING_COUNT` | Number of Jetson sensor IDs with valid cached readings. |

## Unit Tests
| Name | Description | Output | Coverage |
|---|---|---|---|
| `initializesSafeModeAndGatesJetsonOn` | First tick initializes Safe Mode and rejects a Jetson ON authorization request. | `FAILURE`, Jetson OFF request, rejection event | FP-001, FP-002, FP-003 |
| `entersHpcModeAndAcceptsJetsonOn` | Enables HPC Mode and permits a Jetson ON authorization request. | `SUCCESS` and no rejection event | FP-003 |
| `attributesJetsonFaultAndReturnsSafe` | Aggregates the nine Jetson sensor readings, identifies sensor 4, reports its full reading, powers off the Jetson, and returns to Safe Mode. | Fault event with source, sensor ID, temperature, state, location, and timestamp; Jetson OFF | FP-004, FP-005 |
| `fatalShutdownForwardsAndLatches` | Routes `$fatal` to the terminal emergency shutdown path, forwards the fatal event, emits emergency shutdown, powers down protected devices, and rejects later Jetson ON requests. | Fatal forwarding, shutdown event, Jetson OFF, peripheral OFF, authorization failure | FP-007, FP-008 |

The FPManager UT target is built with `fprime-util generate imx8x --ut --disable-sanitizers`; execution requires an ARM64 target or an AArch64 emulator.

## Requirements
| Name | Description | Validation |
|---|---|---|
| FP-001 | The first tick after startup shall initialize the system in Safe Mode. | `initializesSafeModeAndGatesJetsonOn` |
| FP-002 | Safe Mode shall keep the Jetson powered off. | `initializesSafeModeAndGatesJetsonOn` |
| FP-003 | Jetson power-on shall be accepted only in HPC Mode. | `initializesSafeModeAndGatesJetsonOn`, `entersHpcModeAndAcceptsJetsonOn` |
| FP-004 | Any Jetson `ThermalStates.FAULT` reading shall assert the Jetson fault condition. | `attributesJetsonFaultAndReturnsSafe` |
| FP-005 | Jetson fault recovery shall preserve and report the offending full `ThermalReading`. | `attributesJetsonFaultAndReturnsSafe` |
| FP-006 | i.MX or peripheral faults shall enter Fault Mode. | State-machine test coverage pending |
| FP-007 | `$fatal` shall immediately enter terminal Emergency Shutdown and shall not enter Fault Mode. | `fatalShutdownForwardsAndLatches` |
| FP-008 | Emergency Shutdown shall power off all protected devices and rely on satellite power cycling for reset. | `fatalShutdownForwardsAndLatches`; deployment-level power-cycle test remains pending |

## Change Log
| Date | Description |
|---|---|
| 2026-07-21 | Documented initial FP state-machine design and implementation checkpoints. |
| 2026-07-21 | Implemented FPManager interfaces, reading cache/aggregation, local thermal wiring, and protection actions. |
| 2026-07-21 | Wired the nine Jetson thermal readings through GenericHub serial channel 4 into FPManager. |
| 2026-07-21 | Added synchronous Jetson power authorization, synchronous internal recovery/shutdown power paths, latched peripheral shutdown, and fatal-handler forwarding. |
| 2026-07-21 | Required a completed Safe Mode health-check action before accepting `ENABLE_HPC_MODE`; updated unit-test sequencing and verification notes. |
