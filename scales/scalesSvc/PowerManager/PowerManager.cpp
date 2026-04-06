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
      PowerManagerComponentBase(compName),
      m_hasPendingCmd(false),
      m_pendingOpCode(0),
      m_pendingCmdSeq(0),
      m_requestedMode(PowerModeID::MAX),
      m_timeoutTicks(0)
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

    // If we are waiting for the Jetson to confirm a mode change, check whether
    // the reported mode matches what we requested. If so, complete the deferred
    // REQUEST_POWER_MODE command with OK. This fires once after the Jetson
    // reboots and its schedIn reports the current mode back through the hub.
    if (m_hasPendingCmd && modeNow.e == m_requestedMode.e) {
      this->cmdResponse_out(m_pendingOpCode, m_pendingCmdSeq, Fw::CmdResponse::OK);
      m_hasPendingCmd = false;
    }
  }

  void PowerManager ::
    schedIn_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // If a REQUEST_POWER_MODE is pending, tick the timeout counter.
    // The Jetson must reboot and reconnect within CMD_TIMEOUT_TICKS ticks or
    // the command is failed so the GDS doesn't wait forever.
    if (m_hasPendingCmd) {
      m_timeoutTicks++;
      if (m_timeoutTicks >= CMD_TIMEOUT_TICKS) {
        this->cmdResponse_out(m_pendingOpCode, m_pendingCmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        m_hasPendingCmd = false;
        m_timeoutTicks = 0;
      }
    }
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
    // Store the command identifiers so we can send a deferred response once the
    // Jetson confirms the mode change after rebooting. Do NOT call cmdResponse_out
    // here — the command stays open until currentPwrMode_handler gets a matching
    // report from the Jetson (or the timeout fires in schedIn).
    m_pendingOpCode = opCode;
    m_pendingCmdSeq = cmdSeq;
    m_requestedMode = mode;
    m_hasPendingCmd = true;
    m_timeoutTicks = 0;

    // Send the mode change request to the Jetson via the hub port.
    // JetsonPowerModeManager::powerModeRecieve_handler will run nvpmodel and reboot.
    this->reqPwrMode_out(0, mode);
    this->log_ACTIVITY_HI_POWER_MODE_REQUESTED(mode);
  }

}
