// ======================================================================
// \title  InaManager.hpp
// \author jacob
// \brief  hpp file for InaManager component implementation class
// ======================================================================

#ifndef scalesSvc_InaManager_HPP
#define scalesSvc_InaManager_HPP

#include "scales/scalesSvc/InaManager/InaManagerComponentAc.hpp"
namespace scalesSvc {

  class InaManager :
    public InaManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct InaManager object
      InaManager(
          const char* const compName //!< The component name
      );

      //! Destroy InaManager object
      ~InaManager();

    PRIVATE:


      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for run
      //!
      //! Input port for sending data each tick
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command READ_CURRENT
      //!
      //! Command to read current
      void READ_CURRENT_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      //! Handler implementation for command READ_VOLTAGE
      //!
      //! Command to read voltage
      void READ_VOLTAGE_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      //! Handler implementation for command READ_POWER
      //!
      //! Command to read power
      void READ_POWER_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      //! Handler implementation for command READ_ALL
      //!
      //! Command to read all values
      void READ_ALL_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;


      Drv::I2cStatus readRegister16(U8 register_address, U16& value);

      F32 convertCurrentRawToMilliAmps(U16 raw) const;
      F32 convertVoltageRawToMilliVolts(U16 raw) const;
      F32 convertPowerRawToMilliWatts(U16 raw) const;

      bool readSensorOnce(F32& current_mA, F32& voltage_mV, F32& power_mW);

      static constexpr U32 INA260_I2C_ADDRESS = 0x41;

      static constexpr U32 INA260_REG_CURRENT = 0x01;
      static constexpr U32 INA260_REG_VOLTAGE = 0x02;
      static constexpr U32 INA260_REG_POWER = 0x03;


  };

}

#endif
