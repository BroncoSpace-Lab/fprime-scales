// ======================================================================
// \title  JetsonManagerTester.hpp
// \brief  hpp file for JetsonManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_JetsonManagerTester_HPP
#define scalesSvc_JetsonManagerTester_HPP

#include "scales/scalesSvc/JetsonManager/JetsonManager.hpp"
#include "scales/scalesSvc/JetsonManager/JetsonManagerGTestBase.hpp"

namespace scalesSvc {

class JetsonManagerTester final : public JetsonManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;

    JetsonManagerTester();
    ~JetsonManagerTester();

    void requestPowerModeDeferredCompletion();
    void requestPowerModeTimeout();
    void requestJetsonPowerStateOnDrivesGpioImmediately();
    void requestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut();
    void requestJetsonPowerStateOffRejectedWhileBooting();
    void requestJetsonPowerStateOffNoLongerRejectedAfterBootConfirmationTimeout();
    void requestJetsonPowerStateOffAcceptedAfterRedundantOnCommand();
    void requestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower();
    void requestJetsonPowerStateOffConfirmedOffIsIdempotent();
    void requestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut();
    void requestJetsonPowerStateRejectedByAuthorization();
    void requestJetsonPowerStateBusyWhilePending();
    void fpJetsonPowerRequestInIgnoresOnAndActsOnOff();
    void currentJetsonPwrStateIgnoredWithoutPendingCommand();

    // Overrides for the synchronous fpJetsonPowerAuthorize output port so
    // tests can control FPManager's authorization result without a real
    // FPManager instance.
    Fw::Success from_fpJetsonPowerAuthorize_handler(FwIndexType portNum,
                                                     const scalesSvc::JetsonPowerStateID& stateReq) override;

  private:
    void connectPorts();
    void initComponents();

    JetsonManager component;
    Fw::Success m_authorizeResult;
};

}  // namespace scalesSvc

#endif
