# Components::ImxThermalManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

The ImxThermalManager will periodically read from the the thermalzone buffer in `/sys/class/thermal/thermal_zone0/temp` and update the GDS with that data as telemetry in degrees celsius.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| ITM-001 | ImxThermalManager must read the CPU temperature from the linux kernel and update it as telemetry per an interal of time | Unit Test |

## Design

### Diagrams

### Typical Usage

Open the GDS and go to telemetry channels, there the user can see the updated temperature values of the IMX CPU.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Handler invoked by a rate group, takes care of reading from `/sys/class/thermal/thermal_zone0/temp` at the rate group interval |
| output port | imxThermalReadingOut | ThermalReadingPort | Sends the complete IMX thermal reading to FPManager. |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | IDLE state flag |
| WARN | WARN state flag |
| FAULT | FAULT state flag |

## Parameters

The six IDLE/WARN/FAULT thresholds are bundled into one `TempBounds` struct
parameter (`faultLow, warnLow, idleLow, idleHigh, warnHigh, faultHigh`,
ascending left to right) instead of six separate scalars, so GDS shows one
row instead of six and a PRM_SET always supplies a complete,
self-consistent set of bounds.

The update is **gated**: it is only adopted as the active bounds if
`thresholdsAreOrdered()` holds (`faultLow <= warnLow <= idleLow <= idleHigh
<= warnHigh <= faultHigh`). A rejected update never takes effect -- the
component keeps using its last-known-good bounds -- because a misconfigured
critical boundary can otherwise cause FPManager to misclassify a reading and
assert an emergency shutdown. See `THRESHOLDS_MISCONFIGURED` below.

| **Name** | **Description** |
| --- | --- |
| IMX_CPU_BOUNDS | `TempBounds` (id 0x00), IMX CPU IDLE/WARN/FAULT bounds. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| imx_cpu_temp_read | Outputs a struct called ThermalReading, which sends sensor id, location, timestamp and temperature. |
| IMX_CPU_BOUNDS | `TempBounds` currently active for the IMX CPU. Republished unconditionally every evaluate cycle (not just on boot/change), so a GDS session that connects mid-run still sees it on the next tick. A rejected PRM_SET leaves this (and classification) unchanged. |

## Events

| **Name** | **Description** |
| --- | --- |
| FAIL_TO_READ_TEMP | Warning emitted when the OSAL read of the thermal zone file fails. |
| THRESHOLDS_MISCONFIGURED | Warning emitted every time a newly-set `IMX_CPU_BOUNDS` update is rejected for not being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`). Fires on every rejected attempt, not just the first, since a rejected update never takes effect -- the component keeps its previous bounds. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| ITM-001 | Test whether within 2 cycles the count of telemetry increases in size and that the output data types are correct | in 2 cycles TLM increases to 2, and the data types in each cycle are correct | 100% |
| ITM-002 | Evaluate whether temperature readings produce IDLE, WARN, or FAULT at the configured bounds, including failed reads. | Telemetry readings contain the expected state for 42, 75, 80, 0, and -30 degrees C (80, exactly at WARN_HIGH, is WARN -- see the boundary-inclusivity fix in the Change Log). | 100% |
| ITM-003 | `boundsUpdateGating`: applies a valid `TempBounds` update and confirms it is adopted and telemetered, applies an inverted WARN_HIGH/IDLE_HIGH configuration and confirms it is rejected (bounds unchanged, event fires), confirms repeating the same bad attempt fires again, then confirms a second valid update is adopted normally. | Active bounds, telemetry, and `THRESHOLDS_MISCONFIGURED` event count match expectations at each step | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial Implementation complete. Still needs temperature parameters, state machine, SSM STATE port, and Unit tests to verify these. | Luca Lanzillotta |
| 1.1.0 | Revised initial implementation. Added parameter evaluation, state machine, state flag output, and SSM port. 
Missing Unit test | Luca Lanzillotta |
| 1.1.1 | Revised initial implementation. Added Unit Testing for Parameter checking | Luca Lanzillotta |
| 1.2.0 | Added `THRESHOLDS_MISCONFIGURED`, emitted once when the thresholds are found out of ascending order, to catch misconfiguration that previously silently misclassified readings as FAULT. Added a `parameterUpdated()` override (previously absent -- thresholds were read live with no update hook) and a public `setTempPath()` test seam. Fixed the pre-existing broken unit test (stale accessor/port names, `component.setTempPath` referencing a nonexistent method, missing `deinit()`) and added coverage for the new check. | Luca Lanzillotta |
| 1.3.0 | Added telemetry readback for all six current IMX CPU threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
| 1.4.0 | Replaced the six individual scalar threshold parameters and their six scalar readback channels with one `TempBounds` struct parameter and one `TempBounds` struct telemetry channel, so GDS shows one bounds row instead of six. Bounds are now cached (`m_activeBounds`) instead of read live from `paramGet_*` every tick, and a `PRM_SET` is gated: it's only adopted if it passes `thresholdsAreOrdered()`, otherwise the component keeps its last-known-good bounds and `THRESHOLDS_MISCONFIGURED` fires (now on every rejected attempt, not just the first, since misconfiguration never takes effect). Also fixed a boundary-inclusivity bug in `doEvaluate()`: a reading exactly at `WARN_HIGH` was claimed by FAULT (inclusive) before WARN (exclusive) could claim it; WARN now claims its own upper boundary, symmetric with its already-inclusive lower boundary. Removed the dead `imxThermalStateOut` port (never connected in the topology or written in code) and the now-orphaned `ThermalStateOut`/`ThermalStateIn` port types. | Luca Lanzillotta |
| 1.4.1 | `IMX_CPU_BOUNDS` telemetry now republishes unconditionally every evaluate cycle instead of only on boot/change, so a GDS session that connects after boot still sees the current bounds on the next tick rather than missing the one-time boot publish. | Luca Lanzillotta |
