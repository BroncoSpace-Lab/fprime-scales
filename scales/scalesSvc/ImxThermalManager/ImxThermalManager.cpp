// ======================================================================
// \title  ImxThermalManager.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"
#include <fstream>
#include <iostream>


namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManager ::
    ImxThermalManager(const char* const compName) :
      ImxThermalManagerComponentBase(compName)
  {

  }

  ImxThermalManager ::
    ~ImxThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void ImxThermalManager ::imxCpuTemp_handler(FwIndexType portNum, U32 context) {
      this->thermalStateMachine_sendSignal_tick();
}

void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal) {
      
      m_startTime = this->getTime().getSeconds(); // Record the start time at boot to track uptime in telemetry
      std::ifstream tempFile(tempPath); // Open the temperature file
      // Rob says to use FPrime OSAL for file reading.
      if(tempFile){ //if the file opened successfully, read the data
      tempFile >> this->m_tempMilliC;         // Read the raw temperature value into the variable
      this->m_tempC = this->m_tempMilliC / 1000.0f; // Convert from millidegrees Celsius to Celsius
      (this->m_cpu_thermal_read).settemperature(m_tempC);
      (this->m_cpu_thermal_read).setsensorId(0);
      (this->m_cpu_thermal_read).setlocation(Fw::String("CPU"));
      (this->m_cpu_thermal_read).settimestamp(this->getTime().getSeconds()- m_startTime);
      
      this->thermalStateMachine_sendSignal_success();
    }
      else {
        this->thermalStateMachine_sendSignal_fail();
      }

  }

void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doEvaluate( SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
  if(paramGet_IMX_CPU_FAULT_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_WARN_LOW(m_paramValid)){
    this->m_cpu_thermal_read.settempState(scalesSvc::ThermalStates::FAULT); // Set the thermal state to FAULT
    this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state
    
      this->thermalStateMachine_sendSignal_success();
  }
  if(paramGet_IMX_CPU_WARN_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_IDLE_LOW(m_paramValid)){
    this->m_cpu_thermal_read.settempState(scalesSvc::ThermalStates::WARN); // Set the thermal state to WARN
    this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

      this->thermalStateMachine_sendSignal_success();
  }
  if(paramGet_IMX_CPU_IDLE_LOW(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_IDLE_HIGH(m_paramValid)){
    this->m_cpu_thermal_read.settempState(scalesSvc::ThermalStates::IDLE); // Set the thermal state to IDLE
    this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

      this->thermalStateMachine_sendSignal_success();
  }
  if(paramGet_IMX_CPU_IDLE_HIGH(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_WARN_HIGH(m_paramValid)){
    this->m_cpu_thermal_read.settempState(scalesSvc::ThermalStates::WARN); // Set the thermal state to WARN
    this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state

      this->thermalStateMachine_sendSignal_success();
  }
  if(paramGet_IMX_CPU_WARN_HIGH(m_paramValid) <= this->m_tempC && this->m_tempC < paramGet_IMX_CPU_FAULT_HIGH(m_paramValid)){
    this->m_cpu_thermal_read.settempState(scalesSvc::ThermalStates::FAULT); // Set the thermal state to FAULT
    this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read); // emit the telemetry with the state
   
      this->thermalStateMachine_sendSignal_success();
  }
}

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
      
      std::ifstream tempFile(tempPath); // Open the temperature file
      if(tempFile){ //if the file opened successfully, read the data
      tempFile >> this->m_tempMilliC;         // Read the raw temperature value into the variable
      this->m_tempC = this->m_tempMilliC / 1000.0f; // Convert from millidegrees Celsius to Celsius
      this->thermalStateMachine_sendSignal_success();
      }
      else{
        this->m_cpu_thermal_read.setlocation(Fw::String("FAILED_READ"));
        this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read);
        this->thermalStateMachine_sendSignal_fail();
      }
      
    }
}
