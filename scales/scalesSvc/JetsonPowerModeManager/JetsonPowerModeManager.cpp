// ======================================================================
// \title  JetsonPowerModeManager.cpp
// \author dragon-scales
// \brief  cpp file for JetsonPowerModeManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManager.hpp"
#include <cstdlib>
#include <cstdio>
#include <string>

int get_nvp_mode();

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonPowerModeManager ::
    JetsonPowerModeManager(const char* const compName) :
      JetsonPowerModeManagerComponentBase(compName),
      m_modeReported(false)
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
    powerModeRecieve_handler(
        FwIndexType portNum,
        const scalesSvc::PowerModeID& modeReq
    )
  {
    U8 modeNow = get_nvp_mode();
    if (modeReq.e != static_cast<PowerModeID::T>(modeNow)) {
      // Mode change needed: run nvpmodel. The Jetson will reboot automatically.
      // Mark m_modeReported false so when the system comes back up, the first
      // schedIn tick will report the new mode to the IMX for confirmation.
      m_modeReported = false;
      std::system(("yes | sudo -n /usr/sbin/nvpmodel -m " + std::to_string(static_cast<U8>(modeReq.e))).c_str());
    } else {
      // Already in the requested mode — no reboot needed.
      // Send the current mode back immediately so the IMX can complete its
      // deferred REQUEST_POWER_MODE command without waiting for a reboot.
      this->powerModeSend_out(0, modeReq);
      this->tlmWrite_CurrentPowerMode(modeReq);
    }
  }

  void JetsonPowerModeManager ::
    schedIn_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // On the first schedIn tick after boot (m_modeReported == false), read and
    // report the current power mode. This is how the IMX knows the Jetson has
    // finished rebooting and can confirm the REQUEST_POWER_MODE command.
    // We only fire once per boot to avoid spamming the hub every tick.
    if (!m_modeReported) {
      U8 pwr_mode = get_nvp_mode();
      if (pwr_mode != 4) {
        PowerModeID current(static_cast<PowerModeID::T>(pwr_mode));
        this->powerModeSend_out(0, current);
        this->tlmWrite_CurrentPowerMode(current);
        m_modeReported = true;
      }
    }
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
    U8 modeNow = get_nvp_mode();
    if(mode.e != static_cast<PowerModeID::T>(modeNow))
    { //if the jetson's mode does not match the requested mode
      std::system(("yes | sudo -n /usr/sbin/nvpmodel -m " + std::to_string(static_cast<U8>(mode.e))).c_str()); // executes the NVIDIA power mode control command
      // sets the next power mode to specified mode after reset
      // answers "yes" to request to reset prompt
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
    else{//if the jetson is already in the requested mode, do nothing
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
    
  }

  void JetsonPowerModeManager ::
    GET_POWER_MODE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    U8 pwr_mode = get_nvp_mode();
    if (pwr_mode == 4)
    {//error getting power mode from jetson
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
    }
    else{//return jetson power mode
        this->powerModeSend_out(0, PowerModeID(static_cast<PowerModeID::T>(pwr_mode)));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
  }

} // namespace scalesSvc

// ----------------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------------

int get_nvp_mode()
{
  FILE *mode_file = popen("/usr/sbin/nvpmodel -q", "r");
  if (mode_file == nullptr) {
    return 4;
  }

  // Output format:
  //   NV Power Mode: MAXN
  //   0
  // Parse whichever line is a bare integer.
  char buffer[128];
  while (fgets(buffer, sizeof(buffer), mode_file) != nullptr) {
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    if (endptr != buffer && (*endptr == '\n' || *endptr == '\0')) {
      pclose(mode_file);
      return static_cast<int>(val);
    }
  }

  pclose(mode_file);
  return 4; // error: mode number not found in output
}
