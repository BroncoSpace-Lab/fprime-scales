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
    // Set the temperature readings to be within each sensor's own IDLE threshold values
    for(int i = 0; i < 3; i++){
      this->component.m_thermalReadings[i].set_temperature(
          (this->component.IDLE_LOW_THR[i] + this->component.IDLE_HIGH_THR[i]) / 2);
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

  void McpManagerTester::thresholdsMisconfiguredEmitsOnceOnTransition() {
    // Directly craft an inverted configuration for sensor 0 (IMX): WARN_HIGH
    // set below IDLE_HIGH, mirroring the real-world mistake of lowering
    // WARN_HIGH without adjusting IDLE_HIGH to match.
    this->component.FAULT_LOW_THR[0] = -40.0F;
    this->component.WARN_LOW_THR[0] = -20.0F;
    this->component.IDLE_LOW_THR[0] = 10.0F;
    this->component.IDLE_HIGH_THR[0] = 60.0F;
    this->component.WARN_HIGH_THR[0] = 30.0F;
    this->component.FAULT_HIGH_THR[0] = 100.0F;

    this->component.validateThresholds("IMX", 0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED(0, "IMX", -40.0F, -20.0F, 10.0F, 60.0F, 30.0F, 100.0F);

    // Re-validating the same bad configuration must not re-emit.
    this->component.validateThresholds("IMX", 0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);

    // Fixing the configuration clears the latch silently (no new event).
    this->component.WARN_HIGH_THR[0] = 80.0F;
    this->component.validateThresholds("IMX", 0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);

    // Breaking it again now DOES re-emit, since the latch was cleared.
    this->component.WARN_HIGH_THR[0] = 30.0F;
    this->component.validateThresholds("IMX", 0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
  }
}