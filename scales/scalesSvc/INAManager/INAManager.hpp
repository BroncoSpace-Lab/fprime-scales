// ======================================================================
// \title  INAManager.hpp
// \author jacob
// \brief  hpp file for INAManager component implementation class
// ======================================================================

#ifndef scalesSvc_INAManager_HPP
#define scalesSvc_INAManager_HPP

#include "scales/scalesSvc/INAManager/INAManagerComponentAc.hpp"

namespace scalesSvc {

  class INAManager :
    public INAManagerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct INAManager object
      INAManager(
          const char* const compName //!< The component name
      );

      //! Destroy INAManager object
      ~INAManager();

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command TODO
      //!
      //! TODO
      void TODO_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

  };

}

#endif
