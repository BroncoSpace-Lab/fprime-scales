# scalesSvc::SpacecraftStateManager

## Status: Not Yet Implemented

`SpacecraftStateManager.cpp` and `SpacecraftStateManager.hpp` are both empty
placeholder files (`// placeholder. Use fprime-util impl`) -- there is no
component class, no handler implementations, and no behavior of any kind.
`SpacecraftStateManager.fpp` declares only two ports (`stateReq` in,
`stateNow` out) with the rest of the file left as commented-out FPP
boilerplate (example command/telemetry/event/parameter). The `CMakeLists.txt`
unit-test block is also commented out, and no `test/ut` directory exists.
`SpacecraftStateManager` is not instantiated in either `ImxDeployment` or
`JetsonDeployment`'s `instances.fpp`/`topology.fpp`.

Because there is no implementation, there is nothing to unit-test yet --
tests would only be able to exercise the auto-generated `ComponentBase`
scaffolding, not real behavior. This section should be replaced with real
`## Requirements`/`## Unit Tests` content (matching the rest of the
`scalesSvc` audit: `Verified By`/`Verifies` traceability,
`RecordProperty("requirement", ...)` tags, measured coverage) once an actual
implementation exists.

## Planned Design (not yet implemented)

The sections below describe the intended design as drafted before
implementation began. They are aspirational, not a description of current
behavior -- nothing here should be treated as accurate until the component
is actually built. Several referenced concepts (a `scalesSvc::PowerManager`
component, `GET_SCALES_STATE`/`SET_SCALES_STATE` commands, a `CurrentState`/
`ReqState` parameter pair) do not exist anywhere else in `scalesSvc` today.

SCALES system state manager -- intended to own a top-level operating mode for
the whole SCALES stack (Safe Mode, Nominal Mode, four HPC power sub-modes,
and Thermal Recovery Mode), switchable by ground command or by request from
other SCALES components via the `stateReq`/`stateNow` port pair already
declared in the `.fpp`.

### Port Descriptions
| Kind | Name | Description |
|---|---|---|
| output | `stateNow` | Current mode/state of the system. |
| async input | `stateReq` | Requested mode/state of the system, either from ground command or another SCALES component. |

### Component States (planned)
| Name | Description | Enter Mode Triggers | Exit Mode Triggers | IMX Status | Jetson Status |
|---|---|---|---|---|---|
| Safe Mode | Comms on, a power-management component running but not turning anything on; can collect data/telemetry but makes no decisions. | Coming out of a FATAL. | Ground command | On, reduced power mode. Maintaining health-critical and comms operations only. | Off. |
| Nominal Mode | Jetson is on but idle to perform basic file management and uplink/downlink tasks with the IMX. | --- | Command to switch to HPC mode, or a FATAL error. | On. | On, but idle. |
| Minimal HPC Mode | 15W power mode on the Jetson. Able to do basic tasks while minimizing power consumption. Default HPC mode. | IMX/Ground command | Power-mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, 15W power mode |
| Balanced HPC Mode | 30W power mode on the Jetson. Can complete more computationally intensive tasks than in Minimal HPC Mode, but still managing power consumption. | IMX/Ground command | Power-mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in 30W power mode. |
| Extra HPC Mode | 50W power mode on the Jetson. Most performance possible while capping power consumption on the Jetson. | IMX/Ground command | Power-mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in 50W power mode. |
| Maximum HPC Mode | MAXN power mode on the Jetson. Absolute maximum in both power consumption and performance. Best for intense computations or operations. | IMX/Ground command | Power-mode switch threshold reached. <br> HPC task finished -> return to Nominal mode. <br> Jetson temperature too hot, enter Thermal Recovery Mode. | On | On, in MAXN power mode. |
| Thermal Recovery Mode | If the Jetson got too hot while in HPC mode, Thermal Recovery Mode is triggered. Turns off the Jetson and waits for temperature to stabilize again. | Jetson temperature too hot while in HPC mode. | Jetson temperature stabilized -> return to Nominal mode. | On | Off |

![State Machine Diagram](<fprime-scales-reference-State Machine.png>)

### Planned Parameters
| Name | Description |
|---|---|
| CurrentState | Current system state. |
| ReqState | Requested state to change into. |

### Planned Commands
| Name | Description |
|---|---|
| GET_SCALES_STATE | Command to return the current state of the SCALES system. |
| SET_SCALES_STATE | Command to change the state of the SCALES system. |

Note: today's `.fpp` has no `command`, `param`, `event`, or `telemetry`
declarations at all -- these would all need to be added before any of the
above could be implemented.

## Change Log
| Date | Description |
|---|---|
| --- | Initial draft (aspirational design, no implementation). |
| 2026-07-28 | SDD accuracy audit: the previous version of this document presented the planned design as if it were the current, implemented behavior (Requirements table, Commands, Parameters, Unit Tests placeholders), but `SpacecraftStateManager.cpp`/`.hpp` are empty placeholder files with no component class or handlers, the component is not instantiated in any deployment topology, and its unit-test CMake block is commented out. Rewrote this document to lead with that status and moved the original content into a clearly-labeled "Planned Design (not yet implemented)" section so it is not mistaken for a description of current behavior. |
