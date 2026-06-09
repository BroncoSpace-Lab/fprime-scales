// ======================================================================
// \title  ImxThermalManager.cpp
// \author lucal
// \brief  cpp file for ImxThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManager ::
    ImxThermalManager(const char* const compName) :
      ImxThermalManagerComponentBase(compName)
  {

  }

  ImxThermalManager ::
    ~ImxThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void ImxThermalManager ::
    imxCpuTemp_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // TODO
  }

  // ----------------------------------------------------------------------
  // Implementations for internal state machine actions
  // ----------------------------------------------------------------------

  void ImxThermalManager ::
    scalesSvc_ThermalStateMachine_action_doRead(
        SmId smId,
        scalesSvc_ThermalStateMachine::Signal signal
    )
  {
    // TODO
  }

  void ImxThermalManager ::
    scalesSvc_ThermalStateMachine_action_paramEvaluate(
        SmId smId,
        scalesSvc_ThermalStateMachine::Signal signal
    )
  {
    // TODO
  }

  void ImxThermalManager ::
    scalesSvc_ThermalStateMachine_action_readFail(
        SmId smId,
        scalesSvc_ThermalStateMachine::Signal signal
    )
  {
    // TODO
  }

}
