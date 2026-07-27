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

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| PBM-001 | The component must maintain a high GPIO state unless a reset condition is passed | Unit Test |
| PBM-002 | The reset state must return to an always on state. | Unit Test |
| PBM-003 | The interval time between the reset loop must be configurable | Unit Test |
| PBM-004 | The GpioDriver call must correspond to /dev/gpiochip2 pin 18 | By inspection |

## Design

### Diagrams

### Typical Usage

Upon execution of the ImxDeployment on the imx8x, the GPIO state is always held high. If the user chooses to reset the Peripheral Board for whatever reason, they can excecute an OFF command, which prints an event notifying, updates the telemetry, then turns the gpio off for a set interval of time, and then turns it back on, 

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| async input port | run | Svc.Sched | Run handler for m_powerMode logic. Maintains switch case conditionals for CMD input |
| output port | gpioSet | Drv.GpioWrite | Output port of GpioWrite type to signal the GpioDriver to write a value to the gpio |

## Component States

| **Name** | **Description** |
| --- | --- |
| m_powerMode = Fw::On::ON | When m_powerMode is set to ON by the onOff:CmdHandler, a switch case within the run handler goes to an ON case, and sets the GPIO high, updates telemetry, and repeats |
| m_powerMode = Fw::On::OFF | When m_powerMode is set to OFF by the onOff:CmdHandler, a switch case within the run handler goes to an OFF case, holding the gpio low until the time between the start of the off state and the current time exceeds param OfftimeSec.
Once it does, m_powerMode is set to ON and the cycle continues |

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

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| UTPBM-001 | Verify that on a single, first cycle, gpioSet is invoked and the telemetry of the gpioState is updated with a parameter | Test Passed | 100% |
| UTPBM-002 | Verify that on an OFF command, the GPIO is set to LOW, an event is emitted, and telemetry is updated. Then after time > 2 seconds, the GPIO goes back to an ON state | Test Passed | 100% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| Initial implementation | Basic capabilities and unit test | Luca Lanzilotta |
| Improved initial implementation. | Unit tests complete |  |