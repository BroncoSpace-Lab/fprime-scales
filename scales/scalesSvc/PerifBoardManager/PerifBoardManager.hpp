// ======================================================================
// \title  PerifBoardManager.hpp
// \author lucal
// \brief  hpp file for PerifBoardManager component implementation class
// ======================================================================

#ifndef scalesSvc_PerifBoardManager_HPP
#define scalesSvc_PerifBoardManager_HPP

#include "scales/scalesSvc/PerifBoardManager/PerifBoardManagerComponentAc.hpp"

namespace scalesSvc {

  class PerifBoardManager :
    public PerifBoardManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct PerifBoardManager object
      PerifBoardManager(
          const char* const compName //!< The component name
      );

      //! Destroy PerifBoardManager object
      ~PerifBoardManager();

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------
      Fw::ParamValid m_isValid = Fw::ParamValid::VALID; //instantiate valid type for paramget
      Fw::On m_powerMode = Fw::On::ON; //!< Power mode of the board, default to ON
      U32 m_startTimeSec;
      Fw::On m_onOff = Fw::On::ON; //instantiate on type for command handler
      bool m_emergencyShutdown = false;

      void emergencyPowerOff_handler(FwIndexType portNum) override;
      //! Handler implementation for run
      //!
      //! input port to run the manager
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command powerOn
      //!
      //! command to set the state of the gpio
      //! Sets the state of the GPIO to high
      void powerOn_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq, //!< The command sequence number
          Fw::On highLow
      ) override;

  };

}

#endif
