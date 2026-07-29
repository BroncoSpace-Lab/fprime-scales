// ======================================================================
// \title  PerifBoardManagerTester.cpp
// \author luquito
// \brief  cpp file for PerifBoardManager component test harness implementation class
// ======================================================================

#include "PerifBoardManagerTester.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  PerifBoardManagerTester ::
    PerifBoardManagerTester() :
      PerifBoardManagerGTestBase("PerifBoardManagerTester", PerifBoardManagerTester::MAX_HISTORY_SIZE),
      component("PerifBoardManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  PerifBoardManagerTester ::
    ~PerifBoardManagerTester()
  {
    this->component.deinit();
  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void PerifBoardManagerTester ::
    testPerifBoardManager()
  {
    // Load Parameters
    this->component.loadParameters(); //load the component parameters
    
    //Cycle 1: Check default state of the component, run two internal cycles
    this->invoke_to_run(0, 0); //invoke the port run
    this->component.doDispatch();  // Trigger execution of async port
    ASSERT_from_gpioSet_SIZE(1); //check an output from gpioSet was set
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH); //check GPIO was set to high
    ASSERT_TLM_gpioState_SIZE(1); //check telemetry was emitted when GPIO was set on
    ASSERT_TLM_gpioState(0, Fw::Logic::HIGH); //check telemetry value is correct
   
    this->invoke_to_run(0, 0); //invoke the port run
    this->component.doDispatch();  // Trigger execution of async port
    ASSERT_from_gpioSet_SIZE(2); //check an output from gpioSet was set
    ASSERT_from_gpioSet(1, Fw::Logic::HIGH); //check GPIO was set to high
    ASSERT_TLM_gpioState_SIZE(2); //check telemetry was emitted when GPIO was set on
    ASSERT_TLM_gpioState(1, Fw::Logic::HIGH); //check telemetry value is correct
    // Cycle 1 complete, component was allowed to run twice by default

    this->clearHistory(); //resets history of ports for new unit test cycle

/////////////////////////////////////////////////////////////////
    
// Cycle 2: Send Off command, check gpioOn event, confirm response, check off state until t=2, which will flip back to on. t > 2 GPIO comes back on
    // t = 0, send command to turn off the board, check that event was emitted for gpioOn with correct value, and command response was emitted
    this->setTestTime(Fw::Time(0, 0)); //set time to 0 seconds
    this->sendCmd_powerOn(0, 0, Fw::On::OFF); //send command to turn off the board
    this->component.doDispatch(); // Trigger execution of command handler
    ASSERT_EVENTS_gpioOn_SIZE(1); //check event was emitted for gpioOn
    ASSERT_EVENTS_gpioOn(0, Fw::On::OFF); //check event value is set to OFF
    ASSERT_CMD_RESPONSE_SIZE(1); //check command response was emitted
    // Command Handler done, now invoke run handler to enter switch case for OFF

    //Cycle 2: Continued - check that board turns off for 2 seconds and then goes to ON
    this->invoke_to_run(0, 0); //invoke the port run
    this->component.doDispatch();  // Trigger execution of async port
    // Now entering switch case for Fw::On::OFF in run handler
    ASSERT_TLM_gpioState_SIZE(1); //check telemetry was emitted when GPIO was set off
    ASSERT_TLM_gpioState(0, Fw::Logic::LOW); //check telemetry value is correct for off state
    ASSERT_from_gpioSet_SIZE(1); //check an output from gpioSet was set to turn off
    ASSERT_from_gpioSet(0, Fw::Logic::LOW); //check GPIO value is correct for off state
    
    // t = 2, still outputs LOW,
    this->setTestTime(Fw::Time(2, 0)); //2 seconds pass, switching from low to high on next cycle
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);

    // next cycle sees m_powerMode == ON -> HIGH
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(3);
    ASSERT_from_gpioSet(2, Fw::Logic::HIGH);

    // another cycle m_powerMode should stay ON
    // next cycle sees m_powerMode == ON -> HIGH
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(4);
    ASSERT_from_gpioSet(3, Fw::Logic::HIGH);

    // New Cycle. When CMD ON is sent, we check that we get a command response

    this->clearHistory(); //resets history of ports for new unit test cycle
    this->sendCmd_powerOn(0, 0, Fw::On::ON); //send command to turn the board back on
    this->component.doDispatch();
    //check inside the if statement
    ASSERT_EVENTS_gpioOn_SIZE(1); //check event was emitted for gpioOn
    ASSERT_EVENTS_gpioOn(0, Fw::On::ON); //check event value is set to ON
    ASSERT_CMD_RESPONSE_SIZE(1); //check command response was emitted

  }

  void PerifBoardManagerTester ::
    emergencyShutdownLatch()
  {
    this->component.loadParameters();
    this->clearHistory();

    // Board is in its default ON state, gpio held high.
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(1);
    ASSERT_from_gpioSet(0, Fw::Logic::HIGH);

    // FPManager asserts the emergency power-off signal (sync port, no dispatch needed):
    // GPIO is forced low immediately, independent of the run cycle.
    this->invoke_to_emergencyPowerOff(0);
    ASSERT_from_gpioSet_SIZE(2);
    ASSERT_from_gpioSet(1, Fw::Logic::LOW);
    ASSERT_TLM_gpioState_SIZE(2);
    ASSERT_TLM_gpioState(1, Fw::Logic::LOW);

    // The latch holds across further run cycles even though m_powerMode is
    // still ON underneath -- run_handler short-circuits before the switch.
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(3);
    ASSERT_from_gpioSet(2, Fw::Logic::LOW);

    // A powerOn(ON) command still "succeeds" (event + response), but the
    // latch is never cleared, so the GPIO stays low regardless.
    this->sendCmd_powerOn(0, 0, Fw::On::ON);
    this->component.doDispatch();
    ASSERT_EVENTS_gpioOn_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(1);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet_SIZE(4);
    ASSERT_from_gpioSet(3, Fw::Logic::LOW);
  }

  void PerifBoardManagerTester ::
    configurableOffInterval()
  {
    this->clearHistory();

    // Configure a non-default interval (5s instead of the 2s default) in the
    // tester's mock parameter store, then loadParameters() so the component
    // pulls it into its cached m_offTimeSec -- paramGet_offTimeSec() reads
    // that cache, not the port live, so the mock must be set first.
    this->paramSet_offTimeSec(5, Fw::ParamValid::VALID);
    this->component.loadParameters();

    this->setTestTime(Fw::Time(0, 0));
    this->sendCmd_powerOn(0, 0, Fw::On::OFF);
    this->component.doDispatch();
    this->invoke_to_run(0, 0);
    this->component.doDispatch();

    // Before the configured interval elapses, gpio stays low.
    this->setTestTime(Fw::Time(4, 0));
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet(this->fromPortHistory_gpioSet->size() - 1, Fw::Logic::LOW);

    // Once the configured 5s interval elapses, the board returns to ON.
    this->setTestTime(Fw::Time(5, 0));
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet(this->fromPortHistory_gpioSet->size() - 1, Fw::Logic::LOW);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_from_gpioSet(this->fromPortHistory_gpioSet->size() - 1, Fw::Logic::HIGH);
  }
}
