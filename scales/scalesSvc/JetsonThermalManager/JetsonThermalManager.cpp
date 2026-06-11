// ======================================================================
// \title  JetsonThermalManager.cpp
// \author scales
// \brief  cpp file for JetsonThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManager.hpp"
#include <unordered_map> // Required header for hashmap
#include <string>

std::unordered_map<U8, std::string> indexToZone = {
    {0, "CPU"},
    {1, "GPU"},
    {2, "CV0"},
    {3, "CV1"},
    {4, "CV2"},
    {5, "SOC0"},
    {6, "SOC1"},
    {7, "SOC2"},
    {8, "TJ"}
};

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonThermalManager :: JetsonThermalManager(const char* const compName) : 
    JetsonThermalManagerComponentBase(compName),
    m_justBooted(true),
    m_successfulRead(true)
  {

  }

  JetsonThermalManager :: ~JetsonThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void JetsonThermalManager :: run_handler(FwIndexType portNum, U32 context)
  {
    this->jetson_thermalStateMachine_sendSignal_tick(); // Send tick signal to drive state machine transitions and actions
  }

  // ----------------------------------------------------------------------
  // Implementations for internal state machine actions
  // ----------------------------------------------------------------------

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    // If the device just booted, set up the parameters. If not, then start reading temp
    if (m_justBooted) {
      printf("Device just booted. Setting up parameters...\n");
      m_justBooted = false;
      
      // Default threshold values, can be updated by sending commands
      this->IDLE_LOW_THR = this->paramGet_JETSON_IDLE_LOW(m_paramIsValid); 
      this->IDLE_HIGH_THR = this->paramGet_JETSON_IDLE_HIGH(m_paramIsValid);
      this->WARN_LOW_THR = this->paramGet_JETSON_WARN_LOW(m_paramIsValid);
      this->WARN_HIGH_THR = this->paramGet_JETSON_WARN_HIGH(m_paramIsValid);
      this->FAULT_LOW_THR = this->paramGet_JETSON_FAULT_LOW(m_paramIsValid);
      this->FAULT_HIGH_THR = this->paramGet_JETSON_FAULT_HIGH(m_paramIsValid);
    } else {
        for (int i = 0; i < 9; i++){
          F32 temp;
          if (this->readTemp(i, temp)) {
            this->m_jetsonThermalReadings[i].settemperature(temp);
          }
          else {
            this->m_jetsonThermalReadings[i].settemperature(0.0f);
            this->m_successfulRead = false;
          }
          this->m_jetsonThermalReadings[i].setsensorId(i); 
          this->m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->m_jetsonThermalReadings[i].setlocation(Fw::String(indexToZone[i].c_str()));

        }

        // Transition to next state to evaluate the readings
        if(m_successfulRead){
          this->jetson_thermalStateMachine_sendSignal_success();
        } else { // If any read failed, transition to read failure state to log the failure event
          this->jetson_thermalStateMachine_sendSignal_fail();
        }
    }
  }

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doEvaluate(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Evaluating temperature readings against thresholds and updating telemetry...\n");
    for (int i = 0; i < 9; i++){
        scalesSvc::ThermalStates tempState = this->determineTempState(this->m_jetsonThermalReadings[i].gettemperature());
        this->m_jetsonThermalReadings[i].settempState(tempState); // Set the temp state
        
        switch(i){
          case 0:
            this->tlmWrite_jetson_cpu_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 1:
            this->tlmWrite_jetson_gpu_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 2:
            this->tlmWrite_jetson_cv0_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 3:
            this->tlmWrite_jetson_cv1_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 4:
            this->tlmWrite_jetson_cv2_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 5:
            this->tlmWrite_jetson_soc0_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 6:
            this->tlmWrite_jetson_soc1_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 7:
            this->tlmWrite_jetson_soc2_temp_read(this->m_jetsonThermalReadings[i]);
            break;
          case 8:
            this->tlmWrite_jetson_tj_temp_read(this->m_jetsonThermalReadings[i]);
            break;
        }
    }

    this->jetson_thermalStateMachine_sendSignal_success(); // Transition back to initial state to read temp again on next tick
  }

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Failed to read from sensor. Logging failure event...\n");

    this->m_successfulRead = true; // Reset success flag for next read attempt
    this->jetson_thermalStateMachine_sendSignal_success(); // Transition back to initial state to try reading again on next tick
  }

  void JetsonThermalManager :: parameterUpdated(FwPrmIdType id) {
    // Update threshold values based on parameter updates
    printf("Parameter with ID 0x%X has been updated. Updating threshold values...\n", id);

    switch (id) {
      case PARAMID_JETSON_IDLE_LOW:
        this->IDLE_LOW_THR = this->paramGet_JETSON_IDLE_LOW(m_paramIsValid);
        break;
      case PARAMID_JETSON_IDLE_HIGH:
        this->IDLE_HIGH_THR = this->paramGet_JETSON_IDLE_HIGH(m_paramIsValid);
        break;
      case PARAMID_JETSON_WARN_LOW:
        this->WARN_LOW_THR = this->paramGet_JETSON_WARN_LOW(m_paramIsValid);
        break;
      case PARAMID_JETSON_WARN_HIGH:
        this->WARN_HIGH_THR = this->paramGet_JETSON_WARN_HIGH(m_paramIsValid);
        break;
      case PARAMID_JETSON_FAULT_LOW:
        this->FAULT_LOW_THR = this->paramGet_JETSON_FAULT_LOW(m_paramIsValid);
        break;
      case PARAMID_JETSON_FAULT_HIGH:
        this->FAULT_HIGH_THR = this->paramGet_JETSON_FAULT_HIGH(m_paramIsValid);
        break;
      default:
        // Handle unexpected parameter ID
        printf("Warning: Received update for unrecognized parameter ID 0x%X. No threshold values were updated.\n", id);
        break;
    }
  }

  bool JetsonThermalManager :: readTemp(U8 index, F32& temp) {
    float tempMilliC = 0.0f;
    char path[128];
    std::snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%u/temp", index);
    std::ifstream tempFile(path);

    if (!tempFile.is_open()) {
      printf("Could not open thermal file: %s at zone: %s\n", path, indexToZone[index].c_str());
      temp = 0.0f; // Set temp to 0 if we fail to read so that the system defaults to IDLE state for that sensor
      return false;
    }

    if (!(tempFile >> tempMilliC)) {
      printf("Could not read thermal file: %s at zone: %s\n", path,indexToZone[index].c_str());
      temp = 0.0f; // Set temp to 0 if we fail to read so that the system defaults to IDLE state for that sensor
      return false;
    }

    temp = tempMilliC / 1000.0f;
    return true;
  }

  scalesSvc::ThermalStates JetsonThermalManager :: determineTempState(F32 tempCelsius) {
    if (this->IDLE_LOW_THR <= tempCelsius && tempCelsius <= this->IDLE_HIGH_THR){
      return scalesSvc::ThermalStates::IDLE;
    } else if ((this->WARN_LOW_THR <= tempCelsius && tempCelsius < this->IDLE_LOW_THR) || (this->IDLE_HIGH_THR < tempCelsius && tempCelsius <= this->WARN_HIGH_THR)){
      return scalesSvc::ThermalStates::WARN;
    } else {
      return scalesSvc::ThermalStates::FAULT;
    }
  }
}
