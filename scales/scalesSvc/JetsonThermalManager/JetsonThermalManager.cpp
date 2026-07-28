// ======================================================================
// \title  JetsonThermalManager.cpp
// \author scales
// \brief  cpp file for JetsonThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManager.hpp"
#include <Fw/Types/StringUtils.hpp>
#include <Os/File.hpp>
#include <cstdio>
#include <unordered_map> // Required header for hashmap
#include <string>

namespace {
  constexpr FwSizeType TEMP_FILE_BUFFER_SIZE = 32;
}

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
      m_startTime = this->getTime().getSeconds();
      // Gate the saved/default bounds through the same validity check as a
      // live PRM_SET, and publish the resulting active bounds.
      this->applyBounds(this->paramGet_JETSON_BOUNDS(m_paramIsValid));
    } else {
        for (int i = 0; i < 9; i++){
          F32 temp;
          if (this->readTemp(i, temp)) {
            this->m_jetsonThermalReadings[i].set_temperature(temp);
            this->m_jetsonThermalReadings[i].set_tempState(this->determineTempState(temp));
          }
          else {
            this->m_jetsonThermalReadings[i].set_temperature(0.0F);
            this->m_jetsonThermalReadings[i].set_tempState(scalesSvc::ThermalStates::NOT_USED);
            // this->m_successfulRead = false;
          }
          this->m_jetsonThermalReadings[i].set_sensorId(i);
          this->m_jetsonThermalReadings[i].set_timestamp(this->getTime().getSeconds()-m_startTime);
          this->m_jetsonThermalReadings[i].set_location(Fw::String(indexToZone[i].c_str()));

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
    // printf("Evaluating temperature readings against thresholds and updating telemetry...\n");
    for (int i = 0; i < 9; i++){
        if (this->m_jetsonThermalReadings[i].get_tempState() != scalesSvc::ThermalStates::NOT_USED) {
          scalesSvc::ThermalStates tempState = this->determineTempState(this->m_jetsonThermalReadings[i].get_temperature());
          this->m_jetsonThermalReadings[i].set_tempState(tempState); // Set the temp state
        }

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
        this->jetsonThermalReadingOut_out(0, this->m_jetsonThermalReadings[i]);
    }

    // Republish the active bounds every cycle (not just on boot/change) so a
    // GDS session that connects late still sees them on the next tick
    // instead of waiting for another PRM_SET. One write per cycle, since the
    // bounds are shared across all nine zones.
    this->tlmWrite_JETSON_BOUNDS(this->m_activeBounds);

    this->jetson_thermalStateMachine_sendSignal_success(); // Transition back to initial state to read temp again on next tick
  }

  void JetsonThermalManager :: scalesSvc_ThermalStateMachine_action_doReadFail(SmId smId, scalesSvc_ThermalStateMachine::Signal signal)
  {
    printf("Failed to read from sensor. Logging failure event...\n");

    this->m_successfulRead = true; // Reset_ success flag for next read attempt
    this->jetson_thermalStateMachine_sendSignal_success(); // Transition back to initial state to try reading again on next tick
  }

  void JetsonThermalManager :: parameterUpdated(FwPrmIdType id) {
    switch (id) {
      case PARAMID_JETSON_BOUNDS:
        this->applyBounds(this->paramGet_JETSON_BOUNDS(m_paramIsValid));
        break;
      default:
        break;
    }
  }

  bool JetsonThermalManager :: readTemp(U8 index, F32& temp) {
    CHAR path[128];
    std::snprintf(path, sizeof(path), this->m_tempPathTemplate, static_cast<unsigned int>(index));

    Os::File tempFile;
    Os::File::Status fileStatus = tempFile.open(path, Os::File::Mode::OPEN_READ);
    if (fileStatus != Os::File::Status::OP_OK) {
      printf("Could not open thermal file: %s at zone: %s\n", path, indexToZone[index].c_str());
      temp = 0.0F; // The caller marks failed reads as NOT_USED so this fallback is not treated as a real temperature
      return false;
    }

    CHAR tempBuffer[TEMP_FILE_BUFFER_SIZE] = {};
    FwSizeType readSize = sizeof(tempBuffer) - 1;
    fileStatus = tempFile.read(reinterpret_cast<U8*>(tempBuffer), readSize, Os::File::WaitType::NO_WAIT);
    tempFile.close();
    if ((fileStatus != Os::File::Status::OP_OK) || (readSize == 0)) {
      printf("Could not read thermal file: %s at zone: %s\n", path, indexToZone[index].c_str());
      temp = 0.0F; // The caller marks failed reads as NOT_USED so this fallback is not treated as a real temperature
      return false;
    }
    tempBuffer[readSize] = '\0';

    I32 tempMilliC = 0;
    CHAR* parseEnd = nullptr;
    Fw::StringUtils::StringToNumberStatus parseStatus =
        Fw::StringUtils::string_to_number(tempBuffer, sizeof(tempBuffer), tempMilliC, &parseEnd, 10);
    if ((parseStatus != Fw::StringUtils::StringToNumberStatus::SUCCESSFUL_CONVERSION) || (parseEnd == nullptr)) {
      printf("Could not parse thermal file: %s at zone: %s\n", path, indexToZone[index].c_str());
      temp = 0.0F; // The caller marks failed reads as NOT_USED so this fallback is not treated as a real temperature
      return false;
    }
    while ((*parseEnd == ' ') || (*parseEnd == '\t') || (*parseEnd == '\r') || (*parseEnd == '\n')) {
      parseEnd++;
    }
    if (*parseEnd != '\0') {
      printf("Could not parse thermal file: %s at zone: %s\n", path, indexToZone[index].c_str());
      temp = 0.0F; // The caller marks failed reads as NOT_USED so this fallback is not treated as a real temperature
      return false;
    }

    temp = static_cast<F32>(tempMilliC) / 1000.0F;
    return true;
  }

  scalesSvc::ThermalStates JetsonThermalManager :: determineTempState(F32 tempCelsius) {
    const F32 faultLow = this->m_activeBounds.get_faultLow();
    const F32 warnLow = this->m_activeBounds.get_warnLow();
    const F32 idleLow = this->m_activeBounds.get_idleLow();
    const F32 idleHigh = this->m_activeBounds.get_idleHigh();
    const F32 warnHigh = this->m_activeBounds.get_warnHigh();
    const F32 faultHigh = this->m_activeBounds.get_faultHigh();

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

  bool JetsonThermalManager :: thresholdsAreOrdered(const scalesSvc::TempBounds& bounds) const {
    return bounds.get_faultLow() <= bounds.get_warnLow() &&
           bounds.get_warnLow() <= bounds.get_idleLow() &&
           bounds.get_idleLow() <= bounds.get_idleHigh() &&
           bounds.get_idleHigh() <= bounds.get_warnHigh() &&
           bounds.get_warnHigh() <= bounds.get_faultHigh();
  }

  void JetsonThermalManager :: applyBounds(const scalesSvc::TempBounds& candidate) {
    if (this->thresholdsAreOrdered(candidate)) {
      this->m_activeBounds = candidate;
      this->tlmWrite_JETSON_BOUNDS(this->m_activeBounds);
    } else {
      this->log_WARNING_HI_THRESHOLDS_MISCONFIGURED(
          Fw::String("JETSON"), candidate.get_faultLow(), candidate.get_warnLow(),
          candidate.get_idleLow(), candidate.get_idleHigh(),
          candidate.get_warnHigh(), candidate.get_faultHigh());
    }
  }
}
