# scalesSvc::JetsonPowerModeManager

Runs entirely on the Jetson (the only instance is `jetson_pwrModeManager` in
`JetsonDeployment`). It applies power-mode (`nvpmodel`) and power-state
(`shutdown -h now`) changes requested by the i.MX-side `JetsonManager`, and
reports the Jetson's current mode/state back over the hub link so
`JetsonManager` can confirm deferred commands and decide when it's safe to
cut GPIO power.

## Usage Examples
The `JetsonPowerModeManager` component is used to control the power mode of the Jetson device. It provides capabilities to set different power modes (15W, 30W, 50W, MAXN) through the `nvpmodel` commands built into the Jetson, and to gracefully shut the Jetson's own OS down (`shutdown -h now`) when the i.MX requests Jetson OFF.

## Design Summary

Two hardware-facing operations are behind injectable seams so this component
can be unit-tested without ever touching real hardware or a real shell:

- `PowerModeReader` (`int (*)()`, default `&get_nvp_mode`, which shells out to
  `nvpmodel -q`) -- reads the Jetson's current nvpmodel index, or `4` on any
  error (missing binary, unexpected output format, etc.).
- `ShellCommandRunner` (`int (*)(const char*)`, default `&std::system`) --
  runs `nvpmodel -m <mode>` and `shutdown -h now`.

`configurePowerModeReader()`/`configureShellRunner()` override these; a unit
test must always override `ShellCommandRunner` before exercising any path
that calls it, since the default genuinely shuts the machine down. Production
code never calls either setter, so the real `nvpmodel`/`shutdown` binaries are
used unless a test explicitly substitutes them.

Every handler that changes power mode compares the requested mode against
`m_powerModeReader()`'s current reading first, and is a no-op (immediate
`powerModeSend`/`OK`, no shell call) if they already match -- `nvpmodel -m`
is only ever invoked on an actual mismatch, since running it reboots the
Jetson.

`jetsonPowerStateReceive_handler`'s OFF path acknowledges OFF to the i.MX
(`jetsonPowerStateSend(OFF)` + telemetry) *before* running `shutdown -h now`,
so the i.MX can start its own delay-then-GPIO-cut sequence even if this
process is torn down mid-shutdown. If the shell command fails
(`ShellCommandRunner` returns non-zero), `JETSON_POWER_STATE_CHANGE_FAILED`
fires and ON is re-reported, since the Jetson is presumably still alive.

## Port Descriptions
| Kind | Name | Description |
|---|---|---|
| async input | `powerModeReceive` | Power mode change request from the i.MX-side `JetsonManager`, forwarded over the hub. |
| output | `powerModeSend` | Reports the Jetson's current power mode back to `JetsonManager`. |
| async input | `jetsonPowerStateReceive` | Power state (ON/OFF) request from `JetsonManager`, forwarded over the hub. |
| output | `jetsonPowerStateSend` | Reports the Jetson's current power state back to `JetsonManager`. |
| sync input | `schedIn` | Rate-group tick. Reports the boot-time power state once and the boot-time power mode once (retried every tick until the reader stops erroring). |

## Component States

There is no internal state machine; behavior is driven directly off the
real-time `nvpmodel -q` reading and two one-shot "have I reported this yet
this boot" flags (`m_modeReported`, `m_powerStateReported`). The commented-out
`## Component States` link to `scalesSvc::SpacecraftStateManager` in a
previous revision of this document described aspirational HPC power-level
naming (Minimal/Balanced/Extra/Maximum) that belongs to `PowerModeID`, not to
this component's own internal states -- `SpacecraftStateManager` itself is an
unimplemented stub (see its own SDD) and this component does not depend on it
in any way.

| Name | Description |
|---|---|
| `PowerModeID` | `MAX` (MAXN), `MIN` (15W), `BALANCED` (30W), `EXTRA` (50W) -- the four `nvpmodel` indices this component reads/sets. Not component-internal states; see `scales/Types/PowerTypes.fpp`. |

## Parameters
| Name | Description |
|---|---|
| `PWR_MODE_REQ` | Declared in `JetsonPowerModeManager.fpp` (`U8`, default 0, id 0) but never read or written anywhere in `JetsonPowerModeManager.cpp` -- dead/unused. Not a real behavior of this component today; documented here so it isn't mistaken for one. |

## Commands
| Name | Description |
|---|---|
| `SET_POWER_MODE` | Sets the Jetson power mode. No-op (`OK`, no shell call) if already in the requested mode; otherwise runs `nvpmodel -m <mode>` and responds `OK` regardless of that command's exit status (unlike the `powerModeReceive` hub path, this command handler does not check the exit code -- see Change Log). |
| `GET_POWER_MODE` | Returns the Jetson's current power mode via `powerModeSend` and `OK`, or `VALIDATION_ERROR` if the reader returns the error sentinel (`4`). |
| `SET_JETSON_POWER_STATE` | Locally requests a Jetson power-state change. ON reports ON immediately (already running, since this command is running locally on the Jetson). OFF reports OFF, runs `shutdown -h now`, and responds `OK` or `EXECUTION_ERROR` depending on the shell result -- exactly one response either way (see Change Log for a fixed double-response bug). Any other state responds `VALIDATION_ERROR`. |

## Events
| Name | Description |
|---|---|
| `POWER_MODE_REQUEST_RECEIVED` | A power-mode change request arrived via the hub port (from `JetsonManager`). |
| `POWER_MODE_CHANGED` | Declared in the `.fpp` but never logged anywhere in `JetsonPowerModeManager.cpp` -- dead, not currently reachable. Documented here rather than removed since fixing it is a product decision (what should trigger it, and when) outside the scope of this audit. |
| `POWER_MODE_CHANGE_FAILED` | The `nvpmodel -m` call (via the hub-driven `powerModeReceive` path only, not the `SET_POWER_MODE` command) returned a non-zero exit status. |
| `JETSON_POWER_STATE_REQUEST_RECEIVED` | A power-state change request arrived via the hub port. |
| `JETSON_SHUTDOWN_STARTED` | The Jetson-side OFF handler has acknowledged OFF and is about to run `shutdown -h now`. |
| `JETSON_POWER_STATE_CHANGE_FAILED` | Either the `shutdown -h now` call returned non-zero (hub-driven `jetsonPowerStateReceive` path), or an unrecognized `JetsonPowerStateID` was received on either the hub port or the `SET_JETSON_POWER_STATE` command. |

## Telemetry
| Name | Description |
|---|---|
| `CurrentPowerMode` | Current power mode of Jetson (15W, 30W, 50W, or MAXN), written whenever a mode is reported. |
| `CurrentJetsonPowerState` | Current power state of the Jetson (ON/OFF), written whenever a state is reported. |

## Unit Tests

Measured via `fprime-util check --coverage`: **84.8% line (95/112), 90.9%
function (10/11), 48.5% branch (94/194)**. The one uncovered function is the
real `get_nvp_mode()` free function itself, which is deliberately never
called by any test (every test overrides `PowerModeReader` so the real
implementation, which shells out to `nvpmodel -q`, is never exercised) --
the remaining line/branch gaps are that function's body plus the usual
ASan/UBSan instrumentation edges documented throughout this audit. Each test is tagged with
`RecordProperty("requirement", "<REQ-IDs>")`, so running the test binary with
`--gtest_output=xml:<path>` produces a JUnit-style XML report whose
`<testcase>` elements carry that mapping as a machine-checkable artifact. All
tests mock both `PowerModeReader` and `ShellCommandRunner` -- none of them
touch a real shell or query real hardware.

| Name | Description | Verifies |
|---|---|---|
| `PowerModeReceiveChangesModeWhenMismatched` | Mocked reader reports MIN, request BALANCED; confirms the shell runner is called with the matching `nvpmodel -m` argument and no immediate report is sent (the reboot confirmation happens later via `schedIn`). | JPSM-002 |
| `PowerModeReceiveReportsFailureWhenNvpmodelFails` | Same as above but the shell runner returns non-zero; confirms `POWER_MODE_CHANGE_FAILED` and that the unchanged current mode is sent back. | JPSM-002 |
| `PowerModeReceiveNoopWhenAlreadyInMode` | Reader already reports the requested mode; confirms an immediate report + telemetry and zero shell calls. | JPSM-002 |
| `JetsonPowerStateReceiveOnReportsOn` | Confirms the ON path reports ON immediately with no shell call. | JPSM-005 |
| `JetsonPowerStateReceiveOffShutsDownGracefully` | Confirms OFF is acknowledged before the shell call, `JETSON_SHUTDOWN_STARTED` fires, the shell runner is called with `shutdown -h now`, and a successful (`0`) exit produces no failure event or re-report. | JPSM-004 |
| `JetsonPowerStateReceiveOffReportsFailureWhenShutdownFails` | Same but the shell runner returns `-1`; confirms `JETSON_POWER_STATE_CHANGE_FAILED` and a follow-up ON report. | JPSM-004 |
| `SchedInReportsOnceAfterBoot` | First tick reports both power state and (mocked, non-error) power mode exactly once; a second tick repeats neither. | JPSM-001, JPSM-003 |
| `SchedInSkipsModeReportOnReaderError` | Reader returns the error sentinel (`4`) on the first tick -- confirms the mode report is withheld (state is still reported) -- then confirms it fires once the reader recovers on a later tick. | JPSM-001 |
| `SetPowerModeCmdChangesMode` | `SET_POWER_MODE` with a mismatched mode; confirms the correct `nvpmodel -m` argument and `OK`. | JPSM-002 |
| `SetPowerModeCmdNoopWhenAlreadyInMode` | `SET_POWER_MODE` already matching; confirms zero shell calls and `OK`. | JPSM-002 |
| `GetPowerModeCmdReturnsCurrentMode` | `GET_POWER_MODE` with a valid reading; confirms the report and `OK`. | JPSM-001 |
| `GetPowerModeCmdValidationErrorOnReaderFailure` | `GET_POWER_MODE` with the reader returning the error sentinel; confirms `VALIDATION_ERROR` and no report. | JPSM-001, JPSM-006 |
| `SetJetsonPowerStateCmdOnReportsOn` | `SET_JETSON_POWER_STATE(ON)`; confirms the report and `OK`. | JPSM-005 |
| `SetJetsonPowerStateCmdOffShutsDownGracefully` | `SET_JETSON_POWER_STATE(OFF)` with a successful shell result; confirms **exactly one** `OK` response -- regression test for a double-`cmdResponse_out()` bug this audit fixed (see Change Log). | JPSM-004, JPSM-006 |
| `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` | Same but the shell runner fails; confirms exactly one `EXECUTION_ERROR` response. | JPSM-004, JPSM-006 |

## Requirements
| Name | Description | Verified By |
|---|---|---|
| JPSM-001 | The component shall obtain the current power mode from the Jetson, and shall treat the reader's error sentinel as "mode unknown" rather than a real mode. | `SchedInReportsOnceAfterBoot`, `SchedInSkipsModeReportOnReaderError`, `GetPowerModeCmdReturnsCurrentMode`, `GetPowerModeCmdValidationErrorOnReaderFailure` |
| JPSM-002 | The component shall provide commands and a hub-driven path to change the Jetson's power mode, running `nvpmodel -m` only on an actual mismatch. | `PowerModeReceiveChangesModeWhenMismatched`, `PowerModeReceiveReportsFailureWhenNvpmodelFails`, `PowerModeReceiveNoopWhenAlreadyInMode`, `SetPowerModeCmdChangesMode`, `SetPowerModeCmdNoopWhenAlreadyInMode` |
| JPSM-003 | The component shall report the Jetson's power mode back to `JetsonManager` once per boot so a deferred `REQUEST_POWER_MODE` command on the i.MX side can be confirmed without a manual `GET_POWER_MODE`. | `SchedInReportsOnceAfterBoot` |
| JPSM-004 | A commanded or hub-driven Jetson OFF shall acknowledge OFF before running `shutdown -h now`, and shall report failure (and re-report ON) if the shutdown command does not succeed. | `JetsonPowerStateReceiveOffShutsDownGracefully`, `JetsonPowerStateReceiveOffReportsFailureWhenShutdownFails`, `SetJetsonPowerStateCmdOffShutsDownGracefully`, `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` |
| JPSM-005 | A commanded or hub-driven Jetson ON shall report ON immediately, since this component only runs when the Jetson is already up. | `JetsonPowerStateReceiveOnReportsOn`, `SetJetsonPowerStateCmdOnReportsOn` |
| JPSM-006 | Every command shall complete with exactly one response, and a `GET_POWER_MODE` with no valid reading available shall be rejected `VALIDATION_ERROR` rather than reporting a bogus mode. (Both handlers also have an `JETSON_POWER_STATE_CHANGE_FAILED`/`VALIDATION_ERROR` branch for a `JetsonPowerStateID` outside `{ON, OFF}`, but this cannot be safely unit-tested: constructing an out-of-range `JetsonPowerStateID` and letting anything format/log it -- which both of these paths do -- trips a hard assert in the generated enum-to-string conversion, verified by inspection only.) | `GetPowerModeCmdValidationErrorOnReaderFailure`, `SetJetsonPowerStateCmdOffShutsDownGracefully`, `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` |
| JPSM-007 | The real `nvpmodel`/`shutdown` shell invocations and the `nvpmodel -q` read shall be behind an injectable seam so the rest of this component's behavior can be unit-tested without touching real hardware or a real shell. | N/A (verified by inspection: `configurePowerModeReader`/`configureShellRunner` in `JetsonPowerModeManager.hpp`; every test in this suite overrides both before exercising any handler) |

## Change Log
| Date | Description |
|---|---|
| May 7, 2025 | Initial Draft |
| December 4, 2025 | Use nvpmodel to change modes |
| 2026-07-28 | SDD accuracy audit and first real unit test suite for this component (previously untested). Found and fixed two real bugs: (1) `get_nvp_mode()`/`std::system()` were called directly with no test seam at all -- a naive unit test would have shelled out to real `nvpmodel`/`sudo shutdown -h now` on whatever machine ran it. Added `PowerModeReader`/`ShellCommandRunner` injectable function-pointer members (`configurePowerModeReader`/`configureShellRunner`, mirroring the `configureTcpStatusPoller` pattern already used in `GdsCmdAuthMux`), defaulting to the real implementations, so unit tests never touch real hardware/shell state. (2) `SET_JETSON_POWER_STATE_cmdHandler`'s OFF branch called `cmdResponse_out()` twice for the same command -- once unconditionally right after the OFF report, then again after checking the shell result -- which would assert in the real F´ command dispatcher on hardware/GDS. Removed the first, unconditional call so exactly one response is ever sent. Documented the previously-undocumented `jetsonPowerStateReceive`/`jetsonPowerStateSend`/`schedIn` ports, `SET_JETSON_POWER_STATE` command, and `JETSON_POWER_STATE_*` events/telemetry, corrected the stale reference to an "IMX PowerManager"/`scalesSvc::PowerManager` component that does not exist (the real peer is `scalesSvc::JetsonManager`), and flagged two dead/unused declarations found during the audit (the `PWR_MODE_REQ` parameter and the `POWER_MODE_CHANGED` event) rather than guessing at removing or wiring them up. Discovered while writing this suite that a literally out-of-range `JetsonPowerStateID` cannot be safely unit-tested at all in this codebase -- the generated enum-to-string conversion (used whenever a value is formatted for an event or GDS display) hard-asserts on an invalid value, so the two "unrecognized state" defensive branches are verified by inspection only, not by test. Measured 84.8% line, 90.9% function, 48.5% branch coverage (the one uncovered function is the real `get_nvp_mode()`, deliberately never called). Added `Verified By`/`Verifies` traceability and `RecordProperty("requirement", ...)` tags. | Luca Lanzillotta |
