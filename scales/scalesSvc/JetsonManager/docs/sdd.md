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
booted. `m_jetsonPowerStateKnown`/`m_currentJetsonPowerState` (and,
downstream, FPManager's own `m_jetsonPowerState`) are updated only by a real
report received from the Jetson (`currentJetsonPwrState_handler`), never by
the ON command itself -- `fpJetsonPowerStateOut` must never be sent
optimistically the instant ON is commanded, or FPManager's own Jetson-on
gating (`remoteJetsonCmdIn_handler`, `jetsonPowerAuthorizeIn_handler`) would
be defeated during exactly the boot window it exists to protect (JM-010).
Instead, commanding ON sets a separate `m_awaitingBootConfirmation` guard --
but only on a genuine off->on transition: if the Jetson is already confirmed
on (a real report already arrived), a redundant ON command leaves the guard
alone (clearing it if it somehow were still set) rather than re-arming it. A
redundant ON doesn't cause the Jetson to send a fresh unsolicited report, so
re-arming on it would have nothing left to clear the guard except the
bounded timeout -- this was a real bug: sending ON once (booting the
Jetson), waiting for confirmation, then sending ON again (e.g. to double
check) re-armed the guard and left a subsequent OFF stuck for up to
`CMD_TIMEOUT_TICKS`.

A commanded OFF that arrives while `m_awaitingBootConfirmation` is set is
**not** rejected. It is *deferred* (`m_deferredOffPending`, no
`cmdResponse_out` yet) and automatically fired -- via the shared
`beginJetsonOffSequence()` helper, which then takes the normal
graceful-vs-direct-cut path below -- the instant a real report arrives
(`currentJetsonPwrState_handler`) or the boot window times out
(`schedIn_handler` force-fires it via a direct GPIO cut once
`CMD_TIMEOUT_TICKS` elapses with no report at all, logging
`JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT`; if no OFF was ever deferred, the
timeout instead just clears the guard and logs
`JETSON_BOOT_CONFIRMATION_TIMEOUT` as before). This applies identically
whether the OFF intent came from the GDS-facing `REQUEST_JETSON_POWER_STATE`
command or FPManager's internal `fpJetsonPowerRequestIn` path (e.g.
`DISABLE_HPC_MODE`) -- neither may ever race the boot by sending the
graceful hub request or cutting GPIO power to a Jetson that hasn't reported
in yet, but both must still eventually complete rather than being rejected
outright. See JM-009.

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

`REQUEST_POWER_MODE` still routes through the Jetson hub link because it
requires the Jetson-side power-mode manager to apply the mode, via the
outbound `reqPwrMode` port. That port turned out to have the *exact* same
unguarded ComStub-crash exposure `reqJetsonPwrState` had before JM-006:
wired straight through GenericHub with no queue/gate, called completely
unconditionally with no confirmed-link check of any kind. `REQUEST_POWER_MODE`
also had no `m_hasPendingCmd` BUSY guard at all (unlike
`REQUEST_JETSON_POWER_STATE`'s `m_hasPendingPowerCmd` check) -- a second
request while one was outstanding silently clobbered the first's tracked
opcode/seq and re-fired `reqPwrMode_out()`. A private helper,
`isJetsonHubLinkTrusted()`, now gates every hub-routed send from this
component (both `reqPwrMode_out` and `reqJetsonPwrState_out`): true only
when the Jetson is confirmed on, not still awaiting its first boot
confirmation, and not already mid a `REQUEST_POWER_MODE`-triggered reboot.
`REQUEST_POWER_MODE_cmdHandler` now checks `m_hasPendingCmd` first (`BUSY`
if already outstanding), then `isJetsonHubLinkTrusted()` -- unlike Jetson
OFF, there is no hardware-safe fallback action for "set power mode," so an
untrusted link fails the command fast (`VALIDATION_ERROR`) rather than
risking the hub call or deferring indefinitely. This also keeps
`m_hasPendingCmd` and `m_awaitingBootConfirmation` provably mutually
exclusive: a `REQUEST_POWER_MODE` commanded while the Jetson is still
booting for the first time is rejected outright, not deferred (a resolved
design fork -- deferring would need its own new pending-mode state and an
unresolved policy for which of two simultaneously-deferred intents, OFF vs.
mode, wins if both end up waiting on the same boot confirmation). See
JM-011/JM-012/JM-013.

A `REQUEST_POWER_MODE`-triggered reboot leaves `m_currentJetsonPowerState`
`ON` throughout (the Jetson never loses GPIO power, only its OS/hub link is
temporarily down while it reboots to apply the new mode) -- so
`beginJetsonOffSequence()`'s `confirmedOn` check alone would have
incorrectly trusted the hub link during that window too. It now also defers
(reusing `m_deferredOffPending`, same as the boot-confirmation case) when
`m_hasPendingCmd` is set, logging `JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING`;
`currentPwrMode_handler` resumes it on a matching mode confirmation, and
`schedIn_handler`'s existing `m_hasPendingCmd`/`CMD_TIMEOUT_TICKS` timeout
resumes it (`JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT`) if the mode
change is never confirmed -- mirroring the boot-confirmation defer/auto-fire/
force-resume pattern exactly, just keyed on a different "hub link can't be
trusted right now" reason. Since `beginJetsonOffSequence()` now owns both
defer decisions itself, its two callers (`REQUEST_JETSON_POWER_STATE`'s OFF
branch, `fpJetsonPowerRequestIn_handler`) no longer special-case
`m_awaitingBootConfirmation` themselves -- they just call it unconditionally.

A `SET_POWER_MODE` command sent directly to the Jetson-side power-mode manager
(bypassing `JetsonManager`/`FPManager` entirely, via the generic remote-command
routing path) can also trigger an `nvpmodel`-driven reboot -- but
`JetsonManager` previously had zero visibility into it, so
`isJetsonHubLinkTrusted()` stayed (wrongly) true throughout that reboot too.
`JetsonPowerModeManager` now proactively notifies `JetsonManager` over the hub,
via a new `localModeChangeStarted` port (reusing the `PowerModeReceive` port
type), right before running `nvpmodel` locally. `localModeChangeStarted_handler`
arms the same `m_hasPendingCmd` guard `REQUEST_POWER_MODE_cmdHandler` uses --
but only if no guard is already armed (a no-op if a real hub-driven
`REQUEST_POWER_MODE` is already in flight, so as not to clobber that command's
own opcode/cmdSeq/response bookkeeping) -- and sets `m_modeChangeCmdRespond =
false`, since there is no opcode/cmdSeq to respond to. `currentPwrMode_handler`
was restructured so a matching-mode response only completes a real command
(`m_modeChangeCmdRespond == true`); an externally-triggered guard (armed by
`localModeChangeStarted`) instead clears on *any* report, since there is no
specific mode to match against. See JM-014.

`JetsonManager` also reports its own hub-trust verdict outward, so `FPManager`
can gate the same risk on its *own* remote-command path (the generic
GDS/CmdSequencer routing to the Jetson, which never touches `JetsonManager` at
all): a new `fpJetsonHubTrustedOut` port republishes `isJetsonHubLinkTrusted()`
unconditionally on every `schedIn` tick (not just on change), so a GDS session
that connects late still sees an accurate value -- mirroring FPManager's own
`FAULT_DEBOUNCE_COUNT` telemetry republish pattern. `FPManager`'s
`remoteJetsonCmdIn_handler` was extended with `jetsonHubTrustedIn`, defaulting
true at construction (harmless, since the existing `m_jetsonPowerState != ON`
gate already blocks everything until the Jetson's first real report
regardless). See JM-015 and FPManager's FP-022.

Hardware testing then surfaced a second, more fundamental gap in this same
mechanism: `JetsonPowerModeManager`'s own process can report a matching mode
over the hub -- clearing the externally-triggered `m_hasPendingCmd` guard
above -- well before the actual Linux `reboot()` that `nvpmodel` triggers has
severed the TCP link; `nvpmodel` applies the mode and returns success
synchronously, while the reboot that follows is asynchronous and can lag by
seconds. No application-level self-report from that same still-running
process can ever prove the reboot has physically happened -- only the
transport layer itself can. `JetsonManager` now also tracks the *real*
connection status of the imx<->Jetson hub link via a new `hubComStatusIn`
port (a standard `Svc::ComStub`/`Fw.SuccessCondition` signal: `SUCCESS` on a
genuine reconnect, `FAILURE` the first time a send actually fails), fed from
`imx_hubComStub.comStatusOut` via `imx_hubComAdapter`'s
`comStatusIn`/`comStatusOut[2]` fan-out (see below and `HubComAdapter`'s own
SDD) -- the same signal also feeds a `Svc::ComQueue` inserted into the hub
send path, so this port's value is no longer the only thing standing between
a premature app-level belief and the crash it used to risk (see the
`imx_hubComQueue` note below). `isJetsonHubLinkTrusted()` now additionally
requires `m_hubLinkConnected`, closing the race regardless of how or when the
mode-match self-report arrives (JM-016). `beginJetsonOffSequence()`'s
graceful-vs-direct-cut decision, which used to hand-derive its own
`confirmedOn` subset of the same conditions, now calls
`isJetsonHubLinkTrusted()` directly instead, so it can never drift out of sync
with `REQUEST_POWER_MODE_cmdHandler`'s gate again and automatically inherits
the same fix (JM-017). `FPManager` needed no changes -- it already trusts
`fpJetsonHubTrustedOut`, which is now backed by a more accurate signal. Two
new events, `JETSON_HUB_LINK_DOWN`/`JETSON_HUB_LINK_RECONNECTED`, log only on
actual transitions (not every `comStatus` report) for GDS visibility.

**Structural backstop added after this fix shipped**: `imx_hubComQueue`
(`Svc::ComQueue`), inserted into `ImxDeployment/Top/topology.fpp`'s
`send_hub` connections between `imx_hub` and `imx_hubFramer`, now gates
*every* hub-bound send -- from any component, including ones not yet
written -- on the same real `comStatus` signal, holding a buffer until the
last known status is `SUCCESS` rather than forwarding it unconditionally.
This makes `Svc::ComStub`'s never-connected `FW_ASSERT` structurally
unreachable from this link regardless of what any application-level check
believes, closing the gap for future callers that might not remember to
check `isJetsonHubLinkTrusted()`/`m_jetsonHubTrusted`. It does not replace
JM-016/FP-022: those stay as the fast, informative layer that gives an
operator an immediate `BUSY`/`VALIDATION_ERROR` instead of a command
silently sitting in (or being dropped from) the queue. See `HubComAdapter`'s
SDD (HSR-001) for the `comStatus` fan-out this required, and
`ImxDeployment/Top/instances.fpp`/`ImxDeploymentTopology.cpp` for
`imx_hubComQueue`'s configuration. No `JetsonManager` code changed for this
addition.

## Class Diagram
Add a class diagram here

## Port Descriptions
| Name | Description |
|---|---|
| `currentPwrMode` | Current Jetson power mode reported by the Jetson-side manager. |
| `currentJetsonPwrState` | Current Jetson power state reported by the Jetson-side manager. |
| `localModeChangeStarted` | Notification from `JetsonPowerModeManager`, over the hub, that a *local* (non-hub) `SET_POWER_MODE` command is about to reboot the Jetson -- arms the same hub-link-distrust guard as a hub-driven `REQUEST_POWER_MODE` (JM-014). |
| `fpJetsonPowerRequestIn` | Internal FPManager recovery, HPC disable, and emergency OFF request. Only OFF is acted on. If the Jetson is known ON and the Jetson-side request path is available, this requests graceful Jetson shutdown first; otherwise it drives GPIO low directly. |
| `fpJetsonPowerAuthorize` | Synchronous gate called before `REQUEST_JETSON_POWER_STATE` executes. |
| `fpJetsonPowerStateOut` | Current Jetson power state reported to FPManager after JetsonManager commands or receives a power-state update. |
| `fpJetsonHubTrustedOut` | Current hub-link-trust verdict (`isJetsonHubLinkTrusted()`) republished to FPManager every `schedIn` tick, so its own remote-command gate (`remoteJetsonCmdIn_handler`) can reject Jetson-bound GDS/CmdSequencer traffic during a reboot it has no other visibility into (JM-015). |
| `hubComStatusIn` | Real transport-level status of the imx<->Jetson hub TCP link, fed directly from `imx_hubComStub.comStatusOut` (`SUCCESS` on a genuine reconnect, `FAILURE` the first time a send actually fails). Required, alongside the app-level signals above, for `isJetsonHubLinkTrusted()` to report true (JM-016). |
| `schedIn` | Rate-group tick used for deferred power-mode timeout handling and republishing `fpJetsonHubTrustedOut`. |
| `reqPwrMode` | Outbound request to the Jetson-side power-mode manager. Wired straight through GenericHub with no queue/gate, same as `reqJetsonPwrState` -- only called when `isJetsonHubLinkTrusted()` (JM-011). |
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
| `REQUEST_POWER_MODE` | Requests a Jetson power-mode change through the Jetson-side manager and waits for confirmation or timeout. Rejected `BUSY` if a previous request is still outstanding (JM-012), or `VALIDATION_ERROR` if the hub link isn't currently trusted -- confirmed off, never confirmed, still booting, or a previous mode-change reboot already in flight (JM-013). |
| `REQUEST_JETSON_POWER_STATE` | Requests Jetson ON or OFF after FPManager authorization. ON drives GPIO high and starts the boot-confirmation window. OFF requested while that window is open, or while a `REQUEST_POWER_MODE`-triggered reboot is in flight, is deferred (accepted, held open) and fires automatically once the Jetson's boot/mode change is confirmed or the relevant window times out (JM-009/JM-011); once confirmed, OFF is graceful when the Jetson is confirmed ON, and direct/idempotent otherwise. |

## Events
| Name | Description |
|---|---|
| `POWER_MODE_REQUESTED` | A Jetson power-mode request was sent. |
| `POWER_MODE_RECEIVED` | A current Jetson power mode was received. |
| `JETSON_POWER_STATE_REQUESTED` | A Jetson power-state command was accepted. |
| `JETSON_POWER_STATE_RECEIVED` | A current Jetson power state was received. |
| `JETSON_POWER_STATE_TIMEOUT` | A deferred Jetson power-state operation timed out. |
| `JETSON_BOOT_CONFIRMATION_TIMEOUT` | JetsonManager gave up waiting for the Jetson's first report after ON, with no OFF request deferred pending it. |
| `JETSON_OFF_DEFERRED_BOOTING` | A commanded/internal Jetson OFF request was deferred because the Jetson was commanded ON and hasn't reported in yet; it will fire automatically once boot is confirmed. |
| `JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT` | JetsonManager gave up waiting for the Jetson's first report while an OFF request was deferred pending it; the OFF was force-completed via a direct GPIO cut. |
| `POWER_MODE_REQUEST_REJECTED` | A `REQUEST_POWER_MODE` command was rejected because the hub link to the Jetson cannot currently be trusted (confirmed off, never confirmed, still booting, or a previous mode-change reboot already in flight). |
| `JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING` | A commanded/internal Jetson OFF request was deferred because a `REQUEST_POWER_MODE`-triggered reboot is currently in flight; it will fire automatically once the mode change is confirmed or its window times out. |
| `JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT` | JetsonManager gave up waiting for the Jetson's power-mode confirmation while a deferred OFF request was also pending; the deferred OFF is now resumed. |
| `LOCAL_MODE_CHANGE_STARTED_RECEIVED` | JetsonManager received a `localModeChangeStarted` notification (a `SET_POWER_MODE` command was issued directly against the Jetson, bypassing JetsonManager/FPManager) and armed the hub-link-distrust guard. |
| `JETSON_HUB_LINK_DOWN` | The real imx<->Jetson hub TCP link was just lost (`comStatusOut` reported `FAILURE` for the first time since the last reconnect); logged only on the down transition. |
| `JETSON_HUB_LINK_RECONNECTED` | The real imx<->Jetson hub TCP link just came back (`comStatusOut` reported `SUCCESS` after having been down); logged only on the up transition. |

## Telemetry
| Name | Description |
|---|---|
| `JetsonPowerMode` | Last reported Jetson power mode. |
| `JetsonPowerState` | Last commanded or reported Jetson power state. |

## Unit Tests

Measured via `fprime-util check --coverage`: **98.3% line (228/232), 100%
function (12/12), 62.5% branch (190/304)**. The remaining gaps are ASan/UBSan
instrumentation edges around construction (the same non-actionable pattern
documented throughout this audit). Each test is tagged with
`RecordProperty("requirement", "<REQ-IDs>")`, so running the test binary
with `--gtest_output=xml:<path>` produces a JUnit-style XML report whose
`<testcase>` elements carry that mapping as a machine-checkable artifact.

| Name | Description | Verifies |
|---|---|---|
| `RequestPowerModeDeferredCompletion` | Confirms ON first (hub-link-trust precondition), sends `REQUEST_POWER_MODE`, confirms it stays open (no immediate response), confirms a mismatched `currentPwrMode` report does not complete it, then confirms a matching report completes it with `OK`. | JM-004 |
| `RequestPowerModeTimeout` | Same ON preamble; sends `REQUEST_POWER_MODE` and never reports a matching mode; confirms it stays open through 119 ticks and completes with `EXECUTION_ERROR` on the 120th (`CMD_TIMEOUT_TICKS`). | JM-004 |
| `RequestPowerModeBusyWhilePending` | Confirms ON, sends `REQUEST_POWER_MODE` twice in a row; confirms the second is rejected `BUSY` without touching the first's tracked opcode/seq, then confirms the first still completes correctly with its original mode. | JM-012 |
| `RequestPowerModeRejectedWhenUnconfirmed` | Fresh component (never confirmed); sends `REQUEST_POWER_MODE`; confirms `VALIDATION_ERROR` + `POWER_MODE_REQUEST_REJECTED`, no `reqPwrMode` call. | JM-013 |
| `RequestPowerModeRejectedWhileAwaitingBootConfirmation` | Commands ON (unconfirmed), sends `REQUEST_POWER_MODE`; confirms the same rejection -- proves `m_hasPendingCmd`/`m_awaitingBootConfirmation` stay mutually exclusive (resolved design fork: reject, not defer). | JM-013 |
| `RequestJetsonPowerStateOnDrivesGpioImmediately` | Sends `REQUEST_JETSON_POWER_STATE(ON)` with authorization granted; confirms GPIO high, telemetry, an immediate `OK`, and -- JM-010 -- that the report to FPManager is NOT sent optimistically (`fpJetsonPowerStateOut` size 0). | JM-001, JM-010 |
| `RequestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut` | Regression test for the ComStub crash this fixed: on a fresh component (state unconfirmed, cached default is OFF), sends `REQUEST_JETSON_POWER_STATE(OFF)` and confirms it takes the direct GPIO-cut path immediately -- NOT the hub-routed `reqJetsonPwrState` call -- since the hub link's liveness is unproven when the Jetson has never reported in. | JM-006 |
| `RequestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown` | Commands ON, then immediately commands OFF before any real report arrives; confirms OFF is deferred (`JETSON_OFF_DEFERRED_BOOTING`, no hub call, no GPIO cut, no command response yet), confirms the real ON report auto-fires the graceful hub request, then drives the ack and `JETSON_POWER_OFF_DELAY_TICKS` grace period through to completion (`OK`). | JM-009 |
| `RequestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout` | Commands ON, then OFF (deferred) with the Jetson never reporting in; confirms the deferral holds through 119 ticks and force-completes via a direct GPIO cut on the 120th (`JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT`, `OK`), not `JETSON_BOOT_CONFIRMATION_TIMEOUT`. | JM-009 |
| `RequestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff` | Commands ON and never reports in, with no OFF ever deferred; confirms the boot-confirmation guard holds through 119 ticks, clears on the 120th (`JETSON_BOOT_CONFIRMATION_TIMEOUT`, not the forced-timeout event), and a subsequent fresh OFF command takes the normal (unconfirmed) direct GPIO-cut path. | JM-009 |
| `RequestJetsonPowerStateOffAcceptedAfterRedundantOnCommand` | Regression test: commands ON, confirms via a real report, commands ON again (redundant -- Jetson already confirmed up), then commands OFF; confirms OFF is accepted immediately and takes the graceful path, not deferred (`JETSON_OFF_DEFERRED_BOOTING` size 0). | JM-009 |
| `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation` | Confirms ON, sends `REQUEST_POWER_MODE` (leaves `m_hasPendingCmd` set), then commands OFF; confirms OFF is deferred (`JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING`, no hub call, no GPIO cut), confirms a matching `currentPwrMode` report completes the mode command AND auto-fires the deferred OFF via the graceful path, then drives it through to completion. | JM-011 |
| `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout` | Same setup, but the mode change never confirms; confirms the mode command times out `EXECUTION_ERROR` on the 120th tick and the deferred OFF resumes (`JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT`) via the graceful path (state still cached ON). | JM-011 |
| `RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower` | Confirms ON via a real status report, requests OFF, confirms the graceful request and no immediate GPIO cut, confirms a matching OFF report starts the delay window, confirms GPIO stays high through `JETSON_POWER_OFF_DELAY_TICKS - 1` ticks, then cuts on the final tick with `OK`. | JM-005, JM-006 |
| `RequestJetsonPowerStateOffConfirmedOffIsIdempotent` | Confirms OFF via a real status report, then requests OFF again; confirms it completes synchronously via direct GPIO cut without ever attempting the graceful path. | JM-002, JM-006 |
| `RequestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut` | Confirms ON, requests OFF, and never acknowledges it; confirms `JETSON_POWER_STATE_TIMEOUT` fires and GPIO is cut directly after `CMD_TIMEOUT_TICKS`. | JM-003 |
| `RequestJetsonPowerStateRejectedByAuthorization` | Mocks `fpJetsonPowerAuthorize` to return `FAILURE`; confirms `VALIDATION_ERROR` and no GPIO action. | JM-001 |
| `RequestJetsonPowerStateBusyWhilePending` | Sends a second `REQUEST_JETSON_POWER_STATE` while the first is still outstanding; confirms `BUSY` and that the first request's state is untouched. | JM-007 |
| `FpJetsonPowerRequestInIgnoresOnAndActsOnOff` | Confirms an ON request on the internal FPManager path is a no-op, then confirms an OFF request (Jetson known ON) takes the graceful path, mirroring the GDS-facing command. | JM-003 |
| `FpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires` | Confirms FPManager's internal OFF path defers exactly like the GDS-facing command while the Jetson is booting, auto-fires the graceful request on boot confirmation, and never sends a command response throughout (this port has none). | JM-003, JM-009 |
| `CurrentJetsonPwrStateIgnoredWithoutPendingCommand` | Sends an unsolicited `currentJetsonPwrState` report with no pending power command; confirms telemetry/cache still update but no GPIO or command-response action is taken. | JM-008 |
| `LocalModeChangeStartedArmsHubTrustGuardThenClearsOnNextReport` | Confirms ON+hub-connected, sends `localModeChangeStarted`; confirms no command response (nothing to respond to), the event fires, a subsequent `REQUEST_POWER_MODE` is rejected `BUSY`, `fpJetsonHubTrustedOut` reports `false` on the next tick, and a `currentPwrMode` report clears the guard and restores `fpJetsonHubTrustedOut(true)`. | JM-014, JM-015 |
| `LocalModeChangeStartedDoesNotClobberPendingRequestPowerMode` | Regression test: sends a real `REQUEST_POWER_MODE` first, then a racing `localModeChangeStarted` -- confirms it's a no-op (guard already armed) and the original command's opcode/cmdSeq/exact-mode-match bookkeeping survives untouched. | JM-014 |
| `LocalModeChangeStartedDefersJetsonOffLikeHubDrivenModeChangePending` | Confirms `localModeChangeStarted` defers a commanded OFF exactly like a hub-driven mode change in flight would (mirrors the JM-011 test), auto-firing the graceful OFF once a matching mode report arrives. | JM-014 |
| `SchedInRepublishesHubTrustStatusEveryTick` | Confirms ON+hub-connected, then confirms `fpJetsonHubTrustedOut(true)` republishes on three consecutive `schedIn` ticks with no intervening state change. | JM-015 |
| `HubLinkDownPreventsTrustEvenWhenModeReportRacesAheadOfReconnect` | Regression test for the hardware-observed race: after `localModeChangeStarted` arms the guard, the real hub link is reported down (`hubComStatusIn(FAILURE)`); a premature matching `currentPwrMode` report then clears `m_hasPendingCmd`, but `fpJetsonHubTrustedOut` must stay `false` since the transport link is still known down -- only reports `true` again once `hubComStatusIn(SUCCESS)` arrives. | JM-016 |
| `HubComStatusInLogsTransitionsOnlyOnce` | Confirms `JETSON_HUB_LINK_DOWN`/`JETSON_HUB_LINK_RECONNECTED` fire only on an actual transition, not on every repeated `comStatus` report of the same value. | JM-016 |

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
| JM-009 | A commanded/internal OFF request that arrives while a commanded ON is still awaiting the Jetson's first real report (the boot-confirmation window) shall be *deferred*, not rejected -- accepted and held open, then fired automatically (taking the normal graceful-vs-direct-cut path) the instant a real report arrives or the window times out. The window shall be bounded by `CMD_TIMEOUT_TICKS`, after which a still-pending deferred OFF is force-completed via a direct GPIO cut so it can never wait indefinitely; a redundant ON commanded while the Jetson is already confirmed on shall not re-arm the window. | `RequestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown`, `RequestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout`, `RequestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff`, `RequestJetsonPowerStateOffAcceptedAfterRedundantOnCommand`, `FpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires` |
| JM-010 | JetsonManager shall never report the Jetson's power state to FPManager (`fpJetsonPowerStateOut`) optimistically -- only a real report received from the Jetson (`currentJetsonPwrState_handler`) may do so. Commanding ON must not report ON the instant GPIO is driven high, since that would let FPManager's own Jetson-on gating (`remoteJetsonCmdIn_handler`, `jetsonPowerAuthorizeIn_handler`) be defeated during exactly the boot window it exists to protect. | `RequestJetsonPowerStateOnDrivesGpioImmediately` |
| JM-011 | Hub-routed output ports (`reqPwrMode_out`, `reqJetsonPwrState_out`) shall never be called unless the hub link is currently trusted (`isJetsonHubLinkTrusted()`: Jetson confirmed ON, not awaiting first boot confirmation, not mid a `REQUEST_POWER_MODE`-triggered reboot). A commanded/internal OFF that arrives while a mode-change reboot is in flight shall be deferred (reusing the JM-009 mechanism) rather than risking the graceful hub call against a link that may be down for the same reboot-related reason, and shall be resumed once the mode change confirms or times out. | `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation`, `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout` |
| JM-012 | A `REQUEST_POWER_MODE` received while a previous one is still outstanding shall be rejected `BUSY` (mirrors JM-007), rather than silently clobbering the first request's tracked opcode/sequence number. | `RequestPowerModeBusyWhilePending` |
| JM-013 | A `REQUEST_POWER_MODE` received while the hub link is not currently trusted shall be rejected `VALIDATION_ERROR` rather than risking `reqPwrMode_out()` against an unproven link. Unlike Jetson OFF, this is a hard rejection, not a defer -- there is no hardware-safe fallback action for "set power mode," and deferring would break the JM-009/JM-011 invariant that `m_hasPendingCmd` and `m_awaitingBootConfirmation` never overlap. | `RequestPowerModeRejectedWhenUnconfirmed`, `RequestPowerModeRejectedWhileAwaitingBootConfirmation` |
| JM-014 | A LOCAL `SET_POWER_MODE` command issued directly against `JetsonPowerModeManager` (bypassing JetsonManager/FPManager) shall notify JetsonManager, over the hub, before running `nvpmodel`, so the same hub-link-distrust guard used for a hub-driven `REQUEST_POWER_MODE` is armed for this path too. This notification shall never clobber a real, already-in-flight `REQUEST_POWER_MODE`'s response bookkeeping. | `LocalModeChangeStartedArmsHubTrustGuardThenClearsOnNextReport`, `LocalModeChangeStartedDoesNotClobberPendingRequestPowerMode`, `LocalModeChangeStartedDefersJetsonOffLikeHubDrivenModeChangePending` |
| JM-015 | JetsonManager shall republish its hub-link-trust verdict (`isJetsonHubLinkTrusted()`) to FPManager every `schedIn` tick, so FPManager can gate its own remote-command routing path (which never passes through JetsonManager) against the same reboot risk. | `SchedInRepublishesHubTrustStatusEveryTick` |
| JM-016 | `isJetsonHubLinkTrusted()` shall additionally require that the real transport-level hub TCP link is currently connected (`hubComStatusIn`/`m_hubLinkConnected`), not merely that application-level self-reports (mode/power-state) appear to confirm it -- since an `nvpmodel`-triggered reboot can leave the Jetson's own process reporting a matching mode well before the actual reboot has severed the link. | `HubLinkDownPreventsTrustEvenWhenModeReportRacesAheadOfReconnect`, `HubComStatusInLogsTransitionsOnlyOnce` |
| JM-017 | `beginJetsonOffSequence()`'s graceful-vs-direct-cut decision shall use `isJetsonHubLinkTrusted()` directly rather than hand-deriving an equivalent subset of its conditions, so the two can never drift out of sync. | Covered by every existing test exercising the graceful OFF path (e.g. `RequestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower`), all of which now require `confirmJetsonOnAndHubConnected()` |

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
| 2026-07-30 | **Bug fix (JM-009 follow-up)**: the guard added just above was still unconditionally re-armed on *every* ON command, including a redundant ON sent to a Jetson that was already confirmed up (a real report had already arrived). Observed on hardware: ON, wait for the real boot confirmation, ON again (harmless double-command), then OFF -- rejected `BUSY` again, because the second ON re-armed `m_awaitingBootConfirmation` with nothing left to clear it (the Jetson only sends its one-shot boot report once, not again just because it was told to turn on a second time), so the guard could only clear via the full `CMD_TIMEOUT_TICKS` fallback. The ON branch now only arms the guard on a genuine off->on transition (`!(m_jetsonPowerStateKnown && m_currentJetsonPowerState.e == ON)`); a redundant ON while already confirmed on explicitly clears it instead. Added `RequestJetsonPowerStateOffAcceptedAfterRedundantOnCommand`. Coverage: 94.1% line / 100% function / 56.6% branch. | Luca Lanzillotta |
| 2026-07-30 | **Behavior change (JM-009 redesign) + bug fix (JM-010)**: consolidated Jetson-on gating so FPManager is the single authority for "is it safe to talk to the Jetson," sourced from JetsonPowerModeManager's real reports -- and changed a commanded/internal OFF that arrives while the Jetson is still booting from an outright *reject* (`BUSY` + `JETSON_OFF_REJECTED_BOOTING`) to a *defer*: accepted, held open, and automatically fired the instant a real boot report arrives or the boot window times out. Both call sites (`REQUEST_JETSON_POWER_STATE_cmdHandler`'s OFF branch and `fpJetsonPowerRequestIn_handler`, used by FPManager's `DISABLE_HPC_MODE`) now share a single `beginJetsonOffSequence()` helper for the confirmedOn-graceful-vs-direct-cut decision, avoiding two copies of that logic. Added `m_deferredOffPending`; the boot-confirmation timeout in `schedIn_handler` now force-completes a still-pending deferred OFF via a direct GPIO cut (`JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT`) instead of merely clearing the guard, and the pre-existing power-state timeout no longer double-ticks during the defer-wait phase. Removed `JETSON_OFF_REJECTED_BOOTING`; added `JETSON_OFF_DEFERRED_BOOTING` and `JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT`. Also fixed **JM-010**: the ON branch was still unconditionally forwarding an optimistic `ON` report to FPManager (`fpJetsonPowerStateOut_out`) the instant GPIO was driven high, before any real confirmation -- this defeated FPManager's own `remoteJetsonCmdIn_handler`/`jetsonPowerAuthorizeIn_handler` gating during the exact boot window it exists to protect, and undermined the new FPManager-side boot-outstanding tracking this redesign depends on. That optimistic forward is removed; only a real report ever updates FPManager's view now. Rewrote/renamed the JM-009 test trio to match (`RequestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown`, `RequestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout`, `RequestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff`), added `FpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires`, and updated `RequestJetsonPowerStateOnDrivesGpioImmediately` for JM-010. Coordinated with a matching FPManager change (`m_jetsonBootOutstanding`, `DISABLE_HPC_MODE`'s three-branch response wording, see FPManager's own change log). Coverage: 98.5% line / 100% function / 60.4% branch. | Luca Lanzillotta |
| 2026-07-30 | **Bug fix (JM-012) + hub-link-trust unification (JM-011/JM-013)**: tested on hardware -- `REQUEST_POWER_MODE`'s `nvpmodel` exit-status misclassification bug (fixed on the `JetsonPowerModeManager` side this same day, see JPSM-009 there) surfaced two latent gaps on this side while investigating it. First, `REQUEST_POWER_MODE_cmdHandler` had no `m_hasPendingCmd` BUSY guard at all -- a second request while one was outstanding silently clobbered the first's tracked opcode/seq and re-fired `reqPwrMode_out()`, orphaning the first command's GDS caller. Added the missing guard (JM-012), mirroring `REQUEST_JETSON_POWER_STATE_cmdHandler`'s existing `m_hasPendingPowerCmd` pattern. Second, and more seriously: `reqPwrMode_out()` turned out to have the exact same unguarded `Svc::ComStub`-crash exposure JM-006 fixed for `reqJetsonPwrState_out()` -- wired identically through GenericHub with no queue/gate, but called completely unconditionally with zero confirmed-link check. Added a unified `isJetsonHubLinkTrusted()` helper (Jetson confirmed on, not awaiting boot confirmation, not mid a mode-change reboot) as the single precondition for both hub-routed sends; `REQUEST_POWER_MODE` now fails fast `VALIDATION_ERROR` (`POWER_MODE_REQUEST_REJECTED`) rather than defer, since unlike Jetson OFF there's no hardware-safe fallback for "set power mode" -- deferring would also break the now-enforced invariant that `m_hasPendingCmd` and `m_awaitingBootConfirmation` never overlap (JM-013; resolved design fork -- reject, not defer, chosen specifically to keep that invariant simple rather than introduce a second deferred-intent type with an unresolved OFF-vs-mode priority question). This investigation also surfaced a previously-unrecognized gap in the *existing* JM-006/009 gate: a `REQUEST_POWER_MODE`-triggered reboot leaves `m_currentJetsonPowerState` `ON` throughout (the Jetson keeps GPIO power, only its hub link goes down while it reboots), so `beginJetsonOffSequence()`'s old `confirmedOn`-only check would have wrongly trusted the hub link during that window too. It now also defers on `m_hasPendingCmd` (new event `JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING`), resumed by `currentPwrMode_handler` on a matching mode confirmation or by `schedIn_handler`'s existing mode-timeout block (`JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT`) -- mirroring the boot-confirmation defer/auto-fire/force-resume pattern exactly (JM-011). Since `beginJetsonOffSequence()` now owns both defer decisions itself, its two callers no longer special-case `m_awaitingBootConfirmation` -- they call it unconditionally. Added `RequestPowerModeBusyWhilePending`, `RequestPowerModeRejectedWhenUnconfirmed`, `RequestPowerModeRejectedWhileAwaitingBootConfirmation`, `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation`, `RequestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout`; updated `RequestPowerModeDeferredCompletion`/`RequestPowerModeTimeout` to confirm ON first (previously ran against a fresh, unconfirmed component, which would now be rejected). Updated `ImxDeployment/Top/topology.fpp`'s comments on both `reqPwrMode`/`reqJetsonPwrState` connections to document the unified gate. Coverage: 98.6% line / 100% function / 61.2% branch. | Luca Lanzillotta |
| 2026-07-30 | **Cross-deployment gap closed (JM-014/JM-015)**: the JM-011/012/013 hub-link-trust gate only covered `REQUEST_POWER_MODE` issued through JetsonManager itself. A `SET_POWER_MODE` sent directly to `JetsonPowerModeManager` (bypassing JetsonManager/FPManager via the generic remote-command routing path) could trigger the exact same `nvpmodel` reboot with zero visibility on this side. Added `localModeChangeStarted` (hub-routed input, from a new `JetsonPowerModeManager` output of the same name, fired right before it runs `nvpmodel` locally) to arm `m_hasPendingCmd` for this path too -- guarded against clobbering a real in-flight `REQUEST_POWER_MODE`'s bookkeeping by no-op'ing if the guard is already armed. Also added `fpJetsonHubTrustedOut`, republishing `isJetsonHubLinkTrusted()` to FPManager every `schedIn` tick, so FPManager's own remote-command gate (`remoteJetsonCmdIn_handler`, which never passes through JetsonManager) can reject Jetson-bound GDS/CmdSequencer traffic during either kind of reboot. New hub channel: index 6 (confirmed free in both topology files). Added `LocalModeChangeStartedArmsHubTrustGuardThenClearsOnNextReport`, `LocalModeChangeStartedDoesNotClobberPendingRequestPowerMode`, `LocalModeChangeStartedDefersJetsonOffLikeHubDrivenModeChangePending`, `SchedInRepublishesHubTrustStatusEveryTick`. Bumped `JetsonManagerTester::MAX_HISTORY_SIZE` from 20 to 150 -- `fpJetsonHubTrustedOut` now fires unconditionally on every `schedIn` tick, and several existing tests loop `invoke_to_schedIn` up to 120 times without an intervening `clearHistory()`; F´'s generated `History::push_back()` hard-aborts (not a graceful test failure) on overflow. Coverage: 98.6% line / 100% function / 61.2% branch. | Luca Lanzillotta |
| 2026-07-31 | **Bug fix (JM-016/JM-017), found on hardware after JM-014/015 shipped**: a real hardware test still crashed `imx_hubComStub` (`ComStub.cpp:28` `FW_ASSERT`) despite the JM-014/015 fix above. Root cause: `nvpmodel -m X` applies the mode and returns success *synchronously*, while the actual Linux `reboot()` it triggers runs *asynchronously*, seconds later -- `JetsonPowerModeManager`'s still-running process reports the new mode over the hub (its normal per-boot "report once" `schedIn` logic) well before the real reboot severs the TCP link. Since `currentPwrMode_handler` clears the externally-triggered `m_hasPendingCmd` guard on *any* report (there's no specific mode to match for that case), `isJetsonHubLinkTrusted()` flipped back to `true` -- and `fpJetsonHubTrustedOut`/`FPManager.m_jetsonHubTrusted` followed -- well before the link had actually gone down, reopening the exact crash window JM-011/014 were meant to close. No application-level self-report can ever prove a reboot has physically happened; only the transport layer can. Added `hubComStatusIn`, fed directly from `imx_hubComStub.comStatusOut` (a standard `Svc::ComStub` port already emitting `SUCCESS`/`FAILURE` on real reconnect/first-failed-send) -- previously wired to `imx_hubFramer.comStatusIn`, confirmed by inspection to be a dead end (`Svc::FprimeFramer` just re-emits it on its own `comStatusOut`, which nothing consumed), so redirecting it cost nothing. `isJetsonHubLinkTrusted()` now additionally requires `m_hubLinkConnected` (JM-016). Also fixed `beginJetsonOffSequence()`, which hand-derived its own `confirmedOn` subset of the same conditions instead of calling `isJetsonHubLinkTrusted()` -- an identical, previously-unnoticed exposure on the OFF-request path; it now calls `isJetsonHubLinkTrusted()` directly so the two can't drift apart again (JM-017). Added two events, `JETSON_HUB_LINK_DOWN`/`JETSON_HUB_LINK_RECONNECTED`, logged only on actual transitions. `FPManager`/`JetsonPowerModeManager` needed no code changes -- FPManager already trusts `fpJetsonHubTrustedOut`, now backed by a more accurate signal (both suites re-run clean, 43 and 28 tests respectively). Added `confirmJetsonOnAndHubConnected()` test helper and updated 16 existing test call sites that previously only confirmed the Jetson ON (now `m_hubLinkConnected` also defaults false and must be established); added `HubLinkDownPreventsTrustEvenWhenModeReportRacesAheadOfReconnect` (the regression test for this exact race) and `HubComStatusInLogsTransitionsOnlyOnce`. Cross-verified with an architecture diagram of the full hub/Framer/ComStub/comStatus chain before implementation. Coverage: 98.3% line / 100% function / 62.5% branch. `imx8x` cross-build verified clean. | Luca Lanzillotta |
| 2026-07-31 | **Structural backstop added (no `JetsonManager` code changes)**: verified against the F´ reference docs that the framework's own "Framer Status Protocol" (a `Svc::ComQueue` gating sends on `comStatus`, as already used for the UART GDS and direct-TCP-GDS downlinks in this deployment) was never actually wired in for the imx<->Jetson hub link -- `GenericHub`/`HubComAdapter` have no `comStatus` awareness at all, so JM-016 above was the *only* thing preventing the `imx_hubComStub` crash, and depended entirely on every current and future caller remembering to check `isJetsonHubLinkTrusted()`/`m_jetsonHubTrusted`. Inserted `imx_hubComQueue` (`Svc::ComQueue`) into `ImxDeployment/Top/topology.fpp`'s `send_hub` connections, between `imx_hub` and `imx_hubFramer`, gated by `imx_hubComStub`'s real `comStatus` -- this makes the crash structurally unreachable from this link regardless of application-level state, closing the gap for any future hub-bound port. `comStatus` needed a second consumer (the queue, in addition to this component's own `hubComStatusIn`), and F´ ports are strictly 1:1, so a small fan-out (`comStatusIn`/`comStatusOut[2]`) was added to `HubComAdapter` -- the existing, minimal, stateless component already sitting in this exact part of the pipeline -- rather than a new single-purpose component (see `HubComAdapter`'s own SDD, HSR-001, for that change and why). JM-016/JM-017 are unchanged and stay as the fast, informative layer (immediate `BUSY`/`VALIDATION_ERROR` instead of a command silently queuing or being dropped); the queue is purely a backstop underneath them. No file-size or throughput impact: file transfer chunk size is fixed far upstream (`FW_COM_BUFFER_MAX_SIZE`, unrelated to this queue), and the queue only adds latency during an actual outage, by design (`QUEUE_DROP_NEWEST`, depth `HUB_QUEUE_DEPTH=10`, tunable). `FPManager`/`JetsonPowerModeManager` unaffected (regression-verified, 43 and 28 tests respectively, both unchanged). `imx8x` cross-build verified clean. | Luca Lanzillotta |
