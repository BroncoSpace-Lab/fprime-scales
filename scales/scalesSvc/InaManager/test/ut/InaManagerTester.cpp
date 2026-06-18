// ======================================================================
// \title  InaManagerTester.cpp
// \author jacob
// \brief  cpp file for InaManager component test harness implementation class
// ======================================================================

#include "InaManagerTester.hpp"

#include <gtest/gtest.h>

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Construction and destruction
  // ----------------------------------------------------------------------

  InaManagerTester ::
    InaManagerTester() :
      InaManagerGTestBase("InaManagerTester", InaManagerTester::MAX_HISTORY_SIZE),
      component("InaManager")
  {
    this->initComponents();
    this->connectPorts();
  }

  InaManagerTester ::
    ~InaManagerTester()
  {

  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void InaManagerTester ::
    testNominalReadAllSensors()
  {
    this->clearHistory();

    // Raw values:
    // current: 1000 * 0.00125 A = 1.25 A
    // voltage: 10000 * 0.00125 V = 12.5 V
    // power: 250 * 0.010 W = 2.5 W
    this->enqueueSuccessfulSensorRead(JETSON_ADDR, 1000, 10000, 250);

    // Raw values:
    // current: 2000 * 0.00125 A = 2.5 A
    // voltage: 8000 * 0.00125 V = 10.0 V
    // power: 123 * 0.010 W = 1.23 W
    this->enqueueSuccessfulSensorRead(OBC_ADDR, 2000, 8000, 123);

    // Raw values:
    // current: 400 * 0.00125 A = 0.5 A
    // voltage: 5000 * 0.00125 V = 6.25 V
    // power: 150 * 0.010 W = 1.5 W
    this->enqueueSuccessfulSensorRead(PERIPHERAL_ADDR, 400, 5000, 150);

    this->invokeRunAndDispatch();

    ASSERT_EQ(0, this->m_i2cResponses.size());

    ASSERT_EVENTS_SIZE(0);

    ASSERT_TLM_INA260_Jetson_SIZE(1);
    ASSERT_TLM_INA260_OBC_SIZE(1);
    ASSERT_TLM_INA260_Peripheral_SIZE(1);

    const PowerReading expectedJetson = 
      this->makeExpectedReading(1.25F, 12.5F, 2.5F, JETSON_ADDR, 0);
    
    const PowerReading expectedObc =
      this->makeExpectedReading(2.5F, 10.0F, 1.23F, OBC_ADDR, 0);

    const PowerReading expectedPeripheral = 
      this->makeExpectedReading(0.5F, 6.25F, 1.5F, PERIPHERAL_ADDR, 0);

    ASSERT_TLM_INA260_Jetson(0, expectedJetson);
    ASSERT_TLM_INA260_OBC(0, expectedObc);
    ASSERT_TLM_INA260_Peripheral(0, expectedPeripheral);
  }

  void InaManagerTester ::
    testI2cFailureSkipsFailedSensorAndLogsEvent()
  {
    this->clearHistory();

    // Fail immediately on Jetson current register.
    this->enqueueI2cResponse(
      JETSON_ADDR,
      REG_CURRENT,
      0,
      Drv::I2cStatus::I2C_ADDRESS_ERR
    );

    // OBC and Peripheral should still be read successfully.
    this->enqueueSuccessfulSensorRead(OBC_ADDR, 2000, 8000, 123);
    this->enqueueSuccessfulSensorRead(PERIPHERAL_ADDR, 400, 5000, 150);

    this->invokeRunAndDispatch();

    ASSERT_EQ(0, this->m_i2cResponses.size());

    ASSERT_EVENTS_I2cReadFailed_SIZE(1);
    ASSERT_EVENTS_I2cReadFailed(
      0,
      REG_CURRENT,
      static_cast<I32>(Drv::I2cStatus::I2C_ADDRESS_ERR)
    );

    // Jetson failed, so no Jetson telemetry should be emitted.
    ASSERT_TLM_INA260_Jetson_SIZE(0);

    ASSERT_TLM_INA260_OBC_SIZE(1);
    ASSERT_TLM_INA260_Peripheral_SIZE(1);

    const PowerReading expectedObc = 
      this->makeExpectedReading(2.5F, 10.0F, 1.23F, OBC_ADDR, 0);
    
    const PowerReading expectedPeripheral =
      this->makeExpectedReading(0.5F, 6.25F, 1.5F, PERIPHERAL_ADDR, 0);

    ASSERT_TLM_INA260_OBC(0, expectedObc);
    ASSERT_TLM_INA260_Peripheral(0, expectedPeripheral);

  }

  // ----------------------------------------------------------------------
  // Helper methods
  // ----------------------------------------------------------------------

  void InaManagerTester :: 
    enqueueSuccessfulSensorRead(
      U32 sensorAddress,
      U16 rawCurrent,
      U16 rawVoltage,
      U16 rawPower
    )
  {
    this->enqueueI2cResponse(sensorAddress, REG_CURRENT, rawCurrent);
    this->enqueueI2cResponse(sensorAddress, REG_VOLTAGE, rawVoltage);
    this->enqueueI2cResponse(sensorAddress, REG_POWER, rawPower);
  }

  void InaManagerTester ::
    enqueueI2cResponse(
      U32 sensorAddress,
      U8 registerAddress,
      U16 rawValue,
      Drv::I2cStatus status
    )
  {
    I2cReadResponse response = {};
    response.sensorAddress = sensorAddress;
    response.registerAddress = registerAddress;
    response.rawValue = rawValue;
    response.status = status;
    this->m_i2cResponses.push_back(response);
  }

  void InaManagerTester ::
    invokeRunAndDispatch(U32 context)
  {
    this->component.run_handler(0, context);
  }

  PowerReading InaManagerTester ::
    makeExpectedReading(
      F32 current,
      F32 voltage,
      F32 power,
      U32 sourceId,
      U32 timestamp
    )
  {
    PowerReading reading = {};
    reading.setcurrent(current);
    reading.setvoltage(voltage);
    reading.setpower(power);
    reading.setsourceId(sourceId);
    reading.settimestamp(timestamp);
    return reading;
  }

  // ----------------------------------------------------------------------
  // From-port handlers
  // ----------------------------------------------------------------------

  Drv::I2cStatus InaManagerTester ::
    from_busWriteRead_handler (
      FwIndexType portNum,
      U32 addr,
      Fw::Buffer& writeBuffer,
      Fw::Buffer& readBuffer
    )
  {
    EXPECT_EQ(0, portNum);
    EXPECT_EQ(1, writeBuffer.getSize());
    EXPECT_EQ(2, readBuffer.getSize());

    const I2cReadResponse response = this->m_i2cResponses.front();
    this->m_i2cResponses.pop_front();

    const U8 requestedRegister = writeBuffer.getData()[0];

    EXPECT_EQ(response.sensorAddress, addr);
    EXPECT_EQ(response.registerAddress, requestedRegister);

    if (response.status == Drv::I2cStatus::I2C_OK) {
      readBuffer.getData()[0] = static_cast<U8>((response.rawValue >> 8) & 0xFF);
      readBuffer.getData()[1] = static_cast<U8>(response.rawValue & 0xFF);
    }

    return response.status;
  }
  
}
