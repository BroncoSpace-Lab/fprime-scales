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

  // Write to the target address, -> read two bytes -> check the I2C status 
  // -> combine the two bytes into a 16-bit value
  Drv::I2cStatus InaManager ::
    readRegister16(U8 register_address, U16& value)
    {
      U8 writeData[1] = {register_address};
      U8 readData[2] = {0, 0};

      Fw::Buffer writeBuffer(writeData, sizeof(writeData));
      Fw::Buffer readBuffer(readData, sizeof(readData));

      Drv::I2cStatus status = 
        this->busWriteRead_out(0, INA260_I2C_ADDRESS, writeBuffer, readBuffer);

      if(status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_I2cReadFailed(register_address, status);
        return status;
      }

      value = static_cast<U16>(
        (static_cast<U16>(readData[0]) << 8) |
        static_cast<U16>(readData[1])
      );

      return status;
    }

    // Conversion helper functions to convert raw INA260 register values to 
    // decimal values with appropriate units (mA, mV, mW)
    F32 InaManager ::
      convertCurrentRawToMilliAmps(I16 raw) const
      {
        return static_cast<F32>(raw) * 1.25; // INA260 current LSB is 1.25mA
      }

    F32 InaManager ::
      convertVoltageRawToMilliVolts(I16 raw) const
      {
        return static_cast<F32>(raw) * 1.25; // INA260 voltage LSB is 1.25mV
      }

    F32 InaManager ::
      convertPowerRawToMilliWatts(I16 raw) const
      {
        return static_cast<F32>(raw) * 10.0; // INA260 power LSB is 10mW
      }
  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void InaManager ::
    READ_CURRENT_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    U16 raw = 0;
    Drv::I2cStatus status = this->readRegister16(INA260_REG_CURRENT, raw);

    if (status == Drv::I2cStatus::I2C_OK) {
      I16 signedRaw = static_cast<I16>(raw);
      F32 current_mA = this->convertCurrentRawToMilliAmps(signedRaw);
      this->tlmWrite_Current_mA(current_mA);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
  } 

  void InaManager ::
    READ_VOLTAGE_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    U16 raw = 0;
    Drv::I2cStatus status = this->readRegister16(INA260_REG_VOLTAGE, raw);

    if (status == Drv::I2cStatus::I2C_OK) {
      I16 signedRaw = static_cast<I16>(raw);
      F32 voltage_mV = this->convertVoltageRawToMilliVolts(signedRaw);
      this->tlmWrite_Current_mA(voltage_mV);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
  }

  void InaManager ::
    READ_POWER_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    U16 raw = 0;
    Drv::I2cStatus status = this->readRegister16(INA260_REG_POWER, raw);

    if (status == Drv::I2cStatus::I2C_OK) {
      I16 signedRaw = static_cast<I16>(raw);
      F32 power_mW = this->convertPowerRawToMilliWatts(signedRaw);
      this->tlmWrite_Current_mA(power_mW);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }  
  }

  void InaManager ::
    READ_ALL_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    U16 rawCurrent = 0;
    U16 rawVoltage = 0;
    U16 rawPower = 0;

    if (this->readRegister16(INA260_REG_CURRENT, rawCurrent) != Drv::I2cStatus::I2C_OK || 
        this->readRegister16(INA260_REG_VOLTAGE, rawVoltage) != Drv::I2cStatus::I2C_OK ||
        this->readRegister16(INA260_REG_POWER, rawPower) != Drv::I2cStatus::I2C_OK) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse:: EXECUTION_ERROR);
        return;
      }

      this->tlmWrite_Current_mA(
        this->convertCurrentRawToMilliAmps(static_cast<I16>(rawCurrent))
      );
      this->tlmWrite_Voltage_mV(
        this->convertVoltageRawToMilliVolts(rawVoltage)
      );
      this->tlmWrite_Power_mW(
        this->convertPowerRawToMilliWatts(rawPower)
      );

      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }
}
