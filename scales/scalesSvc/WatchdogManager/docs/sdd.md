# scalesSvc::WatchdogManager

Functional Description: F Prime Component Watchdog Manager to “pet” F Prime components and check for response.

## Usage Example

Pets the watchdog on the SCALES EPS system every 1 second. Event is emitted on first cycle, and then telemetry is regularly updated every second with the status of the PET.

## Requirements

| **Name** | **Description** | **Validation** | **Verified By** |
| --- | --- | --- | --- |
| WTD-001 | The WatchdogManager shall ping its subsystems Watchdog circuit on the SCALES EPS | Unit Test / Inspection | `Nominal.WatchdogTester` |
| WTD-002 | The WatchdogManager will emit a single event when it first starts that is is ON, then it will pet the watchdog every second. This will be shown in telemetry | Unit Test / Inspection | `Nominal.WatchdogTester` |

## Design

### Diagrams

### Typical Usage

WatchdogManager runs on boot of each system, emits a single ON event on start and then emits telemetry of PETTING and NOT_PETTING every 1 second, making sure to hold the GPIO on for 1 second on each cycle

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input | run | Svc.Sched | run handler for interval pinging |
| output | gpioWatchDog | Drv.GpioWrite | Drives the watchdog GPIO line HIGH while petting, LOW otherwise |

## Parameters

None. The pet interval (`m_intervalSec`) is a compile-time constant fixed at 1 second
(`WatchdogManager.hpp`); it is not exposed as a runtime parameter.

## Events

| **Name** | **Description** |
| --- | --- |
| WatchdogState | Used to emit the first and only ON when the component starts running. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| WatchdogPet | emits the WatchdogPet status, either PETTING or NOT_PETTING |

## Unit Tests

There is one gtest case, `Nominal.WatchdogTester`, which drives the component through
three sequential `run` cycles in a single test body:

| **Name** | **Description** | **Verifies** |
| --- | --- | --- |
| `Nominal.WatchdogTester` | Cycle 1: first-ever tick while `DISABLED` -- confirms `NOT_PETTING`/GPIO LOW telemetry, the single `WatchdogState(ON)` event, and the transition to `ENABLED`. Cycle 2: two sub-ticks -- one with no time elapsed (confirms the pet interval is respected, no state change) and one 2s later (confirms the not-yet-petting -> `PETTING`/GPIO HIGH transition). Cycle 3: 1s later, confirms the `PETTING` -> `NOT_PETTING`/GPIO LOW transition. | WTD-001, WTD-002 |

Measured via `fprime-util check --coverage`: **100% line, 100% function, 56.8% branch**
(21/37). The two states of `Fw::Enabled` (`DISABLED`, `ENABLED`) and both outcomes of
`if (this->m_isPetting)` are all exercised by the test above; the remaining unreached
branches are compiler/sanitizer-instrumented edges in the constructor and base-class
calls (ASan/UBSan build), not reachable application logic, so 100% branch coverage is
not attainable here through the component's public API.

Each test case is tagged with `RecordProperty("requirement", "<REQ-IDs>")`, so running
the test binary with `--gtest_output=xml:<path>` produces a JUnit-style XML report
whose `<testcase>` elements carry a `<properties><property name="requirement" .../>`
entry -- a machine-checkable artifact tying each test to the requirement(s) it
verifies, per the `Requirements` table's `Verified By` column above.

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| V1.0.0 | Watchdog Manager with Unit Tests complete | Luca Lanzillotta |
| V1.1.0 | SDD accuracy audit: removed a documented `watchDogPetInterval` parameter that does not exist in code (the interval is a fixed 1s constant), added the missing `gpioWatchDog` port to Port Descriptions, corrected the Unit Tests section to reflect the actual single `Nominal.WatchdogTester` gtest case (previously listed as three separate tests), replaced unverified 100%-coverage claims with measured `fprime-util check --coverage` numbers, added `Verified By` traceability from each requirement to the test that verifies it, and tagged the test with `RecordProperty("requirement", ...)` so a `--gtest_output=xml` run produces a machine-checkable requirements artifact. Also fixed a pre-existing compile break (`WatchdogManagerTester` accessed private members without a `friend` declaration) and a memory leak (missing `component.deinit()` in the tester's destructor). | Luca Lanzillotta |

---