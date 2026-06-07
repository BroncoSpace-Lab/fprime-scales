// ======================================================================
// \title  PerifBoardManager.cpp
// \author lucal
// \brief  cpp file for PerifBoardManager component implementation class
// ======================================================================

#include "scales/scalesSvc/PerifBoardManager/PerifBoardManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  PerifBoardManager ::
    PerifBoardManager(const char* const compName) :
      PerifBoardManagerComponentBase(compName)
  {

  }

  PerifBoardManager ::
    ~PerifBoardManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void PerifBoardManager ::
    run_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    switch (m_powerMode) { //switch case for gpio state
      case Fw::On::ON:
          this->gpioSet_out(0, Fw::Logic::HIGH); //keep on
          this->tlmWrite_gpioState(Fw::Logic::HIGH); //emit telemetry
        break;
      case Fw::On::OFF:
          this->tlmWrite_gpioState(Fw::Logic::LOW); //emit telemetry before we lose connection
          this->gpioSet_out(0, Fw::Logic::LOW); //turn off temporarily
          if(this->getTime().getSeconds() - m_startTimeSec >= paramGet_offTimeSec(m_isValid)){ //check if it's time to turn back on
            m_powerMode = Fw::On::ON; //turn to m_powermode to ON to go in ON case.
          }
        break;
      default:
        FW_ASSERT(0, portNum);
    }
    
  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void PerifBoardManager ::
    powerOn_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        Fw::On highLow
    )
  {
    if (highLow == Fw::On::ON){
      m_powerMode = Fw::On::ON;
      m_onOff = Fw::On::ON; //record the state we set for telemetry
      this->log_ACTIVITY_HI_gpioOn(m_onOff); 
    }
    else if (highLow == Fw::On::OFF){
      m_powerMode = Fw::On::OFF; //set power mode to false to go into OFF case in run handler
      m_onOff = Fw::On::OFF; //record the state we set for telemetry
      this->log_ACTIVITY_HI_gpioOn(m_onOff); //log that we are not turning off the board
      m_startTimeSec = this->getTime().getSeconds(); //record the time we are turning off
    }
    else {
      FW_ASSERT(0, opCode);
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
