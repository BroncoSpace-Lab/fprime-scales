// ======================================================================
// \title  JetsonPowerModeManager.cpp
// \author dragon-scales
// \brief  cpp file for JetsonPowerModeManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonPowerModeManager ::
    JetsonPowerModeManager(const char* const compName) :
      JetsonPowerModeManagerComponentBase(compName)
  {

  }

  JetsonPowerModeManager ::
    ~JetsonPowerModeManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void JetsonPowerModeManager ::
    pingReceive_handler(
        FwIndexType portNum,
        U32 key
    )
  {
    // TODO
  }

  void JetsonPowerModeManager ::
    powerStateRecieve_handler(
        FwIndexType portNum,
        const scalesSvc::PowerModeID& recieve
    )
  {
    // TODO
  }

  void JetsonPowerModeManager ::
    schedIn_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // TODO
  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void JetsonPowerModeManager ::
    SET_POWER_MODE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        scalesSvc::PowerModeID state
    )
  {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void JetsonPowerModeManager ::
    GET_POWER_MODE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void JetsonPowerModeManager ::
    CHECK_AWAKE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
