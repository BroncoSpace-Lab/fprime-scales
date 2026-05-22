// ======================================================================
// \title  McpManager.hpp
// \author bidat
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"

namespace scalesSvc {

  class McpManager :
    public McpManagerComponentBase
  {

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
