// ======================================================================
// \title  DataProducerTestMain.cpp
// \brief  cpp file for DataProducer component test main function
// ======================================================================

#include "DataProducerTester.hpp"

TEST(Nominal, DisabledByDefaultBlocksContainerInit) {
    RecordProperty("requirement", "DP-001");
    scalesSvc::DataProducerTester tester;
    tester.disabledByDefaultBlocksContainerInit();
}

TEST(Nominal, EnableDataProductsInitializesAllContainers) {
    RecordProperty("requirement", "DP-001");
    scalesSvc::DataProducerTester tester;
    tester.enableDataProductsInitializesAllContainers();
}

TEST(Nominal, DisableDataProductsInvalidatesContainers) {
    RecordProperty("requirement", "DP-001,DP-002");
    scalesSvc::DataProducerTester tester;
    tester.disableDataProductsInvalidatesContainers();
}

TEST(Nominal, McpReadingIgnoredWhileContainerInvalid) {
    RecordProperty("requirement", "DP-003");
    scalesSvc::DataProducerTester tester;
    tester.mcpReadingIgnoredWhileContainerInvalid();
}

TEST(Nominal, McpReadingBatchesAndSendsAfterRecordCount) {
    RecordProperty("requirement", "DP-003");
    scalesSvc::DataProducerTester tester;
    tester.mcpReadingBatchesAndSendsAfterRecordCount();
}

TEST(Nominal, JetsonZoneReadingBatchesAndSendsAfterRecordCount) {
    RecordProperty("requirement", "DP-003");
    scalesSvc::DataProducerTester tester;
    tester.jetsonZoneReadingBatchesAndSendsAfterRecordCount();
}

TEST(Nominal, CpuReadingBatchesAndSendsAfterRecordCount) {
    RecordProperty("requirement", "DP-003");
    scalesSvc::DataProducerTester tester;
    tester.cpuReadingBatchesAndSendsAfterRecordCount();
}

TEST(Nominal, InaReadingBatchesAndSendsAfterRecordCount) {
    RecordProperty("requirement", "DP-003");
    scalesSvc::DataProducerTester tester;
    tester.inaReadingBatchesAndSendsAfterRecordCount();
}

TEST(Nominal, ContainerReinitializesAfterBatchSend) {
    RecordProperty("requirement", "DP-004");
    scalesSvc::DataProducerTester tester;
    tester.containerReinitializesAfterBatchSend();
}

TEST(Nominal, ContainerGetFailureLeavesContainerInvalid) {
    RecordProperty("requirement", "DP-005");
    scalesSvc::DataProducerTester tester;
    tester.containerGetFailureLeavesContainerInvalid();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
