module SCALES{
    @ Array of input ports for receiving data from multiple components
    port dataInput(datain: StorageData)
    # async input port dataInput: [10] components.StorageData

    @ Array of output ports for sending data to multiple components
    port dataOutput(dataout: StorageData)
    # output port dataOutput: [10] components.StorageData
}