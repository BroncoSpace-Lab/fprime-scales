// ======================================================================
// \title  FPManager.hpp
// \brief  Fault Protection Manager for the SCALES system.
// ======================================================================

#ifndef scalesSvc_FPManager_HPP
#define scalesSvc_FPManager_HPP

#include "Fw/Types/String.hpp"
#include "scales/scalesSvc/FPManager/FPManagerComponentAc.hpp"

namespace scalesSvc {

class FPManager final : public FPManagerComponentBase {
  public:
    explicit FPManager(const char* const compName);
    ~FPManager() override;

  private:
    static constexpr FwSizeType JETSON_SENSOR_COUNT = 9;

    void run_handler(FwIndexType portNum, U32 context) override;
    void fatalIn_handler(FwIndexType portNum, FwEventIdType Id) override;
    void imxThermalReadingIn_handler(FwIndexType portNum,
                                     const ThermalReading& reading) override;
    void peripheralThermalReadingIn_handler(FwIndexType portNum,
                                             const ThermalReading& reading) override;
    void mcpThermalReadingIn_handler(FwIndexType portNum,
                                     const ThermalReading& reading) override;
    void jetsonThermalReadingIn_handler(FwIndexType portNum,
                                        const ThermalReading& reading) override;
    void remoteJetsonCmdIn_handler(FwIndexType portNum,
                                   Fw::ComBuffer& data,
                                   U32 context) override;
    void remoteJetsonCmdResponseIn_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) override;
    Fw::Success jetsonPowerAuthorizeIn_handler(
        FwIndexType portNum, const JetsonPowerStateID& stateReq) override;
    void jetsonPowerStateIn_handler(FwIndexType portNum,
                                    const JetsonPowerStateID& stateNow) override;

    void ENABLE_HPC_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void DISABLE_HPC_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void scalesSvc_FPStateMachine_action_initializeSafeMode(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_safeModeHealthCheck(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_hpcModeHealthCheck(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_enableHpcMode(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_disableHpcMode(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_confirmJetsonFaultAndPowerOff(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_reportFault(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_faultModeHealthCheck(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;
    void scalesSvc_FPStateMachine_action_SHUTDOWN(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;

    bool readingIsFault(const ThermalReading& reading) const;
    bool findJetsonFault(ThermalReading& faultReading) const;
    FwOpcodeType extractOpcode(Fw::ComBuffer& data) const;
    void invalidateJetsonReadings();
    void rememberFault(const char* source, const ThermalReading& reading);
    void triggerImxEmergencyShutdown(const ThermalReading& reading);
    void triggerPeripheralEmergencyShutdown(const ThermalReading& reading);
    void reportReadingFault();
    void writeStateTelemetry();

    FPManagerState m_mode;
    ThermalReading m_imxReading;
    ThermalReading m_peripheralReading;
    ThermalReading m_jetsonReadings[JETSON_SENSOR_COUNT];
    bool m_imxReadingValid;
    bool m_peripheralReadingValid;
    bool m_jetsonReadingValid[JETSON_SENSOR_COUNT];
    JetsonPowerStateID m_jetsonPowerState;
    ThermalReading m_faultReading;
    Fw::String m_faultSource;
    bool m_hasFault;
    bool m_safeModeHealthy;
    bool m_shutdownOutputsAsserted;
    FPManagerState m_lastPublishedState;
    bool m_jetsonFaultSignalPending;
};

}  // namespace scalesSvc

#endif
