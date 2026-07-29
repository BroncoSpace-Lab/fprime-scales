// ======================================================================
// \title  HubComAdapterTester.hpp
// \brief  hpp file for HubComAdapter component test harness implementation class
// ======================================================================

#ifndef scalesSvc_HubComAdapterTester_HPP
#define scalesSvc_HubComAdapterTester_HPP

#include "scales/scalesSvc/HubComAdapter/HubComAdapter.hpp"
#include "scales/scalesSvc/HubComAdapter/HubComAdapterGTestBase.hpp"

namespace scalesSvc {

class HubComAdapterTester final : public HubComAdapterGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 10;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    HubComAdapterTester();
    ~HubComAdapterTester();

    void bufferInForwardsToComOutWithDefaultContext();
    void comReturnInForwardsToBufferInReturn();
    void comInForwardsToBufferOut();
    void bufferOutReturnForwardsToComInReturnWithDefaultContext();

  private:
    void connectPorts();
    void initComponents();

    HubComAdapter component;
};

}  // namespace scalesSvc

#endif
