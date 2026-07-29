module scalesSvc {
    @ State machine for selecting the active flight processor.
    state machine FPStateMachine {

        @ Start in initialization and enter Safe Mode after the first tick.
        initial enter init

        ##########################################################################
        # Signals for the FP State Machine
        ##########################################################################

        @ Rate group driven signal.
        signal tick

        @ Signal to represent a healthy check.
        signal healthy

        @ Current action completed successfully.
        signal success

        @ Current action failed.
        signal failure

        @ Fatal error signal
        signal $fatal

        @ Signal to switch to HPC Mode
        signal hpcMode_en

        @ Signal to leave HPC Mode and return to Safe Mode.
        signal hpcMode_dis

        @ Signal that the Jetson has a thermal or power fault while in HPC Mode.
        signal jetson_fault

        @ Another component in the topology announced a FATAL-severity event.
        @ Unlike $fatal this is not a platform-power condition: the flight
        @ software process restarts, the i.MX stays on.
        signal component_fatal


        ##########################################################################
        # Actions for the FP State Machine
        ##########################################################################

        @ Initialize the outputs and enforce the Safe Mode power configuration.
        action initializeSafeMode

        @ Check i.MX and peripheral health. i.MX FAULT is system fatal;
        @ peripheral FAULT powers off the peripheral board only.
        action safeModeHealthCheck

        @ Check i.MX, peripheral, Jetson, and power health in HPC Mode.
        @ i.MX FAULT is system fatal; peripheral and Jetson faults are isolated.
        action hpcModeHealthCheck

        @ Enable HPC Mode and permit Jetson power-on requests.
        action enableHpcMode

        @ Disable HPC Mode, power off the Jetson, and return to Safe Mode.
        action disableHpcMode

        @ Confirm the Jetson fault, report its source, and power it off.
        action confirmJetsonFaultAndPowerOff

        @ Report the component and sensor responsible for a non-recoverable fault.
        action reportFault

        @ Recheck thermal health while recovering from a peripheral fault.
        action faultModeHealthCheck

        @ Emergency Shutdown: emit the fatal warning and power off protected outputs.
        action SHUTDOWN

        @ Emergency Reboot: latch the FSW-restart state and publish it. Unlike
        @ SHUTDOWN this asserts no hardware output and cuts no power -- the
        @ process is about to be aborted by Svc.FatalHandler and respawned by
        @ systemd (Restart=on-failure).
        action REBOOT


        ##########################################################################
        # States for the FP State Machine
        ##########################################################################

        @ Initialization state. The first tick establishes Safe Mode.
        state init {
            on tick do {initializeSafeMode} enter safeMode
            on $fatal do {SHUTDOWN} enter emergencyShutdown
            on component_fatal do {REBOOT} enter emergencyReboot
        }

        @ Safe Mode. The Jetson must remain powered off.
        state safeMode {
            on tick do {safeModeHealthCheck}
            on failure do {reportFault} enter faultMode
            on hpcMode_en do {enableHpcMode} enter hpcMode
            on $fatal do {SHUTDOWN} enter emergencyShutdown
            on component_fatal do {REBOOT} enter emergencyReboot
        }

        @ HPC Mode. All thermal and power sources are checked.
        state hpcMode {
            on tick do {hpcModeHealthCheck}
            on hpcMode_dis do {disableHpcMode} enter safeMode
            on jetson_fault enter jetsonFaultRecovery
            on failure do {reportFault} enter faultMode
            on $fatal do {SHUTDOWN} enter emergencyShutdown
            on component_fatal do {REBOOT} enter emergencyReboot
        }

        @ Recover from a Jetson-only fault without treating the emergency power-off as fatal.
        state jetsonFaultRecovery {
            on tick do {confirmJetsonFaultAndPowerOff}
            on success enter safeMode
            on failure do {reportFault} enter faultMode
            on $fatal do {SHUTDOWN} enter emergencyShutdown
            on component_fatal do {REBOOT} enter emergencyReboot
        }

        @ Fault Mode for non-system-fatal faults outside the Jetson-only recovery path.
        state faultMode {
            on tick do {faultModeHealthCheck}
            on healthy enter safeMode
            on $fatal do {SHUTDOWN} enter emergencyShutdown
            on component_fatal do {REBOOT} enter emergencyReboot
        }

        @ Latched emergency state. SHUTDOWN is a one-shot action; no recovery transition exists.
        state emergencyShutdown {
        }

        @ Latched FSW-restart state. REBOOT is a one-shot action; no recovery
        @ transition exists within this process lifetime -- Svc.FatalHandler
        @ aborts the process moments later and systemd respawns it, which
        @ reconstructs FPManager fresh in `init`.
        state emergencyReboot {
        }

    }
}
