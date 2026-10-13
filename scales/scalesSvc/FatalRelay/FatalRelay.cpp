// ======================================================================
// \title  FatalRelay.cpp
// \brief  Passive FATAL-event relay standing in for CdhCore's fatalHandler
// ======================================================================

#include "scales/scalesSvc/FatalRelay/FatalRelay.hpp"

namespace scalesSvc {

FatalRelay::FatalRelay(const char* const compName) : FatalRelayComponentBase(compName) {}

FatalRelay::~FatalRelay() = default;

void FatalRelay::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    this->fatalOut_out(0, Id);
}

}  // namespace scalesSvc
