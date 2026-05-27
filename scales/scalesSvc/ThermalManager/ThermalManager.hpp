// ======================================================================
// \title  ThermalManager.hpp
// \author luquitolanzi
// \brief  hpp file for ThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_ThermalManager_HPP
#define scalesSvc_ThermalManager_HPP

#include "scales/scalesSvc/ThermalManager/ThermalManagerComponentAc.hpp"

namespace scalesSvc {

class ThermalManager : public ThermalManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ThermalManager object
    ThermalManager(const char* const compName  //!< The component name
    );

    //! Destroy ThermalManager object
    ~ThermalManager();

  PRIVATE:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for imxCpuTemp
    void imxCpuTemp_handler(FwIndexType portNum,  //!< The port number
                            U32 context           //!< The call order
                            ) override;
    
    scalesSvc::ThermalReading cpu_thermal_read;                        
};

}  // namespace scalesSvc

#endif