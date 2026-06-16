// ======================================================================
// \title  McpManagerTester.hpp
// \author bidat
// \brief  hpp file for McpManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_McpManagerTester_HPP
#define scalesSvc_McpManagerTester_HPP

#include "scales/scalesSvc/McpManager/McpManagerGTestBase.hpp"
#include "scales/scalesSvc/McpManager/McpManager.hpp"

namespace scalesSvc {

  class McpManagerTester :
    public McpManagerGTestBase
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

      //! Construct object McpManagerTester
      McpManagerTester();

      //! Destroy object McpManagerTester
      ~McpManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! To do
      void mcpTest();

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
      McpManager component;

  };

}

#endif
