// ======================================================================
// \title  PerifBoardManagerTester.hpp
// \author luquito
// \brief  hpp file for PerifBoardManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_PerifBoardManagerTester_HPP
#define scalesSvc_PerifBoardManagerTester_HPP

#include "scales/scalesSvc/PerifBoardManager/PerifBoardManagerGTestBase.hpp"
#include "scales/scalesSvc/PerifBoardManager/PerifBoardManager.hpp"

namespace scalesSvc {

  class PerifBoardManagerTester :
    public PerifBoardManagerGTestBase
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

      //! Construct object PerifBoardManagerTester
      PerifBoardManagerTester();

      //! Destroy object PerifBoardManagerTester
      ~PerifBoardManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! Test the PerifBoardManager component
      void testPerifBoardManager();

      //! FPManager's emergencyPowerOff signal latches the board off: GPIO is
      //! forced LOW immediately and stays LOW on every subsequent run_handler
      //! cycle, even across a powerOn(ON) command, since the latch is never
      //! cleared once set.
      void emergencyShutdownLatch();

      //! offTimeSec is a live parameter, not a compile-time constant -- a
      //! non-default value must change how long the OFF state is held.
      void configurableOffInterval();

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
      PerifBoardManager component;

  };

}

#endif
