// ======================================================================
// \title  McpManager.hpp
// \author bidat
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"

namespace scalesSvc {

  class McpManager : public McpManagerComponentBase {

    // Device Address and Target Register Addresses for MCP9808
    public:
    
      static constexpr U8 IMX_TEMP_ADDR = 0x19; //!< I2C address for IMX temperature sensor
      static constexpr U8 PERIPHERAL_TEMP_ADDR = 0x1A; //!< I2C address for peripheral temperature sensor
      static constexpr U8 JETSON_TEMP_ADDR = 0x1B; //!< I2C address for Jetson temperature sensor
      U8 deviceAddrs[3]; //!< Array of device addresses for iterating through sensors
      
      static constexpr U8 TEMP_REG_ADDR = 0x05; //!< Register address for temperature data    

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct McpManager object
      McpManager(
          const char* const compName //!< The component name
      );

      //! Destroy McpManager object
      ~McpManager();

      F32 readTemp(U8 deviceAddr); //!< Function to read temperature from a given I2C device address

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for pollTempData
      //!
      //! Async scheduler input port to poll temp data from the sensors
      void pollTempData_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

    private:
      /* Implementation-specific members */
      scalesSvc::ThermalReading m_thermalReadings[3]; //!< The 3 thermal readings to be logged to telemetry 
      scalesSvc::ThermalReading imx_thermalReadings;
      scalesSvc::ThermalReading peripheral_thermalReadings;
      scalesSvc::ThermalReading jetson_thermalReadings;

  };

}

#endif