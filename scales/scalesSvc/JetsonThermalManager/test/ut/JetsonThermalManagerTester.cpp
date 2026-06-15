// ======================================================================
// \title  JetsonThermalManagerTester.cpp
// \author lucal
// \brief  cpp file for JetsonThermalManager component test harness implementation class
// ======================================================================

#include "JetsonThermalManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  JetsonThermalManagerTester ::
    JetsonThermalManagerTester() :
      JetsonThermalManagerGTestBase("JetsonThermalManagerTester", JetsonThermalManagerTester::MAX_HISTORY_SIZE),
      component("JetsonThermalManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  JetsonThermalManagerTester ::
    ~JetsonThermalManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void JetsonThermalManagerTester ::
    JetsonThermalManagerUnitTester()
  {
    this->paramSet_JETSON_IDLE_LOW(10.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_IDLE_HIGH(60.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_WARN_LOW(-20.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_WARN_HIGH(80.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_FAULT_LOW(-40.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_FAULT_HIGH(100.0f, Fw::ParamValid::VALID);
    this->component.loadParameters();

    this->setTestTime(Fw::Time(0,0));

    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();
    ASSERT_TLM_SIZE(0);

    this->invoke_to_run(0, 1);
    this->component.doDispatch();
    this->component.doDispatch();
    this->component.doDispatch();
    ASSERT_TLM_SIZE(0);

    this->invoke_to_run(0, 2);
    this->component.doDispatch();
    this->component.doDispatch();
    this->component.doDispatch();

    ASSERT_TLM_SIZE(9);
    ASSERT_TLM_jetson_cpu_temp_read_SIZE(1);
    ASSERT_TLM_jetson_gpu_temp_read_SIZE(1);
    ASSERT_TLM_jetson_cv0_temp_read_SIZE(1);
    ASSERT_TLM_jetson_cv1_temp_read_SIZE(1);
    ASSERT_TLM_jetson_cv2_temp_read_SIZE(1);
    ASSERT_TLM_jetson_soc0_temp_read_SIZE(1);
    ASSERT_TLM_jetson_soc1_temp_read_SIZE(1);
    ASSERT_TLM_jetson_soc2_temp_read_SIZE(1);
    ASSERT_TLM_jetson_tj_temp_read_SIZE(1);

    this->clearHistory();
    this->component.loadParameters();

    // First tick: boot/setup path. State stays INIT, m_justBooted becomes false.
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();

    // Force the next read to transition to FAIL.
    this->component.m_successfulRead = false;

    // Second tick: doRead runs, then queues fail; final dispatch enters FAIL.
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();
    this->component.doDispatch();

    // Third tick: now state is FAIL, so this executes doReadFail.
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();
    this->component.doDispatch();

    // doReadFail resets this flag.
    ASSERT_EQ(false,this->component.m_successfulRead);
    ASSERT_TLM_SIZE(9);
  }

}
