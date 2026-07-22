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
      U32 m_powerTimeoutTicks;  //!< Ticks elapsed since the power state change request was sent
      bool m_waitingToCutJetsonPower; //!< True if we've sent a shutdown command and are waiting to cut power after a delay
      U32 m_powerOffDelayTicks; //!< Ticks elapsed since sending the shutdown command, used to delay cutting power to allow for graceful shutdown
      bool m_pendingPowerCmdRespond; //!< Whether the pending power request has a command sequence to complete

  };

}

#endif
