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
      // it does NOT mean the Jetson has actually finished booting. Previously
      // m_jetsonPowerStateKnown/m_currentJetsonPowerState were set to
      // "confirmed ON" optimistically at the moment ON was commanded, so a
      // commanded OFF sent moments later (before the Jetson had booted far
      // enough to be reachable over the hub) took the graceful hub-routed
      // path against a link that may not exist yet, and its m_hasPendingPowerCmd
      // bookkeeping could stay BUSY for the full CMD_TIMEOUT_TICKS window --
      // during which every REQUEST_JETSON_POWER_STATE (on or off) is rejected
      // BUSY. m_jetsonPowerStateKnown is now set ONLY by a real report
      // received from the Jetson (see currentJetsonPwrState_handler); this
      // separate flag tracks "commanded ON, first report not seen yet" and
      // rejects a commanded OFF outright while it's true, instead of letting
      // it race the boot.
      // ----------------------------------------------------------------------

      bool m_awaitingBootConfirmation; //!< True after ON is commanded until a real report arrives or the boot window times out
      U32 m_bootConfirmationTimeoutTicks; //!< Ticks elapsed since ON was commanded, bounds m_awaitingBootConfirmation

  };

}

#endif
