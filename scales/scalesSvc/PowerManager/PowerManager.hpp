// ======================================================================
// \title  PowerManager.hpp
// \author dragon-scales
// \brief  hpp file for PowerManager component implementation class
// ======================================================================

#ifndef scalesSvc_PowerManager_HPP
#define scalesSvc_PowerManager_HPP

#include "scales/scalesSvc/PowerManager/PowerManagerComponentAc.hpp"

namespace scalesSvc {

  class PowerManager :
    public PowerManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct PowerManager object
      PowerManager(
          const char* const compName //!< The component name
      );

      //! Destroy PowerManager object
      ~PowerManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

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

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command REQUEST_POWER_MODE
      //!
      //! Command to request a power mode change on the Jetson
      void REQUEST_POWER_MODE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          scalesSvc::PowerModeID mode //!< Requested power mode
      ) override;

  };

}

#endif
