// ======================================================================
// \title  ThermalManager.cpp
// \author luquitolanzi
// \brief  cpp file for ThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/ThermalManager/ThermalManager.hpp"
#include <fstream>
#include <iostream>

// Path to the imx_cpu temperature file
const char* tempPath = "/sys/class/thermal/thermal_zone0/temp";

namespace scalesSvc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ThermalManager ::ThermalManager(const char* const compName) : ThermalManagerComponentBase(compName) {}

ThermalManager ::~ThermalManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ThermalManager ::imxCpuTemp_handler(FwIndexType portNum, U32 context) {

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
    // this->log_ACTIVITY_LO_IMXCPUTEMPREAD(this->cpu_thermal_read);
    this->tlmWrite_imx_cpu_temp_read(this->cpu_thermal_read);

}

}  // namespace scalesSvc
