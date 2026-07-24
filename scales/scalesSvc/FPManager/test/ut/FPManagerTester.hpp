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
    void emitsStateTransitionEventsOnlyOnChange();
    void entersHpcModeAndAcceptsJetsonOn();
    void disablesHpcModeAndGatesJetsonOn();
    void imxFaultTriggersEmergencyShutdown();
    void peripheralFaultPowersOffPeripheralOnly();
    void peripheralFaultRecoversToSafeMode();
    void faultModeJetsonFaultRequestsOffAndStaysFault();
    void faultModeImxFaultOverridesJetsonAndPeripheral();
    void jetsonFaultReadingTriggersRecoveryInHpc();
    void jetsonFaultRecoveryClearsCachedReadingsBeforeHpcReentry();
    void attributesJetsonFaultAndReturnsSafe();
    void fatalShutdownForwardsAndLatches();
    void rejectsRemoteJetsonCommandWhenJetsonOff();
    void forwardsRemoteJetsonCommandWhenJetsonOn();

  private:
    void connectPorts();
    void initComponents();

    void drainStateMachine();
    ThermalReading reading(U8 sensorId, ThermalStates state, F32 temperature,
                           const char* location, U32 timestamp);
    Fw::ComBuffer commandBuffer(FwOpcodeType opcode);
    void initializeSafeMode();
    void enterHpcMode();

    FPManager component;
};

}  // namespace scalesSvc

#endif
