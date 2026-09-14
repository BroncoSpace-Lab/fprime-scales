module scalesSvc{
    
    @ Subsytem of which INA260 is a part of.
    enum Subsytem{
        JETSON @< Jetson subsystem
        OBC @< OBC subsystem
        PERIPHERAL @< Peripheral subsystem
    }

    @ All average modes on the INA260 sensor
    enum AverageMode{
        _1_ @< 1 sample average
        _4_ @< 4 sample average
        _16_ @< 16 sample average
        _64_ @< 64 sample average
        _256_ @< 256 sample average
        _1024_ @< 1024 sample average
    }
}