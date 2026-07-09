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

    public:

      void setTempPath(const char* path) {
        this->tempPath = path;
  }

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct ImxThermalManager object
      ImxThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy ImxThermalManager object
      ~ImxThermalManager();

    private:

      
      //! Handler implementation for imxCpuTemp
    void imxCpuTemp_handler(FwIndexType portNum,  //!< The port number
                            U32 context           //!< The call order
                            ) override;
          
      
    scalesSvc::ThermalReading m_cpu_thermal_read;
    Fw::ParamValid m_paramValid = Fw::ParamValid::VALID;
    F32 m_tempMilliC = 0.0f;
    F32 m_tempC = 0.0f;
    U32  m_startTime = 0;
    bool m_justBooted = true;
    const char* tempPath = "/sys/class/thermal/thermal_zone0/temp";

    scalesSvc::ThermalStates m_idleState = scalesSvc::ThermalStates::IDLE;
    scalesSvc::ThermalStates m_warnState = scalesSvc::ThermalStates::WARN;
    scalesSvc::ThermalStates m_faultState = scalesSvc::ThermalStates::FAULT;

    bool readTemperatureFile();

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


  };

}

#endif
