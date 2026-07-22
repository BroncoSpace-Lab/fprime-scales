#include "FPManagerTester.hpp"

TEST(FPManager, InitializesSafeModeAndGatesJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.initializesSafeModeAndGatesJetsonOn();
}

TEST(FPManager, EntersHpcModeAndAcceptsJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.entersHpcModeAndAcceptsJetsonOn();
}

TEST(FPManager, DisablesHpcModeAndGatesJetsonOn) {
  scalesSvc::FPManagerTester tester;
  tester.disablesHpcModeAndGatesJetsonOn();
}

TEST(FPManager, AttributesJetsonFaultAndReturnsSafe) {
  scalesSvc::FPManagerTester tester;
  tester.attributesJetsonFaultAndReturnsSafe();
}

TEST(FPManager, FatalShutdownForwardsAndLatches) {
  scalesSvc::FPManagerTester tester;
  tester.fatalShutdownForwardsAndLatches();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
