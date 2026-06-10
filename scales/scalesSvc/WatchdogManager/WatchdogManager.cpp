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
  const U32 nowSec = this->getTime().getSeconds();
  const U32 intervalSec = this->paramGet_watchdogPetInterval(this->m_isValid);

  switch (this->m_wdStatus.e) {
    case Fw::Enabled::DISABLED:
      this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::NOT_PETTING);
      this->gpioWatchDog_out(0, Fw::Logic::LOW);

      this->m_isPetting = false;
      this->m_startTimeSec = nowSec;

      this->m_wdStatus = Fw::Enabled::ENABLED;
      this->log_ACTIVITY_HI_WatchdogState(Fw::On::ON);
      break;

    case Fw::Enabled::ENABLED:
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

        this->m_startTimeSec = nowSec;
      }
      break;
  }
}

}
