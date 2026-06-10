// ======================================================================
// \title  WatchdogManagerTester.cpp
// \author lucal
// \brief  cpp file for WatchdogManager component test harness implementation class
// ======================================================================

#include "WatchdogManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  WatchdogManagerTester ::
    WatchdogManagerTester() :
      WatchdogManagerGTestBase("WatchdogManagerTester", WatchdogManagerTester::MAX_HISTORY_SIZE),
      component("WatchdogManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  WatchdogManagerTester ::
    ~WatchdogManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void WatchdogManagerTester ::
    toDo()
  {
    // TODO
  }

}
