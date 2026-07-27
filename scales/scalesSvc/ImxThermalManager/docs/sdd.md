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
| async input port | imxCpuTemp | Svc.Sched | Handler invoked by a rate group, takes care of reading from `/sys/class/thermal/thermal_zone0/temp` at the rate group interval |

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

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| ITM-001 | Test whether within 2 cycles the count of telemetry increases in size and that the output data types are correct | in 2 cycles TLM increases to 2, and the data types in each cycle are correct | 100% |
| ITM-002 | Evaluate if change in parameter puts system in a IDLE, WARN, or FAULT state. Iterate through each bound | TODO | TODO |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial Implementation complete. Still needs temperature parameters, state machine, SSM STATE port, and Unit tests to verify these. | Luca Lanzillotta |
| 1.1.0 | Revised initial implementation. Added parameter evaluation, state machine, state flag output, and SSM port. 
Missing Unit test | Luca Lanzillotta |
| 1.1.1 | Revised initial implementation. Added Unit Testing for Parameter checking | Luca Lanzillotta |