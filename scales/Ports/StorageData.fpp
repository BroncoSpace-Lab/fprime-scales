module scalesSvc{
    @ Array of input ports for receiving data from multiple components
    port DataInput(datain: StorageData)

    @ Array of output ports for sending data to multiple components
    port DataOutput(dataout: StorageData)
}