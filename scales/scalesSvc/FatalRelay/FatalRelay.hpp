// ======================================================================
// \title  FatalRelay.hpp
// \brief  Passive FATAL-event relay standing in for CdhCore's fatalHandler
// ======================================================================

#ifndef scales_scalesSvc_FatalRelay_HPP
#define scales_scalesSvc_FatalRelay_HPP

#include "scales/scalesSvc/FatalRelay/FatalRelayComponentAc.hpp"

namespace scalesSvc {

class FatalRelay final : public FatalRelayComponentBase {
  public:
    explicit FatalRelay(const char* const compName);
    ~FatalRelay() override;

  private:
    void FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) override;
};

}  // namespace scalesSvc

#endif
