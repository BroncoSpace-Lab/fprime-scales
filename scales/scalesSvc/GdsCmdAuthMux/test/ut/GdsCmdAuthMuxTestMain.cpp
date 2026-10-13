// ======================================================================
// \title  GdsCmdAuthMuxTestMain.cpp
// \author luquitolanzi
// \brief  cpp file for GdsCmdAuthMux component test main function
// ======================================================================

#include "GdsCmdAuthMuxTester.hpp"

TEST(GdsCmdAuthMux, StartupWithTcpUp) {
    RecordProperty("requirement", "GCA-001");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.startupWithTcpUp();
}

TEST(GdsCmdAuthMux, StartupWithTcpDownAndGracePeriod) {
    RecordProperty("requirement", "GCA-002");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.startupWithTcpDownAndGracePeriod();
}

TEST(GdsCmdAuthMux, TcpRecoveryDuringGracePeriod) {
    RecordProperty("requirement", "GCA-003");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.tcpRecoveryDuringGracePeriod();
}

TEST(GdsCmdAuthMux, CommandGatingAndResponseRouting) {
    RecordProperty("requirement", "GCA-004,GCA-005");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.commandGatingAndResponseRouting();
}

TEST(GdsCmdAuthMux, RecoveryAndManualReturnToTcp) {
    RecordProperty("requirement", "GCA-006,GCA-007,GCA-008");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.recoveryAndManualReturnToTcp();
}

TEST(GdsCmdAuthMux, MalformedCommandsAndFailureRecovery) {
    RecordProperty("requirement", "GCA-009,GCA-010");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.malformedCommandsAndFailureRecovery();
}

TEST(GdsCmdAuthMux, UartAuthoritySteadyStateTick) {
    RecordProperty("requirement", "GCA-002");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.uartAuthoritySteadyStateTick();
}

TEST(GdsCmdAuthMux, TcpStatusPollerDrivesAuthority) {
    RecordProperty("requirement", "GCA-011");
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.tcpStatusPollerDrivesAuthority();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
