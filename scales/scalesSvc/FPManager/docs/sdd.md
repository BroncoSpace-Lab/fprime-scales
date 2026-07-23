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

The Jetson is not permitted to be powered on by command in Safe Mode. A Jetson
power-on request is accepted only in HPC Mode. `DISABLE_HPC_MODE` returns the
system to Safe Mode, requests Jetson OFF, and republishes `FP_STATE=SAFE`.
Startup Safe Mode does not issue a one-shot Jetson OFF request; active Jetson
OFF requests remain available for operator disable, protection, and recovery
actions.

`$fatal` is an emergency override. It is invoked only when CdhCore's
`EventManager` announces a FATAL event on `FatalAnnounce`, which is wired to
`FPManager.fatalIn`. FPManager performs the one-shot protection action before
forwarding the fatal event to the standard fatal handler. The action emits the
high-priority shutdown event, synchronously requests Jetson and peripheral
power removal, writes `FP_STATE=EMERGENCY`, and latches `emergencyShutdown`.
It does not transition through `faultMode` and has no recovery transition.

i.MX, peripheral, and Jetson thermal faults are handled as separate fault
domains. An i.MX `ThermalStates.FAULT` is a system-level fatal condition:
FPManager reports the offending `ThermalReading`, immediately runs the global
Emergency Shutdown protection action, and forwards a fatal event to the
standard fatal handler. A peripheral `ThermalStates.FAULT` runs a *local
peripheral emergency shutdown*: FPManager reports the offending
`ThermalReading`, asserts the `peripheralPowerOff` emergency output, and enters
`faultMode`. It does not emit the global `EMERGENCY_SHUTDOWN` event, power off
the Jetson, or forward `fatalOut`. A Jetson-only `ThermalStates.FAULT` enters
`jetsonFaultRecovery`, powers off the Jetson, reports the offending reading,
and returns to Safe Mode.

The current implementation does not directly power off the i.MX hardware.
After FPManager runs its protection action, it forwards the fatal event to
F Prime's Linux `Svc.FatalHandler`, which delays briefly and aborts/exits the
process. A complete i.MX hardware power-off still requires a board-level
shutdown path, PMIC control, watchdog reset, or external satellite power cycle.

## Functional Diagrams

The diagrams below separate the state machine, topology wiring, decision logic,
and fatal path so each view stays readable.

### State Machine

This is a 1:1 rendering of `FPStateMachine.fpp`. Rounded nodes are states,
rectangular nodes are FPP actions, and labels on the arrows are FPP signals.
The health-check outcomes that produce those signals are shown separately
below, because they are component logic rather than additional state-machine
transitions.

```mermaid
flowchart LR
    start((initial))
    init((init))
    safe((safeMode))
    hpc((hpcMode))
    jetson((jetsonFaultRecovery))
    fault((faultMode))
    emergency((emergencyShutdown))

    initSafe[initializeSafeMode]
    safeCheck[safeModeHealthCheck]
    hpcCheck[hpcModeHealthCheck]
    enable[enableHpcMode]
    disable[disableHpcMode]
    confirm[confirmJetsonFaultAndPowerOff]
    report[reportFault]
    shutdown[SHUTDOWN]

    start --> init
    init -->|tick| initSafe --> safe
    init -->|"$fatal"| shutdown --> emergency

    safe -->|tick| safeCheck --> safe
    safe -->|hpcMode_en| enable --> hpc
    safe -->|failure| report --> fault
    safe -->|"$fatal"| shutdown

    hpc -->|tick| hpcCheck --> hpc
    hpc -->|hpcMode_dis| disable --> safe
    hpc -->|jetson_fault| jetson
    hpc -->|failure| report
    hpc -->|"$fatal"| shutdown

    jetson -->|tick| confirm -->|success| safe
    jetson -->|failure| report
    jetson -->|"$fatal"| shutdown

    fault -->|healthy| safe
    fault -->|"$fatal"| shutdown

    shutdown --> emergency
```

### Component Relationships

```mermaid
flowchart TB
    subgraph Cdh["CdhCore and GDS"]
        GDS["GDS commands telemetry events"]
        EVT["EventManager FatalAnnounce"]
        FH["FatalHandler"]
    end

    subgraph FP["FPManager protection boundary"]
        FPM["FPManager"]
        SM["FPStateMachine"]
    end

    subgraph Thermal["Thermal inputs"]
        ITM["ImxThermalManager"]
        MCP["McpManager"]
        HUB["GenericHub"]
    end

    subgraph Power["Power control"]
        JM["JetsonManager"]
        PBM["PerifBoardManager"]
    end

    ITM -->|"i.MX ThermalReading"| FPM
    MCP -->|"MCP sensor 1 or 2 ThermalReading"| FPM
    HUB -->|"Jetson ThermalReading channel 4"| FPM

    FPM <-->|"signals and actions"| SM

    GDS -->|"ENABLE_HPC_MODE or DISABLE_HPC_MODE"| FPM
    GDS -->|"REQUEST_JETSON_POWER_STATE"| JM
    FPM -->|"FP_STATE and fault events"| GDS

    JM -->|"authorize requested Jetson state"| FPM
    JM -->|"reported Jetson power state"| FPM
    FPM -->|"internal OFF request"| JM
    FPM -->|"emergency peripheral OFF"| PBM

    EVT -->|"FATAL event id"| FPM
    FPM -->|"forward FATAL event id"| FH
```

### Health Evaluation And Fault Scope

```mermaid
flowchart TD
    Tick["run tick"] --> State{"current FP state"}

    State -->|"INIT"| Init["initializeSafeMode"]
    Init --> Safe["FP_STATE SAFE"]

    State -->|"SAFE"| SafeCheck["safeModeHealthCheck"]
    SafeCheck --> SafeFault{"fault source"}
    SafeFault -->|"none"| SafeHealthy["set safeModeHealthy true<br>write FP_STATE SAFE"]
    SafeFault -->|"i.MX"| ImxEmergency["report IMX ThermalReading<br>SHUTDOWN<br>forward fatalOut"]
    SafeFault -->|"peripheral"| PerifFault["peripheralPowerOff<br>local peripheral emergency shutdown"]

    State -->|"HPC"| HpcCheck["hpcModeHealthCheck"]
    HpcCheck --> HpcFault{"fault source"}
    HpcFault -->|"none"| HpcHealthy["write FP_STATE HPC"]
    HpcFault -->|"Jetson only"| JetsonFault["remember Jetson ThermalReading<br>send jetson_fault"]
    HpcFault -->|"i.MX"| ImxEmergency
    HpcFault -->|"peripheral"| PerifFault

    ImxEmergency --> Emergency["write FP_STATE EMERGENCY"]
    PerifFault --> FaultReport["failure signal<br>reportFault<br>write FP_STATE FAULT"]

    JetsonFault --> Recovery["confirmJetsonFaultAndPowerOff"]
    Recovery --> RecoveryOk{"Jetson FAULT still present"}
    RecoveryOk -->|"yes"| RecoverSafe["emit FAULT_DETECTED<br>request Jetson OFF<br>write FP_STATE SAFE"]
    RecoveryOk -->|"no"| FaultReport
```

The resulting behavior is intentionally asymmetric:

| Fault source | Immediate protection action | State/event result |
| --- | --- | --- |
| i.MX `FAULT` | Global `SHUTDOWN`; Jetson OFF; peripheral OFF; `fatalOut` | `FP_STATE=EMERGENCY`; `EMERGENCY_SHUTDOWN`; process fatal path |
| Peripheral `FAULT` | Peripheral-only emergency output `peripheralPowerOff` | `FAULT_DETECTED`; `failure`; `FP_STATE=FAULT`; no global shutdown |
| Jetson `FAULT` in HPC | Jetson OFF through JetsonManager recovery path | `FAULT_DETECTED`; `jetsonFaultRecovery`; `FP_STATE=SAFE` |
| Upstream `$fatal` | Global `SHUTDOWN`; Jetson OFF; peripheral OFF; `fatalOut` | `FP_STATE=EMERGENCY`; `EMERGENCY_SHUTDOWN`; terminal latch |

This table is the implementation contract. In particular, “peripheral
emergency shutdown” means the emergency power-off of the peripheral board; it
does not mean the global FPManager `emergencyShutdown` state. The current code
and tests implement that local scope. If the requirement instead is for a
peripheral fault to enter global `FP_STATE=EMERGENCY`, emit
`EMERGENCY_SHUTDOWN`, and forward `fatalOut`, that is a different safety
policy and must be changed in both `FPManager.cpp` and the tests.

### Jetson Power Authorization

```mermaid
sequenceDiagram
    actor Operator as GDS Operator
    participant JM as JetsonManager
    participant FPM as FPManager
    participant GPIO as Jetson Power GPIO

    Operator->>JM: REQUEST_JETSON_POWER_STATE
    JM->>FPM: fpJetsonPowerAuthorize
    alt unsupported requested state
        FPM-->>JM: FAILURE
        JM-->>Operator: VALIDATION_ERROR
    else ON requested outside HPC
        FPM-->>JM: FAILURE
        FPM-->>Operator: JETSON_POWER_REQUEST_REJECTED
        JM-->>Operator: VALIDATION_ERROR
    else emergency latched
        FPM-->>JM: FAILURE
        FPM-->>Operator: JETSON_POWER_REQUEST_REJECTED
        JM-->>Operator: VALIDATION_ERROR
    else request authorized
        FPM-->>JM: SUCCESS
        alt requested ON
            JM->>GPIO: HIGH
            JM->>FPM: fpJetsonPowerStateOut(ON)
            JM-->>Operator: OK
        else requested OFF through command path
            JM->>JM: graceful OFF if Jetson is known ON
            JM->>GPIO: LOW
            JM->>FPM: fpJetsonPowerStateOut(OFF)
            JM-->>Operator: OK
        end
    end
```

### HPC Enable And Disable

```mermaid
sequenceDiagram
    actor Operator as GDS Operator
    participant FPM as FPManager
    participant SM as FPStateMachine
    participant JM as JetsonManager

    Operator->>FPM: ENABLE_HPC_MODE
    alt FP_STATE=SAFE and safe health check passed
        FPM->>SM: hpcMode_en
        SM->>FPM: enableHpcMode
        FPM-->>Operator: OK
        FPM-->>Operator: FP_STATE=HPC
    else not safe or health not confirmed
        FPM-->>Operator: VALIDATION_ERROR
    end

    Operator->>FPM: DISABLE_HPC_MODE
    alt FP_STATE=HPC
        FPM->>SM: hpcMode_dis
        SM->>FPM: disableHpcMode
        opt Jetson is known ON
            FPM->>JM: jetsonPowerRequestOut(OFF)
            JM->>JM: direct FP protection OFF
            JM->>FPM: fpJetsonPowerStateOut(OFF)
        end
        FPM-->>Operator: OK
        FPM-->>Operator: FP_STATE=SAFE
    else FP_STATE=SAFE
        FPM-->>Operator: OK
        FPM-->>Operator: FP_STATE=SAFE
    else FAULT or EMERGENCY
        FPM-->>Operator: VALIDATION_ERROR
    end
```

### Jetson Fault Recovery

```mermaid
sequenceDiagram
    participant FPM as FPManager
    participant SM as FPStateMachine
    participant JM as JetsonManager
    participant PBM as PerifBoardManager
    actor GDS as GDS

    FPM->>FPM: hpcModeHealthCheck
    FPM->>FPM: findJetsonFault
    alt Jetson FAULT and local health is OK
        FPM->>FPM: rememberFault JETSON
        FPM->>SM: jetson_fault
        SM->>FPM: confirmJetsonFaultAndPowerOff
        FPM->>GDS: FAULT_DETECTED with full ThermalReading
        FPM->>JM: jetsonPowerRequestOut OFF
        JM->>JM: direct FP protection OFF
        FPM->>SM: success
        FPM->>GDS: FP_STATE SAFE
    else i.MX FAULT
        FPM->>GDS: FAULT_DETECTED with IMX ThermalReading
        FPM->>FPM: SHUTDOWN
        FPM->>GDS: FP_STATE EMERGENCY
        FPM->>JM: jetsonPowerRequestOut OFF
    else peripheral FAULT
        FPM->>PBM: peripheralPowerOff
        FPM->>SM: failure
        SM->>FPM: reportFault
        FPM->>GDS: FAULT_DETECTED and FP_STATE FAULT
    end
```

### Emergency Shutdown Trigger

```mermaid
sequenceDiagram
    participant Source as Any component
    participant Events as CdhCore EventManager
    participant FPM as FPManager
    participant JM as JetsonManager
    participant PBM as PerifBoardManager
    participant FH as F Prime FatalHandler
    participant HW as Board power system
    actor GDS as GDS

    alt upstream fatal event
        Source->>Events: log FATAL event or FW_ASSERT
        Events->>FPM: FatalAnnounce to fatalIn
    else i.MX ThermalReading FAULT
        FPM->>FPM: safeModeHealthCheck or hpcModeHealthCheck
        FPM->>GDS: FAULT_DETECTED with IMX ThermalReading
    end
    FPM->>FPM: SHUTDOWN one-shot action
    FPM->>GDS: EMERGENCY_SHUTDOWN
    FPM->>JM: jetsonPowerRequestOut OFF
    FPM->>PBM: peripheralPowerOff
    FPM->>GDS: FP_STATE EMERGENCY
    FPM->>FH: fatalOut
    FH->>FH: abort or exit FSW process
    Note over FPM,HW: FPManager does not directly power off i.MX hardware
    HW-->>FPM: external power cycle resets latch
```

### Operating Rules

1. On the first scheduler tick, initialize FPManager in Safe Mode and gate
   Jetson ON commands.
2. In Safe Mode, evaluate i.MX and peripheral readings and keep Jetson power-on
   commands gated.
3. After a Safe Mode health check passes, `ENABLE_HPC_MODE` may transition to
   HPC Mode. HPC Mode evaluates i.MX, peripheral, and Jetson readings on each
   tick; Jetson ON commands remain gated by the resulting state.
4. While still in HPC Mode, operators may command `REQUEST_JETSON_POWER_STATE`
   to `OFF` to use the graceful-ish Jetson shutdown path.
5. `DISABLE_HPC_MODE` may transition from HPC Mode back to Safe Mode. The
   transition requests direct Jetson OFF if Jetson is still known ON and updates
   `FP_STATE` to `SAFE`, causing subsequent Jetson ON requests to be rejected
   again.
6. If the i.MX is faulty, record and report the full reading, immediately run
   Emergency Shutdown, and forward a fatal event to the standard fatal handler.
7. If the peripheral board is faulty, record and report the full reading, latch
   the peripheral board off, and enter Fault Mode without shutting down the
   whole system.
8. If only the Jetson is faulty, record the offending full reading, power off
   the Jetson, report the cause, and return to Safe Mode.
9. If CdhCore announces a FATAL event, immediately enter Emergency Shutdown,
   power down the protected Jetson/peripheral outputs, forward to F Prime's
   fatal handler, and wait for an external reset/power cycle.

## Implementation Progress

- [x] Define initialization, Safe Mode, HPC Mode, Jetson fault recovery, Fault
  Mode, and terminal Emergency Shutdown states.
- [x] Define the first-tick Safe Mode initialization behavior.
- [x] Define Jetson aggregate-fault behavior using full thermal readings.
- [x] Define the Jetson power-on gating boundary at HPC Mode.
- [x] Add an operator command to disable HPC Mode, power off Jetson, and return
  to Safe Mode.
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
- [x] Add FPManager unit tests for Safe Mode, HPC gating, i.MX emergency
  shutdown, peripheral-only shutdown, Jetson recovery, and fatal shutdown.
  Runtime execution requires an ARM64 target or emulator.

## Component Relationships

The deployment topology connects thermal managers to FPManager, JetsonManager
to the synchronous power authorization gate, and FPManager protection outputs
to JetsonManager and PerifBoardManager. JetsonManager also reports its last
Jetson power state to `jetsonPowerStateIn`, allowing FPManager to request
Jetson OFF during `DISABLE_HPC_MODE` only when the Jetson is known ON. The
authoritative port wiring is in `ImxDeployment/Top/topology.fpp`.

## Port Descriptions
| Name | Description |
|---|---|
| Thermal reading inputs | Full `ThermalReading` values from i.MX, MCP/local peripheral, and Jetson sources. Jetson input is multi-reading and aggregated by sensor ID. |
| Jetson power authorization | Synchronous gate called by JetsonManager before executing `REQUEST_JETSON_POWER_STATE`. |
| Internal power output | Synchronous OFF request to JetsonManager for recovery, HPC disable, and emergency protection; this uses the direct FP protection path. |
| Peripheral emergency output | Synchronous, latched OFF request that holds the peripheral board power down. |
| Rate-group tick | Drives initialization and periodic health checks. |

## Component States
| Name | Description |
|---|---|
| `init` | Startup state. The first tick initializes Safe Mode. |
| `safeMode` | Jetson power-on is gated; i.MX and peripheral health are checked. |
| `hpcMode` | HPC enabled; i.MX, peripheral, and aggregate Jetson thermal health are checked. Jetson ON commands are authorized only here. |
| `jetsonFaultRecovery` | Confirms and reports a Jetson fault, powers off Jetson, then returns to Safe Mode. |
| `faultMode` | Reports and latches non-system-fatal protection faults, currently including peripheral thermal FAULT. |
| `emergencyShutdown` | Terminal state for `$fatal`; performs the one-shot shutdown before fatal handling is forwarded. |

## Parameters
| Name | Description |
|---|---|
| None | FPManager currently has no configurable parameters. |

## Commands
| Name | Description |
|---|---|
| HPC mode enable | Requests transition from Safe Mode to HPC Mode. The request is accepted only after Safe Mode health checks pass. |
| HPC mode disable | Requests transition from HPC Mode back to Safe Mode. If Jetson is known ON, the request powers it off through the FPManager protection path and republishes `FP_STATE=SAFE`. |
| Jetson power request | Requests Jetson power changes through JetsonManager. ON is gated to HPC Mode; OFF is accepted unless Emergency Shutdown is latched. |

## Events
| Name | Description |
|---|---|
| Fault detected | Reports the failing subsystem and, for thermal faults, the complete source reading. |
| Emergency shutdown | High-priority warning emitted when `$fatal` causes the terminal shutdown action. |

## Telemetry
| Name | Description |
|---|---|
| `FP_STATE` | Current FPManager state enum (`INIT`, `SAFE`, `HPC`, `FAULT`, or `EMERGENCY`). Written on state transitions and steady Safe/HPC health-check ticks. |
| `JETSON_VALID_READING_COUNT` | Number of Jetson sensor IDs with valid cached readings. |

## Unit Tests
| Name | Description | Output | Coverage |
|---|---|---|---|
| `initializesSafeModeAndGatesJetsonOn` | First tick initializes Safe Mode and rejects a Jetson ON authorization request. | `FAILURE`, no startup Jetson OFF request, rejection event | FP-001, FP-002, FP-003 |
| `entersHpcModeAndAcceptsJetsonOn` | Enables HPC Mode and permits a Jetson ON authorization request. | `SUCCESS` and no rejection event | FP-003 |
| `disablesHpcModeAndGatesJetsonOn` | Tracks Jetson ON, disables HPC Mode, requests Jetson OFF, republishes `SAFE`, and rejects a later Jetson ON authorization request. | Jetson OFF, `FP_STATE=SAFE`, authorization failure | FP-002, FP-003, FP-009 |
| `imxFaultTriggersEmergencyShutdown` | Sends an i.MX `ThermalStates.FAULT` reading and verifies the system emergency path. | Fault event, emergency shutdown event, Jetson OFF, peripheral OFF, fatal forwarding, `FP_STATE=EMERGENCY` | FP-006, FP-007, FP-008 |
| `peripheralFaultPowersOffPeripheralOnly` | Sends a peripheral `ThermalStates.FAULT` reading and verifies only the peripheral protection path runs. | Fault event, peripheral OFF, no emergency shutdown, no Jetson OFF, `FP_STATE=FAULT` | FP-006 |
| `attributesJetsonFaultAndReturnsSafe` | Aggregates the nine Jetson sensor readings, identifies sensor 4, reports its full reading, powers off the Jetson, and returns to Safe Mode. | Fault event with source, sensor ID, temperature, state, location, and timestamp; Jetson OFF | FP-004, FP-005 |
| `fatalShutdownForwardsAndLatches` | Routes `$fatal` to the terminal emergency shutdown path, forwards the fatal event, emits emergency shutdown, powers down protected devices, and rejects later Jetson ON requests. | Fatal forwarding, shutdown event, Jetson OFF, peripheral OFF, authorization failure | FP-007, FP-008 |

The FPManager UT target is built with `fprime-util generate imx8x --ut --disable-sanitizers`; execution requires an ARM64 target or an AArch64 emulator.

## Requirements
| Name | Description | Validation |
|---|---|---|
| FP-001 | The first tick after startup shall initialize the system in Safe Mode. | `initializesSafeModeAndGatesJetsonOn` |
| FP-002 | Safe Mode shall reject Jetson power-on commands until HPC Mode is enabled. | `initializesSafeModeAndGatesJetsonOn` |
| FP-003 | Jetson power-on shall be accepted only in HPC Mode. | `initializesSafeModeAndGatesJetsonOn`, `entersHpcModeAndAcceptsJetsonOn` |
| FP-004 | Any Jetson `ThermalStates.FAULT` reading shall assert the Jetson fault condition. | `attributesJetsonFaultAndReturnsSafe` |
| FP-005 | Jetson fault recovery shall preserve and report the offending full `ThermalReading`. | `attributesJetsonFaultAndReturnsSafe` |
| FP-006 | Peripheral thermal FAULT shall latch the peripheral board off and enter Fault Mode without system Emergency Shutdown. | `peripheralFaultPowersOffPeripheralOnly` |
| FP-007 | `$fatal` or i.MX thermal FAULT shall immediately enter terminal Emergency Shutdown and shall not enter Fault Mode. | `fatalShutdownForwardsAndLatches`, `imxFaultTriggersEmergencyShutdown` |
| FP-008 | Emergency Shutdown shall power off the protected Jetson and peripheral outputs and rely on board-level reset or satellite power cycling for full system recovery. | `fatalShutdownForwardsAndLatches`, `imxFaultTriggersEmergencyShutdown`; deployment-level power-cycle test remains pending |
| FP-009 | Disabling HPC Mode shall request Jetson OFF when Jetson is known ON, return FPManager to Safe Mode, and re-gate Jetson ON requests. | `disablesHpcModeAndGatesJetsonOn` |

## Change Log
| Date | Description |
|---|---|
| 2026-07-21 | Documented initial FP state-machine design and implementation checkpoints. |
| 2026-07-21 | Implemented FPManager interfaces, reading cache/aggregation, local thermal wiring, and protection actions. |
| 2026-07-21 | Wired the nine Jetson thermal readings through GenericHub serial channel 4 into FPManager. |
| 2026-07-21 | Added synchronous Jetson power authorization, synchronous internal recovery/shutdown power paths, latched peripheral shutdown, and fatal-handler forwarding. |
| 2026-07-21 | Required a completed Safe Mode health-check action before accepting `ENABLE_HPC_MODE`; updated unit-test sequencing and verification notes. |
| 2026-07-22 | Made steady Safe/HPC health checks republish `FP_STATE` so GDS can observe FPManager after startup. |
| 2026-07-22 | Changed `FP_STATE` telemetry from raw `U8` to the `FPManagerState` enum for labeled GDS display. |
| 2026-07-22 | Added `DISABLE_HPC_MODE` to power off Jetson when needed, return to Safe Mode, and re-gate Jetson ON requests. |
| 2026-07-22 | Removed the startup Jetson OFF request from Safe Mode initialization; active Jetson OFF is reserved for recovery and emergency paths. |
| 2026-07-22 | Added Mermaid diagrams for FPManager state transitions, component relationships, health evaluation, Jetson power authorization, HPC control, Jetson fault recovery, and emergency shutdown. |
| 2026-07-22 | Documented graceful commanded Jetson OFF while keeping FPManager disable, recovery, and emergency OFF direct. |
| 2026-07-22 | Split thermal fault handling by domain: i.MX FAULT enters system Emergency Shutdown, peripheral FAULT latches only peripheral power off, and Jetson FAULT uses Jetson recovery. |
