// ======================================================================
// \title  JetsonThermalManager.hpp
// \author scales
// \brief  hpp file for JetsonThermalManager component implementation class
// ======================================================================

#ifndef scalesSvc_JetsonThermalManager_HPP
#define scalesSvc_JetsonThermalManager_HPP

#include "scales/scalesSvc/JetsonThermalManager/JetsonThermalManagerComponentAc.hpp"
#include <fstream>
#include <iostream>


namespace scalesSvc {

  class JetsonThermalManager :
    public JetsonThermalManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct JetsonThermalManager object
      JetsonThermalManager(
          const char* const compName //!< The component name
      );

      //! Destroy JetsonThermalManager object
      ~JetsonThermalManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for jetsonTempRead
      //!
      //! Synchronous input port to handle incoming jetson temp readings
      void jetsonTempRead_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

      scalesSvc::ThermalReading m_jetsonThermalReadings[9]; //!< The 9 thermal zones on the jetson

  };

}

#endif
