// ======================================================================
// \title  WatchdogManager.cpp
// \author lucal
// \brief  cpp file for WatchdogManager component implementation class
// ======================================================================

#include "scales/scalesSvc/WatchdogManager/WatchdogManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  WatchdogManager ::
    WatchdogManager(const char* const compName) :
      WatchdogManagerComponentBase(compName)
  {

  }

  WatchdogManager ::
    ~WatchdogManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

   void WatchdogManager ::
  run_handler(
      FwIndexType portNum,
      U32 context
  )
{
  const U32 nowSec = this->getTime().getSeconds(); //Define curret time
  const U32 intervalSec = this->paramGet_watchdogPetInterval(this->m_isValid); //Parameter used for the pet interval

  switch (this->m_wdStatus.e){ /*take the actual enum valu */ 
    case Fw::Enabled::DISABLED: //on first cycle ever, wd is disabled
      this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::NOT_PETTING); //let us know
      this->gpioWatchDog_out(0, Fw::Logic::LOW); //make sure the watchdog is not petting

      this->m_isPetting = false;  //petting flag
      this->m_startTimeSec = nowSec; //start time for the petting interval starts now

      this->m_wdStatus = Fw::Enabled::ENABLED;  //enable the watchdog
      this->log_ACTIVITY_HI_WatchdogState(Fw::On::ON); //log the state change
      break;

    case Fw::Enabled::ENABLED: //now that that the watchdog is enabled, check if we have been petting long enough, if not, PET!
      if (nowSec - this->m_startTimeSec >= intervalSec) {
        if (this->m_isPetting) {
          this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::NOT_PETTING);
          this->gpioWatchDog_out(0, Fw::Logic::LOW);
          this->m_isPetting = false;
        } else {
          this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::PETTING);
          this->gpioWatchDog_out(0, Fw::Logic::HIGH);
          this->m_isPetting = true;
        }

        this->m_startTimeSec = nowSec; //reset the start time for the next petting interval
      }
      break;
  }
}

}
