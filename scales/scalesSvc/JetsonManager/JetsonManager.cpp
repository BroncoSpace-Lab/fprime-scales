// ======================================================================
// \title  JetsonManager.cpp
// \author lucal
// \brief  cpp file for JetsonManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonManager/JetsonManager.hpp"

namespace scalesSvc {

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
      m_modeChangeCmdRespond(false),
      m_hasPendingPowerCmd(false),
      m_pendingPowerOpCode(0),
      m_pendingPowerCmdSeq(0),
      m_requestedPowerState(scalesSvc::JetsonPowerStateID::OFF),
      m_currentJetsonPowerState(scalesSvc::JetsonPowerStateID::OFF),
      m_jetsonPowerStateKnown(false),
      m_powerTimeoutTicks(0),
      m_waitingToCutJetsonPower(false),
      m_powerOffDelayTicks(0),
      m_pendingPowerCmdRespond(false),
      m_awaitingBootConfirmation(false),
      m_bootConfirmationTimeoutTicks(0),
      m_deferredOffPending(false),
      m_hubLinkConnected(false)
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

  void JetsonManager::fpJetsonPowerRequestIn_handler(
      FwIndexType portNum,
      const scalesSvc::JetsonPowerStateID& stateReq
  ) {
    if (stateReq.e != JetsonPowerStateID::OFF) {
      return;
    }

    m_pendingPowerOpCode = 0;
    m_pendingPowerCmdSeq = 0;
    m_pendingPowerCmdRespond = false;
    m_requestedPowerState = stateReq;

    // FPManager (e.g. beginDisableHpcMode) asks for this OFF unconditionally
    // and immediately -- beginJetsonOffSequence() decides for itself
    // whether that needs to be deferred (boot in progress, or a
    // mode-change reboot in flight) or can execute now.
    this->beginJetsonOffSequence();
  }

  void JetsonManager ::
    currentJetsonPwrState_handler(
        FwIndexType portNum,
        const scalesSvc::JetsonPowerStateID& stateNow
    )
  {
    this->log_ACTIVITY_LO_JETSON_POWER_STATE_RECEIVED(stateNow);
    const bool bootWasOutstanding = m_awaitingBootConfirmation;
    m_currentJetsonPowerState = stateNow;
    m_jetsonPowerStateKnown = true;
    // Any real report -- ON or OFF -- proves the Jetson has booted far enough
    // to reach us over the hub, so the boot-confirmation guard is satisfied.
    m_awaitingBootConfirmation = false;
    m_bootConfirmationTimeoutTicks = 0;
    this->tlmWrite_JetsonPowerState(stateNow);
    if (this->isConnected_fpJetsonPowerStateOut_OutputPort(0)) {
      this->fpJetsonPowerStateOut_out(0, stateNow);
    }

    if (bootWasOutstanding && m_deferredOffPending) {
      // A boot confirmation just arrived while an OFF request was being held
      // pending it -- fire it now. beginJetsonOffSequence() re-evaluates the
      // just-updated confirmed state itself, so this is correct whether this
      // report says ON (normal case: proceed with the graceful ask) or OFF
      // (treated the same as already-off: idempotent direct cut).
      this->beginJetsonOffSequence();
      return;
    }

    if (!m_hasPendingPowerCmd) {
      printf("Not waiting for any power command\n");
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

    if (!m_hasPendingCmd) {
      return;
    }

    if (m_modeChangeCmdRespond) {
      // A real REQUEST_POWER_MODE command is outstanding -- check whether
      // the reported mode matches what we requested. If so, complete the
      // deferred command with OK. This fires once after the Jetson reboots
      // and its schedIn reports the current mode back through the hub.
      if (modeNow.e != m_requestedMode.e) {
        return; // Reported state doesn't match requested state, keep waiting (or eventually timeout)
      }
      this->cmdResponse_out(m_pendingOpCode, m_pendingCmdSeq, Fw::CmdResponse::OK);
    }
    // else: m_hasPendingCmd was armed externally (localModeChangeStarted_handler,
    // a LOCAL SET_POWER_MODE run directly on the Jetson) -- there is no
    // requested mode to match and no opcode/cmdSeq to respond to. ANY
    // report proves the reboot that started it has completed and the hub
    // link can be trusted again.

    m_hasPendingCmd = false;
    m_timeoutTicks = 0;
    if (m_deferredOffPending) {
      // The mode change just confirmed (or the externally-triggered guard
      // just cleared) and m_hasPendingCmd is now clear, so the hub-trust
      // condition may be satisfiable again -- re-evaluate and fire the
      // deferred OFF now, mirroring currentJetsonPwrState_handler's
      // boot-confirmation auto-fire.
      this->beginJetsonOffSequence();
    }
  }

  void JetsonManager ::
    localModeChangeStarted_handler(
        FwIndexType portNum,
        const scalesSvc::PowerModeID& mode
    )
  {
    // A mode-change guard is already in flight -- either a real hub-driven
    // REQUEST_POWER_MODE (m_modeChangeCmdRespond == true; its
    // m_pendingOpCode/m_pendingCmdSeq/m_timeoutTicks bookkeeping must not be
    // clobbered, or that command's caller would never get a response) or an
    // earlier local notification (duplicate/race). Either way the guard is
    // already correctly armed; this call is a harmless no-op.
    if (m_hasPendingCmd) {
      return;
    }

    // JetsonPowerModeManager's SET_POWER_MODE_cmdHandler is about to run
    // nvpmodel locally (bypassing JetsonManager/FPManager entirely -- see
    // JPSM-013/JM-014). Arm the same hub-link-distrust guard
    // REQUEST_POWER_MODE_cmdHandler arms for a hub-driven mode change, so
    // isJetsonHubLinkTrusted() correctly reports false until the reboot
    // completes, and reqPwrMode_out()/reqJetsonPwrState_out() aren't risked
    // against the down hub link. No opcode/cmdSeq to respond to --
    // externally triggered.
    m_hasPendingCmd = true;
    m_modeChangeCmdRespond = false;
    m_timeoutTicks = 0;
    this->log_ACTIVITY_HI_LOCAL_MODE_CHANGE_STARTED_RECEIVED(mode);
  }

  void JetsonManager ::
    hubComStatusIn_handler(
        FwIndexType portNum,
        Fw::Success& condition
    )
  {
    const bool wasConnected = m_hubLinkConnected;
    m_hubLinkConnected = (condition == Fw::Success::SUCCESS);
    if (wasConnected && !m_hubLinkConnected) {
      this->log_WARNING_HI_JETSON_HUB_LINK_DOWN();
    } else if (!wasConnected && m_hubLinkConnected) {
      this->log_ACTIVITY_HI_JETSON_HUB_LINK_RECONNECTED();
    }
  }

  void JetsonManager ::
    schedIn_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    // Report the current hub-link-trust status every tick (not just on
    // change) so a GDS session that connects late still sees an accurate
    // value -- mirrors FPManager's own FAULT_DEBOUNCE_COUNT republish
    // pattern (FPManager.cpp run_handler). i.MX-internal only. See JM-015.
    if (this->isConnected_fpJetsonHubTrustedOut_OutputPort(0)) {
      this->fpJetsonHubTrustedOut_out(0, this->isJetsonHubLinkTrusted());
    }

    // Bound how long a commanded ON can leave a deferred OFF waiting: if the
    // Jetson never reports in (hardware fault, GPIO miswire, etc.), give up
    // after CMD_TIMEOUT_TICKS rather than waiting forever.
    if (m_awaitingBootConfirmation) {
      m_bootConfirmationTimeoutTicks++;
      if (m_bootConfirmationTimeoutTicks >= CMD_TIMEOUT_TICKS) {
        m_awaitingBootConfirmation = false;
        m_bootConfirmationTimeoutTicks = 0;
        if (m_deferredOffPending) {
          // Gave up waiting for the Jetson's first report with an OFF
          // request still held pending it -- force it through via
          // beginJetsonOffSequence()'s direct-cut fallback rather than
          // leaving it stuck forever (same "OFF always eventually
          // completes" philosophy as the power-state timeout fallback
          // below).
          this->log_WARNING_HI_JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT();
          this->beginJetsonOffSequence();
        } else {
          this->log_WARNING_HI_JETSON_BOOT_CONFIRMATION_TIMEOUT();
        }
      }
    }

    // If a REQUEST_POWER_MODE is pending, tick the timeout counter.
    // The Jetson must reboot and reconnect within CMD_TIMEOUT_TICKS ticks or
    // the command is failed so the GDS doesn't wait forever.
    if (m_hasPendingCmd) {
      m_timeoutTicks++;
      if (m_timeoutTicks >= CMD_TIMEOUT_TICKS) {
        if (m_modeChangeCmdRespond) {
          this->cmdResponse_out(m_pendingOpCode, m_pendingCmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        }
        m_hasPendingCmd = false;
        m_timeoutTicks = 0;
        if (m_deferredOffPending) {
          // Gave up waiting for the Jetson's power-mode confirmation with an
          // OFF request still held pending it -- resume it now via
          // beginJetsonOffSequence(), which re-evaluates the (unchanged by
          // this timeout) cached power state itself.
          this->log_WARNING_HI_JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT();
          this->beginJetsonOffSequence();
        }
      }
    }

    // Delay before physically cutting Jetson power after shutdown acknowledgment
    if (m_waitingToCutJetsonPower) {
      m_powerOffDelayTicks++;
      printf("JetsonManager: Waiting to cut Jetson power, tick %d\n", m_powerOffDelayTicks);

      if (m_powerOffDelayTicks >= this->paramGet_JETSON_POWER_OFF_DELAY_TICKS(m_paramIsValid)) {
        printf("JetsonManager: Cutting Jetson power after shutdown acknowledgment\n");
        this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);
        m_currentJetsonPowerState = JetsonPowerStateID::OFF;
        m_jetsonPowerStateKnown = true;
        m_awaitingBootConfirmation = false;
        m_bootConfirmationTimeoutTicks = 0;
        this->tlmWrite_JetsonPowerState(JetsonPowerStateID::OFF);
        if (this->isConnected_fpJetsonPowerStateOut_OutputPort(0)) {
          this->fpJetsonPowerStateOut_out(0, JetsonPowerStateID::OFF);
        }

        if (m_pendingPowerCmdRespond) {
          this->cmdResponse_out(m_pendingPowerOpCode, m_pendingPowerCmdSeq,
                                Fw::CmdResponse::OK);
        }

        m_waitingToCutJetsonPower = false;
        m_hasPendingPowerCmd = false;
        m_powerTimeoutTicks = 0;
        m_powerOffDelayTicks = 0;
      }
    }

    // Timeout for Jetson power-state command. Must not tick while an OFF is
    // still deferred, waiting on a boot confirmation -- that phase has its
    // own bound above (m_bootConfirmationTimeoutTicks); this one only starts
    // once the graceful ask has actually been sent.
    if (m_hasPendingPowerCmd && !m_waitingToCutJetsonPower && !m_deferredOffPending) {
      m_powerTimeoutTicks++;

      if (m_powerTimeoutTicks >= CMD_TIMEOUT_TICKS) {
        this->log_WARNING_HI_JETSON_POWER_STATE_TIMEOUT(m_requestedPowerState);

        // If OFF was requested and Jetson never acknowledged, fall safe by
        // cutting power anyway
        if (m_requestedPowerState.e == JetsonPowerStateID::OFF) {
          this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);
          m_currentJetsonPowerState = JetsonPowerStateID::OFF;
          m_jetsonPowerStateKnown = true;
          m_awaitingBootConfirmation = false;
          m_bootConfirmationTimeoutTicks = 0;
          this->tlmWrite_JetsonPowerState(JetsonPowerStateID::OFF);
          if (this->isConnected_fpJetsonPowerStateOut_OutputPort(0)) {
            this->fpJetsonPowerStateOut_out(0, JetsonPowerStateID::OFF);
          }
        }
        

        if (m_pendingPowerCmdRespond) {
          this->cmdResponse_out(m_pendingPowerOpCode, m_pendingPowerCmdSeq,
                                Fw::CmdResponse::OK);
        }

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
    if (m_hasPendingCmd) {
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
      return;
    }

    // reqPwrMode_out() is wired straight through GenericHub into
    // imx_hubComStub.dataIn with no queue or gate in between, exactly like
    // reqJetsonPwrState_out() (see ImxDeployment/Top/topology.fpp and
    // JM-006) -- calling it while the hub link can't be trusted trips
    // Svc::ComStub's never-connected FW_ASSERT and crashes the whole i.MX
    // flight software. Unlike Jetson OFF, there is no hardware-safe
    // fallback action for "set power mode," so this fails fast instead of
    // risking the hub call or deferring indefinitely (JM-013). This also
    // keeps m_hasPendingCmd and m_awaitingBootConfirmation provably
    // mutually exclusive -- see beginJetsonOffSequence().
    if (!this->isJetsonHubLinkTrusted() || !this->isConnected_reqPwrMode_OutputPort(0)) {
      this->log_WARNING_HI_POWER_MODE_REQUEST_REJECTED(mode);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
      return;
    }

    // Store the command identifiers so we can send a deferred response once the
    // Jetson confirms the mode change after rebooting. Do NOT call cmdResponse_out
    // here — the command stays open until currentPwrMode_handler gets a matching
    // report from the Jetson (or the timeout fires in schedIn).
    m_pendingOpCode = opCode;
    m_pendingCmdSeq = cmdSeq;
    m_requestedMode = mode;
    m_hasPendingCmd = true;
    m_modeChangeCmdRespond = true;
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
      const Fw::Success authorization = this->fpJetsonPowerAuthorize_out(0, jetsonState);
      if (authorization != Fw::Success::SUCCESS) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
      }

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
      m_pendingPowerCmdRespond = true;

      this->log_ACTIVITY_HI_JETSON_POWER_STATE_REQUESTED(jetsonState);

      if (jetsonState.e == JetsonPowerStateID::ON) {
        // Jetson is possibly off, so it cannot receive a port call.
        // Power it on directly from the IMX GPIO and wait for the Jetson app
        // to boot and report ON through currentJetsonPwrState_handler.
        printf("Requesting Jetson power state change to ON\n");
        this->gpioSet_out(0, JETSON_POWER_GPIO_ON);
        // Driving GPIO high is not confirmation the Jetson is actually up --
        // m_jetsonPowerStateKnown/m_currentJetsonPowerState (and, downstream,
        // FPManager's own m_jetsonPowerState) are left alone here and only
        // updated by a real report in currentJetsonPwrState_handler: this
        // component's fpJetsonPowerStateOut report to FPManager must never be
        // sent optimistically here, or FPManager's own Jetson-on gating
        // (remoteJetsonCmdIn_handler, jetsonPowerAuthorizeIn_handler) would be
        // defeated during exactly the boot window it exists to protect
        // (JM-010). Only arm the boot-confirmation guard on a genuine
        // off->on transition: a redundant ON to an already-running,
        // already-confirmed Jetson must not re-arm it, since the Jetson
        // won't send a fresh unsolicited report just because it was told to
        // turn on again -- re-arming here would leave a deferred OFF waiting
        // until the bounded timeout, with nothing left to clear it sooner.
        if (m_jetsonPowerStateKnown && m_currentJetsonPowerState.e == JetsonPowerStateID::ON) {
          m_awaitingBootConfirmation = false;
          m_bootConfirmationTimeoutTicks = 0;
        } else {
          m_awaitingBootConfirmation = true;
          m_bootConfirmationTimeoutTicks = 0;
        }
        this->tlmWrite_JetsonPowerState(jetsonState);
        m_hasPendingPowerCmd = false;

        // Report command completed
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);

      } else if (jetsonState.e == JetsonPowerStateID::OFF) {
        printf("Requesting Jetson power state change to OFF\n");
        // beginJetsonOffSequence() decides for itself whether this needs to
        // be deferred (boot in progress, or a mode-change reboot in
        // flight) or can execute now -- the command stays open either way
        // until it (or a later auto-fire/timeout-resume) responds.
        this->beginJetsonOffSequence();

      } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        m_hasPendingPowerCmd = false;
      }

    }

  void JetsonManager::beginJetsonOffSequence() {
    m_requestedPowerState = JetsonPowerStateID::OFF;
    m_hasPendingPowerCmd = true;
    m_powerTimeoutTicks = 0;
    m_waitingToCutJetsonPower = false;
    m_powerOffDelayTicks = 0;

    if (m_awaitingBootConfirmation) {
      // The Jetson was just commanded on and hasn't reported in yet --
      // defer instead of racing the boot. Fires automatically once a real
      // report arrives (currentJetsonPwrState_handler) or the boot window
      // times out (schedIn_handler force-fires it via the direct-cut tail
      // below). See the boot-confirmation guard comment in the header.
      m_deferredOffPending = true;
      this->log_ACTIVITY_HI_JETSON_OFF_DEFERRED_BOOTING();
      return;
    }

    if (m_hasPendingCmd) {
      // A REQUEST_POWER_MODE-triggered reboot is in flight. The Jetson was
      // confirmed alive when that reboot began, but m_currentJetsonPowerState
      // stays ON throughout a mode-change reboot (the Jetson never lost GPIO
      // power, only its OS/hub link is temporarily down) -- so treating
      // "confirmed ON" alone as trustworthy here would risk the graceful
      // hub call (reqJetsonPwrState_out) against a link that's down for the
      // same reboot-related reason as reqPwrMode_out()'s own gate (JM-011).
      // Defer instead; currentPwrMode_handler/schedIn_handler resume this
      // once the mode change confirms or times out.
      m_deferredOffPending = true;
      this->log_ACTIVITY_HI_JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING();
      return;
    }

    // Only take the graceful Jetson-side shutdown path when the hub link is
    // fully trusted -- reqJetsonPwrState_out() is wired straight through
    // GenericHub into imx_hubComStub.dataIn with no queue/gate in between
    // (see reqJetsonPwrState -> imx_hub.serialIn[1] in
    // ImxDeployment/Top/topology.fpp); if the underlying TCP link isn't
    // actually connected, ComStub's "never send while reinitializing"
    // FW_ASSERT trips immediately and takes down the whole i.MX flight
    // software -- the same class of bug already fixed for remoteJetsonCmdIn
    // (see the topology comment there). Reusing isJetsonHubLinkTrusted()
    // here (rather than hand-deriving the same "confirmed on" subset, as
    // this used to) means this can never drift out of sync with
    // REQUEST_POWER_MODE_cmdHandler's own gate -- in particular it now also
    // requires m_hubLinkConnected, closing the same premature-trust race
    // this function used to be exposed to (JM-016). By this point
    // m_awaitingBootConfirmation and m_hasPendingCmd are already known
    // false (the two branches above return early otherwise), so this
    // reduces to exactly "confirmed on and the real hub link is up." An
    // unconfirmed state (the boot-time default, or a state that was never
    // explicitly confirmed) must be treated the same as confirmed-off here:
    // a direct, idempotent GPIO cut, never a hub call.
    m_deferredOffPending = false;
    if (this->isJetsonHubLinkTrusted() && this->isConnected_reqJetsonPwrState_OutputPort(0)) {
      // The commanded OFF path is graceful when the Jetson is confirmed ON:
      // ask the Jetson-side manager to shut down, wait for its OFF report,
      // then cut physical power after JETSON_POWER_OFF_DELAY_TICKS.
      this->reqJetsonPwrState_out(0, JetsonPowerStateID::OFF);
      return;  // stays pending -- currentJetsonPwrState_handler/schedIn_handler finish it
    }

    // Jetson state unknown or confirmed off, or the Jetson-side shutdown
    // port is unavailable: OFF is idempotent and hardware-safe, and never
    // touches the hub transport.
    this->gpioSet_out(0, JETSON_POWER_GPIO_OFF);
    m_currentJetsonPowerState = JetsonPowerStateID::OFF;
    m_jetsonPowerStateKnown = true;
    m_awaitingBootConfirmation = false;
    m_bootConfirmationTimeoutTicks = 0;
    this->tlmWrite_JetsonPowerState(JetsonPowerStateID::OFF);
    if (this->isConnected_fpJetsonPowerStateOut_OutputPort(0)) {
      this->fpJetsonPowerStateOut_out(0, JetsonPowerStateID::OFF);
    }
    if (m_pendingPowerCmdRespond) {
      this->cmdResponse_out(m_pendingPowerOpCode, m_pendingPowerCmdSeq, Fw::CmdResponse::OK);
    }
    m_hasPendingPowerCmd = false;
    m_powerTimeoutTicks = 0;
    m_waitingToCutJetsonPower = false;
    m_powerOffDelayTicks = 0;
  }

  bool JetsonManager::isJetsonHubLinkTrusted() const {
    return m_jetsonPowerStateKnown
        && m_currentJetsonPowerState.e == JetsonPowerStateID::ON
        && !m_awaitingBootConfirmation
        && !m_hasPendingCmd
        && m_hubLinkConnected;
  }

}
