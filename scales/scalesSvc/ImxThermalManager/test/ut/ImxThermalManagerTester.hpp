// ======================================================================
// \title  ImxThermalManagerTester.hpp
// \author lucal
// \brief  hpp file for ImxThermalManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_ImxThermalManagerTester_HPP
#define scalesSvc_ImxThermalManagerTester_HPP

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManagerGTestBase.hpp"
#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"



namespace scalesSvc {

  class ImxThermalManagerTester :
    public ImxThermalManagerGTestBase
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

      //! Construct object ImxThermalManagerTester
      ImxThermalManagerTester();

      //! Destroy object ImxThermalManagerTester
      ~ImxThermalManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! To do
      void ImxThermalManagerTesting();

      //! A bounds update is only adopted (and its telemetry republished) if
      //! it passes thresholdsAreOrdered(); otherwise the component keeps its
      //! last-known-good bounds and THRESHOLDS_MISCONFIGURED fires -- every
      //! time a bad update is attempted, not just the first.
      void boundsUpdateGating();

      //! Drives the high-side FAULT gap (strictly between WARN_HIGH and
      //! FAULT_HIGH -- a distinct disjunct from the low-side FAULT case
      //! ImxThermalManagerTesting() already covers) and confirms classification.
      void highSideFaultGap();

      //! Writes malformed content (empty file, non-numeric, trailing garbage
      //! after the number) to the temperature file and confirms
      //! readTemperatureFile() rejects each case, exercising the parsing
      //! failure branches ImxThermalManagerTesting()'s missing-file case
      //! doesn't reach.
      void malformedTempFile();

      //! Exercises parameterUpdated() directly (boundsUpdateGating() calls
      //! applyBounds() directly, bypassing this) for both the real parameter
      //! ID and an unrecognized one (default case).
      void parameterUpdatedCoverage();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Write a sysfs-style millidegree Celsius value through the OSAL
      void writeTemperatureFile(const char* path, F32 tempC);

      //! Write arbitrary raw content to the temperature file (for malformed-input cases)
      void writeRawTempFile(const char* path, const char* content);

      //! Run a tick through the component and dispatch the resulting state-machine action
      void runTickAction();

      //! Read the currently configured temperature file and evaluate the result
      void readAndEvaluateTemperature();

      //! Assert the latest thermal telemetry reading
      void assertLatestReading(FwSizeType expectedHistorySize, F32 tempC, scalesSvc::ThermalStates expectedState);

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      ImxThermalManager component;

  };

  

}

#endif
