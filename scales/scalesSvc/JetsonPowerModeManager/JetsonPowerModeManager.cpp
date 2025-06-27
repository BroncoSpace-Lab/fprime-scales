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

char get_nvp_mode();

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
    char modeNow = get_nvp_mode();
    if (modeNow == 'e')
    {//error getting power mode from jetson

    }
    else{//return jetson power mode
      this->
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

  // ----------------------------------------------------------------------
  // Private functions
  // ----------------------------------------------------------------------

  char get_nvp_mode()
  {
    char nvpMode = 0;
    std::string mode_str = "";

    FILE *mode_file = popen("/usr/sbin/nvpmodel -q", "r");

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), mode_file) != NULL)
    { //read until end of process
      mode_str+=buffer;
    }
    if(!mode_str.empty())
    {//if mode_str is not empty (we got output)
      nvpMode = mode_str.back(); //save mode identifier number
      mode_str.pop_back(); //delete mode number from string
      mode_str.erase(0,15); //deletes irrelevant output
      return nvpMode;
    }
    else{//mode_str is empty (no output)
      return 'e';
    }
  }

}
