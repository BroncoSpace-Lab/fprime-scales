// ======================================================================
// \title  JetsonManager.hpp
// \author lucal
// \brief  hpp file for JetsonManager component implementation class
// ======================================================================

#ifndef scalesSvc_JetsonManager_HPP
#define scalesSvc_JetsonManager_HPP

#include "scales/scalesSvc/JetsonManager/JetsonManagerComponentAc.hpp"

namespace scalesSvc {

  class JetsonManager :
    public JetsonManagerComponentBase
  {

    friend class JetsonManagerTester;

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct JetsonManager object
      JetsonManager(
          const char* const compName //!< The component name
      );

      //! Destroy JetsonManager object
      ~JetsonManager();

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for currentJetsonPwrState
      //!
      //! Port for receiving current Jetson power state from JetsonPowerModeManager
      void currentJetsonPwrState_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::JetsonPowerStateID& stateNow
      ) override;

      void fpJetsonPowerRequestIn_handler(
          FwIndexType portNum,
          const scalesSvc::JetsonPowerStateID& stateReq
      ) override;

      //! Handler implementation for currentPwrMode
      //!
      //! Port for receiving current power mode from JetsonPowerModeManager
      void currentPwrMode_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::PowerModeID& modeNow
      ) override;

      //! Handler implementation for schedIn
      //!
      //! Port that receives the rate group tick
      void schedIn_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command TODO
     

      //! Handler implementation for command REQUEST_POWER_MODE
      //!
      //! Command to request a power mode change on the Jetson
      void REQUEST_POWER_MODE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          scalesSvc::PowerModeID mode //!< Requested power mode
      ) override;

      //! Handler implementation for command REQUEST_JETSON_POWER_STATE
      //!
      //! Command to request a power state change (on/off) for the Jetson
      void REQUEST_JETSON_POWER_STATE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          scalesSvc::JetsonPowerStateID jetsonState //!< Requested power state (on/off)
      ) override;

      //! Shared graceful-vs-direct OFF sequence, used by
      //! REQUEST_JETSON_POWER_STATE(OFF), fpJetsonPowerRequestIn(OFF), and the
      //! deferred-OFF auto-fire/force-fire paths (currentJetsonPwrState_handler,
      //! currentPwrMode_handler, schedIn_handler). Re-evaluates
      //! m_jetsonPowerStateKnown/m_currentJetsonPowerState itself, so it is
      //! correct no matter which of those callers triggers it.
      void beginJetsonOffSequence();

      //! True only when it is currently safe to call a hub-routed output
      //! port (reqPwrMode_out/reqJetsonPwrState_out): the Jetson is
      //! confirmed ON by a real report, no ON command is still awaiting its
      //! first boot confirmation, and no REQUEST_POWER_MODE-triggered
      //! reboot is in flight. False for any other reason -- confirmed off,
      //! never confirmed, booting, or mid a pending mode-change reboot --
      //! since the hub TCP link's liveness cannot be trusted and
      //! Svc::ComStub's never-connected FW_ASSERT would trip (JM-006's
      //! crash class, which reqPwrMode_out() is equally exposed to as
      //! reqJetsonPwrState_out() -- see JM-011).
      bool isJetsonHubLinkTrusted() const;

      private:

       // ----------------------------------------------------------------------
      // Deferred command state for REQUEST_POWER_MODE
      //
      // REQUEST_POWER_MODE cannot complete immediately because the Jetson must
      // reboot to apply the new nvpmodel power mode. Instead, the command is
      // held here until the Jetson reconnects and reports its current mode via
      // the currentPwrMode port. If the reported mode matches m_requestedMode,
      // the stored opCode/cmdSeq are used to send the deferred OK response.
      // If the Jetson does not confirm within CMD_TIMEOUT_TICKS, schedIn sends
      // an EXECUTION_ERROR response so the command doesn't hang forever.
      // ----------------------------------------------------------------------

      bool m_hasPendingCmd;         //!< True while waiting for Jetson confirmation
      FwOpcodeType m_pendingOpCode; //!< Opcode of the in-flight REQUEST_POWER_MODE
      U32 m_pendingCmdSeq;          //!< Sequence number of the in-flight command
      PowerModeID m_requestedMode;  //!< Mode we asked the Jetson to switch to
      U32 m_timeoutTicks;           //!< Ticks elapsed since the request was sent
      Fw::ParamValid m_paramIsValid = Fw::ParamValid::VALID;

      //! How many schedIn ticks to wait before timing out (120 ticks ≈ 2 min at 1 Hz)
      static const U32 CMD_TIMEOUT_TICKS = 120;

      bool m_hasPendingPowerCmd; //!< True while waiting for Jetson power state change confirmation
      FwOpcodeType m_pendingPowerOpCode; //!< Opcode of the in-flight REQUEST_JETSON_POWER_STATE
      U32 m_pendingPowerCmdSeq; //!< Sequence number of the in-flight REQUEST_JETSON_POWER_STATE command
      scalesSvc::JetsonPowerStateID m_requestedPowerState; //!< Power state we asked the Jetson to switch to
      scalesSvc::JetsonPowerStateID m_currentJetsonPowerState; //!< Last known (or assumed) Jetson power state
      //! True once m_currentJetsonPowerState reflects an actual confirmed
      //! report from the Jetson or a GPIO action JetsonManager itself took --
      //! i.e. not just the boot-time default. Until this is true, OFF
      //! requests must not treat the Jetson as "already off": at i.MX boot
      //! the Jetson may in fact be alive and running (e.g. i.MX rebooted
      //! while the Jetson stayed up), and treating an unconfirmed state as
      //! OFF would skip the graceful reqJetsonPwrState_out() shutdown request
      //! and cut GPIO power to a live Linux system directly.
      bool m_jetsonPowerStateKnown;
      U32 m_powerTimeoutTicks;  //!< Ticks elapsed since the power state change request was sent
      bool m_waitingToCutJetsonPower; //!< True if we've sent a shutdown command and are waiting to cut power after a delay
      U32 m_powerOffDelayTicks; //!< Ticks elapsed since sending the shutdown command, used to delay cutting power to allow for graceful shutdown
      bool m_pendingPowerCmdRespond; //!< Whether the pending power request has a command sequence to complete

      // ----------------------------------------------------------------------
      // Boot-confirmation guard
      //
      // ON completes synchronously (GPIO driven high, immediate OK response) --
      // it does NOT mean the Jetson has actually finished booting.
      // m_jetsonPowerStateKnown/m_currentJetsonPowerState are set ONLY by a
      // real report received from the Jetson (see currentJetsonPwrState_handler)
      // or by a GPIO action JetsonManager itself took -- never optimistically
      // by the ON command path. m_awaitingBootConfirmation separately tracks
      // "commanded ON, first report not seen yet".
      //
      // A commanded OFF that arrives while m_awaitingBootConfirmation is true
      // is NOT rejected: it is deferred (m_deferredOffPending) and
      // automatically fired -- via beginJetsonOffSequence(), which then takes
      // the normal graceful hub-routed path -- the instant a real report
      // arrives (currentJetsonPwrState_handler) or the boot window times out
      // (schedIn_handler force-fires it via a direct GPIO cut, same fail-safe
      // philosophy as the existing post-ack timeout fallback below).
      //
      // m_hasPendingCmd (a REQUEST_POWER_MODE-triggered reboot in flight) is
      // the same kind of "hub link cannot be trusted right now" condition as
      // m_awaitingBootConfirmation, even though m_currentJetsonPowerState
      // stays ON throughout (the Jetson never lost GPIO power -- only its
      // OS/hub link is temporarily down while it reboots to apply the new
      // mode). beginJetsonOffSequence() defers on this too, reusing
      // m_deferredOffPending; see isJetsonHubLinkTrusted() and JM-011.
      // ----------------------------------------------------------------------

      bool m_awaitingBootConfirmation; //!< True after a genuine off->on ON command until a real report arrives or the boot window times out
      U32 m_bootConfirmationTimeoutTicks; //!< Ticks elapsed since ON was commanded, bounds m_awaitingBootConfirmation
      bool m_deferredOffPending; //!< True while an OFF request is being held pending a boot confirmation (see beginJetsonOffSequence())

  };

}

#endif
