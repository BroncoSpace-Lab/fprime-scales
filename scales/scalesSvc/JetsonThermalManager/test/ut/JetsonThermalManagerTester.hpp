// ======================================================================
// \title  JetsonThermalManagerTester.hpp
// \author lucal
// \brief  hpp file for JetsonThermalManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_JetsonThermalManagerTester_HPP
#define scalesSvc_JetsonThermalManagerTester_HPP

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManagerGTestBase.hpp"
#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManager.hpp"

namespace scalesSvc {

  class JetsonThermalManagerTester :
    public JetsonThermalManagerGTestBase
  {

    public:

      // ----------------------------------------------------------------------
      // Constants
      // ----------------------------------------------------------------------

      // Maximum size of histories storing events, telemetry, and port outputs
      static const FwSizeType MAX_HISTORY_SIZE = 30;

      // Instance ID supplied to the component instance under test
      static const FwEnumStoreType TEST_INSTANCE_ID = 0;

      // Queue depth supplied to the component instance under test
      static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    public:

      // ----------------------------------------------------------------------
      // Construction and destruction
      // ----------------------------------------------------------------------

      //! Construct object JetsonThermalManagerTester
      JetsonThermalManagerTester();

      //! Destroy object JetsonThermalManagerTester
      ~JetsonThermalManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! To do
      void JetsonThermalManagerUnitTester();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Write a sysfs-style millidegree Celsius value through the OSAL
      void writeTemperatureFile(U8 index, F32 tempC);

      //! Run a tick through the component and dispatch queued work
      void runTickAction();

      //! Read the configured temperature files and evaluate the result
      void readAndEvaluateTemperatures();

      //! Assert the latest thermal telemetry reading for one Jetson thermal zone
      void assertLatestReading(
          U8 index,
          FwSizeType expectedHistorySize,
          F32 tempC,
          const char* expectedLocation,
          scalesSvc::ThermalStates expectedState
      );

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      JetsonThermalManager component;

      CHAR m_tempPathTemplate[128];

  };

}

#endif
