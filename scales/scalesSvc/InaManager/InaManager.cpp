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
    jetsonData.set_sourceId(INA260_I2C_ADDRESS_JETSON);
    obcData.set_sourceId(INA260_I2C_ADDRESS_OBC);
    peripheralData.set_sourceId(INA260_I2C_ADDRESS_PERIPHERAL);
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
    jetsonData.set_timestamp(currentTime);
    obcData.set_timestamp(currentTime);
    peripheralData.set_timestamp(currentTime);

    // Dispatch current queued messages
    FwSizeType numMsgs = this->m_queue.getMessagesAvailable();
    for (FwSizeType i = 0; i < numMsgs; ++i) {
        (void) this->doDispatch();
    }

    // Write to the telemetry channel for each INA260 sensor
    if (this->readSensorOnce(jetsonData)) {
      jetsonData.set_location(Fw::String("JETSON"));
      this->tlmWrite_INA260_Jetson(jetsonData);
    } else {
      this->log_WARNING_HI_FAIL_TO_READ_PWR_AT(Fw::String("JETSON"));
    }

    if (this->readSensorOnce(obcData)) {
      obcData.set_location(Fw::String("OBC"));
      this->tlmWrite_INA260_OBC(obcData);
    } else {
      this->log_WARNING_HI_FAIL_TO_READ_PWR_AT(Fw::String("OBC"));
    }

    if (this->readSensorOnce(peripheralData)) {
      peripheralData.set_location(Fw::String("PERIPHERAL"));
      this->tlmWrite_INA260_Peripheral(peripheralData);
    } else {
      this->log_WARNING_HI_FAIL_TO_READ_PWR_AT(Fw::String("PERIPHERAL"));
    }

    // Send to DataProducer
    this->inaPowerReadOut_out(0, obcData, peripheralData, jetsonData);
  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void InaManager ::SET_AVERAGE_MODE_cmdHandler(FwOpcodeType opCode, 
                                                U32 cmdSeq, 
                                                scalesSvc::InaSubsytem subsystem,
                                                scalesSvc::InaAverageMode mode) {
      U32 DEVICE_ADDRESS;
      switch(static_cast<InaSubsytem::T>(subsystem.e)){
        case InaSubsytem::JETSON:
          DEVICE_ADDRESS = INA260_I2C_ADDRESS_JETSON;
          break;
        case InaSubsytem::OBC:
          DEVICE_ADDRESS = INA260_I2C_ADDRESS_OBC;
          break;
        case InaSubsytem::PERIPHERAL:
          DEVICE_ADDRESS = INA260_I2C_ADDRESS_PERIPHERAL;
          break;
        default:
          this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
          return;
      }

      // Read current value of the configuration register
      U16 configValue;
      if(this->readRegister16(DEVICE_ADDRESS, INA260_REG_CONFIG, configValue) != Drv::I2cStatus::I2C_OK){
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
      }
      
      // Clear averaging bits 11:9
      configValue &= ~(0x0E00);

      // Set averaging bits 11:9 to corresponding modes
      switch(static_cast<InaAverageMode::T>(mode.e)){
        case InaAverageMode::_1_:
          configValue = configValue | 0x0000; 
          break;
        case InaAverageMode::_4_:
          configValue = configValue | 0x0200;
          break;
        case InaAverageMode::_16_:
          configValue = configValue | 0x0400;
          break;
        case InaAverageMode::_64_:
          configValue = configValue | 0x0600;
          break;
        case InaAverageMode::_128_:
          configValue = configValue | 0x0800;
          break;
        case InaAverageMode::_256_:
          configValue = configValue | 0x0A00;
          break;
        case InaAverageMode::_512_:
          configValue = configValue | 0x0C00;
          break;
        case InaAverageMode::_1024_:
          configValue = configValue | 0x0E00;
          break;
        default:
          this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
          return;
      }

      if(this->writeRegister16(DEVICE_ADDRESS, INA260_REG_CONFIG, configValue) != Drv::I2cStatus::I2C_OK){
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
      }
      
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
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

  Drv::I2cStatus  InaManager ::writeRegister16(U32 sensorAddress, U8 registerAddress, U16 value){
    U8 writeData[3] = {registerAddress, static_cast<U8>( (value >> 8) & 0xFF), static_cast<U8>(value & 0xFF)};
    Fw::Buffer writeBuffer(writeData, sizeof(writeData));

    Drv::I2cStatus status = this->busWrite_out(0, sensorAddress, writeBuffer);
    if(status != Drv::I2cStatus::I2C_OK) {
      this->log_WARNING_HI_I2cWriteFailed(registerAddress, static_cast<I32>(status.e));
    }

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

    if (this->readRegister16(sensorData.get_sourceId(), INA260_REG_CURRENT, rawCurrent) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorData.get_sourceId(), INA260_REG_VOLTAGE, rawVoltage) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    if (this->readRegister16(sensorData.get_sourceId(), INA260_REG_POWER, rawPower) != Drv::I2cStatus::I2C_OK) {
      return false;
    }

    // Set the class member value of current with 3 decimal places
    sensorData.set_current(
      std::trunc(this->convertCurrentRawToAmps(rawCurrent) * 1000.0f) / 1000.0f
    );

    // Set the class member value of votlage with 3 decimal places
    sensorData.set_voltage(
      std::trunc(this->convertVoltageRawToVolts(rawVoltage) * 1000.0f) / 1000.0f
    );

    // Set the class member value of power with 3 decimal places
    sensorData.set_power(
      std::trunc(this->convertPowerRawToWatts(rawPower) * 1000.0f) / 1000.0f
    );

    return true;
  }
}
