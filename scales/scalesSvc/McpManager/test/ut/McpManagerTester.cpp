// ======================================================================
// \title  McpManagerTester.cpp
// \author bidat
// \brief  cpp file for McpManager component test harness implementation class
// ======================================================================

#include "McpManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  McpManagerTester ::
    McpManagerTester() :
      McpManagerGTestBase("McpManagerTester", McpManagerTester::MAX_HISTORY_SIZE),
      component("McpManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  McpManagerTester :: ~McpManagerTester()
  {
    this->component.deinit();
  }

  void McpManagerTester::drainQueue() {
    // dispatchAvailableMessages() only dispatches the count present at the
    // start of the call (never blocks waiting for more, unlike calling
    // doDispatch() directly past the last real message). Since each signal
    // in this state machine chains at most a couple of messages deep, a
    // handful of calls is always enough to fully settle.
    for (int i = 0; i < 8; i++) {
      this->component.dispatchAvailableMessages();
    }
  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void McpManagerTester :: mcpTest()
  {
    // Load Parameters
    this->component.loadParameters(); //load the component parameters

    // --- when device just booted --- 
    this->invoke_to_run(0, 0); // Trigger the component's run port
    this->component.doDispatch(); // Execute run_handler, write telemetry, and queues the tick 
    this->component.doDispatch(); // Tick is processed, since we're in INIT, doRead gets called and the device just booted so parameters are set up


    // --- device will started reading sensor values ---
    this->invoke_to_run(0, 0); // Trigger the component's run port
    this->component.doDispatch(); // Execute run_handler, write telemetry, and queues the tick 
    this->component.doDispatch(); // Tick is processed, since we're in INIT, doRead gets called and sensor reading is attempted.
    // Note: in unit testing environment, the i2c bus always evalute successfully, and the temperature values read are always 0
    this->component.doDispatch(); // Since the read was successful, success signal is sent, entered EVALUATE state

    
    // --- device will start evaluating sensor values ---

    // --- First case is check to evaluation for IDLE ---
    // Set the temperature readings to be within each sensor's own IDLE bounds
    for(int i = 0; i < 3; i++){
      this->component.m_thermalReadings[i].set_temperature(
          (this->component.m_activeBounds[i].get_idleLow() +
           this->component.m_activeBounds[i].get_idleHigh()) / 2);
    }

    this->invoke_to_run(0, 0); // Trigger the component's run port
    this->component.doDispatch(); // Execute run_handler, write telemetry, and queues the tick
    this->component.doDispatch(); // Since we're in EVALUATE, doEvaluate gets called

    /* Check if telmetry was emmited*/
    ASSERT_TLM_IMX_TEMP_SIZE(1);
    ASSERT_TLM_PERIPHERAL_TEMP_SIZE(1);
    ASSERT_TLM_JETSON_TEMP_SIZE(1);

    /* Check if the states are correct */
    const ThermalReading& imx =
        this->tlmHistory_IMX_TEMP->at(0).arg;
    const ThermalReading& peripheral =
        this->tlmHistory_PERIPHERAL_TEMP->at(0).arg;
    const ThermalReading& jetson =
        this->tlmHistory_JETSON_TEMP->at(0).arg;

    ASSERT_EQ(imx.get_tempState(), scalesSvc::ThermalStates::IDLE);
    ASSERT_EQ(peripheral.get_tempState(), scalesSvc::ThermalStates::IDLE);
    ASSERT_EQ(jetson.get_tempState(), scalesSvc::ThermalStates::IDLE);

    this->component.doDispatch(); // Finish evaluating and transition back to initial state to read temp again on next tick
    // check if the read is successful
    // ASSERT_FROM_PORT_HISTORY_SIZE(1);
  }

  void McpManagerTester::boundsUpdateGating() {
    // A valid, distinct-from-default bounds update for sensor 0 (IMX) is
    // adopted and republished as telemetry.
    const scalesSvc::TempBounds goodBounds(-35.0F, -15.0F, 5.0F, 55.0F, 75.0F, 95.0F);
    this->component.applyBounds("IMX", 0, goodBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);
    ASSERT_TLM_MCP_IMX_BOUNDS_SIZE(1);
    ASSERT_TLM_MCP_IMX_BOUNDS(0, goodBounds);
    ASSERT_EQ(this->component.m_activeBounds[0], goodBounds);

    // Mirror the real-world mistake: lower WARN_HIGH without adjusting
    // IDLE_HIGH to match, inverting the high band. The update must be
    // rejected -- the sensor keeps using goodBounds, and no new telemetry
    // is published.
    const scalesSvc::TempBounds badBounds(-35.0F, -15.0F, 5.0F, 55.0F, 30.0F, 95.0F);
    this->component.applyBounds("IMX", 0, badBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED(0, "IMX", -35.0F, -15.0F, 5.0F, 55.0F, 30.0F, 95.0F);
    ASSERT_TLM_MCP_IMX_BOUNDS_SIZE(1);
    ASSERT_EQ(this->component.m_activeBounds[0], goodBounds);

    // Repeating the exact same bad attempt fires again -- every rejection is
    // its own distinct notice, since misconfiguration never takes effect.
    this->component.applyBounds("IMX", 0, badBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
    ASSERT_TLM_MCP_IMX_BOUNDS_SIZE(1);
    ASSERT_EQ(this->component.m_activeBounds[0], goodBounds);

    // A second valid update is adopted normally.
    const scalesSvc::TempBounds otherGoodBounds(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F);
    this->component.applyBounds("IMX", 0, otherGoodBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
    ASSERT_TLM_MCP_IMX_BOUNDS_SIZE(2);
    ASSERT_TLM_MCP_IMX_BOUNDS(1, otherGoodBounds);
    ASSERT_EQ(this->component.m_activeBounds[0], otherGoodBounds);
  }

  Drv::I2cStatus McpManagerTester::from_mcpWriteRead_handler(
      FwIndexType portNum,
      U32 addr,
      Fw::Buffer& writeBuffer,
      Fw::Buffer& readBuffer
  ) {
    this->pushFromPortEntry_mcpWriteRead(addr, writeBuffer, readBuffer);
    return this->m_forceI2cFailure ? Drv::I2cStatus::I2C_READ_ERR : Drv::I2cStatus::I2C_OK;
  }

  void McpManagerTester::thermalStateEvaluation() {
    this->component.loadParameters();
    // Boot: doRead() just sets up parameters and sends no signal, so the
    // queue is naturally empty after (run_handler dispatch + tick dispatch).
    this->invoke_to_run(0, 0);
    this->drainQueue();

    auto runStage = [this]() {
      this->invoke_to_run(0, 0);
      this->drainQueue();
    };

    runStage(); // read
    this->clearHistory();

    // Drive all three sensors into WARN (just below IDLE_LOW).
    for (int i = 0; i < 3; i++) {
      this->component.m_thermalReadings[i].set_temperature(
          this->component.m_activeBounds[i].get_idleLow() - 1.0F);
    }
    runStage(); // evaluate

    ASSERT_TLM_IMX_TEMP_SIZE(1);
    ASSERT_EQ(this->tlmHistory_IMX_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::WARN);
    ASSERT_EQ(this->tlmHistory_PERIPHERAL_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::WARN);
    ASSERT_EQ(this->tlmHistory_JETSON_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::WARN);
    this->clearHistory();

    runStage(); // read
    this->clearHistory();

    // Drive all three sensors into FAULT (below FAULT_LOW).
    for (int i = 0; i < 3; i++) {
      this->component.m_thermalReadings[i].set_temperature(
          this->component.m_activeBounds[i].get_faultLow() - 1.0F);
    }
    runStage(); // evaluate

    ASSERT_TLM_IMX_TEMP_SIZE(1);
    ASSERT_EQ(this->tlmHistory_IMX_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
    ASSERT_EQ(this->tlmHistory_PERIPHERAL_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
    ASSERT_EQ(this->tlmHistory_JETSON_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
    this->clearHistory();

    runStage(); // read
    this->clearHistory();

    // Drive all three sensors into the high-side FAULT gap: strictly between
    // WARN_HIGH and FAULT_HIGH, a distinct disjunct from the low-side FAULT
    // check above (determineTempState() has separate low/high FAULT terms).
    for (int i = 0; i < 3; i++) {
      const scalesSvc::TempBounds& b = this->component.m_activeBounds[i];
      this->component.m_thermalReadings[i].set_temperature(
          (b.get_warnHigh() + b.get_faultHigh()) / 2.0F);
    }
    runStage(); // evaluate

    ASSERT_TLM_IMX_TEMP_SIZE(1);
    ASSERT_EQ(this->tlmHistory_IMX_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
    ASSERT_EQ(this->tlmHistory_PERIPHERAL_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
    ASSERT_EQ(this->tlmHistory_JETSON_TEMP->at(0).arg.get_tempState(), scalesSvc::ThermalStates::FAULT);
  }

  void McpManagerTester::readFailureHandling() {
    this->component.loadParameters();
    this->invoke_to_run(0, 0); // boot
    this->drainQueue();
    this->clearHistory();

    // Force every mcpWriteRead call to fail for this read cycle.
    this->m_forceI2cFailure = true;
    this->invoke_to_run(0, 0); // read: doRead fires synchronously within this drain
    this->drainQueue();

    // Each failed sensor gets a FAIL_TO_READ_TEMP_AT event and is marked FAULT.
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(3);
    ASSERT_EQ(this->component.m_thermalReadings[0].get_tempState(), scalesSvc::ThermalStates::FAULT);
    ASSERT_EQ(this->component.m_successfulRead, false);

    // Evaluate: sees m_successfulRead == false, sends the "fail" signal instead
    // of "success", moving toward the read-failure state (doReadFail not yet run).
    this->invoke_to_run(0, 0);
    this->drainQueue();

    // doReadFail: overall failure emits FAIL_TO_READ_TEMP and resets flags to retry.
    this->invoke_to_run(0, 0);
    this->drainQueue();
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_SIZE(1);
    ASSERT_EQ(this->component.m_successfulRead, true);
    ASSERT_EQ(this->component.m_successfulReads[0], true);
    ASSERT_EQ(this->component.m_successfulReads[1], true);
    ASSERT_EQ(this->component.m_successfulReads[2], true);
    this->clearHistory();

    // Next cycle succeeds normally again.
    this->m_forceI2cFailure = false;
    this->invoke_to_run(0, 0);
    this->drainQueue();
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(0);
    ASSERT_EQ(this->component.m_successfulRead, true);
  }

  void McpManagerTester::parameterUpdatedSwitchCoverage() {
    this->component.loadParameters();
    this->clearHistory();

    // PARAMID_MCP_IMX_BOUNDS = 0x0, PARAMID_MCP_PERIPHERAL_BOUNDS = 0x1,
    // PARAMID_MCP_JETSON_BOUNDS = 0x2 (McpManagerComponentAc.hpp). Loaded
    // defaults are already ordered, so these just exercise the switch cases
    // boundsUpdateGating() (which calls applyBounds() directly, bypassing
    // this switch entirely) never reaches.
    this->component.parameterUpdated(0x0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);

    this->component.parameterUpdated(0x1);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);

    this->component.parameterUpdated(0x2);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);

    // Unrecognized parameter ID: default case, no bounds touched, no event.
    this->component.parameterUpdated(0xDEAD);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);

    // Out-of-range sensor index: default case in writeBoundsTelemetry, no crash.
    this->component.writeBoundsTelemetry(99);
  }
}