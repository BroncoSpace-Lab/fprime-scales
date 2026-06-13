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

    // --- 1st cycle when device just booted --- 
    this->invoke_to_run(0, 0); // Trigger the component's run port
    this->component.doDispatch(); // Execute run_handler, write telemetry, and queues the tick 
    this->component.doDispatch(); // Tick is processed, since we're in INIT, doRead gets called and the device just booted so parameters are set up

    printf("The parameters are set up with the following values: \n");
    printf("IDLE_LOW_THR: %f\n", this->component.IDLE_LOW_THR);
    printf("IDLE_HIGH_THR: %f\n", this->component.IDLE_HIGH_THR);
    printf("WARN_LOW_THR: %f\n", this->component.WARN_LOW_THR);
    printf("WARN_HIGH_THR: %f\n", this-> component.WARN_HIGH_THR);
    printf("FAULT_LOW_THR: %f\n", this->component.FAULT_LOW_THR);
    printf("FAULT_HIGH_THR: %f\n", this->component.FAULT_HIGH_THR);

    // --- 2nd cycle, device will started reading sensor values ---
    this->invoke_to_run(0, 0); // Trigger the component's run port
    this->component.doDispatch(); // Execute run_handler, write telemetry, and queues the tick 
    this->component.doDispatch(); // Tick is processed, since we're in INIT, doRead gets called and sensor reading is attempted. 
    // this->component.doDispatch(); // Finish processing the read 
    // check if the read is successful
   // ASSERT_FROM_PORT_HISTORY_SIZE(1);
  }
}