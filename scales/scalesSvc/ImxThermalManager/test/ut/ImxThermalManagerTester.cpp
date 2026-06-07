// ======================================================================
// \title  ImxThermalManagerTester.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component test harness implementation class
// ======================================================================

#include "ImxThermalManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManagerTester ::
    ImxThermalManagerTester() :
      ImxThermalManagerGTestBase("ImxThermalManagerTester", ImxThermalManagerTester::MAX_HISTORY_SIZE),
      component("ImxThermalManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  ImxThermalManagerTester ::
    ~ImxThermalManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void ImxThermalManagerTester ::
    ImxThermalManagerTesting()
  {
    //load object parameters
    this->component.loadParameters();
    this->clearHistory();

    //Cycle 1: Start one loop to see data output is true
    const Fw::Time expectedTime_cycle1(0, 0); //set time
    this->setTestTime(expectedTime_cycle1);
    this->invoke_to_imxCpuTemp(0,0); //invoke the port run
    this->component.doDispatch();  // Trigger execution of async port
    ASSERT_TLM_imx_cpu_temp_read_SIZE(1); //check telemetry was emitted when imxCpuTemp was handled
    
    const ThermalReading& actual_cycle1 = this->tlmHistory_imx_cpu_temp_read->at(0).arg;
    ASSERT_EQ(actual_cycle1.getsensorId(), 0);
    ASSERT_STREQ(actual_cycle1.getlocation().toChar(), "CPU");
    ASSERT_EQ(actual_cycle1.gettimestamp(), expectedTime_cycle1.getSeconds()); // if available

    //Cycle 2: Check 2nd loop
    const Fw::Time expectedTime_cycle2(1, 0); //increment time for next loop
    this->setTestTime(expectedTime_cycle2);
    this->invoke_to_imxCpuTemp(0,0); //invoke the port run
    this->component.doDispatch();  // Trigger execution of async port
    ASSERT_TLM_imx_cpu_temp_read_SIZE(2); //check telemetry was emitted when imxCpuTemp was handled
    
    const ThermalReading& actual_cycle2 = this->tlmHistory_imx_cpu_temp_read->at(1).arg;
    ASSERT_EQ(actual_cycle2.getsensorId(), 0);
    ASSERT_STREQ(actual_cycle2.getlocation().toChar(), "CPU");
    ASSERT_EQ(actual_cycle2.gettimestamp(), expectedTime_cycle2.getSeconds()); // if available
  }
}
