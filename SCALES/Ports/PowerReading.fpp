module SCALES{
    @ Port for receiving power data
    port powerData(
        reading: PowerReading
    )
    # async input port powerData: components.PowerReading
}