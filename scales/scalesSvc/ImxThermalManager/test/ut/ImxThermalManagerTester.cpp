// ======================================================================
// \title  ImxThermalManagerTester.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component test harness implementation class
// ======================================================================

#include "ImxThermalManagerTester.hpp"
#include <Os/File.hpp>
#include <cstdio>
#include <unistd.h>


namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManagerTester ::
    ImxThermalManagerTester() :
      ImxThermalManagerGTestBase("ImxThermalManagerTester", ImxThermalManagerTester::MAX_HISTORY_SIZE),
      component("ImxThermalManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  ImxThermalManagerTester ::
    ~ImxThermalManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------
  void ImxThermalManagerTester :: writeTemperatureFile(const char* path, F32 tempC)
  {
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

  void ImxThermalManagerTester :: runTickAction()
  {
    this->invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch();
    this->component.doDispatch();
  }

  void ImxThermalManagerTester :: readAndEvaluateTemperature()
  {
    this->runTickAction();
    this->component.doDispatch();

    this->runTickAction();
    this->component.doDispatch();
  }

  void ImxThermalManagerTester :: assertLatestReading(
      FwSizeType expectedHistorySize,
      F32 tempC,
      scalesSvc::ThermalStates expectedState
  )
  {
    ASSERT_TLM_imx_cpu_temp_read_SIZE(expectedHistorySize);
    const ThermalReading& read = this->tlmHistory_imx_cpu_temp_read->at(expectedHistorySize - 1).arg;
    ASSERT_FLOAT_EQ(read.gettemperature(), tempC);
    ASSERT_STREQ(read.getlocation().toChar(), "CPU");
    ASSERT_EQ(read.gettempState(), expectedState);
  }

  void ImxThermalManagerTester ::
    ImxThermalManagerTesting()
  {
    this->component.loadParameters();
    CHAR fakeTempPath[128];
    std::snprintf(fakeTempPath, sizeof(fakeTempPath), "/tmp/imx_cpu_temp_test_%ld", static_cast<long>(::getpid()));
    this->component.setTempPath(fakeTempPath);

    this->runTickAction();
    this->component.doDispatch();

    this->runTickAction();
    this->component.doDispatch();

    ASSERT_TLM_imx_cpu_temp_read_SIZE(1);
    const ThermalReading& failedRead = this->tlmHistory_imx_cpu_temp_read->at(0).arg;
    ASSERT_STREQ(failedRead.getlocation().toChar(), "FAILED_READ");

    this->writeTemperatureFile(fakeTempPath, 42.0F);
    this->runTickAction();
    this->component.doDispatch();
    this->readAndEvaluateTemperature();
    this->assertLatestReading(2, 42.0F, scalesSvc::ThermalStates::IDLE);

    this->writeTemperatureFile(fakeTempPath, 75.0F);
    this->readAndEvaluateTemperature();
    this->assertLatestReading(3, 75.0F, scalesSvc::ThermalStates::WARN);

    this->writeTemperatureFile(fakeTempPath, 80.0F);
    this->readAndEvaluateTemperature();
    this->assertLatestReading(4, 80.0F, scalesSvc::ThermalStates::FAULT);

    this->writeTemperatureFile(fakeTempPath, 0.0F);
    this->readAndEvaluateTemperature();
    this->assertLatestReading(5, 0.0F, scalesSvc::ThermalStates::WARN);

    this->writeTemperatureFile(fakeTempPath, -30.0F);
    this->readAndEvaluateTemperature();
    this->assertLatestReading(6, -30.0F, scalesSvc::ThermalStates::FAULT);
  }
}
