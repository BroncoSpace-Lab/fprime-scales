# Components::JetsonThermalManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

Used for acquiring temperature information about the 9 thermal zones on the Jetson Orin AGX. Additionally, this component implements a state machine to determine the device thermal health which outputs the state to the SSM

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| JTM-001 | Component must read all 9 thermal zones of the Jetson Orin AGX and output it as telemetry | Unit Test / Inspection |
| JTM-002 | Component must evaluate device state based on thermal readings and send a STATE to the SpacecraftStateManager | Unit Test / Inspection |

## Design

### Diagrams

!image.png

### Typical Usage

Component will output telemetry to GDS, and will determine device state based on thermal levels.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Port invoked by the rate group to read the nine Jetson thermal zones and output their readings as telemetry. |
| output port | jetsonThermalReadingOut | ThermalReadingPort | Sends each complete Jetson thermal reading to FPManager. |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | IDLE state, between IDLE LOW and IDLE HIGH |
| WARN | WARN state, between WARN LOW/IDLE LOW and IDLE HIGH/WARN HIGH |
| FAULT | FAULT state, between FAULT LOW/WARN LOW and WARN HIGH/FAULT HIGH |

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

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| JTM-01 | Verify that data is being requested, that data has been captured. | ASSERT_TLM size returns as correct | 100% |
| JTM-02 | Verify that Telemetry is being updated on every cycle | ASSERT_TLM returns as correct | 100% |
| JTM-03 | Verify that if a temperature parameter is overrun, a state machine state is invoked and a desired flag is outputted. | ASSERT_TLM returns with a state outputtted | 72% |
| JTM-04 | `boundsUpdateGating`: applies a valid `TempBounds` update and confirms it is adopted and telemetered, applies an inverted WARN_HIGH/IDLE_HIGH configuration and confirms it is rejected (bounds unchanged, event fires), confirms repeating the same bad attempt fires again, then confirms a second valid update is adopted normally. | Active bounds, telemetry, and `THRESHOLDS_MISCONFIGURED` event count match expectations at each step | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial implementation complete | Luca Lanzillotta, Dat Nguyen |
| 1.1.0 | Added sufficient Units tests, which paired with inspection can be validated as complete | Luca Lanzillotta |
| 1.2.0 | Added `THRESHOLDS_MISCONFIGURED` for invalid threshold ordering and regression-test coverage. | Luca Lanzillotta |
| 1.3.0 | Added telemetry readback for all six current Jetson thermal threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
| 1.4.0 | Replaced the six individual scalar threshold parameters and their six scalar readback channels with one `TempBounds` struct parameter and one `TempBounds` struct telemetry channel, so GDS shows one bounds row instead of six. A `PRM_SET` is now gated: it's only adopted if it passes `thresholdsAreOrdered()`, otherwise the component keeps its last-known-good bounds and `THRESHOLDS_MISCONFIGURED` fires (now on every rejected attempt, not just the first, since misconfiguration never takes effect). Also fixed a boundary-inclusivity bug in `determineTempState()`: a reading exactly at `WARN_HIGH` was claimed by FAULT (inclusive) before WARN (exclusive) could claim it; WARN now claims its own upper boundary, symmetric with its already-inclusive lower boundary. This was the actual root cause of the previously-flagged pre-existing test failure (round 1, zone 2/CV0, 80°C, expected WARN got FAULT) -- the failure is now fixed, not just flagged. | Luca Lanzillotta |
| 1.4.1 | `JETSON_BOUNDS` telemetry now republishes unconditionally every evaluate cycle instead of only on boot/change, so a GDS session that connects after boot still sees the current bounds on the next tick rather than missing the one-time boot publish. | Luca Lanzillotta |
