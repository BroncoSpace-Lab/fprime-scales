// ======================================================================
// \title  JetsonThermalManager.cpp
// \author scales
// \brief  cpp file for JetsonThermalManager component implementation class
// ======================================================================

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManager.hpp"

namespace scalesSvc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  JetsonThermalManager ::
    JetsonThermalManager(const char* const compName) :
      JetsonThermalManagerComponentBase(compName)
  {

  }

  JetsonThermalManager ::
    ~JetsonThermalManager()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  void JetsonThermalManager ::
    jetsonTempRead_handler(
        FwIndexType portNum,
        U32 context
    )
  {
    for (U8 i = 0; i < 9; i++)
    {
      float tempMilliC = 0.0f;
      char path[128];
      std::snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%u/temp", i);
      std::ifstream tempFile(path);
      tempFile >> tempMilliC;
      F32 tempC = tempMilliC / 1000.0f;

      switch (i) {
        case 0:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(0);
          m_jetsonThermalReadings[i].setlocation(Fw::String("CPU"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_cpu_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 1:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(1);
          m_jetsonThermalReadings[i].setlocation(Fw::String("GPU"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_gpu_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 2:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(2);
          m_jetsonThermalReadings[i].setlocation(Fw::String("CV0"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_cv0_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 3:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(3);
          m_jetsonThermalReadings[i].setlocation(Fw::String("CV1"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_cv1_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 4:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(4);
          m_jetsonThermalReadings[i].setlocation(Fw::String("CV2"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_cv2_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 5:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(5);
          m_jetsonThermalReadings[i].setlocation(Fw::String("SOC0"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_soc0_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 6:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(6);
          m_jetsonThermalReadings[i].setlocation(Fw::String("SOC1"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_soc1_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 7:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(7);
          m_jetsonThermalReadings[i].setlocation(Fw::String("SOC2"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_soc2_temp_read(this->m_jetsonThermalReadings[i]);
          break;
        case 8:
          m_jetsonThermalReadings[i].settemperature(tempC);
          m_jetsonThermalReadings[i].setsensorId(8);
          m_jetsonThermalReadings[i].setlocation(Fw::String("TJ"));
          m_jetsonThermalReadings[i].settimestamp(this->getTime().getSeconds());
          this->tlmWrite_jetson_tj_temp_read(this->m_jetsonThermalReadings[i]);
          break;
      }
  
    }

  }
}
