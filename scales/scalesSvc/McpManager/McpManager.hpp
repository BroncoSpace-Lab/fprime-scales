// ======================================================================
// \title  McpManager.hpp
// \author scales
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"
#include <string>

#define NUM_SENSORS 3

namespace scalesSvc {

  class McpManager :
    public McpManagerComponentBase
  {
    friend class McpManagerTester;

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

    private:

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

    private:

      /* Device Address and Target Register Addresses for MCP9808 */
      static constexpr U8 IMX_TEMP_ADDR = 0x19; //!< I2C address for IMX temperature sensor
      static constexpr U8 PERIPHERAL_TEMP_ADDR = 0x1A; //!< I2C address for peripheral temperature sensor
      static constexpr U8 JETSON_TEMP_ADDR = 0x1B; //!< I2C address for Jetson temperature sensor
      U8 deviceAddrs[3]; //!< Array of device addresses for iterating through sensors

      static constexpr U8 TEMP_REG_ADDR = 0x05; //!< Register address for temperature data

      /* Implementation-specific members */
      scalesSvc::ThermalReading m_thermalReadings[NUM_SENSORS]; //!< The 3 thermal readings to be logged to telemetry

      /* Determines whether the device has just booted, valid parameter values, and read fail state */
      bool m_justBooted;
      bool m_successfulReads[NUM_SENSORS]; // Array to track whether each sensor read was successful, used to determine state machine transitions
      bool m_successfulRead; // Flag to track whether the most recent read was successful, used to determine state machine transitions
      U32  m_startTime = 0;
      Fw::ParamValid m_paramIsValid = Fw::ParamValid::VALID;

      //! Per-sensor bounds currently in effect (indexed by tempLocation:
      //! OBC/PERIF/JETSON). Only ever updated with bounds that pass
      //! thresholdsAreOrdered() -- a rejected PRM_SET leaves this untouched.
      scalesSvc::TempBounds m_activeBounds[NUM_SENSORS];

    private:

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

      bool readTemp(U8 deviceAddr, std::string& location, F32& temperature); //!< Function to read temperature from a given I2C device address

      scalesSvc::ThermalStates determineTempState(F32 tempCelsius, U8 sensorIndex); //!< Function to determine the temperature state (IDLE, WARNING, FAULT) based on the temperature in Celsius, using the given sensor's own active bounds

      //! Returns true if the six bounds are in a sane ascending order:
      //! faultLow <= warnLow <= idleLow <= idleHigh <= warnHigh <= faultHigh
      bool thresholdsAreOrdered(const scalesSvc::TempBounds& bounds) const;

      //! Gate for sensor bounds updates: adopts `candidate` as the sensor's
      //! active bounds (and republishes its telemetry row) only if it passes
      //! thresholdsAreOrdered(); otherwise emits THRESHOLDS_MISCONFIGURED and
      //! leaves the sensor's previous bounds in effect.
      void applyBounds(const char* source, FwIndexType sensorIndex, const scalesSvc::TempBounds& candidate);

      //! Publishes the currently active bounds for one sensor to its
      //! subsystem's telemetry channel.
      void writeBoundsTelemetry(FwIndexType sensorIndex);


  };

}

#endif
