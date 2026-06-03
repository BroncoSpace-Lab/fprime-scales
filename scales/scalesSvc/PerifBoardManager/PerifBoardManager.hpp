// ======================================================================
// \title  PerifBoardManager.hpp
// \author luquitolanzi
// \brief  hpp file for PerifBoardManager component implementation class
// ======================================================================

#ifndef scalesSvc_PerifBoardManager_HPP
#define scalesSvc_PerifBoardManager_HPP

#include "scales/scalesSvc/PerifBoardManager/PerifBoardManagerComponentAc.hpp"

namespace scalesSvc {

class PerifBoardManager : public PerifBoardManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct PerifBoardManager object
    PerifBoardManager(const char* const compName  //!< The component name
    );

    //! Destroy PerifBoardManager object
    ~PerifBoardManager();

  PRIVATE:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for perifBoardManager
    //!
    //! input port tied to a rate group that keeps the GPIO toggling the ethernet switch load switch Enabled
    void perifBoardManager_handler(FwIndexType portNum,  //!< The port number
                                   U32 context           //!< The call order
                                   ) override;
};

}  // namespace scalesSvc

#endif
