# Components::PerifBoardManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

The Peripheral Board of the SCALES Hardware ecosystem is power sequenced by a load switch on the Scales Leviathan board. It is triggered by a high GPIO pin state, corresponding to

```powershell
/dev/gpiochip2/ Pin: 18
```

of the imx8x.

This component must maintain a high state indefinitely on boot, and must provide a reset procedure in the case the end user chooses to do so, along with a configurable interval parameter.

## Requirements

| **Name** | **Description** | **Validation** | **Verified By** |
| --- | --- | --- | --- |
| PBM-001 | The component must maintain a high GPIO state unless a reset condition is passed | Unit Test | `Nominal.testPerifBoardManager` |
| PBM-002 | The reset state must return to an always on state. | Unit Test | `Nominal.testPerifBoardManager` |
| PBM-003 | The interval time between the reset loop must be configurable | Unit Test | `Nominal.testPerifBoardManager`, `Nominal.configurableOffInterval` |
| PBM-004 | The GpioDriver call must correspond to /dev/gpiochip2 pin 18 | By inspection | N/A (topology wiring, not unit-testable) |
| PBM-005 | An `emergencyPowerOff` signal from FPManager must immediately force the GPIO low and latch it there -- once tripped, the board never returns high, even across further run cycles or a subsequent `powerOn(ON)` command | Unit Test | `Nominal.emergencyShutdownLatch` |

## Design

### Diagrams

### Typical Usage

Upon execution of the ImxDeployment on the imx8x, the GPIO state is always held high. If the user chooses to reset the Peripheral Board for whatever reason, they can excecute an OFF command, which prints an event notifying, updates the telemetry, then turns the gpio off for a set interval of time, and then turns it back on, 

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Run handler for m_powerMode logic. Maintains switch case conditionals for CMD input |
| sync input port | emergencyPowerOff | scalesSvc.EmergencyPowerOff | Latched emergency power-off signal from FPManager. Immediately forces the GPIO low; the latch is never cleared, so the board stays off for the rest of the session. |
| output port | gpioSet | Drv.GpioWrite | Output port of GpioWrite type to signal the GpioDriver to write a value to the gpio |

## Component States

| **Name** | **Description** |
| --- | --- |
| m_powerMode = Fw::On::ON | When m_powerMode is set to ON by the onOff:CmdHandler, a switch case within the run handler goes to an ON case, and sets the GPIO high, updates telemetry, and repeats |
| m_powerMode = Fw::On::OFF | When m_powerMode is set to OFF by the onOff:CmdHandler, a switch case within the run handler goes to an OFF case, holding the gpio low until the time between the start of the off state and the current time exceeds param OfftimeSec.
Once it does, m_powerMode is set to ON and the cycle continues |
| m_emergencyShutdown = true | Set once by `emergencyPowerOff_handler()` and never cleared. `run_handler()` checks this flag before the `m_powerMode` switch and, if set, unconditionally forces the GPIO low and returns -- `m_powerMode` keeps updating underneath (e.g. a `powerOn(ON)` command still logs `gpioOn` and responds OK), but it no longer has any effect on the physical GPIO for the rest of the session. |

## Parameters

| **Name** | **Description** |
| --- | --- |
| param offTimeSec | parameter to hold the value of how long in seconds the OFF state must be held for before turning back ON |

## Commands

| **Name** | **Description** |
| --- | --- |
| async command powerOn | the powerOn command takes in an argument type of Fw.On called highLow. The state of highLow is set with the command and sets the power state of the PerifBoard |

## Events

| **Name** | **Description** |
| --- | --- |
| event gpioOn | takes in an argument ($state = Fw.On)
when the gpioOn is set to On by the CmdHandler, it will output a high severity on event.
when the gpioOn is set to Off by the CmdHandler, it will output a high severity off event. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| telemetry gpioState | Outputs the current state of the gpio |

## Unit Tests

Measured via `fprime-util check --coverage`: **100% line (44/44), 100%
function (5/5), 56.1% branch (32/57)**. The uncovered branches are almost
entirely ASan/UBSan instrumentation edges around construction (a pattern
also seen in WatchdogManager/McpManager/ImxThermalManager), not
application-logic gaps.

| **Name** | **Description** | **Verifies** |
| --- | --- | --- |
| `Nominal.testPerifBoardManager` | Confirms the default-ON boot state holds GPIO high across repeated cycles; sends an OFF command and confirms the `gpioOn` event, GPIO LOW, and telemetry; holds LOW through the configured `offTimeSec` window and confirms it returns to HIGH exactly once the window elapses; confirms an ON command re-fires the `gpioOn` event and command response. | PBM-001, PBM-002, PBM-003 |
| `Nominal.emergencyShutdownLatch` | Drives the board to its default ON state, then asserts `emergencyPowerOff` and confirms the GPIO is immediately forced LOW. Confirms the latch survives a further run cycle and a `powerOn(ON)` command -- the command still logs its event and responds OK, but the GPIO never leaves LOW. | PBM-005 |
| `Nominal.configurableOffInterval` | Configures `offTimeSec` to a non-default value (5s) via the mock parameter store and `loadParameters()`, then confirms the OFF state is held through 4s (short of the interval) and only returns ON once 5s have actually elapsed -- proving the interval is a live parameter, not a hardcoded constant. | PBM-003 |

Each test is tagged with `RecordProperty("requirement", "<REQ-IDs>")`, so
running the test binary with `--gtest_output=xml:<path>` produces a
JUnit-style XML report whose `<testcase>` elements carry that mapping as a
machine-checkable artifact.

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| Initial implementation | Basic capabilities and unit test | Luca Lanzilotta |
| Improved initial implementation. | Unit tests complete |  |
| 1.1.0 | SDD accuracy audit: fixed the pre-existing `Nominal.testPerifBoardManager` bug where a stray `invoke_to_run()` before the final `powerOn(ON)` command desynced the dispatch queue, silently leaving the command handler undispatched and its assertions checking stale (empty) history. Fixed a leaked active-component queue in the tester destructor (missing `component.deinit()`), matching the same pre-existing bug found and fixed in WatchdogManager. Documented the previously-undocumented `emergencyPowerOff` port and `m_emergencyShutdown` latch state, added requirement PBM-005 for it, and added `Nominal.emergencyShutdownLatch` to verify the latch actually holds across run cycles and commands. Added `Nominal.configurableOffInterval` to verify PBM-003 with a non-default `offTimeSec`, since the existing test only ever exercised the 2s default. Added `Verified By`/`Verifies` traceability between Requirements and Unit Tests, replaced the unverified 100% coverage claims with measured numbers (100% line, 100% function, 56.1% branch), and tagged every test with `RecordProperty("requirement", ...)`. | Luca Lanzillotta |