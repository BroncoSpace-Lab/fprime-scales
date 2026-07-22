# scalesSvc::JetsonManager

Component that manages Jetson power state from the i.MX side and sends Jetson
power-mode requests to the Jetson power-mode manager.

### Typical Usage
`JetsonManager` receives GDS commands for Jetson power state and power mode. The
`REQUEST_JETSON_POWER_STATE` command is first authorized by `FPManager` through
`fpJetsonPowerAuthorize`; this allows `FPManager` to reject Jetson ON requests
unless HPC Mode is enabled. Jetson OFF requests are allowed in Safe Mode and are
handled as an idempotent direct GPIO action when the Jetson is already known
OFF.

Powering the Jetson ON drives the Jetson power GPIO high and completes the
command immediately. A commanded Jetson OFF uses the graceful-ish Jetson-side
shutdown path when the Jetson is known ON: `JetsonManager` sends
`reqJetsonPwrState(OFF)`, waits for `currentJetsonPwrState(OFF)`, waits
`JETSON_POWER_OFF_DELAY_TICKS`, then drives the GPIO low and completes the
command. If the Jetson is already known OFF or the shutdown port is unavailable,
OFF is completed as an idempotent direct GPIO-low action.

Internal FPManager recovery, HPC disable, and emergency OFF requests always use
the direct GPIO-low path. These protection paths must not depend on the
Jetson-side software or communication link being available.

`REQUEST_POWER_MODE` still routes through the Jetson hub link because it requires
the Jetson-side power-mode manager to apply the mode.

## Class Diagram
Add a class diagram here

## Port Descriptions
| Name | Description |
|---|---|
| `currentPwrMode` | Current Jetson power mode reported by the Jetson-side manager. |
| `currentJetsonPwrState` | Current Jetson power state reported by the Jetson-side manager. |
| `fpJetsonPowerRequestIn` | Internal FPManager recovery, HPC disable, and emergency OFF request. Only OFF is acted on, and it drives GPIO low directly. |
| `fpJetsonPowerAuthorize` | Synchronous gate called before `REQUEST_JETSON_POWER_STATE` executes. |
| `fpJetsonPowerStateOut` | Current Jetson power state reported to FPManager after JetsonManager commands or receives a power-state update. |
| `schedIn` | Rate-group tick used for deferred power-mode timeout handling. |
| `reqPwrMode` | Outbound request to the Jetson-side power-mode manager. |
| `reqJetsonPwrState` | Jetson-side graceful shutdown request path used by commanded OFF when the Jetson is known ON. |
| `gpioSet` | GPIO write used to physically control Jetson power. |

## Component States
| Name | Description |
|---|---|
| No pending mode command | Normal state after initialization or command completion. |
| Pending mode command | Waiting for Jetson to report the requested power mode after a mode request. |

## Sequence Diagrams
Add sequence diagrams here

## Parameters
| Name | Description |
|---|---|
| `JETSON_POWER_OFF_DELAY_TICKS` | Number of scheduler ticks to wait after the Jetson reports OFF before cutting GPIO power in the graceful commanded-OFF path. |

## Commands
| Name | Description |
|---|---|
| `REQUEST_POWER_MODE` | Requests a Jetson power-mode change through the Jetson-side manager and waits for confirmation or timeout. |
| `REQUEST_JETSON_POWER_STATE` | Requests Jetson ON or OFF after FPManager authorization. ON drives GPIO high. OFF is graceful when Jetson is known ON, and direct/idempotent when it is already known OFF. |

## Events
| Name | Description |
|---|---|
| `POWER_MODE_REQUESTED` | A Jetson power-mode request was sent. |
| `POWER_MODE_RECEIVED` | A current Jetson power mode was received. |
| `JETSON_POWER_STATE_REQUESTED` | A Jetson power-state command was accepted. |
| `JETSON_POWER_STATE_RECEIVED` | A current Jetson power state was received. |
| `JETSON_POWER_STATE_TIMEOUT` | A deferred Jetson power-state operation timed out. |

## Telemetry
| Name | Description |
|---|---|
| `JetsonPowerMode` | Last reported Jetson power mode. |
| `JetsonPowerState` | Last commanded or reported Jetson power state. |

## Unit Tests
| Name | Description | Output | Coverage |
|---|---|---|---|
| Build validation | The generated component and deployment build successfully. | `ninja -C build-fprime-automatic-imx8x ImxDeployment` | Confirms command handlers and topology compile. |

## Requirements
| Name | Description | Validation |
|---|---|---|
| JM-001 | Jetson ON shall require successful FPManager authorization. | Command handler checks `fpJetsonPowerAuthorize` before GPIO ON. |
| JM-002 | Jetson OFF shall be accepted in Safe Mode and shall not depend on the Jetson hub link when the Jetson is already known OFF. | Known-OFF requests complete through direct GPIO low. |
| JM-003 | FPManager emergency/recovery OFF shall immediately remove Jetson power without depending on Jetson-side software. | `fpJetsonPowerRequestIn` handles OFF through direct GPIO low. |
| JM-004 | Jetson power-mode requests shall be deferred until the requested mode is reported or timeout occurs. | `REQUEST_POWER_MODE`, `currentPwrMode`, and `schedIn` maintain pending command state. |
| JM-005 | Commanded Jetson OFF while the Jetson is known ON shall request Jetson-side shutdown before cutting GPIO power. | `REQUEST_JETSON_POWER_STATE(OFF)` sends `reqJetsonPwrState(OFF)` and waits for `currentJetsonPwrState(OFF)`. |

## Change Log
| Date | Description |
|---|---|
| 2026-07-22 | Documented FPManager authorization and direct GPIO OFF behavior. |
| 2026-07-22 | Added Jetson power-state reporting from JetsonManager to FPManager. |
| 2026-07-22 | Restored graceful commanded Jetson OFF when Jetson is known ON while keeping FPManager protection OFF direct. |
| 2024-02-28 | Initial Draft |
