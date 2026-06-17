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

    private:

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

      static constexpr U32 INA260_REG_CURRENT = 0x01; // current register address
      static constexpr U32 INA260_REG_VOLTAGE = 0x02; // voltage register address
      static constexpr U32 INA260_REG_POWER = 0x03; // power register address

      static constexpr U32 INA260_I2C_ADDRESS_JETSON = 0x40; // Jetson subsystem INA260 address
      static constexpr U32 INA260_I2C_ADDRESS_OBC = 0x41; // OBC subsystem INA260 address
      static constexpr U32 INA260_I2C_ADDRESS_PERIPHERAL = 0x45; // Peripheral subsystem INA260 address

      PowerReading jetsonData = {0, 0, 0, INA260_I2C_ADDRESS_JETSON, 0};
      PowerReading obcData = {0, 0, 0, INA260_I2C_ADDRESS_OBC, 0};
      PowerReading peripheralData = {0, 0, 0, INA260_I2C_ADDRESS_PERIPHERAL, 0};

      // Functions to convert raw data from registers to usable values
      F32 convertCurrentRawToAmps(U16 raw) const;
      F32 convertVoltageRawToVolts(U16 raw) const;
      F32 convertPowerRawToWatts(U16 raw) const;

      // Function to read the 16-bit value stored in a register at a specified address
      Drv::I2cStatus readRegister16(U32 sensorAddress, U8 registerAddress, U16& value);

      // Function that confirms data has been successfully read from each register
      bool readSensorOnce(PowerReading& sensordata);

      // Time helpers
      bool m_justBooted = true;
      U32 m_startTime = 0;

  };

}

#endif