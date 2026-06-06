// ======================================================================
// \title  McpManager.cpp
// \author Luca, Dat, and Jabob
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"
#include <algorithm> // Required for std::max

F32 convertRawTemp(U8 *rawData); // Forward decleration 

enum temp_state{IDLE = 1, WARNING = 2, FAULT = 3}; 


namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  McpManager :: McpManager(const char* const compName) :
    McpManagerComponentBase(compName), 
    m_justBooted(true),
    m_currentState(0)
  {
    m_thermalReadings[0] = imx_thermalReadings;
    m_thermalReadings[1] = peripheral_thermalReadings;
    m_thermalReadings[2] = jetson_thermalReadings;

    deviceAddrs[0] = IMX_TEMP_ADDR;
    deviceAddrs[1] = PERIPHERAL_TEMP_ADDR;
    deviceAddrs[2] = JETSON_TEMP_ADDR;
  }

  McpManager :: ~McpManager() {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void McpManager :: run_handler( FwIndexType portNum, U32 context) {
    this->thermalStateMachine_sendSignal_tick(); // Send tick signal to trigger thermal state machine to read temp data and log telemetry
  }

  // ----------------------------------------------------------------------
  // Implementations for internal state machine actions
  // ----------------------------------------------------------------------

  void McpManager :: scalesSvc_ThermalStateMachine_action_doReset( SmId smId, scalesSvc_ThermalStateMachine::Signal signal) {
    // If the device just booted, move to READ_TEMP
    if (m_justBooted) {
      m_justBooted = false;
      this->thermalStateMachine_sendSignal_success(); // Send success signal to transition to READ_TEMP
    } else { // Else Reset now
      printf("Manager reached FAULT state, performing reset...\n");
      m_currentState = 0; // Reset current state
    }
    
  }
   
  void McpManager :: scalesSvc_ThermalStateMachine_action_doReadTemp( SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // Read temp data from sensors and log to telemetry
    for (int i = 0; i < 3; i++){
      this->m_thermalReadings[i].settemperature(this->readTemp(deviceAddrs[i])); 
      this->m_thermalReadings[i].setsensorId(i + 1);
      this->m_thermalReadings[i].settimestamp(this->getTime().getSeconds());
      this->m_thermalReadings[i].setlocation(Fw::String("On board MCP9808 sensor"));
    }

    // Logs to each sensor's respective telemetry channel
    this->tlmWrite_IMX_TEMP(m_thermalReadings[0]);
    this->tlmWrite_PERIPHERAL_TEMP(m_thermalReadings[1]);
    this->tlmWrite_JETSON_TEMP(m_thermalReadings[2]);

    // Determine worst temp state among the 3 sensors
    U8 worse_state = std::max({this->determineTempState(m_thermalReadings[0].gettemperature()), this->determineTempState(m_thermalReadings[1].gettemperature()), this->determineTempState(m_thermalReadings[2].gettemperature())});

    switch (worse_state) {
      case IDLE:
        if (m_currentState != IDLE){
          m_currentState = IDLE;
          this->thermalStateMachine_sendSignal_idle();    
        }
        break;
      case WARNING:
        if (m_currentState != WARNING){
          m_currentState = WARNING;
          this->thermalStateMachine_sendSignal_warn();
        }
        break;
      case FAULT:
        if (m_currentState != FAULT){
          m_currentState = FAULT;
          this->thermalStateMachine_sendSignal_fault();
        }
        break;
      default:
        // Handle unexpected state
        this->thermalStateMachine_sendSignal_error();
        break;
    }



    // Transition to IDLE after reading temp
  }

  void McpManager :: scalesSvc_ThermalStateMachine_action_doIdle(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // Log temeletry to show that the system has switched state and is curretly in IDLE
    printf("System is in IDLE state. Temperatures are within normal operating range.\n");
    this->tlmWrite_MCP_THERMAL_STATE(Fw::String("IDLE"));
    this->thermalStateMachine_sendSignal_success(); // Send success signal to transition to READ_TEMP
  }

  void McpManager :: scalesSvc_ThermalStateMachine_action_doWarning(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // Log temeletry to show that the system has switched state and is curretly in WARNING
    printf("System is in WARNING state. Temperatures are above normal operating range.\n");
    this->tlmWrite_MCP_THERMAL_STATE(Fw::String("WARNING"));
    this->thermalStateMachine_sendSignal_success(); // Send success signal to transition to READ_TEMP 
  }

  void McpManager :: scalesSvc_ThermalStateMachine_action_doFault( SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // Log temeletry to show that the system has switched state and is curretly in FAULT
    printf("System is in FAULT state. Temperatures are outside the safe operating range.\n");
    this->tlmWrite_MCP_THERMAL_STATE(Fw::String("FAULT"));
    this->thermalStateMachine_sendSignal_error(); // Send error signal to transition to RESET
    
  }


  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  F32 McpManager :: readTemp(U8 deviceAddr)
  {
    U8 regAddr = TEMP_REG_ADDR;
    U8 rawData[2]; // MCP9808 temperature read back data is 2 bytes
    Fw::Buffer writeBuffer(&regAddr, 1);
    Fw::Buffer readBuffer(rawData, 2);

    // Port call to bus driver to write register address and read data
    Drv::I2cStatus status = this->mcpWriteRead_out(0, deviceAddr, writeBuffer, readBuffer);

    if (status == Drv::I2cStatus::I2C_OK){
      return convertRawTemp(rawData); // Convert raw data to Celsius and return
    }
    
    printf("Error reading from I2C device at address 0x%X\n", deviceAddr);
    this->thermalStateMachine_sendSignal_error(); // Send error signal to transition to RESET in case of I2C read failure
    return -120.0; // Return an error value if the device address is unrecognized 
  }
  
  U8 McpManager :: determineTempState(F32 tempCelsius){
    if (10 < tempCelsius && tempCelsius < 60.0){
      return IDLE;
    } else if ((60.0 <= tempCelsius && tempCelsius < 80.0) || (-20 < tempCelsius && tempCelsius <= 10.0)){
      return WARNING;
    } else {
      return FAULT;
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