// ======================================================================
// \title  HubComAdapter.hpp
// \brief  Buffer/communications interface adapter for GenericHub framing
// ======================================================================

#ifndef scales_scalesSvc_HubComAdapter_HPP
#define scales_scalesSvc_HubComAdapter_HPP

#include "scales/scalesSvc/HubComAdapter/HubComAdapterComponentAc.hpp"

namespace scalesSvc {

class HubComAdapter final : public HubComAdapterComponentBase {
  public:
    explicit HubComAdapter(const char* const compName);
    ~HubComAdapter() override;

  private:
    void bufferIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void comReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const ComCfg::FrameContext& context) override;
    void comIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const ComCfg::FrameContext& context) override;
    void bufferOutReturn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
};

}  // namespace scalesSvc

#endif
