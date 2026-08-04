// ======================================================================
// \title  InaManagerTester.cpp
// \brief  cpp file for InaManager component test harness implementation class
// ======================================================================

#include "InaManagerTester.hpp"

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
    this->component.deinit();
  }

  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  void InaManagerTester :: setRegister(U32 addr, U8 reg, U16 rawValue) {
    this->m_fakeRegisterValues[std::make_pair(addr, reg)] = rawValue;
  }

  void InaManagerTester :: failRegister(U32 addr, U8 reg, Drv::I2cStatus status) {
    this->m_failRegisters[std::make_pair(addr, reg)] = status;
  }

  void InaManagerTester :: assertPowerReading(
      const scalesSvc::PowerReading& reading,
      F32 expVoltage,
      F32 expCurrent,
      F32 expPower,
      U8 expSourceId,
      const char* expLocation,
      U32 expTimestamp
  ) {
    ASSERT_FLOAT_EQ(reading.get_voltage(), expVoltage);
    ASSERT_FLOAT_EQ(reading.get_current(), expCurrent);
    ASSERT_FLOAT_EQ(reading.get_power(), expPower);
    ASSERT_EQ(reading.get_sourceId(), expSourceId);
    ASSERT_STREQ(reading.get_location().toChar(), expLocation);
    ASSERT_EQ(reading.get_timestamp(), expTimestamp);
  }

  FwSizeType InaManagerTester :: countBusWriteReadCalls(U32 addr, U8 reg) const {
    FwSizeType count = 0;
    for (const auto& call : this->m_busWriteReadCalls) {
      if (call.first == addr && call.second == reg) {
        count++;
      }
    }
    return count;
  }

  Drv::I2cStatus InaManagerTester :: from_busWriteRead_handler(
      FwIndexType portNum,
      U32 addr,
      Fw::Buffer& writeBuffer,
      Fw::Buffer& readBuffer
  ) {
    this->pushFromPortEntry_busWriteRead(addr, writeBuffer, readBuffer);

    const U8 reg = writeBuffer.getData()[0];
    this->m_busWriteReadCalls.push_back(std::make_pair(addr, reg));

    const auto key = std::make_pair(addr, reg);

    const auto failIt = this->m_failRegisters.find(key);
    if (failIt != this->m_failRegisters.end()) {
      return failIt->second;
    }

    U16 raw = 0;
    const auto valIt = this->m_fakeRegisterValues.find(key);
    if (valIt != this->m_fakeRegisterValues.end()) {
      raw = valIt->second;
    }

    readBuffer.getData()[0] = static_cast<U8>((raw >> 8) & 0xFF);
    readBuffer.getData()[1] = static_cast<U8>(raw & 0xFF);

    return Drv::I2cStatus::I2C_OK;
  }

  // ----------------------------------------------------------------------
  // Tests
  // ----------------------------------------------------------------------

  void InaManagerTester :: nominalAllSensorsSucceed() {
    // Jetson: clean round numbers (800 * 1.25mA = 1.0A, 4000 * 1.25mV = 5.0V, 500 * 10mW = 5.0W).
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_VOLTAGE, 4000);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_POWER, 500);

    // OBC: current must truncate, not round (803 * 1.25mA = 1.00375A -> 1.003, not 1.004).
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_CURRENT, 803);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_VOLTAGE, 2400);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_POWER, 200);

    // Peripheral: negative current -- INA260's current register is signed 16-bit.
    // 0xFC18 as I16 is -1000; -1000 * 1.25mA = -1.25A.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT, 0xFC18);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_VOLTAGE, 9600);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_POWER, 1000);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();

    ASSERT_EVENTS_I2cReadFailed_SIZE(0);
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT_SIZE(0);

    ASSERT_TLM_INA260_Jetson_SIZE(1);
    this->assertPowerReading(this->tlmHistory_INA260_Jetson->at(0).arg,
        5.0F, 1.0F, 5.0F, static_cast<U8>(InaManager::INA260_I2C_ADDRESS_JETSON), "JETSON", 0u);

    ASSERT_TLM_INA260_OBC_SIZE(1);
    this->assertPowerReading(this->tlmHistory_INA260_OBC->at(0).arg,
        3.0F, 1.003F, 2.0F, static_cast<U8>(InaManager::INA260_I2C_ADDRESS_OBC), "OBC", 0u);

    ASSERT_TLM_INA260_Peripheral_SIZE(1);
    this->assertPowerReading(this->tlmHistory_INA260_Peripheral->at(0).arg,
        12.0F, -1.25F, 10.0F, static_cast<U8>(InaManager::INA260_I2C_ADDRESS_PERIPHERAL), "PERIPHERAL", 0u);

    // 3 sensors x 3 registers each.
    ASSERT_from_busWriteRead_SIZE(9);

    // inaPowerReadOut must carry exactly the same structs just published as
    // telemetry -- confirms the DataProducer fan-out and the GDS telemetry
    // both see identical data from the same tick, not independently
    // recomputed or stale copies.
    ASSERT_from_inaPowerReadOut_SIZE(1);
    const FromPortEntry_inaPowerReadOut& fanOut = this->fromPortHistory_inaPowerReadOut->at(0);
    ASSERT_EQ(fanOut.obcPowerReading, this->tlmHistory_INA260_OBC->at(0).arg);
    ASSERT_EQ(fanOut.perifPowerReading, this->tlmHistory_INA260_Peripheral->at(0).arg);
    ASSERT_EQ(fanOut.jetsonPowerReading, this->tlmHistory_INA260_Jetson->at(0).arg);
  }

  void InaManagerTester :: conversionHelpersDirect() {
    // 1.25 mA/LSB current, signed 16-bit register.
    ASSERT_FLOAT_EQ(this->component.convertCurrentRawToAmps(800), 1.0F);
    ASSERT_FLOAT_EQ(this->component.convertCurrentRawToAmps(static_cast<U16>(0xFC18)), -1.25F);

    // 1.25 mV/LSB voltage, unsigned.
    ASSERT_FLOAT_EQ(this->component.convertVoltageRawToVolts(4000), 5.0F);

    // 10 mW/LSB power, unsigned.
    ASSERT_FLOAT_EQ(this->component.convertPowerRawToWatts(500), 5.0F);
  }

  void InaManagerTester :: timestampAdvancesAcrossTicks() {
    // Clean, identical readings every tick -- only the timestamp behavior is under test.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_VOLTAGE, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_POWER, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_CURRENT, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_VOLTAGE, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_POWER, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_VOLTAGE, 0);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_POWER, 0);

    // First tick: timestamp is always 0, regardless of the absolute clock
    // value seen at boot -- m_startTime is set to this exact tick's time.
    this->setTestTime(Fw::Time(1000, 0));
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_TLM_INA260_Jetson_SIZE(1);
    ASSERT_EQ(this->tlmHistory_INA260_Jetson->at(0).arg.get_timestamp(), 0u);

    // Second tick, 7 seconds later.
    this->setTestTime(Fw::Time(1007, 0));
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_TLM_INA260_Jetson_SIZE(2);
    ASSERT_EQ(this->tlmHistory_INA260_Jetson->at(1).arg.get_timestamp(), 7u);

    // Third tick, 20 seconds after boot -- confirms it keeps tracking
    // elapsed-since-boot, not just elapsed-since-previous-tick.
    this->setTestTime(Fw::Time(1020, 0));
    this->invoke_to_run(0, 0);
    this->component.doDispatch();
    ASSERT_TLM_INA260_Jetson_SIZE(3);
    ASSERT_EQ(this->tlmHistory_INA260_Jetson->at(2).arg.get_timestamp(), 20u);
  }

  void InaManagerTester :: singleRegisterFailureStopsSubsequentReadsForThatSensor() {
    // Jetson: CURRENT succeeds, VOLTAGE fails -- POWER must never be attempted.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT, 800);
    this->failRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_VOLTAGE, Drv::I2cStatus::I2C_READ_ERR);

    // OBC and Peripheral: unaffected, succeed normally.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_VOLTAGE, 4000);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_POWER, 500);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_VOLTAGE, 4000);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_POWER, 500);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();

    ASSERT_EVENTS_I2cReadFailed_SIZE(1);
    ASSERT_EVENTS_I2cReadFailed(0,
        static_cast<U8>(InaManager::INA260_REG_VOLTAGE),
        static_cast<I32>(Drv::I2cStatus::I2C_READ_ERR));
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT_SIZE(1);
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT(0, "JETSON");

    // Jetson gets no telemetry at all this tick; OBC and Peripheral are unaffected.
    ASSERT_TLM_INA260_Jetson_SIZE(0);
    ASSERT_TLM_INA260_OBC_SIZE(1);
    ASSERT_TLM_INA260_Peripheral_SIZE(1);

    // The read pipeline actually stopped after the VOLTAGE failure -- POWER
    // was never attempted for Jetson, unlike the two healthy sensors.
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_POWER), 0u);
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT), 1u);
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_VOLTAGE), 1u);
    // Jetson: 2 calls (CURRENT + VOLTAGE). OBC and Peripheral: 3 each. Total 8.
    ASSERT_from_busWriteRead_SIZE(8);

    // DataProducer still gets a reading this tick despite the partial failure.
    ASSERT_from_inaPowerReadOut_SIZE(1);
  }

  void InaManagerTester :: allSensorsFailStillForwardsToDataProducer() {
    this->failRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT, Drv::I2cStatus::I2C_READ_ERR);
    this->failRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_CURRENT, Drv::I2cStatus::I2C_READ_ERR);
    this->failRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT, Drv::I2cStatus::I2C_READ_ERR);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();

    ASSERT_EVENTS_I2cReadFailed_SIZE(3);
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT_SIZE(3);

    ASSERT_TLM_INA260_Jetson_SIZE(0);
    ASSERT_TLM_INA260_OBC_SIZE(0);
    ASSERT_TLM_INA260_Peripheral_SIZE(0);

    // Each sensor stops at its first (and only attempted) register.
    ASSERT_from_busWriteRead_SIZE(3);

    // SSM still gets a reading to act on even when every sensor failed this
    // tick (INA-002) -- silence here would leave fault protection with
    // nothing to evaluate.
    ASSERT_from_inaPowerReadOut_SIZE(1);
  }

  void InaManagerTester :: powerRegisterFailureAfterCurrentAndVoltageSucceed() {
    // Peripheral: CURRENT and VOLTAGE succeed, POWER fails -- the only way
    // to reach readSensorOnce's third and final failure branch.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_VOLTAGE, 4000);
    this->failRegister(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_POWER, Drv::I2cStatus::I2C_READ_ERR);

    // Jetson and OBC: unaffected, succeed normally.
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_VOLTAGE, 4000);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_JETSON, InaManager::INA260_REG_POWER, 500);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_CURRENT, 800);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_VOLTAGE, 4000);
    this->setRegister(InaManager::INA260_I2C_ADDRESS_OBC, InaManager::INA260_REG_POWER, 500);

    this->invoke_to_run(0, 0);
    this->component.doDispatch();

    ASSERT_EVENTS_I2cReadFailed_SIZE(1);
    ASSERT_EVENTS_I2cReadFailed(0,
        static_cast<U8>(InaManager::INA260_REG_POWER),
        static_cast<I32>(Drv::I2cStatus::I2C_READ_ERR));
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT_SIZE(1);
    ASSERT_EVENTS_FAIL_TO_READ_PWR_AT(0, "PERIPHERAL");

    ASSERT_TLM_INA260_Peripheral_SIZE(0);
    ASSERT_TLM_INA260_Jetson_SIZE(1);
    ASSERT_TLM_INA260_OBC_SIZE(1);

    // All three registers were attempted for Peripheral -- unlike the
    // VOLTAGE-failure case, the failure here is the last one in the sequence.
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_CURRENT), 1u);
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_VOLTAGE), 1u);
    ASSERT_EQ(this->countBusWriteReadCalls(InaManager::INA260_I2C_ADDRESS_PERIPHERAL, InaManager::INA260_REG_POWER), 1u);
    ASSERT_from_busWriteRead_SIZE(9);

    ASSERT_from_inaPowerReadOut_SIZE(1);
  }

}
