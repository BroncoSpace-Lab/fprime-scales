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

      //! Returns true if the six bounds are in a sane ascending order:
      //! faultLow <= warnLow <= idleLow <= idleHigh <= warnHigh <= faultHigh
      bool thresholdsAreOrdered(const scalesSvc::TempBounds& bounds) const;

      //! Gate for a bounds update: adopts `candidate` as the active bounds
      //! (and republishes telemetry) only if it passes thresholdsAreOrdered();
      //! otherwise emits THRESHOLDS_MISCONFIGURED and leaves the previous
      //! bounds in effect.
      void applyBounds(const scalesSvc::TempBounds& candidate);

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

      //! Bounds currently in effect. Only ever updated with bounds that pass
      //! thresholdsAreOrdered() -- a rejected PRM_SET leaves this untouched.
      //! Initialized to the compiled-in safe default so a rejected boot-time
      //! read (e.g. a bad value saved to non-volatile storage previously)
      //! still leaves the component with a sane configuration.
      scalesSvc::TempBounds m_activeBounds{-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F};


  };

}

#endif
