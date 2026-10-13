// ======================================================================
// \title  JetsonThermalManager.hpp
// \author scales
// \brief  hpp file for JetsonThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_JetsonThermalManager_HPP
#define scalesSvc_JetsonThermalManager_HPP

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManagerComponentAc.hpp"


namespace scalesSvc {

  class JetsonThermalManager :
    public JetsonThermalManagerComponentBase
  {
    friend class JetsonThermalManagerTester;

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

      void setTempPathTemplate(const char* pathTemplate) {
        this->m_tempPathTemplate = pathTemplate;
      }

    private:

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

    private:

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

      // ----------------------------------------------------------------------
      // Implementations for parameters update
      // ----------------------------------------------------------------------
      void parameterUpdated(FwPrmIdType id) override;  //! Handler implementation for parameter updates, used to update threshold values when parameters are updated

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      bool readTemp(U8 index, F32& temp); // Helper function to read temp from a given device address

      scalesSvc::ThermalStates determineTempState(F32 tempCelsius); //!< Function to determine the temperature state (IDLE, WARNING, FAULT) based on the temperature in Celsius

      //! Returns true if the six bounds are in a sane ascending order:
      //! faultLow <= warnLow <= idleLow <= idleHigh <= warnHigh <= faultHigh
      bool thresholdsAreOrdered(const scalesSvc::TempBounds& bounds) const;

      //! Gate for a bounds update: adopts `candidate` as the active bounds
      //! (shared across all nine zones, and republishes telemetry) only if
      //! it passes thresholdsAreOrdered(); otherwise emits
      //! THRESHOLDS_MISCONFIGURED and leaves the previous bounds in effect.
      void applyBounds(const scalesSvc::TempBounds& candidate);

    private:

      // Private instance variables/specific members
      scalesSvc::ThermalReading m_jetsonThermalReadings[9]; //!< The 9 thermal zones on the jetson
      bool m_justBooted; // Whether the device has just booted, used to determine whether to set up parameters or read temp on first tick
      bool m_successfulRead; // Whether the most recent read was successful, used to determine whether to transition to EVALUATE or FAIL state
      Fw::ParamValid m_paramIsValid = Fw::ParamValid::VALID;
      U32  m_startTime = 0;
      const char* m_tempPathTemplate = "/sys/class/thermal/thermal_zone%u/temp";

      //! Bounds currently in effect, shared across all nine zones. Only ever
      //! updated with bounds that pass thresholdsAreOrdered() -- a rejected
      //! PRM_SET leaves this untouched. Initialized to the compiled-in safe
      //! default so a rejected boot-time read (e.g. a bad value saved to
      //! non-volatile storage previously) still leaves the component with a
      //! sane configuration.
      scalesSvc::TempBounds m_activeBounds{-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F};
  };

}

#endif
