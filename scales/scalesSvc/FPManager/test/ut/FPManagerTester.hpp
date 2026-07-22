#ifndef scalesSvc_FPManagerTester_HPP
#define scalesSvc_FPManagerTester_HPP

#include "scales/scalesSvc/FPManager/FPManager.hpp"
#include "scales/scalesSvc/FPManager/FPManagerGTestBase.hpp"

namespace scalesSvc {

class FPManagerTester final : public FPManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 40;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;

    FPManagerTester();
    ~FPManagerTester();

    void initializesSafeModeAndGatesJetsonOn();
    void entersHpcModeAndAcceptsJetsonOn();
    void attributesJetsonFaultAndReturnsSafe();
    void fatalShutdownForwardsAndLatches();

  private:
    void connectPorts();
    void initComponents();

    void drainStateMachine();
    ThermalReading reading(U8 sensorId, ThermalStates state, F32 temperature,
                           const char* location, U32 timestamp);
    void initializeSafeMode();
    void enterHpcMode();

    FPManager component;
};

}  // namespace scalesSvc

#endif
