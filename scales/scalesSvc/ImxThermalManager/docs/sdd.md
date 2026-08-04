# Components::ImxThermalManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

The ImxThermalManager will periodically read from the the thermalzone buffer in `/sys/class/thermal/thermal_zone0/temp` and update the GDS with that data as telemetry in degrees celsius.

## Requirements

| **Name** | **Description** | **Validation** | **Verified By** |
| --- | --- | --- | --- |
| ITM-001 | ImxThermalManager must read the CPU temperature from the linux kernel and update it as telemetry per an interal of time | Unit Test | `Nominal.ImxThermalManagerTesting`, `Nominal.malformedTempFile` |
| ITM-002 | Classify the reading into IDLE/WARN/FAULT against the configured thermal bounds and output it to FPManager and DataProducer | Unit Test | `Nominal.ImxThermalManagerTesting`, `Nominal.highSideFaultGap` |
| ITM-003 | Gate `IMX_CPU_BOUNDS` parameter updates so a misconfigured (non-ascending) update never takes effect, logging `THRESHOLDS_MISCONFIGURED` on every rejected attempt | Unit Test | `Nominal.boundsUpdateGating`, `Nominal.parameterUpdatedCoverage` |
| ITM-004 | On a read failure, mark the reading `FAILED_READ` and log `FAIL_TO_READ_TEMP`, recovering automatically once reads succeed again | Unit Test | `Nominal.ImxThermalManagerTesting` |

## Design

### Diagrams

### Typical Usage

Open the GDS and go to telemetry channels, there the user can see the updated temperature values of the IMX CPU.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Handler invoked by a rate group, takes care of reading from `/sys/class/thermal/thermal_zone0/temp` at the rate group interval |
| output port | imxThermalReadingOut | ThermalReadingPort | Sends the complete IMX thermal reading to FPManager. |
| output port | cpuThermalReadOut | CpuThermalReadings | Sends the complete IMX thermal reading to DataProducer. |

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
| FAIL_TO_READ_TEMP | Warning emitted when the OSAL read of the thermal zone file fails while in the read-retry (`doReadFail`) state -- i.e. a failure that follows an already-failed read, not the very first attempt. |
| THRESHOLDS_MISCONFIGURED | Warning emitted every time a newly-set `IMX_CPU_BOUNDS` update is rejected for not being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`). Fires on every rejected attempt, not just the first, since a rejected update never takes effect -- the component keeps its previous bounds. |

## Unit Tests

Measured via `fprime-util check --coverage`: **99.1% line (106/107), 100%
function (11/11), 60.9% branch (78/128)**. The one uncovered line is the
final `else` in `doEvaluate()`'s classification -- unreachable given
well-ordered bounds (every `IMX_CPU_BOUNDS` update is gated through
`thresholdsAreOrdered()`, so the FAULT/WARN/IDLE ranges are always
contiguous and exhaustive); it exists only as a defensive catch-all for an
invariant the gating already guarantees.

| **Name** | **Description** | **Verifies** |
| --- | --- | --- |
| `Nominal.ImxThermalManagerTesting` | Boots with a missing temperature file (confirms `FAILED_READ` telemetry and the `FAIL_TO_READ_TEMP` event once retrying), then creates the file and drives readings through 42/75/80/0/-30 degrees C, confirming IDLE/WARN/WARN(boundary)/WARN/FAULT classification. | ITM-001, ITM-002, ITM-004 |
| `Nominal.boundsUpdateGating` | Applies a valid `TempBounds` update and confirms it is adopted and telemetered, applies an inverted WARN_HIGH/IDLE_HIGH configuration and confirms it is rejected (bounds unchanged, event fires), confirms repeating the same bad attempt fires again, then confirms a second valid update is adopted normally. | ITM-003 |
| `Nominal.highSideFaultGap` | Drives a reading strictly between WARN_HIGH and FAULT_HIGH -- a distinct disjunct in `doEvaluate()`'s FAULT condition from the low-side case above -- and confirms it is classified FAULT. | ITM-002 |
| `Nominal.malformedTempFile` | Exercises `readTemperatureFile()` directly against an empty file, non-numeric content, and trailing garbage after an otherwise-valid number, confirming each is rejected; confirms a well-formed file still succeeds. | ITM-001 |
| `Nominal.parameterUpdatedCoverage` | Calls `parameterUpdated()` directly for `IMX_CPU_BOUNDS` and an unrecognized ID (`boundsUpdateGating` only reaches `applyBounds()` directly, bypassing this switch). | ITM-003 |

Each test is tagged with `RecordProperty("requirement", "<REQ-IDs>")`, so
running the test binary with `--gtest_output=xml:<path>` produces a
JUnit-style XML report whose `<testcase>` elements carry that mapping as a
machine-checkable artifact.

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
| 1.5.0 | SDD accuracy audit: fixed a real bug where `doReadFail()`'s failure branch never actually logged `FAIL_TO_READ_TEMP` despite the event being defined and documented -- added the missing `log_WARNING_HI_FAIL_TO_READ_TEMP()` call and a test assertion locking it in. Added the undocumented `cpuThermalReadOut` port (to DataProducer) to Port Descriptions, split the single ITM-001 requirement into ITM-001 through ITM-004 to separately cover reading, classification, bounds gating, and failure handling, and added `Verified By`/`Verifies` traceability between the Requirements and Unit Tests tables. Replaced unverified 100%-coverage claims with measured numbers and added `highSideFaultGap` (the WARN_HIGH/FAULT_HIGH gap, a disjunct the existing test never reached), `malformedTempFile` (empty/non-numeric/trailing-garbage content, calling `readTemperatureFile()` directly), and `parameterUpdatedCoverage` (`parameterUpdated()` was never exercised -- `boundsUpdateGating` calls `applyBounds()` directly) to raise coverage from 88.7%/90.9%/54.7% to 99.1%/100%/60.9% (line/function/branch). Tagged every test with `RecordProperty("requirement", ...)`. | Luca Lanzillotta |
