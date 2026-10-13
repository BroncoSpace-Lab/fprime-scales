// ======================================================================
// \title  JetsonThermalManagerTester.cpp
// \author lucal
// \brief  cpp file for JetsonThermalManager component test harness implementation class
// ======================================================================

#include "JetsonThermalManagerTester.hpp"
#include <Os/File.hpp>
#include <cstdio>
#include <cstring>
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

  void JetsonThermalManagerTester :: writeRawTemperatureFile(U8 index, const char* text)
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

    Os::File tempFile;
    Os::File::Status status = tempFile.open(path, Os::File::Mode::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE);
    ASSERT_EQ(Os::File::Status::OP_OK, status);

    FwSizeType writeSize = static_cast<FwSizeType>(std::strlen(text));
    if (writeSize > 0) {
      status = tempFile.write(reinterpret_cast<const U8*>(text), writeSize, Os::File::WaitType::WAIT);
    }
    tempFile.close();
    ASSERT_EQ(Os::File::Status::OP_OK, status);
  }

  void JetsonThermalManagerTester :: setUpTemperatureFiles()
  {
    this->paramSet_JETSON_BOUNDS(
        scalesSvc::TempBounds(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F),
        Fw::ParamValid::VALID);
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

    // First tick only applies the boot-time bounds, it does not read.
    this->runTickAction();
    this->clearHistory();
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
    this->paramSet_JETSON_BOUNDS(
        scalesSvc::TempBounds(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F),
        Fw::ParamValid::VALID);
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
    ASSERT_TLM_SIZE(1);
    ASSERT_TLM_JETSON_BOUNDS_SIZE(1);
    ASSERT_TLM_JETSON_BOUNDS(0, scalesSvc::TempBounds(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F));

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
    ASSERT_TLM_SIZE(11); // 9 zone readings + 1 bounds republish this cycle
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 1, firstTemps[i], locations[i], firstStates[i]);
    }
    ASSERT_EVENTS_SIZE(0); // every zone read cleanly

    for (U8 i = 0; i < 9; i++) {
      if ((i >= 2) && (i <= 4)) {
        this->removeTemperatureFile(i);
      } else {
        this->writeTemperatureFile(i, secondTemps[i]);
      }
    }
    this->readAndEvaluateTemperatures();
    ASSERT_TLM_SIZE(21); // + 9 zone readings + 1 bounds republish this cycle
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 2, secondTemps[i], locations[i], secondStates[i]);
    }

    // The three removed zones each report an open failure; the per-zone
    // failure does not escalate to the whole-read FAIL_TO_READ_TEMP.
    ASSERT_EVENTS_SIZE(3);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(3);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(0, "CV0", scalesSvc::ThermalReadFailure::OPEN);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(1, "CV1", scalesSvc::ThermalReadFailure::OPEN);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(2, "CV2", scalesSvc::ThermalReadFailure::OPEN);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_SIZE(0);

    for (U8 i = 0; i < 9; i++) {
      this->removeTemperatureFile(i);
    }
  }

  void JetsonThermalManagerTester :: boundsUpdateGating()
  {
    // Friend access to applyBounds() lets this lightweight harness exercise
    // the real gating logic directly, without needing to drive a real
    // PRM_SET through cmdIn's active-component message queue/dispatch.

    // A valid, distinct-from-default bounds update is adopted and
    // republished as telemetry.
    const scalesSvc::TempBounds goodBounds(-35.0F, -15.0F, 5.0F, 55.0F, 75.0F, 95.0F);
    this->component.applyBounds(goodBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);
    ASSERT_TLM_JETSON_BOUNDS_SIZE(1);
    ASSERT_TLM_JETSON_BOUNDS(0, goodBounds);

    // Mirror the real-world mistake: lower WARN_HIGH without adjusting
    // IDLE_HIGH to match, inverting the high band. The update must be
    // rejected -- the component keeps using goodBounds.
    const scalesSvc::TempBounds badBounds(-35.0F, -15.0F, 5.0F, 55.0F, 30.0F, 95.0F);
    this->component.applyBounds(badBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(1);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED(0, "JETSON", -35.0F, -15.0F, 5.0F, 55.0F, 30.0F, 95.0F);
    ASSERT_TLM_JETSON_BOUNDS_SIZE(1);

    // Repeating the exact same bad attempt fires again -- every rejection is
    // its own distinct notice, since misconfiguration never takes effect.
    this->component.applyBounds(badBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
    ASSERT_TLM_JETSON_BOUNDS_SIZE(1);

    // A second valid update is adopted normally.
    const scalesSvc::TempBounds otherGoodBounds(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F);
    this->component.applyBounds(otherGoodBounds);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(2);
    ASSERT_TLM_JETSON_BOUNDS_SIZE(2);
    ASSERT_TLM_JETSON_BOUNDS(1, otherGoodBounds);
  }

  void JetsonThermalManagerTester :: readFailureLogsEventPerZone()
  {
    this->setUpTemperatureFiles();

    this->removeTemperatureFile(0);              // CPU: cannot be opened
    this->writeRawTemperatureFile(1, "");        // GPU: opens but reads nothing
    this->writeRawTemperatureFile(2, "abc\n");   // CV0: not a number
    this->writeRawTemperatureFile(3, "42000x\n"); // CV1: number followed by garbage
    for (U8 i = 4; i < 9; i++) {
      this->writeTemperatureFile(i, 42.0F);
    }

    this->readAndEvaluateTemperatures();

    ASSERT_EVENTS_SIZE(4);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(4);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(0, "CPU", scalesSvc::ThermalReadFailure::OPEN);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(1, "GPU", scalesSvc::ThermalReadFailure::READ);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(2, "CV0", scalesSvc::ThermalReadFailure::PARSE);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT(3, "CV1", scalesSvc::ThermalReadFailure::PARSE);

    // The failed zones are published as NOT_USED, the rest evaluate normally.
    this->assertLatestReading(0, 1, 0.0F, "CPU", scalesSvc::ThermalStates::NOT_USED);
    this->assertLatestReading(3, 1, 0.0F, "CV1", scalesSvc::ThermalStates::NOT_USED);
    this->assertLatestReading(4, 1, 42.0F, "CV2", scalesSvc::ThermalStates::IDLE);

    for (U8 i = 0; i < 9; i++) {
      this->removeTemperatureFile(i);
    }
  }

  void JetsonThermalManagerTester :: readFailureEventIsThrottled()
  {
    this->setUpTemperatureFiles();
    for (U8 i = 0; i < 9; i++) {
      this->removeTemperatureFile(i);
    }

    // First cycle: one event per zone.
    this->readAndEvaluateTemperatures();
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(9);

    // Second cycle: nine more failures, but the event is throttled at 10.
    this->readAndEvaluateTemperatures();
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_AT_SIZE(10);
    ASSERT_EVENTS_SIZE(10);
  }

  void JetsonThermalManagerTester :: readFailActionLogsEvent()
  {
    this->setUpTemperatureFiles();
    for (U8 i = 0; i < 9; i++) {
      this->writeTemperatureFile(i, 42.0F);
    }

    // doRead currently never clears m_successfulRead (a failed zone is
    // marked NOT_USED instead), so force the flag to reach the state
    // machine's read-failure action.
    this->component.m_successfulRead = false;
    this->readAndEvaluateTemperatures();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_FAIL_TO_READ_TEMP_SIZE(1);
    ASSERT_TRUE(this->component.m_successfulRead);

    for (U8 i = 0; i < 9; i++) {
      this->removeTemperatureFile(i);
    }
  }

  void JetsonThermalManagerTester :: parameterUpdatedCoverage()
  {
    this->component.loadParameters();
    this->clearHistory();

    // PARAMID_JETSON_BOUNDS = 0x0 (JetsonThermalManagerComponentAc.hpp).
    // boundsUpdateGating() calls applyBounds() directly, bypassing this
    // switch entirely, so it's otherwise never exercised.
    this->component.parameterUpdated(0x0);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);

    // Unrecognized parameter ID: default case, no-op.
    this->component.parameterUpdated(0xDEAD);
    ASSERT_EVENTS_THRESHOLDS_MISCONFIGURED_SIZE(0);
  }

}
