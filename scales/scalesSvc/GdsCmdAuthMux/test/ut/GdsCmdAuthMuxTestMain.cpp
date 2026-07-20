// ======================================================================
// \title  GdsCmdAuthMuxTestMain.cpp
// \author luquitolanzi
// \brief  cpp file for GdsCmdAuthMux component test main function
// ======================================================================

#include "GdsCmdAuthMuxTester.hpp"

TEST(GdsCmdAuthMux, StartupWithTcpUp) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.startupWithTcpUp();
}

TEST(GdsCmdAuthMux, StartupWithTcpDownAndGracePeriod) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.startupWithTcpDownAndGracePeriod();
}

TEST(GdsCmdAuthMux, TcpRecoveryDuringGracePeriod) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.tcpRecoveryDuringGracePeriod();
}

TEST(GdsCmdAuthMux, CommandGatingAndResponseRouting) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.commandGatingAndResponseRouting();
}

TEST(GdsCmdAuthMux, RecoveryAndManualReturnToTcp) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.recoveryAndManualReturnToTcp();
}

TEST(GdsCmdAuthMux, MalformedCommandsAndFailureRecovery) {
    scalesSvc::GdsCmdAuthMuxTester tester;
    tester.malformedCommandsAndFailureRecovery();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
