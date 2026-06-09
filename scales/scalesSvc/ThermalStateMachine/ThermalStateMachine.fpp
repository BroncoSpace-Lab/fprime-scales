module scalesSvc {
    state machine ThermalStateMachine {

        @ Enter Initial state on component startup
        initial enter INIT                  # Start the state machine

        @ Tick signal driven by a rate group
        signal tick
        
        @ Fail tick to transition to FAIL state
        signal fail

        @ Success signal to transition to next state
        signal success

        @ Read the temp values from the device
        action doRead

        @ Evaluate the temp values against thresholds and update telemetry
        action doEvaluate

        @ Log a read failure event
        action doReadFail

        state INIT {
            on tick do {doRead}             # Read the thermal data from the device
            on success enter EVALUATE          # Next step
            on fail enter FAIL              # couldnt evaluate for some reason
        }
        
        state EVALUATE {
            on tick do {doEvaluate}      # Evaluate the thermal data against thresholds
            on success enter INIT              # Loop back to the beginning
            on fail enter FAIL              # read error from device
        }

        state FAIL {
            on tick do {doReadFail}           # Log a read failure
            on success enter INIT              # Loop back to the beginning
        }
    

    }
}