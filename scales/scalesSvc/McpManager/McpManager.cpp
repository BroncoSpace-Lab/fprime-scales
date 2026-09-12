// ======================================================================
// \title  McpManager.cpp
// \author scales
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"
#include <unordered_map> // Required header for hashmap

F32 convertRawTemp(U8 *rawData); // Forward decleration

enum tempLocation{
  OBC = 0,
  PERIF = 1,
  JETSON = 2
};

std::unordered_map<U8, std::string> indexToLocation = {
    {OBC, "OBC"},
    {PERIF, "PERIPHERAL"},
    {JETSON, "JETSON"}
};

namespace {
  //! Compiled-in safe fallback, matching the FPP parameter defaults. Used as
  //! the initial active bounds for every sensor so that even a rejected
  //! boot-time PRM_SET (e.g. a bad value saved to non-volatile storage in a
  //! previous session) still leaves the sensor with a sane configuration.
  const scalesSvc::TempBounds SAFE_DEFAULT_BOUNDS(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F);
}

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  McpManager :: McpManager(const char* const compName) :
    McpManagerComponentBase(compName),
    m_successfulRead(true), // Initialize successful read flag to true as default
    m_successfulReads{true, true, true}, // Initialize successful reads array to true for all sensors
    m_justBooted(true),
    m_activeBounds{SAFE_DEFAULT_BOUNDS, SAFE_DEFAULT_BOUNDS, SAFE_DEFAULT_BOUNDS}
  {
    deviceAddrs[OBC] = IMX_TEMP_ADDR;
    deviceAddrs[PERIF] = PERIPHERAL_TEMP_ADDR;
    deviceAddrs[JETSON] = JETSON_TEMP_ADDR;
  }

  McpManager :: ~McpManager() {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void McpManager :: run_handler(FwIndexType portNum, U32 context)
  {
    this->mcp_thermalStateMachine_sendSignal_tick(); // Trigger state machine tick
    this->dispatchCurrentMessages(); // Dispatch any messages that may have been queued during the tick
  }

  // ----------------------------------------------------------------------
  // Implementations for internal state machine actions
  // ----------------------------------------------------------------------

  void McpManager :: scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal
    )
  {
    // If the device just booted, set up the parameters. If not, then start reading temp
    if (m_justBooted) {
      m_justBooted = false;
      m_startTime = this->getTime().getSeconds(); // Record the start time at boot to track uptime in telemetry
      printf("Device just booted. Setting up parameters...\n");
      // Gate each sensor's saved/default bounds through the same validity
      // check as a live PRM_SET, and publish the resulting active bounds.
      this->applyBounds("IMX", OBC, this->paramGet_MCP_IMX_BOUNDS(m_paramIsValid));
      this->applyBounds("PERIPHERAL", PERIF, this->paramGet_MCP_PERIPHERAL_BOUNDS(m_paramIsValid));
      this->applyBounds("JETSON", JETSON, this->paramGet_MCP_JETSON_BOUNDS(m_paramIsValid));
    } else {
        // Read temp data from sensors and log to telemetry
        for (int i = 0; i < NUM_SENSORS; i++){

          F32 tempCelsius;
          if (this->readTemp(deviceAddrs[i], indexToLocation[i], tempCelsius)){
            this->m_thermalReadings[i].set_temperature(tempCelsius);
          } else {
            this->m_thermalReadings[i].set_temperature(0.0f); // Set temp to 0.0f if the read failed
            this->m_thermalReadings[i].set_tempState(scalesSvc::ThermalStates::FAULT); // Set temp state to FAULT if the read failed
            m_successfulReads[i] = false; // Set successful read flag to false if the read failed
          }

          this->m_thermalReadings[i].set_sensorId(i + 1);
          this->m_thermalReadings[i].set_timestamp(this->getTime().getSeconds()- m_startTime); // Log uptime in seconds as the timestamp for telemetry
          this->m_thermalReadings[i].set_location(Fw::String(indexToLocation[i].c_str()));
        }
        m_successfulRead = m_successfulReads[OBC] && m_successfulReads[PERIF] && m_successfulReads[JETSON]; // Update the overall successful read flag based on individual sensor reads

        this->mcp_thermalStateMachine_sendSignal_success(); // Transition to next state to evaluate the readings
    }
  }


  void McpManager :: scalesSvc_ThermalStateMachine_action_doEvaluate(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // printf("Evaluating thermal readings against thresholds...\n");
    for (int i = 0; i < NUM_SENSORS; i++){

      if(m_successfulReads[i]){ // Only evaluate if this sensor readding was successful, otherwise the temp state is already set to FAULT
        scalesSvc::ThermalStates tempState = this->determineTempState(this->m_thermalReadings[i].get_temperature(), i);
        this->m_thermalReadings[i].set_tempState(tempState); // Set the temp state in the reading struct to log to telemetry
      }

      switch(i){
        case OBC:
          this->tlmWrite_IMX_TEMP(m_thermalReadings[OBC]);
          break;
        case PERIF:
          this->tlmWrite_PERIPHERAL_TEMP(m_thermalReadings[PERIF]);
          break;
        case JETSON:
          this->tlmWrite_JETSON_TEMP(m_thermalReadings[JETSON]);
          break;
          default:
          printf("Warning: Unrecognized sensor index %d. No telemetry was logged for this sensor.\n", i);
          break;
      }
      // Republish this sensor's active bounds every cycle (not just on
      // boot/change) so a GDS session that connects late still sees them
      // on the next tick instead of waiting for another PRM_SET.
      this->writeBoundsTelemetry(i);
      this->thermalReadingOut_out(0, this->m_thermalReadings[i]);
    }

    // Send thermal readings to DataProducer
    this->mcpThermalReadOut_out(0, m_thermalReadings[OBC], m_thermalReadings[PERIF], m_thermalReadings[JETSON]);

    if(m_successfulRead){
      this->mcp_thermalStateMachine_sendSignal_success(); // Transition back to initial state to read temp again on next tick
    } else{
      this->mcp_thermalStateMachine_sendSignal_fail(); // Transition to read failure state to log the failure event
    }

  }

  void McpManager :: scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Failed to read from sensor. Logging failure event...\n");
    this->log_WARNING_HI_FAIL_TO_READ_TEMP(); // Log event for read failure
    m_successfulRead = true; // Reset successful read flag to true to try reading again on next tick
    for (int i = 0; i < NUM_SENSORS; i++){
      m_successfulReads[i] = true; // Reset successful reads array to true for all sensors to try reading again on next tick
    }
    this->mcp_thermalStateMachine_sendSignal_success(); // Transition back to initial state to try reading again on next tick
  }


  // ----------------------------------------------------------------------
  // Handler implementations for parameters update
  // ----------------------------------------------------------------------

  void McpManager :: parameterUpdated(FwPrmIdType id){
    printf("Parameter with ID 0x%X has been updated. Re-validating bounds...\n", id);

    switch(id){
      case PARAMID_MCP_IMX_BOUNDS:
        this->applyBounds("IMX", OBC, this->paramGet_MCP_IMX_BOUNDS(m_paramIsValid));
        break;
      case PARAMID_MCP_PERIPHERAL_BOUNDS:
        this->applyBounds("PERIPHERAL", PERIF, this->paramGet_MCP_PERIPHERAL_BOUNDS(m_paramIsValid));
        break;
      case PARAMID_MCP_JETSON_BOUNDS:
        this->applyBounds("JETSON", JETSON, this->paramGet_MCP_JETSON_BOUNDS(m_paramIsValid));
        break;
      default:
        // Handle unexpected parameter ID
        printf("Warning: Received update for unrecognized parameter ID 0x%X. No bounds were updated.\n", id);
        break;
    }
  }

  void McpManager :: writeBoundsTelemetry(FwIndexType sensorIndex) {
    switch (sensorIndex) {
      case OBC:
        this->tlmWrite_MCP_IMX_BOUNDS(this->m_activeBounds[OBC]);
        break;
      case PERIF:
        this->tlmWrite_MCP_PERIPHERAL_BOUNDS(this->m_activeBounds[PERIF]);
        break;
      case JETSON:
        this->tlmWrite_MCP_JETSON_BOUNDS(this->m_activeBounds[JETSON]);
        break;
      default:
        break;
    }
  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void McpManager ::SET_TEMP_UPPER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 temp) {
      // TODO
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void McpManager ::SET_TEMP_LOWER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 temp) {
      // TODO
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  bool McpManager ::readTemp(U8 deviceAddr, std::string& location, F32& temperature)
  {
    U8 regAddr = TEMP_REG_ADDR;
    U8 rawData[2]; // MCP9808 temperature read back data is 2 bytes
    Fw::Buffer writeBuffer(&regAddr, 1);
    Fw::Buffer readBuffer(rawData, 2);

    // Port call to bus driver to write register address and read data
    Drv::I2cStatus status = this->mcpWriteRead_out(0, deviceAddr, writeBuffer, readBuffer);

    if (status == Drv::I2cStatus::I2C_OK){
      temperature = convertRawTemp(rawData); // Convert raw data to Celsius and return
      return true;
    }

    printf("Error reading from I2C device at location: %s\n", location.c_str());
    this->log_WARNING_HI_FAIL_TO_READ_TEMP_AT(Fw::String(location.c_str())); // Log event for read failure
    temperature = 0.0f;
    return false; // Return false if the device address is unrecognized
  }

  scalesSvc::ThermalStates McpManager :: determineTempState(F32 tempCelsius, U8 sensorIndex){
    const scalesSvc::TempBounds& bounds = this->m_activeBounds[sensorIndex];
    const F32 faultLow = bounds.get_faultLow();
    const F32 faultHigh = bounds.get_faultHigh();
    const F32 warnLow = bounds.get_warnLow();
    const F32 warnHigh = bounds.get_warnHigh();
    const F32 idleLow = bounds.get_idleLow();
    const F32 idleHigh = bounds.get_idleHigh();

    if (tempCelsius < faultLow || faultHigh <= tempCelsius ||
        (faultLow <= tempCelsius && tempCelsius < warnLow) ||
        (warnHigh < tempCelsius && tempCelsius < faultHigh)) {
      return scalesSvc::ThermalStates::FAULT;
    } else if ((warnLow <= tempCelsius && tempCelsius < idleLow) ||
               (idleHigh < tempCelsius && tempCelsius <= warnHigh)) {
      return scalesSvc::ThermalStates::WARN;
    } else if (idleLow <= tempCelsius && tempCelsius <= idleHigh){
      return scalesSvc::ThermalStates::IDLE;
    } else {
      // Treat gaps caused by invalid or overlapping parameters as unsafe.
      return scalesSvc::ThermalStates::FAULT;
    }
  }

  bool McpManager :: thresholdsAreOrdered(const scalesSvc::TempBounds& bounds) const {
    return bounds.get_faultLow() <= bounds.get_warnLow() &&
           bounds.get_warnLow() <= bounds.get_idleLow() &&
           bounds.get_idleLow() <= bounds.get_idleHigh() &&
           bounds.get_idleHigh() <= bounds.get_warnHigh() &&
           bounds.get_warnHigh() <= bounds.get_faultHigh();
  }

  void McpManager :: applyBounds(const char* source, FwIndexType sensorIndex, const scalesSvc::TempBounds& candidate) {
    if (this->thresholdsAreOrdered(candidate)) {
      this->m_activeBounds[sensorIndex] = candidate;
      this->writeBoundsTelemetry(sensorIndex);
    } else {
      this->log_WARNING_HI_THRESHOLDS_MISCONFIGURED(
          Fw::String(source), candidate.get_faultLow(), candidate.get_warnLow(),
          candidate.get_idleLow(), candidate.get_idleHigh(),
          candidate.get_warnHigh(), candidate.get_faultHigh());
    }
  }

}

// Helper function to convert raw 2-byte data from MCP9808 into a temperature value in Celsius
F32 convertRawTemp(U8 *rawData){

  // rawData[0] == UpperByte
  // rawData[1] == LowerByte

  F32 tempCelsius = 0.0;

  rawData[0] &= 0x1F; // Clear flag bits , keep only temperature data

  if ((rawData[0] & 0x10) == 0x10) { // Check if temperature is negative
    rawData[0] = rawData[0] & 0x0F; // Clear sign bit

    tempCelsius = 256 - ((rawData[0] * 16.0) + (rawData[1] / 16.0)); // Calculate negative temperature according to datasheet
    return -tempCelsius;

  } else {
    tempCelsius = ((rawData[0] * 16.0) + (rawData[1] / 16.0)); // Calculate positive temperature according to datasheet
    return tempCelsius;
  }
}
