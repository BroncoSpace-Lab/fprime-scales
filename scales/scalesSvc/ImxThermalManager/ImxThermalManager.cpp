// ======================================================================
// \title  ImxThermalManager.cpp
// \author luquito
// \brief  cpp file for ImxThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManager.hpp"
#include <fstream>
#include <iostream>


namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  ImxThermalManager ::
    ImxThermalManager(const char* const compName) :
      ImxThermalManagerComponentBase(compName)
  {

  }

  ImxThermalManager ::
    ~ImxThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void ImxThermalManager ::imxCpuTemp_handler(FwIndexType portNum, U32 context) {

    // Variable to store the raw temperature from Linux
    float tempMilliC = 0.0f;

    // Open the temperature file
    std::ifstream tempFile(tempPath);

    // Read the raw temperature value into the variable
    tempFile >> tempMilliC;

    // Convert from millidegrees Celsius to Celsius
    F32 tempC = tempMilliC / 1000.0f;

    // Send temp value to GDS
    
    (this->cpu_thermal_read).settemperature(tempC);
    (this->cpu_thermal_read).setsensorId(0);
    (this->cpu_thermal_read).setlocation(Fw::String("CPU"));
    (this->cpu_thermal_read).settimestamp(this->getTime().getSeconds());
    this->tlmWrite_imx_cpu_temp_read(this->cpu_thermal_read);

}


}
