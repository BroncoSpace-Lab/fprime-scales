// ======================================================================
// \title  ImxThermalManagerTester.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component test harness implementation class
// ======================================================================

#include "ImxThermalManagerTester.hpp"
#include "fstream"


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
  void ImxThermalManagerTester ::
    ImxThermalManagerTesting()
  {
    this->component.loadParameters();
    //Cycle 1: Fail Case
    invoke_to_imxCpuTemp(0,0); //invoked the run handler
    this->component.doDispatch(); // async port handler, queues tick
    this->component.doDispatch(); // tick in INIT, runs doRead, queues fail
    this->component.doDispatch(); // fail signal, enters FAIL

    invoke_to_imxCpuTemp(0,0); // invoked the runhandler
    this->component.doDispatch(); // async port handler, queues tick
    this->component.doDispatch(); // tick in FAIL, runs readFail, emits telemetry

    ASSERT_TLM_imx_cpu_temp_read_SIZE(1);
    const ThermalReading& failedRead = this->tlmHistory_imx_cpu_temp_read->at(0).arg; //remember & means that ThermalReading is itself of this instance. 
    ASSERT_STREQ(failedRead.getlocation().toChar(), "FAILED_READ");

    // End Fail case test

    // Make fake temp file: 42 C = 42000 milli-C
    const char* fakeTempPath = "/tmp/imx_cpu_temp_test";
    F32 tempC = 42.0f;

    std::ofstream fakeTempFile(fakeTempPath);
    fakeTempFile << tempC * 1000.0f << "\n";
    fakeTempFile.close();

    this->component.setTempPath(fakeTempPath);

    // We are currently in FAIL.
    // This tick runs readFail(), sees the file exists, queues success.
    invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch(); // run handler queues tick
    this->component.doDispatch(); // tick in FAIL runs readFail, queues success
    this->component.doDispatch(); // success moves FAIL -> INIT

    // Now in INIT.
    // This tick runs doRead(), reads fake temp file, queues success.
    invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch(); // run handler queues tick
    this->component.doDispatch(); // tick in INIT runs doRead, queues success
    this->component.doDispatch(); // success moves INIT -> EVALUATE

    // Now in EVALUATE.
    // This tick runs paramEvaluate(), writes telemetry.
    invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch(); // run handler queues tick
    this->component.doDispatch(); // tick in EVALUATE runs paramEvaluate, writes telemetry
    this->component.doDispatch(); // success moves EVALUATE -> INIT

    ASSERT_TLM_imx_cpu_temp_read_SIZE(2);

    const ThermalReading& read = this->tlmHistory_imx_cpu_temp_read->at(1).arg;

    ASSERT_FLOAT_EQ(read.gettemperature(), tempC);
    ASSERT_STREQ(read.getlocation().toChar(), "CPU");
    ASSERT_EQ(read.gettempState(), scalesSvc::ThermalStates::IDLE);




    // Make fake temp file: 80 C = 80000 milli-C
    const char* fakeTempPath_2 = "/tmp/imx_cpu_temp_test_2";
    tempC = 75.0f;

    std::ofstream fakeTempFile_2(fakeTempPath);
    fakeTempFile_2 << tempC * 1000.0f << "\n";
    fakeTempFile_2.close();

    // read the 80.0 value
    invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch(); // run handler queues tick
    this->component.doDispatch(); // tick in Failed runs doRead
    this->component.doDispatch(); // success
    
    invoke_to_imxCpuTemp(0, 0);
    this->component.doDispatch(); // run handler queues tick
    this->component.doDispatch(); // tick in INIT runs doRead
    this->component.doDispatch(); // success moves INIT -> EVALUATE

    ASSERT_TLM_imx_cpu_temp_read_SIZE(3);
    const ThermalReading& read_2 = this->tlmHistory_imx_cpu_temp_read->at(2).arg;
    ASSERT_FLOAT_EQ(read_2.gettemperature(), tempC);
    ASSERT_STREQ(read_2.getlocation().toChar(), "CPU");
    ASSERT_EQ(read_2.gettempState(), scalesSvc::ThermalStates::WARN);

    // Make fake temp file: 75 C = 75000 milli-C
tempC = 80.0f;

std::ofstream fakeTempFile_3(fakeTempPath);
fakeTempFile_3 << tempC * 1000.0f << "\n";
fakeTempFile_3.close();

// Read the 75.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // pending success moves EVALUATE -> INIT
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in INIT runs doRead, queues success

// Evaluate the 75.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // success moves INIT -> EVALUATE
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in EVALUATE runs paramEvaluate, writes telemetry

ASSERT_TLM_imx_cpu_temp_read_SIZE(4);
const ThermalReading& read_3 = this->tlmHistory_imx_cpu_temp_read->at(3).arg;
ASSERT_FLOAT_EQ(read_3.gettemperature(), tempC);
ASSERT_STREQ(read_3.getlocation().toChar(), "CPU");
ASSERT_EQ(read_3.gettempState(), scalesSvc::ThermalStates::FAULT);

// Make fake temp file: 0 C = 0 milli-C
tempC = 0.0f;

std::ofstream fakeTempFile_4(fakeTempPath);
fakeTempFile_4 << tempC * 1000.0f << "\n";
fakeTempFile_4.close();

// Read the 0.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // pending success moves EVALUATE -> INIT
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in INIT runs doRead, queues success

// Evaluate the 0.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // success moves INIT -> EVALUATE
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in EVALUATE runs paramEvaluate, writes telemetry

ASSERT_TLM_imx_cpu_temp_read_SIZE(5);
const ThermalReading& read_4 = this->tlmHistory_imx_cpu_temp_read->at(4).arg;
ASSERT_FLOAT_EQ(read_4.gettemperature(), tempC);
ASSERT_STREQ(read_4.getlocation().toChar(), "CPU");
ASSERT_EQ(read_4.gettempState(), scalesSvc::ThermalStates::WARN);

// Make fake temp file: 30 C = 30000 milli-C
tempC = -30.0f;

std::ofstream fakeTempFile_5(fakeTempPath);
fakeTempFile_5 << tempC * 1000.0f << "\n";
fakeTempFile_5.close();

// Read the 30.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // pending success moves EVALUATE -> INIT
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in INIT runs doRead, queues success

// Evaluate the 30.0 value
invoke_to_imxCpuTemp(0, 0);
this->component.doDispatch(); // success moves INIT -> EVALUATE
this->component.doDispatch(); // run handler queues tick
this->component.doDispatch(); // tick in EVALUATE runs paramEvaluate, writes telemetry

ASSERT_TLM_imx_cpu_temp_read_SIZE(6);
const ThermalReading& read_5 = this->tlmHistory_imx_cpu_temp_read->at(5).arg;
ASSERT_FLOAT_EQ(read_5.gettemperature(), tempC);
ASSERT_STREQ(read_5.getlocation().toChar(), "CPU");
ASSERT_EQ(read_5.gettempState(), scalesSvc::ThermalStates::FAULT);


  }





}
