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

      //! Handler implementation for pingReceive
      //!
      //! Port for receiving ping requests to check if Jetson is awake
      void pingReceive_handler(
          FwIndexType portNum, //!< The port number
          U32 key //!< Value to return to pinger
      ) override;

      //! Handler implementation for powerStateRecieve
      //!
      //! Port for receiving power mode change requests (e.g., 15W, 30W, 50W)
      void powerStateRecieve_handler(
          FwIndexType portNum, //!< The port number
          const scalesSvc::PowerModeID& recieve
      ) override;

      //! Handler implementation for schedIn
      //!
      //! Port that receives the rate group "tick" for ping intervals
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
          scalesSvc::PowerModeID state //!< Power mode to set (15W, 30W, or 50W)
      ) override;

      //! Handler implementation for command GET_POWER_MODE
      //!
      //! Command to request current power mode
      void GET_POWER_MODE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      //! Handler implementation for command CHECK_AWAKE
      //!
      //! Command to check if Jetson is awake
      void CHECK_AWAKE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

  };

}

#endif
