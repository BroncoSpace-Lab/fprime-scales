# Components::JetsonThermalManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

Used for acquiring temperature information about the 9 thermal zones on the Jetson Orin AGX. Additionally, this component implements a state machine to determine the device thermal health which outputs the state to the SSM

## Requirements

| **Name** | **Description** | **Validation** | **Verified By** |
| --- | --- | --- | --- |
| JTM-001 | Component must read all 9 thermal zones of the Jetson Orin AGX every cycle and output each reading as telemetry | Unit Test | `Nominal.JetsonThermalManagerUnitTester` |
| JTM-002 | Classify each of the nine readings into IDLE/WARN/FAULT against the shared `JETSON_BOUNDS` thresholds and output them to FPManager (`jetsonThermalReadingOut`) and DataProducer (`jetsonThermalReadOut`) | Unit Test | `Nominal.JetsonThermalManagerUnitTester` |
| JTM-003 | Gate `JETSON_BOUNDS` parameter updates so a misconfigured (non-ascending) update never takes effect, logging `THRESHOLDS_MISCONFIGURED` on every rejected attempt | Unit Test | `Nominal.boundsUpdateGating`, `Nominal.parameterUpdatedCoverage` |
| JTM-004 | A zone whose thermal-zone file can't be opened, read, or parsed is marked `NOT_USED` for that cycle rather than failing the whole read; the other eight zones are read and evaluated normally | Unit Test | `Nominal.JetsonThermalManagerUnitTester` |

## Design

### Diagrams

!image.png

### Typical Usage

Component will output telemetry to GDS, and will determine device state based on thermal levels.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Port invoked by the rate group to read the nine Jetson thermal zones and output their readings as telemetry. |
| output port | jetsonThermalReadingOut | ThermalReadingPort | Sends each complete Jetson thermal reading to FPManager, one call per zone per cycle. |
| output port | jetsonThermalReadOut | JetsonThermalReadings | Sends all nine Jetson thermal readings in a single call to DataProducer. |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | IDLE state, between IDLE LOW and IDLE HIGH |
| WARN | WARN state, between WARN LOW/IDLE LOW and IDLE HIGH/WARN HIGH |
| FAULT | FAULT state, between FAULT LOW/WARN LOW and WARN HIGH/FAULT HIGH |
| NOT_USED | This zone's `readTemp()` failed this cycle (missing/unreadable/malformed thermal-zone file) -- the reading is left out of thermal-fault classification entirely rather than defaulting to a real state. |

### Known Limitation: the per-zone read-failure path is unreachable

`doRead()`'s per-zone loop marks a zone `NOT_USED` on a failed read but does
not set `m_successfulRead = false` (that line is present in the source but
commented out). Since `m_successfulRead` starts `true` and is never set to
`false` anywhere in the component, the state machine's `fail` signal --
and therefore the `doReadFail` action -- can never actually fire; every
cycle unconditionally proceeds via `success` regardless of how many zones
failed to read. This is why `doReadFail` (and the "read failure" `printf`
inside it) shows up as uncovered in the coverage report: it is genuinely
dead code under the current implementation, not a testing gap. No event is
emitted anywhere for a per-zone read failure -- `NOT_USED` telemetry is the
only externally visible sign.

## Parameters

The six IDLE/WARN/FAULT thresholds -- shared across all nine on-die zones,
since they're all on one die -- are bundled into one `TempBounds` struct
parameter (`faultLow, warnLow, idleLow, idleHigh, warnHigh, faultHigh`,
ascending left to right) instead of six separate scalars, so GDS shows one
row instead of six and a PRM_SET always supplies a complete,
self-consistent set of bounds.

The update is **gated**: it is only adopted as the active bounds if
`thresholdsAreOrdered()` holds (`faultLow <= warnLow <= idleLow <= idleHigh
<= warnHigh <= faultHigh`). A rejected update never takes effect -- the
component keeps using its last-known-good bounds for all nine zones --
because a misconfigured critical boundary can otherwise cause FPManager to
misclassify a reading and assert an emergency shutdown. See
`THRESHOLDS_MISCONFIGURED` below.

| **Name** | **Description** |
| --- | --- |
| JETSON_BOUNDS | `TempBounds` (id 0x00), IDLE/WARN/FAULT bounds shared across all nine Jetson thermal zones. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| jetson_cpu_temp_read | Telemetry channel reporting the current Jetson CPU thermal zone temperature. |
| jetson_gpu_temp_read | Telemetry channel reporting the current Jetson GPU thermal zone temperature. |
| jetson_cv0_temp_read | Telemetry channel reporting the current Jetson CV0 thermal zone temperature. |
| jetson_cv1_temp_read | Telemetry channel reporting the current Jetson CV1 thermal zone temperature. |
| jetson_cv2_temp_read | Telemetry channel reporting the current Jetson CV2 thermal zone temperature. |
| jetson_soc0_temp_read | Telemetry channel reporting the current Jetson SOC0 thermal zone temperature. |
| jetson_soc1_temp_read | Telemetry channel reporting the current Jetson SOC1 thermal zone temperature. |
| jetson_soc2_temp_read | Telemetry channel reporting the current Jetson SOC2 thermal zone temperature. |
| jetson_tj_temp_read | Telemetry channel reporting the current Jetson TJ thermal zone temperature. |
| JETSON_BOUNDS | `TempBounds` currently active, shared across all nine zones. Republished unconditionally every evaluate cycle (not just on boot/change), so a GDS session that connects mid-run still sees it on the next tick. A rejected PRM_SET leaves this (and classification for all nine zones) unchanged. |

## Events

| **Name** | **Description** |
| --- | --- |
| THRESHOLDS_MISCONFIGURED | Warning emitted every time a newly-set `JETSON_BOUNDS` update is rejected for not being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`). Fires on every rejected attempt, not just the first, since a rejected update never takes effect -- the component keeps its previous bounds. |

## Unit Tests

Measured via `fprime-util check --coverage`: **89.3% line (142/159), 91.7%
function (11/12), 53.5% branch (107/200)**. The one uncovered function is
`doReadFail()` -- see "Known Limitation" above, it is genuinely
unreachable in the current implementation, not an untested path. The
remaining uncovered lines/branches are that dead function's body plus the
usual ASan/UBSan instrumentation edges around construction (the same
non-actionable pattern documented in WatchdogManager/McpManager/
ImxThermalManager).

| **Name** | **Description** | **Verifies** |
| --- | --- | --- |
| `Nominal.JetsonThermalManagerUnitTester` | Boots and confirms the default `JETSON_BOUNDS` telemetry publish, then drives all nine zones through a first set of readings (IDLE/WARN/FAULT mix, including a WARN_HIGH boundary case) and confirms each zone's telemetry (temperature, sensor id, location, state). Then removes three of the nine zones' files and drives a second cycle, confirming those three read as `NOT_USED` while the other six continue to be read and classified normally. | JTM-001, JTM-002, JTM-004 |
| `Nominal.boundsUpdateGating` | Applies a valid `TempBounds` update and confirms it is adopted and telemetered, applies an inverted WARN_HIGH/IDLE_HIGH configuration and confirms it is rejected (bounds unchanged, event fires), confirms repeating the same bad attempt fires again, then confirms a second valid update is adopted normally. | JTM-003 |
| `Nominal.parameterUpdatedCoverage` | Calls `parameterUpdated()` directly for `JETSON_BOUNDS` and an unrecognized ID (`boundsUpdateGating` only reaches `applyBounds()` directly, bypassing this switch). | JTM-003 |

Each test is tagged with `RecordProperty("requirement", "<REQ-IDs>")`, so
running the test binary with `--gtest_output=xml:<path>` produces a
JUnit-style XML report whose `<testcase>` elements carry that mapping as a
machine-checkable artifact.

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial implementation complete | Luca Lanzillotta, Dat Nguyen |
| 1.1.0 | Added sufficient Units tests, which paired with inspection can be validated as complete | Luca Lanzillotta |
| 1.2.0 | Added `THRESHOLDS_MISCONFIGURED` for invalid threshold ordering and regression-test coverage. | Luca Lanzillotta |
| 1.3.0 | Added telemetry readback for all six current Jetson thermal threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
| 1.4.0 | Replaced the six individual scalar threshold parameters and their six scalar readback channels with one `TempBounds` struct parameter and one `TempBounds` struct telemetry channel, so GDS shows one bounds row instead of six. A `PRM_SET` is now gated: it's only adopted if it passes `thresholdsAreOrdered()`, otherwise the component keeps its last-known-good bounds and `THRESHOLDS_MISCONFIGURED` fires (now on every rejected attempt, not just the first, since misconfiguration never takes effect). Also fixed a boundary-inclusivity bug in `determineTempState()`: a reading exactly at `WARN_HIGH` was claimed by FAULT (inclusive) before WARN (exclusive) could claim it; WARN now claims its own upper boundary, symmetric with its already-inclusive lower boundary. This was the actual root cause of the previously-flagged pre-existing test failure (round 1, zone 2/CV0, 80°C, expected WARN got FAULT) -- the failure is now fixed, not just flagged. | Luca Lanzillotta |
| 1.4.1 | `JETSON_BOUNDS` telemetry now republishes unconditionally every evaluate cycle instead of only on boot/change, so a GDS session that connects after boot still sees the current bounds on the next tick rather than missing the one-time boot publish. | Luca Lanzillotta |
| 1.5.0 | SDD accuracy audit: corrected JTM-002, which incorrectly claimed the component sends a STATE to SpacecraftStateManager -- no such port exists; it actually outputs to FPManager and DataProducer. Added JTM-003/JTM-004 to separately cover bounds gating and per-zone read-failure handling, and added `Verified By`/`Verifies` traceability between Requirements and Unit Tests. Documented the previously-undocumented `jetsonThermalReadOut` port and the `NOT_USED` state. Identified and documented (as a known limitation, not a fix -- out of scope for this audit) that the per-zone read-failure path is dead code: `m_successfulRead` is never set `false` (the line is commented out in source), so `doReadFail` can never fire. Added `parameterUpdatedCoverage` (`parameterUpdated()` was never exercised -- `boundsUpdateGating` calls `applyBounds()` directly) and tagged every test with `RecordProperty("requirement", ...)`. Replaced the unverified/inconsistent coverage claims (100%/100%/72%) with measured numbers (89.3%/91.7%/53.5%). | Luca Lanzillotta |
