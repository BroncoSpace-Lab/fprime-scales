// ======================================================================
// \title  WatchdogManagerTester.cpp
// \author lucal
// \brief  cpp file for WatchdogManager component test harness implementation class
// ======================================================================

#include "WatchdogManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  WatchdogManagerTester ::
    WatchdogManagerTester() :
      WatchdogManagerGTestBase("WatchdogManagerTester", WatchdogManagerTester::MAX_HISTORY_SIZE),
      component("WatchdogManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  WatchdogManagerTester ::
    ~WatchdogManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Testss
  // ----------------------------------------------------------------------

  void WatchdogManagerTester :: WatchdogTester()
  {
    U32 wdTime = 1;
    const U32 u_nowSec = 0;
    const U32 u_nowSec_1 = 1;
    const U32 u_nowSec_2 = 2;
    const U32 u_nowSec_3 = 3;
    this->clearHistory();

    
    ASSERT_EVENTS_SIZE(0); //starting with no events
    ASSERT_EQ(Fw::Enabled::DISABLED, this->component.m_wdStatus); // Check that m_wdStatus is disabled before the start of the cycle
    
    //Cycle 1: Iterate through when m_wdStatus is DISABLED
    this->invoke_to_run(0,0); //Invoke the run handler
    this->component.doDispatch();

    ASSERT_EQ(this->component.m_intervalSec, wdTime); //Check that the parameter value is 1
  
    ASSERT_TLM_WatchdogPet_SIZE(1); //Check that we have emitted Not petting telemetry 
    ASSERT_TLM_WatchdogPet(0,scalesSvc::WatchdogStates::NOT_PETTING); //Confirm that value
    ASSERT_from_gpioWatchDog(0, Fw::Logic::LOW);
    ASSERT_EQ(this->component.m_isPetting, false); //Not petting yet
    ASSERT_EQ(u_nowSec, this->component.m_startTimeSec); //Check that we are still at time zero
    ASSERT_EQ(Fw::Enabled::ENABLED, this->component.m_wdStatus); //Check that we are now enabled
    ASSERT_EVENTS_SIZE(1); //Check that we have emitted first ON event
    ASSERT_EVENTS_WatchdogState(0, Fw::On::ON); //Confirm the event value

    //Cycle 2: Iterate through when m_wdStatus is ENABLED
    this->setTestTime(Fw::Time(u_nowSec,0)); //start at 0 time
    this->invoke_to_run(0,0); //Invoke the run handler
    this->component.doDispatch();
    //Since m_isPetting was false and now m_wdStatus is ENABLED, go into ENABLED case
    //Immediately skips the if(nowSec-this->m_startTimeSec...) statement because it hasnt been long enough
    ASSERT_EQ(u_nowSec, this->component.m_startTimeSec); //Check that we set the start time to zero

    this->setTestTime(Fw::Time(u_nowSec_2,0)); //Advance to 2 seconds to trigger the if(nowSec-this->m_startTime....) statement
    this->invoke_to_run(0,0); //Invoke the run handler
    this->component.doDispatch();

    // enter the else in if(this->m_isPetting)
    ASSERT_EQ(Fw::Enabled::ENABLED, this->component.m_wdStatus); //Verify we are in enabled state
    ASSERT_TLM_WatchdogPet(1, scalesSvc::WatchdogStates::PETTING);
    ASSERT_from_gpioWatchDog(1, Fw::Logic::HIGH);
    ASSERT_EQ(true, this->component.m_isPetting);
    ASSERT_EQ(u_nowSec_2, this->component.m_startTimeSec); //verify we are still at time 2
    
    //Advance the time by one second
    this->setTestTime(Fw::Time(u_nowSec_3,0));
    this->invoke_to_run(0,0); //Invoke the run handler
    this->component.doDispatch();
    ASSERT_TLM_WatchdogPet(2, scalesSvc::WatchdogStates::NOT_PETTING);
    ASSERT_from_gpioWatchDog(2, Fw::Logic::LOW);
    ASSERT_EQ(false, this->component.m_isPetting);


  

  } 

}