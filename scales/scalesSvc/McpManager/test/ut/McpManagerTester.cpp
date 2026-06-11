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

    this->invoke_to_run(0, 0); // Trigger the component's run port 
  }

}
