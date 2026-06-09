// ======================================================================
// \title  JetsonThermalManager.cpp
// \author scales
// \brief  cpp file for JetsonThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonThermalManager :: JetsonThermalManager(const char* const compName) : JetsonThermalManagerComponentBase(compName)
  {

  }

  JetsonThermalManager :: ~JetsonThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void JetsonThermalManager :: run_handler(FwIndexType portNum, U32 context)
  {
    // TODO
  }

  // ----------------------------------------------------------------------
  // Implementations for internal state machine actions
  // ----------------------------------------------------------------------

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // TODO
  }

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doEvaluate(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // TODO
  }

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // TODO
  }

}
