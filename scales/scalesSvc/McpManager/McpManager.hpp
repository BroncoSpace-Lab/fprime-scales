// ======================================================================
// \title  McpManager.hpp
// \author bidat
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"

namespace scalesSvc {

  class McpManager : public McpManagerComponentBase {

    // Device Address and Target Register Addresses for MCP9808
    public:
    
      static constexpr U8 IMX_TEMP_ADDR = 0x19; //!< I2C address for IMX temperature sensor
      static constexpr U8 PERIPHERAL_TEMP_ADDR = 0x1A; //!< I2C address for peripheral temperature sensor
      static constexpr U8 JETSON_TEMP_ADDR = 0x1B; //!< I2C address for Jetson temperature sensor
      U8 deviceAddrs[3]; //!< Array of device addresses for iterating through sensors
      
      static constexpr U8 TEMP_REG_ADDR = 0x05; //!< Register address for temperature data    

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct McpManager object
      McpManager(
          const char* const compName //!< The component name
      );

      //! Destroy McpManager object
      ~McpManager();

      F32 readTemp(U8 deviceAddr); //!< Function to read temperature from a given I2C device address

      U8 determineTempState(F32 tempCelsius); //!< Function to determine the temperature state (IDLE, WARNING, FAULT) based on the temperature in Celsius

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for run input port
      //!
      //! Async scheduler input port to poll temp data from the sensors 
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    PRIVATE:
      /* Implementation-specific members */
      scalesSvc::ThermalReading m_thermalReadings[3]; //!< The 3 thermal readings to be logged to telemetry 
      scalesSvc::ThermalReading imx_thermalReadings;
      scalesSvc::ThermalReading peripheral_thermalReadings;
      scalesSvc::ThermalReading jetson_thermalReadings;

      /* Determines whether the device has just booted */
      bool m_justBooted;
      U8 m_currentState;

    PRIVATE:
      // ----------------------------------------------------------------------
      // Implementations for internal state machine actions
      // ----------------------------------------------------------------------

      //! Implementation for action doReset of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doReset(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doReadTemp of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doReadTemp(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doIdle of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doIdle(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doWarning of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doWarning(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doFault of state machine scalesSvc_ThermalStateMachine
      void scalesSvc_ThermalStateMachine_action_doFault(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

  };

}

#endif
