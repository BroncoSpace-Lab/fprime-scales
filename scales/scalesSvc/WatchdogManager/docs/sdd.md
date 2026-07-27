# scalesSvc::WatchdogManager

Functional Description: F Prime Component Watchdog Manager to “pet” F Prime components and check for response.

## Usage Example

Pets the watchdog on the SCALES EPS system every 1 second. Event is emitted on first cycle, and then telemetry is regularly updated every second with the status of the PET.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| WTD-001 | The WatchdogManager shall ping its subsystems Watchdog circuit on the SCALES EPS | Unit Test / Inspection |
| WTD-002 | The WatchdogManager will emit a single event when it first starts that is is ON, then it will pet the watchdog every second. This will be shown in telemetry | Unit Test / Inspection |

## Design

### Diagrams

### Typical Usage

WatchdogManager runs on boot of each system, emits a single ON event on start and then emits telemetry of PETTING and NOT_PETTING every 1 second, making sure to hold the GPIO on for 1 second on each cycle

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input | run | Svc.Sched | run handler for interval pinging |

## Parameters

| **Name** | **Description** |
| --- | --- |
| watchDogPetInterval | U32 Parameter that can be set to change the PetIntervalTime |

## Events

| **Name** | **Description** |
| --- | --- |
| WatchdogState | Used to emit the first and only ON when the component starts running. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| WatchdogPet | emits the WatchdogPet status, either PETTING or NOT_PETTING |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| WDM-001 | Check that on first cycle WD is first off and everything inside is set to start the Watchdog on next cycle | 100% | 100% |
| WDM-002 | Check that on the second cycle we trigger the WD ON | 100% | 100% |
| WDM-003 | Check that on the third cycle we emulate the time elapsed and we stop petting momentarilly | 100% | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| V1.0.0 | Watchdog Manager with Unit Tests complete | Luca Lanzillotta |

---