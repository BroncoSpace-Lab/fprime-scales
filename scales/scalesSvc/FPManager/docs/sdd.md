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

Peripheral fault recovery is health-gated. After a peripheral fault enters
`faultMode`, FPManager checks the cached readings on every rate-group tick. It
remains in `FAULT` while the peripheral reading is missing or still
`ThermalStates.FAULT`, or while any cached Jetson reading is still
`ThermalStates.FAULT`. A Jetson fault detected while already in Fault Mode is
reported and requests Jetson OFF defensively, but remains a non-system-fatal
Fault Mode condition. Once the peripheral reading is valid and outside `FAULT`,
no Jetson fault is cached, and no i.MX fault is present, FPManager emits the
internal `healthy` signal, clears the fault latch, and transitions to Safe
Mode. A new i.MX `FAULT` during this recovery check still uses the global
Emergency Shutdown path.

Jetson readings are aggregated in FPManager because the sensors share the same
die. A Jetson fault is asserted when any valid Jetson reading has
`ThermalStates.FAULT`; JetsonThermalManager is not responsible for this
system-level aggregation. While FPManager is in HPC Mode, a newly received
Jetson `FAULT` reading immediately asserts the Jetson fault signal; the system
does not need to wait for the next FPManager health-check tick.
Whenever FPManager commands or observes Jetson `OFF`, it invalidates the cached
Jetson readings and writes `JETSON_VALID_READING_COUNT=0`. This prevents a
pre-shutdown Jetson `FAULT` reading from being reused when the operator later
re-enters HPC Mode; a new Jetson fault requires a new reading after power-on.

FPManager also tracks `ThermalStates.WARN` for each fault domain purely for
operator awareness -- unlike `FAULT`, entering or exiting `WARN` triggers no
protective action at all. i.MX and peripheral each have their own one-shot
latch (`m_imxWarnActive`, `m_peripheralWarnActive`); Jetson uses a single
aggregate latch (`m_jetsonWarnActive`) across all nine sensors, mirroring how
Jetson `FAULT` is already aggregated, since the sensors share the same die.
The check runs directly inside each reading handler
(`imxThermalReadingIn_handler`, `peripheralThermalReadingIn_handler`,
`jetsonThermalReadingIn_handler`) as soon as a new reading arrives -- it does
not wait for the next health-check tick the way `FAULT` handling for i.MX and
peripheral does. `WARN_STATE_ENTERED` is emitted on the transition into
`WARN` from any other state; `WARN_STATE_EXITED` is emitted on the transition
out of `WARN`, whether that's recovery to `IDLE`, escalation to `FAULT`, or
the reading becoming unavailable (`NOT_USED`). Repeated `WARN` readings do
not re-emit `WARN_STATE_ENTERED`, and the Jetson aggregate only exits `WARN`
once every cached Jetson reading has left `WARN`. This is a read-only
observability layer: it never sets `FP_STATE`, never asserts a protection
output, and is completely independent of the `FPStateMachine` and the
SAFE/HPC/FAULT/EMERGENCY mode -- it keeps tracking regardless of what mode
FPManager itself is in.

The Jetson is not permitted to be powered on by command in Safe Mode. A Jetson
power-on request is accepted only in HPC Mode. `DISABLE_HPC_MODE` returns the
system to Safe Mode, requests Jetson OFF, and republishes `FP_STATE=SAFE`.
Startup Safe Mode does not issue a one-shot Jetson OFF request; active Jetson
OFF requests remain available for operator disable, protection, and recovery
actions.

Remote Jetson deployment commands are also gated on the i.MX before they reach
the hub transport. There are two independent remote command sources in the
topology -- `imx_cmdSplitter` (GDS-direct commands) and `imx_seqCmdSplitter`
(commands issued by a running `CmdSequencer` sequence, e.g. `CS_RUN`) -- and
both are routed through the *same* gate: `remoteJetsonCmdIn`/
`remoteJetsonCmdOut`/`remoteJetsonCmdResponseIn`/`remoteJetsonCmdResponseOut`
are `[2]`-sized arrays, with index 0 wired to `imx_cmdSplitter` and index 1
wired to `imx_seqCmdSplitter`. `remoteJetsonCmdIn_handler` and
`remoteJetsonCmdResponseIn_handler` thread the `portNum` argument straight
through to the matching output index, so one handler implementation gates
both sources identically -- there is no separate code path per source. If the
last reported Jetson power state is `OFF`, FPManager rejects the command
locally with `Fw.CmdResponse.BUSY` and emits
`REMOTE_JETSON_COMMAND_REJECTED`. This prevents commands such as
`JetsonThermalManager` parameter updates -- issued directly from GDS or from a
sequence -- from being transmitted into the hub while the Jetson is powered
off or the hub link is unavailable. If the Jetson is known `ON`, FPManager
forwards the command to GenericHub and passes the remote command response
back to the originating CmdSplitter unchanged.

Before this gate covered both sources, a sequence targeting the Jetson (e.g.
`run-ml.bin`, which issues `JetsonDeployment.jetson_mlManager.*` commands)
while the Jetson was powered off would route straight from
`imx_seqCmdSplitter.RemoteCmd[0]` to `imx_hub.cmdDispIn[1]` with no gating at
all, reach `imx_hubComStub.dataIn_handler` with the TCP link never
established, and trip `FW_ASSERT(!this->m_reinitialize ||
!this->isConnected_comStatusOut_OutputPort(0))` in
`lib/fprime/Svc/ComStub/ComStub.cpp` -- crashing the whole flight software
process instead of being rejected gracefully. Routing `imx_seqCmdSplitter`
through the same FPManager gate as `imx_cmdSplitter` fixes this.

`$fatal` is an emergency override. CdhCore's `EventManager.FatalAnnounce` and
`Svc.FatalHandler.FatalReceive` are both scalar (non-array) ports, and
CdhCore's subtopology always wires them together internally, so a deployment
cannot add a second consumer of `FatalAnnounce` directly. `ImxDeployment`
works around this with `scalesSvc.FatalRelay`
(`CdhCoreFatalHandlerConfig.fpp`), a minimal passive component swapped in as
CdhCore's `fatalHandler` instance: it exposes a `FatalReceive` port to satisfy
CdhCore's internal wiring, then re-announces the event on a fresh `fatalOut`
port that is free to route anywhere. The real path is
`EventManager.FatalAnnounce -> FatalRelay.FatalReceive -> FatalRelay.fatalOut
-> FPManager.fatalIn -> FPManager.fatalOut -> imx_realFatalHandler.FatalReceive`
(a separate `Svc.FatalHandler` instance dedicated to actually terminating the
process). See `ImxDeployment/Top/topology.fpp` and
`ImxDeployment/SubtopologyConfig/CdhCoreFatalHandlerConfig.fpp`.

`fatalIn_handler` writes `FP_STATE=EMERGENCY` and emits the high-priority
shutdown event first, then triggers the platform poweroff directly and
unconditionally (see "Platform Poweroff" below) before touching the state
machine at all, then signals `$fatal` (which asserts the Jetson/peripheral
protection outputs through the `SHUTDOWN` action) and only then forwards
`fatalOut`. The platform poweroff call is placed first, ahead of the state
machine dispatch and the Jetson/peripheral output calls, specifically so it
cannot be skipped or delayed by anything else on this path succeeding or
failing. `emergencyShutdown` has no recovery transition; FPManager does not
transition through `faultMode` on `$fatal`.

### Platform Poweroff

`ImxDeployment` runs as a systemd service (`Restart=on-failure`,
`RestartSec=5`, no capability or sandboxing restrictions). `FatalHandler`
aborts the FSW process on any fatal condition; because this is a supervised
service, systemd's default `KillMode=control-group` reaps every process still
in the service's cgroup the instant the main process exits, and the
`Restart=` policy then respawns the flight software. Earlier
approaches -- forking a `poweroff -f` child, or asking systemd via
`systemctl/poweroff --no-block` -- were both still racing that cgroup
teardown or depended on which `poweroff` implementation happens to be
installed on the image. `FPManager::triggerPlatformPoweroff()`
(`FPManager.cpp`) instead:

1. Calls `::sync()`, then the kernel's `reboot(RB_POWER_OFF)` syscall
   directly, in this still-alive process -- no forked child needs to survive
   this process's teardown, and no external binary or init system is
   involved. `RB_POWER_OFF` (not `RB_AUTOBOOT`) cuts power rather than
   restarting; on success this call does not return, so it must be (and is)
   the last thing the shared `SHUTDOWN` action does.
2. If that syscall fails (e.g. missing `CAP_SYS_BOOT`), logs
   `PLATFORM_POWEROFF_SYSCALL_FAILED` with the `errno`, then falls back to
   `systemd-run --no-block --collect -- /sbin/poweroff -f`. This hands the
   confirmed-working `poweroff -f` command to a brand-new transient unit
   that systemd (PID 1) spawns and owns directly, in its own cgroup outside
   `ImxDeployment.service`, so it survives this service being torn down
   regardless of timing. If that fallback's exit status is non-zero (or it
   could not even spawn a shell), `PLATFORM_POWEROFF_FALLBACK_FAILED` is
   logged with the exit code (127 typically means the command was not found
   on this image).

The call is idempotent and guarded by `m_platformPoweroffTriggered` (set on
first entry, checked before doing any work), and is invoked directly from
both `fatalIn_handler` and `triggerImxEmergencyShutdown` -- not only from the
shared `SHUTDOWN` action -- so it cannot be conditioned on the state
machine's `$fatal` dispatch, or on the Jetson/peripheral output calls,
succeeding. Confirmed working on hardware.

i.MX, peripheral, and Jetson thermal faults are handled as separate fault
domains. An i.MX `ThermalStates.FAULT` is a system-level fatal condition:
FPManager reports the offending `ThermalReading`, writes `FP_STATE=EMERGENCY`,
emits `EMERGENCY_SHUTDOWN`, forwards a fatal event to the standard fatal
handler, and then runs the global Emergency Shutdown protection action. A
peripheral `ThermalStates.FAULT` runs a *local peripheral emergency shutdown*:
FPManager reports the offending `ThermalReading`, writes `FP_STATE=FAULT`, and
only then asserts the `peripheralPowerOff` emergency output. It does not emit
the global `EMERGENCY_SHUTDOWN` event, power off the Jetson, or forward
`fatalOut`. A Jetson-only `ThermalStates.FAULT` enters
`jetsonFaultRecovery`, powers off the Jetson, reports the offending reading,
and returns to Safe Mode.

For an i.MX thermal `FAULT`, FPManager emits the detailed `FAULT_DETECTED`
event, writes `FP_STATE=EMERGENCY`, and emits `EMERGENCY_SHUTDOWN`, then
triggers the platform poweroff directly (see "Platform Poweroff" above)
before signaling `$fatal` (which asserts the Jetson/peripheral protection
outputs through `SHUTDOWN`) and forwarding `fatalOut`. The platform poweroff
attempt is intentionally the first thing that happens after the
events/telemetry, ahead of the Jetson/peripheral cuts and the fatal forward,
so it cannot be skipped or delayed by anything else on this path.

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
    faultCheck[faultModeHealthCheck]
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

    fault -->|tick| faultCheck --> fault
    fault -->|healthy| safe
    fault -->|"$fatal"| shutdown

    shutdown --> emergency
```

### Component Relationships

```mermaid
flowchart TB
    subgraph Cdh["CdhCore and GDS"]
        GDS["GDS commands telemetry events"]
        SPLIT["imx_cmdSplitter (GDS-direct, index 0)"]
        SEQSPLIT["imx_seqCmdSplitter (CmdSequencer, index 1)"]
        EVT["EventManager FatalAnnounce"]
        RELAY["FatalRelay (swapped in as CdhCore.fatalHandler)"]
        FH["imx_realFatalHandler (Svc.FatalHandler)"]
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
    GDS -->|"remote Jetson command"| SPLIT
    GDS -->|"CS_RUN sequence"| SEQSPLIT
    SPLIT -->|"RemoteCmd[0] -> remoteJetsonCmdIn[0]"| FPM
    SEQSPLIT -->|"RemoteCmd[0] -> remoteJetsonCmdIn[1]"| FPM
    FPM -->|"forward only if Jetson ON"| HUB
    FPM -->|"BUSY if Jetson OFF"| SPLIT
    FPM -->|"BUSY if Jetson OFF"| SEQSPLIT
    FPM -->|"FP_STATE and fault events"| GDS

    JM -->|"authorize requested Jetson state"| FPM
    JM -->|"reported Jetson power state"| FPM
    FPM -->|"internal OFF request"| JM
    FPM -->|"emergency peripheral OFF"| PBM

    EVT -->|"FatalAnnounce (single connection)"| RELAY
    RELAY -->|"fatalOut"| FPM
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
    SafeFault -->|"i.MX"| ImxEmergency["report IMX ThermalReading<br>FP_STATE EMERGENCY<br>EMERGENCY_SHUTDOWN<br>trigger platform poweroff"]
    SafeFault -->|"peripheral"| PerifFault["report peripheral ThermalReading<br>FP_STATE FAULT<br>peripheralPowerOff"]

    State -->|"HPC"| HpcCheck["hpcModeHealthCheck"]
    HpcCheck --> HpcFault{"fault source"}
    HpcFault -->|"none"| HpcHealthy["write FP_STATE HPC"]
    HpcFault -->|"Jetson only"| JetsonFault["remember Jetson ThermalReading<br>send jetson_fault"]
    HpcFault -->|"i.MX"| ImxEmergency
    HpcFault -->|"peripheral"| PerifFault

    ImxEmergency --> Emergency["$fatal signal: SHUTDOWN outputs (Jetson/peripheral OFF)<br>fatalOut forwarded"]
    PerifFault --> FaultReport["failure signal<br>enter faultMode"]

    FaultReport --> FaultCheck["faultModeHealthCheck on next tick"]
    FaultCheck --> ImxFaultCheck{"i.MX FAULT"}
    ImxFaultCheck -->|"yes"| ImxEmergency
    ImxFaultCheck -->|"no"| JetsonFaultCheck{"Jetson FAULT cached"}
    JetsonFaultCheck -->|"yes"| JetsonFaulted["report Jetson ThermalReading<br>request Jetson OFF<br>write FP_STATE FAULT"]
    JetsonFaulted --> FaultCheck
    JetsonFaultCheck -->|"no"| PeripheralClear{"peripheral valid<br>and non-FAULT"}
    PeripheralClear -->|"no: remain faulted"| FaultCheck
    PeripheralClear -->|"yes"| SafeRecovery["emit healthy<br>clear fault latch<br>write FP_STATE SAFE"]
    SafeRecovery --> SafeMode["safeMode"]

    JetsonFault --> Recovery["confirmJetsonFaultAndPowerOff"]
    Recovery --> RecoveryOk{"Jetson FAULT still present"}
    RecoveryOk -->|"yes: Jetson FAULT still present"| RecoverSafe["emit FAULT_DETECTED<br>request Jetson OFF<br>write FP_STATE SAFE"]
    RecoveryOk -->|"no: no Jetson fault"| FaultReport
```

The resulting behavior is intentionally asymmetric:

| Fault source | Immediate protection action | State/event result |
| --- | --- | --- |
| i.MX `FAULT` | Declare Emergency, then global `SHUTDOWN`; Jetson OFF; peripheral OFF; `fatalOut` | `FP_STATE=EMERGENCY`; `EMERGENCY_SHUTDOWN`; process fatal path |
| Peripheral `FAULT` | Report fault and `FP_STATE=FAULT`, then peripheral-only emergency output `peripheralPowerOff`; periodic recovery check | `FAULT_DETECTED`; `failure`; `FP_STATE=FAULT` until the peripheral reading is valid/non-FAULT and no Jetson fault is cached, then `healthy` and `FP_STATE=SAFE`; no global shutdown |
| Jetson `FAULT` while already in Fault Mode | Report Jetson fault and request Jetson OFF defensively | `FAULT_DETECTED`; `FP_STATE=FAULT`; no global shutdown |
| Jetson `FAULT` in HPC | Jetson OFF through JetsonManager recovery path; graceful if Jetson is known ON and link is available | `FAULT_DETECTED`; `jetsonFaultRecovery`; `FP_STATE=SAFE` |
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
    participant FH as imx_realFatalHandler
    participant HW as i.MX platform power
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
        FPM->>GDS: FP_STATE EMERGENCY, EMERGENCY_SHUTDOWN
        FPM->>HW: trigger platform poweroff (direct, unconditional)
        FPM->>FPM: $fatal -> SHUTDOWN
        FPM->>JM: jetsonPowerRequestOut OFF
        FPM->>PBM: peripheralPowerOff
        FPM->>FH: fatalOut
    else peripheral FAULT
        FPM->>GDS: FAULT_DETECTED with peripheral ThermalReading
        FPM->>GDS: FP_STATE FAULT
        FPM->>PBM: peripheralPowerOff
        FPM->>SM: failure
        SM->>FPM: enter faultMode
    end
```

### Emergency Shutdown Trigger

```mermaid
sequenceDiagram
    participant Source as Any component
    participant Events as CdhCore EventManager
    participant Relay as FatalRelay (CdhCore.fatalHandler)
    participant FPM as FPManager
    participant JM as JetsonManager
    participant PBM as PerifBoardManager
    participant FH as imx_realFatalHandler
    participant HW as i.MX platform power
    actor GDS as GDS

    alt upstream fatal event
        Source->>Events: log FATAL event or FW_ASSERT
        Events->>Relay: FatalAnnounce (CdhCore's only internal connection)
        Relay->>FPM: fatalOut to fatalIn
    else i.MX ThermalReading FAULT
        FPM->>FPM: safeModeHealthCheck or hpcModeHealthCheck
        FPM->>GDS: FAULT_DETECTED with IMX ThermalReading
    end
    FPM->>GDS: FP_STATE EMERGENCY
    FPM->>GDS: EMERGENCY_SHUTDOWN
    FPM->>HW: trigger platform poweroff (direct call, unconditional,\nbefore the state machine or fatalOut below)
    FPM->>FPM: $fatal signal -> SHUTDOWN one-shot action
    FPM->>JM: jetsonPowerRequestOut OFF
    FPM->>PBM: peripheralPowerOff
    FPM->>FH: fatalOut
    FH->>FH: abort or exit FSW process; systemd respawns the service
    HW->>HW: reboot(RB_POWER_OFF), or systemd-run poweroff -f fallback
    Note over FPM,HW: The platform poweroff attempt runs first and does not depend on\nthe Jetson/peripheral outputs, the state machine, or fatalOut succeeding.\nsystemd restarting the FSW process is expected and harmless once the\nboard itself is powering off.
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
6. If the i.MX is faulty, record and report the full reading, write
   `FP_STATE=EMERGENCY`, emit `EMERGENCY_SHUTDOWN`, and trigger the platform
   poweroff directly and unconditionally before doing anything else on this
   path. Only then assert the protected Jetson/peripheral shutdown outputs
   and forward the fatal event to the standard fatal handler.
7. If the peripheral board is faulty, record and report the full reading, latch
   the peripheral board off, and enter Fault Mode without shutting down the
   whole system. While faulted, recheck i.MX, Jetson, and peripheral readings
   on every tick. i.MX FAULT escalates to Emergency Shutdown, Jetson FAULT is
   reported and commanded OFF while staying in Fault Mode, and only a valid
   non-FAULT peripheral reading with no cached Jetson FAULT allows recovery to
   Safe Mode.
8. If only the Jetson is faulty, record the offending full reading, power off
   the Jetson, report the cause, and return to Safe Mode.
9. If CdhCore announces a FATAL event (via `FatalRelay`), immediately enter
   Emergency Shutdown, trigger the platform poweroff directly, power down the
   protected Jetson/peripheral outputs, and forward to the standard fatal
   handler. The platform poweroff is not conditioned on the state machine,
   the protected outputs, or the fatal forward succeeding; recovery still
   requires an operator to physically power the board back on.

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
- [x] Recheck peripheral and Jetson health in Fault Mode and recover to Safe
  Mode only after a valid non-FAULT peripheral reading and no cached Jetson
  FAULT.
- [x] Emit i.MX emergency events and telemetry before asserting global shutdown outputs.
- [x] Add FPManager unit tests for Safe Mode, HPC gating, i.MX emergency
  shutdown, peripheral-only shutdown, Jetson recovery, and fatal shutdown.
  Runtime execution requires an ARM64 target or emulator.
- [x] Fix the `FatalAnnounce`/`FatalReceive` port-arity conflict: added
  `scalesSvc.FatalRelay`, swapped in via `CdhCoreFatalHandlerConfig.fpp` as
  CdhCore's `fatalHandler` instance, and a dedicated `imx_realFatalHandler`
  instance so `FPManager` sits between the announcement and the real
  process-terminating handler without a second connection on either scalar
  port.
- [x] Replace the forked `poweroff -f` (and later `systemctl --no-block`)
  platform-shutdown mechanism, both of which lost the race against
  systemd's `KillMode=control-group` cgroup teardown of `ImxDeployment.service`,
  with a direct `reboot(RB_POWER_OFF)` syscall and a `systemd-run`-detached
  `poweroff -f` fallback. Confirmed working on hardware.
- [x] Decouple `triggerPlatformPoweroff()` from the state machine's `$fatal`
  dispatch: call it directly and unconditionally from both `fatalIn_handler`
  and `triggerImxEmergencyShutdown`, guarded by a dedicated one-shot flag
  (`m_platformPoweroffTriggered`), so it cannot be skipped by anything else
  on either path failing or behaving unexpectedly.
- [x] Add a regression test confirming a second FATAL announcement re-fires
  the announcement/forwarding events but does not re-assert the
  already-latched Jetson/peripheral protected outputs.
- [x] Route the `CmdSequencer`-originated remote Jetson command path
  (`imx_seqCmdSplitter`) through the same Jetson-power-state gate as the
  GDS-direct path (`imx_cmdSplitter`), by arrayizing
  `remoteJetsonCmdIn`/`remoteJetsonCmdOut`/`remoteJetsonCmdResponseIn`/
  `remoteJetsonCmdResponseOut` to `[2]` and threading `portNum` through one
  shared handler implementation. Fixes a crash where a sequence targeting a
  powered-off Jetson reached `Svc::ComStub`'s never-connected assert instead
  of being rejected gracefully.
- [x] Add `WARN_STATE_ENTERED`/`WARN_STATE_EXITED` events, tracked
  per-reading-handler (not tick-driven) for i.MX and peripheral individually
  and aggregated across all nine Jetson sensors, as a read-only observability
  layer alongside existing telemetry -- no protective action is taken for
  `WARN`, only `FAULT` still triggers shutdown.

## Component Relationships

The deployment topology connects thermal managers to FPManager, JetsonManager
to the synchronous power authorization gate, and FPManager protection outputs
to JetsonManager and PerifBoardManager. JetsonManager also reports its last
Jetson power state to `jetsonPowerStateIn`, allowing FPManager to request
Jetson OFF during `DISABLE_HPC_MODE` only when the Jetson is known ON. The
remote Jetson command path is similarly routed through FPManager so remote
commands are rejected locally while the Jetson is known OFF instead of being
sent into a disconnected hub transport. The
authoritative port wiring is in `ImxDeployment/Top/topology.fpp`.

## Port Descriptions
| Name | Description |
|---|---|
| Thermal reading inputs | Full `ThermalReading` values from i.MX, MCP/local peripheral, and Jetson sources. Jetson input is multi-reading and aggregated by sensor ID. |
| Jetson power authorization | Synchronous gate called by JetsonManager before executing `REQUEST_JETSON_POWER_STATE`. |
| Remote Jetson command gate | Synchronous `[2]`-sized `Fw.Com` gate between GenericHub and both remote command sources: index 0 is `imx_cmdSplitter.RemoteCmd[0]` (GDS-direct), index 1 is `imx_seqCmdSplitter.RemoteCmd[0]` (`CmdSequencer`-originated). Rejects with `BUSY` while Jetson power state is `OFF`; forwards and relays responses while Jetson is `ON`. One handler implementation gates both indices identically by threading `portNum` through to the matching output. |
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
| `faultMode` | Reports and latches non-system-fatal protection faults, currently including peripheral thermal FAULT; rechecks i.MX, Jetson, and peripheral health on each tick and recovers only after a valid non-FAULT peripheral reading and no cached Jetson FAULT. |
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
| Emergency shutdown | High-priority warning emitted when an i.MX thermal FAULT or `$fatal` causes the terminal shutdown action. |
| FP state changed | Activity event emitted when the published FPManager state changes. Repeated telemetry writes in the same state do not emit this event. |
| Remote Jetson command rejected | Warning emitted when a remote Jetson command is blocked because the Jetson is not powered on. |
| Platform poweroff syscall failed | Warning emitted when the direct `reboot(RB_POWER_OFF)` syscall fails, with the `errno`. Compiled out under `BUILD_UT`. |
| Platform poweroff fallback failed | Warning emitted when the `systemd-run ... poweroff -f` fallback exits non-zero or could not be spawned, with the exit code. Compiled out under `BUILD_UT`. |
| WARN state entered | Low-severity warning emitted when i.MX, peripheral, or the aggregate Jetson die transitions into `ThermalStates.WARN`. Purely informational; no protective action is taken. |
| WARN state exited | Activity event emitted when i.MX, peripheral, or the aggregate Jetson die transitions out of `ThermalStates.WARN` (to `IDLE`, `FAULT`, or unavailable). |

## Telemetry
| Name | Description |
|---|---|
| `FP_STATE` | Current FPManager state enum (`INIT`, `SAFE`, `HPC`, `FAULT`, or `EMERGENCY`). Written on state transitions and steady Safe/HPC health-check ticks. |
| `JETSON_VALID_READING_COUNT` | Number of Jetson sensor IDs with valid cached readings. |

## Unit Tests
| Name | Description | Output | Coverage |
|---|---|---|---|
| `initializesSafeModeAndGatesJetsonOn` | First tick initializes Safe Mode and rejects a Jetson ON authorization request. | `FAILURE`, no startup Jetson OFF request, rejection event | FP-001, FP-002, FP-003 |
| `emitsStateTransitionEventsOnlyOnChange` | Verifies `FP_STATE_CHANGED` on INIT->SAFE, SAFE->HPC, and HPC->SAFE, and no extra event on a repeated Safe health tick. | Three state transition events, no same-state spam | FP-012 |
| `entersHpcModeAndAcceptsJetsonOn` | Enables HPC Mode and permits a Jetson ON authorization request. | `SUCCESS` and no rejection event | FP-003 |
| `disablesHpcModeAndGatesJetsonOn` | Tracks Jetson ON, disables HPC Mode, requests Jetson OFF, republishes `SAFE`, and rejects a later Jetson ON authorization request. | Jetson OFF, `FP_STATE=SAFE`, authorization failure | FP-002, FP-003, FP-009 |
| `imxFaultTriggersEmergencyShutdown` | Sends an i.MX `ThermalStates.FAULT` reading and verifies the system emergency path. | Fault event, emergency shutdown event, Jetson OFF, peripheral OFF, fatal forwarding, `FP_STATE=EMERGENCY` | FP-006, FP-007, FP-008 |
| `peripheralFaultPowersOffPeripheralOnly` | Sends a peripheral `ThermalStates.FAULT` reading and verifies only the peripheral protection path runs. | Fault event, peripheral OFF, no emergency shutdown, no Jetson OFF, `FP_STATE=FAULT` | FP-006 |
| `peripheralFaultRecoversToSafeMode` | Sends a peripheral `FAULT`, then a valid non-FAULT reading, and verifies tick-driven recovery. | `FP_STATE=FAULT`, then `FP_STATE=SAFE`, no fatal forwarding | FP-006, FP-010 |
| `faultModeJetsonFaultRequestsOffAndStaysFault` | While already in Fault Mode, sends a Jetson `FAULT` and verifies redundant Jetson protection. | Jetson fault event, Jetson OFF, `FP_STATE=FAULT`, no fatal forwarding | FP-010, FP-011 |
| `faultModeImxFaultOverridesJetsonAndPeripheral` | While already in Fault Mode with Jetson also faulted, sends an i.MX `FAULT` and verifies i.MX priority. | i.MX fault event, emergency shutdown event, Jetson OFF, peripheral OFF, fatal forwarding, `FP_STATE=EMERGENCY` | FP-007, FP-010 |
| `jetsonFaultReadingTriggersRecoveryInHpc` | Sends a Jetson `FAULT` reading while in HPC and verifies the input handler asserts recovery without waiting for the next HPC health-check tick. | Jetson fault event, Jetson OFF request, `FP_STATE=SAFE` | FP-004, FP-005 |
| `jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry` | Recreates the operator sequence of HPC ON, Jetson ON, Jetson FAULT, recovery to Safe, and HPC re-entry without new Jetson readings. | Jetson cache invalidated, no repeated stale fault, `FP_STATE=HPC` after re-entry | FP-004, FP-005 |
| `attributesJetsonFaultAndReturnsSafe` | Aggregates the nine Jetson sensor readings, identifies sensor 4, reports its full reading, powers off the Jetson, and returns to Safe Mode. | Fault event with source, sensor ID, temperature, state, location, and timestamp; Jetson OFF | FP-004, FP-005 |
| `fatalShutdownForwardsAndLatches` | Routes `$fatal` to the terminal emergency shutdown path, forwards the fatal event, emits emergency shutdown, powers down protected devices, and rejects later Jetson ON requests. | Fatal forwarding, shutdown event, Jetson OFF, peripheral OFF, authorization failure | FP-007, FP-008 |
| `emergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals` | Sends `fatalIn` twice and verifies the second announcement re-fires the announcement/forwarding events but does not re-assert the already-latched protected outputs. | `EMERGENCY_SHUTDOWN`/`fatalOut` count 2, Jetson OFF/peripheral OFF count stays at 1, no extra `FP_STATE_CHANGED` | FP-008, FP-014 |
| `rejectsRemoteJetsonCommandWhenJetsonOff` | Sends a remote Jetson command on port index 0 (GDS-direct) while FPManager's Jetson power state is `OFF`. | No hub command output, local `BUSY` response, rejection event | FP-013 |
| `forwardsRemoteJetsonCommandWhenJetsonOn` | Sends a remote Jetson command on port index 0 (GDS-direct) while FPManager's Jetson power state is `ON`. | Hub command output and remote response relayed unchanged | FP-013 |
| `rejectsSequencerRemoteJetsonCommandWhenJetsonOff` | Sends a remote Jetson command on port index 1 (`imx_seqCmdSplitter`/`CmdSequencer`-originated) while FPManager's Jetson power state is `OFF`. | No hub command output, local `BUSY` response, rejection event | FP-013, FP-015 |
| `forwardsSequencerRemoteJetsonCommandWhenJetsonOn` | Sends a remote Jetson command on port index 1 while FPManager's Jetson power state is `ON`. | Hub command output and remote response relayed unchanged | FP-013, FP-015 |
| `imxWarnStateEntersAndExitsWithoutShutdown` | Sends an i.MX `WARN` reading, a repeated `WARN` reading, then an `IDLE` reading. | `WARN_STATE_ENTERED` once (not re-fired on the repeat), then `WARN_STATE_EXITED` once; no `FAULT_DETECTED`/`EMERGENCY_SHUTDOWN`/protected-output/fatal calls at any point | FP-016 |
| `peripheralWarnStateEntersAndExitsWithoutShutdown` | Sends a peripheral `WARN` reading, then an `IDLE` reading. | `WARN_STATE_ENTERED` then `WARN_STATE_EXITED`; no `FAULT_DETECTED` or peripheral power-off | FP-016 |
| `jetsonWarnStateAggregatesAcrossSensors` | Sends `WARN` readings for two different Jetson sensors (staggered), then clears them one at a time. | One `WARN_STATE_ENTERED` when the first sensor enters `WARN` (none for the second, already-`WARN` aggregate); no `WARN_STATE_EXITED` until the last `WARN` sensor clears | FP-016 |

`triggerPlatformPoweroff()`'s actual `reboot(RB_POWER_OFF)` syscall and
`systemd-run`/`poweroff -f` fallback are compiled out under `BUILD_UT`
(`#ifndef BUILD_UT`) and are not exercised by the native unit tests -- there
is no safe way to unit-test a real power-off. That mechanism is verified on
hardware only; see the fault-injection procedure referenced in the Change
Log below.

The FPManager UT target is built with `fprime-util generate imx8x --ut --disable-sanitizers`; execution requires an ARM64 target or an AArch64 emulator.

## Requirements
| Name | Description | Validation |
|---|---|---|
| FP-001 | The first tick after startup shall initialize the system in Safe Mode. | `initializesSafeModeAndGatesJetsonOn` |
| FP-002 | Safe Mode shall reject Jetson power-on commands until HPC Mode is enabled. | `initializesSafeModeAndGatesJetsonOn` |
| FP-003 | Jetson power-on shall be accepted only in HPC Mode. | `initializesSafeModeAndGatesJetsonOn`, `entersHpcModeAndAcceptsJetsonOn` |
| FP-004 | Any Jetson `ThermalStates.FAULT` reading shall assert the Jetson fault condition while FPManager is in HPC Mode. Cached Jetson readings shall be invalidated when Jetson OFF is commanded or observed so stale faults cannot retrigger on HPC re-entry. | `attributesJetsonFaultAndReturnsSafe`, `jetsonFaultReadingTriggersRecoveryInHpc`, `jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry` |
| FP-005 | Jetson fault recovery shall preserve and report the offending full `ThermalReading` before clearing the Jetson reading cache and returning to Safe Mode. | `attributesJetsonFaultAndReturnsSafe`, `jetsonFaultReadingTriggersRecoveryInHpc`, `jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry` |
| FP-006 | Peripheral thermal FAULT shall latch the peripheral board off and enter Fault Mode without system Emergency Shutdown; recovery requires a valid non-FAULT peripheral reading and no cached Jetson FAULT. | `peripheralFaultPowersOffPeripheralOnly`, `peripheralFaultRecoversToSafeMode` |
| FP-007 | `$fatal` or i.MX thermal FAULT shall immediately enter terminal Emergency Shutdown and shall not enter Fault Mode. | `fatalShutdownForwardsAndLatches`, `imxFaultTriggersEmergencyShutdown` |
| FP-008 | Emergency Shutdown shall power off the protected Jetson and peripheral outputs and trigger a real platform poweroff of the i.MX itself; full system recovery requires an operator to physically power the board back on. | `fatalShutdownForwardsAndLatches`, `imxFaultTriggersEmergencyShutdown` for the protected outputs; the platform poweroff call itself is compiled out under `BUILD_UT` and is confirmed on hardware only (fault-injection test, see Change Log) |
| FP-009 | Disabling HPC Mode shall request Jetson OFF when Jetson is known ON, return FPManager to Safe Mode, and re-gate Jetson ON requests. | `disablesHpcModeAndGatesJetsonOn` |
| FP-010 | Fault Mode shall recheck i.MX, Jetson, and peripheral health on each tick; i.MX FAULT shall escalate to Emergency Shutdown, Jetson FAULT shall request Jetson OFF and remain in Fault Mode, and recovery shall require a valid non-FAULT peripheral reading with no cached Jetson FAULT. | `peripheralFaultRecoversToSafeMode`, `faultModeJetsonFaultRequestsOffAndStaysFault`, `faultModeImxFaultOverridesJetsonAndPeripheral` |
| FP-011 | Jetson faults observed while already in Fault Mode shall preserve fault attribution and request Jetson OFF without global Emergency Shutdown. | `faultModeJetsonFaultRequestsOffAndStaysFault` |
| FP-013 | Remote Jetson deployment commands shall not be forwarded to GenericHub while the Jetson is known OFF; they shall receive a local command response instead. | `rejectsRemoteJetsonCommandWhenJetsonOff`, `forwardsRemoteJetsonCommandWhenJetsonOn` |
| FP-012 | FPManager shall emit a state transition event whenever the published `FP_STATE` changes, and shall not emit transition events for repeated writes of the same state. | `emitsStateTransitionEventsOnlyOnChange` |
| FP-014 | The protected Jetson/peripheral shutdown outputs shall be asserted at most once per process lifetime, regardless of how many times or through which path (`fatalIn`, i.MX thermal FAULT) Emergency Shutdown is re-entered. | `emergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals` |
| FP-015 | The Jetson-power-state gate on remote Jetson commands (FP-013) shall apply identically regardless of which remote command source (GDS-direct via `imx_cmdSplitter`, or `CmdSequencer`-originated via `imx_seqCmdSplitter`) the command arrived through. | `rejectsSequencerRemoteJetsonCommandWhenJetsonOff`, `forwardsSequencerRemoteJetsonCommandWhenJetsonOn` |
| FP-016 | FPManager shall emit `WARN_STATE_ENTERED`/`WARN_STATE_EXITED` for i.MX, peripheral, and the aggregate Jetson die whenever the corresponding domain transitions into or out of `ThermalStates.WARN`, without asserting any protection output, changing `FP_STATE`, or otherwise taking protective action. | `imxWarnStateEntersAndExitsWithoutShutdown`, `peripheralWarnStateEntersAndExitsWithoutShutdown`, `jetsonWarnStateAggregatesAcrossSensors` |

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
| 2026-07-23 | Added tick-driven peripheral Fault Mode recovery; a valid non-FAULT reading emits `healthy` and returns FPManager to Safe Mode. |
| 2026-07-23 | Added redundant Jetson FAULT monitoring while already in Fault Mode; Jetson faults are reported, commanded OFF, and keep FPManager faulted until cleared. |
| 2026-07-23 | Added `FP_STATE_CHANGED` transition events for operator-visible mode changes. |
| 2026-07-23 | Routed remote Jetson commands through FPManager so commands are rejected locally while the Jetson is known OFF instead of being sent into the hub transport. |
| 2026-07-23 | Invalidated cached Jetson thermal readings whenever Jetson OFF is commanded or observed, preventing stale Jetson FAULT readings from retriggering on HPC re-entry. |
| 2026-07-27 | Fixed a CdhCore `FatalAnnounce`/`FatalReceive` port-arity conflict introduced by connecting `FPManager.fatalIn` directly to `CdhCore.events.FatalAnnounce`: added `scalesSvc.FatalRelay`, swapped in as CdhCore's `fatalHandler` instance via `CdhCoreFatalHandlerConfig.fpp`, and a dedicated `imx_realFatalHandler` instance, so the fatal path is now `EventManager -> FatalRelay -> FPManager -> imx_realFatalHandler` without a second connection on either scalar port. |
| 2026-07-27 | Diagnosed and fixed the i.MX not actually powering off on Emergency Shutdown: `ImxDeployment` runs as a systemd service with `Restart=on-failure`, and every attempt to run `poweroff -f` from a forked child of that service (or to ask systemd via `systemctl/poweroff --no-block`) lost the race against systemd's `KillMode=control-group` reaping the service's cgroup once `FatalHandler` aborted the process. Replaced this with a direct `reboot(RB_POWER_OFF)` syscall (confirmed correct on hardware: `RB_POWER_OFF`, not `RB_AUTOBOOT`) with a `systemd-run`-detached `poweroff -f` fallback, and added `PLATFORM_POWEROFF_SYSCALL_FAILED`/`PLATFORM_POWEROFF_FALLBACK_FAILED` diagnostic events. |
| 2026-07-27 | Decoupled `triggerPlatformPoweroff()` from the state machine's `$fatal` dispatch: it is now called directly and unconditionally from both `fatalIn_handler` and `triggerImxEmergencyShutdown`, guarded by a one-shot `m_platformPoweroffTriggered` flag, so the poweroff attempt cannot be skipped by the state machine, the Jetson/peripheral output calls, or anything else on either path. Confirmed on hardware via fault injection (`MCP_IMX_FAULT_HIGH_PRM_SET` / `IMX_CPU_FAULT_HIGH_PRM_SET`) that the i.MX now powers off instead of the flight software merely restarting. Added `emergencyShutdownProtectedOutputsAreLatchedAcrossRepeatedFatals` to cover the resulting cross-path latching behavior (FP-014). |
| 2026-07-27 | Fixed a crash: running the `run-ml.bin` sequence while the Jetson was powered off triggered `FW_ASSERT` in `Svc::ComStub::dataIn_handler` (`lib/fprime/Svc/ComStub/ComStub.cpp:28`) and killed the flight software, because `imx_seqCmdSplitter.RemoteCmd[0]` (the `CmdSequencer`-originated remote path) was wired straight to `imx_hub.cmdDispIn[1]` with no Jetson-power-state gate at all -- only the GDS-direct path through `imx_cmdSplitter` was gated. `remoteJetsonCmdIn`/`remoteJetsonCmdOut`/`remoteJetsonCmdResponseIn`/`remoteJetsonCmdResponseOut` are now `[2]`-sized arrays (index 0 = GDS-direct, index 1 = sequencer), with `portNum` threaded straight through so the same gate rejects both sources identically with `BUSY` instead of crashing. Added `rejectsSequencerRemoteJetsonCommandWhenJetsonOff`/`forwardsSequencerRemoteJetsonCommandWhenJetsonOn` (FP-015). |
| 2026-07-27 | Added `WARN_STATE_ENTERED`/`WARN_STATE_EXITED` as a read-only observability layer alongside the existing `FAULT`-triggered protection actions: i.MX and peripheral each get a one-shot latch, Jetson uses a single aggregate latch across all nine sensors (mirroring how Jetson `FAULT` is already aggregated), checked directly inside each reading handler as soon as a new reading arrives rather than waiting for the next health-check tick. No `FP_STATE` change, no protection output, and no interaction with the `FPStateMachine` (FP-016). |
