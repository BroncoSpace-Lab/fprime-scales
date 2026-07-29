# Components::McpManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

The McpManager will perform i2c read write operations to the three onboard MCP9808T sensors on the SCALES Leviathan Board. It will output the information as a struct as telemetry. The MCP Manager will also have an internal state machine that will parse parameters and output a state dependent on the current thermal readings of the system.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| MCPM-001 | Collect thermal readings as a ThermalReading defined type periodically and output it as telemetry | Unit Test/Inspection |
| MCPM-002 | Output STATES dependent on configurable thermal parameter zones to the Spacecraft State Manager | Unit Test/Inspection |

## Design

### Diagrams

!image.png

### Typical Usage

End user will check temperature parameters in real time. If thermal zones increase or decrease, states will be outputted to notify SpaceCraftStateManager of system health.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| output port | mcpWriteRead | Drv.I2cWriteRead | Create output port to use the Drv.I2cWriteRead type to bring in i2cwriteread functionality to talk to sensors |
| async input port | pollTempData | Svc.Sched | Port to be invoked by scheduler, causes component to print collected data |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | Idle state for idle temperature ranges |
| WARN | Warn state for warn temperature ranges |
| FATAL | Fatal state for fatal temperature ranges |

## Parameters

Each sensor's six IDLE/WARN/FAULT thresholds are bundled into one
`TempBounds` struct parameter (`faultLow, warnLow, idleLow, idleHigh,
warnHigh, faultHigh`, ascending left to right) instead of six separate
scalars, so GDS shows one row per sensor instead of six and a PRM_SET always
supplies a complete, self-consistent set of bounds.

A `TempBounds` update is **gated**: it is only adopted as the sensor's active
bounds if `thresholdsAreOrdered()` holds (`faultLow <= warnLow <= idleLow <=
idleHigh <= warnHigh <= faultHigh`). A rejected update never takes effect --
the sensor keeps using its last-known-good bounds -- because a misconfigured
critical boundary can otherwise cause FPManager to misclassify a reading and
assert an emergency shutdown. See `THRESHOLDS_MISCONFIGURED` below.

| **Name** | **Description** |
| --- | --- |
| MCP_IMX_BOUNDS | `TempBounds` (id 0x00), IMX sensor bounds. |
| MCP_PERIPHERAL_BOUNDS | `TempBounds` (id 0x01), Peripheral sensor bounds. |
| MCP_JETSON_BOUNDS | `TempBounds` (id 0x02), Jetson-board sensor bounds. |

## Telemetry

Telemetry is grouped by subsystem: each sensor's raw `ThermalReading` is
immediately followed by its currently active `TempBounds`. Both are
republished unconditionally every evaluate cycle (not just on boot/change),
so a GDS session that connects mid-run still sees current bounds on the
next tick rather than waiting for the next `PRM_SET`.

| **Name** | **Description** |
| --- | --- |
| IMX_TEMP | `ThermalReading` for the IMX MCP9808 sensor (id 0x00). |
| MCP_IMX_BOUNDS | `TempBounds` (id 0x10) currently active for the IMX sensor. Only updated when a PRM_SET passes the ordering gate -- a rejected update leaves this (and the sensor's classification) unchanged. |
| PERIPHERAL_TEMP | `ThermalReading` for the peripheral MCP9808 sensor (id 0x01). |
| MCP_PERIPHERAL_BOUNDS | `TempBounds` (id 0x11) currently active for the peripheral sensor. |
| JETSON_TEMP | `ThermalReading` for the Jetson-board MCP9808 sensor (id 0x02). |
| MCP_JETSON_BOUNDS | `TempBounds` (id 0x12) currently active for the Jetson-board sensor. |

## Events

| **Name** | **Description** |
| --- | --- |
| FAIL_TO_READ_TEMP_AT | Warning emitted when a specific sensor's I2C read fails. |
| FAIL_TO_READ_TEMP | Warning emitted when one or more sensors failed to read this cycle. |
| THRESHOLDS_MISCONFIGURED | Warning emitted every time a sensor's newly-set bounds are rejected for not being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`). Fires on every rejected attempt, not just the first, since a rejected update never takes effect -- the sensor keeps its previous bounds. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| MCPMUT-001 | Verify that I2cWriteRead cycle is complete, then prints data over 2 cycles | Correct | 100% |
| MCPMUT-002 | Verify that Temperature parameters output expected STATES for each sensor  | Works in GDS and a cycle in unit tests | 70% |
| MCPMUT-003 | `boundsUpdateGating`: applies a valid `TempBounds` update to sensor 0 (IMX) and confirms it is adopted and telemetered, applies an inverted WARN_HIGH/IDLE_HIGH configuration and confirms it is rejected (bounds unchanged, event fires), confirms repeating the same bad attempt fires again, then confirms a second valid update is adopted normally. | Active bounds, telemetry, and `THRESHOLDS_MISCONFIGURED` event count match expectations at each step | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial component implementation without state machine, parameters, or port to SSM. Just telemetry outputting | Luca Lanzillotta, Dat Nguyen |
| 1.1.9 | Added Unit tests, parameters, and State output | Dat Nguyen |
| 1.2.0 | Split the six shared thresholds into per-sensor sets (18 params total: IMX/PERIPHERAL/JETSON) so each sensor can be tuned independently instead of affecting all three. | Luca Lanzillotta |
| 1.3.0 | Added `THRESHOLDS_MISCONFIGURED`, emitted once when a sensor's thresholds are found out of ascending order (e.g. lowering WARN_HIGH below IDLE_HIGH), to catch misconfiguration that previously silently misclassified readings as FAULT. Fixed the pre-existing broken unit test (stale accessor names, private-member access, missing `deinit()`) and added coverage for the new check. | Luca Lanzillotta |
| 1.4.0 | Added telemetry readback for all 18 current per-sensor MCP threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
| 1.5.0 | Replaced the 18 individual scalar threshold parameters and their 18 scalar readback channels with one `TempBounds` struct parameter and one `TempBounds` struct telemetry channel per sensor (IMX/PERIPHERAL/JETSON), so GDS shows one bounds row per sensor instead of six. Bounds updates are now gated: a `PRM_SET` is only adopted if it passes `thresholdsAreOrdered()`, otherwise the sensor keeps its last-known-good bounds and `THRESHOLDS_MISCONFIGURED` fires (now on every rejected attempt, not just the first, since misconfiguration never takes effect). Also fixed a boundary-inclusivity bug in `determineTempState()`: a reading exactly at `WARN_HIGH` was claimed by FAULT (inclusive) before WARN (exclusive) could claim it; WARN now claims its own upper boundary, symmetric with its already-inclusive lower boundary. | Luca Lanzillotta |
| 1.5.1 | Each sensor's `TempBounds` telemetry now republishes unconditionally every evaluate cycle instead of only on boot/change, so a GDS session that connects after boot still sees the current bounds on the next tick rather than missing the one-time boot publish. | Luca Lanzillotta |
