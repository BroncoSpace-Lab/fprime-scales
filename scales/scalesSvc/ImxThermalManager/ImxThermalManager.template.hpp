// ======================================================================
// \title  ImxThermalManager.hpp
// \author lucal
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

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct ImxThermalManager object
      ImxThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy ImxThermalManager object
      ~ImxThermalManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for imxCpuTemp
      //!
      //! asynchronous input port to handle incoming imx cpu temp
      void imxCpuTemp_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    PRIVATE:

      // ----------------------------------------------------------------------
      // Implementations for internal state machine actions
      // ----------------------------------------------------------------------

      //! Implementation for action doRead of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doRead(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action paramEvaluate of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_paramEvaluate(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action readFail of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_readFail(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

  };

}

#endif
