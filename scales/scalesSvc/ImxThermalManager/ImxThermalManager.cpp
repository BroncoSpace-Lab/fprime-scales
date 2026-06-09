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
      
      std::ifstream tempFile(tempPath); // Open the temperature file
      
      if(tempFile){ //if the file opened successfully, read the data
      tempFile >> this->m_tempMilliC;         // Read the raw temperature value into the variable
      this->m_tempC = this->m_tempMilliC / 1000.0f; // Convert from millidegrees Celsius to Celsius
      (this->cpu_thermal_read).settemperature(m_tempC);
      (this->cpu_thermal_read).setsensorId(0);
      (this->cpu_thermal_read).setlocation(Fw::String("CPU"));
      (this->cpu_thermal_read).settimestamp(this->getTime().getSeconds());
      this->tlmWrite_imx_cpu_temp_read(this->cpu_thermal_read);


      this->thermalStateMachine_sendSignal_success();
    }
      else {
        this->thermalStateMachine_sendSignal_fail();
      }

  }

void ImxThermalManager::scalesSvc_ThermalStateMachine_action_paramEvaluate( SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
  
  if (this->m_tempC >= paramGet_IMX_CPU_IDLE_LOW(m_paramValid) && this->m_tempC < paramGet_IMX_CPU_IDLE_HIGH(m_paramValid))
    {
      this->tlmWrite_imx_thermal_state(ThermalStates::IDLE);
      //this->imxThermalStateOut_out(0, ThermalStates::IDLE);
      this->thermalStateMachine_sendSignal_success();
    }
  if (this->m_tempC >= paramGet_IMX_CPU_WARN_LOW(m_paramValid) && this->m_tempC < paramGet_IMX_CPU_WARN_HIGH(m_paramValid))
    {
      this->tlmWrite_imx_thermal_state(ThermalStates::WARN);
      //this->imxThermalStateOut_out(0, ThermalStates::WARN);
      this->thermalStateMachine_sendSignal_success();
    }
  if (this->m_tempC >= paramGet_IMX_CPU_FAULT_LOW(m_paramValid) && this->m_tempC < paramGet_IMX_CPU_FAULT_HIGH(m_paramValid))
    {
      this->tlmWrite_imx_thermal_state(ThermalStates::FAULT);
      //this->imxThermalStateOut_out(0, ThermalStates::FAULT);
      this->thermalStateMachine_sendSignal_success();
    }
  else{
      this->thermalStateMachine_sendSignal_fail();
  }
}

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_readFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
      
      std::ifstream tempFile(tempPath); // Open the temperature file
      if(tempFile){ //if the file opened successfully, read the data
      tempFile >> this->m_tempMilliC;         // Read the raw temperature value into the variable
      this->m_tempC = this->m_tempMilliC / 1000.0f; // Convert from millidegrees Celsius to Celsius
      this->thermalStateMachine_sendSignal_success();
      }
      else{
        (this->cpu_thermal_read).setlocation(Fw::String("FAILED_READ"));
        this->thermalStateMachine_sendSignal_fail();
      }
      
    }
}

