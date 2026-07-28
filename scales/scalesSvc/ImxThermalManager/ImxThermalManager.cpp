// ======================================================================
// \title  ImxThermalManager.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"
#include <Fw/Types/StringUtils.hpp>
#include <Os/File.hpp>

namespace {
  constexpr FwSizeType TEMP_FILE_BUFFER_SIZE = 32;
}

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

  void ImxThermalManager ::run_handler(FwIndexType portNum, U32 context) {
      this->thermalStateMachine_sendSignal_tick();
}

bool ImxThermalManager::readTemperatureFile() {
  Os::File tempFile;
  Os::File::Status fileStatus = tempFile.open(this->tempPath, Os::File::Mode::OPEN_READ);
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

  this->m_tempMilliC = static_cast<F32>(tempMilliC);
  this->m_tempC = this->m_tempMilliC / 1000.0F;
  return true;
}

void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doRead(SmId smId, scalesSvc_ThermalStateMachine::Signal signal) {
      
      if (m_justBooted){
        m_startTime = this->getTime().getSeconds(); // Record the start time at boot to track uptime in telemetry
        m_justBooted = false;
        this->writeParameterTelemetry();
      }
      
      if(this->readTemperatureFile()){ //if the file opened and parsed successfully, read the data
      (this->m_cpu_thermal_read).set_temperature(m_tempC);
      (this->m_cpu_thermal_read).set_sensorId(0);
      (this->m_cpu_thermal_read).set_location(Fw::String("CPU"));
      (this->m_cpu_thermal_read).set_timestamp(this->getTime().getSeconds()- m_startTime);
      
      this->thermalStateMachine_sendSignal_success();
    }
      else {
        this->thermalStateMachine_sendSignal_fail();
      }

  }

void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doEvaluate( SmId smId, scalesSvc_ThermalStateMachine::Signal signal){
  this->validateThresholds();

  const F32 faultLow = paramGet_IMX_CPU_FAULT_LOW(m_paramValid);
  const F32 warnLow = paramGet_IMX_CPU_WARN_LOW(m_paramValid);
  const F32 idleLow = paramGet_IMX_CPU_IDLE_LOW(m_paramValid);
  const F32 idleHigh = paramGet_IMX_CPU_IDLE_HIGH(m_paramValid);
  const F32 warnHigh = paramGet_IMX_CPU_WARN_HIGH(m_paramValid);
  const F32 faultHigh = paramGet_IMX_CPU_FAULT_HIGH(m_paramValid);

  ThermalStates state;
  if (this->m_tempC < faultLow || faultHigh <= this->m_tempC ||
      (faultLow <= this->m_tempC && this->m_tempC < warnLow) ||
      (warnHigh <= this->m_tempC && this->m_tempC < faultHigh)) {
    state = scalesSvc::ThermalStates::FAULT;
  } else if ((warnLow <= this->m_tempC && this->m_tempC < idleLow) ||
             (idleHigh < this->m_tempC && this->m_tempC < warnHigh)) {
    state = scalesSvc::ThermalStates::WARN;
  } else if (idleLow <= this->m_tempC && this->m_tempC <= idleHigh) {
    state = scalesSvc::ThermalStates::IDLE;
  } else {
    // Treat gaps caused by invalid or overlapping parameters as unsafe.
    state = scalesSvc::ThermalStates::FAULT;
  }

  this->m_cpu_thermal_read.set_tempState(state);
  this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read);
  this->imxThermalReadingOut_out(0, this->m_cpu_thermal_read);
  this->thermalStateMachine_sendSignal_success();
}

  void ImxThermalManager::scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal){

      if(this->readTemperatureFile()){ //if the file opened and parsed successfully, read the data
      this->thermalStateMachine_sendSignal_success();
      }
      else{
        this->m_cpu_thermal_read.set_location(Fw::String("FAILED_READ"));
        this->tlmWrite_imx_cpu_temp_read(this->m_cpu_thermal_read);
        this->thermalStateMachine_sendSignal_fail();
      }

    }

  void ImxThermalManager::parameterUpdated(FwPrmIdType id) {
    this->writeParameterTelemetry();
    this->validateThresholds();
  }

  void ImxThermalManager::writeParameterTelemetry() {
    this->tlmWrite_IMX_CPU_IDLE_LOW(this->paramGet_IMX_CPU_IDLE_LOW(m_paramValid));
    this->tlmWrite_IMX_CPU_IDLE_HIGH(this->paramGet_IMX_CPU_IDLE_HIGH(m_paramValid));
    this->tlmWrite_IMX_CPU_WARN_LOW(this->paramGet_IMX_CPU_WARN_LOW(m_paramValid));
    this->tlmWrite_IMX_CPU_WARN_HIGH(this->paramGet_IMX_CPU_WARN_HIGH(m_paramValid));
    this->tlmWrite_IMX_CPU_FAULT_LOW(this->paramGet_IMX_CPU_FAULT_LOW(m_paramValid));
    this->tlmWrite_IMX_CPU_FAULT_HIGH(this->paramGet_IMX_CPU_FAULT_HIGH(m_paramValid));
  }

  bool ImxThermalManager::thresholdsAreOrdered(F32 faultLow, F32 warnLow, F32 idleLow,
                                                F32 idleHigh, F32 warnHigh, F32 faultHigh) const {
    return faultLow <= warnLow && warnLow <= idleLow && idleLow <= idleHigh &&
           idleHigh <= warnHigh && warnHigh <= faultHigh;
  }

  void ImxThermalManager::validateThresholds() {
    const F32 faultLow = paramGet_IMX_CPU_FAULT_LOW(m_paramValid);
    const F32 warnLow = paramGet_IMX_CPU_WARN_LOW(m_paramValid);
    const F32 idleLow = paramGet_IMX_CPU_IDLE_LOW(m_paramValid);
    const F32 idleHigh = paramGet_IMX_CPU_IDLE_HIGH(m_paramValid);
    const F32 warnHigh = paramGet_IMX_CPU_WARN_HIGH(m_paramValid);
    const F32 faultHigh = paramGet_IMX_CPU_FAULT_HIGH(m_paramValid);

    const bool ordered = this->thresholdsAreOrdered(faultLow, warnLow, idleLow, idleHigh, warnHigh, faultHigh);
    if (!ordered && this->m_thresholdsValid) {
      this->m_thresholdsValid = false;
      this->log_WARNING_HI_THRESHOLDS_MISCONFIGURED(
          Fw::String("IMX_CPU"), faultLow, warnLow, idleLow, idleHigh, warnHigh, faultHigh);
    } else if (ordered) {
      this->m_thresholdsValid = true;
    }
  }
}
