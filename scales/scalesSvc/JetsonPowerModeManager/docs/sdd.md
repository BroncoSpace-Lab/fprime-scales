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

A genuinely successful `shutdown -h now` does *not* always come back as a
clean `0` exit, though: the real shutdown it triggers tears down this
process's own systemd service (`jetson-deployment.service`) as part of the
same sequence, which can send `SIGTERM` to the `sudo`/`shutdown` child before
`std::system()`'s `wait()` observes a clean exit. A raw wait-status of just
`SIGTERM` (`WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM`, i.e. the low
7 bits equal 15 with no core-dump bit) immediately after issuing that exact
command is the textbook signature of "my own service got torn down as
collateral of the shutdown I just triggered," not a real failure --
`classifySelfDisruptingCommandStatus()` (file-local in
`JetsonPowerModeManager.cpp`) treats it as `LikelySuccessKilledBySigterm` and
neither handler reports a failure or re-reports `ON` for it. This matters
beyond the local event log: an unwarranted `ON` correction here gets
forwarded by `JetsonManager` to `FPManager`, which can flip `FPManager`'s
`m_jetsonPowerState` back to `ON` while it's mid-wait in `disablingHpc` for a
confirmed `OFF` -- see JPSM-008 and the `JetsonManager`/`FPManager` SDDs for
the downstream half of this. Any other non-zero outcome (real exit failure,
killed by a different signal, `std::system()` failing to spawn a shell at
all) is still reported as a genuine failure exactly as before.

`nvpmodel -m <mode>` reboots the Jetson to apply the new mode, tearing down
this same process's own systemd service exactly the same way `shutdown -h
now` does -- so `powerModeReceive_handler` (and, as of JPSM-012,
`SET_POWER_MODE_cmdHandler`) now reuse the *same*
`classifySelfDisruptingCommandStatus()` classifier for the `nvpmodel` exit
check that was previously bare `ret != 0` with no raw-status decoding at
all: a genuine hardware success killed by `SIGTERM` collateral was
previously misreported as `POWER_MODE_CHANGE_FAILED` even though the mode
change and reboot both genuinely succeeded (JPSM-009). A new event,
`JETSON_POWER_MODE_REBOOT_STARTED`, is logged immediately before the
`nvpmodel` shell call -- mirroring exactly where `JETSON_SHUTDOWN_STARTED`
sits relative to its own shell call -- so GDS sees it even if this process
is torn down moments later by the reboot (JPSM-010).

A new `m_rebootPending` guard (checked at the top of, and set immediately
before the `nvpmodel` call in, both `powerModeReceive_handler` and
`SET_POWER_MODE_cmdHandler`) defends against a second overlapping
mode-change request arriving in the narrow window between invoking
`nvpmodel` and the reboot actually severing the hub link/killing this
process -- logging `POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING` (hub path,
which has no command response to answer with) or responding `BUSY` (local
`SET_POWER_MODE` path) instead of double-invoking `nvpmodel` (JPSM-011).
This is defense-in-depth: the i.MX-side `JetsonManager` is expected to
prevent this in normal operation via its own `m_hasPendingCmd` guard and
hub-link-trust gate (see JM-011/JM-012 in `JetsonManager`'s SDD). The guard
is deliberately left *set* on success/likely-success -- this process
instance is expected to be replaced by a fresh one (constructed with the
guard defaulting `false` again) once the reboot completes -- but is
explicitly *cleared* on a **genuine** `nvpmodel` failure, where no reboot is
coming and this same process instance keeps running; leaving it latched in
that case would permanently block all future mode changes on this
component.

A `SET_POWER_MODE` command sent directly to this component (bypassing
`JetsonManager`/`FPManager` entirely, via the generic remote-command routing
path on the i.MX side) can trigger the exact same `nvpmodel` reboot as a
hub-driven `powerModeReceive` -- but `JetsonManager` had no way to learn about
it, since only `powerModeReceive_handler`'s caller (`JetsonManager` itself)
already knows a reboot is coming. `SET_POWER_MODE_cmdHandler`'s mismatch
branch now unconditionally calls a new `localModeChangeStarted` output (right
after logging `JETSON_POWER_MODE_REBOOT_STARTED`, before running `nvpmodel`)
so `JetsonManager` can arm the same hub-link-distrust guard it already arms
for a hub-driven mode change. `powerModeReceive_handler` deliberately does
*not* call this -- `JetsonManager`'s own `REQUEST_POWER_MODE_cmdHandler`
already self-arms its guard before `reqPwrMode_out()` is even sent, so the
Jetson-side handler for that path never runs before the i.MX side is already
protected. See JPSM-013.

## Port Descriptions
| Kind | Name | Description |
|---|---|---|
| async input | `powerModeReceive` | Power mode change request from the i.MX-side `JetsonManager`, forwarded over the hub. |
| output | `powerModeSend` | Reports the Jetson's current power mode back to `JetsonManager`. |
| async input | `jetsonPowerStateReceive` | Power state (ON/OFF) request from `JetsonManager`, forwarded over the hub. |
| output | `jetsonPowerStateSend` | Reports the Jetson's current power state back to `JetsonManager`. |
| output | `localModeChangeStarted` | Notifies `JetsonManager`, over the hub, that a LOCAL `SET_POWER_MODE` command (not `powerModeReceive`) is about to reboot the Jetson -- lets `JetsonManager` arm its hub-link-distrust guard for this path too, since it would otherwise have no visibility into it at all (JPSM-013). Reuses the `PowerModeReceive` port type rather than introducing a new one. |
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
| `SET_POWER_MODE` | Sets the Jetson power mode. Rejected `BUSY` if a previous mode-change reboot is already pending (`m_rebootPending`, JPSM-011). No-op (`OK`, no shell call) if already in the requested mode; otherwise logs `JETSON_POWER_MODE_REBOOT_STARTED`, notifies `JetsonManager` via `localModeChangeStarted` (JPSM-013), runs `nvpmodel -m <mode>`, and responds `OK` or `EXECUTION_ERROR` depending on the classified shell result (JPSM-012 -- previously responded `OK` unconditionally regardless of exit status, see Change Log). |
| `GET_POWER_MODE` | Returns the Jetson's current power mode via `powerModeSend` and `OK`, or `VALIDATION_ERROR` if the reader returns the error sentinel (`4`). |
| `SET_JETSON_POWER_STATE` | Locally requests a Jetson power-state change. ON reports ON immediately (already running, since this command is running locally on the Jetson). OFF reports OFF, runs `shutdown -h now`, and responds `OK` or `EXECUTION_ERROR` depending on the classified shell result (a `SIGTERM`-killed status counts as `OK`, see JPSM-008) -- exactly one response either way (see Change Log for a fixed double-response bug). Any other state responds `VALIDATION_ERROR`. |

## Events
| Name | Description |
|---|---|
| `POWER_MODE_REQUEST_RECEIVED` | A power-mode change request arrived via the hub port (from `JetsonManager`). |
| `POWER_MODE_CHANGED` | Declared in the `.fpp` but never logged anywhere in `JetsonPowerModeManager.cpp` -- dead, not currently reachable. Documented here rather than removed since fixing it is a product decision (what should trigger it, and when) outside the scope of this audit. |
| `POWER_MODE_CHANGE_FAILED` | The `nvpmodel -m` call returned a genuine failure (via either the hub-driven `powerModeReceive` path or the `SET_POWER_MODE` command, as of JPSM-012 -- a `SIGTERM`-killed status is classified as likely success, not failure, and does not fire this event, JPSM-009). |
| `JETSON_POWER_STATE_REQUEST_RECEIVED` | A power-state change request arrived via the hub port. |
| `JETSON_SHUTDOWN_STARTED` | The Jetson-side OFF handler has acknowledged OFF and is about to run `shutdown -h now`. |
| `JETSON_POWER_STATE_CHANGE_FAILED` | Either the `shutdown -h now` call returned a genuine failure (hub-driven `jetsonPowerStateReceive` path) -- a `SIGTERM`-killed status is classified as likely success, not a failure, and does not fire this event (JPSM-008) -- or an unrecognized `JetsonPowerStateID` was received on either the hub port or the `SET_JETSON_POWER_STATE` command. |
| `JETSON_POWER_MODE_REBOOT_STARTED` | Logged immediately before the `nvpmodel -m` shell call whenever a mode change is needed (either entry point), so GDS sees it even if this process is torn down moments later by the reboot (JPSM-010). |
| `POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING` | A mode-change request (hub-driven or local) arrived while a previous mode-change-triggered reboot was already pending on this process instance (`m_rebootPending`); dropped rather than double-invoking `nvpmodel` (JPSM-011). |

## Telemetry
| Name | Description |
|---|---|
| `CurrentPowerMode` | Current power mode of Jetson (15W, 30W, 50W, or MAXN), written whenever a mode is reported. |
| `CurrentJetsonPowerState` | Current power state of the Jetson (ON/OFF), written whenever a state is reported. |

## Unit Tests

Measured via `fprime-util check --coverage`: **87.6% line (127/145), 91.7%
function (11/12), 52.8% branch (130/246)**. The one uncovered function is the
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
| `PowerModeReceiveChangesModeWhenMismatched` | Mocked reader reports MIN, request BALANCED; confirms `JETSON_POWER_MODE_REBOOT_STARTED` fires, the shell runner is called with the matching `nvpmodel -m` argument, and no immediate report is sent (the reboot confirmation happens later via `schedIn`). | JPSM-002, JPSM-010 |
| `PowerModeReceiveReportsFailureWhenNvpmodelFails` | Same as above but the shell runner returns non-zero; confirms `POWER_MODE_CHANGE_FAILED` and that the unchanged current mode is sent back. | JPSM-002 |
| `PowerModeReceiveReportsFailureOnPackedNonzeroExit` | Parity with the shutdown path's equivalent test: a realistic packed wait-status (`1 << 8`) is classified as a genuine failure, same as a plain nonzero exit. | JPSM-002, JPSM-009 |
| `PowerModeReceiveTreatsSigtermAsLikelySuccess` | Shell runner returns a raw `SIGTERM`-killed status; confirms no `POWER_MODE_CHANGE_FAILED` and no stale-mode correction report. | JPSM-009 |
| `PowerModeReceiveNoopWhenAlreadyInMode` | Reader already reports the requested mode; confirms an immediate report + telemetry, zero shell calls, and no reboot-started event. | JPSM-002 |
| `PowerModeReceiveIgnoredWhileRebootPending` | A second `powerModeReceive` request arrives while the first's reboot is still pending; confirms `nvpmodel` is not invoked a second time and `POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING` fires. | JPSM-011 |
| `PowerModeReceiveClearsRebootPendingOnGenuineFailure` | A genuine `nvpmodel` failure clears `m_rebootPending`; confirms a subsequent mismatched-mode request is not blocked. | JPSM-011 |
| `JetsonPowerStateReceiveOnReportsOn` | Confirms the ON path reports ON immediately with no shell call. | JPSM-005 |
| `JetsonPowerStateReceiveOffShutsDownGracefully` | Confirms OFF is acknowledged before the shell call, `JETSON_SHUTDOWN_STARTED` fires, the shell runner is called with `shutdown -h now`, and a successful (`0`) exit produces no failure event or re-report. | JPSM-004 |
| `JetsonPowerStateReceiveOffReportsFailureWhenShutdownFails` | Same but the shell runner returns `-1`; confirms `JETSON_POWER_STATE_CHANGE_FAILED` and a follow-up ON report. | JPSM-004 |
| `JetsonPowerStateReceiveOffTreatsSigtermAsSuccess` | Shell runner returns a raw `SIGTERM`-killed status; confirms no `JETSON_POWER_STATE_CHANGE_FAILED` and no follow-up ON report -- only the original OFF acknowledgment. | JPSM-008 |
| `SchedInReportsOnceAfterBoot` | First tick reports both power state and (mocked, non-error) power mode exactly once; a second tick repeats neither. | JPSM-001, JPSM-003 |
| `SchedInSkipsModeReportOnReaderError` | Reader returns the error sentinel (`4`) on the first tick -- confirms the mode report is withheld (state is still reported) -- then confirms it fires once the reader recovers on a later tick. | JPSM-001 |
| `SetPowerModeCmdChangesMode` | `SET_POWER_MODE` with a mismatched mode; confirms the correct `nvpmodel -m` argument and `OK`. | JPSM-002 |
| `SetPowerModeCmdNoopWhenAlreadyInMode` | `SET_POWER_MODE` already matching; confirms zero shell calls and `OK`. | JPSM-002 |
| `SetPowerModeCmdReportsExecutionErrorOnNvpmodelFailure` | `SET_POWER_MODE` with a genuine `nvpmodel` failure; confirms `EXECUTION_ERROR` (previously always `OK` regardless of outcome). | JPSM-012 |
| `SetPowerModeCmdTreatsSigtermAsSuccess` | `SET_POWER_MODE` with a `SIGTERM`-killed status; confirms `OK`. | JPSM-009, JPSM-012 |
| `SetPowerModeCmdIgnoredWhileRebootPending` | A second `SET_POWER_MODE` while the first's reboot is pending; confirms `BUSY` and no second `nvpmodel` invocation. | JPSM-011 |
| `GetPowerModeCmdReturnsCurrentMode` | `GET_POWER_MODE` with a valid reading; confirms the report and `OK`. | JPSM-001 |
| `GetPowerModeCmdValidationErrorOnReaderFailure` | `GET_POWER_MODE` with the reader returning the error sentinel; confirms `VALIDATION_ERROR` and no report. | JPSM-001, JPSM-006 |
| `SetJetsonPowerStateCmdOnReportsOn` | `SET_JETSON_POWER_STATE(ON)`; confirms the report and `OK`. | JPSM-005 |
| `SetJetsonPowerStateCmdOffShutsDownGracefully` | `SET_JETSON_POWER_STATE(OFF)` with a successful shell result; confirms **exactly one** `OK` response -- regression test for a double-`cmdResponse_out()` bug this audit fixed (see Change Log). | JPSM-004, JPSM-006 |
| `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` | Same but the shell runner fails; confirms exactly one `EXECUTION_ERROR` response. | JPSM-004, JPSM-006 |
| `SetJetsonPowerStateCmdOffTreatsSigtermAsSuccess` | Shell runner returns a raw `SIGTERM`-killed status; confirms `OK`, not `EXECUTION_ERROR`. | JPSM-008 |
| `SetPowerModeCmdNotifiesLocalModeChangeStarted` | `SET_POWER_MODE` with a mismatched mode; confirms `localModeChangeStarted` fires with the requested mode. | JPSM-013 |
| `SetPowerModeCmdSkipsLocalModeChangeStartedWhenAlreadyInMode` | `SET_POWER_MODE` already matching; confirms `localModeChangeStarted` does not fire (no reboot is happening). | JPSM-013 |
| `PowerModeReceiveDoesNotNotifyLocalModeChangeStarted` | Hub-driven `powerModeReceive` with a mismatched mode; confirms `localModeChangeStarted` does not fire (JetsonManager already self-arms its own guard for this path). | JPSM-013 |

## Requirements
| Name | Description | Verified By |
|---|---|---|
| JPSM-001 | The component shall obtain the current power mode from the Jetson, and shall treat the reader's error sentinel as "mode unknown" rather than a real mode. | `SchedInReportsOnceAfterBoot`, `SchedInSkipsModeReportOnReaderError`, `GetPowerModeCmdReturnsCurrentMode`, `GetPowerModeCmdValidationErrorOnReaderFailure` |
| JPSM-002 | The component shall provide commands and a hub-driven path to change the Jetson's power mode, running `nvpmodel -m` only on an actual mismatch. | `PowerModeReceiveChangesModeWhenMismatched`, `PowerModeReceiveReportsFailureWhenNvpmodelFails`, `PowerModeReceiveNoopWhenAlreadyInMode`, `SetPowerModeCmdChangesMode`, `SetPowerModeCmdNoopWhenAlreadyInMode` |
| JPSM-003 | The component shall report the Jetson's power mode back to `JetsonManager` once per boot so a deferred `REQUEST_POWER_MODE` command on the i.MX side can be confirmed without a manual `GET_POWER_MODE`. | `SchedInReportsOnceAfterBoot` |
| JPSM-004 | A commanded or hub-driven Jetson OFF shall acknowledge OFF before running `shutdown -h now`, and shall report failure (and re-report ON) if the shutdown command does not genuinely succeed -- see JPSM-008 for what counts as genuine. | `JetsonPowerStateReceiveOffShutsDownGracefully`, `JetsonPowerStateReceiveOffReportsFailureWhenShutdownFails`, `SetJetsonPowerStateCmdOffShutsDownGracefully`, `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` |
| JPSM-005 | A commanded or hub-driven Jetson ON shall report ON immediately, since this component only runs when the Jetson is already up. | `JetsonPowerStateReceiveOnReportsOn`, `SetJetsonPowerStateCmdOnReportsOn` |
| JPSM-006 | Every command shall complete with exactly one response, and a `GET_POWER_MODE` with no valid reading available shall be rejected `VALIDATION_ERROR` rather than reporting a bogus mode. (Both handlers also have an `JETSON_POWER_STATE_CHANGE_FAILED`/`VALIDATION_ERROR` branch for a `JetsonPowerStateID` outside `{ON, OFF}`, but this cannot be safely unit-tested: constructing an out-of-range `JetsonPowerStateID` and letting anything format/log it -- which both of these paths do -- trips a hard assert in the generated enum-to-string conversion, verified by inspection only.) | `GetPowerModeCmdValidationErrorOnReaderFailure`, `SetJetsonPowerStateCmdOffShutsDownGracefully`, `SetJetsonPowerStateCmdOffReportsExecutionErrorOnFailure` |
| JPSM-007 | The real `nvpmodel`/`shutdown` shell invocations and the `nvpmodel -q` read shall be behind an injectable seam so the rest of this component's behavior can be unit-tested without touching real hardware or a real shell. | N/A (verified by inspection: `configurePowerModeReader`/`configureShellRunner` in `JetsonPowerModeManager.hpp`; every test in this suite overrides both before exercising any handler) |
| JPSM-008 | A `shutdown -h now` invocation killed by `SIGTERM` (this process's own service cgroup torn down as part of the real shutdown it just triggered) shall be classified as likely success, not failure -- it shall not fire `JETSON_POWER_STATE_CHANGE_FAILED`, shall not re-report `ON` (hub-driven path), and shall respond `OK` (local `SET_JETSON_POWER_STATE` command) -- while any other non-zero outcome (real exit failure, a different signal, or `std::system()` failing to spawn) still reports genuine failure exactly as before. | `JetsonPowerStateReceiveOffTreatsSigtermAsSuccess`, `SetJetsonPowerStateCmdOffTreatsSigtermAsSuccess` |
| JPSM-009 | An `nvpmodel -m <mode>` invocation killed by `SIGTERM` (this process's own service cgroup torn down as part of the real reboot it just triggered) shall be classified as likely success, not failure, using the same classifier as JPSM-008 -- it shall not fire `POWER_MODE_CHANGE_FAILED` or re-report the stale mode (hub-driven path) and shall respond `OK` (local `SET_POWER_MODE` command) -- while any other non-zero outcome still reports genuine failure. | `PowerModeReceiveTreatsSigtermAsLikelySuccess`, `PowerModeReceiveReportsFailureOnPackedNonzeroExit`, `SetPowerModeCmdTreatsSigtermAsSuccess` |
| JPSM-010 | The component shall log `JETSON_POWER_MODE_REBOOT_STARTED` immediately before invoking `nvpmodel -m`, whenever a mode change is needed, on both the hub-driven and local entry points. | `PowerModeReceiveChangesModeWhenMismatched` |
| JPSM-011 | The component shall guard against a second mode-change request (hub-driven or local) arriving while a previously-triggered reboot is still pending on this process instance, dropping/rejecting it rather than invoking `nvpmodel` a second time; the guard shall be cleared on a genuine (non-SIGTERM) `nvpmodel` failure, since no reboot is coming and this process instance keeps running. | `PowerModeReceiveIgnoredWhileRebootPending`, `PowerModeReceiveClearsRebootPendingOnGenuineFailure`, `SetPowerModeCmdIgnoredWhileRebootPending` |
| JPSM-012 | `SET_POWER_MODE` shall check `nvpmodel`'s actual (classified) exit status and respond `EXECUTION_ERROR` on genuine failure, `OK` on success/likely-success -- previously it discarded the shell result entirely and always responded `OK`. | `SetPowerModeCmdReportsExecutionErrorOnNvpmodelFailure`, `SetPowerModeCmdTreatsSigtermAsSuccess` |
| JPSM-013 | A LOCAL `SET_POWER_MODE` command (not the hub-driven `powerModeReceive` path) shall notify `JetsonManager`, over the hub, before running `nvpmodel`, so `JetsonManager` can arm its hub-link-distrust guard for this path too -- since `JetsonManager` otherwise has no visibility into a reboot triggered this way. The hub-driven path shall not send this notification, since `JetsonManager` already self-arms its guard before that path's request is even sent. | `SetPowerModeCmdNotifiesLocalModeChangeStarted`, `SetPowerModeCmdSkipsLocalModeChangeStartedWhenAlreadyInMode`, `PowerModeReceiveDoesNotNotifyLocalModeChangeStarted` |

## Change Log
| Date | Description |
|---|---|
| May 7, 2025 | Initial Draft |
| December 4, 2025 | Use nvpmodel to change modes |
| 2026-07-28 | SDD accuracy audit and first real unit test suite for this component (previously untested). Found and fixed two real bugs: (1) `get_nvp_mode()`/`std::system()` were called directly with no test seam at all -- a naive unit test would have shelled out to real `nvpmodel`/`sudo shutdown -h now` on whatever machine ran it. Added `PowerModeReader`/`ShellCommandRunner` injectable function-pointer members (`configurePowerModeReader`/`configureShellRunner`, mirroring the `configureTcpStatusPoller` pattern already used in `GdsCmdAuthMux`), defaulting to the real implementations, so unit tests never touch real hardware/shell state. (2) `SET_JETSON_POWER_STATE_cmdHandler`'s OFF branch called `cmdResponse_out()` twice for the same command -- once unconditionally right after the OFF report, then again after checking the shell result -- which would assert in the real F´ command dispatcher on hardware/GDS. Removed the first, unconditional call so exactly one response is ever sent. Documented the previously-undocumented `jetsonPowerStateReceive`/`jetsonPowerStateSend`/`schedIn` ports, `SET_JETSON_POWER_STATE` command, and `JETSON_POWER_STATE_*` events/telemetry, corrected the stale reference to an "IMX PowerManager"/`scalesSvc::PowerManager` component that does not exist (the real peer is `scalesSvc::JetsonManager`), and flagged two dead/unused declarations found during the audit (the `PWR_MODE_REQ` parameter and the `POWER_MODE_CHANGED` event) rather than guessing at removing or wiring them up. Discovered while writing this suite that a literally out-of-range `JetsonPowerStateID` cannot be safely unit-tested at all in this codebase -- the generated enum-to-string conversion (used whenever a value is formatted for an event or GDS display) hard-asserts on an invalid value, so the two "unrecognized state" defensive branches are verified by inspection only, not by test. Measured 84.8% line, 90.9% function, 48.5% branch coverage (the one uncovered function is the real `get_nvp_mode()`, deliberately never called). Added `Verified By`/`Verifies` traceability and `RecordProperty("requirement", ...)` tags. | Luca Lanzillotta |
| 2026-07-30 | The `jetsonPowerStateReceive` OFF path's `JETSON_POWER_STATE_CHANGE_FAILED` event always reported a fixed `"shutdown command failed"` reason regardless of the actual failure, giving no way to tell a missing/misconfigured sudoers rule apart from a wrong binary path or the shell failing to spawn at all without SSHing into the Jetson to re-run the command by hand. The reason string now reports the actual outcome: `std::system()`'s raw wait-status is decoded into either "system() failed to spawn a shell" (`-1`), "process did not exit normally (raw status N)" (`!WIFEXITED`), or "shutdown exited with status N" (`WEXITSTATUS`) -- the real `sudo`/`shutdown` exit code is now visible directly in GDS/telemetry. `make jetson-setup` (repo root `Makefile`) now also installs a `NOPASSWD: /sbin/shutdown` sudoers rule alongside the existing `nvpmodel` one -- the `sudo -n /sbin/shutdown -h now` call here was already correct, it was simply missing that grant on Jetson images set up before this change (re-run `make jetson-setup` on the Jetson to pick it up). | Luca Lanzillotta |
| 2026-07-30 | **Bug fix (JPSM-008)**: with the sudoers rule from the entry above actually installed, a real hardware OFF request now showed `shutdown -h now` genuinely succeeding but still reporting `JETSON_POWER_STATE_CHANGE_FAILED: "shutdown: process did not exit normally (raw status 15)"` -- raw status 15 is `SIGTERM` (`status & 0x7f == 15`, no core-dump bit): `jetson-deployment.service`'s own cgroup gets torn down as part of the real shutdown sequence it just triggered, killing the `sudo`/`shutdown` child before `std::system()`'s `wait()` observes a clean exit, immediately after the exact command that starts that teardown. This false failure wasn't just a misleading log line: `jetsonPowerStateReceive_handler`'s "shutdown failed, re-report ON" correction propagates through `JetsonManager::currentJetsonPwrState_handler` (which forwards every report to `FPManager` unconditionally) and `FPManager::jetsonPowerStateIn_handler` (which unconditionally trusts every report), flipping `FPManager`'s `m_jetsonPowerState` back to `ON` while it was mid-wait in `disablingHpc` for a confirmed `OFF` -- the sequence still eventually completed once JetsonManager's own grace-period GPIO cut re-reported `OFF`, but only after this spurious detour. Added a file-local `classifyShutdownStatus()` in `JetsonPowerModeManager.cpp` that treats a `SIGTERM`-killed status as `LikelySuccessKilledBySigterm`, distinct from a genuine `Failure` (any other nonzero/signal/spawn-failure outcome, unchanged) or a clean `Success`; applied to both `jetsonPowerStateReceive_handler`'s OFF branch (hub-driven path -- no failure event, no ON re-report) and `SET_JETSON_POWER_STATE_cmdHandler`'s OFF branch (local command path, which had the identical `ret == 0`-only version of this same bug and now responds `OK` instead of `EXECUTION_ERROR`). No new event added for the SIGTERM case -- `JETSON_SHUTDOWN_STARTED` already fired, and the absence of a failure event afterward is itself the "this worked" signal. Added `JetsonPowerStateReceiveOffTreatsSigtermAsSuccess`/`SetJetsonPowerStateCmdOffTreatsSigtermAsSuccess` (JPSM-008); all pre-existing failure-path tests (`-1`, packed nonzero exit) are unaffected since neither matches the `SIGTERM` classification. Coverage: 85.7% line / 91.7% function / 51.8% branch. | Luca Lanzillotta |
| 2026-07-30 | **Bug fix (JPSM-009) + new events (JPSM-010) + reboot-in-flight guard (JPSM-011) + parity fix (JPSM-012)**: tested on hardware, commanding `REQUEST_POWER_MODE` on `JetsonManager` reported `POWER_MODE_CHANGE_FAILED: "nvpmodel returned non-zero exit code"` even though `nvpmodel` genuinely succeeded and the Jetson rebooted to apply the new mode -- the exact JPSM-008 bug class, but for `powerModeReceive_handler`, which had zero raw-status decoding at all (a bare `ret != 0`) and had never been given the `classifyShutdownStatus()` treatment. `nvpmodel -m <mode>` reboots the Jetson, tearing down this same process's own systemd service the same way `shutdown -h now` does, so the identical SIGTERM-collateral signature applies. Renamed the classifier and its enum to `classifySelfDisruptingCommandStatus()`/`SelfDisruptingCommandOutcome` (outcome-neutral, since it now covers both a self-triggered poweroff and a self-triggered reboot) and applied it to `powerModeReceive_handler`'s exit check (JPSM-009). Added `JETSON_POWER_MODE_REBOOT_STARTED`, logged immediately before the `nvpmodel` shell call on both the hub-driven and local (`SET_POWER_MODE`) entry points, mirroring exactly where `JETSON_SHUTDOWN_STARTED` sits relative to its own shell call (JPSM-010). Added a new `m_rebootPending` guard, set immediately before and checked at the top of both `nvpmodel` call sites, to defend against a second overlapping mode-change request in the narrow window before a real reboot severs the hub link/kills this process -- logs `POWER_MODE_REQUEST_IGNORED_REBOOT_PENDING` (hub path) or responds `BUSY` (local path) instead of double-invoking `nvpmodel`; the guard is explicitly cleared on a genuine (non-SIGTERM) failure, since no reboot is coming and this process instance keeps running -- leaving it latched in that case would have permanently blocked all future mode changes (JPSM-011). Also fixed `SET_POWER_MODE_cmdHandler`, which had discarded the shell command's exit status entirely and always responded `OK` regardless of outcome -- it now uses the same classifier and responds `EXECUTION_ERROR` on genuine failure (JPSM-012). Coordinated with a matching `JetsonManager` change (`isJetsonHubLinkTrusted()`, `REQUEST_POWER_MODE`'s new BUSY guard and hub-link-trust rejection, `beginJetsonOffSequence()` deferring OFF while a mode-change reboot is in flight -- see JM-011/012/013 in that SDD) so the i.MX side doesn't send an overlapping `REQUEST_POWER_MODE`/`REQUEST_JETSON_POWER_STATE` in the first place; this component's own `m_rebootPending` guard is defense-in-depth on top of that. Added `PowerModeReceiveReportsFailureOnPackedNonzeroExit`/`PowerModeReceiveTreatsSigtermAsLikelySuccess` (parity with the existing shutdown-path tests, which this mode path never had), `PowerModeReceiveIgnoredWhileRebootPending`/`PowerModeReceiveClearsRebootPendingOnGenuineFailure`, `SetPowerModeCmdReportsExecutionErrorOnNvpmodelFailure`/`SetPowerModeCmdTreatsSigtermAsSuccess`/`SetPowerModeCmdIgnoredWhileRebootPending`; all pre-existing tests pass unmodified except `PowerModeReceiveChangesModeWhenMismatched`/`PowerModeReceiveNoopWhenAlreadyInMode`, which gained assertions on the new reboot-started event's presence/absence. Coverage: 87.5% line / 91.7% function / 52.8% branch. | Luca Lanzillotta |
| 2026-07-30 | **Closed a cross-deployment gap (JPSM-013)**: `SET_POWER_MODE` issued directly against this component (bypassing `JetsonManager`/`FPManager` via the generic i.MX-side remote-command routing path) triggers the same `nvpmodel` reboot as a hub-driven `powerModeReceive`, but `JetsonManager` had no way to learn about it, since it only knows to arm its hub-link-distrust guard when *it* is the one sending the request. Added a new output port, `localModeChangeStarted` (reusing the `PowerModeReceive` port type), called unconditionally from `SET_POWER_MODE_cmdHandler`'s mismatch branch, right after `JETSON_POWER_MODE_REBOOT_STARTED` and before running `nvpmodel`. `powerModeReceive_handler` deliberately does not call it -- `JetsonManager`'s own `REQUEST_POWER_MODE_cmdHandler` already arms its guard before sending that request, so the Jetson-side handler for that path never runs before the i.MX side is already protected. New hub channel: index 6 (confirmed free in both topology files). Companion change on `JetsonManager` (JM-014/JM-015, see that SDD): a new `localModeChangeStarted` input arms the same `m_hasPendingCmd` guard `REQUEST_POWER_MODE_cmdHandler` uses (guarded against clobbering a real in-flight request), and a new `fpJetsonHubTrustedOut` republishes the resulting hub-trust verdict to FPManager every tick (FP-022 there), since this remote-command path never touches JetsonManager or FPManager either. Added `SetPowerModeCmdNotifiesLocalModeChangeStarted`, `SetPowerModeCmdSkipsLocalModeChangeStartedWhenAlreadyInMode`, `PowerModeReceiveDoesNotNotifyLocalModeChangeStarted`. Coverage: 87.6% line / 91.7% function / 52.8% branch. | Luca Lanzillotta |
