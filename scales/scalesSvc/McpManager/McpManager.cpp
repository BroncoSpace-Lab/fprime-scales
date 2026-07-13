// ======================================================================
// \title  McpManager.cpp
// \author scales
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"
#include <unordered_map> // Required header for hashmap

F32 convertRawTemp(U8 *rawData); // Forward decleration 

std::unordered_map<U8, std::string> indexToLocation = {
    {0, "OBC"},
    {1, "PERIPHERAL"},
    {2, "JETSON"}
};


namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  McpManager :: McpManager(const char* const compName) :
    McpManagerComponentBase(compName), 
    m_successfulRead(true), // Initialize successful read flag to true as default
    m_successfulReads{true, true, true}, // Initialize successful reads array to true for all sensors
    m_justBooted(true)
  {
    deviceAddrs[0] = IMX_TEMP_ADDR;
    deviceAddrs[1] = PERIPHERAL_TEMP_ADDR;
    deviceAddrs[2] = JETSON_TEMP_ADDR;
  }

  McpManager :: ~McpManager() {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void McpManager :: run_handler(FwIndexType portNum, U32 context)
  { 
    this->mcp_thermalStateMachine_sendSignal_tick(); // Trigger state machine tick 
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
      // Default threshold values, can be updated by sending commands
      this->IDLE_LOW_THR = this->paramGet_MCP_IDLE_LOW(m_paramIsValid); 
      this->IDLE_HIGH_THR = this->paramGet_MCP_IDLE_HIGH(m_paramIsValid);
      this->WARN_LOW_THR = this->paramGet_MCP_WARN_LOW(m_paramIsValid);
      this->WARN_HIGH_THR = this->paramGet_MCP_WARN_HIGH(m_paramIsValid);
      this->FAULT_LOW_THR = this->paramGet_MCP_FAULT_LOW(m_paramIsValid);
      this->FAULT_HIGH_THR = this->paramGet_MCP_FAULT_HIGH(m_paramIsValid);
    } else {
        // Read temp data from sensors and log to telemetry
        for (int i = 0; i < 3; i++){

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
        m_successfulRead = m_successfulReads[0] && m_successfulReads[1] && m_successfulReads[2]; // Update the overall successful read flag based on individual sensor reads

        this->mcp_thermalStateMachine_sendSignal_success(); // Transition to next state to evaluate the readings
    }
  }


  void McpManager :: scalesSvc_ThermalStateMachine_action_doEvaluate(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // printf("Evaluating thermal readings against thresholds...\n");
    for (int i = 0; i < 3; i++){

      if(m_successfulReads[i]){ // Only evaluate if this sensor readding was successful, otherwise the temp state is already set to FAULT
        scalesSvc::ThermalStates tempState = this->determineTempState(this->m_thermalReadings[i].get_temperature());
        this->m_thermalReadings[i].set_tempState(tempState); // Set the temp state in the reading struct to log to telemetry
      }
      
      switch(i){
        case 0:
          this->tlmWrite_IMX_TEMP(m_thermalReadings[0]);
          break;
        case 1:
          this->tlmWrite_PERIPHERAL_TEMP(m_thermalReadings[1]);
          break;
        case 2:
          this->tlmWrite_JETSON_TEMP(m_thermalReadings[2]);
          break;
        default:
          printf("Warning: Unrecognized sensor index %d. No telemetry was logged for this sensor.\n", i);
          break;
      }
    }
    
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
    for (int i = 0; i < 3; i++){
      m_successfulReads[i] = true; // Reset successful reads array to true for all sensors to try reading again on next tick
    }
    this->mcp_thermalStateMachine_sendSignal_success(); // Transition back to initial state to try reading again on next tick
  }


  // ----------------------------------------------------------------------
  // Handler implementations for parameters update
  // ----------------------------------------------------------------------

  void McpManager :: parameterUpdated(FwPrmIdType id){
    // Update threshold values based on parameter updates
    printf("Parameter with ID 0x%X has been updated. Updating threshold values...\n", id);
    switch(id){
      case PARAMID_MCP_IDLE_LOW:
        this->IDLE_LOW_THR = this->paramGet_MCP_IDLE_LOW(m_paramIsValid);
        break;
      case PARAMID_MCP_IDLE_HIGH:
        this->IDLE_HIGH_THR = this->paramGet_MCP_IDLE_HIGH(m_paramIsValid);
        break;
      case PARAMID_MCP_WARN_LOW:
        this->WARN_LOW_THR = this->paramGet_MCP_WARN_LOW(m_paramIsValid);
        break;
      case PARAMID_MCP_WARN_HIGH:
        this->WARN_HIGH_THR = this->paramGet_MCP_WARN_HIGH(m_paramIsValid);
        break;
      case PARAMID_MCP_FAULT_LOW:
        this->FAULT_LOW_THR = this->paramGet_MCP_FAULT_LOW(m_paramIsValid);
        break;
      case PARAMID_MCP_FAULT_HIGH:
        this->FAULT_HIGH_THR = this->paramGet_MCP_FAULT_HIGH(m_paramIsValid);
        break;
      default:
        // Handle unexpected parameter ID
        printf("Warning: Received update for unrecognized parameter ID 0x%X. No threshold values were updated.\n", id);
        break;
    }
  }

  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  bool McpManager ::readTemp(U8 deviceAddr, std::string location, F32& temperature)
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
  
  scalesSvc::ThermalStates McpManager :: determineTempState(F32 tempCelsius){
    if (this->IDLE_LOW_THR <= tempCelsius && tempCelsius <= this->IDLE_HIGH_THR){
      return scalesSvc::ThermalStates::IDLE;
    } else if ((this->WARN_LOW_THR <= tempCelsius && tempCelsius < this->IDLE_LOW_THR) || (this->IDLE_HIGH_THR < tempCelsius && tempCelsius <= this->WARN_HIGH_THR)){
      return scalesSvc::ThermalStates::WARN;
    } else {
      return scalesSvc::ThermalStates::FAULT;
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
