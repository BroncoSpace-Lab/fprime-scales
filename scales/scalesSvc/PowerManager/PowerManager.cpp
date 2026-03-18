// ======================================================================
// \title  PowerManager.cpp
// \author dragon-scales
// \brief  cpp file for PowerManager component implementation class
// ======================================================================

#include "scales/scalesSvc/PowerManager/PowerManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  PowerManager ::
    PowerManager(const char* const compName) :
      PowerManagerComponentBase(compName)
  {

  }

  PowerManager ::
    ~PowerManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void PowerManager ::
    currentPwrMode_handler(
        FwIndexType portNum,
        const scalesSvc::PowerModeID& modeNow
    )
  {
    this->log_ACTIVITY_LO_POWER_MODE_RECEIVED(modeNow);
    this->tlmWrite_JetsonPowerMode(modeNow);
  }

  void PowerManager ::
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

  void PowerManager ::
    REQUEST_POWER_MODE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        scalesSvc::PowerModeID mode
    )
  {
    this->reqPwrMode_out(0, mode);
    this->log_ACTIVITY_HI_POWER_MODE_REQUESTED(mode);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
