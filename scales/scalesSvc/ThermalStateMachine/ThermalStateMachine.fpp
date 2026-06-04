module scalesSvc {
    @ Define the ThermalStateMachine component which manages the thermal states of the system based on temperature readings
    state machine ThermalStateMachine {
        
        # Initial state
        @ Initial state: reset the device when on boot up, when temperature readings are not avilabl, or when the system is in a fault state
        initial enter RESET


        # Signal to change state or do actions
        @ Rate-group driven tick signal
        signal tick

        @ Current state passed successfully
        signal success

        @ Current state erred
        signal error

        @ Idle state signal
        signal idle

        @ Warning state signal
        signal warn

        @ Fault state signal
        signal fault

        # defined actions for each state
        action doReset

        action doReadTemp

        action doIdle

        action doWarning

        action doFault

        # State definitions and behaviorss
        @ State: Reset Device
        state RESET {
            on success enter READ_TEMP
            on tick do { doReset }

        }

        @ State: Read temperature data and determine thermal state
        state READ_TEMP {
            on idle enter IDLE 
            on warn enter WARNING
            on fault enter FAULT
            on error enter RESET
            on tick do { doReadTemp }
        }

        @ State: Idle state when temperatures are within normal operating range
        state IDLE {
            on success enter READ_TEMP
            on error enter RESET
            on tick do { doIdle }
        }

        @ State: Warning state when temperatures are approaching critical thresholds
        state WARNING {
            on success enter READ_TEMP
            on error enter RESET
            on tick do { doWarning }
        }
        @ State: Fault state when temperatures exceed critical thresholds
        state FAULT {
            on error enter RESET
            on tick do { doFault }
        }


    }
}