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
    ThermalReading* read = nullptr;
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
    ASSERT_TLM_SIZE(0);

    const F32 firstTemps[9] = {42.0F, 75.0F, 80.0F, 0.0F, -30.0F, 55.0F, 65.0F, 10.0F, 100.0F};
    const F32 secondTemps[9] = {15.0F, 61.0F, 81.0F, 9.0F, -21.0F, -39.0F, 60.0F, 79.0F, 101.0F};
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
        scalesSvc::ThermalStates::FAULT,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::FAULT,
        scalesSvc::ThermalStates::FAULT,
        scalesSvc::ThermalStates::IDLE,
        scalesSvc::ThermalStates::WARN,
        scalesSvc::ThermalStates::FAULT
    };

    for (U8 i = 0; i < 9; i++) {
      this->writeTemperatureFile(i, firstTemps[i]);
    }
    this->readAndEvaluateTemperatures();
    ASSERT_TLM_SIZE(9);
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 1, firstTemps[i], locations[i], firstStates[i]);
    }

    for (U8 i = 0; i < 9; i++) {
      this->writeTemperatureFile(i, secondTemps[i]);
    }
    this->readAndEvaluateTemperatures();
    ASSERT_TLM_SIZE(18);
    for (U8 i = 0; i < 9; i++) {
      this->assertLatestReading(i, 2, secondTemps[i], locations[i], secondStates[i]);
    }
  }

}
