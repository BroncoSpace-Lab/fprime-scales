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

Thresholds are per-sensor (one set of six each for IMX, PERIPHERAL, and
JETSON) rather than shared, so each sensor's IDLE/WARN/FAULT bounds can be
tuned independently. Set/save opcodes follow `id*2 + 1` / `id*2 + 2`.

| **Name** | **Description** |
| --- | --- |
| MCP_IMX_IDLE_LOW / IDLE_HIGH / WARN_LOW / WARN_HIGH / FAULT_LOW / FAULT_HIGH | F32 (ids 0x00-0x05) IMX sensor thresholds. |
| MCP_PERIPHERAL_IDLE_LOW / IDLE_HIGH / WARN_LOW / WARN_HIGH / FAULT_LOW / FAULT_HIGH | F32 (ids 0x06-0x0B) Peripheral sensor thresholds. |
| MCP_JETSON_IDLE_LOW / IDLE_HIGH / WARN_LOW / WARN_HIGH / FAULT_LOW / FAULT_HIGH | F32 (ids 0x0C-0x11) Jetson-board sensor thresholds. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| IMX_TEMP / PERIPHERAL_TEMP / JETSON_TEMP | `ThermalReading` for each of the three MCP9808 sensors (ids 0x00-0x02). |
| MCP_IMX_* / MCP_PERIPHERAL_* / MCP_JETSON_* (18 channels, ids 0x10-0x21) | Threshold readback channels, published during initialization and whenever a parameter is updated. |

## Events

| **Name** | **Description** |
| --- | --- |
| FAIL_TO_READ_TEMP_AT | Warning emitted when a specific sensor's I2C read fails. |
| FAIL_TO_READ_TEMP | Warning emitted when one or more sensors failed to read this cycle. |
| THRESHOLDS_MISCONFIGURED | Warning emitted once when a sensor's six thresholds stop being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`); does not re-fire until the ordering is fixed and broken again. Purely informational -- no protective action is taken. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| MCPMUT-001 | Verify that I2cWriteRead cycle is complete, then prints data over 2 cycles | Correct | 100% |
| MCPMUT-002 | Verify that Temperature parameters output expected STATES for each sensor  | Works in GDS and a cycle in unit tests | 70% |
| MCPMUT-003 | `thresholdsMisconfiguredEmitsOnceOnTransition`: directly configures sensor 0 (IMX) with an inverted WARN_HIGH/IDLE_HIGH ordering and verifies the event fires once, stays silent on repeat, and re-fires after being fixed and broken again. | `THRESHOLDS_MISCONFIGURED` event count matches expectations at each step | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial component implementation without state machine, parameters, or port to SSM. Just telemetry outputting | Luca Lanzillotta, Dat Nguyen |
| 1.1.9 | Added Unit tests, parameters, and State output | Dat Nguyen |
| 1.2.0 | Split the six shared thresholds into per-sensor sets (18 params total: IMX/PERIPHERAL/JETSON) so each sensor can be tuned independently instead of affecting all three. | Luca Lanzillotta |
| 1.3.0 | Added `THRESHOLDS_MISCONFIGURED`, emitted once when a sensor's thresholds are found out of ascending order (e.g. lowering WARN_HIGH below IDLE_HIGH), to catch misconfiguration that previously silently misclassified readings as FAULT. Fixed the pre-existing broken unit test (stale accessor names, private-member access, missing `deinit()`) and added coverage for the new check. | Luca Lanzillotta |
| 1.4.0 | Added telemetry readback for all 18 current per-sensor MCP threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
