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
| output port | imxThermalStateOut | ThermalStateOut | Outputs the evaluated IMX thermal state. |
| output port | imxThermalReadingOut | ThermalReadingPort | Sends the complete IMX thermal reading to FPManager. |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | IDLE state flag |
| WARN | WARN state flag |
| FAULT | FAULT state flag |

## Parameters

| **Name** | **Description** |
| --- | --- |
| IMX_CPU_IDLE_LOW | IMX cpu idle low parameter |
| IMX_CPU_IDLE_HIGH | IMX cpu idle high parameter |
| IMX_CPU_WARN_LOW | IMX cpu warn low parameter |
| IMX_CPU_WARN_HIGH | IMX cpu warn high parameter |
| IMX_CPU_FAULT_LOW | IMX cpu fault low parameter |
| IMX_CPU_FAULT_HIGH | IMX cpu fault high parameter |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| imx_cpu_temp_read | Outputs a struct called ThermalReading, which sends sensor id, location, timestamp and temperature. |
| IMX_CPU_IDLE_LOW / IMX_CPU_IDLE_HIGH / IMX_CPU_WARN_LOW / IMX_CPU_WARN_HIGH / IMX_CPU_FAULT_LOW / IMX_CPU_FAULT_HIGH | Current i.MX CPU thermal threshold values, published for GDS readback. |

## Events

| **Name** | **Description** |
| --- | --- |
| FAIL_TO_READ_TEMP | Warning emitted when the OSAL read of the thermal zone file fails. |
| THRESHOLDS_MISCONFIGURED | Warning emitted once when the six thresholds stop being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`); does not re-fire until the ordering is fixed and broken again. Checked both on every `parameterUpdated()` call and at the top of `doEvaluate()` (since thresholds are read live via `paramGet_*` rather than cached). Purely informational -- no protective action is taken. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| ITM-001 | Test whether within 2 cycles the count of telemetry increases in size and that the output data types are correct | in 2 cycles TLM increases to 2, and the data types in each cycle are correct | 100% |
| ITM-002 | Evaluate whether temperature readings produce IDLE, WARN, or FAULT at the configured bounds, including failed reads. | Telemetry readings contain the expected state for 42, 75, 80, 0, and -30 degrees C. | 100% |
| ITM-003 | `thresholdsMisconfiguredEmitsOnceOnTransition`: exercises the `thresholdsAreOrdered()` predicate directly with both the real defaults (ascending, valid) and the real-world mistake of lowering WARN_HIGH below IDLE_HIGH (inverted, invalid). `ImxThermalManagerTesting` already confirms the full `doEvaluate()` integration path stays silent across five ticks with the same valid defaults. | Predicate returns true/false as expected for each configuration | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial Implementation complete. Still needs temperature parameters, state machine, SSM STATE port, and Unit tests to verify these. | Luca Lanzillotta |
| 1.1.0 | Revised initial implementation. Added parameter evaluation, state machine, state flag output, and SSM port. 
Missing Unit test | Luca Lanzillotta |
| 1.1.1 | Revised initial implementation. Added Unit Testing for Parameter checking | Luca Lanzillotta |
| 1.2.0 | Added `THRESHOLDS_MISCONFIGURED`, emitted once when the thresholds are found out of ascending order, to catch misconfiguration that previously silently misclassified readings as FAULT. Added a `parameterUpdated()` override (previously absent -- thresholds were read live with no update hook) and a public `setTempPath()` test seam. Fixed the pre-existing broken unit test (stale accessor/port names, `component.setTempPath` referencing a nonexistent method, missing `deinit()`) and added coverage for the new check. | Luca Lanzillotta |
| 1.3.0 | Added telemetry readback for all six current IMX CPU threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
