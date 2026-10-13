// ======================================================================
// \title  JetsonPowerModeManagerTester.hpp
// \brief  hpp file for JetsonPowerModeManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_JetsonPowerModeManagerTester_HPP
#define scalesSvc_JetsonPowerModeManagerTester_HPP

#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManager.hpp"
#include "scales/scalesSvc/JetsonPowerModeManager/JetsonPowerModeManagerGTestBase.hpp"

namespace scalesSvc {

class JetsonPowerModeManagerTester final : public JetsonPowerModeManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;

    JetsonPowerModeManagerTester();
    ~JetsonPowerModeManagerTester();

    void powerModeReceiveChangesModeWhenMismatched();
    void powerModeReceiveReportsFailureWhenNvpmodelFails();
    void powerModeReceiveReportsFailureOnPackedNonzeroExit();
    void powerModeReceiveTreatsSigtermAsLikelySuccess();
    void powerModeReceiveNoopWhenAlreadyInMode();
    void powerModeReceiveIgnoredWhileRebootPending();
    void powerModeReceiveClearsRebootPendingOnGenuineFailure();
    void jetsonPowerStateReceiveOnReportsOn();
    void jetsonPowerStateReceiveOffShutsDownGracefully();
    void jetsonPowerStateReceiveOffReportsFailureWhenShutdownFails();
    void jetsonPowerStateReceiveOffReportsFailureOnNonzeroExitNotNegativeOne();
    void jetsonPowerStateReceiveOffTreatsSigtermAsSuccess();
    void schedInReportsOnceAfterBoot();
    void schedInSkipsModeReportOnReaderError();
    void setPowerModeCmdChangesMode();
    void setPowerModeCmdNoopWhenAlreadyInMode();
    void setPowerModeCmdReportsExecutionErrorOnNvpmodelFailure();
    void setPowerModeCmdTreatsSigtermAsSuccess();
    void setPowerModeCmdIgnoredWhileRebootPending();
    void getPowerModeCmdReturnsCurrentMode();
    void getPowerModeCmdValidationErrorOnReaderFailure();
    void setJetsonPowerStateCmdOnReportsOn();
    void setJetsonPowerStateCmdOffShutsDownGracefully();
    void setJetsonPowerStateCmdOffReportsExecutionErrorOnFailure();
    void setJetsonPowerStateCmdOffTreatsSigtermAsSuccess();
    void setPowerModeCmdNotifiesLocalModeChangeStarted();
    void setPowerModeCmdSkipsLocalModeChangeStartedWhenAlreadyInMode();
    void powerModeReceiveDoesNotNotifyLocalModeChangeStarted();

  private:
    void connectPorts();
    void initComponents();

    JetsonPowerModeManager component;
};

}  // namespace scalesSvc

#endif
