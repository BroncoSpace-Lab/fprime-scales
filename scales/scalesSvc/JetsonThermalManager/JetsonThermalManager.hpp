// ======================================================================
// \title  JetsonThermalManager.hpp
// \author scales
// \brief  hpp file for JetsonThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_JetsonThermalManager_HPP
#define scalesSvc_JetsonThermalManager_HPP

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManagerComponentAc.hpp"
#include <fstream>
#include <iostream>


namespace scalesSvc {

  class JetsonThermalManager :
    public JetsonThermalManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct JetsonThermalManager object
      JetsonThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy JetsonThermalManager object
      ~JetsonThermalManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for jetsonTempRead
      //!
      //! Synchronous input port to handle incoming jetson temp readings
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    PRIVATE:

      // ----------------------------------------------------------------------
      // Implementations for internal state machine actions
      // ----------------------------------------------------------------------

      //! Implementation for action doRead of state machine scalesSvc_ThermalStateMachine
      //!
      //! Read the temp values from the device
      void scalesSvc_ThermalStateMachine_action_doRead(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doEvaluate of state machine scalesSvc_ThermalStateMachine
      //!
      //! Evaluate the temp values against thresholds and update telemetry
      void scalesSvc_ThermalStateMachine_action_doEvaluate(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doReadFail of state machine scalesSvc_ThermalStateMachine
      //!
      //! Log a read failure event`
      void scalesSvc_ThermalStateMachine_action_doReadFail(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;
    
    PRIVATE:

      // Private instance variables/specific members
      scalesSvc::ThermalReading m_jetsonThermalReadings[9]; //!< The 9 thermal zones on the jetson


  };

}

#endif
