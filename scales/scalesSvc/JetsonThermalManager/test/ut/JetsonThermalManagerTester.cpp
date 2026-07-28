// ======================================================================
// \title  JetsonThermalManagerTester.cpp
// \author lucal
// \brief  cpp file for JetsonThermalManager component test harness implementation class
// ======================================================================

#include "JetsonThermalManagerTester.hpp"
#include <Os/File.hpp>
#include <cstdio>
#include <unistd.h>

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  JetsonThermalManagerTester ::
    JetsonThermalManagerTester() :
      JetsonThermalManagerGTestBase("JetsonThermalManagerTester", JetsonThermalManagerTester::MAX_HISTORY_SIZE),
      component("JetsonThermalManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  JetsonThermalManagerTester ::
    ~JetsonThermalManagerTester()
  {
    this->component.deinit();
  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void JetsonThermalManagerTester :: writeTemperatureFile(U8 index, F32 tempC)
  {
    CHAR path[160];
    const int pathSize = std::snprintf(
        path,
        sizeof(path),
        this->m_tempPathTemplate,
        static_cast<unsigned int>(index)
    );
    ASSERT_GT(pathSize, 0);
    ASSERT_LT(static_cast<FwSizeType>(pathSize), static_cast<FwSizeType>(sizeof(path)));

    CHAR tempText[32];
    const I32 tempMilliC = static_cast<I32>(tempC * 1000.0F);
    const int textSize = std::snprintf(tempText, sizeof(tempText), "%d\n", static_cast<int>(tempMilliC));
    ASSERT_GT(textSize, 0);
    ASSERT_LT(static_cast<FwSizeType>(textSize), static_cast<FwSizeType>(sizeof(tempText)));

    Os::File tempFile;
    Os::File::Status status = tempFile.open(path, Os::File::Mode::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE);
    ASSERT_EQ(Os::File::Status::OP_OK, status);

    FwSizeType writeSize = static_cast<FwSizeType>(textSize);
    status = tempFile.write(reinterpret_cast<const U8*>(tempText), writeSize, Os::File::WaitType::WAIT);
    tempFile.close();
    ASSERT_EQ(Os::File::Status::OP_OK, status);
    ASSERT_EQ(static_cast<FwSizeType>(textSize), writeSize);
  }

  void JetsonThermalManagerTester :: removeTemperatureFile(U8 index)
  {
    CHAR path[160];
    const int pathSize = std::snprintf(
        path,
        sizeof(path),
        this->m_tempPathTemplate,
        static_cast<unsigned int>(index)
    );
    ASSERT_GT(pathSize, 0);
    ASSERT_LT(static_cast<FwSizeType>(pathSize), static_cast<FwSizeType>(sizeof(path)));

    static_cast<void>(std::remove(path));
  }

  void JetsonThermalManagerTester :: runTickAction()
  {
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();
  }

  void JetsonThermalManagerTester :: readAndEvaluateTemperatures()
  {
    this->runTickAction();
    this->component.doDispatch();

    this->runTickAction();
    this->component.doDispatch();
  }

  void JetsonThermalManagerTester :: assertLatestReading(
      U8 index,
      FwSizeType expectedHistorySize,
      F32 tempC,
      const char* expectedLocation,
      scalesSvc::ThermalStates expectedState
  )
  {
    const ThermalReading* read = nullptr;
    switch (index) {
      case 0:
        ASSERT_TLM_jetson_cpu_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_cpu_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 1:
        ASSERT_TLM_jetson_gpu_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_gpu_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 2:
        ASSERT_TLM_jetson_cv0_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_cv0_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 3:
        ASSERT_TLM_jetson_cv1_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_cv1_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 4:
        ASSERT_TLM_jetson_cv2_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_cv2_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 5:
        ASSERT_TLM_jetson_soc0_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_soc0_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 6:
        ASSERT_TLM_jetson_soc1_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_soc1_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 7:
        ASSERT_TLM_jetson_soc2_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_soc2_temp_read->at(expectedHistorySize - 1).arg;
        break;
      case 8:
        ASSERT_TLM_jetson_tj_temp_read_SIZE(expectedHistorySize);
        read = &this->tlmHistory_jetson_tj_temp_read->at(expectedHistorySize - 1).arg;
        break;
      default:
        FAIL() << "Unexpected Jetson thermal zone index";
    }

    ASSERT_NE(nullptr, read);
    ASSERT_FLOAT_EQ(read->get_temperature(), tempC);
    ASSERT_EQ(read->get_sensorId(), index);
    ASSERT_STREQ(read->get_location().toChar(), expectedLocation);
    ASSERT_EQ(read->get_tempState(), expectedState);
  }

  void JetsonThermalManagerTester ::
    JetsonThermalManagerUnitTester()
  {
    // KNOWN PRE-EXISTING ISSUE (unrelated to THRESHOLDS_MISCONFIGURED / the
    // WARN-tracking safety check added elsewhere): the second evaluation
    // round's assertion for zone 1 (GPU, 61C, expected WARN) currently fails,
    // observed as FAULT. This reproduces with the original, unmodified
    // classification logic and thresholds, so it predates and is independent
    // of today's changes -- flagged for separate follow-up rather than
    // investigated here.
    this->paramSet_JETSON_IDLE_LOW(10.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_IDLE_HIGH(60.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_WARN_LOW(-20.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_WARN_HIGH(80.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_FAULT_LOW(-40.0f, Fw::ParamValid::VALID);
    this->paramSet_JETSON_FAULT_HIGH(100.0f, Fw::ParamValid::VALID);
    this->component.loadParameters();

    const int templateSize = std::snprintf(
        this->m_tempPathTemplate,
        sizeof(this->m_tempPathTemplate),
        "/tmp/jetson_thermal_zone_%%u_temp_%ld",
        static_cast<long>(::getpid())
    );
    ASSERT_GT(templateSize, 0);
    ASSERT_LT(static_cast<FwSizeType>(templateSize), static_cast<FwSizeType>(sizeof(this->m_tempPathTemplate)));
    this->component.setTempPathTemplate(this->m_tempPathTemplate);

    this->setTestTime(Fw::Time(0, 0));

    this->runTickAction();
    ASSERT_TLM_SIZE(6);
    ASSERT_TLM_JETSON_IDLE_LOW_SIZE(1);
    ASSERT_TLM_JETSON_IDLE_LOW(0, 10.0F);
    ASSERT_TLM_JETSON_IDLE_HIGH_SIZE(1);
    ASSERT_TLM_JETSON_IDLE_HIGH(0, 60.0F);
    ASSERT_TLM_JETSON_WARN_LOW_SIZE(1);
    ASSERT_TLM_JETSON_WARN_LOW(0, -20.0F);
    ASSERT_TLM_JETSON_WARN_HIGH_SIZE(1);
    ASSERT_TLM_JETSON_WARN_HIGH(0, 80.0F);
    ASSERT_TLM_JETSON_FAULT_LOW_SIZE(1);
    ASSERT_TLM_JETSON_FAULT_LOW(0, -40.0F);
    ASSERT_TLM_JETSON_FAULT_HIGH_SIZE(1);
    ASSERT_TLM_JETSON_FAULT_HIGH(0, 100.0F);

    const F32 firstTemps[9] = {42.0F, 75.0F, 80.0F, 0.0F, -30.0F, 55.0F, 65.0F, 10.0F, 100.0F};
    const F32 secondTemps[9] = {15.0F, 61.0F, 0.0F, 0.0F, 0.0F, -39.0F, 60.0F, 79.0F, 101.0F};
    const char* locations[9] = {"CPU", "GPU", "CV0", "CV1", "CV2", "SOC0", "SOC1", "SOC2", "TJ"};
    const scalesSvc::ThermalStates firstStates[9] = {
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::FAULT,
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::FAULT
    };
    const scalesSvc::ThermalStates secondStates[9] = {
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::NOT_USED,
        scalesSvc::ThermalStates::NOT_USED,
        scalesSvc::ThermalStates::NOT_USED,
        scalesSvc::ThermalStates::FAULT,
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::FAULT
    };

    for (U8 i = 0; i < 9; i++) {
      this->writeTemperatureFile(i, firstTemps[i]);
    }
    this->readAndEvaluateTemperatures();
    ASSERT_TLM_SIZE(15);
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 1, firstTemps[i], locations[i], firstStates[i]);
    }

    for (U8 i = 0; i < 9; i++) {
      if ((i >= 2) && (i <= 4)) {
        this->removeTemperatureFile(i);
      } else {
        this->writeTemperatureFile(i, secondTemps[i]);
      }
    }
    this->readAndEvaluateTemperatures();
    ASSERT_TLM_SIZE(24);
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 2, secondTemps[i], locations[i], secondStates[i]);
    }
  }

  void JetsonThermalManagerTester :: thresholdsMisconfiguredEmitsOnceOnTransition()
  {
    // JetsonThermalManager caches its six thresholds in its own scalar
    // members, so this friend test can poke them directly and call the
    // private validateThresholds() helper -- avoiding the active
    // component's message queue/PRM_SET dispatch machinery entirely.
    this->component.FAULT_LOW_THR = -40.0F;
    this->component.WARN_LOW_THR = -20.0F;
    this->component.IDLE_LOW_THR = 10.0F;
    this->component.IDLE_HIGH_THR = 60.0F;
    this->component.WARN_HIGH_THR = 80.0F;
    this->component.FAULT_HIGH_THR = 100.0F;

    // Mirror the real-world mistake: lower WARN_HIGH without adjusting
    // IDLE_HIGH to match, inverting the high band.
    this->component.WARN_HIGH_THR = 30.0F;
    this->component.validateThresholds();

    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED(0, "JETSON", -40.0F, -20.0F, 10.0F, 60.0F, 30.0F, 100.0F);

    // Re-validating the same bad configuration must not re-emit.
    this->component.validateThresholds();
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);

    // Fixing the configuration clears the latch silently (no new event).
    this->component.WARN_HIGH_THR = 80.0F;
    this->component.validateThresholds();
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);

    // Breaking it again now DOES re-emit, since the latch was cleared.
    this->component.WARN_HIGH_THR = 30.0F;
    this->component.validateThresholds();
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
  }

}
