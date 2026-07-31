module scalesSvc {
    @ Component that manages the Jetson On/Off state, and interacts with the JetsonPowerModeManager to pass the previous power mode for the next boot cycle
    active component JetsonManager {
        ###############################################################################
        #                                 General Ports                               #
        ###############################################################################

        @ Port for receiving current power mode from JetsonPowerModeManager
        async input port currentPwrMode: PowerModeSend

        @ Notification from JetsonPowerModeManager that a LOCAL (non-hub)
        @ SET_POWER_MODE command is about to reboot the Jetson -- arms the
        @ same hub-link-distrust guard as a hub-driven REQUEST_POWER_MODE.
        @ See JM-014.
        async input port localModeChangeStarted: PowerModeReceive

        @ Port for receiving current Jetson power state from JetsonPowerModeManager
        async input port currentJetsonPwrState: JetsonPowerStateSend

        @ Internal power-off request from FPManager during recovery or shutdown.
        sync input port fpJetsonPowerRequestIn: JetsonPowerStateReceive

        @ Synchronous authorization gate for REQUEST_JETSON_POWER_STATE.
        output port fpJetsonPowerAuthorize: JetsonPowerStateAuthorize

        @ Current Jetson power state report to FPManager.
        output port fpJetsonPowerStateOut: JetsonPowerStateSend

        @ Current hub-link-trust status report to FPManager, republished
        @ every schedIn tick (see isJetsonHubLinkTrusted()) so a GDS session
        @ that connects late still sees an accurate value -- mirrors
        @ FPManager's own FAULT_DEBOUNCE_COUNT telemetry republish pattern
        @ (FPManager.cpp run_handler). i.MX-internal only. See JM-015.
        output port fpJetsonHubTrustedOut: JetsonHubTrustStatus

        @ Port that receives the rate group tick
        sync input port schedIn: Svc.Sched

        @ Port for sending power mode change requests to JetsonPowerModeManager
        output port reqPwrMode: PowerModeReceive

         @ Jetson-side graceful power-state request path for commanded OFF.
        output port reqJetsonPwrState: JetsonPowerStateReceive

          @ Port for driving GPIO to control hardware power
        output port gpioSet: Drv.GpioWrite

        ###############################################################################
        #                                  Commands                                   #
        ###############################################################################

        @ Command to request a power mode change on the Jetson
        async command REQUEST_POWER_MODE(
            mode: PowerModeID @< Requested power mode
        )

        @ Command to request a power state change (on/off) for the Jetson
        async command REQUEST_JETSON_POWER_STATE(
            jetsonState: JetsonPowerStateID @< Requested power state (on/off)
        )

        ###############################################################################
        #                                   Events                                    #
        ###############################################################################

        @ Event indicating a power mode change was requested
        event POWER_MODE_REQUESTED(
            mode: PowerModeID @< The requested power mode
        ) severity activity high id 0 format "JetsonManager requested Jetson power mode {}"

        @ Event indicating the current power mode was received
        event POWER_MODE_RECEIVED(
            mode: PowerModeID @< The current power mode reported by Jetson
        ) severity activity low id 1 format "JetsonManager received Jetson power mode {}"

        @ Event indicating a Jetson power state change was requested
        event JETSON_POWER_STATE_REQUESTED(
            jetsonState: JetsonPowerStateID @< The requested Jetson power state (on/off)
        ) severity activity high id 2 format "JetsonManager requested Jetson power state change to {}"

        @ Event indicating the current Jetson power state was received
        event JETSON_POWER_STATE_RECEIVED(
            jetsonState: JetsonPowerStateID @< The current Jetson power state reported by Jetson
        ) severity activity low id 3 format "JetsonManager received Jetson power state {}"

        @Event indicating a Jetson power commmand timed out
        event JETSON_POWER_STATE_TIMEOUT(
            jetsonState: JetsonPowerStateID @< The Jetson power state that timed out
        ) severity warning high id 4 format "JetsonManager timed out waiting for Jetson power state response to command {}"

        @ Event indicating JetsonManager gave up waiting for the Jetson's
        @ first report after being commanded ON, with no OFF request pending
        event JETSON_BOOT_CONFIRMATION_TIMEOUT severity warning high id 6 \
            format "JetsonManager timed out waiting for the Jetson's first report after being commanded ON"

        @ Event indicating a commanded/internal Jetson OFF request was
        @ deferred because a boot confirmation is still outstanding; it will
        @ be sent automatically once the Jetson's first real report arrives
        @ (or force-completed if that window times out)
        event JETSON_OFF_DEFERRED_BOOTING severity activity high id 7 \
            format "Jetson OFF deferred: Jetson has not finished booting since it was last commanded ON; will auto-fire once boot is confirmed"

        @ Event indicating JetsonManager gave up waiting for the Jetson's
        @ first report while a deferred OFF was pending; the OFF was
        @ force-completed via a direct GPIO cut
        event JETSON_DEFERRED_OFF_FORCED_BY_TIMEOUT severity warning high id 8 \
            format "JetsonManager timed out waiting for the Jetson's boot confirmation; forcing the deferred OFF via direct GPIO cut"

        @ Event indicating a REQUEST_POWER_MODE command was rejected because
        @ the hub link to the Jetson cannot currently be trusted (confirmed
        @ off, never confirmed, still awaiting first boot confirmation, or a
        @ previous mode-change reboot is already in flight) -- unlike Jetson
        @ OFF there is no hardware-safe fallback action for "set power
        @ mode," so the command fails fast rather than risking
        @ reqPwrMode_out() against an unproven hub link or deferring
        @ indefinitely
        event POWER_MODE_REQUEST_REJECTED(
            mode: PowerModeID @< The requested power mode
        ) severity warning high id 9 format "JetsonManager rejected REQUEST_POWER_MODE({}): Jetson/hub link not currently trusted"

        @ Event indicating a commanded/internal Jetson OFF request was
        @ deferred because a REQUEST_POWER_MODE-triggered reboot is
        @ currently in flight; it will be sent automatically once the mode
        @ change is confirmed (or force-resumed if that window times out)
        event JETSON_OFF_DEFERRED_MODE_CHANGE_PENDING severity activity high id 10 \
            format "Jetson OFF deferred: a power-mode-change reboot is in flight; will auto-fire once the Jetson reconfirms over the hub or the window times out"

        @ Event indicating JetsonManager gave up waiting for the Jetson's
        @ power-mode change to be confirmed while a deferred OFF request was
        @ also pending; the deferred OFF is now resumed (graceful hub
        @ request if the Jetson is still confirmed on, otherwise a direct
        @ GPIO cut)
        event JETSON_DEFERRED_OFF_RESUMED_AFTER_MODE_TIMEOUT severity warning high id 11 \
            format "JetsonManager timed out waiting for the Jetson's power-mode confirmation; resuming the deferred OFF request"

        @ Event indicating JetsonManager received a local-mode-change-started
        @ notification from JetsonPowerModeManager (a SET_POWER_MODE command
        @ was issued directly against the Jetson, bypassing
        @ JetsonManager/FPManager entirely) and armed the same hub-link-
        @ distrust guard it uses for a hub-driven REQUEST_POWER_MODE
        event LOCAL_MODE_CHANGE_STARTED_RECEIVED(
            mode: PowerModeID @< The power mode nvpmodel is applying locally
        ) severity activity high id 12 format "JetsonManager received local mode-change-started notification for mode {}; hub link not trusted until the reboot completes"

        ###############################################################################
        #                                 Telemetry                                   #
        ###############################################################################

        @ Current power mode of the Jetson as reported by JetsonPowerModeManager
        telemetry JetsonPowerMode: PowerModeID

        @ Current power state of the Jetson
        telemetry JetsonPowerState: JetsonPowerStateID

        ###############################################################################
        #                                 Parameters                                  #
        ###############################################################################

        @ Number of ticks to turn off Jetson power after requesting a power off command
        param JETSON_POWER_OFF_DELAY_TICKS: U32 \
            default 15 \
            id 0x12 \
            set opcode 0x13 \
            save opcode 0x14

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}
