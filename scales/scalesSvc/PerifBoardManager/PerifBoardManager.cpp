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
          this->log_ACTIVITY_HI_gpioOn(m_onOff); //log event
          this->tlmWrite_gpioState(Fw::Logic::HIGH); //emit telemetry
        break;
      case Fw::On::OFF:
          this->tlmWrite_gpioState(Fw::Logic::LOW); //emit telemetry before we lose connection
          m_startTimeSec = this->getTime().getSeconds(); //record the time we turned off
          this->gpioSet_out(0, Fw::Logic::LOW); //turn off temporarily
          
          if(this->getTime().getSeconds() - m_startTimeSec >= paramGet_offTimeSec(m_isValid)){ //check if it's time to turn back on
            m_powerMode = Fw::On::ON;
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
      gpioSet_out(0, Fw::Logic::HIGH);
    }
    else if (highLow == Fw::On::OFF){
      m_powerMode = Fw::On::OFF;
      gpioSet_out(0, Fw::Logic::LOW);
      m_startTimeSec = this->getTime().getSeconds();
    }
    else {
      FW_ASSERT(0, opCode);
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
