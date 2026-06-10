// ======================================================================
// \title  WatchdogManagerTester.hpp
// \author lucal
// \brief  hpp file for WatchdogManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_WatchdogManagerTester_HPP
#define scalesSvc_WatchdogManagerTester_HPP

#include "scales/scalesSvc/WatchdogManager/WatchdogManagerGTestBase.hpp"
#include "scales/scalesSvc/WatchdogManager/WatchdogManager.hpp"

namespace scalesSvc {

  class WatchdogManagerTester :
    public WatchdogManagerGTestBase
  {

    public:

      // ----------------------------------------------------------------------
      // Constants
      // ----------------------------------------------------------------------

      // Maximum size of histories storing events, telemetry, and port outputs
      static const FwSizeType MAX_HISTORY_SIZE = 10;

      // Instance ID supplied to the component instance under test
      static const FwEnumStoreType TEST_INSTANCE_ID = 0;

      // Queue depth supplied to the component instance under test
      static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    public:

      // ----------------------------------------------------------------------
      // Construction and destruction
      // ----------------------------------------------------------------------

      //! Construct object WatchdogManagerTester
      WatchdogManagerTester();

      //! Destroy object WatchdogManagerTester
      ~WatchdogManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! To do
      void toDo();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      WatchdogManager component;

  };

}

#endif
