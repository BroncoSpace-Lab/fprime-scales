    module scalesSvc {
        state machine ThermalStateMachine {

            initial enter INIT                  # Start the state machine

            signal tick
            signal fail
            signal success

            action doRead
            action paramEvaluate
            action readFail

            state INIT {
                on tick do {doRead}             # Read the thermal data from the device
                on success enter EVALUATE          # Next step
                on fail enter FAIL              # couldnt evaluate for some reason
            }
            
            state EVALUATE {
                on tick do {paramEvaluate}      # Evaluate the thermal data against thresholds
                on success enter INIT              # Loop back to the beginning
                on fail enter FAIL              # read error from device
            }

            state FAIL {
                on tick do {readFail}           # Log a read failure
                on success enter INIT              # Loop back to the beginning
            }
        

        }
    }