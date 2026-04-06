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

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct JetsonPowerModeManager object
      JetsonPowerModeManager(
          const char* const compName //!< The component name
      );

      //! Destroy JetsonPowerModeManager object
      ~JetsonPowerModeManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for powerModeRecieve
      //!
      //! Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
      void powerModeRecieve_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::PowerModeID& modeReq
      ) override;

      //! Handler implementation for schedIn
      //!
      //! Port that receives the rate group tick
      void schedIn_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    PRIVATE:

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

    PRIVATE:

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

  };

}

#endif
