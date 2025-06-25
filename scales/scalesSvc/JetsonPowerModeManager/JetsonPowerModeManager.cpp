// ======================================================================
// \title  JetsonPowerModeManager.cpp
// \author dragon-scales
// \brief  cpp file for JetsonPowerModeManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManager.hpp"
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <iostream>

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
    powerModeRecieve_handler(
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
        scalesSvc::PowerModeID mode
    )
  {
    // TODO
    std::system("yes | /usr/sbin/nvpmodel -m "+mode); // executes the NVIDIA power mode control command
    // sets the next power mode to specified mode after reset
    // answers "yes" to request to reset prompt
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void JetsonPowerModeManager ::
    GET_POWER_MODE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    // TODO
    // std::system("/usr/sbin/nvpmodel -q"); // executes the NVIDIA power mode control command
    // returns the current power mode
    FILE *mode_file = popen("/usr/sbin/nvpmodel -q", "r");

    char buffer[128];
    std::string mode_str = "";
    while (fgets(buffer, sizeof(buffer), mode_file) != NULL)
    { //read until end of process
      mode_str+=buffer;
    }
    if(!mode_str.empty())
    {
      mode_str.pop_back(); //delete last character of string
      mode_str.erase(0,15); //deletes irrelevant output

      this->powerModeSend_out(0, mode_str);

      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
    
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
