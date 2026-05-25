// ======================================================================
// \title  McpManager.cpp
// \author Luca, Dat, and Jabob
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"

F32 convertRawTemp(U8 *rawData); // Forward decleration 

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  McpManager :: McpManager(const char* const compName) :
    McpManagerComponentBase(compName)
  {
    m_thermalReadings[0] = imx_thermalReadings;
    m_thermalReadings[1] = peripheral_thermalReadings;
    m_thermalReadings[2] = jetson_thermalReadings;

    deviceAddrs[0] = IMX_TEMP_ADDR;
    deviceAddrs[1] = PERIPHERAL_TEMP_ADDR;
    deviceAddrs[2] = JETSON_TEMP_ADDR;
  }

  McpManager :: ~McpManager() {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void McpManager :: pollTempData_handler( FwIndexType portNum, U32 context) {
    
    // Set thermal readings for each sensor
    for (int i = 0; i < 3; i++){
      this->m_thermalReadings[i].settemperature(this->readTemp(deviceAddrs[i])); 
      this->m_thermalReadings[i].setsensorId(i + 1);
      this->m_thermalReadings[i].settimestamp(this->getTime().getSeconds());
      this->m_thermalReadings[i].setlocation(Fw::String("On board MCP9808 sensor"));
    }

    // Logs to each sensor's respective telemetry channel
    this->tlmWrite_IMX_TEMP(m_thermalReadings[0]);
    this->tlmWrite_PERIPHERAL_TEMP(m_thermalReadings[1]);
    this->tlmWrite_JETSON_TEMP(m_thermalReadings[2]);
  }

  // ----------------------------------------------------------------------
  // Helper functions
  // ----------------------------------------------------------------------

  F32 McpManager :: readTemp(U8 deviceAddr)
  {
    U8 regAddr = TEMP_REG_ADDR;
    U8 rawData[2]; // MCP9808 temperature read back data is 2 bytes
    Fw::Buffer writeBuffer(&regAddr, 1);
    Fw::Buffer readBuffer(rawData, 2);

    // Port call to bus driver to write register address and read data
    Drv::I2cStatus status = this->mcpWriteRead_out(0, deviceAddr, writeBuffer, readBuffer);

    if (status == Drv::I2cStatus::I2C_OK){
      return convertRawTemp(rawData); // Convert raw data to Celsius and return
    }
  
    return -67.0; // Return an error value if the device address is unrecognized 
  }

}

// Helper function to convert raw 2-byte data from MCP9808 into a temperature value in Celsius
F32 convertRawTemp(U8 *rawData){

  // rawData[0] == UpperByte
  // rawData[1] == LowerByte

  F32 tempCelsius = 0.0;

  rawData[0] &= 0x1F; // Clear flag bits , keep only temperature data

  if ((rawData[0] & 0x10) == 0x10) { // Check if temperature is negative
    rawData[0] = rawData[0] & 0x0F; // Clear sign bit

    tempCelsius = 256 - ((rawData[0] * 16.0) + (rawData[1] / 16.0)); // Calculate negative temperature according to datasheet
    return tempCelsius;

  } else {
    tempCelsius = ((rawData[0] * 16.0) + (rawData[1] / 16.0)); // Calculate positive temperature according to datasheet
    return tempCelsius;
  }

}