// ======================================================================
// \title  ImxThermalManager.hpp
// \author luquito
// \brief  hpp file for ImxThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_ImxThermalManager_HPP
#define scalesSvc_ImxThermalManager_HPP

#include "scales/scalesSvc/ImxThermalManager/ImxThermalManagerComponentAc.hpp"


namespace scalesSvc {

  class ImxThermalManager :
    public ImxThermalManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct ImxThermalManager object
      ImxThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy ImxThermalManager object
      ~ImxThermalManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command TODO
      //!
      //! Handler implementation for imxCpuTemp
    void imxCpuTemp_handler(FwIndexType portNum,  //!< The port number
                            U32 context           //!< The call order
                            ) override;
    
    scalesSvc::ThermalReading cpu_thermal_read;  
    
    // Path to the imx_cpu temperature file
    const char* tempPath = "/sys/class/thermal/thermal_zone0/temp";

  };

}

#endif
