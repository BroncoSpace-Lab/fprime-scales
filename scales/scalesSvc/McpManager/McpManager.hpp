// ======================================================================
// \title  McpManager.hpp
// \author scales
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"

namespace scalesSvc {

  class McpManager :
    public McpManagerComponentBase
  {

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
      
    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for run
      //!
      //! Async scheduler input port to poll temp data from the sensors
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    PRIVATE:

      /* Implementation-specific members */
      scalesSvc::ThermalReading m_thermalReadings[3]; //!< The 3 thermal readings to be logged to telemetry 
      
      /* Determines whether the device has just booted, valid parameter values, and read fail state */
      bool m_justBooted;
      bool m_successfulRead; // Flag to track whether the most recent read was successful, used to determine state machine transitions
      U32  m_startTime = 0;
      Fw::ParamValid m_paramIsValid = Fw::ParamValid::VALID;

      /* Telemetry values for temperature thresholds */
      F32 IDLE_LOW_THR;
      F32 IDLE_HIGH_THR;
      F32 WARN_LOW_THR;
      F32 WARN_HIGH_THR;
      F32 FAULT_LOW_THR;
      F32 FAULT_HIGH_THR;

    PRIVATE:

      // ----------------------------------------------------------------------
      // Implementations for internal state machine actions
      // ----------------------------------------------------------------------

      //! Implementation for action doRead of state machine scalesSvc_ThermalStateMachine
      //!
      //! Read the temp values from the device
      void scalesSvc_ThermalStateMachine_action_doRead(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doEvaluate of state machine scalesSvc_ThermalStateMachine
      //!
      //! Evaluate the temp values against thresholds and update telemetry
      void scalesSvc_ThermalStateMachine_action_doEvaluate(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      //! Implementation for action doReadFail of state machine scalesSvc_ThermalStateMachine
      //!
      //! Log a read failure event
      void scalesSvc_ThermalStateMachine_action_doReadFail(
          SmId smId, //!< The state machine id
          scalesSvc_ThermalStateMachine::Signal signal //!< The signal
      ) override;

      // ----------------------------------------------------------------------
      // Implementations for parameters update
      // ----------------------------------------------------------------------
      void parameterUpdated(FwPrmIdType id) override;  //! Handler implementation for parameter updates, used to update threshold values when parameters are updated

      // ----------------------------------------------------------------------
      // Class helper functions
      // ----------------------------------------------------------------------

      bool readTemp(U8 deviceAddr, F32& temperature); //!< Function to read temperature from a given I2C device address

      scalesSvc::ThermalStates determineTempState(F32 tempCelsius); //!< Function to determine the temperature state (IDLE, WARNING, FAULT) based on the temperature in Celsius


  };

}

#endif
