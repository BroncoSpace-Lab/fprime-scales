module scalesSvc {
    @ State machine for selecting the active GDS command authority.
    state machine GdsMuxStateMachine {

        @ Start with TCP GDS as the command authority.
        initial enter init

        ##########################################################################
        # Signals for the GdsCmdAuthMux State Machine
        ##########################################################################

        @ Rate group driven signal.
        signal tick

        @ TCP GDS is up.
        signal tcp_gds_up

        @ TCP GDS is down.
        signal tcp_gds_down

        @ User command to switch authority back to TCP after TCP is stable.
        signal tcp_auth_set

        @ Current action completed successfully.
        signal success

        @ Current action failed.
        signal failure

        #########################################################################
        # Actions for the GdsCmdAuthMux State Machine
        #########################################################################

        @ Initialize TCP GDS as the command authority.
        action tcp_init

        @ Run TCP authority behavior and gate non-TCP command paths.
        action tcp_run

        @ Run UART authority behavior and gate non-UART command paths.
        action uart_run

        @ Start the TCP-down grace timer.
        action start_tcp_down_grace

        @ Monitor the TCP-down grace timer.
        action monitor_tcp_down_grace

        @ Switch command authority to UART and emit CommandAuthoritySwitchedToUart.
        action switch_to_uart

        @ Emit TcpGdsRecovered.
        action emit_tcp_recovered

        @ Start the TCP stable timer.
        action start_tcp_stable_timer

        @ Monitor the TCP stable timer.
        action monitor_tcp_stable_timer

        @ Mark TCP stable and ready for user-selected authority.
        action mark_tcp_ready

        @ Switch command authority to TCP and emit CommandAuthoritySwitchedToTcp.
        action switch_to_tcp

        @ Evaluate failure conditions and handle errors.
        action failure_eval

        ########################################################################
        # State Machine States for the GdsCmdAuthMux State Machine
        ########################################################################

        @ Error state to handle failures and transition back to init.
        state error {
            on tick do {failure_eval}
            on success enter init
            on failure enter error
        }

        @ Init state for the state machine.
        state init {
            on tick do {tcp_init}
            on success enter tcp_gds_cmd_authority
            on failure enter error
        }

        @ TCP GDS has authority; keep it unless TCP remains down past the grace period.
        state tcp_gds_cmd_authority {
            on tick do {tcp_run}
            on tcp_gds_down do {start_tcp_down_grace} enter tcp_down_grace
            on failure enter error
        }

        @ TCP is down, but TCP keeps authority for the configured grace period.
        state tcp_down_grace {
            on tick do {monitor_tcp_down_grace}
            on tcp_gds_up enter tcp_gds_cmd_authority
            on success do {switch_to_uart} enter uart_gds_cmd_authority
            on failure enter error
        }

        @ UART GDS has authority; monitor TCP recovery while gating TCP commands.
        state uart_gds_cmd_authority {
            on tick do {uart_run}
            on tcp_gds_up do {
                emit_tcp_recovered
                start_tcp_stable_timer
            } enter tcp_stable_wait
            on failure enter error
        }

        @ TCP has recovered; wait for it to remain stable before allowing manual return.
        state tcp_stable_wait {
            on tick do {monitor_tcp_stable_timer}
            on tcp_gds_down enter uart_gds_cmd_authority
            on success do {mark_tcp_ready} enter uart_gds_tcp_ready
            on failure enter error
        }

        @ UART still has authority, but TCP is stable and can be selected by command.
        state uart_gds_tcp_ready {
            on tick do {uart_run}
            on tcp_gds_down enter uart_gds_cmd_authority
            on tcp_auth_set do {switch_to_tcp} enter tcp_gds_cmd_authority
            on failure enter error
        }

    }
}
