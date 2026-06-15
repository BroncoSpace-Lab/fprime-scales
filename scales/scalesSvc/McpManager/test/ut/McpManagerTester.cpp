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
    // Set the temperature readings to be within the IDLE threshold values
    for(int i = 0; i < 3; i++){
      this->component.m_thermalReadings[i].settemperature((this->component.IDLE_LOW_THR + this->component.IDLE_HIGH_THR) / 2);
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

    ASSERT_EQ(imx.gettempState(), scalesSvc::ThermalStates::IDLE);
    ASSERT_EQ(peripheral.gettempState(), scalesSvc::ThermalStates::IDLE);
    ASSERT_EQ(jetson.gettempState(), scalesSvc::ThermalStates::IDLE);

    this->component.doDispatch(); // Finish evaluating and transition back to initial state to read temp again on next tick
    // check if the read is successful
    // ASSERT_FROM_PORT_HISTORY_SIZE(1);
  }
}