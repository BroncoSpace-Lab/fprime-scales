// ======================================================================
// \title  InaManager.cpp
// \author jacob
// \brief  cpp file for InaManager component implementation class
// ======================================================================

#include "scales/scalesSvc/InaManager/InaManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  InaManager ::
    InaManager(const char* const compName) :
      InaManagerComponentBase(compName)
  {

  }

  InaManager ::
    ~InaManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void InaManager ::
    run_handler(
      FwIndexType portNum,
      U32 context
    )
  {
    (void)(portNum);
    (void)(context);

    F32 jetson_current = 0.0F, jetson_voltage = 0.0F, jetson_power = 0.0F;
    F32 OBC_current = 0.0F, OBC_voltage = 0.0F, OBC_power = 0.0F;
    F32 peripheral_current = 0.0F, peripheral_voltage = 0.0F, peripheral_power = 0.0F;

    if (this->readSensorOnce(
      INA260_I2C_ADDRESS_JETSON,
      jetson_current,
      jetson_voltage,
      jetson_power

      )) {
      this->tlmWrite_INA260_Jetson_Current_Amps(jetson_current);
      this->tlmWrite_INA260_Jetson_Voltage_Volts(jetson_voltage);
      this->tlmWrite_INA260_Jetson_Power_Watts(jetson_power);
    }

    if (this->readSensorOnce(
      INA260_I2C_ADDRESS_OBC,
      OBC_current,
      OBC_voltage,
      OBC_power

      )) {
      this->tlmWrite_INA260_OBC_Current_Amps(OBC_current);
      this->tlmWrite_INA260_OBC_Voltage_Volts(OBC_voltage);
      this->tlmWrite_INA260_OBC_Power_Watts(OBC_power);
    }

    if (this->readSensorOnce(
      INA260_I2C_ADDRESS_PERIPHERAL,
      peripheral_current,
      peripheral_voltage,
      peripheral_power

      )) {
      this->tlmWrite_INA260_Peripheral_Current_Amps(peripheral_current);
      this->tlmWrite_INA260_Peripheral_Voltage_Volts(peripheral_voltage);
      this->tlmWrite_INA260_Peripheral_Power_Watts(peripheral_power);
    }
  }

  // ----------------------------------------------------------------------
  // Helper Functions
  // ----------------------------------------------------------------------

  // Write to the target address, -> read two bytes -> check the I2C status 
  // -> combine the two bytes into a 16-bit value -> return the value
  Drv::I2cStatus InaManager ::
    readRegister16(U32 sensorAddress, U8 registerAddress, U16& value)
  {
    U8 writeData[1] = {registerAddress};
    U8 readData[2] = {0, 0};

    Fw::Buffer writeBuffer(writeData, sizeof(writeData));
    Fw::Buffer readBuffer(readData, sizeof(readData));

    Drv::I2cStatus status = 
      this->busWriteRead_out(0, sensorAddress, writeBuffer, readBuffer);

    if(status != Drv::I2cStatus::I2C_OK) {
      this->log_WARNING_HI_I2cReadFailed(registerAddress, static_cast<I32>(status.e));
      return status;
    }

    value = 
      static_cast<U16>(
        (static_cast<U16>(readData[0]) << 8) |
         static_cast<U16>(readData[1])
      );

    return status;
  }

  // Conversion helper functions to convert raw INA260 register values to 
  // decimal values with appropriate units (A, V, W)
  F32 InaManager ::
    convertCurrentRawToAmps(U16 raw) const
    {
      const I16 signedRaw = static_cast<I16>(raw);
      return static_cast<F32>(signedRaw) * 0.00125F; // INA260 current LSB is 1.25mA
    }

  F32 InaManager ::
    convertVoltageRawToVolts(U16 raw) const
    {
      return static_cast<F32>(raw) * 0.00125F; // INA260 voltage LSB is 1.25mV
    }

  F32 InaManager ::
    convertPowerRawToWatts(U16 raw) const
    {
      return static_cast<F32>(raw) * 0.010F; // INA260 power LSB is 10mW
    }

  bool InaManager :: 
    readSensorOnce(
      U32 sensorAddress,
      F32& current,
      F32& voltage,
      F32& power
    )
  {
    U16 rawCurrent = 0;
    U16 rawVoltage = 0;
    U16 rawPower = 0;

    if (this->readRegister16(sensorAddress, INA260_REG_CURRENT, rawCurrent) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorAddress, INA260_REG_VOLTAGE, rawVoltage) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorAddress, INA260_REG_POWER, rawPower) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    current = this->convertCurrentRawToAmps(rawCurrent);
    voltage = this->convertVoltageRawToVolts(rawVoltage);
    power = this->convertPowerRawToWatts(rawPower);

    return true;
  }
}
