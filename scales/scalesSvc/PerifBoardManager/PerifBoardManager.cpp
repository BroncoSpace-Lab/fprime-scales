// ======================================================================
// \title  PerifBoardManager.cpp
// \author luquitolanzi
// \brief  cpp file for PerifBoardManager component implementation class
// ======================================================================

#include "scales/scalesSvc/PerifBoardManager/PerifBoardManager.hpp" 

namespace scalesSvc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

PerifBoardManager ::PerifBoardManager(const char* const compName) : PerifBoardManagerComponentBase(compName) {}

PerifBoardManager ::~PerifBoardManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void PerifBoardManager ::perifBoardManager_handler(FwIndexType portNum, U32 context) {

    Fw::Logic val = Fw::Logic::HIGH;
    this->gpioSet_out(0, val);
    this->gpioGet_out(0, val);
    this->tlmWrite_perif_board_state(this->gpioGet_out(0,val));

}

}  // namespace scalesSvc
