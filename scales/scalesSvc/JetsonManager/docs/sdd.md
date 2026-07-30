# scalesSvc::JetsonManager

Component that manages Jetson power state from the i.MX side and sends Jetson
power-mode requests to the Jetson power-mode manager.

### Typical Usage
`JetsonManager` receives GDS commands for Jetson power state and power mode. The
`REQUEST_JETSON_POWER_STATE` command is first authorized by `FPManager` through
`fpJetsonPowerAuthorize`; this allows `FPManager` to reject Jetson ON requests
unless HPC Mode is enabled. Jetson OFF requests are allowed in Safe Mode and are
handled as an idempotent direct GPIO action when the Jetson is already known
OFF.

Powering the Jetson ON drives the Jetson power GPIO high and completes the
command immediately -- but that is *not* confirmation the Jetson has actually
booted. `m_jetsonPowerStateKnown`/`m_currentJetsonPowerState` are updated only
by a real report received from the Jetson (`currentJetsonPwrState_handler`),
never by the ON command itself. Instead, commanding ON sets a separate
`m_awaitingBootConfirmation` guard that rejects a commanded OFF outright
(`BUSY` + `JETSON_OFF_REJECTED_BOOTING`) until either the Jetson's first real
report arrives or `CMD_TIMEOUT_TICKS` elapses with no report at all (logging
`JETSON_BOOT_CONFIRMATION_TIMEOUT` and giving up on that basis, after which
OFF falls through to the normal confirmed-state gate below). Without this,
an OFF sent while the Jetson was still booting raced the boot: it could take
the graceful hub-routed path against a link that wasn't up yet, and its
`m_hasPendingPowerCmd` bookkeeping could then sit `BUSY` -- rejecting every
subsequent `REQUEST_JETSON_POWER_STATE`, ON or OFF -- for the full
`CMD_TIMEOUT_TICKS` window. See JM-009.

A commanded Jetson OFF uses the graceful-ish Jetson-side
shutdown path only when the Jetson's power state is *confirmed* on:
`JetsonManager` sends `reqJetsonPwrState(OFF)`, waits for
`currentJetsonPwrState(OFF)`, waits `JETSON_POWER_OFF_DELAY_TICKS`, then
drives the GPIO low and completes the command. Otherwise -- confirmed off,
never confirmed either way, or the Jetson-side shutdown port unavailable --
OFF is completed immediately as an idempotent direct GPIO-low action.

"Confirmed" specifically means a real status report has been received from the
Jetson (`currentJetsonPwrState`), or JetsonManager itself already took a GPIO
action -- not just whatever `m_currentJetsonPowerState` happens to hold at
that moment. `reqJetsonPwrState_out()` is wired straight through GenericHub
into `imx_hubComStub.dataIn` with no queue or gate in between (see
`ImxDeployment/Top/topology.fpp`); if the underlying TCP link to the Jetson
isn't actually connected, `Svc::ComStub`'s "never send while reinitializing"
`FW_ASSERT` trips immediately and takes down the *entire* i.MX flight
software -- the same class of bug already fixed for `remoteJetsonCmdIn` (see
that port's topology comment). A real report received over the hub link is
the only evidence JetsonManager ever has that the link is alive, so an
*unconfirmed* state must be treated the same as confirmed-off: go straight to
the direct, hardware-safe GPIO cut, never attempt the hub call. This closes
that crash at the cost of the narrow case where the i.MX rebooted
independently while the Jetson stayed alive and hasn't re-reported yet --
that window closes as soon as the Jetson's next boot-time report arrives, and
a genuinely-off Jetson is unaffected either way. See JM-006 and the
2026-07-30 change log entry (this superseded an earlier, 2026-07-28 version of
this same gate that went the other way -- preferring the graceful/hub path
whenever the state *wasn't confirmed off* -- which reintroduced exactly this
ComStub crash whenever OFF was requested on a fresh or rebooted i.MX with the
Jetson actually off).

Internal FPManager recovery, HPC disable, and emergency OFF requests
(`fpJetsonPowerRequestIn`) use the *same* graceful-only-when-confirmed-on gate
as the GDS-facing command -- they are not unconditionally direct. A
graceful attempt that never gets acknowledged still falls back to a direct
GPIO cut via the same bounded `schedIn` timeout used by the commanded path, so
these protection paths are still guaranteed to complete without depending
indefinitely on the Jetson-side software or communication link.

`REQUEST_POWER_MODE` still routes through the Jetson hub link because it requires
the Jetson-side power-mode manager to apply the mode.

## Class Diagram
Add a class diagram here

## Port Descriptions
| Name | Description |
|---|---|
| `currentPwrMode` | Current Jetson power mode reported by the Jetson-side manager. |
| `currentJetsonPwrState` | Current Jetson power state reported by the Jetson-side manager. |
| `fpJetsonPowerRequestIn` | Internal FPManager recovery, HPC disable, and emergency OFF request. Only OFF is acted on. If the Jetson is known ON and the Jetson-side request path is available, this requests graceful Jetson shutdown first; otherwise it drives GPIO low directly. |
| `fpJetsonPowerAuthorize` | Synchronous gate called before `REQUEST_JETSON_POWER_STATE` executes. |
| `fpJetsonPowerStateOut` | Current Jetson power state reported to FPManager after JetsonManager commands or receives a power-state update. |
| `schedIn` | Rate-group tick used for deferred power-mode timeout handling. |
| `reqPwrMode` | Outbound request to the Jetson-side power-mode manager. |
| `reqJetsonPwrState` | Jetson-side graceful shutdown request path used by commanded OFF when the Jetson is known ON. |
| `gpioSet` | GPIO write used to physically control Jetson power. |

## Component States
| Name | Description |
|---|---|
| No pending mode command | Normal state after initialization or command completion. |
| Pending mode command | Waiting for Jetson to report the requested power mode after a mode request. |

## Sequence Diagrams
Add sequence diagrams here

## Parameters
| Name | Description |
|---|---|
| `JETSON_POWER_OFF_DELAY_TICKS` | Number of scheduler ticks to wait after the Jetson reports OFF before cutting GPIO power in the graceful commanded-OFF path. |

## Commands
| Name | Description |
|---|---|
| `REQUEST_POWER_MODE` | Requests a Jetson power-mode change through the Jetson-side manager and waits for confirmation or timeout. |
| `REQUEST_JETSON_POWER_STATE` | Requests Jetson ON or OFF after FPManager authorization. ON drives GPIO high and starts the boot-confirmation window. OFF is rejected `BUSY` while that window is open (JM-009); once closed, OFF is graceful when the Jetson is confirmed ON, and direct/idempotent otherwise. |

## Events
| Name | Description |
|---|---|
| `POWER_MODE_REQUESTED` | A Jetson power-mode request was sent. |
| `POWER_MODE_RECEIVED` | A current Jetson power mode was received. |
| `JETSON_POWER_STATE_REQUESTED` | A Jetson power-state command was accepted. |
| `JETSON_POWER_STATE_RECEIVED` | A current Jetson power state was received. |
| `JETSON_POWER_STATE_TIMEOUT` | A deferred Jetson power-state operation timed out. |
| `JETSON_OFF_REJECTED_BOOTING` | A commanded OFF was rejected because the Jetson was commanded ON and hasn't reported in yet. |
| `JETSON_BOOT_CONFIRMATION_TIMEOUT` | JetsonManager gave up waiting for the Jetson's first report after ON; OFF is no longer rejected on that basis. |

## Telemetry
| Name | Description |
|---|---|
| `JetsonPowerMode` | Last reported Jetson power mode. |
| `JetsonPowerState` | Last commanded or reported Jetson power state. |

## Unit Tests

Measured via `fprime-util check --coverage`: **94.1% line (174/185), 100%
function (8/8), 56.4% branch (141/250)**. The remaining gaps are ASan/UBSan
instrumentation edges around construction (the same non-actionable pattern
documented throughout this audit). Each test is tagged with
`RecordProperty("requirement", "<REQ-IDs>")`, so running the test binary
with `--gtest_output=xml:<path>` produces a JUnit-style XML report whose
`<testcase>` elements carry that mapping as a machine-checkable artifact.

| Name | Description | Verifies |
|---|---|---|
| `RequestPowerModeDeferredCompletion` | Sends `REQUEST_POWER_MODE`, confirms it stays open (no immediate response), confirms a mismatched `currentPwrMode` report does not complete it, then confirms a matching report completes it with `OK`. | JM-004 |
| `RequestPowerModeTimeout` | Sends `REQUEST_POWER_MODE` and never reports a matching mode; confirms it stays open through 119 ticks and completes with `EXECUTION_ERROR` on the 120th (`CMD_TIMEOUT_TICKS`). | JM-004 |
| `RequestJetsonPowerStateOnDrivesGpioImmediately` | Sends `REQUEST_JETSON_POWER_STATE(ON)` with authorization granted; confirms GPIO high, telemetry, the report to FPManager, and an immediate `OK`. | JM-001 |
| `RequestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut` | Regression test for the ComStub crash this fixed: on a fresh component (state unconfirmed, cached default is OFF), sends `REQUEST_JETSON_POWER_STATE(OFF)` and confirms it takes the direct GPIO-cut path immediately -- NOT the hub-routed `reqJetsonPwrState` call -- since the hub link's liveness is unproven when the Jetson has never reported in. | JM-006 |
| `RequestJetsonPowerStateOffRejectedWhileBooting` | Regression test for the BUSY-lock bug this fixed: commands ON, then immediately commands OFF before any real report arrives; confirms OFF is rejected `BUSY` with `JETSON_OFF_REJECTED_BOOTING` and neither a hub call nor a GPIO cut happens, then confirms a real ON report clears the guard and a subsequent OFF takes the normal graceful path. | JM-009 |
| `RequestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout` | Commands ON and never reports in; confirms the boot-confirmation guard holds through 119 ticks, clears on the 120th (`JETSON_BOOT_CONFIRMATION_TIMEOUT`), and a subsequent OFF is no longer rejected on that basis (falls to the safe direct GPIO cut, since the state is still otherwise unconfirmed). | JM-009 |
| `RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower` | Confirms ON via a real status report, requests OFF, confirms the graceful request and no immediate GPIO cut, confirms a matching OFF report starts the delay window, confirms GPIO stays high through `JETSON_POWER_OFF_DELAY_TICKS - 1` ticks, then cuts on the final tick with `OK`. | JM-005, JM-006 |
| `RequestJetsonPowerStateOffConfirmedOffIsIdempotent` | Confirms OFF via a real status report, then requests OFF again; confirms it completes synchronously via direct GPIO cut without ever attempting the graceful path. | JM-002, JM-006 |
| `RequestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut` | Confirms ON, requests OFF, and never acknowledges it; confirms `JETSON_POWER_STATE_TIMEOUT` fires and GPIO is cut directly after `CMD_TIMEOUT_TICKS`. | JM-003 |
| `RequestJetsonPowerStateRejectedByAuthorization` | Mocks `fpJetsonPowerAuthorize` to return `FAILURE`; confirms `VALIDATION_ERROR` and no GPIO action. | JM-001 |
| `RequestJetsonPowerStateBusyWhilePending` | Sends a second `REQUEST_JETSON_POWER_STATE` while the first is still outstanding; confirms `BUSY` and that the first request's state is untouched. | JM-007 |
| `FpJetsonPowerRequestInIgnoresOnAndActsOnOff` | Confirms an ON request on the internal FPManager path is a no-op, then confirms an OFF request (Jetson known ON) takes the graceful path, mirroring the GDS-facing command. | JM-003 |
| `CurrentJetsonPwrStateIgnoredWithoutPendingCommand` | Sends an unsolicited `currentJetsonPwrState` report with no pending power command; confirms telemetry/cache still update but no GPIO or command-response action is taken. | JM-008 |

## Requirements
| Name | Description | Verified By |
|---|---|---|
| JM-001 | Jetson ON shall require successful FPManager authorization. | `RequestJetsonPowerStateOnDrivesGpioImmediately`, `RequestJetsonPowerStateRejectedByAuthorization` |
| JM-002 | Jetson OFF shall be accepted in Safe Mode and shall not depend on the Jetson hub link when the Jetson is already known OFF. | `RequestJetsonPowerStateOffConfirmedOffIsIdempotent` |
| JM-003 | FPManager recovery OFF shall prefer Jetson-side graceful shutdown when the Jetson is known ON, but shall fall back to direct GPIO low when the Jetson-side path is unavailable or times out. | `RequestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut`, `FpJetsonPowerRequestInIgnoresOnAndActsOnOff` |
| JM-004 | Jetson power-mode requests shall be deferred until the requested mode is reported or timeout occurs. | `RequestPowerModeDeferredCompletion`, `RequestPowerModeTimeout` |
| JM-005 | Commanded Jetson OFF while the Jetson is known ON shall request Jetson-side shutdown before cutting GPIO power. | `RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower` |
| JM-006 | Commanded Jetson OFF shall take the graceful Jetson-side shutdown path only when the Jetson's power state has been *confirmed* on (a real status report received over the hub link) -- an unconfirmed cached state (e.g. the boot-time default) must never attempt the hub-routed `reqJetsonPwrState_out()` call, since the hub link's liveness is unproven and doing so can trip `Svc::ComStub`'s never-connected `FW_ASSERT` and crash the entire i.MX flight software; unconfirmed and confirmed-off both fall back to the same direct, idempotent GPIO cut. | `RequestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut`, `RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower`, `RequestJetsonPowerStateOffConfirmedOffIsIdempotent` |
| JM-007 | A `REQUEST_JETSON_POWER_STATE` received while a previous one is still outstanding shall be rejected `BUSY`. (The handler also has a `VALIDATION_ERROR` branch for a `JetsonPowerStateID` outside `{ON, OFF}`, but this cannot be safely unit-tested: the command dispatcher itself rejects an out-of-range enum with `FORMAT_ERROR` before the handler ever runs, and even directly constructing an out-of-range `JetsonPowerStateID` value trips a hard assert elsewhere the moment anything tries to format/log it -- verified by inspection only.) | `RequestJetsonPowerStateBusyWhilePending` |
| JM-008 | An unsolicited `currentJetsonPwrState` report (no pending power command) shall still update the cached state and telemetry, but shall take no GPIO or command-response action. | `CurrentJetsonPwrStateIgnoredWithoutPendingCommand` |
| JM-009 | A commanded OFF shall be rejected `BUSY` while a commanded ON is still awaiting the Jetson's first real report (the boot-confirmation window), rather than racing the graceful shutdown path against a hub link that may not exist yet; the window shall be bounded by `CMD_TIMEOUT_TICKS` so it can never block OFF indefinitely if the Jetson never reports in. | `RequestJetsonPowerStateOffRejectedWhileBooting`, `RequestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout` |

## Change Log
| Date | Description |
|---|---|
| 2026-07-22 | Documented FPManager authorization and direct GPIO OFF behavior. |
| 2026-07-22 | Added Jetson power-state reporting from JetsonManager to FPManager. |
| 2026-07-22 | Restored graceful commanded Jetson OFF when Jetson is known ON while keeping FPManager protection OFF direct. |
| 2024-02-28 | Initial Draft |
| 2026-07-28 | **Bug fix**: `m_currentJetsonPowerState` defaulted to `OFF` at i.MX boot and was trusted as fact by `REQUEST_JETSON_POWER_STATE(OFF)`'s graceful-vs-direct gate -- if OFF was requested before the Jetson's first status report ever arrived (e.g. the i.MX rebooted independently while the Jetson stayed powered and running), JetsonManager wrongly believed the Jetson was already off and cut GPIO power directly, skipping the graceful `reqJetsonPwrState`/`shutdown -h now` request entirely and yanking power from a live Linux system. Added `m_jetsonPowerStateKnown`, set only by a real confirmed report or a GPIO action JetsonManager itself took; the graceful-vs-direct gate now checks "confirmed off", not just "cached value is OFF" (JM-006). This applies to both the GDS-facing `REQUEST_JETSON_POWER_STATE` command and the internal `fpJetsonPowerRequestIn` path. Added `friend class JetsonManagerTester;` and wrote the first real unit test suite for this component (previously verified only by "the deployment builds"), covering JM-001 through JM-008 (JM-007/JM-008 newly documented -- the BUSY/invalid-state guard and unsolicited-report handling were implemented but never previously written down). Measured 94.5% line, 100% function, 55.3% branch coverage. Added `RecordProperty("requirement", ...)` traceability tags and `Verified By`/`Verifies` columns. | Luca Lanzillotta |
| 2026-07-30 | **Bug fix (supersedes 2026-07-28's JM-006 fix)**: the previous fix made `REQUEST_JETSON_POWER_STATE(OFF)`/`fpJetsonPowerRequestIn` prefer the graceful `reqJetsonPwrState_out()` hub call whenever the Jetson's state merely *wasn't confirmed off* -- including the never-confirmed boot-time default. On hardware this meant enabling HPC Mode and immediately requesting Jetson OFF (with the Jetson genuinely powered off and its hub link never established) sent a port call straight through GenericHub into `imx_hubComStub.dataIn`, which has no queue or connectivity gate in front of it; `Svc::ComStub::dataIn_handler`'s `FW_ASSERT(!this->m_reinitialize || !this->isConnected_comStatusOut_OutputPort(0))` (ComStub.cpp:28) tripped immediately, and `FPManager::fatalIn_handler` latched `EMERGENCY_REBOOT`, restarting the *entire* i.MX flight software over what should have been a routine "Jetson is already off" acknowledgment -- the same crash class already fixed for `remoteJetsonCmdIn` (`ImxDeployment/Top/topology.fpp`), which this port had never been given the equivalent gate for. Flipped both gates (`REQUEST_JETSON_POWER_STATE_cmdHandler`'s OFF branch and `fpJetsonPowerRequestIn_handler`) to require *confirmed on* (`m_jetsonPowerStateKnown && m_currentJetsonPowerState.e == ON`) before ever attempting `reqJetsonPwrState_out()`; unconfirmed and confirmed-off now both take the same direct, idempotent, hub-independent GPIO cut. This knowingly reopens a narrower version of the 2026-07-28 concern (an i.MX that reboots independently while the Jetson stays alive will get a direct GPIO cut instead of a graceful ask, until the Jetson's next boot-time report re-confirms ON) in exchange for never crashing the whole flight computer over a Jetson power request -- the deliberate trade-off the fix prioritizes. Renamed `RequestJetsonPowerStateOffUnconfirmedPrefersGraceful` to `RequestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut` with inverted assertions, and updated `RequestJetsonPowerStateBusyWhilePending` to confirm ON first so its first OFF request still exercises the pending/BUSY path. | Luca Lanzillotta |
| 2026-07-30 | **Bug fix (JM-009)**: the 2026-07-30 JM-006 fix above removed the ComStub crash but exposed a second bug from the same root cause -- `REQUEST_JETSON_POWER_STATE_cmdHandler`'s ON branch set `m_jetsonPowerStateKnown = true`/`m_currentJetsonPowerState = ON` *optimistically*, the instant GPIO was driven high, not when the Jetson actually booted. A commanded OFF sent moments later (before the Jetson had booted far enough to be reachable over the hub) read that optimistic flag as "confirmed on," took the graceful `reqJetsonPwrState_out()` path against a hub link that wasn't up yet, and its `m_hasPendingPowerCmd` bookkeeping then sat `BUSY` for the full `CMD_TIMEOUT_TICKS` window -- during which *every* `REQUEST_JETSON_POWER_STATE`, ON or OFF, was rejected `BUSY`, observed on hardware as ON/OFF "no longer working" after testing an OFF sent while the Jetson was still booting. Stopped the ON branch from touching `m_jetsonPowerStateKnown`/`m_currentJetsonPowerState` at all (only a real report from `currentJetsonPwrState_handler` sets those now) and added a dedicated `m_awaitingBootConfirmation` guard, set when ON is commanded and cleared either by the Jetson's first real report or by a bounded `CMD_TIMEOUT_TICKS` timeout (`JETSON_BOOT_CONFIRMATION_TIMEOUT`) if it never reports in. A commanded OFF is now rejected outright (`BUSY` + new `JETSON_OFF_REJECTED_BOOTING` event) while that guard is set, instead of racing the boot. Added `RequestJetsonPowerStateOffRejectedWhileBooting` and `RequestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout`; all pre-existing tests pass unmodified (none exercised ON followed immediately by OFF against the optimistic-confirmation path). Coverage: 94.1% line / 100% function / 56.4% branch. | Luca Lanzillotta |
