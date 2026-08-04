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

      //! A sensor's bounds update is only adopted (and its telemetry
      //! republished) if it passes thresholdsAreOrdered(); otherwise the
      //! sensor keeps its last-known-good bounds and THRESHOLDS_MISCONFIGURED
      //! fires -- every time a bad update is attempted, not just the first.
      void boundsUpdateGating();

      //! Drives each sensor's temperature into its WARN and FAULT bands (in
      //! addition to mcpTest()'s IDLE coverage) and confirms determineTempState()
      //! classifies each band correctly.
      void thermalStateEvaluation();

      //! Forces mcpWriteRead to fail for one run cycle: confirms the failing
      //! sensor is set to FAULT with FAIL_TO_READ_TEMP_AT, confirms the overall
      //! read-failure path (doReadFail) emits FAIL_TO_READ_TEMP and resets the
      //! per-sensor flags so the next cycle retries normally.
      void readFailureHandling();

      //! Exercises parameterUpdated() for the PERIPHERAL and JETSON bounds
      //! (boundsUpdateGating() only covers IMX) plus the default case for an
      //! unrecognized parameter ID, and writeBoundsTelemetry()'s default case
      //! for an out-of-range sensor index.
      void parameterUpdatedSwitchCoverage();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

      //! Dispatch messages until the queue is empty. The state machine chains
      //! several internally-generated signals per external tick (run_handler
      //! -> tick -> action -> success/fail signal), so a fixed dispatch count
      //! is fragile; this drains whatever chain a single invoke_to_run start.
      void drainQueue();

      //! Override: when m_forceI2cFailure is true, reports I2C_READ_ERR
      //! instead of I2C_OK, so readTemp()'s failure path can be exercised
      //! deterministically.
      Drv::I2cStatus from_mcpWriteRead_handler(
          FwIndexType portNum,
          U32 addr,
          Fw::Buffer& writeBuffer,
          Fw::Buffer& readBuffer
      ) override;

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      McpManager component;

      //! When true, from_mcpWriteRead_handler reports I2C_READ_ERR
      bool m_forceI2cFailure = false;

  };

}

#endif
