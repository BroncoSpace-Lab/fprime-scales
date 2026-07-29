// ======================================================================
// \title  HubComAdapterTester.cpp
// \brief  cpp file for HubComAdapter component test harness implementation class
// ======================================================================

#include "HubComAdapterTester.hpp"

namespace scalesSvc {

HubComAdapterTester ::HubComAdapterTester()
    : HubComAdapterGTestBase("HubComAdapterTester", HubComAdapterTester::MAX_HISTORY_SIZE),
      component("HubComAdapter") {
    this->initComponents();
    this->connectPorts();
}

HubComAdapterTester ::~HubComAdapterTester() {}

void HubComAdapterTester ::bufferInForwardsToComOutWithDefaultContext() {
    U8 data[4] = {1, 2, 3, 4};
    Fw::Buffer buffer(data, sizeof(data));

    this->invoke_to_bufferIn(0, buffer);

    ASSERT_from_comOut_SIZE(1);
    const auto& call = this->fromPortHistory_comOut->at(0);
    ASSERT_EQ(call.data.getData(), buffer.getData());
    ASSERT_EQ(call.data.getSize(), buffer.getSize());
    ASSERT_TRUE(call.context == ComCfg::FrameContext());
}

void HubComAdapterTester ::comReturnInForwardsToBufferInReturn() {
    U8 data[4] = {5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ComCfg::FrameContext context;

    this->invoke_to_comReturnIn(0, buffer, context);

    ASSERT_from_bufferInReturn_SIZE(1);
    ASSERT_from_bufferInReturn(0, buffer);
}

void HubComAdapterTester ::comInForwardsToBufferOut() {
    U8 data[4] = {9, 10, 11, 12};
    Fw::Buffer buffer(data, sizeof(data));
    ComCfg::FrameContext context;

    this->invoke_to_comIn(0, buffer, context);

    ASSERT_from_bufferOut_SIZE(1);
    ASSERT_from_bufferOut(0, buffer);
}

void HubComAdapterTester ::bufferOutReturnForwardsToComInReturnWithDefaultContext() {
    U8 data[4] = {13, 14, 15, 16};
    Fw::Buffer buffer(data, sizeof(data));

    this->invoke_to_bufferOutReturn(0, buffer);

    ASSERT_from_comInReturn_SIZE(1);
    const auto& call = this->fromPortHistory_comInReturn->at(0);
    ASSERT_EQ(call.data.getData(), buffer.getData());
    ASSERT_EQ(call.data.getSize(), buffer.getSize());
    ASSERT_TRUE(call.context == ComCfg::FrameContext());
}

}  // namespace scalesSvc
