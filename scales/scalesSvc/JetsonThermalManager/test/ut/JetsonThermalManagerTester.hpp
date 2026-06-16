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
      static const FwSizeType MAX_HISTORY_SIZE = 10;

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

  };

}

#endif
