module scalesSvc {

  @ Command source currently authorized to forward GDS commands.
  enum CommandAuthority: U8 {
    TCP = 0 @< TCP GDS has command authority
    UART = 1 @< UART GDS has command authority
  }

}
