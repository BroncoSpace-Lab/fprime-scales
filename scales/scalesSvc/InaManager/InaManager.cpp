// ======================================================================
// \title  InaManager.cpp
// \author jacob
// \brief  cpp file for InaManager component implementation class
// ======================================================================

#include "scales/scalesSvc/InaManager/InaManager.hpp"
#include <cmath>

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

    // Evaluate the start time of time
    if (m_justBooted == true) {
      m_justBooted = false;
      m_startTime = getTime().getSeconds();
    }
  
    // Evaluate the current time by subtracting the start time from the current time
    U32 currentTime = getTime().getSeconds() - m_startTime; 

    // Set the current time for each object's timestamp variable
    jetsonData.settimestamp(currentTime);
    obcData.settimestamp(currentTime);
    peripheralData.settimestamp(currentTime);

    // Write to the telemetry channel for each INA260 sensor
    if (this->readSensorOnce(jetsonData)) {
      this->tlmWrite_INA260_Jetson(jetsonData);
    }

    if (this->readSensorOnce(obcData)) {
      this->tlmWrite_INA260_OBC(obcData);
    }

    if (this->readSensorOnce(peripheralData)) {
      this->tlmWrite_INA260_Peripheral(peripheralData);
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
      PowerReading& sensorData
    )
  {
    U16 rawCurrent = 0;
    U16 rawVoltage = 0;
    U16 rawPower = 0;

    if (this->readRegister16(sensorData.getsourceId(), INA260_REG_CURRENT, rawCurrent) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorData.getsourceId(), INA260_REG_VOLTAGE, rawVoltage) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorData.getsourceId(), INA260_REG_POWER, rawPower) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    // Set the class member value of current with 3 decimal places
    sensorData.setcurrent(
      std::trunc(this->convertCurrentRawToAmps(rawCurrent) * 1000.0f) / 1000.0f
    );

    // Set the class member value of votlage with 3 decimal places
    sensorData.setvoltage(
      std::trunc(this->convertVoltageRawToVolts(rawVoltage) * 1000.0f) / 1000.0f
    );

    // Set the class member value of power with 3 decimal places
    sensorData.setpower(
      std::trunc(this->convertPowerRawToWatts(rawPower) / 1000.0f) / 1000.0f
    );

    return true;
  }
}