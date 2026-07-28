// ======================================================================
// \title  ImxThermalManager.hpp
// \author luquito
// \brief  hpp file for ImxThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_ImxThermalManager_HPP
#define scalesSvc_ImxThermalManager_HPP

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManagerComponentAc.hpp"


namespace scalesSvc {

  class ImxThermalManager :
    public ImxThermalManagerComponentBase
  {
    friend class ImxThermalManagerTester;

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct ImxThermalManager object
      ImxThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy ImxThermalManager object
      ~ImxThermalManager();

      //! Overrides the OSAL path read every tick; used by tests to point at
      //! a fake temperature file instead of the real thermal zone.
      void setTempPath(const char* path) {
        this->tempPath = path;
      }

    private:

      
      //! Handler implementation for run
      void run_handler(FwIndexType portNum,  //!< The port number
                            U32 context           //!< The call order
                            ) override;

    //! Implementation for action doRead of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doRead(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doEvaluate of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doEvaluate(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doReadFail of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doReadFail(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Helper function to read temperature files with OSAL
      bool readTemperatureFile();

      // ----------------------------------------------------------------------
      // Implementations for parameters update
      // ----------------------------------------------------------------------
      void parameterUpdated(FwPrmIdType id) override;

      //! Returns true if the six thresholds are in a sane ascending order:
      //! FAULT_LOW <= WARN_LOW <= IDLE_LOW <= IDLE_HIGH <= WARN_HIGH <= FAULT_HIGH
      bool thresholdsAreOrdered(F32 faultLow, F32 warnLow, F32 idleLow,
                                 F32 idleHigh, F32 warnHigh, F32 faultHigh) const;

      //! Reads the current six thresholds and emits THRESHOLDS_MISCONFIGURED
      //! on the transition into a bad ordering.
      void validateThresholds();

      //! Publishes the current parameter values for GDS readback.
      void writeParameterTelemetry();

    private:

      //! Class instance variables
      scalesSvc::ThermalReading m_cpu_thermal_read;
      Fw::ParamValid m_paramValid = Fw::ParamValid::VALID;
      F32 m_tempMilliC = 0.0f;
      F32 m_tempC = 0.0f;
      U32  m_startTime = 0;
      bool m_justBooted = true;
      bool m_successfulRead = true;
      const char* tempPath = "/sys/class/thermal/thermal_zone0/temp";

      //! Tracks whether the thresholds were last found to be in a sane
      //! ascending order, so THRESHOLDS_MISCONFIGURED is only emitted on the
      //! transition into a bad configuration, not on every check.
      bool m_thresholdsValid = true;


  };

}

#endif
