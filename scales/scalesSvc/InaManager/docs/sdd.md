# Components::InaManager

Functional Description: Manager for INA260 current, voltage, and power sensor.

## Usage Examples

InaManager will forward Voltage, Current, and Power from each SCALES subsystem to the Spacecraft State Manager for state decision making.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| INAM-001 | The InaManager will send telemetry data of Voltage, Current, and Power to the GDS with proper formatting | `nominalAllSensorsSucceed`, `conversionHelpersDirect`, `timestampAdvancesAcrossTicks`, `singleRegisterFailureStopsSubsequentReadsForThatSensor`, `powerRegisterFailureAfterCurrentAndVoltageSucceed` |
| INA-002 | The InaManager will forward the telemetry to the SSM for fault protection | `singleRegisterFailureStopsSubsequentReadsForThatSensor`, `allSensorsFailStillForwardsToDataProducer`, `powerRegisterFailureAfterCurrentAndVoltageSucceed` |

## Design

### Diagrams

Add diagrams here

!image.png

### Typical Usage

Component is always running, can be seen in the GDS with live hardware telemetry.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| output port | busWriteRead | Drv.I2cWriteRead | Port for performing I2C write/read transactions with the INA260 sensor |
| async input port | run | Svc.Sched | Input port for sending data each tick |

## Events

| **Name** | **Description** |
| --- | --- |
| I2cReadFailed | INA260 I2C read failed for register 0x{} with status {} |
| SensorReadComplete | INA260 read complete: current {} mA, voltage {} mV, power {} mW |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| INA260_Jetson | PowerReading (id 0) INA260 Jetson subsystem |
| INA260_OBC | PowerReading (id 1) INA260 OBC subsystem |
| INA260_Peripheral | PowerReading (id 2) INA260 Peripheral subsystem |

## Unit Tests

Measured via `fprime-util check --coverage`: **100% line (70/70), 100%
function (8/8), 59.8% branch (49/82)**. The remaining branch gap is
ASan/UBSan instrumentation edges around construction, the same
non-actionable pattern documented throughout this audit (see
HubComAdapter/McpManager/ImxThermalManager SDDs). Each test is tagged with
`RecordProperty("requirement", "<REQ-ID>")`, so running the test binary with
`--gtest_output=xml:<path>` produces a JUnit-style XML report whose
`<testcase>` elements carry that mapping as a machine-checkable artifact
(see `RequirementTraceability/` at the project root).

| Name | Description | Verifies |
|---|---|---|
| `nominalAllSensorsSucceed` | One full tick with all three sensors succeeding: register read order, unit conversion (including a 3-decimal truncation case and a negative/signed current case), per-sensor location/timestamp/sourceId, telemetry write, and the `inaPowerReadOut` fan-out to DataProducer. | INAM-001 |
| `conversionHelpersDirect` | Calls the three private raw-to-engineering-unit conversion helpers directly (friend access): LSB math, the signed current case, and truncation. | INAM-001 |
| `timestampAdvancesAcrossTicks` | Confirms the first tick after boot always reports timestamp 0 regardless of the absolute wall-clock value, and later ticks report elapsed seconds since that first tick. | INAM-001 |
| `singleRegisterFailureStopsSubsequentReadsForThatSensor` | Jetson's VOLTAGE register (2nd of 3) fails: confirms POWER is never attempted afterward, `I2cReadFailed`/`FAIL_TO_READ_TEMP_AT` fire with the correct register/location, no telemetry is written for that sensor, the other two sensors are unaffected, and `inaPowerReadOut` still fires. | INAM-001, INA-002 |
| `allSensorsFailStillForwardsToDataProducer` | All three sensors fail on their first register: confirms `inaPowerReadOut` is still sent once even though nothing was read successfully, so SSM always has a reading to act on. | INA-002 |
| `powerRegisterFailureAfterCurrentAndVoltageSucceed` | Peripheral's POWER register (3rd and last) fails after CURRENT and VOLTAGE both succeed -- the one failure point not reached by the other failure tests. | INAM-001, INA-002 |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| v1.0.0 | Initial implementation. No unit tests existed at this point despite this row's original text -- corrected below. | Dat Nguyen, Luca Lanzillotta |
| 2026-07-31 | Added `InaManagerTester` (`test/ut/`) and wired `register_fprime_ut` into `CMakeLists.txt` -- this component had no test target at all before (the UT block was commented out with a stale note about unfinished work that was never actually implemented). A fake I2C responder drives `busWriteRead` by (address, register), independent per sensor, so per-register success/failure can be controlled precisely. Measured 100% line, 100% function, 59.8% branch coverage. Tagged every test with `RecordProperty("requirement", ...)` and added a `Verifies` column to this table. | Luca Lanzillotta |