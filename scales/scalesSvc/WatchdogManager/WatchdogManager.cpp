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
    switch (this->m_wdStatus.e) {
      // First case will only run once
      case Fw::Enabled::DISABLED:            // On first cycle of program m_wdState is initially set to DISABLED
        this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::NOT_PETTING); //Emit telemetry that the watchdog is not being pet
        //Enable the Watchdog
        this->m_wdStatus = Fw::Enabled::ENABLED;                //Set the watchdog to enabled to pet forever
        this->log_ACTIVITY_HI_WatchdogState(Fw::On::ON);  //Since the board is coming back to ON, let GDS know
        break;

      case Fw::Enabled::ENABLED: //Loop forever in here petting the watchdog indefinitely at the desired parameter time  

        this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::PETTING); //Emit telemetry that the watchdog is being pet
        this->gpioWatchDog_out(0, Fw::Logic::HIGH); //Pet the Watchdog

        if(this->getTime().getSeconds() - m_startTimeSec >= paramGet_watchdogPetInterval(m_isValid)){ // Check for it to be time to stop petting the watchdog
          this->tlmWrite_WatchdogPet(scalesSvc::WatchdogStates::NOT_PETTING);
          this->gpioWatchDog_out(0, Fw::Logic::LOW); //Stop petting the Watchdog
          //Start over again
          }
        break;
    }
    
    //Pet watch dog @ interval of parameter
    //Emit On when on, off when off
    //Only emit one event at the start, then emit on off forever
  }

}
