// ======================================================================
// \title  McpManager.cpp
// \author scales
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"
#include <unordered_map> // Required header for hashmap
#include <string>

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
    // Log temperature threshold values to telemetry on each run, in case they were updated by a command
    // this->tlmWrite_MCP_IDLE_LOW(this->IDLE_LOW_THR);
    // this->tlmWrite_MCP_IDLE_HIGH(this->IDLE_HIGH_THR);
    // this->tlmWrite_MCP_WARN_LOW(this->WARN_LOW_THR);
    // this->tlmWrite_MCP_WARN_HIGH(this->WARN_HIGH_THR);
    // this->tlmWrite_MCP_FAULT_LOW(this->FAULT_LOW_THR);
    // this->tlmWrite_MCP_FAULT_HIGH(this->FAULT_HIGH_THR);
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
        printf("Reading temperature data from sensors...\n");
        // Read temp data from sensors and log to telemetry
        for (int i = 0; i < 3; i++){
          // this->m_thermalReadings[i].settemperature(this->readTemp(deviceAddrs[i])); 

          F32 tempCelsius;
          if (this->readTemp(deviceAddrs[i], tempCelsius)){
            this->m_thermalReadings[i].settemperature(tempCelsius);
          } else {
            this->m_thermalReadings[i].settemperature(0.0f); // Set temp to 0 if the read failed, which will be evaluated as IDLE in determineTempState, and log the failure in the next state
            m_successfulRead = false; // Set successful read flag to false if any read failed, which will be used to determine state machine transition in the next states
          }

          this->m_thermalReadings[i].setsensorId(i + 1);
          this->m_thermalReadings[i].settimestamp(this->getTime().getSeconds()- m_startTime); // Log uptime in seconds as the timestamp for telemetry
          this->m_thermalReadings[i].setlocation(Fw::String(indexToLocation[i].c_str()));
        }

        // // Logs to each sensor's respective telemetry channel
        // this->tlmWrite_IMX_TEMP(m_thermalReadings[0]);
        // this->tlmWrite_PERIPHERAL_TEMP(m_thermalReadings[1]);
        // this->tlmWrite_JETSON_TEMP(m_thermalReadings[2]);
        
        if (m_successfulRead) {
          // Transition to next state to evaluate the readings
          this->mcp_thermalStateMachine_sendSignal_success();
        } else {
          // If any read failed, transition to read failure state to log the failure event
          this->mcp_thermalStateMachine_sendSignal_fail();
        }
    }
  }


  void McpManager :: scalesSvc_ThermalStateMachine_action_doEvaluate(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Evaluating thermal readings against thresholds...\n");
    for (int i = 0; i < 3; i++){
      scalesSvc::ThermalStates tempState = this->determineTempState(this->m_thermalReadings[i].gettemperature());
      this->m_thermalReadings[i].settempState(tempState); // Set the temp state in the reading struct to log to telemetry
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
    this->mcp_thermalStateMachine_sendSignal_success(); // Transition back to initial state to read temp again on next tick
  }

  void McpManager :: scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Failed to read from sensor. Logging failure event...\n");
    m_successfulRead = true; // Reset successful read flag to true to try reading again on next tick
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

  bool McpManager ::readTemp(U8 deviceAddr, F32& temperature)
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
    
    printf("Error reading from I2C device at address 0x%X\n", deviceAddr);
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
    return tempCelsius;

  } else {
    tempCelsius = ((rawData[0] * 16.0) + (rawData[1] / 16.0)); // Calculate positive temperature according to datasheet
    return tempCelsius;
  }
}
