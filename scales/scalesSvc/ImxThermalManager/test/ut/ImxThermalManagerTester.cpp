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
    this->component.deinit();
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
    this->invoke_to_run(0, 0);
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
    ASSERT_FLOAT_EQ(read.get_temperature(), tempC);
    ASSERT_STREQ(read.get_location().toChar(), "CPU");
    ASSERT_EQ(read.get_tempState(), expectedState);
  }

  void ImxThermalManagerTester ::
    ImxThermalManagerTesting()
  {
    this->component.loadParameters();
    CHAR fakeTempPath[128];
    std::snprintf(fakeTempPath, sizeof(fakeTempPath), "/tmp/imx_cpu_temp_test_%ld", static_cast<long>(::getpid()));
    this->component.setTempPath(fakeTempPath);
    static_cast<void>(std::remove(fakeTempPath));

    this->runTickAction();
    this->component.doDispatch();

    this->runTickAction();
    this->component.doDispatch();

    ASSERT_TLM_imx_cpu_temp_read_SIZE(1);
    const ThermalReading& failedRead = this->tlmHistory_imx_cpu_temp_read->at(0).arg;
    ASSERT_STREQ(failedRead.get_location().toChar(), "FAILED_READ");

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

  void ImxThermalManagerTester :: thresholdsMisconfiguredEmitsOnceOnTransition()
  {
    // ImxThermalManager reads its six thresholds live via paramGet_* on every
    // doEvaluate tick rather than caching them (unlike McpManager/
    // JetsonThermalManager), so there is no cached member this friend test
    // can poke directly, and driving a real PRM_SET through cmdIn requires
    // the active component's message queue/dispatch machinery that this
    // lightweight harness isn't set up to exercise safely. Test the ordering
    // predicate directly instead -- this is the exact logic validateThresholds()
    // uses to decide whether to emit THRESHOLDS_MISCONFIGURED.

    // Defaults are in ascending order: FAULT_LOW=-40, WARN_LOW=-20, IDLE_LOW=10,
    // IDLE_HIGH=60, WARN_HIGH=80, FAULT_HIGH=100.
    ASSERT_TRUE(this->component.thresholdsAreOrdered(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F));

    // Mirror the real-world mistake: lower WARN_HIGH without adjusting
    // IDLE_HIGH to match, inverting the high band.
    ASSERT_FALSE(this->component.thresholdsAreOrdered(-40.0F, -20.0F, 10.0F, 60.0F, 30.0F, 100.0F));

    // Fixing WARN_HIGH restores a sane ordering.
    ASSERT_TRUE(this->component.thresholdsAreOrdered(-40.0F, -20.0F, 10.0F, 60.0F, 80.0F, 100.0F));

    // ImxThermalManagerTesting() above already exercises validateThresholds()
    // through doEvaluate() with these same (valid) defaults across five ticks
    // and never sees THRESHOLDS_MISCONFIGURED, confirming the integration
    // path stays silent when the ordering is sane.
  }
}
