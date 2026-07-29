# scalesSvc::DataProducer

Producer of F´ Data Products (`Fw::DpContainer`s) from the thermal and power
readings collected elsewhere in `scalesSvc`. Instantiated as `imx_dataProducer`
in `ImxDeployment` (Jetson thermal readings reach it over the hub, forwarded
by `JetsonThermalManager` on the Jetson side).

## Design Summary

Collection is off by default (`m_dpCollectMode = false`) and is gated by two
commands, `ENABLE_DATA_PRODUCTS`/`DISABLE_DATA_PRODUCTS`. `DISABLE_DATA_PRODUCTS`
immediately invalidates all four containers so any reading that arrives after
disabling -- or before the first `ENABLE_DATA_PRODUCTS` -- is silently dropped,
not queued or buffered for later.

There are four independent container "pipelines", one per data source, each
with its own valid flag and record counter:

| Source | Container | Records per container | Reading port |
|---|---|---|---|
| MCP (McpManager) | `McpTemperatureContainer` | 3 (IMX, Peripheral, Jetson) | `McpThermalReadingIn` |
| i.MX CPU (ImxThermalManager) | `CpuTemperatureContainer` | 1 | `cpuThermalReadIn` |
| Jetson die (JetsonThermalManager, via hub) | `JetsonTemperatureZoneContainer` | 9 (CPU/GPU/CV0-2/SOC0-2/TJ) | `jetsonThermalReadIn` |
| INA260 (InaManager) | `InaPowerContainer` | 3 (OBC, Peripheral, Jetson) | `inaPowerReadIn` |

Each pipeline follows the same lifecycle:

1. `run_handler` (rate-group tick) checks the pipeline's valid flag; if
   invalid *and* `m_dpCollectMode` is true, it calls `dpGet_<Container>(size, container)`
   to allocate a fresh container from the `DataProducts` subtopology and
   marks it valid on success. A failed allocation leaves the flag false and
   is retried on the next tick -- there is no backoff or failure event.
2. Every time the corresponding reading port fires *while the container is
   valid*, the handler serializes one record of each type into the container
   and increments that pipeline's record counter. If the container is
   invalid (never initialized, mid-batch-reset, or collection disabled), the
   reading is dropped -- there is no retry or queuing.
3. Once a pipeline's counter reaches `RECORD_COUNT` (50, a compile-time
   constant in `DataProducer.hpp`; not configurable at runtime), the full
   container is handed to `dpSend()`, and that pipeline's valid flag and
   counter both reset to zero -- so the very next `run_handler` tick
   allocates a fresh container and the cycle repeats.

`run_handler` gates *initialization* on `m_dpCollectMode`, but the reading
handlers themselves only check their own container's valid flag, not
`m_dpCollectMode` directly -- this is fine because `DISABLE_DATA_PRODUCTS`
also unconditionally clears every valid flag, and no code path can set a
valid flag back to true while `m_dpCollectMode` is false (since only
`run_handler`'s init calls do that, and those are gated).

## Port Descriptions
| Kind | Name | Description |
|---|---|---|
| sync input | `run` | Rate-group tick. Initializes (or reinitializes) any invalid container, only while data product collection is enabled. |
| async input | `McpThermalReadingIn` | Complete MCP reading set (IMX, Peripheral, Jetson) from `McpManager`, one call per pipeline tick. |
| async input | `cpuThermalReadIn` | i.MX CPU reading from `ImxThermalManager`. |
| async input | `jetsonThermalReadIn` | All nine Jetson thermal zone readings from `JetsonThermalManager`, forwarded over the hub. |
| async input | `inaPowerReadIn` | Complete INA260 reading set (OBC, Peripheral, Jetson) from `InaManager`. |
| product get port | `productGetOut` | Requests a fresh container buffer from the `DataProducts` subtopology (`Fw.Interfaces.DataProductSync`-shaped). |
| product send port | `productSendOut` | Hands a filled container off to `DataProducts.dpWriter` once a batch of `RECORD_COUNT` records is complete. |

## Component States
| Name | Description |
|---|---|
| Collection disabled (`m_dpCollectMode = false`) | Default at boot. No container is ever initialized; every reading is dropped regardless of any container's valid flag. |
| Collection enabled, container invalid | The pipeline's container has not yet been allocated (boot, just-sent, or a prior `dpGet` failure). Readings for that pipeline are dropped until the next successful `run_handler` allocation. |
| Collection enabled, container valid, mid-batch | Readings are being serialized into the container; `dpSend()` has not yet fired for the current batch. |

## Parameters
| Name | Description |
|---|---|
| None | `DataProducer` has no configurable parameters. `RECORD_COUNT` (50) and the four per-source record counts are compile-time constants in `DataProducer.hpp`. |

## Commands
| Name | Description |
|---|---|
| `ENABLE_DATA_PRODUCTS` | Sets `m_dpCollectMode = true`. Does not itself allocate any container -- the next `run` tick does that. |
| `DISABLE_DATA_PRODUCTS` | Sets `m_dpCollectMode = false` and immediately invalidates all four containers, dropping any in-progress batch (its already-serialized records are discarded, not flushed). |

## Events
| Name | Description |
|---|---|
| None | `DataProducer` has no events. Container-allocation and serialization failures are logged only via `printf` (visible in `journalctl`, not in GDS) -- see the Change Log for why this is flagged rather than fixed in this pass. |

## Telemetry
| Name | Description |
|---|---|
| None | `DataProducer` has no telemetry channels. |

## Unit Tests

Measured via `fprime-util check --coverage`: **77.9% line (148/190), 100%
function (17/17), 44.3% branch (85/192)**. Line/branch coverage is lower
than most other components in this audit mainly because the four
serialization-failure branches (`serializeRecord_*` returning non-OK, which
requires an intentionally-undersized mock buffer per container to trigger)
are not exercised -- they are identical in shape across all four pipelines
and print-only (no distinguishing behavior), so this pass covers the
allocation-failure path (`ContainerGetFailureLeavesContainerInvalid`)
instead as the higher-value representative failure case. Each test is tagged with
`RecordProperty("requirement", "<REQ-IDs>")`, so running the test binary with
`--gtest_output=xml:<path>` produces a JUnit-style XML report whose
`<testcase>` elements carry that mapping as a machine-checkable artifact.
The `productGetOut` port is mocked (`productGet_handler` override, following
the pattern in `lib/fprime/FppTestProject/FppTest/dp/test/ut/DpTestTester.cpp`,
the closest real precedent for testing an F´ Data Product producer) so no
test depends on a real `DataProducts` subtopology.

| Name | Description | Verifies |
|---|---|---|
| `DisabledByDefaultBlocksContainerInit` | A `run` tick with no prior `ENABLE_DATA_PRODUCTS`; confirms zero container-get requests. | DP-001 |
| `EnableDataProductsInitializesAllContainers` | `ENABLE_DATA_PRODUCTS` then one `run` tick; confirms all four containers are requested. | DP-001 |
| `DisableDataProductsInvalidatesContainers` | Enables and initializes, then `DISABLE_DATA_PRODUCTS`; confirms a subsequent reading is dropped and a subsequent `run` tick requests nothing. | DP-001, DP-002 |
| `McpReadingIgnoredWhileContainerInvalid` | Sends an MCP reading with collection never enabled; confirms no `dpSend`. | DP-003 |
| `McpReadingBatchesAndSendsAfterRecordCount` | Sends `RECORD_COUNT` MCP readings; confirms `dpSend` fires only after the last one, for the MCP container id. | DP-003 |
| `JetsonZoneReadingBatchesAndSendsAfterRecordCount` | Same for the nine-argument Jetson zone path -- regression test for a fixed typo (`this-dpGet_...`) that previously made this container's init fail to compile for any real build target. | DP-003 |
| `CpuReadingBatchesAndSendsAfterRecordCount` | Same for the single-argument i.MX CPU path. | DP-003 |
| `InaReadingBatchesAndSendsAfterRecordCount` | Same for the three-argument INA power path. | DP-003 |
| `ContainerReinitializesAfterBatchSend` | After the CPU pipeline's batch completes, confirms the next `run` tick re-requests only that pipeline's container (the other three are still mid-batch and stay valid). | DP-004 |
| `ContainerGetFailureLeavesContainerInvalid` | Mocks a `dpGet` failure for one container; confirms a reading during that window is dropped, then confirms a later successful `run` tick allows the pipeline to resume. | DP-005 |

## Requirements
| Name | Description | Verified By |
|---|---|---|
| DP-001 | Data product collection shall be disabled by default and controlled by `ENABLE_DATA_PRODUCTS`/`DISABLE_DATA_PRODUCTS`; no container shall be allocated while disabled. | `DisabledByDefaultBlocksContainerInit`, `EnableDataProductsInitializesAllContainers`, `DisableDataProductsInvalidatesContainers` |
| DP-002 | `DISABLE_DATA_PRODUCTS` shall invalidate all four containers immediately, discarding any in-progress batch rather than flushing it. | `DisableDataProductsInvalidatesContainers` |
| DP-003 | Each of the four sources shall batch its records independently and only call `dpSend()` once exactly `RECORD_COUNT` records have been serialized; a reading that arrives while its container is invalid (collection disabled, not yet allocated, or just sent) shall be dropped, not queued. | `McpReadingIgnoredWhileContainerInvalid`, `McpReadingBatchesAndSendsAfterRecordCount`, `JetsonZoneReadingBatchesAndSendsAfterRecordCount`, `CpuReadingBatchesAndSendsAfterRecordCount`, `InaReadingBatchesAndSendsAfterRecordCount` |
| DP-004 | Once a pipeline's batch is sent, its container shall become invalid so a later `run` tick reallocates it before further readings for that pipeline can be serialized. | `ContainerReinitializesAfterBatchSend` |
| DP-005 | A failed container allocation shall leave that pipeline's container invalid (readings dropped) rather than crashing or retrying immediately; a later successful allocation shall resume normal batching. | `ContainerGetFailureLeavesContainerInvalid` |

## Change Log
| Date | Description |
|---|---|
| 2026-07-28 | First real SDD for this component (previously a bare requirements-template stub) and first unit test suite (previously zero tests, `register_fprime_ut`/CMake UT block commented out). Found and fixed a real compile-breaking typo in `initJetsonTempContainer()`: `this-dpGet_JetsonTemperatureZoneContainer(...)` (missing `>`, parses as pointer-minus-return-value, not a call through `this`) instead of `this->dpGet_JetsonTemperatureZoneContainer(...) == Fw::Success::SUCCESS` -- this would have failed to compile for any real (imx8x/aarch64) target, silently undetected because nothing on the native Linux UT toolchain used before this audit ever compiled this file (`ImxDeployment`/`JetsonDeployment` are excluded on the native platform, and this library had no UT target of its own). Documented the on/off gating design (`m_dpCollectMode`, defaulting to disabled) that a separate ongoing effort added to fix an earlier bug where data products were collected unconditionally from boot. Flagged, but did not change, that allocation/serialization failures are only logged via `printf` (invisible in GDS) -- adding real events is a product decision outside this audit's scope. `run` is a *synchronous* port (unlike the four `async` reading ports), so the test harness never calls `doDispatch()` after `invoke_to_run()` -- doing so once during development hung the test binary indefinitely, since `doDispatch()` blocks rather than returning "empty" when nothing is queued (the same landmine documented in this session's WatchdogManager/McpManager work). Measured 77.9% line, 100% function, 44.3% branch coverage. | Luca Lanzillotta |
