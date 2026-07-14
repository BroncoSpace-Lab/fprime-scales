// ======================================================================
// \title  ImxThermalManager.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"
#include <Fw/Types/StringUtils.hpp>
#include <Os/File.hpp>

const char* tempPath = "/sys/class/thermal/thermal_zone0/temp"; // Path to the temperateu file for the imx CPU.
constexpr FwSizeType TEMP_FILE_BUFFER_SIZE = 32;

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManager :: ImxThermalManager(const char* const compName) :
      ImxThermalManagerComponentBase(compName)
  {

  }

  ImxThermalManager :: ~ImxThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void ImxThermalManager ::run_handler(FwIndexType portNum, U32 context) {
      this->thermalStateMachine_sendSignal_tick(); // Send tick signal to drive state machine transitions and actions
}

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal) {
        
      if (m_justBooted){
        m_startTime = this->getTime().getSeconds(); // Record the start time at boot to track uptime in telemetry
        this->m_cpu_thermal_read.set_sensorId(0);
        this->m_cpu_thermal_read.set_location(Fw::String("CPU"));
        m_justBooted = false;
      }
      
      this->m_cpu_thermal_read.set_timestamp(this->getTime().getSeconds()- m_startTime);

      if (this->readTemperatureFile()) {
          this->m_cpu_thermal_read.set_temperature(this->m_tempC);
      } else {
          this->m_cpu_thermal_read.set_temperature(0.0f);
          this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::FAULT); // Set the thermal state to FAULT
          m_successfulRead = false;
      }

      this->thermalStateMachine_sendSignal_success(); // Send success signal to transition to the next state
    }

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doEvaluate( SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
    if (!m_successfulRead) {
        this->m_cpu_thermal_read.set_location(Fw::String("FAILED_READ"));
        this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read);
        this->thermalStateMachine_sendSignal_fail();
        return;
    }

    if(paramGet_IMX_CPU_FAULT_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_WARN_LOW(m_paramValid)){
      this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::FAULT); // Set the thermal state to FAULT
      this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state
      
        this->thermalStateMachine_sendSignal_success();
    }
    if(paramGet_IMX_CPU_WARN_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_IDLE_LOW(m_paramValid)){
      this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::WARN); // Set the thermal state to WARN
      this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

        this->thermalStateMachine_sendSignal_success();
    }
    if(paramGet_IMX_CPU_IDLE_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_IDLE_HIGH(m_paramValid)){
      this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::IDLE); // Set the thermal state to IDLE
      this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

        this->thermalStateMachine_sendSignal_success();
    }
    if(paramGet_IMX_CPU_IDLE_HIGH(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_WARN_HIGH(m_paramValid)){
      this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::WARN); // Set the thermal state to WARN
      this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

        this->thermalStateMachine_sendSignal_success();
    }
    if(paramGet_IMX_CPU_WARN_HIGH(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_FAULT_HIGH(m_paramValid)){
      this->m_cpu_thermal_read.set_tempState(scalesSvc::ThermalStates::FAULT); // Set the thermal state to FAULT
      this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state
    
        this->thermalStateMachine_sendSignal_success();
    }
  }

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
      
    printf("Failed to read temperature file. Logging failure event...\n");
    this->log_WARNING_HI_FAIL_TO_READ_TEMP(); // Log the failure event
    m_successfulRead = true; // Reset the successful read flag for the next read attempt
    this->thermalStateMachine_sendSignal_success(); // Transition back to initial state to try reading again on next tick
      
  }
  
  bool ImxThermalManager::readTemperatureFile(){

    // Open temperature file using OSAL
    Os::File tempFile;
    Os::File::Status fileStatus = tempFile.open(tempPath, Os::File::Mode::OPEN_READ);
    if (fileStatus != Os::File::Status::OP_OK) {
      return false;
    }

    CHAR tempBuffer[TEMP_FILE_BUFFER_SIZE] = {};
    FwSizeType readSize = sizeof(tempBuffer) - 1;
    fileStatus = tempFile.read(reinterpret_cast<U8*>(tempBuffer), readSize, Os::File::WaitType::NO_WAIT);
    tempFile.close();
    if ((fileStatus != Os::File::Status::OP_OK) || (readSize == 0)) {
      return false;
    }
    tempBuffer[readSize] = '\0';

    I32 tempMilliC = 0;
    CHAR* parseEnd = nullptr;
    Fw::StringUtils::StringToNumberStatus parseStatus =
        Fw::StringUtils::string_to_number(tempBuffer, sizeof(tempBuffer), tempMilliC, &parseEnd, 10);
    if ((parseStatus != Fw::StringUtils::StringToNumberStatus::SUCCESSFUL_CONVERSION) || (parseEnd == nullptr)) {
      return false;
    }
    while ((*parseEnd == ' ') || (*parseEnd == '\t') || (*parseEnd == '\r') || (*parseEnd == '\n')) {
      parseEnd++;
    }
    if (*parseEnd != '\0') {
      return false;
    }

    this->m_tempC = tempMilliC / 1000.0F;
    return true;

  }

}
