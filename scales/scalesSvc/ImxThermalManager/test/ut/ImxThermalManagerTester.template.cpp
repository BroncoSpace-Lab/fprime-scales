// ======================================================================
// \title  ImxThermalManagerTester.cpp
// \author lucal
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
    toDo()
  {
    // TODO
  }

}
