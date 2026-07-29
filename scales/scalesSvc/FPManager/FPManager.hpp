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
    friend class FPManagerTester;

  public:
    explicit FPManager(const char* const compName);
    ~FPManager() override;

  private:
    static constexpr FwSizeType JETSON_SENSOR_COUNT = 9;
    //! Upper sanity bound on FAULT_DEBOUNCE_COUNT, and the saturation cap for
    //! every fault streak counter. Saturating at a fixed cap (not at the
    //! currently active threshold) means raising the threshold via a live
    //! PRM_SET can never strand an already-saturated streak below the new
    //! threshold.
    static constexpr U32 FAULT_DEBOUNCE_MAX = 1000;
    static constexpr U32 FAULT_DEBOUNCE_DEFAULT = 3;

    //! Independent fault-streak sources. Keyed by INPUT SOURCE, not by
    //! domain: two producers (ImxThermalManager and McpManager sensor 1) both
    //! write the i.MX domain, so a single shared per-domain counter would be
    //! reset by whichever source is currently healthy, potentially masking a
    //! sustained fault from the other. Likewise peripheral has two sources
    //! (only one is wired in the topology today, the other is future-proofing).
    enum FaultSource {
        SRC_IMX_LOCAL = 0,    //!< imxThermalReadingIn (ImxThermalManager, CPU die)
        SRC_IMX_MCP = 1,      //!< mcpThermalReadingIn, sensorId 1 (MCP i.MX/OBC sensor)
        SRC_PERIF_LOCAL = 2,  //!< peripheralThermalReadingIn (not wired in the topology today)
        SRC_PERIF_MCP = 3,    //!< mcpThermalReadingIn, sensorId 2 (MCP peripheral sensor)
        SRC_COUNT = 4
    };

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
    void scalesSvc_FPStateMachine_action_REBOOT(
        SmId smId, scalesSvc_FPStateMachine::Signal signal) override;

    void parameterUpdated(FwPrmIdType id) override;

    bool readingIsFault(const ThermalReading& reading) const;
    bool readingIsWarn(const ThermalReading& reading) const;
    bool findJetsonWarn(ThermalReading& warnReading) const;
    FwOpcodeType extractOpcode(Fw::ComBuffer& data) const;
    void invalidateJetsonReadings();
    void rememberFault(const char* source, const ThermalReading& reading);
    void triggerImxEmergencyShutdown(const ThermalReading& reading);
    void triggerPeripheralEmergencyShutdown(const ThermalReading& reading);
    void triggerPlatformPoweroff();
    void reportReadingFault();
    void writeStateTelemetry();
    void updateWarnTracking(const char* source, bool currentlyWarn,
                             bool& warnActiveFlag, const ThermalReading& reading);

    // ----------------------------------------------------------------------
    // Fault-detection debounce
    // ----------------------------------------------------------------------

    //! Update the cached reading/valid flag/warn-tracking for the i.MX or
    //! peripheral domain and record the fault streak for the source that
    //! produced it.
    void updateImxDomain(FaultSource src, const ThermalReading& reading);
    void updatePeripheralDomain(FaultSource src, const ThermalReading& reading);
    //! Increment (saturating) on a FAULT reading, else reset to zero --
    //! including on an unavailable (NOT_USED) reading, consistent with how an
    //! invalid reading is already treated everywhere else in this component.
    void updateFaultStreak(U32& streak, const ThermalReading& reading);

    //! Streak-gated wrappers around the raw readingIsFault() check above.
    //! The raw check stays in use directly for WARN tracking and the
    //! peripheral recovery gate in faultModeHealthCheck, which are
    //! deliberately NOT debounced -- debounce must make FPManager slower to
    //! act, never slower to stay safe.
    bool imxFaultConfirmed() const;
    bool peripheralFaultConfirmed() const;
    bool jetsonSensorFaultConfirmed(U8 sensorId) const;
    bool findConfirmedJetsonFault(ThermalReading& faultReading) const;

    void applyFaultDebounceCount(U32 candidate);
    void loadDebounceParameterOnFirstTick();

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
    bool m_platformPoweroffTriggered;
    FPManagerState m_lastPublishedState;
    bool m_jetsonFaultSignalPending;
    bool m_imxWarnActive;
    bool m_peripheralWarnActive;
    bool m_jetsonWarnActive;

    U32 m_sourceFaultStreak[SRC_COUNT];
    U32 m_jetsonFaultStreak[JETSON_SENSOR_COUNT];
    FaultSource m_imxLastSource;
    FaultSource m_peripheralLastSource;
    U32 m_activeFaultDebounceCount;
    bool m_justBooted;
    Fw::ParamValid m_paramValid;
};

}  // namespace scalesSvc

#endif
