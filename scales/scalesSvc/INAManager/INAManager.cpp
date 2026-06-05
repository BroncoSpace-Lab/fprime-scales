// ======================================================================
// \title  INAManager.cpp
// \author jacob
// \brief  cpp file for INAManager component implementation class
// ======================================================================

#include "scales/scalesSvc/INAManager/INAManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  INAManager ::
    INAManager(const char* const compName) :
      INAManagerComponentBase(compName)
  {

  }

  INAManager ::
    ~INAManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void INAManager ::
    TODO_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
