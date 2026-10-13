// ======================================================================
// \title  HubComAdapterTestMain.cpp
// \brief  cpp file for HubComAdapter component test main function
// ======================================================================

#include "HubComAdapterTester.hpp"

TEST(Nominal, BufferInForwardsToComOutWithDefaultContext) {
    RecordProperty("requirement", "HCA-001");
    scalesSvc::HubComAdapterTester tester;
    tester.bufferInForwardsToComOutWithDefaultContext();
}

TEST(Nominal, ComReturnInForwardsToBufferInReturn) {
    RecordProperty("requirement", "HCA-002");
    scalesSvc::HubComAdapterTester tester;
    tester.comReturnInForwardsToBufferInReturn();
}

TEST(Nominal, ComInForwardsToBufferOut) {
    RecordProperty("requirement", "HCA-003");
    scalesSvc::HubComAdapterTester tester;
    tester.comInForwardsToBufferOut();
}

TEST(Nominal, BufferOutReturnForwardsToComInReturnWithDefaultContext) {
    RecordProperty("requirement", "HCA-004");
    scalesSvc::HubComAdapterTester tester;
    tester.bufferOutReturnForwardsToComInReturnWithDefaultContext();
}

TEST(Nominal, ComStatusInFansOutToAllConnectedIndices) {
    RecordProperty("requirement", "HCA-005");
    scalesSvc::HubComAdapterTester tester;
    tester.comStatusInFansOutToAllConnectedIndices();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
