# Components::InaManager

Functional Description: Manager for INA260 current, voltage, and power sensor.

## Usage Examples

InaManager will forward Voltage, Current, and Power from each SCALES subsystem to the Spacecraft State Manager for state decision making.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| INAM-001 | The InaManager will send telemetry data of Voltage, Current, and Power to the GDS with proper formatting | Unit Tests/Inspection |
| INA-002 | The InaManager will forward the telemetry to the SSM for fault protection | Unit Tests/Inspection |

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

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| INAUT-001 | Verify that on a cycle the device generates telemetry and that that telemetry falls within a preset range | Passed | 100% |
| INAUT-001 | Verify that on multiple cycles telemetry updates | Passed  | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| v1.0.0 | Initial implementation + Unit Tests to verify the code works. Also works within GDS | Dat Nguyen, Luca Lanzillotta |