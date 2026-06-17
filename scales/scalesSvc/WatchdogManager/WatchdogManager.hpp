// ======================================================================
// \title  WatchdogManager.hpp
// \author lucal
// \brief  hpp file for WatchdogManager component implementation class
// ======================================================================

#ifndef scalesSvc_WatchdogManager_HPP
#define scalesSvc_WatchdogManager_HPP

#include "Fw/Types/EnabledEnumAc.hpp"
#include "scales/scalesSvc/WatchdogManager/WatchdogManagerComponentAc.hpp"

namespace scalesSvc {

  class WatchdogManager :
    public WatchdogManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct WatchdogManager object
      WatchdogManager(
          const char* const compName //!< The component name
      );

      //! Destroy WatchdogManager object
      ~WatchdogManager();

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------
      Fw::Enabled m_wdStatus = Fw::Enabled::DISABLED; //Initial state for the watchdog
      Fw::ParamValid m_isValid = Fw::ParamValid::VALID; //instantiate valid type for paramget
      bool m_isPetting = false;
      const U32 m_intervalSec = 1;
       //! Handler implementation for run
      U32 m_startTimeSec;
      //! run handler
      void run_handler(
          FwIndexType portNum, //!< The port number
          U32 context //!< The call order
      ) override;

  };

}

#endif
