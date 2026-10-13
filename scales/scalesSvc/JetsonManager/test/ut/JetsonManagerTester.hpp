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
    // 150, not 20: several existing tests (e.g. requestPowerModeTimeout) loop
    // invoke_to_schedIn up to 120 times without an intervening clearHistory(),
    // and fpJetsonHubTrustedOut now fires unconditionally on every schedIn
    // tick (JM-015) -- a too-small history buffer here isn't a graceful test
    // failure, it's a hard FW_ASSERT abort that takes down the whole UT binary.
    static const FwSizeType MAX_HISTORY_SIZE = 150;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;

    JetsonManagerTester();
    ~JetsonManagerTester();

    void requestPowerModeDeferredCompletion();
    void requestPowerModeTimeout();
    void requestPowerModeBusyWhilePending();
    void requestPowerModeRejectedWhenUnconfirmed();
    void requestPowerModeRejectedWhileAwaitingBootConfirmation();
    void requestJetsonPowerStateOnDrivesGpioImmediately();
    void requestJetsonPowerStateOffUnconfirmedFallsBackToDirectCut();
    void requestJetsonPowerStateOffDeferredWhileBootingThenAutoFiresGracefulShutdown();
    void requestJetsonPowerStateOffDeferredButBootNeverConfirmsForcesDirectCutOnTimeout();
    void requestJetsonPowerStateOffFallsBackToDirectCutAfterBootConfirmationTimeoutWithNoDeferredOff();
    void requestJetsonPowerStateOffAcceptedAfterRedundantOnCommand();
    void requestJetsonPowerStateOffDeferredWhileModeChangeInFlightThenAutoFiresOnModeConfirmation();
    void requestJetsonPowerStateOffDeferredWhileModeChangeInFlightResumedAfterModeTimeout();
    void requestJetsonPowerStateOffConfirmedOnUsesGracefulThenCutsPower();
    void requestJetsonPowerStateOffConfirmedOffIsIdempotent();
    void requestJetsonPowerStateOffTimesOutAndFallsBackToDirectCut();
    void requestJetsonPowerStateRejectedByAuthorization();
    void requestJetsonPowerStateBusyWhilePending();
    void fpJetsonPowerRequestInIgnoresOnAndActsOnOff();
    void fpJetsonPowerRequestInDefersOffWhileBootingThenAutoFires();
    void currentJetsonPwrStateIgnoredWithoutPendingCommand();
    void localModeChangeStartedArmsHubTrustGuardThenClearsOnNextReport();
    void localModeChangeStartedDoesNotClobberPendingRequestPowerMode();
    void localModeChangeStartedDefersJetsonOffLikeHubDrivenModeChangePending();
    void schedInRepublishesHubTrustStatusEveryTick();
    void hubLinkDownPreventsTrustEvenWhenModeReportRacesAheadOfReconnect();
    void hubComStatusInLogsTransitionsOnlyOnce();

    // Overrides for the synchronous fpJetsonPowerAuthorize output port so
    // tests can control FPManager's authorization result without a real
    // FPManager instance.
    Fw::Success from_fpJetsonPowerAuthorize_handler(FwIndexType portNum,
                                                     const scalesSvc::JetsonPowerStateID& stateReq) override;

  private:
    void connectPorts();
    void initComponents();

    //! Establishes both preconditions isJetsonHubLinkTrusted() now requires:
    //! a real "Jetson confirmed ON" report AND a real "hub TCP link
    //! connected" report (JM-016). Most existing tests want both true and
    //! don't care about the distinction between them -- this collapses that
    //! setup to one call. Tests that specifically exercise the unconfirmed/
    //! disconnected cases invoke the underlying ports directly instead.
    void confirmJetsonOnAndHubConnected();

    JetsonManager component;
    Fw::Success m_authorizeResult;
};

}  // namespace scalesSvc

#endif
