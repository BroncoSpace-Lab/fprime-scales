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
      m_modeReported(false),
      m_powerStateReported(false)
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
    powerModeReceive_handler(
        FwIndexType portNum,
        const scalesSvc::PowerModeID& modeReq
    )
  {
    // Direct stdout log — appears in journalctl regardless of GDS dictionary or hub state.
    // If this line appears but POWER_MODE_REQUEST_RECEIVED does NOT appear in the GDS,
    // the issue is a stale GDS dictionary (re-run merger.py with the updated Jetson dict).
    // If this line does NOT appear, the port call from the IMX is not arriving.
    printf("JetsonPowerModeManager: Received power mode request %d on port %d\n", static_cast<int>(modeReq.e), portNum); //added by kelly for debugging PowerManager issue
    // This event fires as soon as the hub port call is received from the IMX.
    // If you do NOT see this in the GDS after sending REQUEST_POWER_MODE,
    // the hub is not delivering the port call — check TCP connectivity and
    // that both sides were rebuilt with the updated source files.
    this->log_ACTIVITY_HI_POWER_MODE_REQUEST_RECEIVED(modeReq);

    U8 modeNow = get_nvp_mode();
    if (modeReq.e != static_cast<PowerModeID::T>(modeNow)) {
      // Mode change needed: run nvpmodel. The Jetson will reboot automatically.
      // Mark m_modeReported false so when the system comes back up, the first
      // schedIn tick will report the new mode to the IMX for confirmation.
      m_modeReported = false;
      int ret = std::system(("echo y | sudo -n /usr/sbin/nvpmodel -m " + std::to_string(static_cast<U8>(modeReq.e))).c_str());
      if (ret != 0) {
        // nvpmodel failed (non-zero exit) — report it so the IMX can see the error.
        // This also prevents the IMX from waiting forever for a confirmation.
        Fw::String reason("nvpmodel returned non-zero exit code");
        this->log_WARNING_HI_POWER_MODE_CHANGE_FAILED(modeReq, reason);
        // Send the current (unchanged) mode back so the IMX deferred command can
        // detect the mismatch and time out with EXECUTION_ERROR instead of hanging.
        PowerModeID current(static_cast<PowerModeID::T>(modeNow));
        this->powerModeSend_out(0, current);
      }
      // If ret == 0, nvpmodel is rebooting — no further action here.
      // The reboot confirmation flows through schedIn_handler on the next boot.
    } else {
      // Already in the requested mode — no reboot needed.
      // Send the current mode back immediately so the IMX can complete its
      // deferred REQUEST_POWER_MODE command without waiting for a reboot.
      this->powerModeSend_out(0, modeReq);
      this->tlmWrite_CurrentPowerMode(modeReq);
    }
  }

  void JetsonPowerModeManager ::
    jetsonPowerStateReceive_handler(
        FwIndexType portNum,
        const scalesSvc::JetsonPowerStateID& stateReq
      )
    {
      // Line for debugging 
      printf(
          "JetsonPowerModeManager: Received jetson power state request %d on port %d\n",
          static_cast<int>(stateReq.e),
          portNum
      );

      this->log_ACTIVITY_HI_JETSON_POWER_STATE_REQUEST_RECEIVED(stateReq);

      if (stateReq.e == JetsonPowerStateID::ON) {
        // If this handler is running, the Jetson is already on
        this->jetsonPowerStateSend_out(0, JetsonPowerStateID::ON);
        this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::ON);
        return;
      }

      if (stateReq.e == JetsonPowerStateID::OFF) {
        // Acknowldege OFF to the IMX before shutting down. The IMX can then
        // wait a few ticks and cut the Jetson power rail using GPIO.
        this->jetsonPowerStateSend_out(0, JetsonPowerStateID::OFF);
        this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::OFF);
        this->log_ACTIVITY_HI_JETSON_SHUTDOWN_STARTED(stateReq);
        
        int ret = std::system("sudo -n /sbin/shutdown -h now");

        if (ret != 0) {
          Fw::String reason("shutdown command returned non-zero exit code");
          this->log_WARNING_HI_JETSON_POWER_STATE_CHANGE_FAILED(stateReq, reason);

          // Report ON because shutdown failed and the Jetson is still alive.
          this->jetsonPowerStateSend_out(0, JetsonPowerStateID::ON);
          this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::ON);
        }

        return;
      }

      Fw::String reason("Invalid Jetson power state");
      this->log_WARNING_HI_JETSON_POWER_STATE_CHANGE_FAILED(stateReq, reason);
    }

  void JetsonPowerModeManager ::
    schedIn_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // Report power state once after boot. This is how the IMX confirms that
    // a previous REQUEST_JETSON_POWER(ON) command succeeded.
    if (!m_powerStateReported) {
      this->jetsonPowerStateSend_out(0, JetsonPowerStateID::ON);
      this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::ON);
      m_powerStateReported = true;
    }

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
      std::system(("echo y | sudo -n /usr/sbin/nvpmodel -m " + std::to_string(static_cast<U8>(mode.e))).c_str());
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

  void JetsonPowerModeManager ::
    SET_JETSON_POWER_STATE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        scalesSvc::JetsonPowerStateID state
    )
  {
    if (state.e == JetsonPowerStateID::ON) {
      // If this command is running locally on the Jetson, the Jetson is already on.
      this->jetsonPowerStateSend_out(0, JetsonPowerStateID::ON);
      this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::ON);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
      return;
    }

    if (state.e == JetsonPowerStateID::OFF) {
      this->jetsonPowerStateSend_out(0, JetsonPowerStateID::OFF);
      this->tlmWrite_CurrentJetsonPowerState(JetsonPowerStateID::OFF);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);

      int ret = std::system("sudo -n /sbin/shutdown -h now");

      if (ret ==0) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
      } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
      }

      return;
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
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
