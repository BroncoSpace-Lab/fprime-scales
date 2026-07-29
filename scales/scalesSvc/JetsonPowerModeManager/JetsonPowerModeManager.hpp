// ======================================================================
// \title  JetsonPowerModeManager.hpp
// \author dragon-scales
// \brief  hpp file for JetsonPowerModeManager component implementation class
// ======================================================================

#ifndef scalesSvc_JetsonPowerModeManager_HPP
#define scalesSvc_JetsonPowerModeManager_HPP

#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManagerComponentAc.hpp"

namespace scalesSvc {

  class JetsonPowerModeManager :
    public JetsonPowerModeManagerComponentBase
  {

    friend class JetsonPowerModeManagerTester;

    public:

      //! Reads the Jetson's currently active nvpmodel power mode index, or 4
      //! on any error (matches the real get_nvp_mode()'s error convention).
      using PowerModeReader = int (*)();

      //! Runs a shell command and returns its exit status, matching
      //! std::system()'s signature exactly.
      using ShellCommandRunner = int (*)(const char*);

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct JetsonPowerModeManager object
      JetsonPowerModeManager(
          const char* const compName //!< The component name
      );

      //! Destroy JetsonPowerModeManager object
      ~JetsonPowerModeManager();

      //! Override the nvpmodel-query hook. Defaults to the real
      //! get_nvp_mode(), which shells out to `nvpmodel -q`. Test-only seam --
      //! this must never be left pointing at anything that touches real
      //! hardware/shell state outside a unit test.
      void configurePowerModeReader(PowerModeReader reader);

      //! Override the shell-command hook used for `nvpmodel -m <mode>` and
      //! `shutdown -h now`. Defaults to std::system(). Test-only seam -- a
      //! unit test must always override this before exercising any path that
      //! calls it, since the default genuinely runs a shell command
      //! (including a real system shutdown for the OFF path).
      void configureShellRunner(ShellCommandRunner runner);

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for powerModeRecieve
      //!
      //! Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
      void powerModeReceive_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::PowerModeID& modeReq
      ) override;

      void jetsonPowerStateReceive_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::JetsonPowerStateID& stateReq
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

      //! Handler implementation for command SET_POWER_MODE
      //!
      //! Command to set the Jetson power mode
      void SET_POWER_MODE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          scalesSvc::PowerModeID mode //!< Power mode to set (15W, 30W, or 50W)
      ) override;

      //! Handler implementation for command GET_POWER_MODE
      //!
      //! Command to request current power mode
      void GET_POWER_MODE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      void SET_JETSON_POWER_STATE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          scalesSvc::JetsonPowerStateID jetsonState //!< Requested power state (on/off)
      ) override;

    private:

      // ----------------------------------------------------------------------
      // Boot-time reporting state
      //
      // After the Jetson reboots into a new power mode, the IMX PowerManager is
      // waiting for a mode confirmation. schedIn_handler reports the current mode
      // once per boot (on the first tick after initialization) so the IMX can
      // complete its deferred REQUEST_POWER_MODE command without needing a manual
      // GET_POWER_MODE call.
      // ----------------------------------------------------------------------

      //! False until the first schedIn tick has fired and reported the boot mode.
      //! Reset to false each time powerModeRecieve_handler triggers a reboot so
      //! the new boot also reports automatically.
      bool m_modeReported;
      bool m_powerStateReported; //!< False until we have reported the Jetson power state at least once after boot

      PowerModeReader m_powerModeReader; //!< Defaults to the real get_nvp_mode()
      ShellCommandRunner m_shellRunner;  //!< Defaults to std::system()

  };

}

#endif