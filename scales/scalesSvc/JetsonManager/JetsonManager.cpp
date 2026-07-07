// ======================================================================
// \title  JetsonManager.cpp
// \author lucal
// \brief  cpp file for JetsonManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonManager/JetsonManager.hpp"

namespace scalesSvc {

    
    constexpr U32 JETSON_POWER_OFF_DELAY_TICKS = 5;
    const Fw::Logic JETSON_POWER_GPIO_ON = Fw::Logic::HIGH;
    const Fw::Logic JETSON_POWER_GPIO_OFF = Fw::Logic::LOW;


  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonManager ::
    JetsonManager(const char* const compName) :
      JetsonManagerComponentBase(compName),
      m_hasPendingCmd(false),
      m_pendingOpCode(0),
      m_pendingCmdSeq(0),
      m_requestedMode(PowerModeID::MAX),
      m_timeoutTicks(0),
      m_hasPendingPowerCmd(false),
      m_pendingPowerOpCode(0),
      m_pendingPowerCmdSeq(0),
      m_requestedPowerState(scalesSvc::JetsonPowerStateID::OFF),
      m_powerTimeoutTicks(0),
      m_waitingToCutJetsonPower(false),
      m_powerOffDelayTicks(0)
      // instantiate private members in a constructor.
  {

  }

  JetsonManager ::
    ~JetsonManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void JetsonManager ::
    currentJetsonPwrState_handler(
        FwIndexType portNum,
        const scalesSvc::JetsonPowerStateID& stateNow
    )
  {
    this->log_ACTIVITY_LO_JETSON_POWER_STATE_RECEIVED(stateNow);
    this->tlmWrite_JetsonPowerState(stateNow);

    if (!m_hasPendingPowerCmd) {
      return; // Not waiting for a power state change confirmation, ignore
    }

    if(stateNow.e != m_requestedPowerState.e) {
      return; // Reported state doesn't match requested state, keep waiting (or eventually timeout)
    }

    if (stateNow.e == JetsonPowerStateID::OFF){
      // Jetson has acknowledged shutdown. Wait a few ticks before cutting
      // physical power so the shutdown command has time to start cleanly
      m_waitingToCutJetsonPower = true;
      m_powerOffDelayTicks = 0;
    }
  }

  void JetsonManager ::
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

  void JetsonManager ::
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

    // Delay before physically cutting Jetson power after shutdown acknowledgment
    if (m_waitingToCutJetsonPower) {
      m_powerOffDelayTicks++;

      if (m_powerOffDelayTicks >= JETSON_POWER_OFF_DELAY_TICKS) {
        this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);

        this->cmdResponse_out(
          m_pendingPowerOpCode,
          m_pendingPowerCmdSeq,
          Fw::CmdResponse::OK
        );

        m_waitingToCutJetsonPower = false;
        m_hasPendingPowerCmd = false;
        m_powerTimeoutTicks = 0;
        m_powerOffDelayTicks = 0;
      }
    }

    // Timeout for Jetson power-state command 
    if (m_hasPendingPowerCmd && !m_waitingToCutJetsonPower) {
      m_powerTimeoutTicks++;

      if (m_powerTimeoutTicks >= CMD_TIMEOUT_TICKS) {
        this->log_WARNING_HI_JETSON_POWER_STATE_TIMEOUT(m_requestedPowerState);

        // If OFF was requested and Jetson never acknowledged, fall safe by
        // cutting power anyway
        if (m_requestedPowerState.e == JetsonPowerStateID::OFF) {
          this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);
        }
        

        this->cmdResponse_out(
          m_pendingPowerOpCode,
          m_pendingPowerCmdSeq,
          Fw::CmdResponse::OK
        );

        m_hasPendingPowerCmd = false;
        m_powerTimeoutTicks = 0;
        m_waitingToCutJetsonPower = false;
        m_powerOffDelayTicks = 0;
      }
    }
  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void JetsonManager ::
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

  void JetsonManager ::
    REQUEST_JETSON_POWER_STATE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        scalesSvc::JetsonPowerStateID jetsonState
    )
  {
      if (m_hasPendingPowerCmd) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
      }

      m_pendingPowerOpCode = opCode;
      m_pendingPowerCmdSeq = cmdSeq;
      m_requestedPowerState = jetsonState;
      m_hasPendingPowerCmd = true;
      m_powerTimeoutTicks = 0;
      m_waitingToCutJetsonPower = false;
      m_powerOffDelayTicks = 0;

      this->log_ACTIVITY_HI_JETSON_POWER_STATE_REQUESTED(jetsonState);

      if (jetsonState.e == JetsonPowerStateID::ON) {
        // Jetson is possibly off, so it cannot receive a port call.
        // Power it on directly from the IMX GPIO and wait for the Jetson app
        // to boot and report ON through currentJetsonPwrState_handler.
        this->gpioSet_out(0, JETSON_POWER_GPIO_ON);
        this->tlmWrite_JetsonPowerState(jetsonState);
        m_hasPendingPowerCmd = false;
      } else if (jetsonState.e == JetsonPowerStateID::OFF) {
        // Jetson is currently on, so ask it to shut down
        // After it acknowledges OFF, this component will cut power with GPIO
        this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);  
        this->reqJetsonPwrState_out(0, jetsonState);
        m_hasPendingPowerCmd = false;
      } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        m_hasPendingPowerCmd = false;
      }
    }

}
