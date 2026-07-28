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

| **Name** | **Description** |
| --- | --- |
| JETSON_IDLE_LOW | IDLE low temperature threshold (°C) |
| JETSON_IDLE_HIGH | IDLE high temperature threshold (°C) |
| JETSON_WARN_LOW | WARNING low temperature threshold (°C) |
| JETSON_WARN_HIGH | WARNING high temperature threshold (°C) |
| JETSON_FAULT_LOW | FAULT low temperature threshold (°C) |
| JETSON_FAULT_HIGH | FAULT high temperature threshold (°C) |

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
| JETSON_IDLE_LOW / JETSON_IDLE_HIGH / JETSON_WARN_LOW / JETSON_WARN_HIGH / JETSON_FAULT_LOW / JETSON_FAULT_HIGH | Current Jetson thermal threshold values, published for GDS readback. |

## Events

| **Name** | **Description** |
| --- | --- |
| THRESHOLDS_MISCONFIGURED | Warning emitted once when the six Jetson thresholds stop being in ascending order (`FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH`); re-emits only after the ordering is corrected and broken again. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| JTM-01 | Verify that data is being requested, that data has been captured. | ASSERT_TLM size returns as correct | 100% |
| JTM-02 | Verify that Telemetry is being updated on every cycle | ASSERT_TLM returns as correct | 100% |
| JTM-03 | Verify that if a temperature parameter is overrun, a state machine state is invoked and a desired flag is outputted. | ASSERT_TLM returns with a state outputtted | 72% |
| JTM-04 | `thresholdsMisconfiguredEmitsOnceOnTransition`: checks that an invalid threshold ordering emits once, remains latched while invalid, and emits again after being fixed and broken again. | `THRESHOLDS_MISCONFIGURED` event count matches expectations | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial implementation complete | Luca Lanzillotta, Dat Nguyen |
| 1.1.0 | Added sufficient Units tests, which paired with inspection can be validated as complete | Luca Lanzillotta |
| 1.2.0 | Added `THRESHOLDS_MISCONFIGURED` for invalid threshold ordering and regression-test coverage. | Luca Lanzillotta |
| 1.3.0 | Added telemetry readback for all six current Jetson thermal threshold parameters, published at initialization and after parameter updates. | Luca Lanzillotta |
